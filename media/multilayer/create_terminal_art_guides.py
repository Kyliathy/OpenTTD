"""Draw geometry-only truck-stop source references from the compiled native producer.

@file create_terminal_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group25.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
These technical diagrams are never game art. Real native parents and road ground
retain their existing owners; source guides grant no construction or cargo rights.
"""

from PIL import Image, ImageDraw, PngImagePlugin

from tools.ground_terminal_contract import ROOT, load_geometry


OUTPUT = ROOT / "media/multilayer/guides/ground"


def project(x, y, z):
    """Project actual local world coordinates on the common native source canvas.

    @param x Native world X coordinate within one road-stop cell.
    @param y Native world Y coordinate within one road-stop cell.
    @param z World height above the unchanged native road ground.
    @return Source-space reference coordinate with anchor(-31,-33).
    """
    return 31 + 2 * (y - x), 33 + x + y - z


def parent_reference(layout, index, physical_height=None):
    """Draw one source-envelope guide while keeping the exact native sorting box separate.

    @param layout Actual compiled bay/through native layout.
    @param index Native parent sequence index; absent third through parents refuse.
    @param physical_height Optional selected model height within the unchanged native sorting envelope.
    @return Padded96x80 geometry diagram, native canvas origin at(16,0).
    """
    part = layout["parents"][index]
    if part is None:
        raise ValueError("absent native terminal parent cannot supply an original guide")
    x, y, z = part["origin"]
    width, depth, maximum_height = part["extent"]
    # Bay safety rails are physically low while the actual native sorting box
    # remains10 high. The far loading housing alone fills its native height;
    # this reference never rewrites sorting to conceal a rail/turning conflict.
    height = 4 if not layout["drive_through"] and index != 1 else maximum_height
    if physical_height is not None:
        # Selected physical fittings may be lower than the native sorting
        # envelope; authoring must never enlarge that envelope to fit artwork.
        if not 0 < physical_height <= maximum_height:
            raise ValueError("terminal guide height leaves the native sorting envelope")
        height = physical_height
    image = Image.new("RGBA", (96, 80), (242, 245, 247, 255))
    draw = ImageDraw.Draw(image)
    corners = [(x, y), (x + width, y), (x + width, y + depth), (x, y + depth)]
    bottom = [(16 + px, py) for px, py in (project(cx, cy, z) for cx, cy in corners)]
    top = [(16 + px, py) for px, py in (project(cx, cy, z + height) for cx, cy in corners)]
    draw.polygon([top[1], top[2], bottom[2], bottom[1]], fill=(65, 85, 100, 255))
    draw.polygon([top[2], top[3], bottom[3], bottom[2]], fill=(110, 130, 145, 255))
    draw.polygon(top, fill=(150, 165, 175, 255), outline=(35, 50, 65, 255))
    return image


def remaining_component_reference(graphics, parent, kind):
    """Project one missing genuine source component with its real facade visibility.

    @param graphics Actual native bay/through graphics0..5.
    @param parent Actual sorting parent index.
    @param kind Housing or rail physical model; the native sorting box remains unchanged.
    @return Padded geometry-only reference with source origin(16,0).
    """
    if kind not in ("housing", "rail"):
        raise ValueError("unknown terminal original component kind")
    layout = load_geometry()["layouts"][graphics]
    # Through stops preserve their3-unit sorting strips. A physical open rail
    # occupies only the outer1-unit edge, leaving both actual truck lanes and
    # their loading aisle clear; no new box or world footprint is installed.
    if kind == "rail" and layout["drive_through"]:
        import copy
        layout = copy.deepcopy(layout)
        bounds = layout["parents"][parent]
        across = 1 if bounds["extent"][0] == 16 else 0
        if bounds["origin"][across] != 0:
            bounds["origin"][across] += bounds["extent"][across] - 1
        bounds["extent"][across] = 1
    diagram = parent_reference(layout, parent, 10 if kind == "housing" else 7)
    # The fixed camera sees +X/+Y faces. The genuine SW front has its three
    # louver apertures on +X; NE/NW show the outside rear, so their inside
    # louvers/glass/nozzle must remain hidden instead of moving to that rear.
    if kind == "housing" and graphics in (1, 2, 4):
        draw = ImageDraw.Draw(diagram)
        for begin, end in ((4, 6), (7, 9), (10, 12)):
            if graphics == 2:
                points = [project(3, u, z) for u, z in ((begin, 2), (end, 2), (end, 7), (begin, 7))]
            else:
                points = [project(u, 3, z) for u, z in ((begin, 2), (end, 2), (end, 7), (begin, 7))]
            draw.polygon([(16 + x, y) for x, y in points], fill=(185, 195, 205, 255), outline=(25, 40, 55, 255))
    return diagram


def remaining_sheet_reference(kind, version=1):
    """Write the seven missing component guides with separated canonical source frames.

    @param kind Equipment sheet or rails sheet; the latter includes one unused assembled reference.
    @param version Reference revision; rails2 supplies all four missing genuine rail components.
    @return New versioned guide path, preserving every approved base/reference byte.
    """
    if kind not in ("equipment", "rails"):
        raise ValueError("unknown terminal remaining component sheet")
    # Preserve the first recorded reference bytes. The rejected equipment-sheet
    # rail had the wrong world axis, so revision2 gives all four real rail
    # components their own geometry instead of repeating an assembled model.
    if version not in (1, 2) or (kind == "equipment" and version != 1):
        raise ValueError("unsupported terminal guide revision")
    cases = [(0, 1, "housing"), (2, 1, "housing"), (3, 1, "housing"), (0, 0, "rail")] if kind == "equipment" else [(0, 2, "rail"), (4, 1, "rail"), (5, 1, "rail")]
    if kind == "rails" and version == 2:
        cases = [(0, 0, "rail"), (0, 2, "rail"), (4, 1, "rail"), (5, 1, "rail")]
    diagrams = [remaining_component_reference(*case) for case in cases]
    if kind == "rails" and version == 1:
        # The fourth panel is an assembled through-X geometry comparison only;
        # its pixels never supply another source slot or fake a missing view.
        assembled = remaining_component_reference(4, 0, "housing")
        rail = remaining_component_reference(4, 1, "rail")
        for y in range(80):
            for x in range(96):
                value = rail.getpixel((x, y))
                if value != (242, 245, 247, 255):
                    assembled.putpixel((x, y), value)
        diagrams.append(assembled)
    image = Image.new("RGBA", (1536, 1024), (242, 245, 247, 255))
    draw = ImageDraw.Draw(image)
    frames = [(24, 24, 744, 488), (792, 24, 1512, 488), (24, 536, 744, 1000), (792, 536, 1512, 1000)]
    # Each actual source component remains inside its own native-coordinate
    # guide and safe source frame; whole-scene gaps cannot relocate a parent.
    for diagram, frame in zip(diagrams, frames):
        left, top, _, _ = frame
        draw.rectangle(frame, outline=(255, 0, 255, 255), width=6)
        image.paste(diagram.resize((384, 320), Image.Resampling.NEAREST), (left + 168, top + 64))
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", f"r2mlvs14-remaining-assets-and-runtime-integration.md task25.2; archi-map-layers.md; geometry-only remaining {kind} components")
    target = OUTPUT / f"conventional-terminal-{kind}-parents-v{version}.png"
    image.save(target, pnginfo=metadata)
    return target


def isolated_loader_reference():
    """Specify three real louver apertures on the isolated native SE loading housing.

    @return Generous1024x1024 geometry-only reference with native canvas origin(256,192), scale8.
    """
    layout = load_geometry()["layouts"][1]
    diagram = parent_reference(layout, 1)
    draw = ImageDraw.Draw(diagram)
    # The first sheet repeated two broad louvers. Explicit three-panel facade
    # geometry makes the required model inventory independent of that rejected
    # reference, while retaining the actual native housing strip and camera.
    for begin, end in ((4, 6), (7, 9), (10, 12)):
        polygon = [(16 + px, py) for px, py in (project(u, 3, z) for u, z in ((begin, 2), (end, 2), (end, 7), (begin, 7)))]
        draw.polygon(polygon, fill=(185, 195, 205, 255), outline=(25, 40, 55, 255))
    image = Image.new("RGBA", (1024, 1024), (242, 245, 247, 255))
    image.paste(diagram.resize((768, 640), Image.Resampling.NEAREST), (128, 192))
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task25.1; archi-map-layers.md; native SE housing with three explicit technical louver apertures; not art")
    target = OUTPUT / "conventional-terminal-se-loader-inset-v1.png"
    image.save(target, pnginfo=metadata)
    return target


def base_sheet_reference():
    """Write independently framed SE bay parents plus their native assembled geometry.

    @return New four-cell geometry guide path; no art or existing source is changed.
    """
    layout = load_geometry()["layouts"][1]
    image = Image.new("RGBA", (1536, 1024), (242, 245, 247, 255))
    draw = ImageDraw.Draw(image)
    frames = [(24, 24, 744, 488), (792, 24, 1512, 488),
              (24, 536, 744, 1000), (792, 536, 1512, 1000)]
    diagrams = [parent_reference(layout, index) for index in range(3)]
    assembled = Image.new("RGBA", (96, 80), (242, 245, 247, 255))
    # Exact parent order remains visible in the assembled technical reference.
    # Copy diagram subject pixels only, so reference backgrounds do not conceal
    # earlier parents; this creates no authored or distributed source pixels.
    for diagram in diagrams:
        for y in range(80):
            for x in range(96):
                value = diagram.getpixel((x, y))
                if value != (242, 245, 247, 255):
                    assembled.putpixel((x, y), value)
    # Four separated source cells expose the exact independent parents and
    # their relationship without allowing coupled generator placement to move
    # a safety rail, loading housing or native open entrance.
    for diagram, frame in zip(diagrams + [assembled], frames):
        left, top, _, _ = frame
        draw.rectangle(frame, outline=(255, 0, 255, 255), width=6)
        image.paste(diagram.resize((384, 320), Image.Resampling.NEAREST), (left + 168, top + 64))
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task25.1; archi-map-layers.md; geometry only, SE parents0/1/2/assembled")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    target = OUTPUT / "conventional-terminal-se-parent-sheet-v1.png"
    image.save(target, pnginfo=metadata)
    return target


if __name__ == "__main__":
    print(base_sheet_reference().relative_to(ROOT).as_posix())
