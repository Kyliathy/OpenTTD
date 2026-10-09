"""Retain the native road-service gallery built on the completed paid excavation.

@file multilayer_facility_capture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md task85B.2
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.8
The renderer authors every pixel; conversion only changes the image container.
"""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import save_native_capture, current_run_context, run_native, capture_path, report_provenance

CONTEXT = current_run_context("facilities", "service-depots")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()


def main():
    """Run one bounded facility observer and preserve exact native scene provenance.

    @return None; restore startup script bytes and preserve the completed source save.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    args = parser.parse_args()
    source = resolve_input_path(ROOT / "src/tests/fixtures/haulage/spoil-loop-underground/m1-loop-underground-5-complete-loop.sav")
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    script = RUNTIME / "scripts/game_start.scr"
    existed = script.exists()
    original = script.read_bytes() if existed else None
    log = CONTEXT.logs / "r2mlvs3mp-t-road-service-capture.log"
    try:
        script.write_text("# @see r2mlvs13-graphics-and-asset-pipeline.md85B.2 actual paid-floor depot capture.\nm1_facility_art\n", encoding="utf-8")
        with log.open("wb") as sink:
            run_native(CONTEXT, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.m1-facility-art]"], cwd=ROOT / "build",
                           stdout=sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW,
                           check=True, timeout=110)
        evidence = log.read_text(encoding="utf-8", errors="replace")
        assert "All tests passed" in evidence
        assert "M1_FACILITY_RUNTIME frames=12 actual_road_depot_commands=4 actual_unassigned_rail_depot=true previously_paid_floor=true render_owners_unchanged=true" in evidence
        reviews = CONTEXT.images
        names = [f"m1-road-service-direction-{direction}-zoom-{zoom}" for direction in range(4) for zoom in (1, 2)]
        names.extend(f"m1-road-service-transparent-zoom-{zoom}" for zoom in (1, 2))
        names.extend(f"m1-rail-service-ready-zoom-{zoom}" for zoom in (1, 2))
        frames = []
        # Retain native dimensions and every RGB pixel so entrance placement,
        # footprint and real building transparency stay independently reviewable.
        for name in names:
            path = capture_path(CONTEXT, name + ".ppm")
            raster = Image.open(path)
            assert raster.size == (640, 360)
            assert len(set(raster.get_flattened_data())) > 8
            target = path.with_suffix(".png")
            save_native_capture(CONTEXT, raster, path, target)
            assert Image.open(target).tobytes() == raster.tobytes()
            frames.append({"path": target.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(target.read_bytes()).hexdigest()})
            path.unlink()  # Only this observer's disposable repository PPM containers.
        report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md85B.2; actual road-service command/viewport gallery",
                  "source": source.relative_to(ROOT).as_posix(), "source_sha256": source_hash,
                  "log": log.relative_to(ROOT).as_posix(), "asset_ids": ["facility.road-service", "facility.rail-service"], "frames": frames,
                  "setup": "normal depot commands on already paid-open retained job floor; no source save rewritten",
                  "simulation_owners_unchanged_during_render": True, "user_review": "deferred_until_integration"}
        report_provenance(CONTEXT, report, __file__, args.configuration)
        (CONTEXT.reports / "m1-facility-runtime.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print("Actual service gallery: four road entrances and one unassigned rail depot / twelve native and 2x scenes; source and render-time owners unchanged")
    finally:
        if existed:
            script.write_bytes(original)
        else:
            script.unlink()
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "immutable completed source save changed"


if __name__ == "__main__":
    main()
