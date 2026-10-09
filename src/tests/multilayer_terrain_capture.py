"""Run isolated actual-terrain rendering and preserve native RGB review pixels.

@file multilayer_terrain_capture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group82
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
This focused runner restores the original repository startup script and verifies
the supplied immutable save remains unchanged. It does not author source pixels.
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

CONTEXT = current_run_context("geology", "terrain-hardness")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()


def main():
    """Load the native saved world, render actual viewports and convert losslessly.

    @return None; retain actual application evidence and unchanged source identity.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    args = parser.parse_args()
    source = resolve_input_path(ROOT / "src/tests/fixtures/haulage/portal-commissioning/portal-native-deep-stopped.sav")
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    script = RUNTIME / "scripts/game_start.scr"
    script.parent.mkdir(parents=True, exist_ok=True)
    existed = script.exists()
    original = script.read_bytes() if existed else None
    log = CONTEXT.logs / "r2mlvs13-bh-native-terrain-capture.log"
    try:
        script.write_text("# @see r2mlvs13-graphics-and-asset-pipeline.md Group82 native paused-world raster evidence.\nm1_terrain_art\n", encoding="utf-8")
        with log.open("wb") as sink:
            run_native(CONTEXT, [str(ROOT / "build" / args.configuration / "openttd_test.exe"), "[.m1-terrain-art]"], cwd=ROOT / "build",
                           stdout=sink, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW,
                           check=True, timeout=110)
        text = log.read_text(encoding="utf-8", errors="replace")
        assert "M1_TERRAIN_RUNTIME frames=8 picker_views=16" in text
        assert "All tests passed" in text
        reviews = CONTEXT.images
        frames = []
        # Convert only exact artifacts produced by this observer. No resize,
        # palette substitution or painting may conceal native clipping/overlap.
        names = [(f"m1-terrain-layer-{layer}-zoom-{zoom}", layer, zoom) for layer in (1, 3) for zoom in (2, 1)]
        names.extend((f"m1-terrain-picker-{depth}-{endpoint}", None, None)
                     for depth in ("basic", "deep") for endpoint in ("upper", "lower"))
        for name, layer, zoom in names:
            path = capture_path(CONTEXT, name + ".ppm")
            image = Image.open(path)
            assert image.size == ((640, 360) if layer is not None else (640, 160))
            assert len(set(image.get_flattened_data())) > 8, "actual viewport/picker raster is empty"
            output = path.with_suffix(".png")
            save_native_capture(CONTEXT, image, path, output)
            assert Image.open(output).tobytes() == image.tobytes(), "review conversion changed native pixels"
            frames.append({"path": str(output.relative_to(ROOT)).replace("\\", "/"),
                           "sha256": hashlib.sha256(output.read_bytes()).hexdigest(), "layer": layer, "zoom": zoom})
            path.unlink()  # Delete only this observer's disposable repository PPM container.
        report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Group82; actual viewport/picker raster evidence, final-machine crossings pending",
                  "source": str(source.relative_to(ROOT)).replace("\\", "/"), "source_sha256": source_hash,
                  "log": str(log.relative_to(ROOT)).replace("\\", "/"), "renderer": "actual ViewportDoDraw with 8bpp-simple",
                  "frames": frames, "simulation_owners_unchanged_during_render": True, "user_review": "deferred_until_integration"}
        report_provenance(CONTEXT, report, __file__, args.configuration)
        (CONTEXT.reports / "m1-terrain-runtime.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print("Actual M1 terrain raster: four native/2x layer frames and four picker scenes / 16 views, unchanged work/receiver/RNG owners and immutable source save", flush=True)
    finally:
        if existed:
            script.write_bytes(original)
        else:
            script.unlink()
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "immutable native source save changed"


if __name__ == "__main__":
    main()
