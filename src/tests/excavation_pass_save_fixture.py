#!/usr/bin/env python3
"""Actual frozen-pass reloads and missing/tampered/orphan input refusals.

@file excavation_pass_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 14
"""

import struct
from functools import partial

from excavation_save_fixture import active_job, world_tile
from geology_save_fixture import chunks, geology_chunk, read_stream, variant
from multilayer_state_fixture import AREA, ROOT, run_case

PREFIX = "r2mlvs2-u"


def frozen_pass(records):
    """Attach canonical static proof to actual saved paused geometry.

    @param records Actual version-374 native transport, preserving unrelated chunks.
    @return None; all proof fields encode the producer's exact public geometry.
    """
    active_job(records)
    metadata = geology_chunk(records, b"MLMD")
    row = bytearray(metadata["rows"][1])
    row[31:35] = (int.from_bytes(row[31:35], "big") | 8).to_bytes(4, "big")
    metadata["rows"][1] = bytes(row)
    anchor = world_tile(20, 20)
    encoded = bytearray()

    def value(number, width):
        """Append one exact portable field to this fixture's static proof.

        @param number Actual unsigned public field bits.
        @param width Explicit producer byte width.
        @return None.
        """
        encoded.extend(number.to_bytes(width, "big"))

    def tiles(floor):
        """Append exact cardinality and actual canonical world references.

        @param floor Explicit saved cells in producer order.
        @return None.
        """
        value(len(floor), 4)
        for tile in floor:
            value(tile, 4)

    value(0x504153535F563031, 8)
    value(0, 2)
    value(anchor, 4)
    tiles([world_tile(22, 20)])
    tiles([world_tile(x, y) for y in range(19, 22) for x in range(20, 24)])
    value(2, 4)
    # Each complete advancing sweep and original support vector contributes to the same immutable input proof.
    for x in (21, 22):
        value(world_tile(x, 20), 4)
        value(5, 1)
        tiles([world_tile(px, py) for py in range(19, 22) for px in range(x - 1, x + 2)])
        tiles([anchor])
    value(64, 8)
    value(128, 8)
    tiles([anchor])
    value(2, 2)
    value(1, 2)
    value(2, 2)
    fingerprint = 14695981039346656037
    for byte in encoded:
        fingerprint = ((fingerprint ^ byte) * 1099511628211) & ((1 << 64) - 1)
    geology_chunk(records, b"XPAS")["payload"] = struct.pack(">IIIQQQHHIHH", 1, 1, 0, 64, 128, fingerprint, 1, 2, anchor, 1, 2)


def damage(records, kind):
    """Corrupt one bounded pass boundary without modifying unrelated state.

    @param records Actual complete owned pass input.
    @param kind Missing family, mismatched capacity proof or missing typed parent.
    @return None.
    """
    record = geology_chunk(records, b"XPAS")
    if kind == "missing":
        record["remove"] = True
        return
    payload = bytearray(record["payload"])
    if kind == "capacity":
        payload[12:20] = (65).to_bytes(8, "big")
    elif kind == "parent":
        payload[8:12] = (1).to_bytes(4, "big")
    else:
        raise ValueError(kind)
    record["payload"] = bytes(payload)


def main():
    """Run real native/paused/repeated loads and three targeted refusal boundaries.

    @return None; restore supplied startup/config bytes in finally.
    """
    startup, config = ROOT / "build/scripts/game_start.scr", ROOT / "build/grftest.cfg"
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    try:
        config.write_text("# @see r2mlvs2-geology-and-excavation.md, frozen-pass fixture.\n"
                          "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                          "gameplay_profile = multilayer_industry\n"
                          "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n", encoding="utf-8")
        created = run_case("native", ["-G", "1234"], "MLVS_GEOLOGY generator=1", prefix=PREFIX)
        assert read_stream(created)[0] == 374
        active = variant(created, "active", frozen_pass, prefix=PREFIX)
        expected = "MLVS_EXCAVATION_JOB id=0 state=3 cursor=0 partial=77 revision=2"
        loaded = run_case("active", ["-g", active.as_posix()], expected, prefix=PREFIX)
        repeated = run_case("repeat", ["-g", loaded.as_posix()], expected, prefix=PREFIX)
        original = {record["name"]: record["raw"] for record in chunks(read_stream(active)[1])}
        # All actual job/pass/geological authority remains byte-exact across repeated real saves; lookup caches are absent.
        for output in (loaded, repeated):
            restored = {record["name"]: record["raw"] for record in chunks(read_stream(output)[1])}
            for label in (b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"EXJS", b"XJOB", b"XJRT", b"XJTL", b"XPAS"):
                assert original[label] == restored[label], label
        for kind, error in (("missing", "Incomplete excavation pass family"), ("capacity", "Invalid excavation pass proof"), ("parent", "Invalid excavation pass parent")):
            damaged = variant(active, kind, partial(damage, kind=kind), prefix=PREFIX)
            run_case(kind, ["-g", damaged.as_posix()], error, accepted=False, prefix=PREFIX)
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
    (AREA / f"{PREFIX}-pass-save-results.txt").write_text(
        "@see r2mlvs2-geology-and-excavation.md, Group 14.\n"
        "Six actual cases passed: native, active frozen pass, repeated reload, missing family, changed capacity proof, orphan parent.\n"
        "Nine job/pass/geological chunks stayed byte-exact across two reloads. Partial 77/cursor 0/revision 2 retained.\n"
        "Actual paused removed-owner geometry; TBM, wagons and exchange controller remain Plans 3/4/5.\n", encoding="utf-8")


if __name__ == "__main__":
    main()
