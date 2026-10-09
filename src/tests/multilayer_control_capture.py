"""Capture real loaded M1 controls, purchase/cargo resolvers and active seam overlays.

@file multilayer_control_capture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups83B/85B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoints8.7/8.8
Only the real renderer authors these pixels; PPM-to-PNG conversion is lossless.
"""

import hashlib
import json
from pathlib import Path
import subprocess
import argparse
import os

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import save_native_capture, current_run_context, run_native, capture_path, report_provenance

CONTEXT = current_run_context("construction", "layer-controls")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()


def main():
    """Run one bounded actual application observer and preserve its exact native RGB artifacts.

    @return None; restore the startup script and verify the source save stays immutable.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--job-inspection", action="store_true")
    parser.add_argument("--survey-gallery", action="store_true")
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    args = parser.parse_args()
    source = resolve_input_path(ROOT / "src/tests/fixtures/haulage/portal-commissioning/portal-native-deep-stopped.sav")
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    script = RUNTIME / "scripts/game_start.scr"
    script.parent.mkdir(parents=True, exist_ok=True)
    existed = script.exists()
    original = script.read_bytes() if existed else None
    log = CONTEXT.logs / "r2mlvs13-cl-native-control-capture.log"
    environment = dict(os.environ)
    if args.job_inspection:
        environment["OTTD_M1_JOB_ART"] = "1"
    else:
        environment.pop("OTTD_M1_JOB_ART", None)
    if args.survey_gallery:
        environment["OTTD_M1_SURVEY_GALLERY"] = "1"
    else:
        environment.pop("OTTD_M1_SURVEY_GALLERY", None)
    try:
        script.write_text("# @see r2mlvs13-graphics-and-asset-pipeline.md Group83B actual loaded UI observer.\nm1_control_art\n", encoding="utf-8")
        with log.open("wb") as sink:
            run_native(CONTEXT, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.m1-control-art]"], cwd=ROOT / "build",
                           stdout=sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW,
                           env=environment, check=True, timeout=110)
        evidence = log.read_text(encoding="utf-8", errors="replace")
        assert "M1_CONTROL_RUNTIME frames=6 actual_widgets=true engines=5 cargo_icons=2" in evidence
        assert "All tests passed" in evidence
        reviews = CONTEXT.images
        names = [f"m1-controls-layer-{layer}" for layer in (1, 3)]
        names.extend(f"m1-survey-layer-{layer}" for layer in (1, 3))
        names.extend(("m1-controls-portal", "m1-purchase-cargo"))
        if args.job_inspection:
            assert "M1_JOB_RUNTIME frames=2" in evidence
            names.extend(("m1-job-inspection-controls", "m1-job-inspection-footprint"))
        if args.survey_gallery:
            assert "M1_SURVEY_GALLERY resources=10 frames=30 actual_seams=true" in evidence
            # Bind each selected marker to its own genuine seam at both scales,
            # keeping the real inspector text as independent resource evidence.
            for resource in ("COAL", "IORE", "CORE", "BAUX", "SALT", "LITH", "NORE", "COBT", "RARE", "URAN"):
                names.extend((f"m1-survey-{resource}-controls", f"m1-survey-{resource}-zoom-1", f"m1-survey-{resource}-zoom-2"))
        frames = []
        # Keep the actual window dimensions and every native RGB pixel. No
        # resizing, recoloring or repainting may conceal icon/text clipping.
        for name in names:
            path = capture_path(CONTEXT, name + ".ppm")
            raster = Image.open(path)
            assert len(set(raster.get_flattened_data())) > 8, "actual native UI/survey raster is empty"
            output = path.with_suffix(".png")
            save_native_capture(CONTEXT, raster, path, output)
            assert Image.open(output).tobytes() == raster.tobytes(), "conversion changed native raster pixels"
            frames.append({"path": output.relative_to(ROOT).as_posix(), "size": list(raster.size),
                           "sha256": hashlib.sha256(output.read_bytes()).hexdigest()})
            path.unlink()  # Remove only this observer's disposable repository PPM container.
        report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Group83B; actual loaded UI and survey consumer proof",
                  "source": source.relative_to(ROOT).as_posix(), "source_sha256": source_hash,
                  "log": log.relative_to(ROOT).as_posix(), "renderer": "actual widgets, DrawTrainEngine, cargo and ViewportDoDraw with 8bpp-simple",
                  "frames": frames, "simulation_owners_unchanged_during_render": True,
                  "all_selected_survey_resources": args.survey_gallery,
                  "user_review": "deferred_until_integration"}
        report_provenance(CONTEXT, report, __file__, args.configuration)
        (CONTEXT.reports / "m1-control-runtime.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"Actual M1 control raster: {len(frames)} UI/active-seam frames, five real engine previews and two first-party cargo icons; source and owners unchanged", flush=True)
    finally:
        if existed:
            script.write_bytes(original)
        else:
            script.unlink()
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "immutable native source save changed"


if __name__ == "__main__":
    main()
