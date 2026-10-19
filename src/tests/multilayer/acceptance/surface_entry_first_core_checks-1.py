"""Fund the genuine first Core and physically deliver its constructor's Steel.

@file surface_entry_first_core_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CB.
Only actual paid blueprint/order commands and synchronized work mutate owners.
Goods remain physically absent from the job until a later real resupply flight.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_native import execute as native
from surface_gateway_core_ledger import observe
from surface_gateway_mission_ledger import observe as mission
from surface_regional_ledger import TABLE

DIRECTORY = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(DIRECTORY / "surface_entry_gateway_supply_checks-3.py"))["trains"]
ROUTES = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))["all_orders"]
POLICY = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))["route_policy"]
PHASES = ("core-query", "core-buy", "core-order-query", "core-order", "core-steel-work", "core-service")


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual focused native build. @param phase Explicit pure/payment/order/material/service boundary.
    @param receipt Exact selected original predecessor. @param recover_run Successful native run to inspect without repeating work.
    @return Original paid Core/job, sole packet delivery, native orders/navigation and company evidence.
    """
    assert phase in PHASES
    prefixes = dict(plan="CORE_PLAN", state="CORE_STATE", order="CORE_ORDER", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED")
    files = [Path(__file__)] + [DIRECTORY / name for name in ("surface_gateway_core_ledger.py", "orbital_save_checks-1.py", "surface_gateway_mission_ledger.py", "surface_gateway_payload_ledger.py", "surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_facility_ledger.py")]
    current = native(configuration, phase, receipt, observe, prefixes, files, recover_run)
    before, output = [ROOT / current[field]["path"] for field in ("before", "output")]
    initial, final, rows = [current[key] for key in ("initial", "final", "rows")]
    first, last = [chunks(path)[1] for path in (before, output)]
    assert RAIL(before) == RAIL(output) and POLICY(ROUTES(before)) == POLICY(ROUTES(output))
    assert first[b"SPFL"] == last[b"SPFL"] and initial["plant"] == final["plant"]
    assert initial["embodied_inputs"] == final["embodied_inputs"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    assert initial["totals"] == final["totals"] == {"STEL": 20, "GOOD": 26, "PROP": 27}
    assert initial["fuel_burned"] == final["fuel_burned"] == 15
    assert initial["company"]["debt"] == final["company"]["debt"] == 4960000
    if phase in ("core-query", "core-order-query"):
        assert first == last and initial == final and not rows["invoices"]
    if phase == "core-query":
        plan, = rows["plan"]; assert plan["cost"] == 700000 and plan["planned"] == 863000 and plan["target"] == 870000 and plan["credit"] == 0
        assert plan["constructor"] == 38 and plan["constructor_generation"] == 2 and plan["model"] == 20
        assert not plan["target_exists"] and final["core"] is None
        source = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
        assert json.loads(json.dumps(mission(before))) == source["final"]
    else:
        state, = rows["state"]; core = final["core"]; row = core["row"]
        assert state["site"] == core["site"] and state["generation"] == row["generation"] and state["capital"] == 700000 and state["work"] == 0
        assert state["steel"] == row["inputs"][0]["quantity"] and state["goods"] == 0 and state["constructor"] == 38
        assert state["held"] + state["steel"] == 16 and state["paid"]
        # The original first-flight contract must still refuse every paid
        # blueprint. Only explicit Core permission admits the saved input job.
        try:
            mission(output)
        except AssertionError:
            pass
        else:
            raise AssertionError("Original first-mission observer admitted a Core")
        if phase == "core-buy":
            assert initial["core"] is None and state["steel"] == 0
            assert len(rows["invoices"]) == 1 and rows["invoices"][0]["cost"] == 700000
            assert initial["company"]["cash"] - final["company"]["cash"] == 700000
        elif phase == "core-order":
            assert len(rows["invoices"]) == 1 and rows["invoices"][0]["cost"] == 0 and initial["company"] == final["company"]
        if phase in ("core-buy", "core-order", "core-order-query"):
            assert first[b"DATE"] == last[b"DATE"] and first[b"SPSU"] == last[b"SPSU"]
        else:
            earned, = rows["earned"]; ticks = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
            assert earned["earned"] == ticks and not rows["invoices"]
            assert sum(item["amount"] for item in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
            assert state["steel"] == 16 and state["held"] == 0 and state["blocked"] == 9
            if phase == "core-service":
                assert ticks == 3000 and not earned["reached"]
            else:
                assert 0 < ticks < 3000 and earned["reached"]
        if phase in ("core-order-query", "core-order"):
            order, = rows["order"]; assert order["site"] == core["site"] and order["generation"] == row["generation"] and order["admitted"]
            assert order["executed"] == (phase == "core-order")
    return current


def main():
    """Retain one original Core boundary without overwriting or repeating successful native work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=PHASES, required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    current = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=current), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
