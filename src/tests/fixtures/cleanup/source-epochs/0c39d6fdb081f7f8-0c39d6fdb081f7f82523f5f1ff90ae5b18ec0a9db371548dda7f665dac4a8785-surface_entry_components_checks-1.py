"""Run real existing-carrier factory admission and bounded native collection.

@file surface_entry_components_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CF.1/.2.
The native controller owns orders, production, loading, movement and payments.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks
from surface_gateway_components_ledger import observe
from surface_gateway_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
FILES = ["surface_gateway_components_ledger.py", "surface_gateway_construction_ledger.py", "surface_gateway_resupply_ledger.py",
         "surface_gateway_core_ledger.py", "orbital_save_checks-1.py", "surface_gateway_mission_ledger.py", "surface_gateway_payload_ledger.py",
         "surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_facility_ledger.py"]


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual independent native build. @param phase Explicit pure query/dispatch/work/service boundary.
    @param receipt Exact selected original predecessor. @param recover_run Successful native invocation to inspect without repeating work.
    @return Original native receipts and independently conserved actual component custody.
    """
    assert phase in ("components-query", "components-dispatch-query", "components-work-query", "components-recall", "components-traffic-work", "components-dispatch", "components-work", "components-restore", "components-service")
    prefixes = dict(routes="COMPONENT_ROUTE", carriers="COMPONENT_CARRIER", budget="COMPONENT_BUDGET", progress="COMPONENT_PROGRESS",
                    traffic="COMPONENT_TRAFFIC", protection="COMPONENT_PROTECTION", restored="COMPONENT_RESTORED", funding="COMPONENT_FUND",
                    earned="EARNED", expenses="EXPENSE", delivered="DELIVERED", invoices="INVOICE")
    current = native(configuration, phase, receipt, observe, prefixes, [Path(__file__)] + [DIRECTORY / name for name in FILES], recover_run)
    initial, final, rows = current["initial"], current["final"], current["rows"]
    credits = sum(row["credit"] for row in rows["funding"] if row["executed"])
    assert final["company"]["debt"] == initial["company"]["debt"] + credits <= 5000000
    assert initial["orbital_consumed"] == final["orbital_consumed"] and initial["flight"] == final["flight"]
    # Both original and protected-current admission remain entirely pure;
    # quoted bank credit supplies no money until the native dispatch executes.
    if phase in ("components-query", "components-dispatch-query", "components-work-query"):
        assert chunks(ROOT / current["before"]["path"])[1] == chunks(ROOT / current["output"]["path"])[1] and initial == final and not rows["invoices"]
        if phase == "components-work-query":
            funding, = rows["funding"]; assert funding["target"] == 154061 and not funding["executed"] and funding["credit"] == 0
            return current
        assert len(rows["routes"]) == 2 and len(rows["carriers"]) == 5
        assert [(row["station"], row["source"]) for row in rows["routes"]] == [(1, 10), (19, 11)]
        assert [row["crossings"] for row in rows["routes"]] == [1, 0] and len(rows["traffic"]) == 3
        if phase == "components-dispatch-query":
            assert all(row["protected"] and row["orders"] == 3 for row in rows["traffic"])
        budget, = rows["budget"]; assert budget["funded"] and budget["target"] == 158061
    # Recall, dispatch and restoration change only native orders/control;
    # their zero-cost quotes cannot create cargo or alter company books.
    elif phase in ("components-recall", "components-dispatch", "components-restore"):
        assert initial["additional_collected"] == final["additional_collected"]
        if phase != "components-restore":
            assert initial["additional_collected"] == {"STEL": 0, "GOOD": 0, "PROP": 0}
        else:
            for name in ("Surface Steel reserve 1", "Surface Steel reserve 2"):
                truck = initial["shipments"][name]; assert truck["held"] == 18 and truck["home"] and truck["tile"] == 2443
            assert len(rows["restored"]) == 3
        assert initial["company"]["cash"] + credits == final["company"]["cash"] and rows["invoices"]
        assert all(row["cost"] == 0 for row in rows["invoices"])
    else:
        earned, = rows["earned"]; assert 0 < earned["earned"] <= 3000
        assert earned["cash_before"] == initial["company"]["cash"] + credits and earned["cash"] == final["company"]["cash"]
        assert earned["debt"] == final["company"]["debt"]
        assert earned["net_expense"] == sum(row["amount"] for row in rows["expenses"])
        assert earned["cash"] == earned["cash_before"] - earned["net_expense"]
        # The completed first-Core reserve is no longer a second inaccessible
        # cash account. This collection owns100000working cash plus61handling;
        # later module admission still requires its separate150000reserve.
        # Preserve any actual former-target shortfall rather than replaying
        # paid ticks to conceal the lump monthly interest debit.
        assert final["company"]["cash"] >= 100061
        current["reserve_accounting"] = dict(collection_working_reserve=100000, future_handling=61, future_module_reserve=150000,
                                              former_target_shortfall=max(0, 150061 - final["company"]["cash"]))
        for label in ("STEL", "GOOD", "PROP"):
            assert final["additional_collected"][label] >= initial["additional_collected"][label]
        if phase == "components-service":
            assert initial["components_collected"] and final["components_collected"] and earned["earned"] == 3000
    return current


def main():
    """Publish one actual original collection boundary without overwriting accepted evidence."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    current = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=current), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
