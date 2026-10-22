"""Locate substantial original packed orbital parts in actual paused native fields.

@file orbital_final_art_checks-2.py Original exact indexed part/alias matching.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19C.
"""
import hashlib
from pathlib import Path
import runpy

COMMON = runpy.run_path(str(Path(__file__).with_name("space_final_art_checks-2.py")))


def native_matches(native, manifest, sheet, assets):
    """Require every selected orbital family to expose substantial exact original pixels.

    @param native Complete actual ordinary indexed screenshot, never a composite.
    @param manifest Explicit independently qualified packed sprite/range manifest.
    @param sheet Exact selected original indexed packed sheet.
    @param assets Required genuine orbital families from the retained world contract.
    @return Exact native placements retaining every original logical part alias.
    """
    originals = {}
    # Aliases retain their logical identities but share one pixel search.
    # Empty/tiny parts cannot establish a family's visible native body; actual
    # foreground composition receives its own visual/geometry review.
    for row in manifest["sprites"]:
        if row["asset_id"] not in assets:
            continue
        x, y, width, height = row["box"]
        sprite = sheet.crop((x, y, x + width, y + height))
        pixels = sprite.tobytes()
        assert hashlib.sha256(pixels).hexdigest() == row["pixel_sha256"]
        if sum(value != 0 for value in pixels) < 48:
            continue
        key = row["asset_id"], sprite.size, row["pixel_sha256"]
        if key not in originals:
            originals[key] = (sprite, [])
        originals[key][1].append({field: row[field] for field in
                                 ("native_offset", "state", "frame", "direction", "tile_variant", "part")})
    matches = []
    # Search each unique substantial source part; aliases retain their logical
    # identities without pretending identical source pixels are distinct frames.
    for (asset, size, digest), (sprite, rows) in originals.items():
        for box in COMMON["positions"](native, sprite):
            matches.append(dict(asset_id=asset, native_box=box, source_pixel_sha256=digest, logical_aliases=rows))
    if not set(assets) <= {row["asset_id"] for row in matches}:
        raise ValueError("a required orbital family has no substantial exact native part")
    return matches


def main():
    """Require current explicit recipe inputs instead of a fixed historical packed cache."""
    raise SystemExit("Use orbital_final_art_checks-1.py with --selector-result, --build-result, --source-result and --run-id; pixels qualify within that actual current capture.")


if __name__ == "__main__":
    main()
