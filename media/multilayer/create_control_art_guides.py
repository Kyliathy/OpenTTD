"""Expose retained native layer, machinery and cargo art for control authoring.

@file create_control_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group 78B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint 8.5
The composed reference is a guide, never a new source sprite or runtime payload.
"""

from pathlib import Path

from PIL import Image, ImageDraw, PngImagePlugin


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "media/multilayer/guides/m1"


def native_layer_icon(offset):
    """Recover an existing native icon at its uncropped original anchor.

    @param offset Protected M0 map-layer sprite offset with a 20x20 icon canvas.
    @return RGBA 20x20 reference preserving the retained original pixels.
    """
    directory = ROOT / "media/baseset/openttd"
    rows = [line.split() for line in (directory / "map_layers.nfo").read_text(encoding="utf-8").splitlines()
            if "sprites/map_layers.png 8bpp" in line]
    x, y, width, height, rel_x, rel_y = map(int, rows[offset][3:9])
    if width > 20 or height > 20 or rel_x < 0 or rel_y < 0 or rel_x + width > 20 or rel_y + height > 20:
        # This guide reuses only native menu icons. A housing or cursor's world
        # anchor cannot be reinterpreted as a small UI icon crop.
        raise ValueError("requested native sprite is not a 20x20 icon")
    indexed = Image.open(directory / "map_layers.png")
    sheet = indexed.convert("RGBA")
    # Base-set PNGs may omit a PNG transparency chunk because GRF loading owns
    # index-zero transparency. Preserve that native policy in the author guide.
    mask = Image.new("L", indexed.size)
    mask.putdata([0 if value == 0 else 255 for value in indexed.get_flattened_data()])
    sheet.putalpha(mask)
    result = Image.new("RGBA", (20, 20))
    result.paste(sheet.crop((x, y, x + width, y + height)), (rel_x, rel_y))
    return result


def create_guide():
    """Compose exact retained silhouettes and labeled native control purposes.

    @return Repository PNG path, with existing source/template owner metadata.
    """
    machinery = Image.open(ROOT / "logs/proofs/mlm/native-art/derived/purchase-model-00-authored.png").convert("RGBA")
    locked = Image.open(ROOT / "logs/proofs/mlm/native-art/derived/purchase-model-01-authored.png").convert("RGBA")
    references = [("Underground layer", native_layer_icon(1)), ("Rail portal", native_layer_icon(4)),
                  ("Deep layer", native_layer_icon(44)), ("Selected cutter", machinery.crop((0, 0, 20, 20))),
                  ("Existing UI lock", locked.crop((20, 0, 40, 20))),
                  ("Earth cargo", Image.open(ROOT / "logs/proofs/mlm/native-art/derived/cargo-earth.png").convert("RGBA")),
                  ("Rock cargo", Image.open(ROOT / "logs/proofs/mlm/native-art/derived/cargo-rock.png").convert("RGBA"))]
    guide = Image.new("RGB", (7 * 184, 258), (44, 47, 51))
    draw = ImageDraw.Draw(guide)
    draw.text((12, 12), "RETAINED ART REFERENCE: exact native pixels; enlarge only for source-authoring clarity", fill="white")
    # Reference roles stay explicit. The lock example is a menu cue and must
    # never be copied onto a world vehicle or granted construction permissions.
    for index, (label, pixels) in enumerate(references):
        large = pixels.resize((140, 140), Image.Resampling.NEAREST)
        guide.paste(large, (index * 184 + 16, 42), large)
        draw.text((index * 184 + 16, 194), label, fill="white")
    draw.text((12, 220), "New controls: survey / commissioning / floor outline / 45-degree advancing turn / exchange / Deep lock / calendar lock", fill="white")
    draw.text((12, 238), "20x20, anchor(0,0); glyphs only, no widget bevel. Enabled / disabled-refusal / pressed. No orbital controls.", fill="white")
    path = ASSETS / "ground-control-reference-guide.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Group78B; retained UI reference only")
    guide.save(path, pnginfo=metadata)
    return path


if __name__ == "__main__":
    print(create_guide())
