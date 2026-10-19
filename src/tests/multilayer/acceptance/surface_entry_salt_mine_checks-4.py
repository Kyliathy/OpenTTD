"""Verify genuine paid Salt mine construction and finite native output without supplied work.

@file surface_entry_salt_mine_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BG.2.
Native map construction bits prove readiness. Actual reserve, mine history,
industry output and original packet owners conserve every extracted Salt unit.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

OWNERS = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))


def observe(path):
    """Read the actual mine's original finite reserve/output/packet/construction owners.

    @param path Complete genuine native save after actual mine funding.
    @return Exactly conserved mine state and independently decoded machinery readiness.
    """
    table = OWNERS["saved_table"]
    mine, = table(path, b"LMNR").values()
    assert mine["industry"] == 14 and mine["deposit"] == 4 and mine["company"] == 0
    industry = table(path, b"INDY")[14]
    deposit = table(path, b"GDEP")[4]
    assert deposit["claimant"] == 0 and deposit["resource"].to_bytes(4, "little") == b"SALT"
    assert industry["founder"] == 0 and industry["produced.count"] == 1
    waiting, cargo = industry["produced[0].waiting"], industry["produced[0].cargo"]
    assert mine["extracted"] + deposit["remaining"] == 24000
    assert mine["extracted"] == waiting + mine["dispatched"]
    packets = table(path, b"CAPA")
    dispatched = {key: row for key, row in packets.items() if row["source_type"] == 0 and row["source_id"] == 14}
    assert sum(row["count"] for row in dispatched.values()) == mine["dispatched"]
    raw = chunks(path)[1]
    tiletypes, flags, owners = raw[b"MAPT"][3:], raw[b"MAPO"][3:], raw[b"MAP2"][3:]
    cells = []
    # All nine real machinery cells must retain this exact native IndustryID.
    # Completion comes from each native bit7, not a copied flag or test timer.
    for y in range(33, 36):
        for x in range(28, 31):
            tile = 4096 + y * 64 + x
            assert tiletypes[tile] >> 4 == 8 and int.from_bytes(owners[tile * 2:tile * 2 + 2], "big") == 14
            cells.append(dict(tile=tile, complete=bool(flags[tile] & 128), stage=flags[tile] & 3, counter=(flags[tile] >> 2) & 3))
    return dict(mine=mine, cargo=cargo, reserve=deposit["remaining"], waiting=waiting, dispatched_packets=dispatched,
                cells=cells, complete=all(row["complete"] for row in cells))


def execute(configuration, source, phase, require_ready):
    """Fund one real mine or earn exactly3000native construction/production ticks.

    @param configuration Independently linked focused native application.
    @param source Exact actual credit or preceding construction checkpoint.
    @param phase Original funding command or ordinary native game-loop interval.
    @param require_ready Require every actual machinery cell to be complete.
    @return Original process membership, financial invoices and conserved finite output.
    """
    context = create_run_context("space", "surface-entry-salt-mine-" + phase)
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    command = "surface_entry_salt_mine" if phase == "fund" else "surface_entry_production"
    (context.runtime_dir / "scripts/game_start.scr").write_text(command + "\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / (phase + ".log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-salt-mine]" if phase == "fund" else "[.surface-entry-production]"],
                            cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_SALT_MINE_PHASE=phase))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal, log
    assert identity(source) == original
    output = context.saves / ("surface-salt-mine-fund-440.sav" if phase == "fund" else "surface-entry-replayed-440.sav")
    first, final = [OWNERS["observe"](path) for path in (source, output)]
    assert first["debt"] == final["debt"] == 4250000
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"]
    assert first["parked_steel"] == final["parked_steel"] == 36 and first["parked_goods"] == final["parked_goods"] == 14
    dates = [OWNERS["saved_table"](path, b"DATE")[0] for path in (source, output)]
    earned = 0 if phase == "fund" else 3000
    assert dates[1]["tick_counter"] - dates[0]["tick_counter"] == earned and dates[1]["id"] == dates[0]["id"]
    native = observe(output)
    a, b = chunks(source)[1], chunks(output)[1]
    for family in (b"GEOS", b"GSPN", b"GVNT", b"XJOB", b"XJRT", b"XJTL"):
        assert a[family] == b[family], family
    if phase == "fund":
        assert "cost=616584" in terminal and first["cash"] - final["cash"] == 616584 and final["cash"] == 272658
        assert not native["complete"] and native["mine"]["extracted"] == native["mine"]["dispatched"] == native["mine"]["work_ticks"] == 0
        assert native["reserve"] == 24000 and native["waiting"] == 0 and not native["dispatched_packets"]
        assert all(row["stage"] == row["counter"] == 0 for row in native["cells"])
        for family in (b"VEHS", b"CAPA", b"STNN", b"DEPT"):
            assert a[family] == b[family], family
        before, after = [OWNERS["saved_table"](path, b"GDEP") for path in (source, output)]
        for key, row in before.items():
            assert after[key] == (dict(row, claimant=0) if key == 4 else row)
        # Actual mine appearance consumes native RNG, but funding cannot earn
        # construction, calendar, economy, extraction or world-identity progress.
        for name, value in dates[0].items():
            if not name.startswith("random_state["):
                assert dates[1][name] == value, name
    if require_ready:
        assert native["complete"]
    return dict(source=original, output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, native=native, earned_ticks=earned, supplied_stock_or_progress=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests" / ("surface_entry_salt_mine.cpp" if phase == "fund" else "surface_entry_production.cpp"))])


def main():
    """Publish one bounded actual funding or earned construction interval."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("fund", "advance"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--require-ready", action="store_true")
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, source, args.phase, args.require_ready)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), complete=result["native"]["complete"], reserve=result["native"]["reserve"],
                          extracted=result["native"]["mine"]["extracted"], waiting=result["native"]["waiting"])), flush=True)


if __name__ == "__main__":
    main()
