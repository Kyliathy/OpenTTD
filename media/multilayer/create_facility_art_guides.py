"""Render one-tile depot geometry for facility raster authoring.

@file create_facility_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups23/26/54B
@see docs/zz-rhr/upgrades/2026-10/graphics/r2mlvs13-a-sprite-contract.md
These diagrams describe native placement; they never supply game artwork.
"""

from pathlib import Path

from PIL import Image, ImageDraw, PngImagePlugin


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "media/multilayer/guides/m1"
DOORS = ["NE", "SE", "SW", "NW"]


def collector_cell_roles(direction):
    """Rotate model roles logically over the fixed native floor, never a raster.

    @param direction Actual access-side presentation NE/SE/SW/NW.
    @return Nine row-major world cells with original machinery roles and pipe sides.
    """
    turns = {"SE": 0, "NE": 1, "NW": 2, "SW": 3}[direction]
    roles = [("capped borehead", ["SE"]), ("insulated pipe elbow", ["SW", "SE"]),
             ("capped borehead", ["NE"]), ("insulated T manifold", ["NW", "SW", "SE"]),
             ("central pressure drum", ["NE", "NW", "SE"]), ("low service hatch", []),
             ("instrument cabinet and outward valve", ["NW", "SE"]),
             ("insulated pipe elbow", ["SW", "NW"]), ("capped borehead", ["NE"])]
    sides = ["SE", "NE", "NW", "SW"]
    result = []
    # The whole machinery arrangement turns on its real3x3 floor. Original
    # source views must rerender those faces under the same camera/light;
    # changing cell coordinates here never supplies a missing image view.
    for index, (role, ports) in enumerate(roles):
        x, y = index % 3, index // 3
        for unused in range(turns):
            x, y = 2 - y, x
        result.append({"world_cell": [x, y], "role": role,
                       "pipe_sides": [sides[(sides.index(side) + turns) % 4] for side in ports]})
    return sorted(result, key=lambda row: (row["world_cell"][1], row["world_cell"][0]))


def create_collector_cell_guide(direction, world_cells=None):
    """Show nine independent source slots, native floor and shared pipe heights.

    @param direction Actual access-side presentation whose model cells are labelled.
    @param world_cells Optional actual cells needing an independent fitting correction.
    @return Repository reference PNG path; labels and blue dots are geometry only.
    """
    roles = collector_cell_roles(direction)
    if world_cells is not None:
        roles = [row for row in roles if row["world_cell"] in world_cells]
        if len(roles) != len(world_cells):
            raise ValueError("collector correction names absent or repeated native cells")
    columns = 3 if world_cells is None else 2
    rows = (len(roles) + columns - 1) // columns
    guide = Image.new("RGB", (columns * 336 + 16, rows * 344 + 56), (238, 241, 244))
    draw = ImageDraw.Draw(guide)
    draw.text((16, 12), f"GEOMETRY ONLY: collector {direction}; nine INDEPENDENT one-tile source cells, row-major world order", fill="black")
    endpoints = {"NE": (47, 32), "SE": (47, 48), "SW": (15, 48), "NW": (15, 32)}
    # Every module keeps one native tile and common pipe centre eight pixels
    # above its floor edge. These identical contacts expose seams when the
    # genuine cell artwork is assembled at the actual world32/16 pitch.
    for index, row in enumerate(roles):
        tile = Image.new("RGB", (64, 64), (238, 241, 244))
        pen = ImageDraw.Draw(tile)
        pen.polygon([(31, 33), (63, 48), (31, 63), (0, 48)], fill=(183, 192, 198), outline=(55, 75, 85))
        for side in row["pipe_sides"]:
            point = endpoints[side]
            pen.line([(31, 40), point], fill=(30, 160, 185), width=2)
            pen.ellipse((point[0] - 1, point[1] - 1, point[0] + 1, point[1] + 1), fill=(30, 160, 185))
        left, top = 32 + (index % columns) * 336, 48 + (index // columns) * 344
        guide.paste(tile.resize((256, 256), Image.Resampling.NEAREST), (left, top))
        draw.text((left, top + 266), str(row["world_cell"]) + " " + row["role"], fill="black")
        draw.text((left, top + 286), "Pipe sides: " + "/".join(row["pipe_sides"]), fill="black")
    suffix = "" if world_cells is None else "-corrections"
    target = ROOT / f"media/multilayer/guides/ground/collector-cells-{direction.lower()}{suffix}-v1.png"
    target.parent.mkdir(parents=True, exist_ok=True)
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Group54B; archi-map-layers.md; geometry only")
    guide.save(target, pnginfo=metadata)
    return target


def create_collector_campus_guide(direction):
    """Show one connected collector arrangement on its real nine-cell floor.

    @param direction Genuine access-side presentation to rerender with a fixed camera.
    @return Repository geometry reference for a whole original model, never game art.
    """
    native = Image.new("RGB", (256, 192), (235, 239, 243))
    pen = ImageDraw.Draw(native)
    roles = collector_cell_roles(direction)
    centers = {}
    # Native world X runs down-left and Y down-right. All nine geometry
    # cells remain one industry; coloured symbols describe model placement,
    # never an independent heat source, inventory or utility endpoint.
    for row in roles:
        x, y = row["world_cell"]
        cx, cy = 128 + 32 * (y - x), 79 + 16 * (x + y)
        centers[tuple(row["world_cell"])] = (cx, cy)
        pen.polygon([(cx, cy - 15), (cx + 32, cy), (cx, cy + 15), (cx - 31, cy)],
                    fill=(183, 192, 198), outline=(80, 95, 105))
    ends = {"NE": (16, -16), "SE": (16, 0), "SW": (-16, 0), "NW": (-16, -16)}
    # The diagram joins half-pipes across their common physical cell edge.
    # A whole original view can preserve those connections before lossless
    # native packing, avoiding separate floor-image resampling at each joint.
    for row in roles:
        cx, cy = centers[tuple(row["world_cell"])]
        for side in row["pipe_sides"]:
            dx, dy = ends[side]
            pen.line([(cx, cy - 8), (cx + dx, cy + dy)], fill=(30, 145, 180), width=2)
        if row["role"] == "capped borehead":
            pen.ellipse((cx - 5, cy - 13, cx + 5, cy - 3), fill=(30, 145, 180))
        elif row["role"] == "central pressure drum":
            pen.rectangle((cx - 5, cy - 18, cx + 5, cy - 4), fill=(30, 145, 180))
        elif row["role"] == "instrument cabinet and outward valve":
            pen.rectangle((cx - 4, cy - 16, cx + 4, cy - 4), fill=(30, 145, 180))
    page = Image.new("RGB", (1024, 848), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    draw.text((16, 12), f"GEOMETRY ONLY: one collector campus, {direction}; actual3x3/nine native cells, fixed camera/light", fill="black")
    page.paste(native.resize((1024, 768), Image.Resampling.NEAREST), (0, 40))
    draw.text((16, 820), "Three circular boreheads; central drum; one cabinet. Blue marks are placement guides; never paste them as artwork.", fill="black")
    target = ROOT / f"media/multilayer/guides/ground/collector-campus-{direction.lower()}-v1.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md54B; archi-map-layers.md; whole-model geometry only")
    page.save(target, pnginfo=metadata)
    return target


def create_collector_guide():
    """Show the canonical nine-cell collector floor at actual native pitch.

    @return Repository reference PNG path; geometry labels are never game art.
    """
    native = Image.new("RGB", (256, 192), (238, 241, 244))
    pen = ImageDraw.Draw(native)
    # The native IIDS layout is row-major, with nine independent TIDS cells.
    # Project each complete world cell at32/16 pitch; a single tile cannot
    # acquire the physical floor or input ownership of this whole collector.
    for y in range(3):
        for x in range(3):
            cx, cy = 128 + 32 * (y - x), 80 + 16 * (x + y)
            pen.polygon([(cx, cy - 16), (cx + 32, cy), (cx, cy + 16), (cx - 32, cy)],
                        fill=(183, 192, 198), outline=(55, 75, 85))
            pen.text((cx - 8, cy - 4), f"{x},{y}", fill=(35, 45, 55))
    guide = Image.new("RGB", (1024, 848), (238, 241, 244))
    draw = ImageDraw.Draw(guide)
    draw.text((16, 12), "GEOMETRY ONLY: native collector3x3/nine cells; fixed camera and top-left light", fill="black")
    guide.paste(native.resize((1024, 768), Image.Resampling.NEAREST), (0, 40))
    draw.text((16, 820), "Native RemapCoords: X runs down-left; Y down-right. Each cell64x64/anchor(-31,-33). Keep all nine cells.", fill="black")
    target = ROOT / "media/multilayer/guides/ground/collector-nine-cell-v2.png"
    target.parent.mkdir(parents=True, exist_ok=True)
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Group54B; archi-map-layers.md; geometry reference only")
    guide.save(target, pnginfo=metadata)
    return target


def create_depot_guide():
    """Show fixed footprints and the visible walls for each real depot entry.

    @return Repository PNG path, with geometry-only owner metadata.
    """
    guide = Image.new("RGB", (1152, 368), (238, 241, 244))
    draw = ImageDraw.Draw(guide)
    draw.text((16, 12), "GEOMETRY ONLY: one square world tile; floor diamond 2:1; native canvas 64x64", fill="black")
    # A far entrance lies behind the visible near walls. These independent
    # views prevent a mirrored near doorway from impersonating a rear view.
    for index, direction in enumerate(DOORS):
        tile = Image.new("RGB", (64, 64), (238, 241, 244))
        pen = ImageDraw.Draw(tile)
        pen.polygon([(31, 33), (63, 48), (31, 63), (0, 48)], fill=(190, 199, 205), outline=(80, 90, 100))
        pen.polygon([(4, 20), (31, 34), (31, 61), (4, 47)], fill=(117, 132, 146))
        pen.polygon([(31, 34), (58, 20), (58, 47), (31, 61)], fill=(84, 103, 119))
        pen.polygon([(31, 6), (58, 20), (31, 34), (4, 20)], fill=(156, 174, 185), outline=(70, 80, 90))
        if direction == "SE":
            pen.polygon([(38, 35), (51, 28), (51, 46), (38, 53)], fill=(28, 35, 43))
        if direction == "SW":
            pen.polygon([(11, 28), (24, 35), (24, 53), (11, 46)], fill=(28, 35, 43))
        guide.paste(tile.resize((256, 256), Image.Resampling.NEAREST), (index * 288 + 16, 50))
        draw.text((index * 288 + 24, 320), direction + (": FAR entry; opaque near wall" if direction in ("NE", "NW") else ": NEAR visible open mouth"), fill="black")
    draw.text((16, 348), "Fixed camera and top-left light. Rail terminates inside ONE depot; no long plaza. Anchor (-31,-33).", fill="black")
    path = ASSETS / "depot-direction-guide.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Groups 23/26; geometry reference only")
    guide.save(path, pnginfo=metadata)
    return path


def create_receiver_guide(kind):
    """Render fixed native receiver cells and their approved authoring roles.

    @param kind Earth or rock, selecting its measured native world layout.
    @return Repository PNG path with world-cell order and geometry metadata.
    """
    if kind == "earth":
        cells = [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2)]
        roles = ["receiving hopper", "brown soil bank", "retaining wall/drain", "soil belt/pump", "flat receiving yard", "site service hut"]
        columns = 6
    elif kind == "rock":
        cells = [(x, y) for x in range(4) for y in range(4)]
        roles = ["intake hopper", "raw stone pile", "stone belt", "intake yard", "crusher jaws", "sorting drum", "aggregate belt", "rubble pile",
                 "aggregate pile", "residue pile", "dust/filter plant", "retaining wall", "site service hut", "flat paved yard", "stock bank", "drain/settling basin"]
        columns = 4
    else:
        raise ValueError("unknown receiver geometry")
    rows = (len(cells) + columns - 1) // columns
    guide = Image.new("RGB", (columns * 240, rows * 180 + 42), (238, 241, 244))
    draw = ImageDraw.Draw(guide)
    draw.text((12, 10), kind.upper() + " GEOMETRY ONLY: fixed layout; each independent cell is ONE tile / 64x64; no rotation", fill="black")
    # Labeled cells follow the native layout's original column-major order.
    # Artwork stays inside each diamond so assembled neighbours share corners.
    for index, (cell, role) in enumerate(zip(cells, roles)):
        left, top = (index % columns) * 240 + 24, (index // columns) * 180 + 40
        diamond = [(left + 96, top + 20), (left + 191, top + 67), (left + 96, top + 114), (left, top + 67)]
        draw.polygon(diamond, fill=(183, 192, 198), outline=(55, 75, 85))
        draw.text((left, top + 125), f"cell {index}: ({cell[0]},{cell[1]}) {role}", fill="black")
        draw.text((left, top + 144), "south tip aligned; top-left light", fill="black")
    path = ASSETS / (kind + "-receiver-cell-guide.png")
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Groups37-38; geometry reference only")
    metadata.add_text("WorldCells", str(cells))
    guide.save(path, pnginfo=metadata)
    return path


if __name__ == "__main__":
    print(create_depot_guide())
    print(create_receiver_guide("earth"))
    print(create_receiver_guide("rock"))
