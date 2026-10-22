"""Render the approved engine-view vectors as raster authoring references.

@file create_vehicle_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18
@see docs/zz-reach2/upgrades/2026-10/assets/multilayer_geometry_templates.svg
These labeled diagrams convey geometry only; they are never source game art.
"""

from pathlib import Path
import xml.etree.ElementTree as ET

from PIL import Image, ImageDraw, PngImagePlugin


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "docs/zz-reach2/upgrades/2026-10/assets"
DIRECTIONS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]


def create_guide():
    """Render exact approved front vectors, fixed canvases and world anchors.

    @return Repository guide PNG path, carrying owner/template provenance.
    @throws ValueError If a named approved geometry view is missing.
    """
    template = ASSETS / "multilayer_geometry_templates.svg"
    tree = ET.parse(template)
    views = {node.attrib.get("id"): node for node in tree.iter()}
    image = Image.new("RGB", (8 * 208, 230), (238, 241, 244))
    draw = ImageDraw.Draw(image)
    draw.text((12, 12), "GEOMETRY ONLY: eight fixed engine views; arrow/circle identifies cutter FRONT", fill="black")
    # Every heading reads its own approved vector. Rotating a raster or mirroring
    # illumination would conceal an absent rear view in the eventual game source.
    for index, direction in enumerate(DIRECTIONS):
        node = views.get("tbm-" + direction)
        if node is None:
            raise ValueError("missing approved engine geometry: " + direction)
        head = next(child for child in node if child.tag.endswith("circle"))
        dx, dy = int(head.attrib["cx"]), int(head.attrib["cy"])
        origin_x, origin_y = index * 208 + 104, 125
        draw.rectangle((origin_x - 96, origin_y - 96, origin_x + 95, origin_y + 47), outline=(90, 105, 120))
        draw.line((origin_x - dx * 3, origin_y - dy * 3, origin_x + dx * 3, origin_y + dy * 3), fill=(36, 107, 131), width=25)
        x, y = origin_x + dx * 3, origin_y + dy * 3
        draw.ellipse((x - 17, y - 17, x + 17, y + 17), fill=(223, 138, 42))
        draw.line((origin_x - 6, origin_y, origin_x + 6, origin_y), fill="black", width=2)
        draw.line((origin_x, origin_y - 6, origin_x, origin_y + 6), fill="black", width=2)
        draw.text((origin_x - 12, 184), direction, fill="black")
    draw.text((12, 210), "Native canvas 64x48, anchor (-32,-32); fixed top-left lighting. Support follows the same axis; it has no cutter head.", fill="black")
    path = ASSETS / "mlvs/vehicle-direction-guide.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18; geometry-only authoring reference")
    metadata.add_text("Template", template.relative_to(ROOT).as_posix())
    image.save(path, pnginfo=metadata)
    return path


def create_purchase_guide():
    """Expose retained purchase silhouettes as a labeled raster edit reference.

    @return Repository guide path; composed original pixels are geometry inputs.
    """
    image = Image.new("RGBA", (1100, 250), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    labels = ["Standard cutter", "Deep cutter", "Earth hopper", "Rock hopper", "Spoil hauler"]
    # Reuse every existing authored available preview without repainting its
    # shape. Nearest enlargement exposes the exact tiny model identity for the
    # new locked presentation; labels remain outside selected sprite pixels.
    for index, label in enumerate(labels):
        source = ROOT / f"media/multilayer/sprites/m1/purchase-model-{index:02}.png"
        original = Image.open(source).convert("RGBA")
        enlarged = original.resize((160, 160), Image.Resampling.NEAREST)
        image.paste(enlarged, (index * 220 + 30, 30), enlarged)
        draw.text((index * 220 + 15, 210), label, fill="white")
    path = ASSETS / "mlvs/purchase-model-reference-guide.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18; retained available purchase pose reference")
    image.save(path, pnginfo=metadata)
    return path


if __name__ == "__main__":
    print(create_guide().relative_to(ROOT).as_posix())
    print(create_purchase_guide().relative_to(ROOT).as_posix())
