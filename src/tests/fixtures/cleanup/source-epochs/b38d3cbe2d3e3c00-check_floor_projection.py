"""Compare floor resampling footprints against the game's 2:1 tile lattice.

@see docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md
"""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "docs/zz-reach2/upgrades/2026-10/tools"))
import sprite_quantize as sprites


def main():
    """Measure transparent interior pixels and preview the native projected footprint.

    @return None.
    """
    palette, forbidden = sprites.load_palette(sprites.PALETTE_FILE)
    for offset in range(47, 50):
        _, raw, box, *_ = sprites.MANIFEST[offset]
        old = sprites.quantise_file(raw, box, *sprites.CONTRACT[offset], palette, forbidden)
        new = sprites.quantise_file(raw, box, *sprites.CONTRACT[offset], palette, forbidden,
                                   {"project_ground": True})
        for label, sprite in (("old", old), ("projected", new)):
            pw, ph, patch = sprites.ground_patch(*sprite)
            #The central tile region lies fully inside the nine-tile patch, far from its outer edge.
            holes = sum(patch[y * pw + x] == 0 for y in range(25, 57) for x in range(112, 144))
            print(offset, label, "central transparent pixels", holes)
            if label == "projected":
                assert holes == 0, f"floor {offset} has a transparent interior seam"
            picture = sprites.sprite_image((patch, pw, ph, 0, 0), palette)
            picture.resize((pw * 4, ph * 4), 0).save(
                ROOT / f"build/art-tool-checks/floor-projection-{offset}-{label}.png")


if __name__ == "__main__":
    main()
