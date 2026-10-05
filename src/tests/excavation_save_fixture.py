#!/usr/bin/env python3
"""Actual sparse-job progress round trips and required/invalid owner-family refusals.

@file excavation_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 10B
"""

import hashlib
import struct
from functools import partial

from geology_save_fixture import chunks, geology_chunk, variant, read_stream
from multilayer_state_fixture import AREA, ROOT, run_case

PREFIX = "r2mlvs2-o"
JOB_FORMAT = ">IBIIHIBBIQQQQqHHH"


def world_tile(x, y):
    """Encode a bounded owned fixture reference on the saved 64x64 Underground plane.

    @param x Saved layer-local column.
    @param y Saved layer-local row.
    @return Explicit serialized world index; gameplay resolves it through its own layer controller.
    """
    return 4096 + y * 64 + x


def active_job(records):
    """Supply explicit removed-owner paused geometry with 77 saved work ticks; no company/vehicle/stock is fabricated.

    @param records Owned version-373 native chunks; unrelated raw fields remain exact.
    @return None; dense descriptor headers are retained from the actual new writer.
    """
    metadata = geology_chunk(records, b"MLMD")
    row = bytearray(metadata["rows"][1])
    if len(row) != 51:
        raise ValueError("unexpected metadata row")
    row[31:35] = (int.from_bytes(row[31:35], "big") | 4).to_bytes(4, "big")
    metadata["rows"][1] = bytes(row)
    anchor = world_tile(20, 20)
    requested = [world_tile(22, 20)]
    authorized = [world_tile(x, y) for y in range(19, 22) for x in range(20, 24)]
    routes, tiles = [], []
    # Explicit canonical floor references precede segment children, matching the producer's actual descriptor order.
    for kind, floor in enumerate((requested, authorized)):
        tiles.extend(struct.pack(">IBHHI", 0, kind, 0, i, tile) for i, tile in enumerate(floor))
    swept = set()
    # Actual native SW heading advances +x; each bounded 3x3 envelope names its real support floor explicitly.
    for route in range(2):
        x = 21 + route
        envelope = [world_tile(px, py) for py in range(19, 22) for px in range(x - 1, x + 2)]
        swept.update(envelope)
        routes.append(struct.pack(">IHIBBB", 0, route, world_tile(x, 20), 5, 9, 1))
        for kind, floor in ((2, envelope), (3, [anchor])):
            tiles.extend(struct.pack(">IBHHI", 0, kind, route, i, tile) for i, tile in enumerate(floor))
    header = geology_chunk(records, b"EXJS")
    header["payload"] = struct.pack(">IIIII", 1, 1, 1, len(routes), len(tiles))
    jobs = geology_chunk(records, b"XJOB")
    # Public VehicleID/StationID invalid sentinels are 0xFFFFF/0xFFFF; this is deliberately an unbound geometry reservation.
    jobs["rows"] = jobs["rows"][:1] + [struct.pack(JOB_FORMAT, 0, 255, 0xFFFFF, 0xFFFF, 0, anchor, 3, 9, 0, 77, 2, 0, 0, 0, len(requested), len(authorized), 2)]
    route_chunk = geology_chunk(records, b"XJRT")
    route_chunk["rows"] = route_chunk["rows"][:1] + routes
    tile_chunk = geology_chunk(records, b"XJTL")
    tile_chunk["rows"] = tile_chunk["rows"][:1] + tiles
    terrain = geology_chunk(records, b"GEOS")
    payload = bytearray(terrain["payload"])
    # Every active packed cell has this exact saved owner; the existing support floor remains excavated.
    for tile in sorted(swept):
        local = tile - 4096
        shift = (local % 4) * 2
        index = 29 + local // 4
        state = 2 if tile == anchor else 1
        payload[index] = (payload[index] & ~(3 << shift)) | (state << shift)
    terrain["payload"] = bytes(payload)


def corrupt_job(records, kind):
    """Create one targeted reference/claim/family failure in owned active-job transport.

    @param records Actual complete active input chunks.
    @param kind Missing family, invalid cutter, orphan terrain or competing second job.
    @return None; refusal must precede startup/save and preserve every supplied predecessor.
    """
    if kind == "missing":
        geology_chunk(records, b"XJTL")["remove"] = True
        return
    if kind == "reference":
        jobs = geology_chunk(records, b"XJOB")
        row = bytearray(jobs["rows"][1])
        row[5:9] = (123456).to_bytes(4, "big")
        jobs["rows"][1] = bytes(row)
        return
    if kind == "orphan":
        terrain = geology_chunk(records, b"GEOS")
        payload = bytearray(terrain["payload"])
        local = 30 * 64 + 30
        payload[29 + local // 4] |= 1 << ((local % 4) * 2)
        terrain["payload"] = bytes(payload)
        return
    if kind != "conflict":
        raise ValueError("unknown job fixture corruption")
    header = geology_chunk(records, b"EXJS")
    _, _, _, routes, tiles = struct.unpack(">IIIII", header["payload"])
    header["payload"] = struct.pack(">IIIII", 1, 2, 2, routes * 2, tiles * 2)
    # A second real saved identity names the same cells; exclusive reconstruction must refuse before publishing either candidate.
    for label in (b"XJOB", b"XJRT", b"XJTL"):
        chunk = geology_chunk(records, label)
        children = []
        for original in chunk["rows"][1:]:
            row = bytearray(original)
            row[:4] = (1).to_bytes(4, "big")
            children.append(bytes(row))
        chunk["rows"].extend(children)


def main():
    """Exercise actual new/active/repeated/predecessor saves and four targeted job failures.

    @return None; restore startup/config and verify every supplied predecessor SHA256 in finally.
    """
    startup, config = ROOT / "build/scripts/game_start.scr", ROOT / "build/grftest.cfg"
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    older = AREA / "r2mlvs2-i-native.sav"
    older_hash = hashlib.sha256(older.read_bytes()).hexdigest()
    try:
        config.write_text("# @see r2mlvs2-geology-and-excavation.md, sparse-job fixture.\n"
                          "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                          "gameplay_profile = multilayer_industry\n"
                          "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n", encoding="utf-8")
        created = run_case("native", ["-G", "1234"], "MLVS_GEOLOGY generator=1", prefix=PREFIX)
        assert read_stream(created)[0] in (373, 374, 375, 376, 377, 378), "job fixture requires delivered sparse-job writer"
        active = variant(created, "active", active_job, prefix=PREFIX)
        expected = "MLVS_EXCAVATION_JOB id=0 state=3 cursor=0 partial=77 revision=2"
        loaded = run_case("active", ["-g", active.as_posix()], expected, prefix=PREFIX)
        repeated = run_case("active-reload", ["-g", loaded.as_posix()], expected, prefix=PREFIX)
        original = {record["name"]: record["raw"] for record in chunks(read_stream(active)[1])}
        # Real descriptor round trips preserve authority exactly; rebuilt cache allocations never enter the file.
        for output in (loaded, repeated):
            restored = {record["name"]: record["raw"] for record in chunks(read_stream(output)[1])}
            for label in (b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"EXJS", b"XJOB", b"XJRT", b"XJTL"):
                assert original[label] == restored[label], (label, "job/geological authority changed")
        run_case("older-geology", ["-g", older.as_posix()], "MLVS_GEOLOGY_LOADED generator=1", prefix=PREFIX)
        # Each corruption targets this current group only; earlier Surface/industry regressions are not rerun.
        for kind, expected_error in (("missing", "Incomplete excavation family"), ("reference", "Invalid geology authority"),
                                     ("orphan", "Invalid geology authority"), ("conflict", "Invalid geology authority")):
            damaged = variant(active, kind, partial(corrupt_job, kind=kind), prefix=PREFIX)
            run_case(kind, ["-g", damaged.as_posix()], expected_error, accepted=False, prefix=PREFIX)
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        assert hashlib.sha256(older.read_bytes()).hexdigest() == older_hash, "supplied predecessor changed"
    (AREA / f"{PREFIX}-job-save-results.txt").write_text(
        "R2MLVS2 Group 10B: new/active/repeated/geology-only predecessor and four targeted job failures passed.\n"
        "All eight geological/job chunks remained byte-exact through two actual active-progress reloads.\n"
        "Partial work remained 77, cursor 0, revision 2; competing/orphan/missing/dangling references refused.\n"
        "Original startup/config/predecessor bytes preserved. Actual TBM/wagon controller remains Plans 3/4/5.\n", encoding="utf-8")


if __name__ == "__main__":
    main()
