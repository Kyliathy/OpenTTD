"""Verify authentic mine site queries, actual settled retirement and whole retained-cutter room quotes.

@file surface_entry_mine_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BF.1.
Missing floor remains a native refusal. Public retirement releases only actual
settled custody; original hulls, floor, reserves, cash and useful stock survive.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

OWNERS = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))


def execute(configuration, source, phase):
    """Run one exact original-world mine preparation phase with no earned time.

    @param configuration Independently linked focused full-loader application.
    @param source Exact completed first-cut checkpoint or its actual retired output.
    @param phase Pure initial characterization, actual native retirement or pure room plan.
    @return Exact native artifacts, original owner changes and admitted room geometry.
    """
    context = create_run_context("space", "surface-entry-mine-" + phase)
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_mine\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / (phase + ".log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-mine]"], cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_MINE_PHASE=phase))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and f"SURFACE_MINE_COMPLETE phase={phase}" in terminal, log
    assert identity(source) == original
    before = context.saves / f"surface-mine-before-{phase}-440.sav"
    output = context.saves / f"surface-mine-{phase}-440.sav"
    first, final = [OWNERS["observe"](path) for path in (before, output)]
    assert first == final and final["parked_steel"] == 36 and final["parked_goods"] == 14 and final["debt"] == 3500000
    a, b = chunks(before)[1], chunks(output)[1]
    assert a.keys() == b.keys()
    changed = sorted(key.decode() for key in a if a[key] != b[key])
    if phase != "release":
        assert not changed
    else:
        assert "SURFACE_MINE_RETIRED cutter=7" in terminal
        assert len(OWNERS["saved_table"](before, b"TBMP")) == 1 and not OWNERS["saved_table"](output, b"TBMP")
        assert not OWNERS["saved_table"](output, b"TBXA") and not OWNERS["saved_table"](output, b"TBXS")
        for family in (b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"CAPA", b"INDY", b"XJOB", b"XJRT", b"XJTL", b"DATE"):
            assert a[family] == b[family], family
        company = [OWNERS["saved_table"](path, b"PLYR")[0] for path in (before, output)]
        # The normal command kernel remembers the real source action tile.
        # This presentation coordinate cannot authorize any changed account,
        # loan, company setting, inventory or financial history field.
        for name, value in company[0].items():
            assert company[1][name] == (1949 if name == "last_build_coordinate" else value), name
        vehicles = [OWNERS["saved_table"](path, b"VEHS") for path in (before, output)]
        assert vehicles[0].keys() == vehicles[1].keys()
        # Actual retirement may stop the withdrawn front. It cannot move any
        # part, recouple wagons, change finance or delete physical inventory.
        for vehicle, fields in vehicles[0].items():
            for name, value in fields.items():
                expected = value | 2 if vehicle == 7 and name == "train[0].common[0].vehstatus" else value
                assert vehicles[1][vehicle][name] == expected, (vehicle, name)
    quotes = []
    for values in re.findall(r"SURFACE_MINE_ROOM_QUOTE mine=(\d+) first=(\d+) last=(\d+) route=(\d+) cells=(\d+) earth=(\d+) rock=(\d+) purchase=(\d+) fingerprint=(\d+)", terminal):
        quotes.append(dict(zip(("mine", "first", "last", "route", "cells", "earth", "rock", "purchase", "fingerprint"), map(int, values), strict=True)))
    if phase == "plan":
        assert quotes and all(row["purchase"] == 0 for row in quotes)
    return dict(source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, quotes=quotes, changed_native_families=changed, simulation_ticks=0, supplied_floor_or_stock=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_mine.cpp")])


def main():
    """Select one original actual mine preparation step and publish its fresh membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("characterize", "release", "plan"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, source, args.phase)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), quotes=result["quotes"], changed=result["changed_native_families"])), flush=True)


if __name__ == "__main__":
    main()
