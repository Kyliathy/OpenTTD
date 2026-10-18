"""Exact original-source expectations for the actual orbital native decoder.

@file orbital_art_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K
This records source pixels and anchors, never a replacement GRF decoder.
"""
import hashlib
import json
from pathlib import Path
import struct

from PIL import Image
from multilayer_packed_art_fixture import pixel_hash

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "logs/proofs/mlm/native-art/expected/orbital-packed-sprites.bin"


def main():
    """Write each appended sprite's exact crop/anchor/indexed-pixel expectation.

    @return None; the actual application must independently decode these bytes.
    """
    report = json.loads((ROOT / "logs/proofs/mlm/native-art/space/orbital-packed-manifest.json").read_text())
    image = Image.open(ROOT / "media/baseset/openttd/orbital_final.png")
    records = []
    # Original native order includes transparent foreground cells and still
    # aliases. Missing pixels, holes, anchors or indices cannot count as loading.
    for index, row in enumerate(report["sprites"]):
        assert row["native_offset"] == report["native_start"] + index
        x, y, width, height = row["box"]
        pixels = image.crop((x, y, x + width, y + height)).tobytes()
        assert hashlib.sha256(pixels).hexdigest() == row["pixel_sha256"]
        records.append((row["native_offset"], width, height, *row["anchor"], pixel_hash(pixels)))
    assert len(records) == 976
    assert report["native_start"] == 5167
    assert report["native_sprite_bound"] == 6143
    payload = b"ORBART1\0" + struct.pack("<HHH", len(records), report["native_sprite_bound"], report["native_start"])
    payload += b"".join(struct.pack("<HHHhhI", *record) for record in records)
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(payload)
    print("976 orbital source expectations staged; actual native decoding remains separate", flush=True)


if __name__ == "__main__":
    main()
