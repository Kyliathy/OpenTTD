"""Build small code-authored native preview sprites, independently of final Plan13 art.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
No existing source sheet or selected M1 sprite is edited. This deliberately small
pixel-art preview occupies its own appended native block and grants no gameplay.
"""
from pathlib import Path
import random
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / "media/baseset/openttd"


def main():
    palette = Image.open(DEST / "map_layers.png").getpalette()
    sheet = Image.new("P", (80, 64 * 11), 0)
    sheet.putpalette(palette)

    def colour(rgb):
        return min(range(1, 215), key=lambda n: sum((palette[n * 3 + c] - rgb[c]) ** 2 for c in range(3)))

    sprites = []
    tones = [(142, 133, 118), (125, 173, 198), (147, 126, 170)]
    for family, tone in enumerate(tones):
        for size, (width, height) in enumerate([(20, 16), (36, 26), (64, 44)]):
            top = len(sprites) * 64
            draw = ImageDraw.Draw(sheet)
            cx, cy = width // 2 + 2, top + height // 2 + 2
            rx, ry = width // 2 - 1, height // 2 - 1
            points = [(cx-rx, cy-ry//4), (cx-rx*3//4, cy-ry*3//4), (cx-rx//4, cy-ry),
                      (cx+rx//3, cy-ry*4//5), (cx+rx*4//5, cy-ry//3), (cx+rx, cy+ry//3),
                      (cx+rx//2, cy+ry*4//5), (cx-rx//3, cy+ry), (cx-rx*4//5, cy+ry//2)]
            dark = colour(tuple(int(c * .38) for c in tone))
            base = colour(tone)
            draw.polygon(points, fill=base, outline=dark)
            mask = Image.new("1", sheet.size)
            ImageDraw.Draw(mask).polygon(points, fill=1)
            noise = random.Random(731 + family * 101 + size)
            for _ in range(width * height // 3):
                x, y = noise.randrange(2, width + 2), noise.randrange(top + 2, top + height + 2)
                if mask.getpixel((x, y)):
                    light = 1.30 - (y - top) / height * .65 + noise.uniform(-.2, .2)
                    draw.point((x, y), fill=colour(tuple(min(255, int(c * light)) for c in tone)))
            for px, py, radius in [(cx-rx//3, cy-ry//4, max(1, width//10)), (cx+rx//3, cy+ry//4, max(1, width//14))]:
                draw.ellipse((px-radius, py-radius//2, px+radius, py+radius//2+1), fill=dark)
                draw.line((px-radius, py+radius//2+1, px+radius, py+radius//2+1), fill=colour(tuple(min(255, int(c*1.25)) for c in tone)))
            sprites.append((2, top + 2, width, height, -width//2, -height//2))

    for attached in (False, True):
        top = len(sprites) * 64
        draw = ImageDraw.Draw(sheet)
        blue = colour((58, 91, 150)); white = colour((216, 220, 215)); gold = colour((206, 152, 55))
        edge = colour((35, 45, 60)); glow = colour((98, 207, 228))
        draw.polygon([(2, top+11), (12, top+6), (19, top+10), (9, top+15)], fill=blue, outline=edge)
        draw.polygon([(21, top+12), (31, top+7), (38, top+11), (28, top+16)], fill=blue, outline=edge)
        for x in range(6, 34, 5):
            draw.line((x, top+10, x+3, top+12), fill=glow)
        draw.polygon([(15, top+9), (21, top+6), (26, top+10), (21, top+15), (15, top+12)], fill=white, outline=edge)
        draw.polygon([(16, top+8), (21, top+6), (25, top+9), (20, top+12)], fill=gold, outline=edge)
        draw.line((20, top+6, 20, top+2), fill=white)
        draw.line((18, top+2, 22, top+2), fill=white)
        draw.line((22, top+14, 26, top+20), fill=gold)
        if attached:
            draw.line((26, top+20, 30, top+21), fill=white)
        else:
            draw.polygon([(18, top+15), (22, top+15), (20, top+20)], fill=glow)
        sprites.append((1, top+1, 39, 23, -20, -12))

    sheet.save(DEST / "space_preview.png", transparency=0)
    lines = ["// Code-authored initial Space preview. Final Plan13 families remain pending.",
             "// @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1",
             "//@@WARNING DISABLE 44", "   -1 * 0 05 9C 0B FF 04 0E // 11 sprites at appended offset3588", "//@@WARNING ENABLE 44"]
    lines += [f"   -1 sprites/space_preview.png 8bpp {x} {y} {w} {h} {ox} {oy} normal" for x,y,w,h,ox,oy in sprites]
    (DEST / "space_preview.nfo").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("Space preview: 9 asteroid class/size sprites + 2 probe poses; prior source sheets preserved")


if __name__ == "__main__":
    main()
