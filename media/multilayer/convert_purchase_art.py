"""Bake retained available and authored locked M1 purchase UI previews.

@file convert_purchase_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md task 8.3
Padlocks belong to purchase-menu icons only; rolling stock uses separate art.
"""

import argparse
import hashlib
import json

from PIL import Image, ImageDraw

import convert_m1_art as base
from convert_terrain_art import png_bytes


RECORD = base.ASSETS / "purchase-generation-record.json"
REPORT = base.OUTPUT / "purchase-details-manifest.json"


def derive():
    """Select real retained previews and convert the new locked UI originals.

    @return Exact pose/provenance manifest and reproducible indexed atlas bytes.
    @throws ValueError On changed originals, wrong model coverage or bad geometry.
    """
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    locked = document["locked_source"]
    if base.digest(base.repository_path(locked["path"])) != locked["sha256"]:
        raise ValueError("locked purchase original changed")
    for reference in locked["references"]:
        if base.digest(base.repository_path(reference["path"])) != reference["sha256"]:
            raise ValueError("locked purchase reference changed")
    cells, boxes = base.source_cells(locked)
    public = json.loads((base.ASSETS.parent / "multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in public["assets"]}
    outputs, files, cache = [], {}, {}
    # The five existing model identities keep their exact available previews.
    # Only the separate locked menu pose is new; padlocks never enter a vehicle
    # body or substitute for the actual profile/calendar/capability owner.
    for model, available in enumerate(document["available_sources"]):
        identity = f"purchase.model-{model:02}"
        family = families[identity]
        if family["native_size"] != [20, 20] or family["anchor"] != [0, 0] or family["states"] != ["available", "locked"]:
            # A UI preview cannot borrow a world vehicle canvas/anchor or invent
            # a new dated state; the published icon family remains authoritative.
            raise ValueError("purchase preview disagrees with public UI geometry")
        if base.digest(base.repository_path(available["path"])) != available["sha256"]:
            raise ValueError("retained available purchase preview changed")
        original = Image.open(base.repository_path(available["path"])).convert("RGBA")
        if original.size != (20, 20):
            raise ValueError("retained purchase icon has wrong native size")
        subject = cells[0][model]
        crop = subject.getchannel("A").point(lambda value: 255 if value >= 128 else 0).getbbox()
        selected = base.fit(subject.crop(crop), (20, 20), (18, 18))
        sheet = Image.new("P", (40, 20), 0)
        sheet.putpalette(base.PALETTE_BYTES)
        sheet.paste(base.quantize(original, cache), (0, 0))
        sheet.paste(base.quantize(selected, cache), (20, 0))
        path = base.OUTPUT / f"purchase-model-{model:02}-authored.png"
        encoded = png_bytes(sheet)
        files[path] = encoded
        outputs.append({"asset_id": identity, "path": path.relative_to(base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "size": [40, 20], "native_cell": [20, 20],
                        "anchor": [0, 0], "coverage_complete": True, "runtime_activated": False,
                        "sprites": [{"state": "available", "frame": 0, "direction": "none", "box": [0, 0, 20, 20],
                                     "source_path": available["path"], "source_sha256": available["sha256"]},
                                    {"state": "locked", "frame": 0, "direction": "none", "box": [20, 0, 20, 20],
                                     "source_path": locked["path"], "source_sha256": locked["sha256"],
                                     "source_box": boxes[model], "source_crop": list(crop)}]})
    return {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18; purchase UI only, no runtime gate",
            "schema_version": 1, "source_record": RECORD.relative_to(base.ROOT).as_posix(),
            "runtime_activated": False, "pose_count": 10, "atlases": outputs}, files


def publish(report, files):
    """Retain exact UI atlases, public source coverage and a native review sheet.

    @param report Complete available/locked icon pose and source provenance.
    @param files Encoded deterministic native atlas bytes by repository path.
    @return None; write source-stage artifacts without activating gameplay sprites.
    """
    for path, encoded in files.items():
        path.write_bytes(encoded)
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    manifest_path = base.ASSETS.parent / "multilayer_sprite_manifest.json"
    public = json.loads(manifest_path.read_text(encoding="utf-8"))
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    locked = record["locked_source"]
    # Complete UI source coverage remains explicitly separate from distributed
    # packing and runtime loading, retaining protected IDs and original previews.
    for atlas in report["atlases"]:
        family = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        family["authoring"] = {"date": "2026-10-07", "status": "complete_purchase_ui_source_runtime_pending",
                               "source_record": report["source_record"], "atlas_manifest": REPORT.relative_to(base.ROOT).as_posix(),
                               "native_atlas": atlas["path"], "native_sha256": atlas["sha256"], "staged_pose_count": 2,
                               "required_pose_count": 2, "missing_pose_count": 0, "coverage_complete": True, "runtime_activated": False,
                               "originals": list(dict.fromkeys(family.get("authoring", {}).get("originals", []) + [locked["path"]]))}
        family["source"]["status"] = "authored"
    manifest_path.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    review = Image.new("RGB", (900, 380), (44, 47, 51))
    draw = ImageDraw.Draw(review)
    draw.text((12, 12), "SOURCE ONLY: available / locked purchase UI; padlocks never appear on map vehicles", fill="white")
    # Native icons expose tiny-state readability; enlargement preserves the
    # exact indexed pixels for later user inspection without smoothing details.
    for index, atlas in enumerate(report["atlases"]):
        image = Image.open(base.ROOT / atlas["path"]).convert("RGBA")
        left = 12 + index * 176
        draw.text((left, 42), atlas["asset_id"], fill="white")
        review.paste(image, (left, 65), image)
        large = image.resize((160, 80), Image.Resampling.NEAREST)
        review.paste(large, (left, 105), large)
    review.save(base.ASSETS / "purchase-details-review.png")


def check(report, files):
    """Reject stale, empty, wrong-palette or falsely identical purchase states.

    @param report Exact source-derived pose/geometry expectations.
    @param files Expected reproducible indexed PNG bytes.
    @return None; fail when a native UI pose is incomplete or changed.
    """
    if json.loads(REPORT.read_text(encoding="utf-8")) != report:
        raise ValueError("purchase source report differs from originals")
    for index, (path, encoded) in enumerate(files.items()):
        if path.read_bytes() != encoded:
            raise ValueError("purchase icon conversion is not reproducible")
        image = Image.open(path)
        if image.mode != "P" or image.info.get("transparency") != 0 or image.getpalette() != base.PALETTE_BYTES:
            # The UI shares DOS transparent-index/color policies with the loaded
            # profile; a palette mismatch changes the appearance of locked cues.
            raise ValueError("purchase palette/alpha contract mismatch")
        if set(image.get_flattened_data()) & (base.FORBIDDEN - {0}):
            raise ValueError("purchase preview uses reserved palette colors")
        before, after = image.crop((0, 0, 20, 20)), image.crop((20, 0, 40, 20))
        if not any(before.get_flattened_data()) or not any(after.get_flattened_data()) or before.tobytes() == after.tobytes():
            # Available and locked are real distinct authored menu presentations;
            # an empty or copied available icon cannot satisfy the locked state.
            raise ValueError("missing or duplicated purchase state")
        record = json.loads(RECORD.read_text(encoding="utf-8"))
        original = Image.open(base.repository_path(record["available_sources"][index]["path"]))
        if before.tobytes() != original.tobytes():
            raise ValueError("existing available purchase pixels were altered")
    print("Purchase source conversion: 5 models / 10 available+locked UI poses; exact regeneration, palette and coverage pass; runtime pending")


def main():
    """Publish or verify the bounded M1 purchase-preview source slice.

    @return None; fail on changed selected originals or derived UI artifacts.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="verify without writing")
    report, files = derive()
    if not parser.parse_args().check:
        publish(report, files)
    check(report, files)


if __name__ == "__main__":
    main()
