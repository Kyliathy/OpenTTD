"""Carry the genuine first Core's Goods on the original rocket's second mission.

@file surface_entry_core_resupply_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CC.
Native commands release the constructor and load real stock; synchronized
flight transfers those packets once while the paid Core job retains its Steel.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_native import execute as native
from surface_gateway_resupply_ledger import observe
from surface_gateway_core_ledger import observe as core
from surface_regional_ledger import TABLE

DIRECTORY = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(DIRECTORY / "surface_entry_gateway_supply_checks-3.py"))["trains"]
ROUTES = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))["all_orders"]
POLICY = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))["route_policy"]
PHASES = ("resupply-query", "resupply-load", "resupply-flight-query", "resupply-flight", "resupply-service")


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual focused native build. @param phase Explicit pure/release/loading/flight/service boundary.
    @param receipt Exact selected original predecessor. @param recover_run Successful native run to inspect without repeating work.
    @return Exact native ownership, intermediate phases, company payment and original useful-service evidence.
    """
    assert phase in PHASES
    prefixes = dict(budget="RESUPPLY_BUDGET", plan="RESUPPLY_PLAN", flight="RESUPPLY_FLIGHT_PLAN", state="RESUPPLY_STATE", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED")
    files = [Path(__file__)] + [DIRECTORY / name for name in ("surface_gateway_resupply_ledger.py", "surface_gateway_core_ledger.py", "orbital_save_checks-1.py", "surface_gateway_mission_ledger.py", "surface_gateway_payload_ledger.py", "surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_facility_ledger.py")]
    current = native(configuration, phase, receipt, observe, prefixes, files, recover_run)
    before, output = [ROOT / current[field]["path"] for field in ("before", "output")]
    initial, final, rows = [current[key] for key in ("initial", "final", "rows")]
    first, last = [chunks(path)[1] for path in (before, output)]
    assert RAIL(before) == RAIL(output) and POLICY(ROUTES(before)) == POLICY(ROUTES(output))
    assert initial["company"]["debt"] == final["company"]["debt"] == 4960000 and initial["plant"] == final["plant"]
    assert initial["embodied_inputs"] == final["embodied_inputs"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    # Delivery changes custody without paying freight income or consuming
    # either recipe material. Ordinary packet aging cannot change feeder credit.
    for label in ("STEL", "GOOD"):
        assert sum(packet["feeder_share"] for packet in initial["cohort"] if packet["label"] == label) == sum(packet["feeder_share"] for packet in final["cohort"] if packet["label"] == label)
    if phase in ("resupply-query", "resupply-flight-query"):
        assert first == last and initial == final and not rows["invoices"]
    if phase == "resupply-query":
        source = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
        assert json.loads(json.dumps(core(before))) == source["final"]
        plan, = rows["plan"]; assert plan["goods"] == 24 and plan["mass"] == 192 and plan["capacity"] == 480
        assert plan["pending_order_busy"] and plan["release_admitted"] and plan["load_admitted"] and not plan["flight_begun"]
    else:
        # Earlier Core permission must continue to reject both the loaded
        # rocket and the actual24Goodsreceiver after its second flight.
        try:
            core(output)
        except AssertionError:
            pass
        else:
            raise AssertionError("Original Steel-only Core observer admitted Goods resupply")
        if phase == "resupply-load":
            assert first[b"DATE"] == last[b"DATE"] and first[b"SPFL"] == last[b"SPFL"] and first[b"SPST"] == last[b"SPST"]
            assert len(rows["invoices"]) == 2 and all(row["cost"] == 0 for row in rows["invoices"]) and initial["company"] == final["company"]
            assert final["goods_holds"] == {54: 24, 38: 0} and not final["space_orders"]["queues"][38]["entries"]
        elif phase in ("resupply-flight", "resupply-service"):
            ticks = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
            assert sum(row["earned"] for row in rows["earned"]) == ticks
            expense = sum(row["amount"] for row in rows["expenses"])
            assert expense == sum(row["net_expense"] for row in rows["earned"])
            fee = sum(row["cost"] for row in rows["invoices"])
            assert initial["company"]["cash"] - final["company"]["cash"] == expense + fee
            assert final["flight"]["started"] == final["flight"]["completed"] == 2 and final["fuel_burned"] == 30
            assert final["goods_holds"] == {54: 0, 38: 24} and final["totals"] == {"STEL": 20, "GOOD": 26, "PROP": 12}
            if phase == "resupply-flight":
                assert fee == 5000 and len(rows["invoices"]) == 1 and [row["earned"] for row in rows["earned"]] == [148, 222, 1, 296]
                assert all(row["reached"] for row in rows["earned"]) and ticks == 667
                references, states = [], []
                # Inspect every saved real phase from this same invocation,
                # binding physical packet/burn receipts to exact native bytes.
                for name, expected in zip(("preparing", "ascent", "orbit", "delivered", "recovered"), (1, 2, 3, 4, 0), strict=True):
                    path = output.parent / f"surface-gateway-resupply-flight-{name}-440.sav"; state = observe(path)
                    assert state["flight"]["phase"] == expected
                    references.append(dict(name=name, saved=identity(path), observed=state)); states.append(expected)
                assert [row["phase"] for row in rows["state"]] == states
                current["stages"] = references
            else:
                assert fee == 0 and not rows["invoices"] and len(rows["earned"]) == 1 and ticks == 3000 and not rows["earned"][0]["reached"]
    if phase != "resupply-service":
        assert rows["budget"] and all(row["planned"] == 161500 and row["target"] == 170000 and row["credit"] == 0 for row in rows["budget"])
    if phase in ("resupply-load", "resupply-flight-query", "resupply-flight"):
        flight, = rows["flight"]; assert flight["target"] == 38 and flight["target_generation"] == 2 and flight["fee"] == 5000 and flight["mass"] == 192 and not flight["begun"]
    return current


def main():
    """Retain one actual resupply boundary without overwriting or repeating successful native work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    current = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=current), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
