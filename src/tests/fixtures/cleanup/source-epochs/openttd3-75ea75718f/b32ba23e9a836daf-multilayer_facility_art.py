"""Focused native-source placement and corruption fixtures for M1 facilities.

@file multilayer_facility_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 23/26/37/38
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint 8.4
These fixtures validate retained source pixels; gameplay rendering is later.
"""

import argparse
import copy
import json
from pathlib import Path
import sys

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_facility_art as art


def verify_family(asset_id):
    """Verify unchanged physical contacts and real door/cell/phase coverage.

    @param asset_id Exact selected public facility identity in this focused run.
    @return Number of native source cell/part sprites checked.
    """
    report = json.loads(art.REPORT.read_text(encoding="utf-8"))
    document = json.loads(art.RECORD.read_text(encoding="utf-8"))
    source = next(row for row in document["families"] if row["asset_id"] == asset_id)
    atlas = next(row for row in report["atlases"] if row["asset_id"] == asset_id)
    image = Image.open(art.base.repository_path(atlas["path"]))
    cells, _boxes = art.select_cells(source)
    count = len(source["tile_cells"])
    poses = {}
    fixed_cells, palette_cache = {}, {}
    # Compare each phase against independent unanimated source reduction. All
    # geometry outside the equipment fitting stays byte-exact, including alpha.
    for row in atlas["sprites"]:
        x, y, width, height = row["box"]
        pose = image.crop((x, y, x + width, y + height))
        key = (row["direction"], row["state"], row["frame"], row["tile_variant"], row["part"])
        assert key not in poses
        poses[key] = pose
        assert pose.getbbox() is not None, "empty physical facility part"
        if row["part"] != "back":
            continue
        column = source["directions"].index(row["direction"]) * count + row["tile_variant"]
        state = art.STATES.index(row["state"])
        fixed_key = (state, column)
        if fixed_key not in fixed_cells:
            # A fixed cell is reduced once. Comparing four animation phases
            # must not repeat its expensive source-to-DOS color search.
            fixed_cells[fixed_key] = art.base.quantize(art.native_housing(cells[state][column]), palette_cache)
        fixed = fixed_cells[fixed_key]
        target = source["cue_targets"][column]
        allowed = set()
        if target is not None:
            tx, ty, tw, th = target
            allowed = {(px, py) for py in range(ty, ty + th) for px in range(tx, tx + tw)}
        for py in range(64):
            for px in range(64):
                assert bool(pose.getpixel((px, py))) == bool(fixed.getpixel((px, py))), "animated ground/silhouette moved"
                if (px, py) not in allowed:
                    assert pose.getpixel((px, py)) == fixed.getpixel((px, py)), "phase repainted fixed housing"
    # Far doors cover opaque near walls. Near available/occupied openings expose
    # the real entry while retaining rear interiors and closed blocked gates.
    for direction in source["directions"]:
        if direction not in ("NE", "SE", "SW", "NW"):
            continue
        for state in art.STATES:
            for frame in range(4):
                back = poses[(direction, state, frame, 0, "back")]
                front = poses[(direction, state, frame, 0, "front")]
                if direction in ("NE", "NW"):
                    assert back.tobytes() == front.tobytes(), "far opaque housing loses cover"
                if direction in ("SE", "SW") and state in ("ready", "working"):
                    # Only the real completed near mouth is transparent to the
                    # vehicle; partial/blocked structures retain their own art.
                    point = (42, 44) if direction == "SE" else (21, 44)
                    assert back.getpixel(point) != 0
                    assert front.getpixel(point) == 0
    return len(atlas["sprites"])


def verify_refusals(asset_id):
    """Reject invalid source cell crops and a lamp insert on transparent ground.

    @param asset_id Exact source family supplying realistic crop corruption cases.
    @return None; fail if invalid placement is silently accepted.
    """
    source = next(row for row in json.loads(art.RECORD.read_text(encoding="utf-8"))["families"] if row["asset_id"] == asset_id)
    if "state_sources" in source:
        # Corrupt one real authored sheet, not the logical assembly of separately
        # retained machinery and reused Earth panels, to test physical bounds.
        source = source["state_sources"]["ready"]
    _cells, boxes = art.select_cells(source)
    # Mutations exercise independent source-boundary failures without editing
    # immutable originals or writing broken public manifests to the workspace.
    for mutation in ("missing", "overlap", "outside"):
        invalid = copy.deepcopy(source)
        invalid["source_cell_boxes"] = copy.deepcopy(boxes)
        if mutation == "missing":
            invalid["source_cell_boxes"].pop()
        elif mutation == "overlap":
            invalid["source_cell_boxes"][1] = invalid["source_cell_boxes"][0]
        else:
            invalid["source_cell_boxes"][0][0] = -1
        try:
            art.select_cells(invalid)
        except ValueError:
            continue
        raise AssertionError("invalid facility source crop accepted")
    empty = Image.new("RGBA", (64, 64))
    try:
        art.insert_indicator(empty, Image.new("RGBA", (3, 3), (255, 255, 255, 255)), [10, 10, 3, 3])
    except ValueError:
        return
    raise AssertionError("lamp insert created a detached physical silhouette")


def main():
    """Run one actively developed facility's source handoff and refusal cases.

    @return None; print checked pose count, failing on source-contract violations.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--family", required=True)
    args = parser.parse_args()
    report, files = art.derive()
    art.check(report, files, args.family)
    count = len(next(row for row in report["atlases"] if row["asset_id"] == args.family)["sprites"])
    verify_refusals(args.family)
    print(f"Facility source/refusal fixtures: {args.family}, {count} poses; crop bounds/overlap and detached lamp insert refusals pass")


if __name__ == "__main__":
    main()
