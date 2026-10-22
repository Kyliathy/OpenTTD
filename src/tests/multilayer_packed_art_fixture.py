"""Write exact native-loader sprite expectations from staged M1/M0 source art.

@file multilayer_packed_art_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group18; explicit current source expectations.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.6
The fixture records source dimensions, anchors and indexed-pixel hashes. Native
loading must reproduce those pixels; the fixture is not a second sprite loader.
"""

import json
from pathlib import Path
import struct
import sys

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "logs/proofs/mlm/native-art/expected/m1-packed-sprites.bin"
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import contained


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


def source_payload(report, image_path):
    """Encode the original M0/M1 source expectation protocol from selected packing.

    @param report Explicit original packer report; selection/hash checks belong to its caller.
    @param image_path Exact selected indexed packing sheet, never a GRF decoder output.
    @return Deterministic original binary protocol including every protected M0 sprite.
    """
    directory = ROOT / "media/baseset/openttd"
    current = Image.open(image_path)
    legacy = Image.open(directory / "map_layers.png")
    rows = [line.split() for line in (directory / "map_layers.nfo").read_text(encoding="utf-8").splitlines()
            if "sprites/map_layers.png 8bpp" in line]
    records = []
    # Protect every old published native sprite, not just a representative icon.
    # Existing NFO crop/anchor geometry is read verbatim, with no source rewrite.
    for offset, row in enumerate(rows):
        x, y, width, height, rx, ry = map(int, row[3:9])
        records.append((offset, width, height, rx, ry, pixel_hash(legacy.crop((x, y, x + width, y + height)).tobytes())))
    # Every selected logical pose retains its published native ID and cropped
    # source anchor; source aliases still receive independent decoder records.
    for sprite in report["sprites"]:
        x, y, width, height = sprite["box"]
        rx, ry = sprite["anchor"]
        records.append((sprite["native_offset"], width, height, rx, ry, pixel_hash(current.crop((x, y, x + width, y + height)).tobytes())))
    payload = b"MLVART1\0" + struct.pack("<HH", len(records), report["native_sprite_bound"])
    payload += b"".join(struct.pack("<HHHhhI", *record) for record in records)
    current.close()
    legacy.close()
    # Existing native IDs include reserved gaps. Complete source cardinality
    # and the original bound must survive expectation regeneration unchanged.
    if len(records) != 3561 or report["native_sprite_bound"] != 3588:
        raise ValueError("M0/M1 source expectation coverage changed")
    return payload


def write_selected_payload(destination, payload):
    """Write only an explicitly selected generated binary data role.

    @param destination Exact current review data filename, supplied by the caller.
    @param payload Original source-derived binary protocol bytes.
    @return None; source/media/log/runtime owners cannot become destinations.
    """
    path = contained(destination, ROOT / "mlm/reviews")
    parts = path.relative_to(ROOT / "mlm/reviews").parts
    # Generated expectations are current data, separate from immutable sources
    # and process evidence; traversal or another role cannot authorize a write.
    if len(parts) != 5 or parts[3] != "data" or path.suffix != ".bin":
        raise ValueError("selected sprite expectation requires an explicit generated data file")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)


def produce_selected(report, image_path, destination, prefixes=None):
    """Run the original source expectation algorithm with explicit current inputs/output.

    @param report Selected original M1 packing report, already identity/role checked.
    @param image_path Current source-derived indexed packing sheet.
    @param destination Explicit current binary data destination.
    @param prefixes Empty mapping; M0 originals are durable media inputs.
    @return Original payload record count; actual native loading remains separate.
    """
    if prefixes:
        raise ValueError("M1 expectations do not consume generated prefix payloads")
    write_selected_payload(destination, source_payload(report, image_path))
    return 3561


def main():
    """Retain the historical authoring entry until its routing guard migration.

    @return None; selected cleanup recipes use produce_selected without old proof reads.
    """
    report = json.loads((ROOT / "logs/proofs/mlm/native-art/derived/packed-manifest.json").read_text(encoding="utf-8"))
    payload = source_payload(report, ROOT / "media/baseset/openttd/map_layers_m1.png")
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(payload)
    # Native tests materialize the immutable configuration through their shared
    # runtime helper; producing pixel expectations must not modify build startup.
    print(f"Native art expectations: 3561 protected/selected sprites, bound {report['native_sprite_bound']}; actual application loading remains separate")


if __name__ == "__main__":
    main()
