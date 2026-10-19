"""Drive actual original material shipments and paid native Surface handling.

@file surface_entry_gateway_supply_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQL.
The separate gateway observer admits original128units across their actual
owners while earlier parked-reserve observers remain unchanged and strict.
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
from surface_gateway_material_ledger import TABLE, observe, SHIPMENTS, REQUIRED

DIRECTORY = Path(__file__).resolve().parent
FIELDS = runpy.run_path(str(DIRECTORY / "surface_entry_chemical_checks-2.py"))["fields"]
REVISION = runpy.run_path(str(DIRECTORY / "surface_entry_checks-7.py"))["known_revision"]
PAUSE = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))["restore_pause"]
ROUTES = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))["all_orders"]
POLICY = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))["route_policy"]
PHASES = ("shipment-query", "dispatch", "ship-work", "handling-query", "handling", "handle-work", "service")


def freight_credit(snapshot):
    """@param snapshot Actual sole-owner material snapshot. @return Per-label real feeder credit, excluding quantity history and other commercial stock."""
    return {label: sum(packet["feeder_share"] for packet in snapshot["cohort"] if packet["label"] == label) for label in REQUIRED}


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual focused build. @param phase One explicit pure, paid/control or real work boundary.
    @param receipt Exact selected original predecessor. @param recover_run Successful existing native run to inspect without repeating commands.
    @return Native payment/work, immutable source/process identities and complete physical material custody.
    """
    assert phase in PHASES
    value = json.loads(receipt.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
    original = value["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-gateway-supply-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_gateway\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / f"build/{configuration}/openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_GATEWAY_PHASE=phase)
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-gateway]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=environment)
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and "SURFACE_GATEWAY_COMPLETE " in terminal, log
    before, output = [context.saves / f"surface-gateway-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    initial, final = observe(before), observe(output)
    assert initial["facility"] == final["facility"] and POLICY(ROUTES(before)) == POLICY(ROUTES(output))
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_GATEWAY_" + prefix + " ")]
            for name, prefix in dict(shipments="SHIPMENT", handling="HANDLING", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED").items()}
    if phase in ("ship-work", "handle-work", "service"):
        earned, = rows["earned"]; assert 0 <= earned["earned"] <= 3000 and not rows["invoices"]
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == earned["earned"]
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
        assert initial["company"]["debt"] == final["company"]["debt"]
        if phase != "ship-work":
            assert freight_credit(initial) == freight_credit(final)
        if phase == "service":
            assert earned["earned"] == 3000 and not earned["reached"]
    else:
        assert first[b"DATE"] == last[b"DATE"] and initial["company"] == final["company"]
        assert first[b"CAPA"] == last[b"CAPA"] and freight_credit(initial) == freight_credit(final)
        if phase in ("shipment-query", "handling-query"):
            assert first == last and initial == final and not rows["invoices"]
        else:
            assert rows["invoices"] and all(row["cost"] == 0 for row in rows["invoices"])
            selected = {row["vehicle"] for row in initial["shipments"].values()}; a, b = TABLE(before, b"VEHS"), TABLE(output, b"VEHS")
            # Dispatch may change only the seven selected native control rows.
            # Handling changes no body or wagon; all actual useful service and
            # unrelated chemical custody remain byte-exact at the same tick.
            assert all(b[key] == row for key, row in a.items() if phase == "handling" or key not in selected)
            if phase == "dispatch":
                old_orders, new_orders = TABLE(before, b"ORDL"), TABLE(output, b"ORDL")
                for name in SHIPMENTS:
                    key = initial["shipments"][name]["vehicle"]; prefix = "roadveh[0].common[0]."
                    old = old_orders[a[key][prefix + "orders"] - 1]; new = new_orders[b[key][prefix + "orders"] - 1]
                    assert old["orders.count"] == new["orders.count"] == 2 and new["orders[0].dest"] == 0
                    assert {k:v for k,v in old.items() if k.startswith("orders[1].")} == {k:v for k,v in new.items() if k.startswith("orders[1].")}
            for family in (b"MAPT", b"MAPH", b"MAPO", b"MAP2", b"M3LO", b"M3HI", b"MAP5", b"MAPE", b"MAP7", b"MAP8", b"INDY", b"LPCR", b"LPCC"):
                assert first[family] == last[family]
    assert identity(source) == original
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert len(processes) == 1
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=final, source_pause=source_pause, retained_harness=identity(retained), native_processes=processes,
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_gateway.cpp"), identity(DIRECTORY / "surface_gateway_material_ledger.py"),
                         identity(DIRECTORY / "surface_gateway_facility_ledger.py"), identity(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"),
                         identity(DIRECTORY / "surface_entry_regional_checks-3.py")])


def main():
    """Retain one selected original shipment/handling boundary without overwriting successful evidence."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
