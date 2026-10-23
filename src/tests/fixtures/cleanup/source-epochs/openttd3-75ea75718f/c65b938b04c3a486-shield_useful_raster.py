"""Inspect the real paid moving model12 from its accepted original useful Shield world.

@file shield_useful_raster.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15EDE.
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
from shield_useful_checks import PREPARE_RECEIPT, original_route_environment
from shield_useful_read import SOURCES, read_shield
from tube_travel_sprite_checks import match_travel_pixels

FILE_RECEIPT = "logs/tubes/shield-files/20261018-native-01/verification.json"


def main():
    """Load one exact opposite accepted original moving Shield and verify real native pixels/picking and complete purity.

    @return None; changed originals, failed shared native movement or any world/pixel mismatch prevents a raster report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--writer-report", type=Path, required=True)
    args = parser.parse_args(); receipt = ROOT / FILE_RECEIPT
    assert identity(receipt)["sha256"] == "c996f226130a35ace4865f9cd6ed853b85bd8e8dedd5bc30de1fffefd7564db1"
    history = json.loads(receipt.read_text(encoding="utf-8")); selected = identity(args.writer_report)
    assert selected in [row["report"] for row in history["records"] if row.get("phase") == "write"]
    writer = json.loads(args.writer_report.read_text(encoding="utf-8")); assert writer["configuration"] != args.config
    point = next(row for row in writer["checkpoints"] if (row["scene"], row["step"]) == (2, 0))
    source = ROOT / point["input"]["path"]; assert identity(source) == point["input"] and point["snapshot"]["part2_units"] == 24
    preparation = ROOT / PREPARE_RECEIPT; assert identity(preparation) == writer["prepare_receipt"]
    original = ROOT / writer["prepare_report"]["path"]; assert identity(original) == writer["prepare_report"]
    prepared = json.loads(original.read_text(encoding="utf-8")); lineage = json.loads(preparation.read_text(encoding="utf-8"))
    environment = original_route_environment(prepared, lineage)
    context = current_run_context("tubes", "shield-files"); context.prepare_runtime()
    saved, proof = read_shield(context, args.config, point, source, "shield-raster", environment, "raster")
    output = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
    matches = re.findall(r"TUBE_TRAVEL_VIEW ((?:\w+=-?\d+ )+)picked=true wrong_layer_refused=true before=([^\r\n]+) after=([^\r\n]+)", output)
    assert len(matches) == 1
    fields, before, after = matches[0]; view = {key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", fields)}
    assert view["model"] == 12 and view["state"] == 2 and view["speed"] > 0
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
    view["frames"] = frames; view["packed_pixels"] = match_travel_pixels(context, 12, view)
    motion = re.search(r"SHIELD_RASTER native_steps=(\d+) start=(\d+) end=(\d+) consumed=(\d+) original_units=24 full_native_world=true", output)
    assert motion is not None
    steps, start, end, consumed = map(int, motion.groups())
    assert steps == end - start > 0 and start == point["tick"] and consumed > 0 and end == view["tick"]
    assert proof["observed_snapshot"]["part2_units"] == 24
    assert proof["observed_snapshot"]["reserve"] <= point["snapshot"]["reserve"]
    assert proof["observed_snapshot"]["reserve"] + proof["observed_snapshot"]["mine_extracted"] == 32000
    assert identity(source) == point["input"]
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15EDE; actual original24RARE moving Shield earns shared native inspection, original packed pixels/layer picking and pure complete worlds",
        "configuration": args.config, "focus": "raster", "native": proof, "writer": selected, "checkpoint": point, "view": view,
        "native_steps": steps, "start_tick": start, "end_tick": end, "consumed_j": consumed,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in (*SOURCES, "src/tests/shield_useful_raster_fixture.h", "src/tests/native_art_render_fixture.h",
                                                       "src/transport_tube_vehicle_sprite.cpp", "src/tests/multilayer/tube_travel_sprite_checks.py")],
        "packed_art": identity(ROOT / "media/baseset/openttd.grf"), "file_receipt": identity(receipt),
    }
    path = context.output("reports", f"shield-useful-raster-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


if __name__ == "__main__":
    main()
