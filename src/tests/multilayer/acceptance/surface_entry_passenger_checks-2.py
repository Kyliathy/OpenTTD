"""Drive genuine civic and passenger boundaries without replaying paid actions.

@file surface_entry_passenger_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEB.
Original commands and company books own every payment and town-rating change.
Read-only recovery can finalize a successful native save without repurchasing it.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_propellant_ledger import TABLE, observe, reserve_identity

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]
FUEL_IDENTITY = runpy.run_path(str(Path(__file__).with_name("surface_entry_propellant_checks-8.py")))["fuel_identity"]


def execute(configuration, phase, receipt=None, recover_run=None):
    """@param configuration Actual focused native build. @param phase Pure civic/service quote, original civic purchase or bounded freight work.
    @param receipt Immediate selected predecessor, or canonical completed physical fuel input.
    @param recover_run Existing successful native invocation to inspect without executing again.
    @return Immutable actual payment, physical ownership and native-process evidence.
    """
    foundation_path = ROOT / "logs/surface-entry-propellant-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    if receipt is None:
        entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
        original = entry["output"]
    else:
        predecessor = json.loads(receipt.read_text(encoding="utf-8"))
        assert predecessor["complete"] and predecessor["configuration"] == configuration
        original = predecessor["operation"]["output"]
    source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-passenger-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_launch\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"
        assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-launch]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_LAUNCH_PHASE=phase))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and f"SURFACE_LAUNCH_COMPLETE phase={phase}" in terminal, log
    assert identity(source) == original
    before, output = [context.saves / f"surface-launch-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, final = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == final.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key != b"GLOG")
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == final[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_PASSENGER_" + prefix + " ")]
            for name, prefix in dict(layout="LAYOUT", tariff="TARIFF", capital="CAPITAL", civic="CIVIC", civic_paid="CIVIC_PAID",
                                     earned="EARNED", expenses="EXPENSE", delivered="DELIVERED").items()}
    initial, observed = observe(before), observe(output)
    differences = [key.decode("ascii") for key in first if first[key] != final[key]]
    if phase == "finance":
        earned, = rows["earned"]
        assert 0 < earned["earned"] <= 27000
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == earned["earned"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - observed["company"]["cash"]
        assert initial["company"]["debt"] == observed["company"]["debt"] == 5000000
        assert reserve_identity(initial) == reserve_identity(observed) and FUEL_IDENTITY(initial) == FUEL_IDENTITY(observed)
        assert initial["plant"] == observed["plant"] and observed["company"]["cash"] > 0
    elif phase == "civic":
        assert TABLE(before, b"DATE") == TABLE(output, b"DATE")
        civic, = rows["civic"]; paid, = rows["civic_paid"]
        assert civic["executed"] and civic["trees"] > 0 and civic["cost"] == paid["total"]
        assert initial["company"]["cash"] - observed["company"]["cash"] == paid["total"]
        assert initial["company"]["debt"] == observed["company"]["debt"] == 5000000
        assert {k: v for k, v in initial.items() if k != "company"} == {k: v for k, v in observed.items() if k != "company"}
        assert TABLE(before, b"CITY")[0]["ratings"][0] == civic["rating"]
        assert TABLE(output, b"CITY")[0]["ratings"][0] == paid["rating"] == civic["rating"] + 7 * civic["trees"]
        first_types, last_types = first[b"MAPT"][3:], final[b"MAPT"][3:]
        planted = [index for index, (old, new) in enumerate(zip(first_types, last_types)) if old != new]
        assert len(planted) == civic["trees"] and all(first_types[index] >> 4 == 0 and last_types[index] >> 4 == 4 for index in planted)
        assert set(differences) <= {"MAPT", "MAP2", "M3LO", "M3HI", "MAP5", "MAPE", "MAPO", "PLYR", "CITY"}
    else:
        assert first == final and initial == observed
        if phase == "civic-query":
            civic, = rows["civic"]; assert not civic["executed"] and civic["trees"] > 0
        else:
            capital, = rows["capital"]; layout, = rows["layout"]
            assert capital["affordable"] and capital["reserve"] == 20000
            assert capital["total"] == layout["infrastructure"] + capital["buses"] * capital["bus_cost"]
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    return dict(foundation=identity(foundation_path), receipt=identity(receipt) if receipt else None, source=original,
                before=identity(before), output=identity(output), application=identity(application), terminal=identity(log), rows=rows,
                initial=initial, final=observed, changed_families=differences, retained_harness=identity(retained),
                native_processes=processes, sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_launch.cpp"),
                identity(ROOT / "src/tests/surface_passenger_layout.h"), identity(ROOT / "src/tests/multilayer/acceptance/surface_propellant_ledger.py")])


def main():
    """Run or recover one explicitly selected boundary and retain its exact original output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("civic-query", "civic", "passenger-query", "finance"), required=True)
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve() if args.receipt else None, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase,
                                     operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
