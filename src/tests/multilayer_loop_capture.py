"""Capture real assigned portal crossings and the ordinary paid spoil loop.

@file multilayer_loop_capture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups82/83
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
Each invocation runs one bounded native scenario and keeps immutable source saves.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import save_native_capture, current_run_context, run_native, capture_path, report_provenance

CONTEXT = current_run_context("tbm", "spoil-loop")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()


def main():
    """Run one real loop, retain exact native/2x PNGs and verify physical-event evidence.

    @return None; restore startup script bytes and preserve the source save.
    """
    global CONTEXT
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--scenario", choices=("deep", "underground"), default="deep")
    parser.add_argument("--sound-proof", action="store_true", help="verify actual phase selection and decoded PCM through the native hook")
    parser.add_argument("--gallery-proof", action="store_true", help="add actual stopped/service, direction queries, transparency and body-picking evidence")
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    args = parser.parse_args()
    CONTEXT = CONTEXT.for_feature("tbm", "spoil-loop-" + args.scenario)
    fixtures = ROOT / "src/tests/fixtures/haulage"
    reviews = CONTEXT.images
    source = resolve_input_path(fixtures / ("portal-native-deep-stopped.sav" if args.scenario == "deep" else "depot-native-underground-stopped.sav"))
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    number = 1
    # Every attempt gets a fresh native save/raster namespace. Earlier genuine
    # states and failed-observer diagnostics remain intact and cannot be reused
    # as a later passing frame set.
    while (CONTEXT.saves / f"m1-loop-{args.scenario}-{number}-loaded.sav").exists():
        number += 1
    name = f"m1-loop-{args.scenario}-{number}"
    output = CONTEXT.saves / name
    script = RUNTIME / "scripts/game_start.scr"
    existed = script.exists()
    original = script.read_bytes() if existed else None
    log = CONTEXT.logs / f"r2mlvs13-cx-native-loop-{args.scenario}-{number}.log"
    environment = dict(os.environ, OTTD_TBM_LOGISTICS_SOURCE=str(source), OTTD_TBM_LOGISTICS_OUTPUT=str(output),
                       OTTD_TBM_LOGISTICS_COMPLETE_LOOP="1", OTTD_TBM_LOGISTICS_VIEW="headless", OTTD_M1_LOOP_ART="1")
    environment.pop("OTTD_TBM_PORTAL_LOAD_ONLY", None)
    if args.sound_proof:
        environment["OTTD_M1_SOUND_PROOF"] = "1"
    else:
        environment.pop("OTTD_M1_SOUND_PROOF", None)
    if args.gallery_proof:
        environment["OTTD_M1_GALLERY_ART"] = "1"
    else:
        environment.pop("OTTD_M1_GALLERY_ART", None)
    try:
        script.write_text("# @see r2mlvs13-graphics-and-asset-pipeline.md Groups82/83 ordinary physical loop observer.\nlogistics_probe\n", encoding="utf-8")
        with log.open("wb") as sink:
            run_native(CONTEXT, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.tbm-logistics-runtime]", "--durations", "yes"],
                           cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                           creationflags=subprocess.CREATE_NO_WINDOW, check=True, timeout=110)
        evidence = log.read_text(encoding="utf-8", errors="replace")
        assert "All tests passed" in evidence
        assert "LOGISTICS_COMPLETE_LOOP" in evidence
        if args.sound_proof:
            assert "M1_SOUND_COMPLETE events=4 actual_decoder_and_mixer=true owners_unchanged=true" in evidence, "selected journey does not cover all four actual sound states"
        match = re.search(r"M1_LOOP_ART_COMPLETE scenes=(\d+) frames=(\d+) simulation_owners_unchanged_during_render=true", evidence)
        assert match, "actual loop did not complete its read-only raster observation"
        if args.gallery_proof:
            assert "M1_GALLERY_COMPLETE actual_directions=8 transparency=true body_picking=true" in evidence
            assert "M1_GALLERY_PICK actual_body=true correct_layer=true wrong_layer_refused=true" in evidence
        frames = []
        # Retain each actual physical snapshot at both native zoom levels;
        # lossless conversion cannot conceal clipping, facing or mouth occlusion.
        for path in CONTEXT.capture_paths(name + "-*.ppm"):
            raster = Image.open(path)
            assert raster.size == (640, 360)
            assert len(set(raster.get_flattened_data())) > 8
            target = path.with_suffix(".png")
            save_native_capture(CONTEXT, raster, path, target)
            assert Image.open(target).tobytes() == raster.tobytes()
            frames.append({"path": target.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(target.read_bytes()).hexdigest()})
            path.unlink()  # Only this attempt's disposable repository PPM container.
        assert len(frames) == int(match[2]), "actual observed frame count differs from emitted rasters"
        stages = [line for line in evidence.splitlines() if line.startswith("M1_LOOP_ART stage=")]
        report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups82/83; real ordinary loop and physical portal raster evidence",
                  "source": source.relative_to(ROOT).as_posix(), "source_sha256": source_hash, "scenario": args.scenario,
                  "log": log.relative_to(ROOT).as_posix(), "renderer": "actual ViewportDoDraw at native and 2x scales",
                  "stages": stages, "frames": frames, "simulation_owners_unchanged_during_render": True,
                  "gallery_coverage": args.gallery_proof,
                  "user_review": "deferred_until_integration"}
        report_provenance(CONTEXT, report, __file__, args.configuration)
        (CONTEXT.reports / (name + "-runtime.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"Actual {args.scenario} M1 loop: {len(stages)} physical scenes / {len(frames)} native/2x frames; source and rendering-time simulation owners unchanged", flush=True)
    finally:
        if existed:
            script.write_bytes(original)
        else:
            script.unlink()
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "immutable native source changed"


if __name__ == "__main__":
    main()
