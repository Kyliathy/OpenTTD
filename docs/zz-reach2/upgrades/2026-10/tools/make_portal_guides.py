"""Make aligned drawing guides, never final art, for generated portal pairs.

@see docs/zz-reach2/upgrades/2026-10/r2mlm6-layer-graphics-assets.md
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
"""

from pathlib import Path

from PIL import Image, ImageDraw

ASSETS = Path(__file__).resolve().parents[1] / "assets"
MAGENTA = (255, 0, 255)
DIRECTIONS = ("ne", "se", "sw", "nw")


def make_guide(direction, height):
    """Draw matching geometric placeholders for one back/front pair.

    @param direction Door edge in NE, SE, SW, NW order.
    @param height Contract canvas height, 64 for elevators or 48 for portals.
    @return RGB image enlarged by nearest neighbours for generation input.
    """
    image = Image.new("RGB", (128, height), MAGENTA)
    draw = ImageDraw.Draw(image)
    north_y = height - 31
    ground = ((31, north_y), (63, north_y + 15), (31, height - 1), (0, north_y + 15))
    rise = 22 if height == 64 else 14
    roof = tuple((x, y - rise) for x, y in ground)
    door_edge = DIRECTIONS.index(direction)
    draw.polygon(ground, fill=(130, 122, 106))
    #The two visible wall planes are retained unless that plane is the open door.
    for edge in (1, 2):
        if edge != door_edge:
            following = (edge + 1) % 4
            draw.polygon((ground[edge], ground[following], roof[following], roof[edge]),
                         fill=(112, 116, 118) if edge == 1 else (174, 176, 170))
    draw.polygon(roof, fill=(196, 194, 180))
    #The front frame uses the very same world positions in the second panel.
    following = (door_edge + 1) % 4
    for point in (door_edge, following):
        x, y = ground[point]
        top_x, top_y = roof[point]
        draw.line((x + 64, y, top_x + 64, top_y), fill=(226, 204, 112), width=2)
    draw.line((roof[door_edge][0] + 64, roof[door_edge][1],
               roof[following][0] + 64, roof[following][1]), fill=(226, 204, 112), width=3)
    return image.resize((128 * 8, height * 8), Image.Resampling.NEAREST)


def main():
    """Write eight directional guides beneath the existing assets folder.

    @return None.
    """
    for height, family in ((64, "elevator"), (48, "portal")):
        for direction in DIRECTIONS:
            path = ASSETS / f"guide_{family}_{direction}_pair.png"
            make_guide(direction, height).save(path)
            print(path.name)


if __name__ == "__main__":
    main()
