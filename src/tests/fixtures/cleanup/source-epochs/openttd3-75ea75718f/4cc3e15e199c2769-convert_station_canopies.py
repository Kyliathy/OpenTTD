"""Register genuine canopy supports, structural roofs and native glass children.

@file convert_station_canopies.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task24.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only retained original pixels supply artwork. The compiled native geometry owns
sorting and glass attachment; this source converter grants no runtime activation.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_station_views as views
from create_station_art_guides import canopy_support_spec, project


loader = views.loader
OUTPUT = views.OUTPUT
PREFIX = "station-canopy-views"


def support_box(layout, longitudinal):
    """Register an entire original post between its real deck and roof contacts.

    @param layout Compiled native canopy geometry with unchanged sorting bounds.
    @param longitudinal Authored post centre along the railway, either2 or14.
    @return Half-open six-pixel-wide placement rectangle on the common64x64 canvas.
    """
    if longitudinal not in (2, 14):
        raise ValueError("canopy post must use one of the two declared contacts")
    _, centres, roof_height = canopy_support_spec(layout)
    x, y = centres[0 if longitudinal == 2 else 1]
    px, top = project(x, y, roof_height)
    _, foot = project(x, y, 2)
    return [int(px) - 3, int(top) - 1, int(px) + 3, int(foot) + 2]


def overlay_original(base, fitting):
    """Place original fitting pixels over an unchanged original supporting deck.

    @param base Indexed deck on the shared palette and native64x64 canvas.
    @param fitting Indexed original post source with transparent index0.
    @return Independent indexed parent with all non-fitting base pixels exact.
    """
    result = base.copy()
    result.putdata([new if new else old for old, new in zip(base.get_flattened_data(), fitting.get_flattened_data())])
    return result


def pane_components(glass):
    """Measure connected original glass regions without inventing pane pixels.

    @param glass Indexed64x64 native glass-child source; zero denotes absence.
    @return List of four-neighbour connected nonempty pixel-coordinate sets.
    """
    remaining = {(x, y) for y in range(64) for x in range(64) if glass.getpixel((x, y))}
    components = []
    # Native glass needs four separated real panes. Connectivity checks reject
    # a missing pane, an extra fragment or a cyan frame joining their interiors.
    while remaining:
        seed = remaining.pop()
        component, pending = {seed}, [seed]
        while pending:
            x, y = pending.pop()
            for neighbour in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                if neighbour in remaining:
                    remaining.remove(neighbour)
                    component.add(neighbour)
                    pending.append(neighbour)
        components.append(component)
    return components


def split_roof(original):
    """Separate explicitly authored cyan glass from neutral original structural steel.

    @param original Registered RGBA roof original, with opaque cyan pane interiors.
    @return Disjoint indexed structural roof, glass child and intact original composite.
    @throws ValueError If material selection loses a pane or palette coverage.
    """
    combined = loader.base.quantize(original, {})
    roof, glass = combined.copy(), combined.copy()
    roof_pixels, glass_pixels = [], []
    # The authoring brief reserves saturated cyan for glass and neutral steel
    # for structure. Select those retained material pixels; no guessed hole
    # mask, painted replacement glass or background erasure is permitted.
    for rgba, indexed in zip(original.get_flattened_data(), combined.get_flattened_data()):
        red, green, blue, alpha = rgba
        is_glass = alpha >= 128 and green >= 100 and green - red >= 25 and blue - red >= 25
        roof_pixels.append(0 if is_glass else indexed)
        glass_pixels.append(indexed if is_glass else 0)
    roof.putdata(roof_pixels)
    glass.putdata(glass_pixels)
    components = pane_components(glass)
    # Four substantial panes must survive at actual game scale; source-only
    # high-resolution detail cannot stand in for a readable native glass child.
    if len(components) != 4 or any(len(component) < 12 for component in components):
        raise ValueError("canopy original does not retain four separate native glass panes")
    if set(combined.get_flattened_data()) & (loader.base.FORBIDDEN - {0}):
        raise ValueError("canopy source contains a forbidden native palette index")
    return {"roof": roof, "glass": glass}, combined


def validate_canopy(parts, layout, bare, supports, whole_roof):
    """Verify native support contacts, train clearance and complete original roof ownership.

    @param parts Four indexed native owners: back/front/roof/glass.
    @param layout Actual compiled canopy geometry and screen-relative glass child.
    @param bare Exact approved bare back/front deck sources for this genuine axis.
    @param supports Two original support images with their longitudinal contacts.
    @param whole_roof Intact registered original roof before material separation.
    @return Indexed whole source preview; runtime transparency remains native-owned.
    @throws ValueError Before publication on displaced posts, missing panes or roof contact drift.
    """
    parent, centres, roof_height = canopy_support_spec(layout)
    supporting = "back" if parent == 0 else "front"
    other = "front" if parent == 0 else "back"
    if parts[other].tobytes() != bare[other].tobytes():
        raise ValueError("canopy source changed its unsupported original platform")
    union = set()
    # Entire individually authored posts must reach their deck and elevated
    # roof. A sorting height of2/7 does not authorize shortening their artwork.
    for (longitudinal, sprite), (wx, wy) in zip(supports, centres):
        box = support_box(layout, longitudinal)
        if sprite.getbbox() is None:
            raise ValueError("canopy support original is empty")
        occupied = {(x, y) for y in range(64) for x in range(64) if sprite.getpixel((x, y))}
        if any(not (box[0] <= x < box[2] and box[1] <= y < box[3]) for x, y in occupied):
            raise ValueError("canopy post source leaves its declared physical support")
        if occupied & union:
            raise ValueError("two canopy supports duplicate the same native position")
        union.update(occupied)
        # Contact tolerances include native cap/foot shading, not a free gap
        # between paid deck, original post and the elevated canopy underside.
        for height in (2, roof_height):
            px, py = map(int, project(wx, wy, height))
            if not any((x, y) in occupied for x in range(px - 2, px + 3) for y in range(py - 2, py + 3)):
                raise ValueError("canopy support misses its native deck or roof contact")
    # Foreground posts deliberately occlude narrow physical columns. All
    # other front pixels must remain the exact proven train-clear bare deck;
    # blanket empty-centre rules would incorrectly erase real near-side posts.
    for index, (current, original) in enumerate(zip(parts[supporting].get_flattened_data(), bare[supporting].get_flattened_data())):
        if (index % 64, index // 64) not in union and current != original:
            raise ValueError("canopy source changes a platform outside its two original supports")
    reconstructed = overlay_original(parts["roof"], parts["glass"])
    # Native relative glass must match the same original roof canvas exactly.
    # A stock child or duplicate opaque panes would defeat in-game translucency.
    if reconstructed.tobytes() != whole_roof.tobytes() or any(
            steel and pane for steel, pane in zip(parts["roof"].get_flattened_data(), parts["glass"].get_flattened_data())):
        raise ValueError("native roof/glass sources do not reconstruct their complete original")
    rise = layout["parts"][2]["extent"][2]
    # All four real roof corners must meet the modular native cell. This also
    # distinguishes genuine A/B slopes and genuine X/Y geometry at game scale.
    for u, v in ((0, 0), (16, 0), (16, 16), (0, 16)):
        z = roof_height + rise * (v / 16 if layout["role"] == 2 else 1 - v / 16)
        wx, wy = (u, v) if layout["axis"] == "X" else (v, u)
        px, py = map(int, project(wx, wy, z))
        if not any(whole_roof.getpixel((x, y)) for x in range(max(0, px - 2), min(64, px + 3))
                   for y in range(max(0, py - 2), min(64, py + 3))):
            raise ValueError(f"canopy roof misses a real modular corner: gfx{layout['graphics']} ({px},{py})")
    whole = parts["back"].copy()
    # This contact preview follows native parent/child order. True transparent
    # glass and train sorting remain the later packed/decoded/game proof scope.
    for part in ("front", "roof", "glass"):
        whole = overlay_original(whole, parts[part])
    return whole


def review_board(poses):
    """Compare actual canopy axes, separate native owners and adjoining roof seams.

    @param poses Four ordered layout/part/whole/roof-composite source tuples.
    @return RGB source-only native1x/4x and adjoining A/B contact sheet.
    """
    board = Image.new("RGB", (1280, 960), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((16, 12), "SOURCE ONLY: native canopy4..7, exact bare decks + original posts, separate structural roof/glass", fill="black")
    # Parallel views expose physical post, slope and pane consistency. Showing
    # owners separately keeps opaque composite previews from implying that
    # glass is baked into the structural roof's native packed image.
    for index, (layout, parts, whole, _) in enumerate(poses):
        x = 16 + index * 320
        draw.text((x, 44), f"gfx{layout['graphics']} {layout['axis']} role{layout['role']}", fill="black")
        for owner, image, top in (("whole", whole, 72), ("back", parts["back"], 352),
                                  ("front", parts["front"], 472), ("roof", parts["roof"], 592),
                                  ("glass", parts["glass"], 712)):
            draw.text((x, top - 18), owner, fill="black")
            rgba = image.convert("RGBA")
            scale = 4 if owner == "whole" else 1
            board.paste(rgba.resize((64 * scale, 64 * scale), Image.Resampling.NEAREST), (x, top),
                        rgba.resize((64 * scale, 64 * scale), Image.Resampling.NEAREST))
    # Adjacent native platform rows shift X by(+32,+16), Y by(-32,+16).
    # Their complementary A/B halves must meet at the same real ridge rather
    # than sharing an invented single-cell gable or a travel-direction alias.
    for axis, left in (("X", 32), ("Y", 672)):
        pair = Image.new("RGBA", (128, 96))
        a = next(row[3] for row in poses if row[0]["axis"] == axis and row[0]["role"] == 2).convert("RGBA")
        b = next(row[3] for row in poses if row[0]["axis"] == axis and row[0]["role"] == 3).convert("RGBA")
        pair.alpha_composite(a, (0 if axis == "X" else 32, 0))
        pair.alpha_composite(b, (32 if axis == "X" else 0, 16))
        draw.text((left, 820), f"{axis} adjoining native canopy halves; original roof pixels", fill="black")
        large = pair.resize((256, 192), Image.Resampling.NEAREST)
        board.paste(large, (left + 320, 748), large)
    return board


def build(record):
    """Build all four real ready0 canopy cells in memory after complete validation.

    @param record Immutable original/provenance record with explicit canopy selections.
    @return Partial source report and deterministic output-path-to-bytes mapping.
    """
    geometry = views.load_geometry()
    family = record["families"]["facility.rail-station"]
    selections = family["canopy_sources"]
    keys = [row["graphics"] for row in selections]
    # Every actual canopy role/axis needs independent sources. A repeated or
    # relabelled view must not hide one of the four missing native cells.
    if len(keys) != 4 or set(keys) != set(range(4, 8)):
        raise ValueError("canopy source matrix is missing or ambiguous")
    _, base_files = views.build(record)
    base_atlas = Image.open(io.BytesIO(base_files[OUTPUT / f"{views.PREFIX}-atlas.png"]))
    atlas = base_atlas.copy()
    atlas.putdata([0] * (256 * 256))
    files, poses, rows = {}, [], []
    # Each native cell reuses its already approved genuine-axis bare decks and
    # adds only original posts on the supported parent plus the elevated roof.
    for graphics in range(4, 8):
        layout = geometry["layouts"][graphics]
        selected = next(row for row in selections if row["graphics"] == graphics)
        bare_graphics = graphics % 2
        bare = {part: base_atlas.crop((index * 64, bare_graphics * 64, (index + 1) * 64, (bare_graphics + 1) * 64))
                for index, part in enumerate(("back", "front"))}
        parent, _, _ = canopy_support_spec(layout)
        supporting = "back" if parent == 0 else "front"
        support_rows = selected["supports"]
        # The two actual longitudinal contacts must each own a genuine original
        # post; duplicating one component cannot supply both authored views.
        if [row["longitudinal"] for row in support_rows] != [2, 14] or len({row["component"] for row in support_rows}) != 2:
            raise ValueError("canopy support contacts are missing or duplicated")
        parts = {part: sprite.copy() for part, sprite in bare.items()}
        supports = []
        for row in support_rows:
            source = next(candidate for candidate in record["candidates"] if candidate["id"] == row["source"])
            component = next(component for component in source["components"] if component["id"] == row["component"])
            if component["native_box"] != support_box(layout, row["longitudinal"]):
                raise ValueError("canopy support transform changes a native physical contact")
            sprite = views.native_component(record, {**row, "part": supporting}, layout)
            supports.append((row["longitudinal"], sprite))
            parts[supporting] = overlay_original(parts[supporting], sprite)
        roof_original = views.native_component_rgba(record, {**selected["roof"], "part": "roof"}, layout)
        roof_parts, whole_roof = split_roof(roof_original)
        parts.update(roof_parts)
        whole = validate_canopy(parts, layout, bare, supports, whole_roof)
        poses.append((layout, parts, whole, whole_roof))
        for index, part in enumerate(views.PARTS):
            atlas.paste(parts[part], (index * 64, (graphics - 4) * 64))
            rows.append({"native_graphics": graphics, "axis": layout["axis"], "role": layout["role"],
                         "state": "ready", "frame": 0, "part": part, "structurally_absent": False,
                         "box": [index * 64, (graphics - 4) * 64, (index + 1) * 64, (graphics - 3) * 64],
                         "anchor": [-31, -33], "native_packed_anchor": views.native_packed_anchor(layout, part),
                         "nonempty_pixels": sum(value != 0 for value in parts[part].get_flattened_data())})
        files[OUTPUT / f"{PREFIX}-gfx{graphics}-whole.png"] = views.png_bytes(whole)
    board = io.BytesIO()
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task24.2; archi-map-layers.md; source-only native canopy review")
    review_board(poses).save(board, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files[OUTPUT / f"{PREFIX}-atlas.png"] = views.png_bytes(atlas)
    files[OUTPUT / f"{PREFIX}-review.png"] = board.getvalue()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md task24.2; archi-map-layers.md",
              "asset_id": "facility.rail-station", "native_graphics": list(range(4, 8)), "state": "ready", "frame": 0,
              "source_rows": rows, "source_selections": selections, "nonempty_source_parts": 16,
              "source_coverage_complete": False, "runtime_activated": False,
              "glass_selection": "Retained opaque cyan original material: alpha>=128, green>=100, green-red>=25, blue-red>=25; four connected panes",
              "glass_native_owner": "Screen-relative child(0,0) of unchanged elevated roof; transparent native palette, packed anchor(0,0)",
              "remaining": "Construction/refusal/loading phases and full512-slot source closure; all packing/decoded/game proof remains master10.4D"}
    report["artifacts"] = [{"path": path.relative_to(loader.base.ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items()]
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def main():
    """Publish a complete validated ready0 canopy review without activating sprites.

    @return None; failed source or native registration leaves existing evidence intact.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true", required=True)
    parser.parse_args()
    record = json.loads(loader.RECORD.read_text(encoding="utf-8"))
    public = json.loads(loader.PUBLIC.read_text(encoding="utf-8"))
    views.validate_station_contract(next(row for row in public["assets"] if row["asset_id"] == "facility.rail-station"))
    report, files = build(record)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # No source artifact is written until the entire four-cell matrix passes;
    # a rejected original cannot replace the last accepted partial checkpoint.
    for path, payload in files.items():
        path.write_bytes(payload)
    print(json.dumps({"native_graphics": report["native_graphics"], "nonempty_source_parts": 16,
                      "source_coverage_complete": False, "runtime_activated": False}))


if __name__ == "__main__":
    main()
