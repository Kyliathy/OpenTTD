"""Prepare offline review boards from the existing quantiser, without editing raw art.

@see docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md
"""

from pathlib import Path
import shutil
import sys

from PIL import Image, ImageChops, ImageDraw, ImageOps

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "docs/zz-reach2/upgrades/2026-10/tools"))
import contact_sheet
import sprite_quantize as sprites


def stack_board(title, entries, destination):
    """Stack labelled review images on a neutral board.

    @param title Board title, including the review's provisional status.
    @param entries Caption and PNG path pairs in display order.
    @param destination Review board output path.
    @return None.
    """
    pictures = [(caption, Image.open(path).convert("RGB")) for caption, path in entries]
    width = max(1000, max(picture.width for _, picture in pictures))
    height = 40 + sum(picture.height + 24 for _, picture in pictures)
    board = Image.new("RGB", (width, height), (64, 64, 64))
    draw = ImageDraw.Draw(board)
    draw.text((8, 8), title, fill="white")
    top = 40
    for caption, picture in pictures:
        draw.text((8, top), caption, fill="white")
        board.paste(picture, (0, top + 20))
        top += picture.height + 24
    board.save(destination)


def main():
    """Generate comparisons and boards for the integrated 66-row development set.

    @return None. Assertions fail if any sprite is blank or uses a forbidden index.
    """
    folder = ROOT / "build/art-tool-checks"
    plan = ROOT / "docs/zz-reach2/upgrades/2026-10/assets/reviews"
    palette, forbidden = sprites.load_palette(sprites.PALETTE_FILE)
    paths = []
    for offset, raw, box, *rest in sprites.MANIFEST:
        sprite = sprites.quantise_file(raw, box, *sprites.CONTRACT[offset], palette, forbidden,
                                      rest[0] if rest else None)
        assert any(sprite[0]), f"blank sprite {offset}"
        assert not (set(sprite[0]) - {0}) & forbidden, f"forbidden palette index in {offset}"
        path = folder / f"sprite-{offset:02}.png"
        sprites.sprite_image(sprite, palette).save(path)
        paths.append(path)
    contact_sheet.grid(paths[:44], 8, 4, folder / "retained44-grid.png")
    contact_sheet.grid(paths[44:], 6, 4, folder / "new66-grid.png")

    #Use the legacy panel's exact keyed bounds, rather than refitting its approved texture.
    raw = sprites.MANIFEST[7][1]
    with Image.open(raw) as image:
        panel = image.convert("RGBA").crop((0, 0, image.width // 3, image.height))
    red, green, blue, alpha = panel.split()
    key = ImageChops.multiply(red.point(lambda value: 255 if value >= 220 else 0),
                             blue.point(lambda value: 255 if value >= 220 else 0))
    key = ImageChops.multiply(key, green.point(lambda value: 255 if value <= 40 else 0))
    panel.putalpha(ImageChops.multiply(alpha, ImageOps.invert(key)))
    left, top, right, bottom = panel.getchannel("A").getbbox()
    underground = f"{raw}@{left},{top},{right - left},{bottom - top}"
    floor_specs = [underground] + [row[1] + "@" + ",".join(map(str, row[2])) + "@project-ground"
                                  for row in sprites.MANIFEST[47:50]]
    sprites.compare(47, str(folder / "deep-floor-beside-underground.png"), floor_specs,
                    palette, forbidden)
    for family, base in (("surface", 50), ("underground", 58)):
        for door, name in enumerate(("ne", "se", "sw", "nw")):
            offset, raw, box, *_ = sprites.MANIFEST[base + door * 2]
            depth = 10 if door in (1, 2) else 1
            output = plan / f"r2mlm13-c-composite-{family}-door-{name}.png"
            sprites.composite(offset, door, depth, str(output),
                              raw + "@" + ",".join(map(str, box)), palette, forbidden)
    for name, offset in (("deep-icon", 44), ("road-tool-icons", 45),
                         ("road-tunnel-cursor", 46), ("deep-floor-beside-underground", 47)):
        shutil.copyfile(folder / f"{name}.png", plan / f"r2mlm13-c-compare-{offset}.png")
    repaired = []
    for family in ("elevator-surface", "elevator-underground", "rail-surface", "rail-underground"):
        for name in ("ne", "se", "sw", "nw"):
            before = Image.open(ROOT / f"build/existing-pair-review/{family}-{name}-before.png").convert("RGB")
            after = Image.open(ROOT / f"build/existing-pair-review/{family}-{name}.png").convert("RGB")
            before = before.resize((before.width // 2, before.height // 2), Image.Resampling.NEAREST)
            after = after.resize((after.width // 2, after.height // 2), Image.Resampling.NEAREST)
            pair = Image.new("RGB", (before.width + after.width, max(before.height, after.height)), (64, 64, 64))
            pair.paste(before, (0, 0))
            pair.paste(after, (before.width, 0))
            path = folder / f"front-repair-{family}-{name}.png"
            pair.save(path)
            repaired.append((f"{family} {name}: old front on left, repaired front on right; synthetic vehicle guide.", path))
    stack_board("Sixteen fronts repaired from existing generated backs and near-door frames; raw artwork retained.",
                repaired, plan / "r2mlm13-d-replaced.png")
    entries = [("New offsets 44..65 at 4x; generated development candidates, awaiting in-game review.",
                folder / "new66-grid.png"),
               ("Four layer icons at 1x and 4x.", plan / "r2mlm13-c-compare-44.png"),
               ("Road tunnel and elevator tool icons at 1x and 4x.", plan / "r2mlm13-c-compare-45.png"),
               ("Road tunnel cursor uses the tool icon's raw picture.", plan / "r2mlm13-c-compare-46.png"),
               ("Underground floor followed by the three Deep Underground variants and 3x3 patches.",
                plan / "r2mlm13-c-compare-47.png")]
    for family in ("surface", "underground"):
        for name in ("ne", "se", "sw", "nw"):
            entries.append((f"{family} door {name}: synthetic vehicle occlusion, not a gameplay capture.",
                            plan / f"r2mlm13-c-composite-{family}-door-{name}.png"))
    stack_board("R2MLM13 new art: offline review only; operator approval and game captures pending.",
                entries, plan / "r2mlm13-d-new.png")
    print("All 66 sprites nonblank; forbidden opaque palette indices absent. Review boards prepared.")


if __name__ == "__main__":
    main()
