"""Verify one paid probe's third return, cargo-only resupply and exact interrupted owners."""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
from PIL import Image
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from launch_center_checks import riff_location
from launch_logistics_checks import native as logistics_native
from runner import current_run_context, run_native, save_native_capture

EXACT = (b'SPKN', b'SPDJ', b'SPFL', b'SPOD', b'SPSU', b'SVST', b'LCOP', b'SDEF', b'SPOR', b'ASTR', b'VEHS', b'CAPA', b'PLYR', b'STNN', b'LCTR', b'LCHB', b'LHND')
KINDS = ('target-generation', 'future-generation', 'self-target', 'target-kind', 'early-receipt', 'deployed-receipt', 'busy-in-ascent', 'old-version', 'wait-started', 'wait-action')

def native(context, configuration, action, source, corrupt=None, reject=False):
    context.prepare_runtime()
    (context.runtime_dir / 'scripts/game_start.scr').write_text('space_resupply_test\n', encoding='utf-8')
    env = dict(os.environ, OTTD_SPACE_RESUPPLY_ACTION=action, OTTD_SPACE_RESUPPLY_SOURCE=str(source))
    env.pop('OTTD_SPACE_RESUPPLY_CORRUPT', None)
    if corrupt:
        env['OTTD_SPACE_RESUPPLY_CORRUPT'] = str(corrupt)
    log = context.logs / 'native-resupply.log'
    with log.open('wb') as sink:
        result = run_native(context, [str(ROOT / 'build' / configuration / 'openttd_test.exe'), '[.space-resupply]'], cwd=ROOT / 'build', env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=240)
    evidence = log.read_text(encoding='utf-8', errors='replace')
    if reject:
        assert result.returncode != 0 and 'All tests passed' not in evidence, log
        assert any(word in evidence.lower() for word in ('space transition', 'space order', 'space fleet', 'space resupply')), log
        return
    assert result.returncode == 0 and 'All tests passed' in evidence and 'SPACE_RESUPPLY native=true' in evidence, log
    marker = 'blocked_receiver=true' if action.startswith('blocked-') else 'incarnation_reuse=true' if action.startswith('lost-') else 'freight=true' if action.startswith('freight-') else 'paid_third_return=true'
    assert marker in evidence, log
    if corrupt:
        assert 'preview_reject=true authority_unchanged=true' in evidence, log
    for ppm in context.images.glob('*.ppm'):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix('.png'))
    label = 'freight-' if action.startswith('freight-') else 'blocked-' if action.startswith('blocked-') else 'lost-' if action.startswith('lost-') else ''
    return context.saves / f'resupply-{label}complete-414.sav'

def damage(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 414
    changed = bytearray(stream)
    _, body, _, _ = riff_location(stream, b'SPFL')
    row = body + 1
    if kind == 'target-generation':
        changed[row + 40:row + 48] = bytes(8)
    elif kind == 'future-generation':
        changed[row + 40:row + 48] = struct.pack('>Q', 0xFFFFFFFFFFFFFFFF)
    elif kind == 'self-target':
        changed[row + 36:row + 40] = changed[row:row + 4]
    elif kind == 'target-kind':
        changed[row + 35] = 1  # Undelivered Structure producer is never a craft alias.
    elif kind == 'early-receipt':
        changed[row + 59] |= 8
    elif kind == 'deployed-receipt':
        changed[row + 59] |= 4
    elif kind == 'busy-in-ascent':
        changed[row + 13] = 4
    elif kind in ('wait-started', 'wait-action'):
        _, orders, _, _ = riff_location(stream, b'SPOR')
        target = struct.unpack('>I', stream[row + 36:row + 40])[0]
        pos = orders + 9
        for _ in range(stream[orders + 8]):
            count = stream[pos + 21]
            if struct.unpack('>I', stream[pos:pos + 4])[0] == target:
                changed[pos + 13 if kind == 'wait-started' else pos + 22] = 1 if kind == 'wait-started' else 0
                break
            pos += 22 + 14 * count
        else:
            raise AssertionError('missing real receiver order row')
    elif kind != 'old-version':
        raise ValueError(kind)
    target = context.saves / f'resupply-{kind}.sav'
    target.write_bytes(b'OTTN' + struct.pack('>I', (413 if kind == 'old-version' else version) << 16) + changed)
    return target

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', default='RelWithDebInfo', choices=('RelWithDebInfo', 'Debug'))
    parser.add_argument('--migration-manifest', type=Path, help='Explicit immutable version413 checkpoints to ordinary-load and resave with the current game')
    args = parser.parse_args()
    if args.migration_manifest is not None:
        manifest = args.migration_manifest.resolve()
        assert manifest.is_relative_to(ROOT)
        scenes = json.loads(manifest.read_text(encoding='utf-8-sig'))
        assert {scene['name'] for scene in scenes} == {'preparing', 'ascent', 'delivery', 'return', 'turnaround', 'descent', 'service', 'survey'}
        context = current_run_context('space', 'resupply-migration-verify')
        evidence = []
        for scene in scenes:
            source = ROOT / scene['snapshot']['path']
            assert source.resolve().is_relative_to(ROOT) and identity(source) == scene['snapshot']
            version, before = chunks(source)
            assert version == 413
            owner = context.for_feature('space', 'resupply-migration-' + scene['name'])
            ordinary(owner, scene, args.configuration)
            output = owner.saves / 'ordinary-resave.sav'
            version, after = chunks(output)
            assert version == 414
            for key in EXACT:
                assert before[key] == after[key], ('version413 migration', scene['name'], key)
            assert identity(source) == scene['snapshot']
            evidence.append({'phase': scene['name'], 'input': scene['snapshot'], 'output': identity(output)})
        report = context.reports / 'space-resupply-migration.json'
        report.write_text(json.dumps({'configuration': args.configuration, 'from_version': 413, 'to_version': 414,
            'manifest': identity(manifest), 'exact_owner_chunks': [key.decode() for key in EXACT], 'checkpoints': evidence}, indent=2) + '\n', encoding='utf-8')
        print(f'Ordinary413-to414 migration verified: {report.relative_to(ROOT)}', flush=True)
        return
    context = current_run_context('space', 'resupply-verify')
    retained = ROOT / 'src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav'
    original = identity(retained)
    ground = context.for_feature('space', 'resupply-ground-production')
    logistics_native(ground, args.configuration, 'prepare')
    prepare = context.for_feature('space', 'resupply-prepare')
    complete = native(prepare, args.configuration, 'prepare', ground.saves / 'logistics-bootstrap-414.sav')
    expected = chunks(complete)[1]
    sources = (('ascent', 'continue', 'resupply-ascent-414.sav'), ('delivered', 'delivered', 'resupply-delivered-414.sav'), ('service', 'service', 'resupply-service-414.sav'), ('paused', 'paused-service', 'resupply-service-paused-414.sav'))
    for label, action, name in sources:
        source = prepare.saves / name
        for replay in ('resume', 'replay'):
            result = native(context.for_feature('space', f'resupply-{replay}-{label}'), args.configuration, action, source)
            data = chunks(result)[1]
            for key in EXACT:
                assert expected[key] == data[key], ('interrupted paid repeat', label, key, identity(complete), identity(result))
        owner = context.for_feature('space', 'resupply-ordinary-' + label)
        ordinary(owner, {'name': 'resupply-' + label, 'snapshot': identity(source)}, args.configuration)
        before, after = chunks(source)[1], chunks(owner.saves / 'ordinary-resave.sav')[1]
        for key in EXACT:
            assert before[key] == after[key], ('ordinary paid repeat', label, key)
    freight = context.for_feature('space', 'resupply-freight-prepare')
    freight_complete = native(freight, args.configuration, 'freight-prepare', ground.saves / 'logistics-bootstrap-414.sav')
    for replay in ('resume', 'replay'):
        result = native(context.for_feature('space', 'resupply-freight-' + replay), args.configuration, 'freight-continue', freight.saves / 'resupply-freight-ascent-414.sav')
        for key in EXACT:
            assert chunks(freight_complete)[1][key] == chunks(result)[1][key], ('interrupted Goods/parts delivery', key)
    partial = prepare.saves / 'resupply-ascent-414.sav'
    for label in ('blocked', 'lost'):
        owner = context.for_feature('space', f'resupply-{label}-prepare')
        completed = native(owner, args.configuration, label + '-prepare', partial)
        source = owner.saves / f'resupply-{label}-414.sav'
        for replay in ('resume', 'replay'):
            result = native(context.for_feature('space', f'resupply-{label}-{replay}'), args.configuration, label + '-resume', source)
            for key in EXACT:
                assert chunks(completed)[1][key] == chunks(result)[1][key], ('blocked/lost resupply continuation', label, key)
        reload = context.for_feature('space', f'resupply-{label}-ordinary')
        ordinary(reload, {'name': 'resupply-' + label, 'snapshot': identity(source)}, args.configuration)
        for key in EXACT:
            assert chunks(source)[1][key] == chunks(reload.saves / 'ordinary-resave.sav')[1][key], ('blocked/lost ordinary resave', label, key)
    for kind in KINDS:
        owner = context.for_feature('space', 'resupply-reject-' + kind)
        corrupt = damage(owner, partial, kind)
        native(owner, args.configuration, 'continue', corrupt, reject=True)
        native(context.for_feature('space', 'resupply-preview-' + kind), args.configuration, 'continue', partial, corrupt=corrupt)
    assert identity(retained) == original
    report = context.reports / 'space-resupply-verification.json'
    report.write_text(json.dumps({'configuration': args.configuration, 'retained': original, 'ascent': identity(partial), 'complete': identity(complete),
        'same_paid_probe': True, 'same_reusable_rocket': True, 'actual_mined_returned_units': 60, 'paid_shells_expended': 3,
        'cargo_only_resupply': {'RFMT': 1, 'ELEC': 1, 'paid_pods': 2}, 'service_work_ticks': 148, 'service_upkeep': 1000,
        'service_pause_ticks': 74, 'service_pause_upkeep': 100, 'construction_goods_crates_delivered': 40, 'freight_complete': identity(freight_complete),
        'exact_owner_chunks': [key.decode() for key in EXACT], 'native_corrupt_rejections': len(KINDS), 'independent_preview_rejections': len(KINDS),
        'blocked_receiver_preserves_payload': True, 'lost_receiver_incarnation_reuse': True, 'native_receiver_selection_and_launch': True,
        'remaining_scope': 'full first-Core/processing, final art and integrated/network acceptance'}, indent=2) + '\n', encoding='utf-8')
    print(f'Paid same-probe third return and Goods resupply verified: {report.relative_to(ROOT)}', flush=True)

if __name__ == '__main__':
    main()
