"""Admit actual fuel road delivery and separately stock-backed launch-hub handling.

@file surface_entry_propellant_delivery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CN.
The completed26Source6acquisition stays fixed while native packets change
vehicle/member/job/center owners. Ordinary loading and future capital queries
grant neither cargo nor paid modules; current working reserve stays100000.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe_propellant_delivery as observe
from surface_gateway_native import execute as gateway
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
COMPONENT = runpy.run_path(str(DIRECTORY / "surface_entry_component_delivery_checks-1.py"))
FILES = COMPONENT["FILES"]
PHASES = {"query": "staged-chemical-hub-query", "order": "staged-chemical-hub-order", "work": "staged-chemical-hub-work",
          "handling-query": "staged-chemical-handling-query", "handling-start": "staged-chemical-handling-start", "handling-work": "staged-chemical-handling-work"}


def caller_files():
    """@return Exact current producer, observer, native price/renewal and fee authorities used by this group."""
    files = [Path(__file__)] + [DIRECTORY / name for name in FILES]
    files += [DIRECTORY / name for name in ("surface_entry_component_delivery_checks-1.py", "surface_entry_chemical_checks-4.py",
                                          "surface_propellant_ledger.py", "surface_regional_ledger.py")]
    files += [ROOT / "src" / name for name in ("vehicle.cpp", "economy_type.h", "company_base.h", "surface_handling.cpp")]
    return files


def modules(configuration, receipt, recover_run=None):
    """Readmit actual original module/material/fuel capital after physical new-fuel storage.

    @param configuration Independently linked actual focused application.
    @param receipt Genuine completed original reverse-handling output.
    @param recover_run Existing successful native pure invocation to inspect without repeating it.
    @return Unchanged actual38fuel/32Steel/50Goods and native unpaid module/mission funding gaps.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["phase"] == "staged-chemical-handling-work"
    assert predecessor["operation"]["final"]["new_fuel_at_center"]
    operation = gateway(configuration, "expansion-query", receipt, observe,
                        dict(layout="EXPANSION_LAYOUT", inputs="EXPANSION_INPUT", fuel="EXPANSION_FUEL",
                             budget="EXPANSION_BUDGET", flight="EXPANSION_FLIGHT"), caller_files(), recover_run)
    assert json.loads(json.dumps(operation["initial"])) == predecessor["operation"]["final"]
    assert operation["initial"] == operation["final"]
    assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
    fuel, = operation["rows"]["fuel"]
    budget, = operation["rows"]["budget"]
    assert fuel["needed"] == 30 and fuel["held"] == 38 and fuel["missing"] == 0
    assert all(row["missing"] == 0 for row in operation["rows"]["inputs"])
    assert budget["debt"] == budget["ceiling"] == 5000000 and budget["reserve"] == 150000
    return operation


def execute(configuration, phase, receipt, recover_run=None, limit=3000):
    """Run only one separately admitted original fuel-delivery/handling boundary.

    @param configuration Independently linked current focused native application.
    @param phase Pure route quote, native transfer/return orders, real transport, stock-backed handling quote/start/work.
    @param receipt Immediately preceding original collected fuel or current accepted stage.
    @param recover_run Successful native invocation to inspect without replaying paid work.
    @param limit Fresh cap in1..3000for road work or1..50for actual handling.
    @return Exact current company books, sole native fuel owners and actual bounded progress.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    expected = {"query": ("staged-chemical-fuel-work", "staged-chemical-hub-work"),
                "order": ("staged-chemical-hub-query",), "work": ("staged-chemical-hub-order", "staged-chemical-hub-query"),
                "handling-query": ("staged-chemical-hub-work",), "handling-start": ("staged-chemical-handling-query",),
                "handling-work": ("staged-chemical-handling-start",)}
    assert predecessor["phase"] in expected[phase]
    # Every command/work action needs the exact current funded quote. The
    # handling quote remains a separate stock-backed boundary after actual
    # member delivery; a conditional fee allowance is never cargo authority.
    if phase in ("order", "work", "handling-start", "handling-work"):
        admitted, = predecessor["operation"]["rows"]["admission"]
        assert admitted["funded"] and admitted["ticks"] == limit
    if phase.startswith("handling"):
        assert limit <= 50
    operation = native(configuration, PHASES[phase], receipt, observe,
                       dict(admission="CHEMICAL_ADMISSION", hub_admission="HUB_ADMISSION", handling_admission="HANDLING_ADMISSION",
                            progress="HUB_PROGRESS", handling_progress="HANDLING_PROGRESS", invoices="INVOICE",
                            earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"), caller_files(),
                       recover_run=recover_run, chemical_limit=limit)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert all(json.loads(json.dumps(first[key])) == value for key, value in predecessor["operation"]["final"].items())
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert last["company"]["cash"] >= 100000
    admission, = rows["admission"]
    assert admission["ticks"] == limit and admission["reserve"] == 100000 and admission["handling_allowance"] == 26
    assert admission["refund_counted"] == admission["credit_counted"] == 0 and admission["source_tile"] == 2504
    assert admission["renew_months"] == 6 and admission["renewal_lead"] == 0
    if phase.endswith("query"):
        assert first == last and not rows["invoices"] and not rows["earned"]
        assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
    elif phase == "order":
        assert first["company"] == last["company"] and first["propellant_carrier"]["stored"] == last["propellant_carrier"]["stored"] == 26
        assert len(rows["invoices"]) == 5 and all(row["cost"] == 0 for row in rows["invoices"])
        assert not rows["earned"]
        assert chunks(ROOT / operation["before"]["path"])[1][b"DATE"] == chunks(ROOT / operation["output"]["path"])[1][b"DATE"]
    elif phase == "handling-start":
        assert first["new_fuel_at_member"] and first["company"] == last["company"]
        invoice, = rows["invoices"]
        assert invoice["cost"] == 0 and not rows["earned"]
        job, = last["handling"]["jobs"]
        # The job's referenced packet counts, rather than routing metadata,
        # become the sole actual owner of exactly26units until native ticks move them.
        assert job["label"] == "PROP" and job["remaining"] == 4
        assert sum(packet["count"] for packet in last["cohort"] if tuple(packet["owner"]) == ("handling", job["id"])) == 26
        assert last["delivery_stocks"][0]["PROP"] == 0 and last["delivery_stocks"][21]["PROP"] == 12
        assert chunks(ROOT / operation["before"]["path"])[1][b"DATE"] == chunks(ROOT / operation["output"]["path"])[1][b"DATE"]
    else:
        earned, = rows["earned"]
        assert admission["funded"] and not rows["invoices"]
        assert 0 < earned["earned"] <= limit
        assert last["company"]["cash"] == first["company"]["cash"] - earned["net_expense"]
        assert earned["net_expense"] == sum(row["amount"] for row in rows["expenses"])
        assert sum(row["amount"] for row in rows["expenses"] if row["amount"] > 0) <= admission["work_bound"]
        if phase == "work":
            progress, = rows["progress"]
            assert progress["earned"] == earned["earned"] and progress["member_ready"] == last["new_fuel_at_member"]
            assert progress["member_stock"] == last["delivery_stocks"][0]["PROP"]
            assert progress["center_stock"] == last["delivery_stocks"][21]["PROP"] and not progress["handling_started"]
        else:
            progress, = rows["handling_progress"]
            assert progress["earned"] == earned["earned"] and progress["stored"] == last["new_fuel_at_center"]
            assert last["new_fuel_at_center"] and earned["earned"] == 4
            upkeep = COMPONENT["idle_space_upkeep"](ROOT / operation["before"]["path"], ROOT / operation["output"]["path"])
            other, = [row["amount"] for row in rows["expenses"] if row["type"] == 12]
            # Native loan-interest booking identifies an actual monthly charge
            # within this at-most50tick interval; its fixed station fee shares
            # Otherwith Space upkeep and handling, so reconcile all three.
            interest, = [row["amount"] for row in rows["expenses"] if row["type"] == 11]
            station = admission["station_charge"] if interest > 0 else 0
            assert other - upkeep - station == 26
            operation["actual_handling_fee"] = 26
    return operation


def main():
    """Publish one immutable actual original delivery/handling or unpaid module-admission boundary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=tuple(PHASES) + ("modules",), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--limit", type=int)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    limit = args.limit if args.limit is not None else (10 if args.phase.startswith("handling") else 3000)
    assert 0 < limit <= 3000
    operation = modules(args.configuration, ROOT / args.receipt, args.recover_run) if args.phase == "modules" else execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run, limit)
    phase = "expansion-query" if args.phase == "modules" else PHASES[args.phase]
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                      phase=phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
