"""Create native track-port references for selected tube guideway originals.

@file create_tube_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups47B-47G.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48C native loading platforms.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.4.
These procedural diagrams are geometry guides, never authored game sprites.
"""

from pathlib import Path

from PIL import Image, ImageDraw, PngImagePlugin


ROOT = Path(__file__).resolve().parents[2]
GUIDES = ROOT / "media/multilayer/guides/ground"
PORTS = {"NE": (47, 8), "SE": (47, 24), "SW": (15, 24), "NW": (15, 8)}
AXES = (("X: NE to SW", "NE", "SW"), ("Y: NW to SE", "NW", "SE"))
CURVES = {
    "N": {"track": "Upper", "ports": ("NW", "NE"), "path": ((17, 7), (45, 7))},
    "S": {"track": "Lower", "ports": ("SW", "SE"), "path": ((17, 23), (45, 23))},
    "W": {"track": "Left", "ports": ("NW", "SW"), "path": ((15, 8), (15, 22))},
    "E": {"track": "Right", "ports": ("NE", "SE"), "path": ((47, 8), (47, 22))},
}


def create_straight_guide():
    """Publish both independently authored native axes with fixed edge contacts.

    @return PNG reference path inside the repository; no source art is changed.
    """
    GUIDES.mkdir(parents=True, exist_ok=True)
    guide = Image.new("RGB", (1080, 440), (34, 38, 44))
    labels = ImageDraw.Draw(guide)
    labels.text((16, 14), "TUBE STRAIGHT: native track geometry; diagram colours are not source art", fill="white")
    # Each direction names a real native track axis. Reversal on the same
    # undirected axis retains its geometry; rotating a flat bitmap cannot
    # replace the separately authored other engine view or fixed lighting.
    for index, (name, first, second) in enumerate(AXES):
        tile = Image.new("RGB", (64, 31), (48, 52, 58))
        draw = ImageDraw.Draw(tile)
        draw.polygon(((31, 0), (63, 16), (31, 30), (0, 16)), outline=(170, 180, 190))
        draw.line((PORTS[first], PORTS[second]), fill=(60, 115, 155), width=11)
        draw.line((PORTS[first], PORTS[second]), fill=(245, 200, 80), width=1)
        # The blue band reserves the moving capsule's central channel. Art
        # adds low perimeter shells around it, preserving clear occupancy.
        for port in (first, second):
            px, py = PORTS[port]
            draw.rectangle((px - 1, py - 1, px + 1, py + 1), fill=(245, 125, 90))
        left = index * 536 + 20
        guide.paste(tile.resize((512, 248), Image.Resampling.NEAREST), (left, 66))
        labels.text((left, 42), name, fill="white")
        labels.text((left, 326), f"Ports: {first}{PORTS[first]} / {second}{PORTS[second]}", fill="white")
        labels.text((left, 346), "64x31; anchor(-31,0); one world tile; fixed top-left light", fill="white")
    labels.text((16, 376), "Low sealed-shell cutaway: leave the capsule channel open; no track, floor plate, vehicle, text or guide pixels in final art.", fill="white")
    labels.text((16, 396), "Two authored axes; construction / powered / unpowered; aligned back/front parts share the same canvas and contacts.", fill="white")
    target = GUIDES / "tube-straight-geometry.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Group47B; geometry reference only")
    metadata.add_text("NativePorts", str(PORTS))
    guide.save(target, pnginfo=metadata)
    return target


def create_curve_guide():
    """Publish actual native corner chords and the fixed neighbouring mouth contacts.

    @return Labeled geometry-only PNG path; retained straight references stay unchanged.
    """
    GUIDES.mkdir(parents=True, exist_ok=True)
    guide = Image.new("RGB", (1080, 850), (34, 38, 44))
    labels = ImageDraw.Draw(guide)
    labels.text((16, 14), "TUBE CORNERS: native Upper / Lower / Left / Right; geometry diagram, never final source pixels", fill="white")
    # The four native pieces connect different pairs of physical tile edges.
    # The gold chord follows VehicleEnterTileCoordinates/GetNewVehiclePos,
    # including native7/23 subtile rounding; red marks neighbouring mouths.
    for index, (corner, geometry) in enumerate(CURVES.items()):
        tile = Image.new("RGB", (64, 31), (48, 52, 58))
        draw = ImageDraw.Draw(tile)
        draw.polygon(((31, 0), (63, 16), (31, 30), (0, 16)), outline=(170, 180, 190))
        draw.line(geometry["path"], fill=(60, 115, 155), width=7)
        draw.line(geometry["path"], fill=(245, 200, 80), width=1)
        for port in geometry["ports"]:
            px, py = PORTS[port]
            draw.rectangle((px - 1, py - 1, px + 1, py + 1), fill=(245, 125, 90))
        left, top = index % 2 * 536 + 20, index // 2 * 376 + 66
        guide.paste(tile.resize((512, 248), Image.Resampling.NEAREST), (left, top))
        labels.text((left, top - 24), f"{corner}: native {geometry['track']} / mouths {geometry['ports']}", fill="white")
        labels.text((left, top + 260), f"Actual capsule centre chord: {geometry['path']}", fill="white")
        labels.text((left, top + 280), "64x31; anchor(-31,0); fixed top-left light; low foreground rim", fill="white")
    labels.text((16, 810), "Leave gold path alpha-clear; align red mouths to selected straight shells. No guide colours/marks in final art.", fill="white")
    target = GUIDES / "tube-curve-geometry.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Group47C; native geometry reference only")
    guide.save(target, pnginfo=metadata)
    return target


def create_end_guide():
    """Label four real edge end-cap positions without authoring game pixels.

    @return Geometry-only PNG; actual GetTubeGuidewayEnds selects a cap from native adjacency.
    """
    GUIDES.mkdir(parents=True, exist_ok=True)
    guide = Image.new("RGB", (1080, 850), (34, 38, 44))
    labels = ImageDraw.Draw(guide)
    labels.text((16, 14), "TUBE END CAPS: physical edge ends only; native geometry guide, never final source pixels", fill="white")
    # The direction identifies the unconnected physical mouth, not a travel
    # heading or a power/permission wait. A cap supplements the existing casing
    # locally; it must not paint another long guideway or an opaque floor slab.
    for index, (port, (px, py)) in enumerate(PORTS.items()):
        tile = Image.new("RGB", (64, 31), (48, 52, 58))
        draw = ImageDraw.Draw(tile)
        draw.polygon(((31, 0), (63, 16), (31, 30), (0, 16)), outline=(170, 180, 190))
        opposite = {"NE": "SW", "SE": "NW", "SW": "NE", "NW": "SE"}[port]
        draw.line((PORTS[opposite], (px, py)), fill=(60, 115, 155), width=5)
        draw.rectangle((px - 5, py - 5, px + 5, py + 2), outline=(245, 125, 90))
        draw.point((px, py), fill=(245, 200, 80))
        left, top = index % 2 * 536 + 20, index // 2 * 376 + 66
        guide.paste(tile.resize((512, 248), Image.Resampling.NEAREST), (left, top))
        labels.text((left, top - 24), f"{port} end / contact({px},{py}); existing approach from {opposite}", fill="white")
        labels.text((left, top + 260), "Small low pressure end plate at this mouth; no whole stub/track/floor", fill="white")
        labels.text((left, top + 280), "64x31; anchor(-31,0); independently rendered; fixed top-left light", fill="white")
    labels.text((16, 810), "GetTubeGuidewayEnds is physical adjacency only: an open route with a power/signal/turn wait does not acquire a cap.", fill="white")
    target = GUIDES / "tube-end-geometry.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Group47E; native end geometry only")
    guide.save(target, pnginfo=metadata)
    return target


def create_end_facing_guide():
    """Show the actual projected pressure-face planes after ambiguous facing output.

    @return New versioned geometry PNG; the earlier hashed guide is preserved byte-for-byte.
    """
    GUIDES.mkdir(parents=True, exist_ok=True)
    guide = Image.new("RGB", (1080, 850), (34, 38, 44))
    labels = ImageDraw.Draw(guide)
    labels.text((16, 14), "TUBE END FACE PLANES: orange is geometry only; keep fixed camera/light when rendering the actual model", fill="white")
    # The two opposing physical ends share a projected tangent but expose
    # different model seating sides. SE/NW have the opposite tangent from
    # NE/SW; copying the first face into every cell cannot supply those views.
    for index, (port, (px, py)) in enumerate(PORTS.items()):
        tile = Image.new("RGB", (64, 31), (48, 52, 58))
        draw = ImageDraw.Draw(tile)
        draw.polygon(((31, 0), (63, 16), (31, 30), (0, 16)), outline=(170, 180, 190))
        if port in ("NE", "SW"):
            face = ((px - 4, py - 6), (px + 4, py - 2), (px + 4, py + 2), (px - 4, py - 2))
            tangent = "top/bottom edges slope DOWN RIGHT (+1/2)"
        else:
            face = ((px - 4, py - 2), (px + 4, py - 6), (px + 4, py - 2), (px - 4, py + 2))
            tangent = "top/bottom edges slope UP RIGHT (-1/2)"
        draw.polygon(face, fill=(200, 105, 75), outline=(245, 200, 80))
        draw.line(face[:2], fill=(230, 235, 240), width=1)
        draw.point((px, py), fill=(245, 125, 90))
        left, top = index % 2 * 536 + 20, index // 2 * 376 + 66
        guide.paste(tile.resize((512, 248), Image.Resampling.NEAREST), (left, top))
        labels.text((left, top - 24), f"{port} physical end: {tangent}", fill="white")
        labels.text((left, top + 260), "Same low bolted gray/blue cap, independently rendered; no bitmap rotation", fill="white")
        labels.text((left, top + 280), f"Native contact({px},{py});64x31; anchor(-31,0); fixed top-left light", fill="white")
    labels.text((16, 810), "The face plane follows actual isometric track cross-section. No guide colours, diamond, marks or text in final art.", fill="white")
    target = GUIDES / "tube-end-facings-v2.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Group47E; projected physical end planes only")
    guide.save(target, pnginfo=metadata)
    return target


def create_conduit_guide():
    """Publish floor-level service cable axes and their four native half-branches.

    @return Geometry-only guide path; saved TubePowerConnection paths own placement.
    """
    GUIDES.mkdir(parents=True, exist_ok=True)
    guide = Image.new("RGB", (1080, 820), (34, 38, 44))
    labels = ImageDraw.Draw(guide)
    labels.text((16, 14), "SERVICE CONDUIT: floor cable only; geometry diagram, never final art", fill="white")
    # A saved simple utility corridor joins orthogonally adjacent world cells.
    # Four center-to-edge branches can present only its actual previous/next
    # cells, including turns, without drawing an uninstalled route or hub arm.
    for index, (direction, point) in enumerate(PORTS.items()):
        tile = Image.new("RGB", (64, 31), (48, 52, 58))
        draw = ImageDraw.Draw(tile)
        draw.polygon(((31, 0), (63, 16), (31, 30), (0, 16)), outline=(130, 140, 150))
        draw.line(((31, 16), point), fill=(220, 155, 65), width=2)
        draw.rectangle((30, 15, 32, 17), fill=(95, 130, 155))
        left, top = index % 2 * 536 + 20, index // 2 * 330 + 65
        guide.paste(tile.resize((512, 248), Image.Resampling.NEAREST), (left, top))
        labels.text((left, top - 20), f"{direction}: center(31,16) to edge{point}; floor-level only", fill="white")
        labels.text((left, top + 260), "64x31 / anchor(-31,0); draw beneath vehicles/buildings; no foreground wall", fill="white")
    labels.text((16, 740), "Author two full independent axes X=SW/NE and Y=NW/SE. Original half selection supplies four actual path branches.", fill="white")
    labels.text((16, 765), "No floor plate, track, capsule, tall pipe or generation. Powered status uses the shared actual electrical producer only.", fill="white")
    target = GUIDES / "tube-conduit-geometry.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Group47G; TubePowerConnection utility_path geometry only")
    guide.save(target, pnginfo=metadata)
    return target


def create_terminal_guide():
    """Publish both actual one-cell through-platform axes before model authoring.

    @return Geometry-only reference path; native StationGfx owns cell roles/capacity.
    """
    GUIDES.mkdir(parents=True, exist_ok=True)
    guide = Image.new("RGB", (1080, 720), (34, 38, 44))
    labels = ImageDraw.Draw(guide)
    labels.text((16, 14), "TUBE LOADING PLATFORM: actual native one-cell through geometry; diagram only", fill="white")
    # A native station cell has one actual through track and two side ledges.
    # X/Y axes need independently lit sources; depot door rotations cannot
    # replace these platform views or enlarge their one-cell paid footprint.
    for index, (axis, first, second, normal) in enumerate((("X / NE-SW", "NE", "SW", (6, 3)), ("Y / NW-SE", "NW", "SE", (-6, 3)))):
        tile = Image.new("RGB", (64, 64), (48, 52, 58))
        draw = ImageDraw.Draw(tile)
        draw.polygon(((31, 33), (63, 48), (31, 63), (0, 48)), outline=(130, 140, 150))
        start, end = PORTS[first], PORTS[second]
        start, end = (start[0], start[1] + 33), (end[0], end[1] + 33)
        # Blue marks the two low loading ledges; the gold centerline is an
        # unobstructed capsule path, never a painted rail or cargo buffer.
        for sign in (-1, 1):
            dx, dy = normal[0] * sign, normal[1] * sign
            draw.line(((start[0] + dx, start[1] + dy), (end[0] + dx, end[1] + dy)), fill=(80, 135, 170), width=3)
        draw.line((start, end), fill=(230, 175, 70), width=1)
        for px, py in (start, end):
            draw.rectangle((px - 1, py - 1, px + 1, py + 1), fill=(245, 125, 90))
        left = index * 536 + 20
        guide.paste(tile.resize((512, 512), Image.Resampling.NEAREST), (left, 65))
        labels.text((left, 42), axis + ": bare native role0; two low parallel side ledges", fill="white")
        labels.text((left, 590), "64x64 / anchor(-31,-33); actual floor begins at y33; fixed top-left light", fill="white")
    labels.text((16, 632), "Four actual roles per axis: bare; small rear building; canopy A/rear pillars; canopy B/front pillars.", fill="white")
    labels.text((16, 656), "Start with a bare X model. No campus, depot wall, vehicle, floor slab or glyphs in final source. Keep through mouths open.", fill="white")
    target = GUIDES / "tube-terminal-geometry.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Group48C; native station graphics0..7 geometry only")
    guide.save(target, pnginfo=metadata)
    return target


if __name__ == "__main__":
    print(create_straight_guide())
    print(create_curve_guide())
    print(create_end_guide())
    print(create_end_facing_guide())
    print(create_conduit_guide())
    print(create_terminal_guide())
