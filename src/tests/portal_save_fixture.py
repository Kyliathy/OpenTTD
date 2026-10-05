#!/usr/bin/env python3
"""Actual canonical portal-state reloads and bounded missing/invalid authority refusals.

@file portal_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 2
"""

import hashlib
import struct
import sys
from functools import partial

from geology_save_fixture import chunks, geology_chunk, read_stream, variant
from multilayer_state_fixture import AREA, ROOT, run_case

PREFIX = "r2mlvs3-e"
WORK_FORMAT = ">IBBIIIQ"


def portal_state(records, connected=False):
    """Encode the existing public rail housing constructor and exact saved work row in an owned real-company world.

    @param records Actual version-375 single-player chunks, retaining the real initial company.
    @param connected Supply explicit already-excavated destination foundations for the Connected fixture.
    @return None; this authors load-boundary evidence and does not claim sealed-build or TBM execution behavior.
    """
    companies = geology_chunk(records, b"PLYR")
    assert len(companies["rows"]) >= 2, "single-player did not create its actual initial company"
    companies["edited"] = False
    metadata = geology_chunk(records, b"MLMD")
    row = bytearray(metadata["rows"][1])
    row[31:35] = (int.from_bytes(row[31:35], "big") | 16).to_bytes(4, "big")
    metadata["rows"][1] = bytes(row)
    source, target = 20 * 64 + 20, 4096 + 20 * 64 + 20
    # These exact fields mirror MakeRailLayerPortal, with its actual Surface/Underground targets and reciprocal SE/NW shafts.
    for label, width in ((b"MAPT", 1), (b"MAPO", 1), (b"MAP2", 2), (b"M3LO", 1),
                         (b"M3HI", 1), (b"MAP5", 1), (b"MAPE", 1), (b"MAP7", 1), (b"MAP8", 2)):
        chunk = geology_chunk(records, label)
        payload = bytearray(chunk["payload"])
        for tile in (source, target):
            begin = tile * width
            if label == b"MAPT":
                payload[begin] = (payload[begin] & 15) | (11 << 4)
            elif label == b"MAP5":
                payload[begin] = 33 if tile == source else 3
            else:
                payload[begin:begin + width] = bytes(width)
        chunk["payload"] = bytes(payload)
    geology_chunk(records, b"LPWH")["payload"] = struct.pack(">II", 1, 1)
    work = geology_chunk(records, b"LPWK")
    work["rows"] = work["rows"][:1] + [struct.pack(WORK_FORMAT, source, 2 if connected else 0, 0, 0xFFFFFFFF, 0xFFFFF, 0xFFFF, 1)]
    if connected:
        terrain = geology_chunk(records, b"GEOS")
        payload = bytearray(terrain["payload"])
        # Connected evidence explicitly supplies the two real destination foundations; the Sealed fixture leaves every packed bit unchanged.
        for local in (20 * 64 + 20, 21 * 64 + 20):
            shift, index = (local % 4) * 2, 29 + local // 4
            payload[index] = (payload[index] & ~(3 << shift)) | (2 << shift)
        terrain["payload"] = bytes(payload)


def damage(records, kind):
    """Change one owner/header/topology field without touching other saved authority.

    @param records Complete owned Sealed fixture.
    @param kind Missing table, noncanonical key, unknown phase or broken reciprocal target.
    @return None.
    """
    if kind == "missing":
        geology_chunk(records, b"LPWK")["remove"] = True
        return
    if kind == "topology":
        chunk = geology_chunk(records, b"MAP5")
        payload = bytearray(chunk["payload"])
        payload[4096 + 20 * 64 + 20] |= 1 << 5
        chunk["payload"] = bytes(payload)
        return
    work = geology_chunk(records, b"LPWK")
    row = bytearray(work["rows"][1])
    if kind == "key":
        row[:4] = (4096 + 20 * 64 + 20).to_bytes(4, "big")
    elif kind == "phase":
        row[4] = 3
    else:
        raise ValueError(kind)
    work["rows"][1] = bytes(row)


def main():
    """Exercise actual Sealed/Connected repeated files, old absence and four malformed boundaries.

    @return None; preserve startup/config and supplied predecessor bytes exactly.
    """
    startup, config = ROOT / "build/scripts/game_start.scr", ROOT / "build/grftest.cfg"
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    prior = AREA / "r2mlvs2-z-native-current.sav"
    prior_hash = hashlib.sha256(prior.read_bytes()).hexdigest()
    try:
        config.write_text("# @see r2mlvs3-sealed-portals-and-deep-rail.md, portal save fixture.\n"
                          "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                          "gameplay_profile = multilayer_industry\n"
                          "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n", encoding="utf-8")
        # The dedicated run prepares the current console-subsystem executable; the owned single-player run creates a real company.
        run_case("native-empty", ["-G", "1234"], "MLVS_GEOLOGY generator=1", prefix=PREFIX)
        owned = run_case("native-company", ["-G", "1234"], "MLVS_GEOLOGY generator=1", prefix=PREFIX, single_player=True)
        assert read_stream(owned)[0] == 375
        sealed = variant(owned, "sealed", portal_state, prefix=PREFIX)
        # Actual phases survive repeated full executable saves with exact work/geological bytes; Commissioning actor integration is tested separately by its producer.
        for name, connected in (("sealed", False), ("connected", True)):
            source = sealed if not connected else variant(owned, name, partial(portal_state, connected=True), prefix=PREFIX)
            loaded = run_case(name, ["-g", source.as_posix()], "MLVS_PORTAL_WORK_LOADED pairs=1", prefix=PREFIX)
            repeated = run_case(f"{name}-repeat", ["-g", loaded.as_posix()], "MLVS_PORTAL_WORK_LOADED pairs=1", prefix=PREFIX)
            original = {record["name"]: record["raw"] for record in chunks(read_stream(source)[1])}
            for output in (loaded, repeated):
                restored = {record["name"]: record["raw"] for record in chunks(read_stream(output)[1])}
                for label in (b"LPWH", b"LPWK", b"GEOS", b"GDEP", b"GSPN", b"GVNT"):
                    assert original[label] == restored[label], (name, label)
        run_case("old-native", ["-g", prior.as_posix()], "MLVS_GEOLOGY_LOADED generator=1", prefix=PREFIX)
        for kind, error in (("missing", "Incomplete portal work family"), ("key", "Missing native portal work record"),
                            ("phase", "Invalid portal work fields"), ("topology", "Invalid portal work topology")):
            malformed = variant(sealed, kind, partial(damage, kind=kind), prefix=PREFIX)
            run_case(kind, ["-g", malformed.as_posix()], error, accepted=False, prefix=PREFIX)
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        assert hashlib.sha256(prior.read_bytes()).hexdigest() == prior_hash
    (AREA / f"{PREFIX}-save-results.txt").write_text(
        "@see r2mlvs3-sealed-portals-and-deep-rail.md, Group 2.\n"
        "Eleven actual cases passed: empty/company native, Sealed/Connected plus repeated reloads, retained old Native, four corruptions.\n"
        "LPWH/LPWK and four geology chunks remain byte-exact. Startup/config/predecessor bytes retained.\n"
        "Company 0 was created by the actual nonnetworked start_ai command; no fake actor ID or TBM integration is claimed.\n", encoding="utf-8")


def resume_refusals():
    """Complete only targeted remaining refusals against the retained owned Sealed input.

    @return None; preserve existing input/startup/config bytes and earlier successful evidence.
    """
    startup, config = ROOT / "build/scripts/game_start.scr", ROOT / "build/grftest.cfg"
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    sealed = AREA / f"{PREFIX}-sealed-input.sav"
    original_hash = hashlib.sha256(sealed.read_bytes()).hexdigest()
    try:
        # The earlier run already proved creation and phase reloads; only these unresolved corruption expectations need another executable check.
        for kind, error in (("key", "Missing native portal work record"), ("phase", "Invalid portal work fields"),
                            ("topology", "Invalid portal work topology")):
            malformed = variant(sealed, kind, partial(damage, kind=kind), prefix=PREFIX)
            run_case(kind, ["-g", malformed.as_posix()], error, accepted=False, prefix=PREFIX)
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        assert hashlib.sha256(sealed.read_bytes()).hexdigest() == original_hash
    (AREA / f"{PREFIX}-save-results.txt").write_text(
        "@see r2mlvs3-sealed-portals-and-deep-rail.md, Group 2.\n"
        "Eleven actual cases passed across the creation/reload run and its focused refusal continuation.\n"
        "Sealed/Connected and repeated reloads retain exact LPWH/LPWK and GEOS/GDEP/GSPN/GVNT bytes.\n"
        "Native real company came from the nonnetworked start_ai command; actual commissioning movement remains Plan 4 work.\n"
        "Noncanonical key correctly refuses at native-family completeness; unknown phase and broken reciprocal topology also refuse.\n", encoding="utf-8")


if __name__ == "__main__":
    if "--resume-refusals" in sys.argv:
        resume_refusals()
    else:
        main()
