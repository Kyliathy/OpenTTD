"""Exact original-source expectations for the actual orbital native decoder.

@file orbital_art_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group18; explicit current source expectations.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K
This records source pixels and anchors, never a replacement GRF decoder.
"""
import hashlib
import json
from pathlib import Path
import struct

from PIL import Image
from multilayer_packed_art_fixture import pixel_hash, write_selected_payload

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "logs/proofs/mlm/native-art/expected/orbital-packed-sprites.bin"


def source_payload(report, image_path):
    """Encode the original orbital expectation protocol from explicit source packing.

    @param report Exact original packer report for all976orbital source poses.
    @param image_path Selected original source-derived indexed packing sheet.
    @return Original binary geometry/anchor/index-fingerprint expectations.
    """
    image = Image.open(image_path)
    records = []
    # Original native order includes transparent foreground cells and still
    # aliases. Missing pixels, holes, anchors or indices cannot count as loading.
    for index, row in enumerate(report["sprites"]):
        if row["native_offset"] != report["native_start"] + index:
            raise ValueError("orbital source native order changed")
        x, y, width, height = row["box"]
        pixels = image.crop((x, y, x + width, y + height)).tobytes()
        if hashlib.sha256(pixels).hexdigest() != row["pixel_sha256"]:
            raise ValueError("orbital selected original source pixels changed")
        records.append((row["native_offset"], width, height, *row["anchor"], pixel_hash(pixels)))
    image.close()
    # Every appended source slot retains the original native domain, including
    # transparent foreground cells and static aliases with independent IDs.
    if len(records) != 976 or report["native_start"] != 5167 or report["native_sprite_bound"] != 6143:
        raise ValueError("orbital source expectation coverage changed")
    payload = b"ORBART1\0" + struct.pack("<HHH", len(records), report["native_sprite_bound"], report["native_start"])
    payload += b"".join(struct.pack("<HHHhhI", *record) for record in records)
    return payload


def produce_selected(report, image_path, destination, prefixes=None):
    """Write original orbital source expectations to an explicit generated role.

    @param report Checked current orbital packing report.
    @param image_path Explicit current source-derived indexed sheet.
    @param destination Selected current binary data filename.
    @param prefixes Empty mapping; this protocol covers only the orbital append.
    @return Original expectation count, without qualifying the native decoder.
    """
    if prefixes:
        raise ValueError("orbital expectations do not consume generated prefix payloads")
    write_selected_payload(destination, source_payload(report, image_path))
    return 976


def main():
    """Retain the historical authoring entry until its routing guard migration.

    @return None; selected recipes call produce_selected without old proof-cache input.
    """
    report = json.loads((ROOT / "logs/proofs/mlm/native-art/space/orbital-packed-manifest.json").read_text(encoding="utf-8"))
    payload = source_payload(report, ROOT / "media/baseset/openttd/orbital_final.png")
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(payload)
    print("976 orbital source expectations staged; actual native decoding remains separate", flush=True)


if __name__ == "__main__":
    main()
