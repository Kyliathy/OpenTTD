"""Prepare original ground payload/fuel and query first flight without beginning it.

@file surface_entry_gateway_payload_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQN.
Native packet moves and reciprocal carry links retain one actual payload.
Ground service cannot burn fuel, grant work or manufacture another paid hull.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_native import execute as native
from surface_gateway_payload_ledger import observe
from surface_regional_ledger import TABLE

DIRECTORY = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(DIRECTORY / "surface_entry_gateway_supply_checks-3.py"))["trains"]
ROUTES = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))["all_orders"]
POLICY = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))["route_policy"]
PHASES = ("payload-query", "payload", "flight-query", "payload-service")


def loaded(snapshot):
    """@param snapshot Actual current native ground snapshot. @return True only for original carried constructor16Steel/rocket30PROPwith no loose rocket freight."""
    rocket, = [craft for craft in snapshot["paid_craft"] if craft["model"] == 14]
    constructor, = [craft for craft in snapshot["paid_craft"] if craft["model"] == 20]
    steel = sum(packet["count"] for packet in snapshot["cohort"] if tuple(packet["owner"]) == ("vehicle", constructor["vehicle"]) and packet["label"] == "STEL")
    fuel = sum(packet["count"] for packet in snapshot["cohort"] if tuple(packet["owner"]) == ("supply", rocket["vehicle"], 0) and packet["label"] == "PROP")
    return constructor["placement"] == 4 and rocket["placement"] == 2 and steel == 16 and fuel == 30


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual focused build. @param phase Explicit pure, original loading or ground service boundary.
    @param receipt Exact selected predecessor. @param recover_run Successful native invocation to inspect without repetition.
    @return Paid body/material/carry/fuel/financial evidence, preserving every original useful service and wagon.
    """
    assert phase in PHASES
    prefixes = dict(budget="ASSEMBLY_BUDGET", plan="PAYLOAD_PLAN", loaded="PAYLOAD_LOADED", flight="FLIGHT_PLAN", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED")
    files = [Path(__file__)] + [DIRECTORY / name for name in ("surface_gateway_payload_ledger.py", "surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_facility_ledger.py")]
    operation = native(configuration, phase, receipt, observe, prefixes, files, recover_run)
    before, output = [ROOT / operation[field]["path"] for field in ("before", "output")]
    initial, final, rows = [operation[key] for key in ("initial", "final", "rows")]
    first, last = [chunks(path)[1] for path in (before, output)]
    assert RAIL(before) == RAIL(output) and POLICY(ROUTES(before)) == POLICY(ROUTES(output))
    assert first[b"SPFL"] == last[b"SPFL"] and first[b"SPST"] == last[b"SPST"]
    assert initial["totals"] == final["totals"] and initial["embodied_inputs"] == final["embodied_inputs"] and initial["plant"] == final["plant"]
    if phase == "payload-service":
        earned, = rows["earned"]; assert earned["earned"] == 3000 and not earned["reached"] and not rows["invoices"]
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == 3000
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
        assert initial["company"]["debt"] == final["company"]["debt"] and loaded(initial) and loaded(final)
    else:
        assert first[b"DATE"] == last[b"DATE"]
        a, b = TABLE(before, b"VEHS"), TABLE(output, b"VEHS")
        assert all(b[key] == row for key, row in a.items() if row["type"] != 6)
        if phase in ("payload-query", "flight-query"):
            assert first == last and initial == final and not rows["invoices"]
        else:
            budget, = rows["budget"]; invoice = sum(row["cost"] for row in rows["invoices"])
            assert invoice == 0 and len(rows["invoices"]) in (3, 4)
            assert final["company"]["cash"] == initial["company"]["cash"] + budget["credit"]
            assert final["company"]["debt"] == initial["company"]["debt"] + budget["credit"] <= 5000000 and loaded(final)
            # Native packet splitting/merging retains the whole actual feeder
            # credit for each original material across station, hold and fuel.
            for label in ("STEL", "GOOD", "PROP"):
                assert sum(packet["feeder_share"] for packet in initial["cohort"] if packet["label"] == label) == sum(packet["feeder_share"] for packet in final["cohort"] if packet["label"] == label)
        if phase != "flight-query":
            budget, = rows["budget"]; plan, = rows["plan"]
            assert budget["hulls"] == 0 and budget["planned"] == 868000 and budget["target"] == 870000
            assert plan["steel"] == 16 and plan["fuel"] == 30 and plan["mass"] == 384 and plan["capacity"] == 480
        else:
            assert loaded(final)
        for family in (b"MAPT", b"MAPH", b"MAPO", b"MAP2", b"M3LO", b"M3HI", b"MAP5", b"MAPE", b"MAP7", b"MAP8", b"INDY", b"LPCR", b"LPCC"):
            assert first[family] == last[family]
    if phase in ("payload", "flight-query"):
        flight, = rows["flight"]; assert flight["target"] == 21 and flight["target_generation"] == 1 and flight["fee"] == 5000 and flight["mass"] == 384 and not flight["begun"]
    return operation


def main():
    """Retain one original preflight boundary without overwriting successful native work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
