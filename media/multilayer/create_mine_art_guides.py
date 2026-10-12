"""Draw geometry-only placement guides for the actual fixed native mine layout.

@file create_mine_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups27B/27/27C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
These technical references never supply game pixels. Native machinery/loading
coordinates and the retained M1 control hut remain their existing owners.
"""

from PIL import Image, ImageDraw, PngImagePlugin

from tools.ground_mine_contract import ROOT, mine_contract


OUTPUT = ROOT / "media/multilayer/guides/ground"
ROLES = ("coal-feed-hopper", "feed-conveyor", "roll-crusher", "vibrating-screen",
         "cross-transfer-conveyor", "discharge-chute", "coal-output-bunker", "control-hut", "dust-filter")
VOLUMES = (((3, 3, 4), (10, 10, 9)), ((1, 5, 2), (14, 6, 4)), ((3, 2, 0), (10, 12, 13)),
           ((2, 3, 2), (12, 10, 8)), ((1, 5, 3), (14, 6, 5)), ((4, 2, 0), (8, 12, 13)),
           ((2, 2, 0), (12, 12, 7)), ((3, 3, 0), (10, 10, 14)), ((3, 3, 0), (10, 10, 14)))
FRAME_BOXES = ((24, 24, 744, 488), (792, 24, 1512, 488), (24, 536, 744, 1000), (792, 536, 1512, 1000))
SOURCE_WINDOWS = ((48, 48, 720, 472), (816, 48, 1488, 472), (48, 560, 720, 976), (816, 560, 1488, 976))


def project(x, y, z=0):
    """Project actual local world coordinates on the common native building canvas.

    @param x Tile-local native world X coordinate.
    @param y Tile-local native world Y coordinate.
    @param z Physical source height above the paid floor.
    @return Reference coordinate with native source anchor(-31,-33).
    """
    return 31 + 2 * (y - x), 33 + x + y - z


def volume(draw, origin, extent, colors):
    """Draw a technical machinery envelope without producing authored bitmap material.

    @param draw Pillow drawing target for the geometry-only reference.
    @param origin Actual tile-local model envelope origin.
    @param extent Width/depth/height within the unchanged native16x16x20 sorting box.
    @param colors Technical +X/+Y/top face colors, unrelated to original material pixels.
    @return None.
    """
    x, y, z = origin
    width, depth, height = extent
    corners = ((x, y), (x + width, y), (x + width, y + depth), (x, y + depth))
    bottom = [project(cx, cy, z) for cx, cy in corners]
    top = [project(cx, cy, z + height) for cx, cy in corners]
    draw.polygon([top[1], top[2], bottom[2], bottom[1]], fill=colors[0], outline=(35, 50, 65, 255))
    draw.polygon([top[2], top[3], bottom[3], bottom[2]], fill=colors[1], outline=(35, 50, 65, 255))
    draw.polygon(top, fill=colors[2], outline=(35, 50, 65, 255))


def cell_reference(variant):
    """Draw one approved physical envelope on exactly one native machinery tile.

    @param variant Actual row-major cell0..8 from the compiled fixed mine layout.
    @return Geometry-only64x64 RGBA reference, never selected source artwork.
    """
    contract = mine_contract("COAL")
    if variant not in range(len(contract["native_cells"])):
        raise ValueError("mine source guide requires an actual native machinery cell")
    image = Image.new("RGBA", (64, 64))
    draw = ImageDraw.Draw(image)
    floor = [project(x, y) for x, y in ((0, 0), (15, 0), (15, 15), (0, 15))]
    draw.polygon(floor, fill=(205, 211, 215, 255), outline=(100, 110, 120, 255))
    origin, extent = VOLUMES[variant]
    volume(draw, origin, extent, ((65, 85, 100, 255), (110, 130, 145, 255), (150, 165, 175, 255)))
    # Equipment-specific primitives constrain identity and physical axis.
    # They are technical annotations only; ImageGen supplies actual original
    # hoppers, rollers, conveyor covers, screen and filter material/details.
    if variant in (0, 6):
        x, y, z = origin
        width, depth, height = extent
        draw.polygon([project(x + 2, y + 2, z + height), project(x + width - 2, y + 2, z + height),
                      project(x + width - 2, y + depth - 2, z + height), project(x + 2, y + depth - 2, z + height)],
                     fill=(45, 45, 50, 255))
    if variant in (1, 4):
        x, y, z = origin
        width, depth, height = extent
        draw.line([project(x, y + depth / 2, z + height), project(x + width, y + depth / 2, z + height)], fill=(225, 174, 55, 255), width=1)
    return image


def sheet_reference(variants):
    """Place four independent actual machinery envelopes inside canonical magenta frames.

    @param variants Four distinct row-major native cell identities in requested panel order.
    @return Framed1536x1024 geometry reference with fixed transparent margins.
    """
    # Each panel must describe a different required native cell. Repeated
    # envelopes could hide a missing machinery role before original authoring.
    if len(variants) != 4 or len(set(variants)) != 4:
        raise ValueError("mine source guide needs four independent actual cells")
    sheet = Image.new("RGBA", (1536, 1024))
    draw = ImageDraw.Draw(sheet)
    # Independent panels prevent a generated campus gap from being mistaken
    # for a missing tile or the separate native loading floor. Role labels are
    # guide-only and explicitly forbidden in the resulting source original.
    for variant, frame, window in zip(variants, FRAME_BOXES, SOURCE_WINDOWS):
        draw.rectangle(frame, outline=(255, 0, 255, 255), width=6)
        draw.text((frame[0] + 14, frame[1] + 10), f"GEOMETRY ONLY cell{variant} {ROLES[variant]}", fill=(35, 50, 65, 255))
        sprite = cell_reference(variant).resize((320, 320), Image.Resampling.NEAREST)
        left, top, right, bottom = window
        sheet.alpha_composite(sprite, ((left + right - 320) // 2, (top + bottom - 320) // 2))
    return sheet


def isolated_reference(variant):
    """Put one missing genuine cell envelope inside a generously padded separate source reference.

    @param variant Actual native machinery cell whose framed source needs an isolated fallback.
    @return Geometry-only1254x1254 transparent reference; never source-art pixels.
    """
    image = Image.new("RGBA", (1254, 1254))
    sprite = cell_reference(variant).resize((512, 512), Image.Resampling.NEAREST)
    image.alpha_composite(sprite, (371, 371))
    return image


def construction_cell_reference(variant, stage):
    """Draw a technical unfinished/finishing envelope for one actual native machinery role.

    @param variant Real machinery cell0..8 from the compiled fixed layout.
    @param stage Actual native unfinished1 or finishing2 source phase.
    @return Geometry-only64x64 reference, never a construction sprite substitute.
    """
    # Construction follows native stage progression in the same paid cell.
    # A different layout or unsupported phase must not move machinery onto
    # the loading approach or introduce an art-local work/state owner.
    if variant not in range(9) or stage not in (1, 2):
        raise ValueError("mine construction guide requires an actual cell/stage")
    image = Image.new("RGBA", (64, 64))
    draw = ImageDraw.Draw(image)
    draw.polygon([project(x, y) for x, y in ((0, 0), (15, 0), (15, 15), (0, 15))],
                 fill=(205, 211, 215, 255), outline=(100, 110, 120, 255))
    origin, extent = VOLUMES[variant]
    x, y, z = origin
    width, depth, height = extent
    colors = ((65, 85, 100, 255), (110, 130, 145, 255), (150, 165, 175, 255))
    if stage == 1:
        volume(draw, (x, y, 0), (width, depth, 2), colors)
        # A short foundation/frame identifies the machinery's actual base
        # envelope without pretending the complete housing already operates.
        # ImageGen authors role-specific unfinished feet/joists and hardware.
        for px, py in ((x, y), (x + width - 1, y), (x + width - 1, y + depth - 1), (x, y + depth - 1)):
            volume(draw, (px, py, 2), (1, 1, max(2, min(height - 2, 5))), colors)
        if variant == 8:
            # Exactly two real canister bases preserve the selected filter's
            # inventory while the canisters themselves are not installed.
            # Extra circular mounts would imply a different completed model.
            for cy in (6, 10):
                px, py = project(9, cy, 2)
                draw.ellipse((px - 3, py - 1.5, px + 3, py + 1.5), fill=(65, 85, 100, 255))
                draw.ellipse((px - 1.5, py - .5, px + 1.5, py + .5), fill=(150, 165, 175, 255))
    else:
        volume(draw, origin, extent, colors)
        # The open top marks unfinished installation rather than a stock
        # window or missing source role. Completed stage3 keeps its selected
        # genuine apparatus; this diagram supplies no authored material.
        draw.polygon([project(x + 2, y + 2, z + height), project(x + width - 2, y + 2, z + height),
                      project(x + width - 2, y + depth - 2, z + height), project(x + 2, y + depth - 2, z + height)],
                     fill=(45, 55, 65, 255))
    return image


def construction_sheet_reference(stage, variants, center_subject=False):
    """Place one native construction phase's four real machinery roles in safe source frames.

    @param stage Actual per-tile construction1 or2 selected by the native stage owner.
    @param variants Four distinct existing machinery identities in explicit panel order.
    @param center_subject Center the complete visible technical model; False preserves existingv1 reference bytes.
    @return Framed1536x1024 geometry-only source layout with transparent margins.
    """
    # One original panel per required role prevents generic repeated frames
    # from hiding a missing unfinished or finishing model in the source matrix.
    if len(variants) != 4 or len(set(variants)) != 4:
        raise ValueError("mine construction source guide needs four distinct cells")
    sheet = Image.new("RGBA", (1536, 1024))
    draw = ImageDraw.Draw(sheet)
    for variant, frame, window in zip(variants, FRAME_BOXES, SOURCE_WINDOWS):
        draw.rectangle(frame, outline=(255, 0, 255, 255), width=6)
        draw.text((frame[0] + 14, frame[1] + 10), f"GEOMETRY ONLY construction{stage} cell{variant} {ROLES[variant]}", fill=(35, 50, 65, 255))
        sprite = construction_cell_reference(variant, stage)
        if center_subject:
            # The native canvas has transparent sky above its floor. Center
            # the actual technical subject within the source panel so a model
            # reference cannot push generated floor corners toward a frame.
            sprite = sprite.crop(sprite.getbbox())
            scale = min(320 / sprite.width, 250 / sprite.height)
            sprite = sprite.resize((round(sprite.width * scale), round(sprite.height * scale)), Image.Resampling.NEAREST)
        else:
            sprite = sprite.resize((320, 320), Image.Resampling.NEAREST)
        left, top, right, bottom = window
        sheet.alpha_composite(sprite, ((left + right - sprite.width) // 2, (top + bottom - sprite.height) // 2))
    return sheet


def build():
    """Publish immutable versioned references for the eight new genuine mine components.

    @return Written reference paths; approved existing guides are never overwritten.
    """
    paths = []
    OUTPUT.mkdir(parents=True, exist_ok=True)
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task27.1; archi-map-layers.md; geometry only, never game artwork")
    # The control hut atcell7 is an existing compatible M1 original. No new
    # generated duplicate is requested merely to fill a convenient four-panel
    # sheet; the second panel set supplies actual missing cell8 instead.
    for name, variants in (("coal-machinery-a-v1", (0, 1, 2, 3)), ("coal-machinery-b-v1", (4, 5, 6, 8))):
        path = OUTPUT / (name + ".png")
        if path.exists():
            raise ValueError("mine source reference is immutable; use a new version")
        sheet_reference(variants).save(path, format="PNG", pnginfo=metadata)
        paths.append(path)
    return paths


if __name__ == "__main__":
    for output in build():
        print(output.relative_to(ROOT).as_posix())
