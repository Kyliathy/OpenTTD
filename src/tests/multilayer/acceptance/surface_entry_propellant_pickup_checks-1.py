"""Admit and earn actual26Propellant pickup in the existing27capacity reserve.

@file surface_entry_propellant_pickup_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CM.
Native available loading avoids an impossible27thunit. Actual depot rounds
retain any partial cargo and need fresh admission before another real trip.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe_propellant_pickup as observe
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
FILES = runpy.run_path(str(DIRECTORY / "surface_entry_salt_delivery_checks-1.py"))["FILES"]
PHASES = {"query": "staged-chemical-fuel-query", "order": "staged-chemical-fuel-order", "work": "staged-chemical-fuel-work"}


def execute(configuration, phase, receipt, recover_run=None, limit=3000):
    """Execute only one freshly admitted original Propellant collection boundary.

    @param configuration Independently linked current focused application.
    @param phase Pure quote, actual available-load/return orders or capped pickup round.
    @param receipt Immediate original produced stock, query, order or partial round output.
    @param recover_run Existing successful native invocation to inspect without repeating work.
    @param limit Independently quoted actual work cap in1..3000ticks.
    @return Literal Source6custody, unchanged original launch stock and actual native books.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    expected = {"query": ("staged-chemical-delivery-work", "staged-chemical-fuel-work"),
                "order": ("staged-chemical-fuel-query",), "work": ("staged-chemical-fuel-order", "staged-chemical-fuel-query")}
    assert predecessor["phase"] in expected[phase]
    # Each actual round spends only its own funded current bound. A prior
    # partial round supplies real cargo, never permission to repeat free work.
    if phase in ("order", "work"):
        admitted, = predecessor["operation"]["rows"]["admission"]
        assert admitted["funded"] and admitted["ticks"] == limit and admitted["refund_counted"] == 0
    files = [Path(__file__)] + [DIRECTORY / name for name in FILES]
    files += [DIRECTORY / name for name in ("surface_entry_chemical_checks-4.py", "surface_propellant_ledger.py", "surface_regional_ledger.py")]
    operation = native(configuration, PHASES[phase], receipt, observe,
                       dict(admission="CHEMICAL_ADMISSION", fuel_admission="FUEL_ADMISSION", progress="FUEL_PROGRESS",
                            invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"), files,
                       recover_run=recover_run, chemical_limit=limit)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    old = predecessor["operation"]["final"]
    assert all(json.loads(json.dumps(first[key])) == value for key, value in old.items())
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert last["company"]["cash"] >= 100000 and last["plant"]["additional_produced"] == 26
    assert last["delivery_stocks"][21] == {"STEL": 32, "GOOD": 50, "PROP": 12}
    admission, = rows["admission"]
    fuel_admission, = rows["fuel_admission"]
    assert admission["ticks"] == limit and admission["reserve"] == fuel_admission["reserve"] == 100000
    assert admission["refund_counted"] == admission["credit_counted"] == 0 and admission["source_tile"] == 1896
    assert admission["funded"] == fuel_admission["funded"] and fuel_admission["vehicle"] == 2
    assert fuel_admission["available_load"] and not fuel_admission["impossible_full_load"]
    if phase == "query":
        assert first == last and not rows["invoices"] and not rows["earned"]
        assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
    elif phase == "order":
        assert first["company"] == last["company"] and first["plant"] == last["plant"]
        assert len(rows["invoices"]) == fuel_admission["orders"] + 3 and all(row["cost"] == 0 for row in rows["invoices"])
        assert not rows["earned"] and first["propellant_carrier"]["held"] == last["propellant_carrier"]["held"]
        assert chunks(ROOT / operation["before"]["path"])[1][b"DATE"] == chunks(ROOT / operation["output"]["path"])[1][b"DATE"]
    else:
        earned, = rows["earned"]
        progress, = rows["progress"]
        carrier = last["propellant_carrier"]
        assert admission["funded"] and not rows["invoices"]
        assert 0 < earned["earned"] <= limit and progress["earned"] == earned["earned"]
        assert progress["held"] == carrier["held"] and progress["stored"] == carrier["stored"] and progress["reserved"] == carrier["reserved"]
        assert progress["round_home"] == (carrier["home"] and carrier["tile"] == 2443)
        assert progress["collected"] == (progress["round_home"] and carrier["stored"] == 26 and carrier["reserved"] == 0)
        assert not progress["launch_delivered"]
        assert last["company"]["cash"] == first["company"]["cash"] - earned["net_expense"]
        assert earned["net_expense"] == sum(row["amount"] for row in rows["expenses"])
        assert sum(row["amount"] for row in rows["expenses"] if row["amount"] > 0) <= admission["work_bound"]
    return operation


def main():
    """Publish one immutable actual original fuel-collection boundary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=tuple(PHASES), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--limit", type=int, default=3000)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    assert 0 < args.limit <= 3000
    operation = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run, args.limit)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                      phase=PHASES[args.phase], operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
