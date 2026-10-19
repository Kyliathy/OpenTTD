"""Quote, recall and renew actual conventional freight without replacing useful wagons.

@file surface_entry_renewal_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEE.
Original company and vehicle ledgers own every payment, expense and debt.
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
RESTORE_PAUSE = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-12.py")))["restore_pause"]


def execute(configuration, phase, receipt=None, recover_run=None):
    """@param configuration Actual focused native build. @param phase Pure quote, original recall/arrival, paid renewal or native earned work.
    @param receipt Immediate selected predecessor, or canonical completed initial-capital input.
    @param recover_run Existing successful native invocation to inspect without executing again.
    @return Immutable actual payment, physical ownership and native-process evidence.
    """
    foundation_path = ROOT / "logs/surface-entry-launch-savings-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    if receipt is None:
        entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
        original = entry["output"]
    else:
        predecessor = json.loads(receipt.read_text(encoding="utf-8"))
        assert predecessor["complete"] and predecessor["configuration"] == configuration
        original = predecessor["operation"]["output"]
    source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-renewal-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_renewal\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"
        assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-renewal]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_RENEWAL_PHASE=phase))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and f"SURFACE_RENEWAL_COMPLETE phase={phase}" in terminal, log
    assert identity(source) == original
    before, output = [context.saves / f"surface-renewal-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, final = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == final.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        # An ordinary load can save the actual due-worker bit65. This native
        # observer saves Normal1; verify the original scheduled job and every
        # other DATE field before accepting that sole reverse transition.
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = RESTORE_PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == final[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_RENEWAL_" + prefix + " ")]
            for name, prefix in dict(plan="PLAN", models="MODEL", invoices="INVOICE", recall="RECALL", arrivals="ARRIVAL", wagons="WAGON", heads="HEAD", purchase="PURCHASE",
                                     earned="EARNED", expenses="EXPENSE", delivered="DELIVERED").items()}
    initial, observed = observe(before), observe(output)
    differences = [key.decode("ascii") for key in first if first[key] != final[key]]
    assert reserve_identity(initial) == reserve_identity(observed) and FUEL_IDENTITY(initial) == FUEL_IDENTITY(observed)
    assert initial["plant"] == observed["plant"]
    assert final[b"MAPT"][3 + 2826] >> 4 == 6 and (final[b"MAPO"][3 + 2826] >> 5) & 3 == 1
    if phase == "query":
        assert first == final and initial == observed
        plan, = rows["plan"]; assert plan["model"] == 24 and plan["heads"] == 3 and plan["gross"] == 105468 and plan["reserve"] == 150000
        assert plan["credit"] % 10000 == 0 and plan["cash"] + plan["credit"] - plan["gross"] >= plan["reserve"]
        assert plan["debt"] + plan["credit"] <= plan["ceiling"] == 5000000 and not plan["unsold_value_counted"]
        assert len(rows["models"]) == 3 and all(row["model"] == 24 and row["old_model"] == 26 and row["orders"] == 2 for row in rows["models"])
    elif phase == "recall":
        assert TABLE(before, b"DATE") == TABLE(output, b"DATE") and initial["company"] == observed["company"]
        assert first[b"CAPA"] == final[b"CAPA"] and len(rows["recall"]) == 3
        assert all(row["orders"] == 3 and not row["supplied_pose"] for row in rows["recall"])
    elif phase == "buy":
        assert initial["traffic_halted"] and not observed["traffic_halted"]
        paid, = rows["purchase"]; assert paid["reserve"] == 150000 and paid["ceiling"] == 5000000
        assert sum(row["cost"] for row in rows["invoices"]) == paid["net_cost"]
        assert observed["company"]["cash"] == initial["company"]["cash"] + paid["credit"] - paid["net_cost"]
        assert observed["company"]["debt"] == initial["company"]["debt"] + paid["credit"] <= 5000000
        assert observed["company"]["cash"] >= 150000
        old_vehicles, new_vehicles = TABLE(before, b"VEHS"), TABLE(output, b"VEHS")
        assert len(new_vehicles) == len(old_vehicles) - 3
        assert len(rows["heads"]) == 3 and len(rows["wagons"]) == 32
        # Native single24replaces each front/rear26pair. Every useful original
        # wagon identity and cargo capacity survives unchanged, even when old
        # head pool IDs are normally reused for a different newly bought front.
        renewed_by_name = {row["name"]: row for row in observed["trains"]}
        for old_train in initial["trains"]:
            new_train = renewed_by_name[old_train["name"]]
            old_wagons = [key for key in old_train["chain"] if old_vehicles[key]["train[0].common[0].cargo_cap"] > 0]
            new_wagons = [key for key in new_train["chain"] if new_vehicles[key]["train[0].common[0].cargo_cap"] > 0]
            assert old_wagons == new_wagons
            assert new_vehicles[new_train["head"]]["train[0].common[0].engine_type"] == 24
            for key in old_wagons:
                assert old_vehicles[key]["train[0].common[0].cargo_type"] == new_vehicles[key]["train[0].common[0].cargo_type"]
                assert old_vehicles[key]["train[0].common[0].cargo_cap"] == new_vehicles[key]["train[0].common[0].cargo_cap"]
        assert all(first[key] == final[key] for key in (b"CAPA", b"STNN", b"LPCR", b"LPCC", b"MAPH", b"MAPT"))
        first_date, final_date = TABLE(before, b"DATE")[0], TABLE(output, b"DATE")[0]; rng_fields = {"random_state[0]", "random_state[1]"}
        assert {k: v for k, v in first_date.items() if k not in rng_fields} == {k: v for k, v in final_date.items() if k not in rng_fields}
        state, other = [first_date[f"random_state[{index}]"] for index in (0, 1)]
        # Original BuildRailVehicle issues one Random value for each of the
        # three real single-head bodies; no other DATE field may change.
        for _ in range(3):
            mixed = other ^ 0x1234567F
            state, other = (state + ((mixed >> 7) | (mixed << 25)) + 1) & 0xFFFFFFFF, (((state >> 3) | (state << 29)) - 1) & 0xFFFFFFFF
        assert [final_date[f"random_state[{index}]"] for index in (0, 1)] == [state, other]
    else:
        earned, = rows["earned"]
        assert 0 <= earned["earned"] <= (3000 if phase == "arrive" else 27000)
        if phase == "earn":
            assert earned["earned"] == 27000 and earned["income"] > 0
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == earned["earned"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - observed["company"]["cash"]
        assert initial["company"]["debt"] == observed["company"]["debt"] <= 5000000
    assert observed["company"]["cash"] > 0
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(foundation=identity(foundation_path), receipt=identity(receipt) if receipt else None, source=original,
                before=identity(before), output=identity(output), application=identity(application), terminal=identity(log), rows=rows,
                initial=initial, final=observed, changed_families=differences, source_pause=source_pause, retained_harness=identity(retained),
                native_processes=processes, sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_renewal.cpp"),
                identity(ROOT / "src/tests/surface_entry_work.h"), identity(ROOT / "src/tests/multilayer/acceptance/surface_propellant_ledger.py")])


def main():
    """Run or recover one explicitly selected boundary and retain its exact original output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("query", "recall", "arrive", "buy", "earn"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
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
