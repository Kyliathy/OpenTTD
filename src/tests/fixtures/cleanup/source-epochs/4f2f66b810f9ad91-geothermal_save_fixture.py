#!/usr/bin/env python3
"""Actual natural-vent capacity/identity persistence and required-family refusal.

@file geothermal_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 6
"""

import hashlib
from functools import partial

from geology_save_fixture import chunks, geology_chunk, variant, read_stream
from multilayer_state_fixture import AREA, ROOT, run_case, save_path


def bad_vent(records, remove=False):
    """Introduce one exact owned thermal-family failure, preserving every mineral/terrain byte.

    @param records Actual new native 372 chunk dictionaries.
    @param remove Remove the required family instead of exceeding the physical capacity bound.
    @return None; the mutated file must refuse before gameplay startup/save.
    """
    vent = geology_chunk(records, b"GVNT")
    if remove:
        vent["remove"] = True
        return
    if len(vent["rows"]) != 3 or len(vent["rows"][2]) != 30:
        raise ValueError("unexpected natural-source wire shape")
    row = bytearray(vent["rows"][2])
    row[22:30] = (1000000001).to_bytes(8, "big")
    vent["rows"][2] = bytes(row)


def main():
    """Check natural-field save/reload and two refusals without rerunning earlier geometric regressions.

    @return None; preserve original startup/config and the retained geology-only predecessor.
    """
    startup = ROOT / "build/scripts/game_start.scr"
    config = ROOT / "build/grftest.cfg"
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    older = save_path("r2mlvs2-g-native.sav")
    older_hash = hashlib.sha256(older.read_bytes()).hexdigest()
    try:
        config.write_text("# @see r2mlvs2-geology-and-excavation.md, natural-field fixture.\n"
                          "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                          "gameplay_profile = multilayer_industry\n"
                          "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n", encoding="utf-8")
        created = run_case("native", ["-G", "1234"], "packed_bytes=2048 vents=1", prefix="r2mlvs2-i")
        loaded = run_case("reload", ["-g", created.as_posix()], "packed_bytes=2048 vents=1", prefix="r2mlvs2-i")
        before = {record["name"]: record for record in chunks(read_stream(created)[1])}
        after = {record["name"]: record for record in chunks(read_stream(loaded)[1])}
        # Every authoritative mineral/terrain/thermal field survives; natural capacity is not multiplied by loading or overlap.
        for label in (b"GEOS", b"GDEP", b"GSPN", b"GVNT"):
            assert before[label]["raw"] == after[label]["raw"], (label, "natural/geological authority changed")
        run_case("older-geology", ["-g", older.as_posix()], "packed_bytes=2048 vents=0", prefix="r2mlvs2-i")
        invalid = variant(created, "invalid-capacity", bad_vent, prefix="r2mlvs2-i")
        missing = variant(created, "missing-field", partial(bad_vent, remove=True), prefix="r2mlvs2-i")
        run_case("invalid-capacity", ["-g", invalid.as_posix()], "Invalid geothermal source fields", accepted=False, prefix="r2mlvs2-i")
        run_case("missing-field", ["-g", missing.as_posix()], "Missing required geothermal fields", accepted=False, prefix="r2mlvs2-i")
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        assert hashlib.sha256(older.read_bytes()).hexdigest() == older_hash, "geology-only predecessor changed"
    (AREA / "r2mlvs2-i-vent-results.txt").write_text(
        "R2MLVS2 Group 6: natural-field new/reload, geology-only predecessor and capacity/missing-family refusals passed.\n"
        "Minerals, terrain and thermal ID/capacity remained exact; discovery produced no plant, freight or power grant.\n"
        "Original configuration/startup/predecessor bytes preserved. Runtime heat allocation remains Plan 17.\n", encoding="utf-8")


if __name__ == "__main__":
    main()
