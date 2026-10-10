"""Render the approved engine-view vectors as raster authoring references.

@file create_vehicle_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups49-53.
@see media/multilayer/contracts/multilayer_geometry_templates.svg
These labeled diagrams convey geometry only; they are never source game art.
"""

from pathlib import Path
import io
import math
import xml.etree.ElementTree as ET

from PIL import Image, ImageDraw, PngImagePlugin
from convert_tube_capsules import VECTORS


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "media/multilayer"
DIRECTIONS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]


def create_guide():
    """Render exact approved front vectors, fixed canvases and world anchors.

    @return Repository guide PNG path, carrying owner/template provenance.
    @throws ValueError If a named approved geometry view is missing.
    """
    template = ASSETS / "contracts/multilayer_geometry_templates.svg"
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
    path = ROOT / "media/multilayer/guides/m1/vehicle-direction-guide.png"
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
        source = ROOT / f"logs/proofs/mlm/native-art/derived/purchase-model-{index:02}.png"
        original = Image.open(source).convert("RGBA")
        enlarged = original.resize((160, 160), Image.Resampling.NEAREST)
        image.paste(enlarged, (index * 220 + 30, 30), enlarged)
        draw.text((index * 220 + 15, 210), label, fill="white")
    path = ROOT / "media/multilayer/guides/m1/purchase-model-reference-guide.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18; retained available purchase pose reference")
    image.save(path, pnginfo=metadata)
    return path


def create_capsule_guides():
    """Expose four exact engine axes/front-versus-rear faces per source sheet.

    @return Two immutable versioned geometry guide paths; no game art is drawn.
    """
    paths = []
    # Small sheets keep true front/rear and shallow diagonal axes unambiguous.
    # The source generator receives these diagrams alongside its own model,
    # never as a substitute for an original missing physical view.
    for directions in (("N", "NE", "S", "SW"), ("SE", "E", "NW", "W")):
        board = Image.new("RGB", (1536, 1024), (235, 239, 243))
        draw = ImageDraw.Draw(board)
        for index, direction in enumerate(directions):
            left, top = (index % 2) * 768, (index // 2) * 512
            cx, cy = left + 384, top + 265
            dx, dy = VECTORS[direction]
            scale = 9
            fx, fy = cx + dx * scale / 2, cy + dy * scale / 2
            rx, ry = cx - dx * scale / 2, cy - dy * scale / 2
            length = math.hypot(dx, dy)
            px, py = -dy / length, dx / length
            draw.rectangle((left + 8, top + 8, left + 759, top + 503), outline=(80, 90, 100), width=2)
            draw.text((left + 24, top + 25), f"{direction}: GEOMETRY ONLY; WHITE LAMPS = FRONT, SQUARE COUPLING = REAR", fill="black")
            draw.line((rx, ry, fx, fy), fill=(110, 116, 122), width=72)
            draw.rectangle((rx - 13, ry - 13, rx + 13, ry + 13), fill=(20, 24, 28))
            draw.ellipse((fx - 23, fy - 23, fx + 23, fy + 23), fill=(35, 40, 45))
            for sign in (-1, 1):
                x, y = fx + sign * px * 10, fy + sign * py * 10
                draw.rectangle((x - 5, y - 5, x + 5, y + 5), fill="white")
            draw.text((fx + 30, fy - 12), "FRONT", fill="black")
            draw.text((rx + 22, ry + 12), "REAR", fill="black")
            draw.text((left + 24, top + 455), "Fixed engine projection; keep selected model and upper-left image lighting.", fill="black")
        path = ROOT / "media/multilayer/guides/ground" / ("capsule-" + "-".join(d.lower() for d in directions) + "-v1.png")
        metadata = PngImagePlugin.PngInfo()
        metadata.add_text("Owner", "Plan13 Groups49-53; Plan16 staged10B; geometry-only source guide")
        stream = io.BytesIO()
        board.save(stream, format="PNG", pnginfo=metadata)
        encoded = stream.getvalue()
        if path.exists():
            if path.read_bytes() != encoded:
                raise ValueError("immutable capsule guide needs a sibling version")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(encoded)
        paths.append(path)
    return paths


if __name__ == "__main__":
    print(create_guide().relative_to(ROOT).as_posix())
    print(create_purchase_guide().relative_to(ROOT).as_posix())
