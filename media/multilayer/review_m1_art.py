"""Build the selected M1 gallery from verified native renderer evidence.

@file review_m1_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group85B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.8
Original frame/source bytes remain immutable; the contact sheet only arranges
existing native pixels with captions. Technical review never implies user approval.
"""

import argparse
import copy
import html
import json
import os
from pathlib import Path
from urllib.parse import quote

from PIL import Image, ImageDraw

import activate_m1_art as activation

ROOT = activation.ROOT
REVIEWS = ROOT / "docs/zz-reach2/upgrades/2026-10/assets/reviews"
REPORTS = ("m1-terrain-runtime.json", "m1-control-runtime.json", "m1-loop-deep-4-runtime.json",
           "m1-loop-underground-5-runtime.json", "m1-facility-runtime.json")
GALLERY = REVIEWS / "m1-runtime-gallery.json"


def read_json(path):
    """Read a repository evidence artifact without rewriting it.

    @param path Existing UTF8 JSON file.
    @return Parsed public metadata or native capture report.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def related_frames(asset, frames):
    """Link one selected manifest identity to genuine scenes of its implemented consumer.

    @param asset Exact selected family identity, never a later unselected family.
    @param frames Verified native frame records from all selected scenarios.
    @return Nonempty matching frame paths; unknown family identities refuse.
    """
    if asset.startswith("terrain."):
        terms = ("m1-terrain-layer-3" if "deep" in asset else "m1-terrain-layer-1",)
    elif asset.startswith("survey."):
        terms = ("m1-survey-" + asset.split(".")[1] + "-",)
    elif asset.startswith("control."):
        terms = ("m1-controls-layer-", "m1-job-inspection-")
    elif asset.startswith("purchase.") or asset.startswith("cargo."):
        terms = ("m1-purchase-cargo",)
    elif asset.startswith("vehicle."):
        model = int(asset.rsplit("-", 1)[1])
        # Cutter/support families have independent real model-specific loops.
        # Hoppers/hauler occur in both, retaining actual packet-backed load views.
        terms = (("m1-loop-underground-5-",) if model == 0 else ("m1-loop-deep-4-",) if model == 1
                 else ("loaded-cutter-directions", "actual-disposal", "empty-return"))
    elif asset == "facility.portal.sealed":
        terms = ("m1-portal-picker-basic", "surface-loaded-haul")
    elif asset == "facility.portal.deep-rail":
        terms = ("m1-portal-picker-deep", "m1-loop-deep-4-spanning", "m1-controls-portal")
    elif asset == "facility.exchange":
        terms = ("m1-loop-underground-5-actual-exchange", "m1-loop-underground-5-actual-service")
    elif asset == "facility.rail-service":
        terms = ("m1-rail-service-ready",)
    elif asset == "facility.road-service":
        terms = ("m1-road-service-",)
    elif asset in ("facility.earth-receiver", "facility.rock-receiver"):
        terms = ("actual-disposal",)
    else:
        raise AssertionError("no selected gallery consumer: " + asset)
    result = [frame["path"] for frame in frames if any(term in Path(frame["path"]).name for term in terms)]
    assert result, "selected family has no native gallery scene: " + asset
    return result


def validate(public, packed, runtime, reports):
    """Verify source, raster, simulation, packed distribution and selected-family evidence.

    @param public Public family manifest with selected runtime activation.
    @param packed Canonical selected packing report and distributed payload identity.
    @param runtime Published implemented consumer bindings.
    @param reports Actual native terrain/UI/loop/facility capture reports.
    @return Verified per-family gallery rows and unique native frame records.
    @throws AssertionError If evidence, source bytes, native coverage or distribution is stale.
    """
    activation.validate(public, packed, reports[:2])
    identity = packed["packed_grf"]
    assert runtime["packed_payload"] == identity, "runtime payload differs from reviewed distribution"
    assert activation.digest(ROOT / "build/baseset/openttd.grf") == identity["sha256"], "build distribution differs from reviewed payload"
    unique = {}
    # Retain exact original pixels and immutable saved worlds for every actual
    # scenario. A contact sheet or a source atlas cannot replace this proof.
    for report in reports:
        assert report["simulation_owners_unchanged_during_render"]
        assert activation.digest(ROOT / report["source"]) == report["source_sha256"], "gallery source save changed"
        evidence = activation.read_evidence(ROOT / report["log"])
        assert "All tests passed" in evidence, "native gallery scenario did not pass"
        # Both driven scenarios must retain physical drilling, load, coupling,
        # Surface disposal, empty return, transparency and genuine body picking.
        if "scenario" in report:
            assert report["gallery_coverage"]
            assert "M1_GALLERY_PICK actual_body=true correct_layer=true wrong_layer_refused=true" in evidence
            for stage in ("initial-held", "boring-phase-0", "boring-phase-1", "boring-phase-2", "boring-phase-3",
                          "loaded-cutter", "actual-exchange", "actual-disposal", "empty-return"):
                assert any("stage=" + stage + " " in event for event in report["stages"]), "missing real gallery phase: " + stage
            assert any("transparent" in frame["path"] for frame in report["frames"])
        for frame in report["frames"]:
            assert activation.digest(ROOT / frame["path"]) == frame["sha256"], "gallery frame hash is stale"
            assert Image.open(ROOT / frame["path"]).width > 0
            unique[frame["path"]] = frame
    assert reports[1]["all_selected_survey_resources"], "ten real resource markers are absent"
    assert set(reports[4]["asset_ids"]) == {"facility.road-service", "facility.rail-service"}
    sound = read_json(ROOT / "media/multilayer/sprites/m1/sound-runtime.json")
    assert sound["simulation_owners_unchanged"]
    assert {event["event"] for event in sound["events"]} == {"Idle", "Boring", "Backing", "Service"}
    assert all(event["decoded_bytes"] > 0 and event["nonzero_pcm"] > 0 for event in sound["events"])
    assert "M1_SOUND_COMPLETE events=4 actual_decoder_and_mixer=true owners_unchanged=true" in activation.read_evidence(ROOT / sound["evidence"])
    families = {row["asset_id"]: row for row in public["assets"]}
    rows = []
    # Only this exact selected set loses placeholder status. Later source art,
    # full-profile galleries and personal feedback retain their existing gates.
    for binding in runtime["bindings"]:
        asset = binding["asset_id"]
        family = families[asset]
        assert family["authoring"]["coverage_complete"], "selected source coverage is incomplete"
        assert family["authoring"]["runtime_activated"]
        assert family["source"]["status"] == "authored", "selected placeholder remains"
        assert family["packed"]["sha256"] == identity["sha256"]
        rows.append({"asset_id": asset, "native_offset": binding["native_offset"], "sprite_count": binding["sprite_count"],
                     "consumer": binding["consumer"], "frames": related_frames(asset, list(unique.values())),
                     "evidence": binding["evidence"], "technical_acceptance": "verified", "user_feedback": "deferred"})
    assert len(rows) == 40 and sum(row["sprite_count"] for row in rows) == 2311, "selected gallery scope changed"
    return rows, list(unique.values())


def relative_url(path):
    """Link a repository artifact from the standalone native review directory.

    @param path Repository-relative artifact path.
    @return Escaped relative URL usable without a server or network connection.
    """
    return quote(os.path.relpath(ROOT / path, REVIEWS).replace("\\", "/"), safe="/.")


def contact_sheet(frames):
    """Arrange twelve genuine native frames without scaling, recoloring or changing them.

    @param frames Verified native frame records; each required overview frame must exist.
    @return Path of the contact sheet; all original frame files remain untouched.
    """
    choices = ("m1-loop-underground-5-boring-phase-0-scene-0-zoom-1", "m1-loop-deep-4-boring-phase-0-scene-0-zoom-1",
               "m1-loop-deep-4-loaded-cutter-directions", "m1-loop-underground-5-actual-service-scene-0-zoom-2",
               "m1-road-service-direction-1-zoom-2", "m1-rail-service-ready-zoom-2",
               "m1-loop-deep-4-spanning-entry-scene-1-zoom-2", "m1-loop-deep-4-spanning-retreat-scene-1-zoom-2",
               "m1-loop-deep-4-actual-disposal-scene-0-zoom-1", "m1-loop-deep-4-empty-return-scene-0-zoom-1",
               "m1-controls-portal", "m1-purchase-cargo")
    sheet = Image.new("RGB", (1920, 1600), "#182029")
    draw = ImageDraw.Draw(sheet)
    # Each overview cell carries its exact native scene and a separate caption;
    # scene clipping must be resolved in the renderer, never hidden by resizing.
    for index, term in enumerate(choices):
        frame = next(frame for frame in frames if term in Path(frame["path"]).stem)
        raster = Image.open(ROOT / frame["path"]).convert("RGB")
        assert raster.width <= 640 and raster.height <= 360, "native contact cell would clip"
        x, y = (index % 3) * 640, (index // 3) * 400
        draw.text((x + 8, y + 8), Path(frame["path"]).stem.removeprefix("m1-"), fill="white")
        sheet.paste(raster, (x, y + 30))
    target = REVIEWS / "m1-runtime-contact-sheet.png"
    sheet.save(target)
    return target


def write_html(rows, reports, identity):
    """Publish a standalone gallery with native frames, exact family links and acceptance limits.

    @param rows Verified selected family-to-consumer/frame mappings.
    @param reports Actual scenario reports retaining source, log and frame hashes.
    @param identity Reviewed distributed GRF identity.
    @return None; write only the repository-owned gallery HTML.
    """
    parts = ['<!doctype html><!-- @file m1-runtime-gallery.html; @see r2mlvs13-graphics-and-asset-pipeline.md85B; generated by media/multilayer/review_m1_art.py -->',
             '<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">',
             '<title>M1 native runtime gallery</title><style>body{font:16px system-ui;background:#182029;color:#edf2f7;margin:24px}a{color:#83ccff}table{border-collapse:collapse;width:100%}td,th{padding:8px;border-bottom:1px solid #415064;text-align:left}summary{cursor:pointer;padding:16px}figure{margin:12px 0}img{max-width:100%;image-rendering:pixelated}code{overflow-wrap:anywhere}details{border:1px solid #415064;margin:16px 0}.frames{display:flex;flex-wrap:wrap;gap:16px}.frames figure{width:640px;max-width:100%}figcaption{font-size:13px;overflow-wrap:anywhere}</style>',
             '<h1>M1 native runtime gallery</h1><p>Forty selected families / 2,311 sprites. Technical checks passed; personal art feedback remains deferred.</p>',
             '<p>Direction sheets query the actual renderer without rotating vehicles. Drilling, exchange, turns and occlusion use driven world frames. Service depots were constructed through normal commands on already paid-open ground.</p>',
             '<p>This gallery samples actual states and does not capture every Cartesian source pose. Complete source/packed coverage is separately verified. Later mines, fleet, launch, module, tube and geothermal families retain their own pending gallery work.</p>',
             '<p>Payload SHA256: <code>' + identity['sha256'] + '</code></p>',
             '<p><a href="m1-runtime-gallery.json">Exact provenance and acceptance record</a> · <a href="m1-runtime-contact-sheet.png">Native contact sheet</a> · <a href="../multilayer_sprite_manifest.json">Public manifest</a></p>',
             '<table><tr><th>Manifest ID</th><th>Native range / count</th><th>Actual consumer</th><th>Scenes</th></tr>']
    # Each family has direct native links and its real implementation location;
    # shared scenes remain shared evidence rather than fabricated unique poses.
    for row in rows:
        scene_links = ' '.join('<a href="' + relative_url(path) + '">' + str(index + 1) + '</a>' for index, path in enumerate(row['frames']))
        parts.append('<tr><td>' + html.escape(row['asset_id']) + '</td><td>' + str(row['native_offset']) + ' / ' + str(row['sprite_count']) +
                     '</td><td><a href="' + relative_url(row['consumer']['source']) + '">' + html.escape(row['consumer']['symbol']) + '</a></td><td>' + scene_links + '</td></tr>')
    parts.append('</table>')
    # Built-in expandable groups keep the whole scenario accessible offline;
    # original PNGs and their hashes are presented without JavaScript or edits.
    for name, report in zip(REPORTS, reports):
        parts.append('<details><summary>' + html.escape(name) + ' — ' + str(len(report['frames'])) + ' actual frames</summary><p><a href="' +
                     relative_url(report['source']) + '">Source save</a> · <a href="' + relative_url(report['log']) + '">Native log</a></p><div class="frames">')
        for frame in report['frames']:
            label = Path(frame['path']).name
            parts.append('<figure><a href="' + relative_url(frame['path']) + '"><img loading="lazy" src="' + relative_url(frame['path']) +
                         '" alt="' + html.escape(label) + '"></a><figcaption>' + html.escape(label) + '<br><code>' + frame['sha256'] + '</code></figcaption></figure>')
        parts.append('</div></details>')
    parts.append('</html>')
    (REVIEWS / 'm1-runtime-gallery.html').write_text('\n'.join(parts) + '\n', encoding='utf-8')


def main():
    """Verify the selected gallery, exercise bounded stale-evidence refusals and optionally publish.

    @return None; source/raster/payload bytes remain unchanged, even during publication.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--publish', action='store_true')
    args = parser.parse_args()
    public, packed, runtime = (read_json(path) for path in (activation.PUBLIC, activation.PACKED, activation.RUNTIME))
    reports = [read_json(REVIEWS / name) for name in REPORTS]
    rows, frames = validate(public, packed, runtime, reports)
    # Missing actual animation, a substituted PNG and incomplete selected scope
    # must refuse; mutation fixtures edit metadata copies, never retained bytes.
    for fault in ('phase', 'raster', 'scope'):
        candidates = copy.deepcopy(reports)
        bindings = copy.deepcopy(runtime)
        if fault == 'phase':
            candidates[2]['stages'] = [event for event in candidates[2]['stages'] if 'stage=boring-phase-0 ' not in event]
        elif fault == 'raster':
            candidates[4]['frames'][0]['sha256'] = '0' * 64
        else:
            bindings['bindings'].pop()
        try:
            validate(public, packed, bindings, candidates)
        except AssertionError:
            continue
        raise AssertionError('gallery corruption was accepted: ' + fault)
    if args.publish:
        contact = contact_sheet(frames)
        write_html(rows, reports, packed['packed_grf'])
        record = {'_documentation': 'Owner: r2mlvs13-graphics-and-asset-pipeline.md85B; selected native gallery/distribution technical acceptance',
                  'schema_version': 1, 'selected_families': 40, 'selected_sprites': 2311, 'packed_payload': packed['packed_grf'],
                  'reports': list(REPORTS), 'families': rows, 'frames': frames,
                  'contact_sheet': {'path': contact.relative_to(ROOT).as_posix(), 'sha256': activation.digest(contact)},
                  'html': {'path': (REVIEWS / 'm1-runtime-gallery.html').relative_to(ROOT).as_posix(), 'sha256': activation.digest(REVIEWS / 'm1-runtime-gallery.html')},
                  'sound_proof': 'media/multilayer/sprites/m1/sound-runtime.json', 'technical_acceptance': 'verified', 'user_feedback': 'deferred',
                  'limitations': ['Native scene sampling does not enumerate every Cartesian source pose; full source/loader checks provide that coverage.',
                                  'Direction sheets are real renderer queries; animation and occlusion rely on driven world frames.',
                                  'Later unselected families and whole-profile gallery obligations remain pending.']}
        GALLERY.write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
        # Publish review only for this exact selected set, preserving all later
        # family statuses and keeping personal feedback visibly unapproved.
        selected = {row['asset_id'] for row in rows}
        for family in public['assets']:
            if family['asset_id'] in selected:
                family['runtime']['gallery'] = {'manifest': GALLERY.relative_to(ROOT).as_posix(), 'technical_acceptance': 'verified', 'user_feedback': 'deferred'}
        activation.PUBLIC.write_text(json.dumps(public, indent=2) + '\n', encoding='utf-8')
        for binding in runtime['bindings']:
            binding['gallery_status'] = 'technical_review_verified_user_feedback_deferred'
        runtime['gallery_manifest'] = GALLERY.relative_to(ROOT).as_posix()
        runtime['_documentation'] = 'Owner: r2mlvs13-graphics-and-asset-pipeline.md83B/85B; selected native consumers and technical gallery verified; user feedback deferred'
        activation.RUNTIME.write_text(json.dumps(runtime, indent=2) + '\n', encoding='utf-8')
    print(f'Selected M1 gallery: 40 families / 2311 sprites / {len(frames)} native frames; exact distribution, source and frame hashes plus three corruption refusals pass; user feedback deferred')


if __name__ == '__main__':
    main()
