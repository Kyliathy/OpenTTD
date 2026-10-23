"""Write exact native-loader sprite expectations from staged M1/M0 source art.

@file multilayer_packed_art_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.6
The fixture records source dimensions, anchors and indexed-pixel hashes. Native
loading must reproduce those pixels; the fixture is not a second sprite loader.
"""

import json
from pathlib import Path
import struct

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "logs/proofs/mlm/native-art/expected/m1-packed-sprites.bin"


def pixel_hash(pixels):
    """Hash exact DOS-index pixels with portable unsigned FNV-1a32 arithmetic.

    @param pixels Native indexed bytes in source row-major order.
    @return Unsigned 32-bit expected decoded sprite hash.
    """
    value = 2166136261
    # A pixel-wise fingerprint includes transparent holes as well as material
    # colors, detecting palette, mask and crop drift in the real native decoder.
    for pixel in pixels:
        value = ((value ^ pixel) * 16777619) & 0xFFFFFFFF
    return value


def main():
    """Write protected-M0 and selected-M1 expectations inside the feature fixtures.

    @return None; print the exact actual source record count without loading a GRF.
    """
    directory = ROOT / "media/baseset/openttd"
    report = json.loads((ROOT / "logs/proofs/mlm/native-art/derived/packed-manifest.json").read_text(encoding="utf-8"))
    current = Image.open(directory / "map_layers_m1.png")
    legacy = Image.open(directory / "map_layers.png")
    rows = [line.split() for line in (directory / "map_layers.nfo").read_text(encoding="utf-8").splitlines()
            if "sprites/map_layers.png 8bpp" in line]
    records = []
    # Protect every old published native sprite, not just a representative icon.
    # Existing NFO crop/anchor geometry is read verbatim, with no source rewrite.
    for offset, row in enumerate(rows):
        x, y, width, height, rx, ry = map(int, row[3:9])
        records.append((offset, width, height, rx, ry, pixel_hash(legacy.crop((x, y, x + width, y + height)).tobytes())))
    for sprite in report["sprites"]:
        x, y, width, height = sprite["box"]
        rx, ry = sprite["anchor"]
        records.append((sprite["native_offset"], width, height, rx, ry, pixel_hash(current.crop((x, y, x + width, y + height)).tobytes())))
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    payload = b"MLVART1\0" + struct.pack("<HH", len(records), report["native_sprite_bound"])
    payload += b"".join(struct.pack("<HHHhhI", *record) for record in records)
    OUTPUT.write_bytes(payload)
    # Native tests materialize the immutable configuration through their shared
    # runtime helper; producing pixel expectations must not modify build startup.
    print(f"Native art expectations: {len(records)} protected/selected sprites, bound {report['native_sprite_bound']}; actual application loading remains separate")


if __name__ == "__main__":
    main()
