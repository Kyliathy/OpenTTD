"""Write exact native decoder expectations for final Space and preserved previews.

@file space_art_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG
The binary records source bytes and geometry; the real application decoder must
independently load them from the distributed GRF before acceptance.
"""
import json
from pathlib import Path
import struct
import sys

from PIL import Image

from multilayer_packed_art_fixture import pixel_hash

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_space_art as art


def main():
    """Stage exact source expectations without invoking or replacing a decoder.

    @return None; write only the repository-owned native-art proof fixture.
    """
    report, files = art.derive()
    scope = art.read_scope()
    # The fixture must observe the exact staged sheet/NFO/header used by the forthcoming native generator, including transparent holes and crop anchors.
    for path, data in files.items():
        if path.read_bytes() != data:
            raise ValueError("stage current Space packing inputs before loader expectations")
    sheet = Image.open(ROOT / scope["png"])
    directory = ROOT / "media/baseset/openttd"
    preview = Image.open(directory / "space_preview.png")
    rows = [line.split() for line in (directory / "space_preview.nfo").read_text().splitlines()
            if "sprites/space_preview.png 8bpp" in line]
    records = []
    # All eleven old preview sprites retain their historical IDs/pixels even after final runtime consumers stop using them.
    for index, row in enumerate(rows):
        x, y, width, height, rx, ry = map(int, row[3:9])
        pixels = preview.crop((x, y, x + width, y + height)).tobytes()
        records.append((3588 + index, width, height, rx, ry, pixel_hash(pixels)))
    if len(records) != 11:
        raise ValueError("preserved Space preview count changed")
    # Every explicit new native identity is compared against actual source crop bytes; repeated logical aliases still receive independent loader checks.
    for row in report["sprites"]:
        x, y, width, height = row["box"]
        rx, ry = row["anchor"]
        records.append((row["native_offset"], width, height, rx, ry,
                        pixel_hash(sheet.crop((x, y, x + width, y + height)).tobytes())))
    destination = ROOT / "logs/proofs/mlm/native-art/expected/space-packed-sprites.bin"
    destination.parent.mkdir(parents=True, exist_ok=True)
    payload = b"SPCART1\0" + struct.pack("<HHH", len(records), report["native_sprite_bound"], 3588)
    payload += b"".join(struct.pack("<HHHhhI", *record) for record in records)
    destination.write_bytes(payload)
    print(json.dumps(dict(records=len(records), final_space=report["pose_count"], protected_previews=11,
                          native_bound=report["native_sprite_bound"], fixture=destination.relative_to(ROOT).as_posix())))


if __name__ == "__main__":
    main()
