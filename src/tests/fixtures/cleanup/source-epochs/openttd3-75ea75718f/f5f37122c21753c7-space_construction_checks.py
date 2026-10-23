"""Verify paid first-frame deliveries, retained cancellation and exact native job replay."""
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

EXACT = (b'SPST', b'SPKN', b'SPDJ', b'SPFL', b'SPOD', b'SPSU', b'SVST', b'LCOP', b'SDEF', b'SPOR', b'ASTR', b'VEHS', b'CAPA', b'PLYR', b'STNN', b'LCTR', b'LCHB', b'LHND')
KINDS = ('missing', 'count', 'generation', 'capital', 'strip', 'work', 'consumption', 'quantity', 'duplicate-packet', 'manifest-packet', 'future-target', 'old-version')

def native(context, configuration, action, source, corrupt=None, reject=False):
    context.prepare_runtime()
    (context.runtime_dir / 'scripts/game_start.scr').write_text('space_construction_test\n', encoding='utf-8')
    env = dict(os.environ, OTTD_SPACE_CONSTRUCTION_ACTION=action, OTTD_SPACE_CONSTRUCTION_SOURCE=str(source))
    env.pop('OTTD_SPACE_CONSTRUCTION_CORRUPT', None)
    if corrupt:
        env['OTTD_SPACE_CONSTRUCTION_CORRUPT'] = str(corrupt)
    log = context.logs / 'native-construction.log'
    with log.open('wb') as sink:
        result = run_native(context, [str(ROOT / 'build' / configuration / 'openttd_test.exe'), '[.space-construction]'], cwd=ROOT / 'build', env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=240)
    evidence = log.read_text(encoding='utf-8', errors='replace')
    if reject:
        assert result.returncode != 0 and 'All tests passed' not in evidence, log
        assert any(word in evidence.lower() for word in ('construction', 'space fleet', 'space order')), log
        return
    assert result.returncode == 0 and 'All tests passed' in evidence and 'SPACE_CONSTRUCTION' in evidence, log
    if corrupt:
        assert 'preview_reject=true authority_unchanged=true' in evidence, log
        return
    for ppm in context.images.glob('*.ppm'):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix('.png'))
    return context.saves / ('construction-unowned-415.sav' if action == 'ownership' else 'construction-cancel-complete-415.sav' if action.startswith('cancel') else 'construction-frame-415.sav')

def damage(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 415
    changed = bytearray(stream)
    start, body, _, end = riff_location(stream, b'SPST')
    row = body + 10
    if kind == 'missing':
        del changed[start:end]
    elif kind == 'count':
        changed[body + 8:body + 10] = struct.pack('>H', 1025)
    elif kind == 'generation':
        changed[row + 2:row + 10] = bytes(8)
    elif kind == 'capital':
        changed[row + 19:row + 23] = bytes(4)
    elif kind == 'strip':
        _, asteroid, _, _ = riff_location(stream, b'ASTR')
        height = struct.unpack_from('>I', stream, asteroid + 6)[0]
        changed[row + 15:row + 17] = struct.pack('>H', height // 2)
    elif kind == 'work':
        changed[row + 23:row + 27] = struct.pack('>I', 740)
    elif kind == 'consumption':
        changed[row + 29:row + 31] = struct.pack('>H', 16)
    elif kind == 'quantity':
        changed[row + 31:row + 33] = struct.pack('>H', 15)
    elif kind == 'duplicate-packet':
        count = struct.unpack_from('>H', stream, row + 33)[0]
        second = row + 35 + 6 * count
        assert count and struct.unpack_from('>H', stream, second + 4)[0]
        changed[second + 6:second + 10] = stream[row + 35:row + 39]
    elif kind == 'manifest-packet':
        _, manifests, _, _ = riff_location(stream, b'SPSU')
        pos, packet = manifests + 1, None
        for _ in range(stream[manifests]):
            pos += 61 + 10 * stream[pos + 60]
            for _ in range(3):
                count = stream[pos]
                if count:
                    packet = stream[pos + 1:pos + 5]
                pos += 1 + count * 6
        assert packet is not None
        changed[row + 35:row + 39] = packet
    elif kind == 'future-target':
        _, orders, _, _ = riff_location(stream, b'SPOR')
        pos = orders + 9
        for _ in range(stream[orders + 8]):
            count = stream[pos + 21]
            if count and stream[pos + 22] == 3:
                changed[pos + 28:pos + 36] = struct.pack('>Q', 9999)
                break
            pos += 22 + count * 14
        else:
            raise AssertionError('missing builder Construct order')
    elif kind != 'old-version':
        raise ValueError(kind)
    target = context.saves / f'construction-{kind}.sav'
    target.write_bytes(b'OTTN' + struct.pack('>I', (414 if kind == 'old-version' else version) << 16) + changed)
    return target

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', default='RelWithDebInfo', choices=('RelWithDebInfo', 'Debug'))
    parser.add_argument('--source', type=Path, help='Existing genuine paid logistics bootstrap (immutable input)')
    args = parser.parse_args()
    context = current_run_context('space', 'construction-verify')
    if args.source:
        source = args.source.resolve()
        assert source.is_relative_to(ROOT)
    else:
        ground = context.for_feature('space', 'construction-ground-production')
        logistics_native(ground, args.configuration, 'prepare')
        source = ground.saves / 'logistics-bootstrap-414.sav'  # Producer's retained filename; header is authoritative.
    source_identity = identity(source)
    before_version, before = chunks(source)
    prepare = context.for_feature('space', 'construction-prepare')
    complete = native(prepare, args.configuration, 'prepare', source)
    expected = chunks(complete)[1]
    evidence = []
    for phase, action, name in (('delivery', 'delivery', 'construction-delivery-415.sav'), ('work', 'continue', 'construction-work-415.sav')):
        partial = prepare.saves / name
        for replay in ('resume', 'replay'):
            result = native(context.for_feature('space', f'construction-{replay}-{phase}'), args.configuration, action, partial)
            for key in EXACT:
                assert expected[key] == chunks(result)[1][key], ('construction continuation', phase, key)
            evidence.append(identity(result))
        owner = context.for_feature('space', 'construction-ordinary-' + phase)
        ordinary(owner, {'name': phase, 'snapshot': identity(partial)}, args.configuration)
        for key in EXACT:
            assert chunks(partial)[1][key] == chunks(owner.saves / 'ordinary-resave.sav')[1][key], ('ordinary construction', phase, key)
    cancel = context.for_feature('space', 'construction-cancel')
    cancelled = native(cancel, args.configuration, 'cancel', prepare.saves / 'construction-delivery-415.sav')
    resumed = native(context.for_feature('space', 'construction-cancel-resume'), args.configuration, 'cancel-continue', cancel.saves / 'construction-cancelled-415.sav')
    for key in EXACT:
        assert chunks(cancelled)[1][key] == chunks(resumed)[1][key], ('cancel continuation', key)
    cancel_work = context.for_feature('space', 'construction-cancel-work')
    cancelled_work = native(cancel_work, args.configuration, 'cancel', prepare.saves / 'construction-work-415.sav')
    resumed_work = native(context.for_feature('space', 'construction-cancel-work-resume'), args.configuration, 'cancel-continue', cancel_work.saves / 'construction-cancelled-415.sav')
    for key in EXACT:
        assert chunks(cancelled_work)[1][key] == chunks(resumed_work)[1][key], ('partial-work cancel continuation', key)
    unowned = native(context.for_feature('space', 'construction-ownership'), args.configuration, 'ownership', prepare.saves / 'construction-delivery-415.sav')
    unowned_check = context.for_feature('space', 'construction-unowned-reload')
    ordinary(unowned_check, {'name': 'unowned', 'snapshot': identity(unowned)}, args.configuration)
    assert chunks(unowned)[1][b'SPST'] == chunks(unowned_check.saves / 'ordinary-resave.sav')[1][b'SPST']
    for kind in KINDS:
        owner = context.for_feature('space', 'construction-corrupt-' + kind)
        corrupt = damage(owner, prepare.saves / 'construction-work-415.sav', kind)
        native(context.for_feature('space', 'construction-reject-' + kind), args.configuration, 'continue', corrupt, reject=True)
        native(context.for_feature('space', 'construction-preview-' + kind), args.configuration, 'continue', prepare.saves / 'construction-work-415.sav', corrupt=corrupt)
    migration = context.for_feature('space', 'construction-old-empty')
    ordinary(migration, {'name': 'old-empty', 'snapshot': source_identity}, args.configuration)
    version, migrated = chunks(migration.saves / 'ordinary-resave.sav')
    assert version == 415
    for key in EXACT:
        if key != b'SPST':
            assert before[key] == migrated[key], ('old-empty migration', key)
    assert identity(source) == source_identity
    report = context.reports / 'space-construction-verification.json'
    report.write_text(json.dumps({'configuration': args.configuration, 'source_version': before_version, 'version': 415, 'input': source_identity,
        'completed_frame': identity(complete), 'cancelled_collected': identity(cancelled), 'replays': evidence,
        'corruptions_full_and_preview': list(KINDS), 'exact_chunks': [key.decode() for key in EXACT], 'operational_core': False}, indent=2) + '\n', encoding='utf-8')
    print(f'Construction frame ownership verified: {report.relative_to(ROOT)}', flush=True)

if __name__ == '__main__':
    main()
