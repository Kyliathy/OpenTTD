"""Inspect the actual paid moving Mole from its preserved useful-route world.

@file mole_useful_raster.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md task15BC.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native movement earns the inspection point; palette/anchor matching cannot move art or hide failed pixels.
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
from mole_useful_files import SOURCES, read_mole
from tube_travel_sprite_checks import match_travel_pixels

FILE_RECEIPT = "logs/tubes/useful-files/20261018-native-01/mole-files-verification.json"


def main():
    """Load one opposite original useful-world writer and verify real moving body pixels/picking.

    @return None; stale original, failed full-native movement or any world/pixel mismatch prevents the report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--writer-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / FILE_RECEIPT
    assert identity(receipt)["sha256"] == "66152ddaf28dc50f2febbaf5eebb917d158c5c2a388456ac5f17b501cf53fde5"
    history = json.loads(receipt.read_text(encoding="utf-8"))
    selected = identity(args.writer_report)
    assert selected in [row["report"] for row in history["records"] if row.get("phase") == "write"]
    writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
    assert writer["configuration"] != args.config
    point = next(row for row in writer["checkpoints"] if (row["scene"], row["step"]) == (0, 0))
    source = ROOT / point["input"]["path"]; assert identity(source) == point["input"]
    context = current_run_context("tubes", "useful-files")
    context.prepare_runtime()
    saved, proof = read_mole(context, args.config, point, source, "mole-raster", "raster")
    output = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
    matches = re.findall(r"TUBE_TRAVEL_VIEW ((?:\w+=-?\d+ )+)picked=true wrong_layer_refused=true before=([^\r\n]+) after=([^\r\n]+)", output)
    assert len(matches) == 1
    fields, before, after = matches[0]
    view = {key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", fields)}
    assert view["model"] == 9 and view["speed"] > 0
    view.update(before=identity(before), after=identity(after), complete_chunks_exact=compare_chunks(Path(before), Path(after), None),
                native_picking=True, wrong_layer_refused=True)
    rims = re.findall(r"TUBE_TRAVEL_RIM view=" + str(view["view"]) + r" ((?:\w+=-?\d+ ?)+)", output)
    view["rims"] = [{key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", row)} for row in rims]
    assert view["rims"]
    frames = []
    # Both complete native frame repeats must agree without pixel masks.
    # Conversion changes only the file container, preserving every RGB pixel.
    for zoom in (1, 2):
        previous = None
        for repeat in range(2):
            ppm = context.images / f"tube-travel-{view['view']}-zoom-{zoom}-{repeat}.ppm"
            frame = Image.open(ppm).convert("RGB")
            assert frame.size == (640, 360) and len(set(frame.get_flattened_data())) > 8
            pixels = frame.tobytes()
            if previous is not None:
                assert previous == pixels
            previous = pixels
            png = ppm.with_suffix(".png"); save_native_capture(context, frame, ppm, png)
            frames.append({"zoom": zoom, "repeat": repeat, "file": identity(png)})
    view["frames"] = frames
    view["packed_pixels"] = match_travel_pixels(context, 9, view)
    motion = re.search(r"MOLE_RASTER native_steps=(\d+) start=(\d+) end=(\d+) consumed=(\d+) original_units=20", output)
    assert motion is not None
    steps, start, end, consumed = map(int, motion.groups())
    assert steps == end - start > 0 and start == point["tick"] and consumed > 0 and end == view["tick"]
    assert identity(source) == point["input"]
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15BC.2; original useful-route20-unit Mole earns native moving inspection, exact packed pixels/layer picking and pure complete worlds; other models/bootstrap remain separate",
        "configuration": args.config, "focus": "raster", "native": proof,
        "writer": selected, "checkpoint": point, "view": view,
        "native_steps": steps, "start_tick": start, "end_tick": end, "consumed_j": consumed,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in (*SOURCES, "src/tests/native_art_render_fixture.h", "src/transport_tube_vehicle_sprite.cpp", "src/tests/multilayer/tube_travel_sprite_checks.py")],
        "packed_art": identity(ROOT / "media/baseset/openttd.grf"), "file_receipt": identity(receipt),
    }
    destination = context.output("reports", f"mole-useful-raster-{args.config}.json")
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(destination, flush=True)


if __name__ == "__main__":
    main()
