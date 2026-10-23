"""Convert immutable geological material originals through the existing native sprite pipeline.

@file convert_hardness_art.py
@see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group10
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md
Only declared chroma-key removal, crop, native resampling, palette conversion
and atlas packing occur here. Built-in imagegen owns the original pixels.
"""

import argparse
import copy
import hashlib
import json

from PIL import Image

import convert_m1_art as base
from convert_terrain_art import png_bytes

RECORD = base.ASSETS / "hardness-generation-record.json"
REPORT = base.OUTPUT / "hardness-manifest.json"
PUBLIC = base.PUBLIC
SCOPE = base.ROOT / "media/multilayer/m1-pack-scope.json"


def remove_key(image):
    """@param image Authored edge cell with its calibration floor. @return Copied source alpha with only declared magenta made transparent."""
    result = image.copy()
    result.putdata([(r, g, b, 0 if r >= 180 and b >= 180 and g <= 80 else a)
                    for r, g, b, a in image.get_flattened_data()])
    return result


def derive():
    """@return Pure manifest and encoded atlas bytes after checking both originals and their source references."""
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    assert document["materials"] == ["soft", "mixed", "hard", "basalt"]
    assert [row["key"] for row in document["sources"]] == ["base", "edges"]
    atlases, files, cache = [], {}, {}
    # Each cell retains an explicit material and state/side identity; neither
    # filesystem order nor a runtime random texture chooses its difficulty.
    for source in document["sources"]:
        for original in [source, *source["references"]]:
            assert base.digest(base.repository_path(original["path"])) == original["sha256"], original["path"]
        assert source["grid"] == [4, 4]
        cells, boxes = base.source_cells(source)
        sheet = Image.new("P", (256, 124), 0)
        sheet.putpalette(base.PALETTE_BYTES)
        sprites = []
        for row, material in enumerate(document["materials"]):
            for column, state in enumerate(source["columns"]):
                cell = cells[row][column]
                if source["key"] == "edges":
                    cell = remove_key(cell)
                pixels = base.quantize(base.terrain(cell), cache)
                assert not set(pixels.get_flattened_data()) & (base.FORBIDDEN - {0})
                box = [column * 64, row * 31, 64, 31]
                sheet.paste(pixels, tuple(box[:2]))
                sprites.append({"asset_id": source["asset_id"], "state": f"{material}_{state}", "frame": 0,
                                "direction": "none", "box": box, "anchor": [-31, 0],
                                "source_cell": [column, row]})
        path = base.OUTPUT / f"hardness-{source['key']}.png"
        encoded = png_bytes(sheet)
        files[path] = encoded
        atlases.append({"asset_id": source["asset_id"], "path": path.relative_to(base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "source": source["path"],
                        "source_boxes": boxes, "size": list(sheet.size), "native_cell": [64, 31],
                        "palette": "dos_8bpp", "transparent_index": 0, "sprites": sprites})
    return {"_documentation": "R2TBMF Group10 / Plan13; immutable hardness originals, native crop/key/quantization only",
            "schema_version": 1, "source_record": RECORD.relative_to(base.ROOT).as_posix(),
            "runtime_activated": False, "pose_count": 32, "atlases": atlases}, files


def publish(report, files):
    """@param report Validated exact source geometry. @param files Encoded native atlases. @return None; append new families without changing any old range or original."""
    base.REVIEW.mkdir(parents=True, exist_ok=True)
    base.OUTPUT.mkdir(parents=True, exist_ok=True)
    for path, encoded in files.items():
        path.write_bytes(encoded)
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    template = next(row for row in public["assets"] if row["asset_id"] == "terrain.edges.underground")
    for atlas, source in zip(report["atlases"], document["sources"]):
        family = next((row for row in public["assets"] if row["asset_id"] == source["asset_id"]), None)
        if family is None:
            family = copy.deepcopy(template)
            family["asset_id"] = source["asset_id"]
            family["packed"] = {"namespace": "native_geology_append", "offset": None, "sha256": None}
            family.pop("runtime", None)
            public["assets"].append(family)
        family["states"] = [pose["state"] for pose in atlas["sprites"]]
        family["source"] = {"status": "authored", "path": source["path"], "sha256": source["sha256"],
                            "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
        family["authoring"] = {"date": "2026-10-09", "status": "source_coverage_verified_runtime_pending",
                               "source_record": RECORD.relative_to(base.ROOT).as_posix(), "originals": [source["path"]],
                               "atlas_manifest": REPORT.relative_to(base.ROOT).as_posix(), "native_atlas": atlas["path"],
                               "native_sha256": atlas["sha256"], "staged_pose_count": 16,
                               "runtime_activated": False, "coverage_complete": True}
        if source["asset_id"] not in scope["asset_ids"]:
            scope["asset_ids"].append(source["asset_id"])
    PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    SCOPE.write_text(json.dumps(scope, indent=2) + "\n", encoding="utf-8")


def main():
    """@return None; publish deterministic inputs or verify exact regeneration without reauthoring selected pixels."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    report, files = derive()
    if args.verify:
        assert json.loads(REPORT.read_text(encoding="utf-8")) == report
        for path, encoded in files.items():
            assert path.read_bytes() == encoded, path
    else:
        publish(report, files)
    print("Hardness source conversion: 16 material/state tiles and 16 material/side overlays; immutable originals and existing DOS palette preserved")


if __name__ == "__main__":
    main()
