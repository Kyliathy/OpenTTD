"""Locate exact packed orbital parts in actual ordinary-game screenshots.

@file orbital_final_art_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13L
Detail crops retain original native indexed pixels for visual inspection. They
never replace source art, original screenshots or actual gameplay owners.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy

from PIL import Image
from space_preview_checks import ROOT, identity

COMMON = runpy.run_path(str(Path(__file__).with_name("space_final_art_checks-2.py")))


def qualify(selector, assets):
    """Find substantial exact source parts and retain unscaled native detail crops.

    @param selector Explicit completed ordinary-capture membership JSON.
    @param assets Required source families to inspect in the actual field.
    @return New qualification report; visual/occlusion acceptance is still separate.
    """
    membership = json.loads(selector.read_text())
    report_path = ROOT / membership["report"]["path"]
    assert identity(report_path) == membership["report"]
    proof = json.loads(report_path.read_text())
    assert proof["complete"] and not proof["added_stock"] and not proof["granted_work"] and not proof["granted_pose"]
    manifest_path = ROOT / "logs/proofs/mlm/native-art/space/orbital-packed-manifest.json"
    manifest = json.loads(manifest_path.read_text())
    sheet = Image.open(ROOT / "media/baseset/openttd/orbital_final.png")
    originals = {}
    # Aliases retain their logical identities but share one pixel search.
    # Empty/tiny parts cannot establish a family's visible native body; actual
    # foreground composition receives its own visual/geometry review.
    for row in manifest["sprites"]:
        if row["asset_id"] not in assets:
            continue
        x, y, width, height = row["box"]
        sprite = sheet.crop((x, y, x + width, y + height))
        pixels = sprite.tobytes()
        assert hashlib.sha256(pixels).hexdigest() == row["pixel_sha256"]
        if sum(value != 0 for value in pixels) < 48:
            continue
        key = row["asset_id"], sprite.size, row["pixel_sha256"]
        if key not in originals:
            originals[key] = (sprite, [])
        originals[key][1].append({field: row[field] for field in
                                 ("native_offset", "state", "frame", "direction", "tile_variant", "part")})
    captures = []
    # Each retained original screenshot is searched independently. A live
    # sequence may show different honest states; no pose is assigned to it.
    for capture in proof["captures"]:
        path = ROOT / capture["path"]
        assert identity(path)["sha256"] == capture["sha256"]
        native = Image.open(path)
        assert native.mode == "P"
        assert hashlib.sha256(native.tobytes()).hexdigest() == capture["indexed_sha256"]
        matches = []
        for (asset, size, digest), (sprite, rows) in originals.items():
            for box in COMMON["positions"](native, sprite):
                matches.append(dict(asset_id=asset, native_box=box, source_pixel_sha256=digest, logical_aliases=rows))
        assert matches, (path, assets)
        assert set(assets) <= {row["asset_id"] for row in matches}, (path, "a required family has no substantial exact native part")
        left = max(0, min(row["native_box"][0] for row in matches) - 24)
        top = max(0, min(row["native_box"][1] for row in matches) - 24)
        right = min(native.width, max(row["native_box"][0] + row["native_box"][2] for row in matches) + 24)
        bottom = min(native.height, max(row["native_box"][1] + row["native_box"][3] for row in matches) + 24)
        detail = path.with_name(path.stem + "-detail.png")
        native.crop((left, top, right, bottom)).save(detail)
        captures.append(dict(source=capture, exact_part_matches=matches, detail=identity(detail),
                             detail_native_box=[left, top, right - left, bottom - top]))
    output = report_path.with_name("ordinary-orbital-pixel-matches.json")
    output.write_text(json.dumps(dict(_documentation=__doc__, ordinary_capture=identity(report_path),
                                     manifest=identity(manifest_path), assets=assets, captures=captures,
                                     visual_review_complete=False), indent=2) + "\n")
    return identity(output)


def main():
    """Qualify one explicit capture membership with an actual required family list."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selector", type=Path, required=True)
    parser.add_argument("--assets", nargs="+", required=True)
    args = parser.parse_args()
    print(qualify(ROOT / args.selector, args.assets), flush=True)


if __name__ == "__main__":
    main()
