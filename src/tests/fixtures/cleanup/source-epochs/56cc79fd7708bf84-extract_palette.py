"""Write the DOS palette and the indices the quantiser must not use.

Reads the PLTE chunk of the tunnel-portal sheet, which is a 256-colour
8bpp PNG in the extra GRF.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png_indexed


SOURCE = "media/baseset/openttd/tunnel_portals.png"
OUTPUT = "docs/zz-reach2/upgrades/2026-10/assets/dos_palette.txt"

#Index 0 is transparent in the game. Company colours are the remap hole.
#Animated water and lights are filled at runtime. The unused pink sits between
#the defined palette and the animation. White at 255 stays usable.
#Sources: src/palette.cpp PALETTE_INDEX_CC_START, src/gfx_type.h PALETTE_ANIM_*,
#src/table/palettes.h "unused pink" then "Palette animated colours" then white.
FORBIDDEN = [
    (0, 0, "transparent; src/table/sprites.h and the sprite contract, index 0"),
    (198, 205, "company colour remap; src/palette.cpp PALETTE_INDEX_CC_START for 8 shades"),
    (216, 226, "unused pink hole; src/table/palettes.h between PALETTE_INDEX_END 215 and the animation"),
    (227, 254, "animated water and lights; src/gfx_type.h PALETTE_ANIM_START 227, PALETTE_ANIM_SIZE 28"),
]


def main():
    """Extract the palette and write dos_palette.txt.

    @return None.
    """
    _width, _height, colour_type, palette, _pixels = png_indexed.read_png(SOURCE)
    if colour_type != 3 or palette is None or len(palette) != 256:
        raise SystemExit("tunnel_portals.png did not yield a 256-colour palette")
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    lines = [
        "# DOS palette taken from media/baseset/openttd/tunnel_portals.png PLTE.",
        "# index red green blue",
    ]
    for index, (red, green, blue) in enumerate(palette):
        lines.append(f"{index} {red} {green} {blue}")
    lines.append("")
    lines.append("# Indices the quantiser must not pick.")
    for start, end, reason in FORBIDDEN:
        lines.append(f"forbid {start}-{end}  {reason}")
    open(OUTPUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    print(f"wrote {OUTPUT}")


if __name__ == "__main__":
    main()
