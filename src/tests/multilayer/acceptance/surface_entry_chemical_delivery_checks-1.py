"""Admit and earn actual chemical delivery and bounded native PCHMproduction.

@file surface_entry_chemical_delivery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CL.
The original27Source15CHEMload supplies complete native two-unit batches.
New26PROP stays with actual Industry/station owners until later collection.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe_chemical_delivery as observe
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
FILES = runpy.run_path(str(DIRECTORY / "surface_entry_salt_delivery_checks-1.py"))["FILES"]
PHASES = {"query": "staged-chemical-delivery-query", "order": "staged-chemical-delivery-order", "work": "staged-chemical-delivery-work"}


def execute(configuration, phase, receipt, recover_run=None, limit=3000):
    """Run one independently admitted original chemical-delivery/processing boundary.

    @param configuration Independently linked actual native application.
    @param phase Pure quote, no-load receiver/return orders or bounded genuine work.
    @param receipt Immediate original collected load, quote, order or partial delivery output.
    @param recover_run Existing successful native invocation to inspect without replaying work.
    @param limit Fresh actual work cap in1..3000ticks, keeping all earlier progress.
    @return Literal original cargo custody, native PCHMbatch receipts and actual company books.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    expected = {"query": ("staged-chemical-work", "staged-chemical-delivery-work"),
                "order": ("staged-chemical-delivery-query",), "work": ("staged-chemical-delivery-order", "staged-chemical-delivery-query")}
    assert predecessor["phase"] in expected[phase]
    # Every order/work action requires a funded native quote for this exact
    # cap. Native re-admission, rather than a prior allowance, pays each later
    # continuation when a real trip or production interval remains incomplete.
    if phase in ("order", "work"):
        admitted, = predecessor["operation"]["rows"]["admission"]
        assert admitted["funded"] and admitted["ticks"] == limit and admitted["refund_counted"] == 0
    files = [Path(__file__)] + [DIRECTORY / name for name in FILES]
    files += [DIRECTORY / name for name in ("surface_entry_chemical_checks-4.py", "surface_propellant_ledger.py", "surface_regional_ledger.py")]
    operation = native(configuration, PHASES[phase], receipt, observe,
                       dict(admission="CHEMICAL_ADMISSION", pchm_admission="PCHM_ADMISSION", progress="PCHM_PROGRESS",
                            invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"), files,
                       recover_run=recover_run, chemical_limit=limit)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    old = predecessor["operation"]["final"]
    assert all(json.loads(json.dumps(first[key])) == value for key, value in old.items() if key != "plant")
    assert all(first["plant"][key] == value for key, value in old["plant"].items())
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 12}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert last["company"]["cash"] >= 100000 and last["salt_delivery"]["delivered"] == 60
    admission, = rows["admission"]
    pchm_admission, = rows["pchm_admission"]
    assert admission["ticks"] == limit and admission["reserve"] == pchm_admission["reserve"] == 100000
    assert admission["refund_counted"] == admission["credit_counted"] == 0
    assert admission["funded"] == pchm_admission["funded"]
    if phase == "query":
        assert first == last and not rows["invoices"] and not rows["earned"]
        assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
    elif phase == "order":
        assert first["company"] == last["company"] and first["plant"] == last["plant"]
        assert len(rows["invoices"]) == 5 and all(row["cost"] == 0 for row in rows["invoices"])
        assert not rows["earned"] and last["chemical_carrier"]["stored"] == 27
        assert chunks(ROOT / operation["before"]["path"])[1][b"DATE"] == chunks(ROOT / operation["output"]["path"])[1][b"DATE"]
    else:
        earned, = rows["earned"]
        progress, = rows["progress"]
        carrier, plant = last["chemical_carrier"], last["plant"]
        assert admission["funded"] and not rows["invoices"]
        assert 0 < earned["earned"] <= limit and progress["earned"] == earned["earned"]
        assert progress["held"] == carrier["held"] and progress["stored"] == carrier["stored"] and progress["reserved"] == carrier["reserved"]
        assert progress["delivered"] == plant["delivered"] and progress["produced"] == plant["additional_produced"]
        assert progress["complete"] == (carrier["home"] and carrier["tile"] == 2443 and carrier["held"] == 0 and plant["delivered"] == 27 and plant["additional_produced"] == 26)
        assert not progress["new_fuel_collected"]
        assert last["company"]["cash"] == first["company"]["cash"] - earned["net_expense"]
        assert earned["net_expense"] == sum(row["amount"] for row in rows["expenses"])
        assert sum(row["amount"] for row in rows["expenses"] if row["amount"] > 0) <= admission["work_bound"]
    return operation


def main():
    """Publish one immutable actual chemical-delivery/processing boundary."""
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
