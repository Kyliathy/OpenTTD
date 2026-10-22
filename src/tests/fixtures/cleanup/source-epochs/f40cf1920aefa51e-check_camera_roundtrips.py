"""Verify all map chunks and VIEW coordinates in actual paused GUI round trips."""

import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location("digest", "docs/zz-reach2/upgrades/2026-10/tools/savegame_plane_digest.py")
digest = importlib.util.module_from_spec(spec)
spec.loader.exec_module(digest)


def read_chunks(path):
    """Read the relevant saved chunks without modifying the file.

    @param path Saved GUI fixture.
    @return Version, map chunks and the sole VIEW data record.
    """
    version, stream = digest.read_stream(path)
    reader = digest.Reader(stream)
    planes = {}
    view = None
    while True:
        chunk = reader.take(4)
        if chunk == bytes(4):
            break
        mode = reader.byte()
        if mode & 15 == digest.CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            payload = reader.take(length)
            if chunk in digest.PLANE_BYTES:
                planes[chunk] = payload
        else:
            records = []
            while True:
                length = reader.gamma()
                if not length:
                    break
                records.append(reader.take(length - 1))
            if chunk == b"VIEW":
                assert len(records) == 2 and len(records[1]) == 9
                view = records[1]
    return version, planes, view


views = []
for layer in ("underground", "deep", "space"):
    before = read_chunks(Path(f"build/save/camera-{layer}-repaired.sav"))
    after = read_chunks(Path(f"build/save/camera-{layer}-roundtrip.sav"))
    assert before[0] == after[0] == 370
    assert set(before[1]) == set(digest.PLANE_BYTES)
    assert len(before[1][b"MAPT"]) == 4 * 256 * 256
    assert before[1] == after[1], f"map chunk changed in {layer} round trip"
    assert before[2] == after[2], f"camera changed after loading from {layer}"
    views.append(before[2])
    print(layer, "all 10 map chunks identical; 4 x 256x256 planes; VIEW preserved")
assert len(set(views)) == 1, "same plane position has different saved cameras by layer"
print("All three layer saves encode the same Surface camera:", views[0].hex())
