"""Audit authored rolling-stock states at native scale before the M1 handoff.

@file multilayer_vehicle_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md task 8.3
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group 19
This focused source audit does not claim packed loading or gameplay acceptance.
"""

import argparse
import copy
import json
from pathlib import Path
import sys

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_vehicle_art as vehicles


def native_bytes(image, box):
    """Read one actual indexed pose for independent frame comparisons.

    @param image Published native indexed atlas, not the source's RGBA sheet.
    @param box Declared x/y/width/height rectangle for one selected pose.
    @return Exact palette indices inside that pose's native canvas.
    """
    return image.crop((box[0], box[1], box[0] + box[2], box[1] + box[3])).tobytes()


def audit_registration_refusals(model_id):
    """Reject malformed sheet registrations before pixels can fill coverage.

    @param model_id Actual family whose source registration is being checked.
    @return None; fail if an empty, overlapping or out-of-bounds crop is accepted.
    """
    record = json.loads(vehicles.RECORD.read_text(encoding="utf-8"))
    # Exercise this model's explicit view windows: an unrelated cutter's crop
    # cannot prove the freight locomotive's heading and contact registration.
    original = next(row for row in record["sources"] if row["asset_id"] == model_id and "source_cell_boxes" in row)
    # A broken cell table can silently borrow another heading or clip equipment.
    # Corrupt each independent boundary without touching immutable source files.
    for corruption in ("missing", "overlap", "outside"):
        source = copy.deepcopy(original)
        if corruption == "missing":
            source["source_cell_boxes"].pop()
        elif corruption == "overlap":
            source["source_cell_boxes"][1] = source["source_cell_boxes"][0].copy()
        else:
            source["source_cell_boxes"][0][0] = -1
        try:
            vehicles.source_cells(source)
        except ValueError:
            continue
        raise ValueError("malformed authored sheet was accepted: " + corruption)
    print("Explicit sheet crops: missing, overlapping and out-of-bounds registration refusals pass")


def audit(part, model_id):
    """Require real complete native animation for the selected physical part.

    @param part Actual body or separate permanent support, never both as one.
    @param model_id Public rail model family selected for this focused burst.
    @return None; print precise coverage and fail on lost or duplicated phases.
    """
    report, files = vehicles.derive({model_id})
    vehicles.check(report, files, asset_ids={model_id})
    # An empty selection or an unknown family must not turn a focused audit
    # into a zero-work success. A report for another family also fails closed.
    for rejected in (set(), {"vehicle.model-missing"}):
        try:
            vehicles.derive(rejected)
        except ValueError:
            continue
        raise ValueError("focused source audit accepted an absent family")
    try:
        vehicles.check(report, files, asset_ids={"vehicle.model-missing"})
    except ValueError:
        pass
    else:
        raise ValueError("focused source audit accepted a mismatched report")
    model = next(row for row in report["atlases"] if row["asset_id"] == model_id)
    public = json.loads((vehicles.base.PUBLIC).read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == model["asset_id"])
    image = Image.open(ROOT / model["path"])
    selected = {(row["state"], row["frame"], row["direction"]): row for row in model["sprites"] if row["part"] == part}
    frame_count = family["frames"]
    required = {(state, frame, direction) for state in family["states"] for frame in range(frame_count) for direction in vehicles.DIRECTIONS}
    if set(selected) != required:
        raise ValueError(f"incomplete {part} source handoff: {len(required - set(selected))} missing poses")
    # Real visible equipment phases must survive downsampling and quantization.
    # Comparing actual palette pixels catches a falsely complete metadata sheet.
    for state in family["states"]:
        counts = []
        for direction in vehicles.DIRECTIONS:
            phases = []
            for frame in range(frame_count):
                row = selected[(state, frame, direction)]
                x, y, width, height = row["box"]
                phases.append(native_bytes(image, (x, y, width, height)))
            counts.append(len(set(phases)))
        if min(counts) != frame_count:
            raise ValueError(f"{part} {state} animation lost authored phases: {counts}")
        print(f"{part} {state}: {frame_count * 8} real poses; {frame_count} distinct palette phases in N NE E SE S SW W NW")
    # Backing is legitimate reuse of real wheel phases in inverse order. The
    # forward body and its contacts remain unchanged; no reversed bore is drawn.
    if "reversing" in family["states"]:
        for direction in vehicles.DIRECTIONS:
            for frame in range(frame_count):
                forward = selected[("moving", frame, direction)]["box"]
                backward = selected[("reversing", frame_count - 1 - frame, direction)]["box"]
                if native_bytes(image, forward) != native_bytes(image, backward):
                    raise ValueError("backing does not reuse the same real rolling phase")
    print(f"{model_id} {part}: {len(selected)}/{len(required)} poses complete; original chassis/alpha/contacts preserved; source-only audit")
    audit_registration_refusals(model_id)


def main():
    """Run the focused audit for the explicitly selected source-art group.

    @return None; raise on incomplete coverage or an invalid native animation.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--part", choices=["body", "support"], default="body")
    parser.add_argument("--model", choices=[f"vehicle.model-{index:02}" for index in range(5)], default="vehicle.model-00")
    args = parser.parse_args()
    audit(args.part, args.model)


if __name__ == "__main__":
    main()
