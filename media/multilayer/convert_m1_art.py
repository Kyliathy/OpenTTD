"""Convert the retained Plans 1-5 artwork to staged DOS-palette sprite atlases.

Only crops, native-size resampling and palette conversion are performed here.
AI image generation/editing remains owned by the built-in image tool. Nothing
in this converter activates sprites, changes a GRF or touches gameplay code.
See docs/zz-reach2/upgrades/2026-10/multilayer-m1-art-pass.md.
"""

import argparse
import hashlib
import json
from pathlib import Path
import sys

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "docs/zz-reach2/upgrades/2026-10/assets/mlvs"
OUTPUT = ROOT / "media/multilayer/sprites/m1"
sys.dont_write_bytecode = True
sys.path.insert(0, str(ASSETS.parent.parent / "tools"))
from sprite_quantize import load_palette, nearest

PALETTE, FORBIDDEN = load_palette(ROOT / "docs/zz-reach2/upgrades/2026-10/assets/dos_palette.txt")
FORBIDDEN = FORBIDDEN | {0}
PALETTE_BYTES = [channel for colour in PALETTE for channel in colour]
DIRECTIONS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]


def digest(path):
    """Hash retained inputs and derived outputs without modifying them."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def repository_path(value):
    """Resolve a declared path and refuse an output outside this repository."""
    path = (ROOT / value).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError(f"asset path leaves the repository: {value}")
    return path


def bands(mask, axis, expected):
    """Find sprite rows/columns from transparent gutters, never guessed cells."""
    length = mask.height if axis == "y" else mask.width
    found = []
    start = None
    for position in range(length):
        box = (0, position, mask.width, position + 1) if axis == "y" else (position, 0, position + 1, mask.height)
        occupied = mask.crop(box).getbbox() is not None
        if occupied and start is None:
            start = position
        elif not occupied and start is not None:
            found.append((start, position))
            start = None
    if start is not None:
        found.append((start, length))
    # Small gaps inside a body or between the cutter and its authored debris
    # are not cell gutters. The true inter-cell gutters are substantially wider.
    while len(found) > expected:
        index = min(range(len(found) - 1), key=lambda i: found[i + 1][0] - found[i][1])
        if found[index + 1][0] - found[index][1] > length / (expected * 8):
            break
        found[index:index + 2] = [(found[index][0], found[index + 1][1])]
    if len(found) != expected:
        raise ValueError(f"expected {expected} transparent-separated {axis} bands, got {found}")
    return found


def source_cells(source):
    """Keep the original alpha while measuring the existing 8bpp alpha cutoff."""
    path = repository_path(source["path"])
    image = Image.open(path).convert("RGBA")
    mask = image.getchannel("A").point(lambda value: 255 if value >= 128 else 0)
    columns, rows = source["grid"]
    xs = bands(mask, "x", columns)
    ys = bands(mask, "y", rows)
    cells = []
    boxes = []
    for top, bottom in ys:
        row = []
        for left, right in xs:
            box = (left, top, right, bottom)
            if mask.crop(box).getbbox() is None:
                raise ValueError(f"empty sprite cell in {source['path']}: {box}")
            row.append(image.crop(box))
            boxes.append(list(box))
        cells.append(row)
    return cells, boxes


def quantize(image, cache):
    """Use the existing deterministic DOS quantizer and reserved-index policy."""
    values = []
    for red, green, blue, alpha in image.get_flattened_data():
        colour = (red, green, blue) if alpha >= 128 else None
        if colour not in cache:
            cache[colour] = nearest(colour, PALETTE, FORBIDDEN)
        values.append(cache[colour])
    result = Image.new("P", image.size, 0)
    result.putpalette(PALETTE_BYTES)
    result.putdata(values)
    result.info["transparency"] = 0
    if not any(values):
        raise ValueError("conversion produced an empty sprite")
    return result


def fit(image, size, maximum):
    """Resample into a fixed canvas without stretching or inventing artwork."""
    scale = min(maximum[0] / image.width, maximum[1] / image.height)
    width = max(1, round(image.width * scale))
    height = max(1, round(image.height * scale))
    small = image.resize((width, height), Image.Resampling.BOX)
    canvas = Image.new("RGBA", size)
    canvas.paste(small, ((size[0] - width) // 2, size[1] - 8 - height if size == (64, 48) else (size[1] - height) // 2))
    return canvas


def terrain(image):
    """Apply the existing normal-zoom diamond mask to an authored ground cell."""
    result = image.resize((64, 31), Image.Resampling.BOX)
    mask = Image.new("L", result.size, 0)
    ImageDraw.Draw(mask).polygon(((31, 0), (63, 15), (31, 30), (0, 15)), fill=255)
    # This is palette/transparency conversion, using the established ground mask.
    from PIL import ImageChops
    result.putalpha(ImageChops.multiply(result.getchannel("A"), mask))
    return result


def atlas(name, asset_id, grid, size, anchor, entries, status, cache):
    """Pack staged pixels with explicit state/view provenance and fixed anchors."""
    columns, rows = grid
    sheet = Image.new("P", (columns * size[0], rows * size[1]), 0)
    sheet.putpalette(PALETTE_BYTES)
    records = []
    for index, entry in enumerate(entries):
        sprite = quantize(entry.pop("pixels"), cache)
        x, y = (index % columns) * size[0], (index // columns) * size[1]
        sheet.paste(sprite, (x, y))
        records.append({**entry, "box": [x, y, *size], "anchor": list(anchor)})
    path = OUTPUT / f"{name}.png"
    sheet.save(path, transparency=0, optimize=False, compress_level=9)
    return {"asset_id": asset_id, "status": status, "path": path.relative_to(ROOT).as_posix(),
            "sha256": digest(path), "size": list(sheet.size), "native_cell": list(size),
            "palette": "dos_8bpp", "transparent_index": 0, "sprites": records}


def build():
    """Convert only declared retained sources; publish no runtime sprite IDs."""
    document = json.loads((ASSETS / "generation-record.json").read_text(encoding="utf-8"))
    sources = {source["key"]: source for source in document["sources"]}
    cells = {}
    for key, source in sources.items():
        actual = digest(repository_path(source["path"]))
        if source.get("sha256", actual) != actual:
            raise ValueError(f"immutable original changed: {source['path']}")
        source["sha256"] = actual
        cells[key], source["source_boxes"] = source_cells(source)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    cache = {}
    outputs = []
    for key in ("underground", "deep"):
        states = ["solid", "cut_face", "excavating", "excavated"]
        entries = [{"state": state, "direction": "none", "frame": row, "source": key,
                    "source_cell": [column, row], "pixels": terrain(cells[key][row][column])}
                   for row in range(3) for column, state in enumerate(states)]
        outputs.append(atlas(f"terrain-{key}", sources[key]["asset_id"], (4, 3), (64, 31), (-31, 0),
                             entries, "base_states_staged_edges_and_runtime_pending", cache))
    # Mapping follows the visible cutter nose. The generator repeated front
    # views; real rear views were separately generated, not rotated or mirrored.
    models = [
        ("tbm-standard", 0, ["idle", "moving", "working", "blocked"],
         [("standard_rear", 0), ("standard_rear", 1), ("standard", 6), ("standard", 3),
          ("standard", 0), ("standard", 1), ("standard", 2), ("standard_rear", 2)]),
        ("tbm-deep", 1, ["idle", "moving", "working", "blocked"],
         [("deep_rear", 0), ("deep_rear", 1), ("deep_tbm", 2), ("deep_tbm", 3),
          ("deep_tbm", 0), ("deep_tbm", 1), ("deep_tbm", 6), ("deep_rear", 2)]),
        ("hopper-earth", 2, ["empty", "partial", "full", "service", "blocked"],
         [("earth", column) for column in (0, 5, 2, 3, 4, 5, 6, 7)]),
        ("hopper-rock", 3, ["empty", "partial", "full", "service", "blocked"],
         [("rock", column) for column in range(8)]),
        ("spoil-hauler", 4, ["idle", "moving", "working", "blocked"],
         [("hauler", column) for column in range(8)]),
    ]
    for name, model, states, views in models:
        entries = []
        for row, state in enumerate(states):
            for direction, (key, column) in zip(DIRECTIONS, views):
                # Column bands are common across states, preserving the model's
                # fixed per-view canvas instead of auto-cropping every frame.
                entries.append({"state": state, "direction": direction, "frame": 0,
                                "source": key, "source_cell": [column, row],
                                "pixels": fit(cells[key][row][column], (64, 48), (48, 32))})
        outputs.append(atlas(name, f"vehicle.model-{model:02}", (8, len(states)), (64, 48), (-32, -32),
                             entries, "frame_0_staged_animation_and_runtime_pending", cache))
        key, column = views[6]
        preview = [{"state": "available", "direction": "none", "frame": 0,
                    "source": key, "source_cell": [column, 0],
                    "pixels": fit(cells[key][0][column], (20, 20), (18, 16))}]
        outputs.append(atlas(f"purchase-model-{model:02}", f"purchase.model-{model:02}", (1, 1), (20, 20), (0, 0),
                             preview, "available_staged_locked_pose_and_runtime_pending", cache))
    for key, name in (("cargo_earth", "cargo-earth"), ("cargo_rock", "cargo-rock")):
        entries = [{"state": "normal", "direction": "none", "frame": 0, "source": key,
                    "source_cell": [0, 0], "pixels": fit(cells[key][0][0], (20, 20), (18, 18))}]
        outputs.append(atlas(name, sources[key]["asset_id"], (1, 1), (20, 20), (0, 0),
                             entries, "authored_icon_runtime_pending", cache))
    # Buildings remain whole-layout candidates. Do not falsely label repeated
    # near-facing views as NE/NW, or claim per-cell foreground cuts are done.
    for key, name, size, states in (
        ("portal", "portal-states-candidate", (64, 64), ["sealed", "commissioning", "connected"]),
        ("deep_portal", "portal-deep-candidate", (64, 64), ["sealed", "commissioning", "connected"]),
        ("exchange", "exchange-bay-candidate", (192, 128), ["construction", "ready", "working", "blocked"]),
    ):
        entries = [{"state": state, "candidate_view": column + 1, "frame": 0, "source": key,
                    "source_cell": [column, row], "pixels": fit(cells[key][row][column], size, (size[0] - 4, size[1] - 4))}
                   for row, state in enumerate(states) for column in range(4)]
        outputs.append(atlas(name, sources[key]["asset_id"], (4, len(states)), size, (-31, -33),
                             entries, "candidate_geometry_and_front_back_slicing_pending", cache))
    report = {"schema_version": 1, "date": document["date"], "runtime_activated": False,
              "source_record": (ASSETS / "generation-record.json").relative_to(ROOT).as_posix(),
              "pose_count": sum(len(output["sprites"]) for output in outputs), "atlases": outputs,
              "notes": ["Vehicle frame 0 only; missing animation frames are not duplicated.",
                        "Earth hopper NE/SW share the same correctly angled symmetrical-wagon source view.",
                        "Whole building candidates are not packed per-tile front/back sprites.",
                        "Cargo GRF identity and native Action5 offsets remain unchanged."]}
    (ASSETS / "generation-record.json").write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    (OUTPUT / "atlas-manifest.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    make_review(outputs)
    bind_manifest(document, report)
    return report


def bind_manifest(document, report):
    """Replace planned source references without claiming packing or acceptance."""
    path = ASSETS.parent / "multilayer_sprite_manifest.json"
    manifest = json.loads(path.read_text(encoding="utf-8"))
    sources = document["sources"]
    for output in report["atlases"]:
        row = next(row for row in manifest["assets"] if row["asset_id"] == output["asset_id"])
        matching = [source for source in sources if source["asset_id"] == output["asset_id"]]
        # Purchase previews are derived from the corresponding vehicle's source.
        if not matching:
            matching = [source for source in sources if source["asset_id"] == output["asset_id"].replace("purchase.", "vehicle.")]
        first = matching[0]
        complete_icon = output["asset_id"].startswith("cargo.")
        row["source"] = {"status": "authored" if complete_icon else "authored_partial",
                         "path": first["path"], "sha256": first["sha256"],
                         "provenance": "first_party_original_builtin_image_gen",
                         "rights": "first_party_generated_original"}
        row["authoring"] = {"date": report["date"], "status": output["status"],
                            "source_record": report["source_record"],
                            "originals": [source["path"] for source in matching],
                            "atlas_manifest": (OUTPUT / "atlas-manifest.json").relative_to(ROOT).as_posix(),
                            "native_atlas": output["path"], "native_sha256": output["sha256"],
                            "staged_pose_count": len(output["sprites"]),
                            "runtime_activated": False, "coverage_complete": complete_icon}
        # Keep the original planned offsets and null packed-payload hashes.
    manifest["authoring_pass"] = {"date": report["date"], "scope": "Plans 1-5 partial source authoring",
                                  "record": "docs/zz-reach2/upgrades/2026-10/multilayer-m1-art-pass.md",
                                  "runtime_activated": False}
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def make_review(outputs):
    """Show every converted pose at 4x nearest-neighbour scale for inspection."""
    tiles = []
    for output in outputs:
        sprite = Image.open(ROOT / output["path"]).convert("RGBA")
        large = sprite.resize((sprite.width * 4, sprite.height * 4), Image.Resampling.NEAREST)
        tiles.append((Path(output["path"]).stem, large))
    width = max(image.width for _, image in tiles) + 24
    height = sum(image.height + 36 for _, image in tiles) + 12
    review = Image.new("RGB", (width, height), (43, 45, 48))
    draw = ImageDraw.Draw(review)
    y = 12
    for label, image in tiles:
        draw.text((12, y), label, fill="white")
        review.paste(image, (12, y + 20), image)
        y += image.height + 36
    review.save(ASSETS / "native-review.png")
    # A compact selection remains legible in an ordinary image viewer. The full
    # gallery above retains all poses for detailed source/heading inspection.
    selected = []
    for output in outputs:
        if output["asset_id"].startswith("purchase."):
            continue
        sheet = Image.open(ROOT / output["path"]).convert("RGBA")
        chosen = [sprite for sprite in output["sprites"] if sprite.get("direction") in ("W", "none")
                  or sprite.get("candidate_view") == 1]
        strip = Image.new("RGBA", (sum(sprite["box"][2] for sprite in chosen), output["native_cell"][1]))
        x = 0
        for sprite in chosen:
            left, top, width, height = sprite["box"]
            strip.paste(sheet.crop((left, top, left + width, top + height)), (x, 0))
            x += width
        selected.append((Path(output["path"]).stem, strip.resize((strip.width * 3, strip.height * 3), Image.Resampling.NEAREST)))
    compact = Image.new("RGB", (max(image.width for _, image in selected) + 24,
                                sum(image.height + 36 for _, image in selected) + 12), (43, 45, 48))
    draw = ImageDraw.Draw(compact)
    y = 12
    for label, image in selected:
        draw.text((12, y), label, fill="white")
        compact.paste(image, (12, y + 20), image)
        y += image.height + 36
    compact.save(ASSETS / "summary-preview.png")


def check():
    """Refuse missing/changed originals, wrong dimensions and reserved colors."""
    report = json.loads((OUTPUT / "atlas-manifest.json").read_text(encoding="utf-8"))
    sources = json.loads((ASSETS / "generation-record.json").read_text(encoding="utf-8"))["sources"]
    for source in sources:
        if digest(repository_path(source["path"])) != source["sha256"]:
            raise ValueError(f"source hash mismatch: {source['path']}")
    for output in report["atlases"]:
        path = repository_path(output["path"])
        image = Image.open(path)
        if image.mode != "P" or list(image.size) != output["size"] or image.info.get("transparency") != 0:
            raise ValueError(f"palette, size or transparency mismatch: {path}")
        if image.getpalette() != PALETTE_BYTES:
            raise ValueError(f"wrong DOS palette: {path}")
        if set(image.get_flattened_data()) & (FORBIDDEN - {0}):
            raise ValueError(f"reserved palette index: {path}")
        if digest(path) != output["sha256"]:
            raise ValueError(f"derived hash mismatch: {path}")
        for sprite in output["sprites"]:
            x, y, width, height = sprite["box"]
            if not (0 <= x < x + width <= image.width and 0 <= y < y + height <= image.height):
                raise ValueError(f"sprite box leaves atlas: {sprite}")
            pixels = image.crop((x, y, x + width, y + height)).get_flattened_data()
            if not any(pixels):
                raise ValueError(f"empty converted pose: {sprite}")
    if report["runtime_activated"] or report["pose_count"] != sum(len(row["sprites"]) for row in report["atlases"]):
        raise ValueError("invalid staged-art report")
    print(f"M1 artwork: {len(sources)} retained originals, {len(report['atlases'])} indexed atlases, "
          f"{report['pose_count']} staged poses; hashes/palette/alpha/bounds passed; runtime pending")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="validate retained outputs without writing")
    arguments = parser.parse_args()
    if not arguments.check:
        build()
    check()
