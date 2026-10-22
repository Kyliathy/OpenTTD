"""Compare final world generation and upgrade saves with the immutable pre-epic reference."""

import hashlib
import importlib.util
from pathlib import Path

tool = Path("docs/zz-reach2/upgrades/2026-10/tools/savegame_plane_digest.py")
spec = importlib.util.spec_from_file_location("digest", tool)
digest = importlib.util.module_from_spec(spec)
spec.loader.exec_module(digest)


def read_maps(path):
    """Read actual map chunks and dimensions without modifying the save."""
    version, stream = digest.read_stream(path)
    reader = digest.Reader(stream)
    planes, dimensions = {}, None
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
            assert mode & 15 in digest.CHUNK_RECORDS
            records = []
            while True:
                length = reader.gamma()
                if not length:
                    break
                records.append(reader.take(length - 1))
            if chunk == b"MAPS":
                body = records[-1]
                dimensions = (int.from_bytes(body[:4], "big"), int.from_bytes(body[4:8], "big"), body[8] if version >= 369 else 1)
    assert set(planes) == set(digest.PLANE_BYTES)
    return version, dimensions, planes


reference, current = {}, None
for line in tool.with_name("baseline-digests.txt").read_text().splitlines():
    if line.startswith("file "):
        current = Path(line.split()[1]).stem
        reference[current] = {}
    elif " sha256 " in line:
        parts = line.split()
        reference[current][parts[0].encode()] = parts[-1]

report = ["# Final build map evidence, R2RVM2 task 18.1, 2026-10-04; original reference unchanged."]
for case in ("1234_8x8", "98765_8x8", "1234_9x10", "98765_9x10"):
    size_x, size_y = ((256, 256) if case.endswith("8x8") else (512, 1024))
    tile_count = size_x * size_y
    saves = {}
    for prefix, baseline in (("final", "baseline"), ("final_upgraded", "baseline_reload"), ("final_roundtrip", "baseline_reload")):
        path = Path(f"build/save/{prefix}_{case}.sav")
        version, dimensions, planes = read_maps(path)
        assert version == 370 and dimensions == (size_x, size_y, 4), (path, version, dimensions)
        saves[prefix] = planes
        report.append(f"file {path.as_posix()} version {version} dimensions {size_x}x{size_y} layers 4")
        for chunk, bytes_per_tile in digest.PLANE_BYTES.items():
            payload = planes[chunk]
            assert len(payload) == 4 * tile_count * bytes_per_tile
            surface = hashlib.sha256(payload[:tile_count * bytes_per_tile]).hexdigest()
            assert surface == reference[f"{baseline}_{case}"][chunk], f"Surface changed: {path}, {chunk}"
            report.append(f"{chunk.decode()} length {len(payload)} surface_sha256 {surface} full_sha256 {hashlib.sha256(payload).hexdigest()}")
    assert saves["final_upgraded"] == saves["final_roundtrip"], f"Map changed on round trip: {case}"
    print(f"PASS {case}: generated Surface matches baseline; upgraded/reloaded Surface matches baseline reload; all ten four-layer chunks survive round trip")
    report.append(f"PASS {case}: I-6 generation; I-7 upgrade and complete four-layer round trip")
tool.with_name("final-layer-save-digests.txt").write_text("\n".join(report) + "\n", encoding="utf-8")
