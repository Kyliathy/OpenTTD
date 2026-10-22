"""Check the dates, dimensions and complete map data of actual legacy save runs."""

import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location("digest", "docs/zz-reach2/upgrades/2026-10/tools/savegame_plane_digest.py")
digest = importlib.util.module_from_spec(spec)
spec.loader.exec_module(digest)


def chunks(path):
    """Read dimensions, calendar date and map chunks from a saved test fixture."""
    version, stream = digest.read_stream(path)
    reader = digest.Reader(stream)
    maps, dimensions, date = {}, None, None
    while True:
        chunk = reader.take(4)
        if chunk == bytes(4):
            break
        mode = reader.byte()
        if mode & 15 == digest.CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            payload = reader.take(length)
            if chunk in digest.PLANE_BYTES:
                maps[chunk] = payload
        else:
            records = []
            while True:
                length = reader.gamma()
                if not length:
                    break
                records.append(reader.take(length - 1))
            if chunk == b"MAPS":
                dimensions = (int.from_bytes(records[-1][:4], "big"), int.from_bytes(records[-1][4:8], "big"), records[-1][8])
            if chunk == b"DATE":
                date = int.from_bytes(records[-1][:4], "big", signed=True)
    assert version == 370 and dimensions == (256, 256, 4)
    assert set(maps) == set(digest.PLANE_BYTES)
    return maps, date


loaded = chunks("build/save/final_legacy_loaded.sav")
reloaded = chunks("build/save/final_legacy_roundtrip.sav")
assert loaded == reloaded
preplay = chunks("build/save/final_legacy_preplay.sav")
month = chunks("build/save/final_legacy_month.sav")
month_reload = chunks("build/save/final_legacy_month_roundtrip.sav")
assert month == month_reload
assert month[1] - preplay[1] >= 28
print("TTD trt00.sv0: Debug load and reload; four 256x256 planes; all ten map chunks identical")
print(f"Debug month run: calendar days {preplay[1]} -> {month[1]} ({month[1] - preplay[1]} days); played save reload preserves all ten map chunks")
