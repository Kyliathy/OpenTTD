"""Convert the retained Plans 1-5 artwork to staged DOS-palette sprite atlases.

Only crops, native-size resampling and palette conversion are performed here.
AI image generation/editing remains owned by the built-in image tool. Nothing
in this converter activates sprites, changes a GRF or touches gameplay code.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17.
"""

import argparse
from functools import partial
import hashlib
import json
from pathlib import Path
import sys

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "media/multilayer/source/m1"
OUTPUT = ROOT / "logs/proofs/mlm/native-art/derived"
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
REVIEW = ROOT / "logs/proofs/mlm/native-art/authoring"
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "media/multilayer/tools"))
from sprite_quantize import load_palette, nearest

PALETTE, FORBIDDEN = load_palette(ROOT / "media/multilayer/palettes/dos_palette.txt")
FORBIDDEN = FORBIDDEN | {0}
PALETTE_BYTES = [channel for colour in PALETTE for channel in colour]
DIRECTIONS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]


def digest(path):
    """Hash retained inputs and derived outputs without modifying them.

    @param path Existing source or generated artifact whose exact bytes identify it.
    @return Lowercase SHA256 digest.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def repository_path(value, selected_inputs=None, context=None):
    """Resolve a declared artifact within this source checkout.

    @param value Repository-relative path from the public source/generation contract.
    @param selected_inputs Optional exact historical aliases to selected current input identities.
    @param context Explicit recipe run that owns generated input reads.
    @return Canonical contained path; callers still distinguish source from generated output.
    """
    value = str(value)
    if selected_inputs is not None:
        from evidence import identity
        if value in selected_inputs:
            reference = selected_inputs[value]
            if identity(ROOT / reference["path"]) != reference:
                raise ValueError("selected source predecessor changed: " + value)
            value = reference["path"]
        elif value.startswith("logs/"):
            raise ValueError("source predecessor needs an explicit selected input: " + value)
        if value.startswith("mlm/"):
            path = (ROOT / value).resolve()
            parts = path.relative_to(ROOT / "mlm/reviews").parts
            # Current candidates and explicitly selected predecessors must share
            # the recipe invocation; dated source aliases cannot reopen caches.
            if context is None or len(parts) != 5 or parts[2] != context.run_id or parts[3] not in ("images", "reports", "data"):
                raise ValueError("generated source input has a foreign invocation or role")
    path = (ROOT / value).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError(f"asset path leaves the repository: {value}")
    return path


def bands(mask, axis, expected):
    """Find declared sprite rows or columns from transparent source gutters.

    @param mask Authored alpha/occupancy mask.
    @param axis x for columns or y for rows.
    @param expected Required cell count from the original generation record.
    @return Ordered nonempty source intervals; ambiguous gutter coverage is refused.
    """
    length = mask.height if axis == "y" else mask.width
    found = []
    start = None
    # Transparent gutters delimit authored cells; internal material pixels
    # start/end occupied bands without inventing a fixed crop from sheet size.
    for position in range(length):
        box = (0, position, mask.width, position + 1) if axis == "y" else (position, 0, position + 1, mask.height)
        occupied = mask.crop(box).getbbox() is not None
        # Start only a newly occupied run, then close it at its first gutter.
        # These boundaries determine which original pixels belong to each pose.
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
    """Extract registered original sprite cells without inventing missing views.

    @param source Immutable source record including path and declared grid.
    @return Cell matrix and exact source crop boxes.
    """
    path = repository_path(source["path"])
    image = Image.open(path).convert("RGBA")
    mask = image.getchannel("A").point(lambda value: 255 if value >= 128 else 0)
    columns, rows = source["grid"]
    xs = bands(mask, "x", columns)
    ys = bands(mask, "y", rows)
    cells = []
    boxes = []
    # Cartesian source rows/columns retain their original order and crop boxes
    # so a converted pose remains traceable to the approved authored cell.
    for top, bottom in ys:
        row = []
        # Every declared cell must contain actual source material.
        for left, right in xs:
            box = (left, top, right, bottom)
            if mask.crop(box).getbbox() is None:
                raise ValueError(f"empty sprite cell in {source['path']}: {box}")
            row.append(image.crop(box))
            boxes.append(list(box))
        cells.append(row)
    return cells, boxes


def quantize(image, cache):
    """Map original RGB pixels to the permitted DOS palette deterministically.

    @param image Authored RGBA image after declared crop/resampling.
    @param cache Shared RGB-to-index memo preserving one conversion policy.
    @return Indexed sprite with transparency and the canonical palette.
    """
    values = []
    # Every source pixel uses the same alpha threshold and permitted DOS palette;
    # caching colors changes cost only, never native color or transparency meaning.
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
    """Fit one source cell inside its declared native canvas.

    @param image Original source cell.
    @param size Native canvas dimensions.
    @param maximum Maximum visible subject dimensions within that canvas.
    @return Centered RGBA pose preserving the declared aspect ratio.
    """
    scale = min(maximum[0] / image.width, maximum[1] / image.height)
    width = max(1, round(image.width * scale))
    height = max(1, round(image.height * scale))
    small = image.resize((width, height), Image.Resampling.BOX)
    canvas = Image.new("RGBA", size)
    canvas.paste(small, ((size[0] - width) // 2, size[1] - 8 - height if size == (64, 48) else (size[1] - height) // 2))
    return canvas


def terrain(image):
    """Apply the existing normal-zoom diamond mask to authored ground.

    @param image Original RGBA ground cell selected by its generation record.
    @return Native 64x31 terrain preserving alpha inside the established diamond.
    """
    result = image.resize((64, 31), Image.Resampling.BOX)
    mask = Image.new("L", result.size, 0)
    ImageDraw.Draw(mask).polygon(((31, 0), (63, 15), (31, 30), (0, 15)), fill=255)
    # This is palette/transparency conversion, using the established ground mask.
    from PIL import ImageChops
    result.putalpha(ImageChops.multiply(result.getchannel("A"), mask))
    return result


def atlas(name, asset_id, grid, size, anchor, entries, status, cache, output_directory=None):
    """Pack staged pixels with explicit state/view provenance and fixed anchors.

    @param name Stable generated atlas basename.
    @param asset_id Public family identity associated with the source pixels.
    @param grid Atlas columns and rows.
    @param size Native canvas dimensions for each pose.
    @param anchor Existing native draw anchor, unchanged by cache relocation.
    @param entries Owned pose records carrying the pixels to quantize.
    @param status Explicit source coverage status, independent of native acceptance.
    @param cache Shared palette conversion memo.
    @param output_directory Explicit generated atlas directory, or the authoring owner's default.
    @return Actual path/hash/geometry/pose record for the generated atlas.
    """
    columns, rows = grid
    sheet = Image.new("P", (columns * size[0], rows * size[1]), 0)
    sheet.putpalette(PALETTE_BYTES)
    records = []
    # Each declared pose keeps its native canvas/anchor and palette ownership;
    # output relocation changes storage, never physical sprite identity.
    for index, entry in enumerate(entries):
        sprite = quantize(entry.pop("pixels"), cache)
        x, y = (index % columns) * size[0], (index // columns) * size[1]
        sheet.paste(sprite, (x, y))
        records.append({**entry, "box": [x, y, *size], "anchor": list(anchor)})
    if output_directory is not None:
        from evidence import contained
        output_directory = contained(output_directory, ROOT / "mlm/reviews")
    path = (output_directory or OUTPUT) / f"{name}.png"
    sheet.save(path, transparency=0, optimize=False, compress_level=9)
    return {"asset_id": asset_id, "status": status, "path": path.relative_to(ROOT).as_posix(),
            "sha256": digest(path), "size": list(sheet.size), "native_cell": list(size),
            "palette": "dos_8bpp", "transparent_index": 0, "sprites": records}


def build(cache_only=False, context=None):
    """Regenerate staged source atlases from preserved original artwork.

    @param cache_only Keep public source/provenance declarations unchanged when rebuilding ignored products.
    @param context Optional explicit recipe RunContext for images/reports/review output.
    @return Actual source-conversion report; native runtime acceptance remains separate.
    """
    # A selected recipe may rebuild candidates but cannot use relocation to
    # invoke the existing source publisher or advance authored completion flags.
    if context is not None and not cache_only:
        raise ValueError("selected source conversion requires cache_only=True")
    if context is not None:
        from evidence import contained, identifier
        if context.root.resolve() != ROOT:
            raise ValueError("selected source conversion belongs to another checkout")
        # Explicit context components and shared containment must be validated
        # before original source writers receive their generated destinations.
        for value in (context.feature, context.scenario, context.run_id):
            identifier(value)
        contained(context.images, ROOT / "mlm/reviews")
        contained(context.reports, ROOT / "mlm/reviews")
    output_directory = context.images if context is not None else OUTPUT
    report_path = context.reports / "atlas-manifest.json" if context is not None else OUTPUT / "atlas-manifest.json"
    emit_atlas = partial(atlas, output_directory=output_directory if context is not None else None)
    document = json.loads((ASSETS / "generation-record.json").read_text(encoding="utf-8"))
    sources = {source["key"]: source for source in document["sources"]}
    cells = {}
    # Every converted sheet must still identify its approved original bytes;
    # a new output directory cannot authorize a replacement source image.
    for key, source in sources.items():
        actual = digest(repository_path(source["path"]))
        if source.get("sha256", actual) != actual:
            raise ValueError(f"immutable original changed: {source['path']}")
        source["sha256"] = actual
        cells[key], source["source_boxes"] = source_cells(source)
    output_directory.mkdir(parents=True, exist_ok=True)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    cache = {}
    outputs = []
    # Both geological depths retain their own source cells and original states.
    for key in ("underground", "deep"):
        states = ["solid", "cut_face", "excavating", "excavated"]
        entries = [{"state": state, "direction": "none", "frame": row, "source": key,
                    "source_cell": [column, row], "pixels": terrain(cells[key][row][column])}
                   for row in range(3) for column, state in enumerate(states)]
        outputs.append(emit_atlas(f"terrain-{key}", sources[key]["asset_id"], (4, 3), (64, 31), (-31, 0),
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
    # Source view selection follows the existing physical model contract;
    # each state/view is composed from its original rather than synthesized.
    for name, model, states, views in models:
        entries = []
        for row, state in enumerate(states):
            for direction, (key, column) in zip(DIRECTIONS, views):
                # Column bands are common across states, preserving the model's
                # fixed per-view canvas instead of auto-cropping every frame.
                entries.append({"state": state, "direction": direction, "frame": 0,
                                "source": key, "source_cell": [column, row],
                                "pixels": fit(cells[key][row][column], (64, 48), (48, 32))})
        outputs.append(emit_atlas(name, f"vehicle.model-{model:02}", (8, len(states)), (64, 48), (-32, -32),
                             entries, "frame_0_staged_animation_and_runtime_pending", cache))
        key, column = views[6]
        preview = [{"state": "available", "direction": "none", "frame": 0,
                    "source": key, "source_cell": [column, 0],
                    "pixels": fit(cells[key][0][column], (20, 20), (18, 16))}]
        outputs.append(emit_atlas(f"purchase-model-{model:02}", f"purchase.model-{model:02}", (1, 1), (20, 20), (0, 0),
                             preview, "available_staged_locked_pose_and_runtime_pending", cache))
    # Cargo UI icons retain their separately authored resource meanings.
    for key, name in (("cargo_earth", "cargo-earth"), ("cargo_rock", "cargo-rock")):
        entries = [{"state": "normal", "direction": "none", "frame": 0, "source": key,
                    "source_cell": [0, 0], "pixels": fit(cells[key][0][0], (20, 20), (18, 18))}]
        outputs.append(emit_atlas(name, sources[key]["asset_id"], (1, 1), (20, 20), (0, 0),
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
        outputs.append(emit_atlas(name, sources[key]["asset_id"], (4, len(states)), size, (-31, -33),
                             entries, "candidate_geometry_and_front_back_slicing_pending", cache))
    report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md; deterministic source-conversion cache", "schema_version": 1, "date": document["date"], "runtime_activated": False,
              "source_record": (ASSETS / "generation-record.json").relative_to(ROOT).as_posix(),
              "pose_count": sum(len(output["sprites"]) for output in outputs), "atlases": outputs,
              "notes": ["Vehicle frame 0 only; missing animation frames are not duplicated.",
                        "Earth hopper NE/SW share the same correctly angled symmetrical-wagon source view.",
                        "Whole building candidates are not packed per-tile front/back sprites.",
                        "Cargo GRF identity and native Action5 offsets remain unchanged."]}
    if not cache_only:
        (ASSETS / "generation-record.json").write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if context is not None:
        make_review(outputs, context.images)
    if not cache_only:
        make_review(outputs)
        bind_manifest(document, report)
    return report


def bind_manifest(document, report):
    """Bind authored source references without claiming native runtime acceptance.

    @param document Original generation record and immutable source identities.
    @param report Actual derived atlas/pose coverage.
    @return None; explicitly publish source metadata through the single public manifest.
    """
    path = PUBLIC
    manifest = json.loads(path.read_text(encoding="utf-8"))
    sources = document["sources"]
    # Explicit authoring publication updates only families represented by this
    # source report, retaining the public catalogue as the sole asset authority.
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
                                  "record": "docs/zz-rhr/upgrades/2026-10/graphics/multilayer-m1-art-pass.md",
                                  "runtime_activated": False}
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def make_review(outputs, review_directory=None):
    """Arrange converted source poses for inspection without changing their pixels.

    @param outputs Actual generated atlas records with declared pose geometry.
    @param review_directory Explicit generated images owner, or the authoring default.
    @return None; write generated source review boards under the proof cache.
    """
    if review_directory is not None:
        from evidence import contained
        review_directory = contained(review_directory, ROOT / "mlm/reviews")
    review_directory = review_directory or REVIEW
    review_directory.mkdir(parents=True, exist_ok=True)
    tiles = []
    # The full source board shows every converted pose at an integer scale,
    # preserving original pixels rather than repainting or interpolating them.
    for output in outputs:
        sprite = Image.open(ROOT / output["path"]).convert("RGBA")
        large = sprite.resize((sprite.width * 4, sprite.height * 4), Image.Resampling.NEAREST)
        tiles.append((Path(output["path"]).stem, large))
    width = max(image.width for _, image in tiles) + 24
    height = sum(image.height + 36 for _, image in tiles) + 12
    review = Image.new("RGB", (width, height), (43, 45, 48))
    draw = ImageDraw.Draw(review)
    y = 12
    # Arrange complete sheets below labels without cropping native coverage.
    for label, image in tiles:
        draw.text((12, y), label, fill="white")
        review.paste(image, (12, y + 20), image)
        y += image.height + 36
    review.save(review_directory / "native-review.png")
    # A compact selection remains legible in an ordinary image viewer. The full
    # gallery above retains all poses for detailed source/heading inspection.
    selected = []
    for output in outputs:
        if output["asset_id"].startswith("purchase."):
            continue
        sheet = Image.open(ROOT / output["path"]).convert("RGBA")
        chosen = [sprite for sprite in output["sprites"] if sprite.get("direction") in ("W", "none")
                  or sprite.get("candidate_view") == 1]
        # A detail producer may own diagonal-only poses. The compact review
        # samples its first real pose; the full board still shows every pose.
        if not chosen:
            chosen = output["sprites"][:1]
        strip = Image.new("RGBA", (sum(sprite["box"][2] for sprite in chosen), output["native_cell"][1]))
        x = 0
        # Compact examples retain actual recorded crop geometry and pixel order.
        for sprite in chosen:
            left, top, width, height = sprite["box"]
            strip.paste(sheet.crop((left, top, left + width, top + height)), (x, 0))
            x += width
        selected.append((Path(output["path"]).stem, strip.resize((strip.width * 3, strip.height * 3), Image.Resampling.NEAREST)))
    # Purchase-only source leaves already have the full native review board;
    # their intentionally excluded compact overview need not invent a pose.
    if not selected:
        return
    compact = Image.new("RGB", (max(image.width for _, image in selected) + 24,
                                sum(image.height + 36 for _, image in selected) + 12), (43, 45, 48))
    draw = ImageDraw.Draw(compact)
    y = 12
    # Compact rows are inspection aids; original native atlas bytes stay intact.
    for label, image in selected:
        draw.text((12, y), label, fill="white")
        compact.paste(image, (12, y + 20), image)
        y += image.height + 36
    compact.save(review_directory / "summary-preview.png")


def check(report_path=None):
    """Refuse changed originals, missing poses, wrong dimensions and reserved colors.

    @param report_path Explicit selected source manifest, or the authoring owner's default.
    @return None; failures identify the actual source or derived artifact.
    """
    report = json.loads((report_path or OUTPUT / "atlas-manifest.json").read_text(encoding="utf-8"))
    sources = json.loads((ASSETS / "generation-record.json").read_text(encoding="utf-8"))["sources"]
    # Validate original identities independently before trusting derived pixels.
    for source in sources:
        if digest(repository_path(source["path"])) != source["sha256"]:
            raise ValueError(f"source hash mismatch: {source['path']}")
    # Every actual atlas must retain its declared native palette/canvas/alpha.
    for output in report["atlases"]:
        path = repository_path(output["path"])
        image = Image.open(path)
        # Canvas and transparent index preserve the native consumer's geometry
        # and color meaning across relocation or deterministic reconstruction.
        if image.mode != "P" or list(image.size) != output["size"] or image.info.get("transparency") != 0:
            raise ValueError(f"palette, size or transparency mismatch: {path}")
        if image.getpalette() != PALETTE_BYTES:
            raise ValueError(f"wrong DOS palette: {path}")
        if set(image.get_flattened_data()) & (FORBIDDEN - {0}):
            raise ValueError(f"reserved palette index: {path}")
        if digest(path) != output["sha256"]:
            raise ValueError(f"derived hash mismatch: {path}")
        # Every recorded pose must occupy a real nonempty crop inside this atlas.
        for sprite in output["sprites"]:
            x, y, width, height = sprite["box"]
            if not (0 <= x < x + width <= image.width and 0 <= y < y + height <= image.height):
                raise ValueError(f"sprite box leaves atlas: {sprite}")
            pixels = image.crop((x, y, x + width, y + height)).get_flattened_data()
            if not any(pixels):
                raise ValueError(f"empty converted pose: {sprite}")
    # Source conversion cannot claim runtime activation, and duplicated metadata
    # cannot manufacture pose coverage beyond the actual atlas records.
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
