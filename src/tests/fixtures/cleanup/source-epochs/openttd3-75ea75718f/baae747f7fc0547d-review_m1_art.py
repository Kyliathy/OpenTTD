"""Build the selected M1 gallery from verified native renderer evidence.

@file review_m1_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group85B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.8
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md Group 10R (task 10R.4): the vent collector joins the selected scope.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D; review only delivered runtime families in the larger packed payload.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84C-84D; named selected native receipt extension.
Original frame/source bytes remain immutable; the contact sheet only arranges
existing native pixels with captions. Technical review never implies user approval.
"""

import argparse
import copy
import html
import json
import os
import re
from pathlib import Path
from urllib.parse import quote

from PIL import Image, ImageDraw

import activate_m1_art as activation

ROOT = activation.ROOT
REVIEWS = activation.CACHE / "gallery"
REPORTS = ("terrain", "control", "loop-deep", "loop-underground", "facility", "geology", "collector")
# The delivered runtime review scope remains the42 M1 families plus collector.
# Plan14 79D packs90 families; source-only later consumers cannot enter this
# gallery merely because their pixels now share the distributed payload.
SELECTED_FAMILIES = 43
SELECTED_SPRITES = 3495
GALLERY = REVIEWS / "m1-runtime-gallery.json"


def configure(selection):
    """Point gallery publication at the selected evidence set's generated owner.

    @param selection Explicit content-bound EvidenceSet used by activation too.
    @return None; no file is read from an implicit latest run.
    """
    global REVIEWS, GALLERY
    GALLERY = selection.output("gallery")
    REVIEWS = GALLERY.parent


def scene_name(path):
    """Normalize historical producer spelling when matching semantic gallery scenes.

    @param path Actual selected frame path, retaining its independently verified hash.
    @return Comparison label with loop counters and the old picker spelling reconciled; never a file lookup.
    """
    name = re.sub(r"(m1-loop-(?:deep|underground))-\d+-", r"\1-", Path(path).name)
    return name.replace("m1-terrain-picker-", "m1-portal-picker-")


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
    if asset in ("terrain.hardness", "terrain.hardness-edges"):
        result = [frame["path"] for frame in frames if frame.get("capture_role") == "geology" and "layer-" in Path(frame["path"]).name]
        assert result, "regional hardness has no selected native geology frames"
        return result
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
        terms = (("m1-loop-underground-",) if model == 0 else ("m1-loop-deep-",) if model == 1
                 else ("loaded-cutter-directions", "actual-disposal", "empty-return"))
    elif asset == "facility.portal.sealed":
        terms = ("m1-portal-picker-basic", "surface-loaded-haul")
    elif asset == "facility.portal.deep-rail":
        terms = ("m1-portal-picker-deep", "m1-loop-deep-spanning", "m1-controls-portal")
    elif asset == "facility.exchange":
        terms = ("m1-loop-underground-actual-exchange", "m1-loop-underground-actual-service")
    elif asset == "facility.rail-service":
        terms = ("m1-rail-service-ready",)
    elif asset == "facility.road-service":
        terms = ("m1-road-service-",)
    elif asset in ("facility.earth-receiver", "facility.rock-receiver"):
        terms = ("actual-disposal",)
    elif asset == "facility.vent-collector":
        result = [frame["path"] for frame in frames if frame.get("capture_role") == "collector"]
        assert result, "the vent collector has no selected native collector frames"
        return result
    else:
        raise AssertionError("no selected gallery consumer: " + asset)
    result = [frame["path"] for frame in frames if any(term in scene_name(frame["path"]) for term in terms)]
    assert result, "selected family has no native gallery scene: " + asset
    return result


def validate(public, packed, runtime, reports):
    """Dispatch review through the same owner for explicit legacy or selected evidence.

    @param public Sole public sprite-family contract, including delivery metadata.
    @param packed Complete canonical packed payload and ranges.
    @param runtime Candidate actual bindings from the activation owner.
    @param reports Legacy native reports; selected closures resolve through EvidenceSet.
    @return Verified family rows and unique native frames, without publication.
    """
    if "selected_gallery_boundary" in activation.evidence().document:
        from tools import selected_gallery
        rows, frames, _ = selected_gallery.validate(public, packed, runtime, activation.evidence())
        return rows, frames
    return validate_legacy(public, packed, runtime, reports)


def validate_legacy(public, packed, runtime, reports):
    """Verify source, raster, simulation, packed distribution and selected-family evidence.

    @param public Public family manifest with selected runtime activation.
    @param packed Canonical selected packing report and distributed payload identity.
    @param runtime Published implemented consumer bindings.
    @param reports Actual native terrain/UI/loop/facility capture reports.
    @return Verified per-family gallery rows and unique native frame records.
    @throws AssertionError If evidence, source bytes, native coverage or distribution is stale.
    """
    expected_bindings = activation.validate(public, packed, [reports[5], reports[1]])
    # Runtime review must name the exact delivered identities/ranges, not a
    # same-sized substitution from the larger packed source-only selection.
    binding_fields = ("asset_id", "native_offset", "sprite_count")
    assert [[row[key] for key in binding_fields] for row in runtime["bindings"]] == [
        [row[key] for key in binding_fields] for row in expected_bindings], "gallery differs from delivered runtime scope"
    identity = packed["packed_grf"]
    assert runtime["packed_payload"] == identity, "runtime payload differs from reviewed distribution"
    assert activation.digest(ROOT / "build/baseset/openttd.grf") == identity["sha256"], "build distribution differs from reviewed payload"
    unique = {}
    # Retain exact original pixels and immutable saved worlds for every actual
    # scenario. A contact sheet or a source atlas cannot replace this proof.
    for role, report in zip(REPORTS, reports):
        activation.evidence().validate_capture(report)
        assert report["simulation_owners_unchanged_during_render"]
        assert activation.digest(ROOT / report["source"]) == report["source_sha256"], "gallery source save changed"
        evidence = activation.read_evidence(ROOT / report["log"])
        assert "All tests passed" in evidence, "native gallery scenario did not pass"
        # Both driven scenarios must retain physical drilling, load, coupling,
        # Surface disposal, empty return, transparency and genuine body picking.
        if role in ("loop-deep", "loop-underground"):
            assert report["gallery_coverage"]
            assert "M1_GALLERY_PICK actual_body=true correct_layer=true wrong_layer_refused=true" in evidence
            for stage in ("initial-held", "boring-phase-0", "boring-phase-1", "boring-phase-2", "boring-phase-3",
                          "loaded-cutter", "actual-exchange", "actual-disposal", "empty-return"):
                assert any("stage=" + stage + " " in event for event in report["stages"]), "missing real gallery phase: " + stage
            assert any("transparent" in frame["path"] for frame in report["frames"])
        for frame in report["frames"]:
            assert activation.digest(ROOT / frame["path"]) == frame["sha256"], "gallery frame hash is stale"
            assert Image.open(ROOT / frame["path"]).width > 0
            unique[frame["path"]] = {**frame, "capture_role": role}
    assert reports[1]["all_selected_survey_resources"], "ten real resource markers are absent"
    assert set(reports[4]["asset_ids"]) == {"facility.road-service", "facility.rail-service"}
    sound = read_json(activation.evidence().output("sound"))
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
        # Retained legacy terrain remains source/packing coverage only; its
        # superseded renderer is not reactivated by gallery publication.
        if not binding["retained_only"]:
            assert family["authoring"]["runtime_activated"]
        assert family["source"]["status"] == "authored", "selected placeholder remains"
        assert family["packed"]["sha256"] == identity["sha256"]
        rows.append({"asset_id": asset, "native_offset": binding["native_offset"], "sprite_count": binding["sprite_count"],
                     "consumer": binding["consumer"], "frames": related_frames(asset, list(unique.values())),
                     "evidence": binding["evidence"], "technical_acceptance": "retained_source_only" if binding["retained_only"] else "verified", "user_feedback": "deferred"})
    assert len(rows) == SELECTED_FAMILIES and sum(row["sprite_count"] for row in rows) == SELECTED_SPRITES, "selected gallery scope changed"
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
    choices = ("m1-loop-underground-boring-phase-0-scene-0-zoom-1", "m1-loop-deep-boring-phase-0-scene-0-zoom-1",
               "m1-loop-deep-loaded-cutter-directions", "m1-loop-underground-actual-service-scene-0-zoom-2",
               "m1-road-service-direction-1-zoom-2", "m1-rail-service-ready-zoom-2",
               "m1-loop-deep-spanning-entry-scene-1-zoom-2", "m1-loop-deep-spanning-retreat-scene-1-zoom-2",
               "m1-loop-deep-actual-disposal-scene-0-zoom-1", "m1-loop-deep-empty-return-scene-0-zoom-1",
               "m1-controls-portal", "m1-purchase-cargo")
    REVIEWS.mkdir(parents=True, exist_ok=True)
    selected = [next(frame for frame in frames if term in scene_name(frame["path"])) for term in choices]
    rasters = [Image.open(ROOT / frame["path"]).convert("RGB") for frame in selected]
    # Actual controls can be wider than a terrain viewport. Size the overview
    # to its verified inputs so every native pixel stays visible and unscaled.
    cell_width = max(raster.width for raster in rasters)
    cell_height = max(raster.height for raster in rasters) + 40
    sheet = Image.new("RGB", (3 * cell_width, ((len(selected) + 2) // 3) * cell_height), "#182029")
    draw = ImageDraw.Draw(sheet)
    # Each overview cell carries its exact native scene and a separate caption;
    # scene clipping must be resolved in the renderer, never hidden by resizing.
    for index, (frame, raster) in enumerate(zip(selected, rasters)):
        x, y = (index % 3) * cell_width, (index // 3) * cell_height
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
             '<h1>M1 native runtime gallery</h1><p>' + str(SELECTED_FAMILIES) + ' selected families / ' + f'{SELECTED_SPRITES:,}' + ' sprites, including retained legacy ranges and the vent collector. Technical checks passed; personal art feedback remains deferred.</p>',
             '<p>Direction sheets query the actual renderer without rotating vehicles. Drilling, exchange, turns and occlusion use driven world frames. Service depots were constructed through normal commands on already paid-open ground.</p>',
             '<p>This gallery samples actual states and does not capture every Cartesian source pose. Complete source/packed coverage is separately verified. Later mines, fleet, launch, module, tube and other geothermal families retain their own pending gallery work.</p>',
             '<p>Payload SHA256: <code>' + identity['sha256'] + '</code></p>',
             '<p><a href="m1-runtime-gallery.json">Exact provenance and acceptance record</a> · <a href="m1-runtime-contact-sheet.png">Native contact sheet</a> · <a href="' + relative_url(activation.PUBLIC.relative_to(ROOT).as_posix()) + '">Public manifest</a></p>',
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


def selected_contact_sheet(rows):
    """Arrange one complete current native scene per distinct representative frame.

    @param rows Verified selected family rows with actual frame paths and state contexts.
    @return Contact sheet path; source images remain unscaled and unmodified.
    """
    selected = {}
    for row in rows:  # Shared native scenes represent several actual families; one unscaled scene with their captions keeps the overview compact without inventing separate poses.
        if row['review_kind'] != 'current_selected_native':
            continue
        choices = row['frames']
        preferred = [name for name in choices if 'zoom-1-0' in name]
        name = (preferred or choices)[0]
        selected.setdefault(name, []).append(row['asset_id'])
    rasters = [(name, assets, Image.open(ROOT / name).convert('RGB')) for name, assets in selected.items()]
    width = max(raster.width for _, _, raster in rasters)
    height = max(raster.height for _, _, raster in rasters) + 64
    sheet = Image.new('RGB', (3 * width, ((len(rasters) + 2) // 3) * height), '#182029')
    draw = ImageDraw.Draw(sheet)
    for index, (name, assets, raster) in enumerate(rasters):  # Every full native canvas fits its own cell; labels occupy separate padding and never cover or alter observed pixels.
        x, y = (index % 3) * width, (index // 3) * height
        draw.text((x + 6, y + 6), '\n'.join(assets), fill='white')
        sheet.paste(raster, (x, y + 64))
    REVIEWS.mkdir(parents=True, exist_ok=True)
    target = REVIEWS / 'm1-runtime-contact-sheet.png'
    sheet.save(target)
    return target


def publish_selected(rows, frames, summary, packed):
    """Publish the selected offline review under its named generated evidence owner.

    @param rows Sixty-nine verified family/range/consumer mappings.
    @param frames Unique exact retained/current native PNG identities.
    @param summary Exact accepted closure identities, boundaries and scope counts.
    @param packed Complete immutable payload report; no packing fields are written.
    @return Gallery identity; runtime and public activation remain the following84D task.
    """
    from tools import selected_gallery
    contact = selected_contact_sheet(rows)
    parts = ['<!doctype html><!-- @file m1-runtime-gallery.html; @see r2mlvs14-remaining-assets-and-runtime-integration.md84C; generated by review_m1_art.py -->',
             '<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">',
             '<title>Selected native tube and geothermal gallery</title>',
             '<style>body{font:16px system-ui;background:#182029;color:#edf2f7;margin:24px}a{color:#83ccff}table{border-collapse:collapse;width:100%}td,th{padding:8px;border-bottom:1px solid #415064;text-align:left}details{border:1px solid #415064;margin:16px 0;padding:12px}summary{cursor:pointer}img{max-width:100%;image-rendering:pixelated}figure{display:inline-block;max-width:100%;margin:12px}code,figcaption{overflow-wrap:anywhere}figcaption{font-size:13px}</style>',
             '<h1>Selected native tube and geothermal gallery</h1>',
             '<p>69 review families / 10,557 packed poses; 65 active consumers and four retained terrain ranges. 27 families have current both-reader scenes, including the refreshed collector. The remaining 42 retain their dated review. Technical acceptance; human feedback remains deferred.</p>',
             '<p>Actual five-model useful routes and the original paid geothermal chain retain their saved owners, clock and both random streams. Prepared floor/material/status and paid-command topology scenes are presentation boundaries. Purchase images are native preview canvases, not transactions or whole purchase windows. Natural vent images sample observed surveyed/excavated phases; full Cartesian source/loader coverage is separate.</p>',
             '<p><a href="m1-runtime-gallery.json">Exact states, owners and provenance</a> · <a href="m1-runtime-contact-sheet.png">Complete unscaled native contact sheet</a> · <a href="' + relative_url(summary['retained_gallery']['path']) + '">Retained earlier review</a></p>',
             '<p>Payload SHA256: <code>' + packed['packed_grf']['sha256'] + '</code>. Later unselected factories, full-profile art, network/operator checks and personal approval remain pending.</p>',
             '<table><tr><th>Public family</th><th>Native range / count</th><th>Actual consumer</th><th>Review</th><th>Exact native frames</th></tr>']
    for row in rows:  # Each public ID links all original native frames; shared scenes remain explicit and the retained prefix never acquires a false current-world claim.
        links = ' '.join('<a href="' + relative_url(name) + '">' + str(index + 1) + '</a>' for index, name in enumerate(row['frames']))
        parts.append('<tr><td>' + html.escape(row['asset_id']) + '</td><td>' + str(row['native_offset']) + ' / ' + str(row['sprite_count']) + '</td><td>' + html.escape(row['consumer']['symbol']) + '</td><td>' + html.escape(row['review_kind']) + '</td><td>' + links + '</td></tr>')
    parts.append('</table>')
    indexed = {frame['path']: frame for frame in frames}
    for row in rows:  # Expandable native previews sample each refreshed family while the full exact frame/state matrix remains available through the table and gallery JSON.
        if row['review_kind'] != 'current_selected_native':
            continue
        parts.append('<details><summary>' + html.escape(row['asset_id']) + ' — current native scenes in both readers</summary>')
        for name in dict.fromkeys([row['frames'][0], row['frames'][-1]]):
            parts.append('<figure><a href="' + relative_url(name) + '"><img loading="lazy" src="' + relative_url(name) + '" alt="' + html.escape(row['asset_id']) + '"></a><figcaption>' + html.escape(Path(name).name) + '<br><code>' + indexed[name]['sha256'] + '</code></figcaption></figure>')
        parts.append('</details>')
    parts.append('</html>')
    target = REVIEWS / 'm1-runtime-gallery.html'
    target.write_text('\n'.join(parts) + '\n', encoding='utf-8')
    record = {'_documentation': 'r2mlvs14-remaining-assets-and-runtime-integration.md84C; actual selected native gallery;public activation belongs to84D;human approval deferred',
              'schema_version': 2, **summary, 'packed_payload': packed['packed_grf'], 'families': rows, 'frames': frames,
              'contact_sheet': activation.evidence_identity(contact), 'html': activation.evidence_identity(target),
              'gallery_owner': activation.evidence_identity(Path(__file__)), 'selected_adapter': activation.evidence_identity(Path(selected_gallery.__file__)),
              'technical_acceptance': 'verified', 'user_feedback': 'deferred',
              'simulation_owners_clock_rng_unchanged': True,
              'limitations': ['Native scenes sample actual states, not every Cartesian source pose.', 'Retained review is dated; current scenes cover only the selected delivered additions and collector.',
                              'Prepared presentation boundaries do not prove a paid bootstrap; original useful routes and original paid geothermal receipts retain that separate authority.',
                              'Raw PPM/runtime copies and dated binaries are disposable after sealed lossless conversion; retained PNGs, original saves, source and native logs remain exact.']}
    GALLERY.write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
    return activation.evidence_identity(GALLERY)


def main():
    """Verify the selected gallery, exercise bounded stale-evidence refusals and optionally publish.

    @return None; source/raster/payload bytes remain unchanged, even during publication.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--publish', action='store_true')
    parser.add_argument('--cache-only', action='store_true', help='publish generated gallery without changing public source metadata')
    parser.add_argument('--evidence-manifest', type=Path, default=activation.DEFAULT_MANIFEST)
    parser.add_argument('--prepare-selected', type=Path, help='absent named EvidenceSet destination; preserves the selected binding handoff')
    parser.add_argument('--selected-receipts', nargs=4, help='exact purchase,structure,movement,geothermal completed closures, in order')
    args = parser.parse_args()
    configure(activation.select_evidence(args.evidence_manifest))
    if args.prepare_selected is not None:
        from tools import selected_gallery
        assert args.selected_receipts, 'four exact completed native closures are required'
        target = selected_gallery.prepare(activation.evidence(), args.selected_receipts, args.prepare_selected)
        print(json.dumps({'selected_evidence': activation.evidence_identity(target)}))
        return
    if 'selected_gallery_boundary' in activation.evidence().document:
        from tools import selected_gallery
        assert not args.publish, 'selected public delivery belongs to84D activation after the gallery'
        public, packed = (read_json(path) for path in (activation.PUBLIC, activation.PACKED))
        bindings = activation.validate_binding_review(public, packed)
        rows, frames, summary = selected_gallery.validate(public, packed, {'packed_payload': packed['packed_grf'], 'bindings': bindings}, activation.evidence())
        if args.cache_only:
            summary['gallery'] = publish_selected(rows, frames, summary, packed)
        print(json.dumps({**summary, 'native_frames': len(frames)}))
        return
    public, packed, runtime = (read_json(path) for path in (activation.PUBLIC, activation.PACKED, activation.RUNTIME))
    reports = [activation.evidence().report(name) for name in REPORTS]
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
    if args.publish or args.cache_only:
        contact = contact_sheet(frames)
        write_html(rows, reports, packed['packed_grf'])
        record = {'_documentation': 'Owner: r2mlvs13-graphics-and-asset-pipeline.md85B; selected native gallery/distribution technical acceptance',
                  'schema_version': 1, 'selected_families': SELECTED_FAMILIES, 'selected_sprites': SELECTED_SPRITES, 'packed_payload': packed['packed_grf'],
                  'reports': [activation.evidence().selected('reports', name).relative_to(ROOT).as_posix() for name in REPORTS], 'families': rows, 'frames': frames,
                  'contact_sheet': {'path': contact.relative_to(ROOT).as_posix(), 'sha256': activation.digest(contact)},
                  'html': {'path': (REVIEWS / 'm1-runtime-gallery.html').relative_to(ROOT).as_posix(), 'sha256': activation.digest(REVIEWS / 'm1-runtime-gallery.html')},
                  'sound_proof': activation.evidence().output('sound').relative_to(ROOT).as_posix(), 'technical_acceptance': 'verified', 'user_feedback': 'deferred',
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
        if not args.cache_only:
            activation.PUBLIC.write_text(json.dumps(public, indent=2) + '\n', encoding='utf-8')
        for binding in runtime['bindings']:
            binding['gallery_status'] = 'technical_review_verified_user_feedback_deferred'
        runtime['gallery_manifest'] = GALLERY.relative_to(ROOT).as_posix()
        runtime['_documentation'] = 'Owner: r2mlvs13-graphics-and-asset-pipeline.md83B/85B; selected native consumers and technical gallery verified; user feedback deferred'
        activation.RUNTIME.write_text(json.dumps(runtime, indent=2) + '\n', encoding='utf-8')
    print(f'Selected M1 gallery: {SELECTED_FAMILIES} families / {SELECTED_SPRITES} sprites / {len(frames)} native frames; exact distribution, source and frame hashes plus three corruption refusals pass; user feedback deferred')


if __name__ == '__main__':
    main()
