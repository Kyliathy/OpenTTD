"""Inspect dimensions and map tiles in the supplied save without changing it."""

import importlib.util
import sys
from collections import Counter
from pathlib import Path

spec = importlib.util.spec_from_file_location("digest", "docs/zz-reach2/upgrades/2026-10/tools/savegame_plane_digest.py")
digest = importlib.util.module_from_spec(spec)
spec.loader.exec_module(digest)
version, data = digest.read_stream(sys.argv[1] if len(sys.argv) > 1 else "build/save/makinpr2.sav")
reader = digest.Reader(data)
planes = {}
while True:
    chunk = reader.take(4)
    if chunk == bytes(4):
        break
    mode = reader.byte()
    if mode & 15 == 0:
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
        if chunk == b"MAPS":
            print("MAPS records", [record.hex() for record in records])
        if chunk in (b"VEHS", b"STNS", b"ORDR", b"ORDL"):
            print(chunk.decode(), "records", len(records), "lengths", [len(record) for record in records[:8]])
            Path("build/makinpr2-review/" + chunk.decode() + ".records").write_bytes(b"".join(records))
print("version", version)
for layer in range(4):
    types = planes[b"MAPT"][layer * 65536:(layer + 1) * 65536]
    print("layer", layer, "type counts", Counter(tile >> 4 for tile in types))
    for pos, tile_type in enumerate(types):
        if tile_type >> 4 in (1, 5, 11):
            tile = layer * 65536 + pos
            print("tile", tile, "xy", pos % 256, pos // 256, "type", tile_type >> 4,
                  "m2", int.from_bytes(planes[b"MAP2"][tile * 2:tile * 2 + 2], "big"),
                  "m3", planes[b"M3LO"][tile], "m4", planes[b"M3HI"][tile], "m5", planes[b"MAP5"][tile])
digest.list_portals(planes, 256, 256)
