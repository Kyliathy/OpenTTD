"""Inspect the real paid moving model13 from its accepted original useful Forge world.

@file forge_useful_raster.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84CB; optional real purchase previews retain this world's existing owner/RNG boundary.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FEE.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Actual native work earns the inspection point; original packed pixels, physical anchors and inventories are never repaired.
"""
import argparse
import json
from pathlib import Path
import re
import sys

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context, save_native_capture
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import compare_chunks
from forge_useful_checks import PREPARE_RECEIPT, original_route_environment
from forge_useful_read import SOURCES, read_forge
from tube_travel_sprite_checks import match_travel_pixels

FILE_RECEIPT = "logs/tubes/forge-files/20261018-native-01/verification.json"


def main():
    """Load one exact opposite accepted original moving Forge and verify real native pixels/picking and complete purity.

    @return None; changed originals, failed shared native movement or any world/pixel mismatch prevents a raster report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--writer-report", type=Path, required=True)
    args = parser.parse_args(); receipt = ROOT / FILE_RECEIPT
    assert identity(receipt)["sha256"] == "8b475d2812d10529bba306cfd4bf9b9cf31572eb5f885f180de3cda10e8a775b"
    history = json.loads(receipt.read_text(encoding="utf-8")); selected = identity(args.writer_report)
    assert selected in [row["report"] for row in history["records"] if row.get("phase") == "write"]
    writer = json.loads(args.writer_report.read_text(encoding="utf-8")); assert writer["configuration"] != args.config
    point = next(row for row in writer["checkpoints"] if (row["scene"], row["step"]) == (2, 0))
    source = ROOT / point["input"]["path"]; assert identity(source) == point["input"] and point["snapshot"]["part6_units"] == 32
    preparation = ROOT / PREPARE_RECEIPT; assert identity(preparation) == writer["prepare_receipt"]
    original = ROOT / writer["prepare_report"]["path"]; assert identity(original) == writer["prepare_report"]
    prepared = json.loads(original.read_text(encoding="utf-8")); lineage = json.loads(preparation.read_text(encoding="utf-8"))
    environment = original_route_environment(prepared, lineage)
    context = current_run_context("tubes", "forge-files"); context.prepare_runtime()
    saved, proof = read_forge(context, args.config, point, source, "forge-raster", environment, "raster")
    output = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
    matches = re.findall(r"TUBE_TRAVEL_VIEW ((?:\w+=-?\d+ )+)picked=true wrong_layer_refused=true before=([^\r\n]+) after=([^\r\n]+)", output)
    assert len(matches) == 1
    fields, before, after = matches[0]; view = {key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", fields)}
    assert view["model"] == 13 and view["state"] == 1 and view["speed"] > 0
    view.update(before=identity(before), after=identity(after), complete_chunks_exact=compare_chunks(Path(before), Path(after), None),
                native_picking=True, wrong_layer_refused=True)
    assert view["complete_chunks_exact"] == compare_chunks(Path(after), saved, None) == 126
    rims = re.findall(r"TUBE_TRAVEL_RIM view=" + str(view["view"]) + r" ((?:\w+=-?\d+ ?)+)", output)
    view["rims"] = [{key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", row)} for row in rims]; assert view["rims"]
    frames = []
    # Each complete native RGB repetition must match at its actual scale.
    # Converting PPM to PNG changes only the container, never any pixel value.
    for zoom in (1, 2):
        previous = None
        for repeat in range(2):
            ppm = context.images / f"tube-travel-{view['view']}-zoom-{zoom}-{repeat}.ppm"
            frame = Image.open(ppm).convert("RGB"); assert frame.size == (640, 360) and len(set(frame.get_flattened_data())) > 8
            pixels = frame.tobytes()
            if previous is not None:
                assert pixels == previous
            previous = pixels; png = ppm.with_suffix(".png"); save_native_capture(context, frame, ppm, png)
            frames.append({"zoom": zoom, "repeat": repeat, "file": identity(png)})
    view["frames"] = frames; view["packed_pixels"] = match_travel_pixels(context, 13, view)
    motion = re.search(r"FORGE_RASTER native_steps=(\d+) start=(\d+) end=(\d+) consumed=(\d+) original_units=32 full_native_world=true", output)
    assert motion is not None
    steps, start, end, consumed = map(int, motion.groups())
    assert steps == end - start > 0 and start == point["tick"] and consumed > 0 and end == view["tick"]
    assert proof["observed_snapshot"]["part6_units"] == 32
    # Both independent native reserves retain continuing finite work while all
    # original raw, RFMT, receiver and saved-effect owners advance together.
    for index in range(2):
        assert proof["observed_snapshot"][f"reserve{index}"] <= point["snapshot"][f"reserve{index}"]
        assert proof["observed_snapshot"][f"reserve{index}"] + proof["observed_snapshot"][f"mine{index}_extracted"] == 32000
    assert proof["observed_snapshot"]["train_vehicles"] == 9
    assert proof["observed_snapshot"]["vehicles"] == 9 + proof["observed_snapshot"]["effect_vehicles"]
    assert identity(source) == point["input"]
    purchase_frames = []
    # The optional native purchase observer uses the same routed producer and
    # complete pure boundary. Preserve its recorded RGB canvases losslessly;
    # no new preview policy, engine identity or painted source is introduced.
    for name in re.findall(r"^SELECTED_PURCHASE_FRAME (.+)$", output, re.M):
        record = json.loads(name)
        for native_path in record["paths"]:  # Every native repetition remains separately declared and hash-bound to this exact invocation.
            ppm = Path(native_path); image = Image.open(ppm).convert("RGB"); png = ppm.with_suffix(".png")
            save_native_capture(context, image, ppm, png); assert Image.open(png).convert("RGB").tobytes() == image.tobytes()
            purchase_frames.append(identity(png))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15FEE; actual original32RFMT moving Forge earns shared native inspection, original packed pixels/layer picking and pure complete worlds",
        "configuration": args.config, "focus": "raster", "native": proof, "writer": selected, "checkpoint": point, "view": view,
        "native_steps": steps, "start_tick": start, "end_tick": end, "consumed_j": consumed,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "purchase_frames": purchase_frames,
        "sources": [identity(ROOT / name) for name in (*SOURCES, "src/tests/forge_useful_raster_fixture.h", "src/tests/native_art_render_fixture.h", "src/tests/selected_purchase_view_fixture.h",
                                                       "src/transport_tube_vehicle_sprite.cpp", "src/tests/multilayer/tube_travel_sprite_checks.py")],
        "packed_art": identity(ROOT / "media/baseset/openttd.grf"), "file_receipt": identity(receipt),
    }
    path = context.output("reports", f"forge-useful-raster-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


if __name__ == "__main__":
    main()
