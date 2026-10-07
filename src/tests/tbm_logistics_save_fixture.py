#!/usr/bin/env python3
"""Inspect actual logistics save tables without editing their authoritative rows.

@file tbm_logistics_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 13A/13C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 16 actual portal-only reconstruction
"""

import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess

from geology_generation_fixture import Reader, read_stream
from geology_save_fixture import chunks
from multilayer_state_fixture import log_path

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "src/tests/fixtures/haulage"
WIDTHS = {1: 1, 2: 1, 3: 2, 4: 2, 5: 4, 6: 4, 7: 8, 8: 8, 9: 2}
STATES = ("stopped", "await-exchange", "dispose-origin", "return-empties", "spanning")
STALL_STATES = ("lost-origin", "saturated-receiver", "blocked-line", "wrong-wagons", "no-empties", "occupied-bay", "crashed")


def table_header(reader):
    """Decode the writer's named fields and recursively appended struct headers.

    @param reader Bounded actual table header reader.
    @return Ordered field descriptors, including the actual nested schema.
    """
    fields = []
    # Every nested schema follows the complete parent header, as in SlTableHeader;
    # interpreting nesting inline would assign unrelated bytes to vehicle fields.
    while True:
        kind = reader.byte()
        if kind == 0:
            break
        name = reader.take(reader.gamma()).decode("utf-8")
        fields.append([name, kind, None])
    for field in fields:
        if field[1] & 15 == 11:
            field[2] = table_header(reader)
    return fields


def table_record(reader, fields, prefix=""):
    """Read one real table object using its saved schema and field lengths.

    @param reader Bounded actual row reader.
    @param fields Actual parsed descriptors; no private gameplay definitions.
    @param prefix Parent field path for precise difference reporting.
    @return Named scalar/array values, including all nested object contents.
    """
    values = {}
    # Cargo references, orders and physical state must be decoded together;
    # skipping an unknown byte would invalidate every later custody comparison.
    for name, kind, nested in fields:
        key = prefix + name
        primitive = kind & 15
        if primitive == 11:
            count = reader.gamma()
            values[key + ".count"] = count
            for index in range(count):
                values.update(table_record(reader, nested, f"{key}[{index}]."))
        elif primitive == 10:
            values[key] = reader.take(reader.gamma()).hex()
        else:
            count = reader.gamma() if kind & 16 else 1
            width = WIDTHS[primitive]
            signed = primitive in (1, 3, 5, 7)
            items = [int.from_bytes(reader.take(width), "big", signed=signed) for _ in range(count)]
            values[key] = items if kind & 16 else items[0]
    return values


def saved_table(path, name):
    """Decode an actual current-format table, retaining its sparse object identities.

    @param path Real command-driven source or game-loader output.
    @param name Four-byte owner chunk label.
    @return Object identity to fully decoded fields, without changing any input.
    """
    version, stream = read_stream(path)
    assert version >= 375, "named table inspection requires the delivered native portal table format"
    chunk = next(chunk for chunk in chunks(stream) if chunk["name"] == name)
    assert chunk["kind"] in (3, 4), "owner must be a current table"
    header_reader = Reader(chunk["rows"][0])
    fields = table_header(header_reader)
    assert header_reader.pos == len(header_reader.data), "table header not fully decoded"
    result = {}
    # Sparse tables carry their real pool IDs in each row; dense tables use row order.
    for index, row in enumerate(chunk["rows"][1:]):
        reader = Reader(row)
        identity = reader.gamma() if chunk["kind"] == 4 else index
        result[identity] = table_record(reader, fields)
        assert reader.pos == len(row), (name, identity, "row not fully decoded")
    return result


def owned_chunks(path):
    """Read complete vehicle, packet, project, receipt and company custody descriptors.

    @param path Actual normal-game output to compare at an atomic exchange boundary.
    @return Exact authoritative owner bytes; unknown unrelated chunks remain untouched.
    """
    version, stream = read_stream(path)
    names = {b"EIDS", b"VEHS", b"ORDL", b"CAPA", b"TBMH", b"TBMP", b"TBMS",
             b"TBXH", b"TBXA", b"TBXS", b"TBXW", b"INDY", b"STNN", b"PLYR",
             b"EXJS", b"XJOB", b"XJRT", b"XJTL", b"XPAS", b"GEOS", b"GDEP",
             b"GSPN", b"GVNT", b"SPRH", b"SPRR", b"LPWH", b"LPWK", b"DATE",
             b"ECMY", b"SRND", b"MAPS", b"MAPT", b"MAPH", b"MAPO", b"MAP2", b"M3LO", b"M3HI", b"MAP5", b"MAPE", b"MAP7", b"MAP8"}
    owners = {chunk["name"]: chunk["raw"] for chunk in chunks(stream) if chunk["name"] in names}
    assert owners.keys() == names, "real loaded logistics owner family incomplete"
    return version, owners


def verify_initial_owners(source, loaded):
    """Verify every initial custody/invoice owner before any resume commands.

    @param source Immutable physically produced source snapshot.
    @param loaded Untouched output written immediately after real application loading.
    @return None; changed packets, work, money or physical state fail.
    """
    version, original = owned_chunks(source)
    loaded_version, restored = owned_chunks(loaded)
    assert version == loaded_version, "unexpected fixture format migration"
    # The reduced scenario starts with an unset manager face. Normal application loading assigns its missing
    # presentation style once, consuming its usual RNG draws. All custody, work and invoice fields must still
    # reconstruct exactly; this narrowly declared face initialization does not permit arbitrary money/time changes.
    for name in original.keys() - {b"DATE", b"PLYR"}:
        assert original[name] == restored[name], (name, "initial owner changed on load")
    companies = saved_table(source, b"PLYR")
    restored_companies = saved_table(loaded, b"PLYR")
    assert companies.keys() == restored_companies.keys(), "company identities changed"
    missing_face = False
    for identity, fields in companies.items():
        unset = fields["face_style"] == "" and fields["face"] == 0
        missing_face |= unset
        for name, value in fields.items():
            # Only an explicitly unset scenario face can receive the normal native presentation defaults.
            if unset and name in {"face", "face_style"}:
                continue
            assert value == restored_companies[identity][name], (identity, name, "initial company field changed")
    clocks = saved_table(source, b"DATE")
    restored_clocks = saved_table(loaded, b"DATE")
    assert clocks.keys() == restored_clocks.keys(), "clock identities changed"
    for identity, fields in clocks.items():
        for name, value in fields.items():
            # Native face initialization is the sole allowed initial RNG adjustment; saved game clocks,
            # tick counters and all unrelated date fields must survive the same reconstruction unchanged.
            if missing_face and name in {"random_state[0]", "random_state[1]"}:
                continue
            assert value == restored_clocks[identity][name], (identity, name, "initial clock field changed")


def resume_case(state, evidence_prefix, input_prefix, repair_tile=None, repair_track=1):
    """Compare the next legal native transition across one and two full application loads.

    @param state Physically reached retained source snapshot name.
    @param evidence_prefix Fresh owning-plan evidence prefix for the two process logs.
    @param input_prefix Retained native source basename, allowing improved scenarios without editing earlier inputs.
    @param repair_tile Optional actual removed Surface tile reported by the winding-depot producer; recovery uses an ordinary rail command.
    @param repair_track Actual removed track enum reported by that producer, supplied to the same owning build command.
    @return None; missing physical progress, changed packets/costs or input edits fail.
    """
    startup = ROOT / "build/scripts/game_start.scr"
    original_startup = startup.read_bytes()
    assert Path(input_prefix).name == input_prefix, "input must be a repository-local fixture basename"
    source = FIXTURES / f"{input_prefix}-{state}.sav"
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    outputs = [FIXTURES / f"{input_prefix}-{state}-first", FIXTURES / f"{input_prefix}-{state}-second"]
    elapsed = []
    try:
        startup.write_text("# @see r2mlvs5-spoil-wagons-and-haulage.md, Group 13A actual loader continuation.\n"
                           "logistics_probe\n", encoding="utf-8")
        # Each isolated test initializes the actual application, loads native GRFs,
        # starts only stopped trains by command and observes normal GameLoop frames.
        for index, output in enumerate(outputs):
            loaded_source = source if index == 0 else Path(str(outputs[0]) + "-loaded.sav")
            environment = dict(os.environ, OTTD_TBM_LOGISTICS_SOURCE=str(loaded_source), OTTD_TBM_LOGISTICS_OUTPUT=str(output))
            if repair_tile is not None:
                assert state == "blocked-line", "declared rail repair belongs only to an observed blocked route"
                environment.update(OTTD_TBM_LOGISTICS_REPAIR_TILE=str(repair_tile), OTTD_TBM_LOGISTICS_REPAIR_TRACK=str(repair_track))
            log = log_path(f"{evidence_prefix}-{state}-{index + 1}.log")
            with log.open("wb") as sink:
                subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.tbm-logistics-runtime]", "--durations", "yes"],
                               cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW, check=True, timeout=110)
            evidence = log.read_text(encoding="utf-8", errors="replace")
            # A crashed hauler cannot exchange or be repaired. Its normal delayed deletion is the next legal
            # lifecycle transition; exact destruction receipts replace an imaginary exchange count.
            pattern = (r"LOGISTICS_CRASH_CLEANUP ticks=(\d+) destroyed_earth=(\d+) destroyed_rock=(\d+)" if state == "crashed"
                       else r"LOGISTICS_NEXT_EXCHANGE ticks=(\d+) exchanges=(\d+) phase=(\d+) wait=(\d+)")
            match = re.search(pattern, evidence)
            assert match, "normal loaded game did not reach its next legal native transition"
            elapsed.append(match.groups())
        assert elapsed[0] == elapsed[1], "next exchange timing/cycle changed after reloading"
        suffix = "-crash-cleanup.sav" if state == "crashed" else "-next-exchange.sav"
        first = Path(str(outputs[0]) + suffix)
        second = Path(str(outputs[1]) + suffix)
        assert owned_chunks(first) == owned_chunks(second), "loaded next-exchange packet/controller/cost ledger changed"
        # Exact field decoding also proves the first loader reconstruction retains
        # the writer's physical state and cargo references, including acceleration.
        assert saved_table(source, b"VEHS") == saved_table(Path(str(outputs[0]) + "-loaded.sav"), b"VEHS"), "native vehicle physics changed on first load"
        verify_initial_owners(source, Path(str(outputs[0]) + "-loaded.sav"))
        print(f"{state}: real next legal transition identical after {elapsed[0][0]} native frames", flush=True)
    finally:
        startup.write_bytes(original_startup)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "immutable source changed"


def visibility_case(input_prefix, evidence_prefix, output_prefix=None):
    """Compare complete real source/depot loops with actual visible, hidden and untouched viewport policies.

    @param input_prefix Immutable physically produced stopped source or recurring-depot basename.
    @param evidence_prefix Fresh owning-plan prefix for three isolated actual application logs.
    @param output_prefix Optional fresh output basename for preserving previous native outputs after observer improvements.
    @return None; changed native completion timing, physical custody or full owner bytes fail.
    """
    assert Path(input_prefix).name == input_prefix, "visibility input must be a repository-local basename"
    output_prefix = output_prefix or input_prefix + "-view"
    assert Path(output_prefix).name == output_prefix, "visibility outputs must use a repository-local basename"
    source = FIXTURES / f"{input_prefix}-stopped.sav"
    original_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    startup = ROOT / "build/scripts/game_start.scr"
    original_startup = startup.read_bytes()
    results = []
    try:
        startup.write_text("# @see r2mlvs4-tunnel-boring-machines.md, Group 19 actual visibility pairs.\nlogistics_probe\n", encoding="utf-8")
        # Each fresh application reconstructs the exact same complete native world. Only normal client camera/draw
        # calls differ; native commands, full GameLoop work/vehicle/disposal and accounting remain identical.
        for presentation in ("headless", "visible", "hidden"):
            output = FIXTURES / f"{output_prefix}-{presentation}"
            environment = dict(os.environ, OTTD_TBM_LOGISTICS_SOURCE=str(source), OTTD_TBM_LOGISTICS_OUTPUT=str(output),
                               OTTD_TBM_LOGISTICS_COMPLETE_LOOP="1", OTTD_TBM_LOGISTICS_VIEW=presentation)
            log = log_path(f"{evidence_prefix}-{presentation}.log")
            with log.open("wb") as sink:
                subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.tbm-logistics-runtime]", "--durations", "yes"],
                               cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW, check=True, timeout=110)
            evidence = log.read_text(encoding="utf-8", errors="replace")
            match = re.search(r"LOGISTICS_COMPLETE_LOOP ticks=(\d+) exchanges=(\d+) presentation=" + presentation, evidence)
            assert match, "actual application did not complete native work and final physical Surface disposal"
            verify_initial_owners(source, Path(str(output) + "-loaded.sav"))
            results.append((match.groups(), owned_chunks(Path(str(output) + "-complete-loop.sav"))))
            print(f"{input_prefix} {presentation}: complete real loop after {match[1]} frames / {match[2]} exchanges", flush=True)
        assert results[0] == results[1] == results[2], "actual viewport presentation changed native timing/custody/owner state"
        print("All 40 native owners, including every map field/height/dimension chunk, match across headless, visible and hidden execution.", flush=True)
    finally:
        startup.write_bytes(original_startup)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == original_hash, "immutable visibility input changed"


def portal_case(input_name, output_prefix, evidence_prefix, expected_state):
    """Round-trip an immutable Sealed/Connected or genuine legacy portal through actual application loading.

    @param input_name Existing basename inside the portal fixture directory; earlier supplied names stay unchanged.
    @param output_prefix Fresh non-planning basename for current loaded outputs; no previous evidence is overwritten.
    @param evidence_prefix Fresh owning-plan log prefix for both isolated actual processes.
    @param expected_state Sealed, Connected or explicit legacy compatibility policy to verify after native acceptance.
    @return None; changed pair metadata, opening new map ground or a lost ordinary legacy edge fails.
    """
    directory = ROOT / "src/tests/fixtures/portals"
    assert Path(input_name).name == input_name, "portal input must be an existing fixture basename"
    assert output_prefix and Path(output_prefix).name == output_prefix and not output_prefix.startswith("r2"), "fresh portal outputs must use a non-planning basename"
    source = directory / input_name
    original_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    startup = ROOT / "build/scripts/game_start.scr"
    original_startup = startup.read_bytes()
    fields = (b"MAPT", b"MAPH", b"MAPO", b"MAP2", b"M3LO", b"M3HI", b"MAP5", b"MAPE", b"MAP7", b"MAP8")
    widths = {b"MAP2": 2, b"MAP8": 2}
    version, original_stream = read_stream(source)
    original = {chunk["name"]: chunk for chunk in chunks(original_stream)}
    endpoints = [index for index, value in enumerate(original[b"MAPT"]["payload"]) if value >> 4 == 11]
    assert endpoints, "portal boundary needs actual reciprocal endpoint tiles"
    loaded = []
    try:
        startup.write_text("# @see r2mlvs3-sealed-portals-and-deep-rail.md, Group 16 actual portal load.\nlogistics_probe\n", encoding="utf-8")
        # Native acceptance/save is the legal empty/legacy checkpoint. No missing hauler is invented to make these
        # real older worlds imitate a commissioning project; all original endpoint/ground bits must survive migration.
        for index in range(2):
            output = directory / f"{output_prefix}-{index + 1}"
            input_path = source if index == 0 else loaded[0]
            environment = dict(os.environ, OTTD_TBM_LOGISTICS_SOURCE=str(input_path), OTTD_TBM_LOGISTICS_OUTPUT=str(output), OTTD_TBM_PORTAL_LOAD_ONLY="1")
            log = log_path(f"{evidence_prefix}-{index + 1}.log")
            with log.open("wb") as sink:
                subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.tbm-logistics-runtime]", "--durations", "yes"],
                               cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW, check=True, timeout=110)
            evidence = log.read_text(encoding="utf-8", errors="replace")
            assert "PORTAL_LOAD_ONLY pairs=" in evidence, "native portal acceptance checkpoint absent"
            result = Path(str(output) + "-loaded.sav")
            restored = {chunk["name"]: chunk for chunk in chunks(read_stream(result)[1])}
            # Compare complete geometry/field bytes at every actual original endpoint. Height and all extended
            # fields are included; newer unrelated schemas cannot excuse a changed shaft, door, reservation or owner.
            for name in fields:
                width = widths.get(name, 1)
                for tile in endpoints:
                    start = tile * width
                    assert original[name]["payload"][start:start + width] == restored[name]["payload"][start:start + width], (name, tile, "portal endpoint changed")
            if expected_state == "legacy":
                assert version == 370, "legacy input must be genuine version 370"
                assert "PORTAL_LOAD_ONLY pairs=0" in evidence, "legacy migration introduced Native commissioning state"
                assert "PORTAL_NATIVE_EDGES rail=5/5 road=0/0 elevator=4/4 invalid=0" in evidence, "actual ordinary legacy edges became unusable"
                assert "profile=0" in evidence and "legacy=true" in evidence, "loaded legacy policy changed"
                assert "MLVS_GEOLOGY_LOADED generator=0 deposits=0 packed_bytes=0 vents=0" in evidence, "legacy migration generated Native rock/reserves"
                for name in (b"MAPS", b"MAPT", b"MAPH"):
                    assert original[name]["raw"] == restored[name]["raw"], "legacy map dimensions/type/height changed"
            else:
                expected = 0 if expected_state == "sealed" else 2
                before = saved_table(source, b"LPWK")
                after = saved_table(result, b"LPWK")
                assert before == after, "native work owner changed on empty portal load"
                assert all(row["state"] == expected for row in after.values()), "wrong actual saved portal state"
                assert saved_table(source, b"MLMD") == saved_table(result, b"MLMD"), "native saved profile/date policy changed"
                # The calendar technology date must reconstruct exactly; manager-face presentation setup
                # cannot move an empty portal across its actual Deep availability gate.
                dates = saved_table(source, b"DATE")
                restored_dates = saved_table(result, b"DATE")
                for identity, row in dates.items():
                    for field in ("date", "date_fract"):
                        assert row[field] == restored_dates[identity][field], "native technology date changed"
            loaded.append(result)
        print(f"{input_name}: actual {expected_state} load/reload preserves {len(endpoints)} complete endpoint field sets and policy", flush=True)
    finally:
        startup.write_bytes(original_startup)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == original_hash, "immutable portal input changed"


def main():
    """Report precise actual vehicle-field differences across the five retained loads.

    @return None; malformed descriptors or changed object identities fail.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--resume", choices=STATES + STALL_STATES, help="run one focused actual loader/next-exchange comparison")
    parser.add_argument("--evidence-prefix", default="r2mlvs5-l", help="fresh chronological owning-plan log prefix")
    parser.add_argument("--input-prefix", default="source-native-deep", help="immutable native source save basename")
    parser.add_argument("--repair-tile", type=int, help="actual removed Surface tile reported by the recurring-depot producer")
    parser.add_argument("--repair-track", type=int, choices=range(6), default=1, help="actual removed native track enum")
    parser.add_argument("--visibility", action="store_true", help="compare three actual complete loops with headless/visible/hidden viewport policies")
    parser.add_argument("--output-prefix", help="fresh repository-local visibility outputs; never overwrite retained evidence")
    parser.add_argument("--portal-input", help="existing immutable portal fixture basename for actual portal-only loading")
    parser.add_argument("--portal-state", choices=("sealed", "connected", "legacy"), help="expected native work or legacy policy")
    arguments = parser.parse_args()
    if arguments.portal_input:
        assert arguments.portal_state and not arguments.visibility and arguments.resume is None, "one explicit focused portal family is required"
        portal_case(arguments.portal_input, arguments.output_prefix, arguments.evidence_prefix, arguments.portal_state)
        return
    if arguments.visibility:
        assert arguments.resume is None, "one focused fixture family per run"
        visibility_case(arguments.input_prefix, arguments.evidence_prefix, arguments.output_prefix)
        return
    if arguments.resume:
        resume_case(arguments.resume, arguments.evidence_prefix, arguments.input_prefix, arguments.repair_tile, arguments.repair_track)
        return
    for state in STATES:
        source = saved_table(FIXTURES / f"source-deep-{state}.sav", b"VEHS")
        loaded = saved_table(FIXTURES / f"source-deep-{state}-roundtrip.sav", b"VEHS")
        assert source.keys() == loaded.keys(), "loaded vehicle identities changed"
        differences = [(identity, key, value, loaded[identity][key])
                       for identity, fields in source.items() for key, value in fields.items()
                       if value != loaded[identity][key]]
        print(state, differences, flush=True)


if __name__ == "__main__":
    main()
