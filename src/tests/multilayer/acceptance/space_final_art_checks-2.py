"""Match actual ordinary native pixels against explicitly selected original packed art.

@file space_final_art_checks-2.py Exact original indexed matching algorithms.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19C.
"""
import hashlib

def positions(native, sprite):
    """Locate an actual native sprite by every opaque source palette index.

    @param native Original complete indexed engine screenshot.
    @param sprite Exact cropped indexed packed sheet cell, with index0 transparency.
    @return Native top-left positions where all original opaque pixels match.
    """
    assert native.mode == sprite.mode == "P"
    data, pixels = native.tobytes(), sprite.tobytes()
    width, height = native.size
    sw, sh = sprite.size
    runs = []
    # A longest opaque row cheaply identifies candidates in the complete native field; every candidate still requires all opaque source pixels to agree.
    for y in range(sh):
        x = 0
        while x < sw:
            if pixels[y * sw + x] == 0:
                x += 1
                continue
            end = x + 1
            while end < sw and pixels[y * sw + end] != 0:
                end += 1
            runs.append((pixels[y * sw + x:y * sw + end], x, y))
            x = end
    run, rx, ry = max(runs, key=lambda entry: len(entry[0]))
    opaque = [(offset // sw, offset % sw, value) for offset, value in enumerate(pixels) if value]
    matches = []
    start = data.find(run)
    # Candidate rows cannot wrap around the screenshot boundary; transparent source pixels deliberately retain the world's actual background and neighbours.
    while start >= 0:
        x, y = start % width - rx, start // width - ry
        if 0 <= x <= width - sw and 0 <= y <= height - sh:
            if all(data[(y + py) * width + x + px] == value for py, px, value in opaque):
                matches.append([x, y, sw, sh])
        start = data.find(run, start + 1)
    return matches


def native_matches(native, manifest, sheet, asset, state):
    """Bind visible native pixels to one required final family and genuine state.

    @param native Exact indexed screenshot written by the actual engine.
    @param manifest Accepted packed pose/range/source identities.
    @param sheet Original current indexed packed sheet.
    @param asset Required actual family, never a preview substitute.
    @param state Required authored state, or None for actual natural-body class coverage.
    @return All exact native placements, retaining explicit aliases without claiming distinct frames.
    """
    matches = []
    # Only this actual required family/state qualifies the view. Static aliases may share pixels, but cannot substitute another family or an unearned work state.
    for row in manifest["sprites"]:
        if row["asset_id"] != asset or (state is not None and row["state"] != state):
            continue
        x, y, width, height = row["box"]
        sprite = sheet.crop((x, y, x + width, y + height))
        assert hashlib.sha256(sprite.tobytes()).hexdigest() == row["pixel_sha256"]
        for box in positions(native, sprite):
            matches.append(dict(native_offset=row["native_offset"], asset_id=asset, state=row["state"],
                                frame=row["frame"], direction=row["direction"], tile_variant=row["tile_variant"],
                                native_box=box, source_box=row["box"], source_pixel_sha256=row["pixel_sha256"]))
    return matches


def main():
    """Require the current explicit capture recipe instead of a historical proof cache."""
    raise SystemExit("Use space_final_art_checks-1.py with --selector-result, --build-result, --source-result and --run-id; pixels qualify within that actual current capture.")


if __name__ == "__main__":
    main()
