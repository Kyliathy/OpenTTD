"""Draw actual native station placement references; these diagrams are never game art.

@file create_station_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The compiled descriptor supplies every box. Guides preserve the native track
corridor, four modular roles, two true axes and independent elevated roofs.
"""

from pathlib import Path

from PIL import Image, ImageDraw, PngImagePlugin

from tools.ground_station_contract import load_geometry, ROLES


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "media/multilayer/guides/ground"
COLORS = ((55, 112, 156, 200), (225, 156, 42, 200), (102, 170, 186, 150))


def project(x, y, z):
    """Project native local coordinates onto the actual64x64 building canvas.

    @param x Native tile-local X world coordinate.
    @param y Native tile-local Y world coordinate.
    @param z Native world height, relative to the tile floor.
    @return Pixel reference with native anchor(-31,-33).
    """
    return 31 + 2 * (y - x), 33 + x + y - z


def native_reference(layout):
    """Draw one actual modular role with separate parent silhouettes and track axis.

    @param layout One checked compiled graphics/axis/part geometry row.
    @return Reference-only native RGBA image; no authored sprite pixels.
    """
    image = Image.new("RGBA", (64, 64), (242, 245, 247, 255))
    draw = ImageDraw.Draw(image)
    draw.polygon([project(0, 0, 0), project(16, 0, 0), project(16, 16, 0), project(0, 16, 0)],
                 fill=(219, 224, 228, 255), outline=(100, 115, 125, 255))
    # World sorting parents remain separate. Their projected boxes expose the
    # elevated canopy and physical strip geometry without replacing real track
    # ground, a moving train or the actual renderer's parent/child ordering.
    for index, part in enumerate(layout["parts"]):
        if part is None:
            continue
        x, y, z = part["origin"]
        width, depth, height = part["extent"]
        polygon = [project(x, y, z + height), project(x + width, y, z + height),
                   project(x + width, y + depth, z + height), project(x, y + depth, z + height)]
        draw.polygon(polygon, fill=COLORS[index], outline=(35, 55, 75, 255))
        if index == 2:
            continue
        # Height guides identify sorting volume only. They do not assert that
        # every original must fill its whole native box with opaque machinery.
        for cx, cy in ((x, y), (x + width, y), (x + width, y + depth), (x, y + depth)):
            draw.line([project(cx, cy, z), project(cx, cy, z + height)], fill=(35, 55, 75, 255))
    ends = (project(0, 8, 0), project(16, 8, 0)) if layout["axis"] == "X" else (project(8, 0, 0), project(8, 16, 0))
    draw.line(ends, fill=(210, 35, 75, 255), width=1)
    return image


def build():
    """Write labeled native placement references from the compiled source contract.

    @return Paths to the role overview and independent first-loader reference.
    """
    geometry = load_geometry()
    overview = Image.new("RGB", (1152, 768), (242, 245, 247))
    draw = ImageDraw.Draw(overview)
    draw.text((16, 12), "GEOMETRY ONLY: conventional station; native X/NE and Y/SE, four modular roles", fill="black")
    # One row per native role and one column per genuine axis prevents a draft
    # whole campus or four travel directions from substituting for legal tiles.
    for layout in geometry["layouts"]:
        column, row = layout["graphics"] % 2, layout["role"]
        x, y = 32 + column * 576, 45 + row * 178
        overview.paste(native_reference(layout).resize((160, 160), Image.Resampling.NEAREST).convert("RGB"), (x, y))
        draw.text((x + 180, y + 30), f"gfx{layout['graphics']} Axis{layout['axis']} {ROLES[row]}", fill="black")
        draw.text((x + 180, y + 54), "BLUE rear / GOLD front / CYAN elevated roof", fill="black")
        draw.text((x + 180, y + 78), "RED real track axis; keep its corridor open", fill="black")
        draw.text((x + 180, y + 102), "Native canvas64x64, anchor(-31,-33)", fill="black")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    paths = [OUTPUT / "conventional-station-native-roles-v1.png", OUTPUT / "conventional-station-loader-x-geometry-v1.png"]
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md Group24; archi-map-layers.md; geometry reference only")
    overview.save(paths[0], pnginfo=metadata)
    native_reference(geometry["layouts"][2]).resize((512, 512), Image.Resampling.NEAREST).save(paths[1], pnginfo=metadata)
    return paths


def inset_loader_reference(axis="X"):
    """Put an actual native loader volume inside a generous isolated source margin.

    @param axis Genuine native X/Y railway axis; defaultX preserves its approved guide.
    @return New reference path with native pixel(0,0) at source(256,256), scale8.
    """
    if axis not in ("X", "Y"):
        raise ValueError("station loader reference requires an actual native railway axis")
    layout = load_geometry()["layouts"][2 if axis == "X" else 3]
    image = Image.new("RGBA", (1024, 1024), (242, 245, 247, 255))
    image.paste(native_reference(layout).resize((512, 512), Image.Resampling.NEAREST), (256, 256))
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md Group24; archi-map-layers.md; geometry only; native origin256,256 scale8")
    target = OUTPUT / f"conventional-station-loader-{axis.lower()}-inset-geometry-v1.png"
    image.save(target, pnginfo=metadata)
    return target


def native_parent_reference(layout, parent):
    """Draw one actual parent volume without another platform influencing its placement.

    @param layout Checked compiled station geometry row.
    @param parent Native sorting parent index0..2; this guide uses low platform/building parents.
    @return Native64x64 reference diagram for an independent original cutout.
    """
    part = layout["parts"][parent]
    if part is None:
        raise ValueError("absent native station parent cannot supply a physical guide")
    x, y, z = part["origin"]
    width, depth, height = part["extent"]
    corners = [(x, y), (x + width, y), (x + width, y + depth), (x, y + depth)]
    bottom = [project(cx, cy, z) for cx, cy in corners]
    top = [project(cx, cy, z + height) for cx, cy in corners]
    image = Image.new("RGBA", (64, 64), (242, 245, 247, 255))
    draw = ImageDraw.Draw(image)
    # Visible native sides and top preserve the actual world axes, footprint
    # and height. This diagram creates no raster artwork and never rotates a
    # previously authored view to manufacture a missing axis.
    draw.polygon([top[1], top[2], bottom[2], bottom[1]], fill=(42, 80, 110, 255), outline=(25, 45, 65, 255))
    draw.polygon([top[2], top[3], bottom[3], bottom[2]], fill=(55, 112, 156, 255), outline=(25, 45, 65, 255))
    draw.polygon(top, fill=(98, 143, 173, 255), outline=(25, 45, 65, 255))
    return image


def canopy_support_spec(layout):
    """Locate two authored posts within the actual native supporting platform.

    @param layout Checked canopy graphics4..7 row with the original sorting boxes.
    @return Parent index, world-space post centres and real elevated-roof height.
    @throws ValueError If the native row has no canopy or its support strip changed.
    """
    # Rear/front canopy halves support their own roof from different platforms.
    # Native sorting heights remain untouched: physical posts reach the roof
    # even where the original front/rear sorting box is deliberately only2 high.
    if layout["role"] not in (2, 3) or layout["parts"][2] is None:
        raise ValueError("canopy support reference requires a native elevated roof")
    parent = 0 if layout["role"] == 2 else 1
    strip = layout["parts"][parent]
    across = 3 if parent == 0 else 13
    centres = [(u, across) if layout["axis"] == "X" else (across, u) for u in (2, 14)]
    x, y, _ = strip["origin"]
    width, depth, _ = strip["extent"]
    # Both real post feet must stay entirely inside their original paid strip;
    # the source fitting cannot widen a platform or narrow the railway corridor.
    if any(not (x <= px - 1 < px + 1 <= x + width and y <= py - 1 < py + 1 <= y + depth)
           for px, py in centres):
        raise ValueError("authored canopy support leaves its actual platform strip")
    return parent, centres, layout["parts"][2]["origin"][2]


def canopy_support_reference(layout):
    """Draw only two technical post volumes at their native platform/roof contacts.

    @param layout Actual canopy cell; X/Y are projected independently from world coordinates.
    @return Native64x64 geometry diagram; deck, roof, rail and artwork are absent.
    """
    _, centres, roof_height = canopy_support_spec(layout)
    image = Image.new("RGBA", (64, 64), (242, 245, 247, 255))
    draw = ImageDraw.Draw(image)
    # Back-to-front diagram order makes the two separate support volumes clear.
    # These coordinates describe an authoring reference, never new native boxes.
    for x, y in centres:
        corners = [(x - 0.5, y - 0.5), (x + 0.5, y - 0.5),
                   (x + 0.5, y + 0.5), (x - 0.5, y + 0.5)]
        bottom = [project(px, py, 2) for px, py in corners]
        top = [project(px, py, roof_height) for px, py in corners]
        draw.polygon([top[1], top[2], bottom[2], bottom[1]], fill=(65, 85, 100, 255))
        draw.polygon([top[2], top[3], bottom[3], bottom[2]], fill=(115, 135, 150, 255))
        draw.polygon(top, fill=(160, 175, 185, 255))
    return image


def canopy_roof_reference(layout):
    """Draw one sloping canopy half and four inset panes from native world geometry.

    @param layout Actual canopy cell, preserving its roof origin and maximum sorting height.
    @return Native64x64 roof-only technical diagram with an opaque four-pane guide.
    """
    canopy_support_spec(layout)
    roof = layout["parts"][2]
    base, rise = roof["origin"][2], roof["extent"][2]
    image = Image.new("RGBA", (64, 64), (242, 245, 247, 255))
    draw = ImageDraw.Draw(image)
    vertices = []
    # Complementary halves meet at the ridge of adjacent native platform rows.
    # Projection derives genuine X/Y views; it never rotates an authored raster.
    for u, v in ((0, 0), (16, 0), (16, 16), (0, 16)):
        height = base + rise * (v / 16 if layout["role"] == 2 else 1 - v / 16)
        x, y = (u, v) if layout["axis"] == "X" else (v, u)
        vertices.append(project(x, y, height))
    draw.polygon(vertices, fill=(75, 95, 110, 255), outline=(35, 50, 65, 255))
    # The four apertures are geometry-only guides. Final pane and steel pixels
    # must come from retained generated originals and stay separate roof/glass
    # owners, so the native transparent child can show trains through the glass.
    for u0, u1, v0, v1 in ((1.5, 7, 1.5, 7), (9, 14.5, 1.5, 7),
                          (1.5, 7, 9, 14.5), (9, 14.5, 9, 14.5)):
        pane = []
        for u, v in ((u0, v0), (u1, v0), (u1, v1), (u0, v1)):
            height = base + rise * (v / 16 if layout["role"] == 2 else 1 - v / 16)
            x, y = (u, v) if layout["axis"] == "X" else (v, u)
            pane.append(project(x, y, height))
        draw.polygon(pane, fill=(110, 195, 205, 255))
    return image


def canopy_sheet_reference(kind):
    """Write four safely framed native canopy references without creating game art.

    @param kind Either supports or roofs; the four frames always map to graphics4..7.
    @return New guide path with its owning plan recorded in PNG metadata.
    """
    if kind not in ("supports", "roofs"):
        raise ValueError("unknown native canopy reference kind")
    geometry = load_geometry()
    frames = [(24, 24, 744, 488), (792, 24, 1512, 488),
              (24, 536, 744, 1000), (792, 536, 1512, 1000)]
    image = Image.new("RGBA", (1536, 1024), (242, 245, 247, 255))
    draw = ImageDraw.Draw(image)
    # Fixed separated frames keep each genuine native role/axis independently
    # inspectable. The image model must preserve transparent source clearance.
    for graphics, frame in zip(range(4, 8), frames):
        left, top, _, _ = frame
        draw.rectangle(frame, outline=(255, 0, 255, 255), width=6)
        layout = geometry["layouts"][graphics]
        native = canopy_support_reference(layout) if kind == "supports" else canopy_roof_reference(layout)
        image.paste(native.resize((320, 320), Image.Resampling.NEAREST), (left + 200, top + 72))
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", f"r2mlvs14-remaining-assets-and-runtime-integration.md task24.2; archi-map-layers.md; {kind} geometry only; frame order gfx4,5,6,7")
    target = OUTPUT / f"conventional-station-canopy-{kind}-sheet-v1.png"
    image.save(target, pnginfo=metadata)
    return target


def independent_parent_sheet_reference():
    """Lay out four missing genuine parent originals with fixed frames and generous gutters.

    @return New geometry-only source sheet path: Y loader, Y front, X bare rear, Y bare rear.
    """
    geometry = load_geometry()
    cases = [(3, 0), (3, 1), (0, 0), (1, 0)]
    frames = [(24, 24, 744, 488), (792, 24, 1512, 488), (24, 536, 744, 1000), (792, 536, 1512, 1000)]
    image = Image.new("RGBA", (1536, 1024), (242, 245, 247, 255))
    draw = ImageDraw.Draw(image)
    # Independent native parents allow exact contact/height registration even
    # when a whole-scene generator changes the gap between platform strips.
    # Each four-cell view remains a genuine original; the game, not this guide,
    # owns final parent placement and the open through-station railway corridor.
    for (graphics, parent), frame in zip(cases, frames):
        left, top, right, bottom = frame
        draw.rectangle(frame, outline=(255, 0, 255, 255), width=6)
        diagram = native_parent_reference(geometry["layouts"][graphics], parent).resize((320, 320), Image.Resampling.NEAREST)
        image.paste(diagram, (left + 200, top + 72))
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task24.2; archi-map-layers.md; four independent native parent references only")
    target = OUTPUT / "conventional-station-independent-parent-sheet-v1.png"
    image.save(target, pnginfo=metadata)
    return target


if __name__ == "__main__":
    # Both axes require independently authored model views; the reference
    # changes native coordinates and never rotates an approved raster sprite.
    for target in build() + [inset_loader_reference(axis) for axis in ("X", "Y")] + [independent_parent_sheet_reference()] + [canopy_sheet_reference(kind) for kind in ("supports", "roofs")]:
        print(target.relative_to(ROOT).as_posix())
