"""Write exact native decoder expectations for final Space and preserved previews.

@file space_art_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group18; explicit current source expectations.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG
The binary records source bytes and geometry; the real application decoder must
independently load them from the distributed GRF before acceptance.
"""
import json
from pathlib import Path
import struct
import sys

from PIL import Image

from multilayer_packed_art_fixture import pixel_hash, write_selected_payload

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_space_art as art


def source_payload(report, image_path):
    """Encode original Space source expectations while retaining all eleven previews.

    @param report Explicit source-derived Space packing report.
    @param image_path Exact selected indexed packing sheet, independent of native decoding.
    @return Original deterministic Space binary protocol bytes.
    """
    sheet = Image.open(image_path)
    directory = ROOT / "media/baseset/openttd"
    preview = Image.open(directory / "space_preview.png")
    rows = [line.split() for line in (directory / "space_preview.nfo").read_text(encoding="utf-8").splitlines()
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
    payload = b"SPCART1\0" + struct.pack("<HHH", len(records), report["native_sprite_bound"], 3588)
    payload += b"".join(struct.pack("<HHHhhI", *record) for record in records)
    sheet.close()
    preview.close()
    # Every new Space pose and protected preview retains its published identity;
    # a smaller current source matrix must not appear as a successful payload.
    if len(records) != 1579 or report["native_sprite_bound"] != 5167:
        raise ValueError("Space source expectation coverage changed")
    return payload


def produce_selected(report, image_path, destination, prefixes=None):
    """Write original Space expectations from explicit current packing inputs.

    @param report Checked current Space packing report.
    @param image_path Selected original source-derived indexed sheet.
    @param destination Explicit current binary data output.
    @param prefixes Empty mapping; preserved previews remain authored media inputs.
    @return Complete original native expectation count, without native acceptance.
    """
    if prefixes:
        raise ValueError("Space expectations do not consume generated prefix payloads")
    write_selected_payload(destination, source_payload(report, image_path))
    return 1579


def main():
    """Retain the historical authoring entry until its routing guard migration.

    @return None; selected recipes use explicit produce_selected without this old chain.
    """
    report, files = art.derive()
    scope = art.read_scope()
    # Historical authoring still checks its staged source files; selected
    # recipes supply their already verified current packing directly instead.
    for path, data in files.items():
        if path.read_bytes() != data:
            raise ValueError("stage current Space packing inputs before loader expectations")
    destination = ROOT / "logs/proofs/mlm/native-art/expected/space-packed-sprites.bin"
    destination.parent.mkdir(parents=True, exist_ok=True)
    payload = source_payload(report, ROOT / scope["png"])
    destination.write_bytes(payload)
    print(json.dumps(dict(records=1579, final_space=report["pose_count"], protected_previews=11,
                          native_bound=report["native_sprite_bound"], fixture=destination.relative_to(ROOT).as_posix())))


if __name__ == "__main__":
    main()
