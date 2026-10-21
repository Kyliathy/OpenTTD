"""Requalify the retained collector and actual native saved-world viewport under the corrected publication.

@file ground_selected_closure_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHD.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Reuse the unchanged actual application observer without new construction,
capital, material delivery, operating work or a rewritten historical receipt.
"""

import argparse
import json
import re
import subprocess
import time

from ground_pack_check_support import ROOT, handoff
from ground_industry_pixel_checks import project
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference
from support.artifact_paths import create_run_context
import pack_m1_art

BASELINE = "logs/graphics/selected-ground-runtime/20261021-selected-closure-01/before.json"
SOURCE = ROOT / "src/tests/fixtures/geothermal/fixture389-collector-funded-read.sav"
DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"


def collector_pixels(indices):
    """Require exact accepted retained source pixels and independently discriminate the actual access view.

    @param indices Actual current native640x360 indexed collector viewport.
    @return Original source identity, exact matched construction pixels and wrong-view controls.
    """
    public = handoff.read_json(handoff.PUBLIC.relative_to(ROOT).as_posix())
    family = next(row for row in public["assets"] if row["asset_id"] == "facility.vent-collector")
    poses, atlas = pack_m1_art.source_poses(family, {}, {})
    data = indices.read_bytes()
    if len(data) != 640 * 360:
        raise ValueError("retained current collector indexed viewport has unexpected dimensions")
    views = []
    # The immutable original funded-world input has construction-stage0
    # machinery. Compare every original back/front cell and all declared access
    # views; only one complete source can explain the actual native pixels.
    for direction in family["directions"]:
        rows = [row for key, row in poses.items() if key[0] == "construction" and key[1] == 0 and key[2] == direction]
        if len(rows) != 18:
            raise ValueError("retained collector source lacks its complete nine-cell back/front pose")
        expected = project(rows, atlas, {"width": 3, "height": 3})
        wrong = sum(data[y * 640 + x] != value for (x, y), value in expected.items())
        views.append({"direction": direction, "source_pixels": len(expected), "different_pixels": wrong})
    matches = [row for row in views if row["different_pixels"] == 0]
    if len(matches) != 1 or matches[0]["direction"] != "SE" or matches[0]["source_pixels"] != 8930:
        raise ValueError("current retained collector pixels do not match the original complete native source")
    return {"asset_id": family["asset_id"], "state": "construction", "frame": 0, "view": "SE", "matched_source_pixels": 8930,
            "source_atlas": reference(handoff.bound(family["authoring"]["native_atlas"])),
            "source_manifest": reference(handoff.bound(family["authoring"]["atlas_manifest"])), "view_controls": views}


def main():
    """Observe one current configuration's immutable retained world without constructing or advancing it.

    @return None; retain current native image/payload/source pixels and exact old viewport preservation.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--run", required=True)
    parser.add_argument("--baseline", default=BASELINE, help="Explicit current frozen repository logs snapshot for reproducible later audits")
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("selected closure diagnostics belong beneath repository logs")
    output.mkdir(parents=True, exist_ok=True)
    baseline = handoff.bound(args.baseline)
    if not baseline.is_relative_to(ROOT / "logs"):
        raise ValueError("retained-world preservation requires an explicit repository logs baseline")
    preserved(args.baseline)
    source_before = reference(SOURCE)
    if source_before["sha256"] != "70545cf2b0655934ab50e4d3e72009e4d64a4a1cbdd0da4b3d243399ca5a276a":
        raise ValueError("original retained funded collector input changed")
    application = ROOT / "build" / args.configuration / "openttd_ground_viewport_test.exe"
    application_before = reference(application)
    publication_path = ROOT / "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1/publication.json"
    publication = handoff.read_json(publication_path.relative_to(ROOT).as_posix())
    checked_reference(publication["payload"])
    context = create_run_context("selected-ground", "industries", run_id="ground-retained-" + str(time.time_ns()),
                                 root=ROOT, profile="ground-retained-art", process=args.configuration.lower())
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    original = startup.read_bytes() if startup.exists() else None
    startup.write_text("# @see r2mlvs14-remaining-assets-and-runtime-integration.md79FHD retained world only.\nground_industry_art\n", encoding="utf-8")
    environment = context.environment()
    environment.pop("OTTD_GROUND_INDUSTRY_CHECKPOINT", None)
    terminal = output / "native.log"
    attempt = {"_documentation": __doc__, "configuration": args.configuration, "invocation": context.run_id,
               "source": source_before, "application": application_before,
               "images": context.images.relative_to(ROOT).as_posix(), "reports": context.reports.relative_to(ROOT).as_posix()}
    (output / "attempt.json").write_text(json.dumps(attempt, indent=2) + "\n", encoding="utf-8")
    started = time.monotonic()
    try:
        with terminal.open("w", encoding="utf-8") as sink:
            result = subprocess.run([str(application), "[.ground-industry-runtime]"], cwd=ROOT / "build", env=environment,
                                    stdout=sink, stderr=subprocess.STDOUT, timeout=60, check=False)
    finally:
        if original is None:
            startup.unlink()
        else:
            startup.write_bytes(original)
    text = terminal.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
    marker = "GROUND_INDUSTRY_BOOTSTRAP actual_application=true actual_viewport=true new_industry_coverage=false"
    # A complete actual loaded-world observation must preserve collector stock,
    # work/source digest, cash, tick and both RNGs. Fixture construction commands
    # or another successful Catch filter cannot stand in for retained gameplay.
    if result.returncode != 0 or passed is None or int(passed.group(1).replace(",", "")) != 17 or marker not in text:
        raise ValueError("current retained world observation failed; inspect " + str(terminal))
    paths = [context.images / ("bootstrap-collector" + suffix) for suffix in (".ppm", ".idx")]
    pixels = collector_pixels(paths[1])
    previous_path = ROOT / "logs/graphics/selected-ground-runtime/20261021-industry-01" / ("bootstrap-" + args.configuration.lower()) / "bootstrap.json"
    previous = handoff.read_json(previous_path.relative_to(ROOT).as_posix())
    old_captures = {row["path"].rsplit("/", 1)[-1]: row for row in previous["observation"]["captures"]}
    for path in paths:
        before = checked_reference(old_captures[path.name])
        if path.read_bytes() != before.read_bytes():
            raise ValueError("corrected publication changed the retained original native world viewport")
    runtime_payload = reference(context.runtime_dir / "baseset/openttd.grf")
    if runtime_payload["sha256"] != publication["payload"]["sha256"]:
        raise ValueError("retained world did not load the actual corrected publication")
    if reference(SOURCE) != source_before or reference(application) != application_before:
        raise ValueError("retained observation rewrote its original input or executing image")
    context.write_report("native-retained.json", "src/tests/multilayer/ground_selected_closure_checks.py", paths,
                         [SOURCE, application], args.configuration, "openttd_main/CaptureNativeArtViewport/current retained original world")
    retained = DIRECTORY / "selected-retained-bin" / args.configuration / application.name
    retained.parent.mkdir(parents=True, exist_ok=True)
    if retained.exists() and retained.read_bytes() != application.read_bytes():
        raise ValueError("retained selected-world image has another executed identity")
    if not retained.exists():
        retained.write_bytes(application.read_bytes())
    transitions, count = preserved(args.baseline)
    proof = {"_documentation": __doc__, "passed": True, **attempt, "terminal": reference(terminal), "exit_code": result.returncode,
             "assertions": 17, "seconds": round(time.monotonic() - started, 3), "retained_application": reference(retained),
             "captures": [reference(path) for path in paths], "pixels": pixels, "runtime_payload": runtime_payload,
             "original_bootstrap": reference(previous_path), "original_world_viewport_bytes_preserved": True,
             "drawing_owners_unchanged": True, "replacement_funding_or_materials": False, "transported_bootstrap_claimed": False,
             "preserved_file_count": count, "transitions": transitions,
             "baseline": reference(baseline),
             "inputs": [reference(ROOT / path) for path in ("src/tests/multilayer/ground_selected_closure_checks.py",
                        "src/tests/ground_industry_runtime.cpp", "src/tests/native_art_render_fixture.h")]}
    (output / "retained.json").write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: proof[key] for key in ("passed", "configuration", "assertions", "seconds", "original_world_viewport_bytes_preserved")}))


if __name__ == "__main__":
    main()
