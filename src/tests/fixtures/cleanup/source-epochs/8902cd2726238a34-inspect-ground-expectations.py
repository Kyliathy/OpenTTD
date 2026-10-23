"""Compare original/current Ground source expectation domains without rebaselining.

@file inspect-ground-expectations.py Disposable diagnosis for r2fc-file-cleanup3.md18.
"""

import hashlib
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]


def rows(path):
    """Read the existing full-pixel fixture protocol for a diagnostic comparison.

    @param path Exact original or rejected-current binary file.
    @return Native offset to geometry/fingerprint/pixels, preserving all original bytes.
    """
    payload = path.read_bytes()
    count = struct.unpack_from("<H", payload, 8)[0]
    cursor, result = 16, {}
    # Traverse each independently identified source record, never a decoder or
    # renderer output. The declared pixel dimensions delimit its exact byte span.
    for _index in range(count):
        record = struct.unpack_from("<HHHhhI", payload, cursor)
        cursor += 14
        size = record[1] * record[2]
        result[record[0]] = (record[1:], payload[cursor:cursor + size])
        cursor += size
    assert cursor == len(payload)
    return result


old = rows(ROOT / "logs/proofs/mlm/native-art/expected/ground-packed-sprites.bin")
new = rows(ROOT / "mlm/reviews/multilayer-art/ground-expectations-first/cleanup17de-recipe-03/data/source-expected.bin")
different = [key for key in old if old[key] != new[key]]
print("different", len(different), "prior", sum(key < 7791 for key in different), "new", sum(key >= 7791 for key in different))
# Show a bounded concrete geometry/pixel sample; retain full binaries only as
# existing original witnesses or disposable rejected actuals, never new fixtures.
for key in different[:20]:
    print(key, "old", old[key][0], "new", new[key][0], "old_pixels_sha", hashlib.sha256(old[key][1]).hexdigest(), "new_pixels_sha", hashlib.sha256(new[key][1]).hexdigest())
