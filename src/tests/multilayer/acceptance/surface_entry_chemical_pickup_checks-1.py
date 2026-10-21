"""Admit and earn actual chemical pickup by the genuinely renewed tanker.

@file surface_entry_chemical_pickup_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CK.
Native Industry15packets, original road orders and company books supply all
progress. Full stored27/zeroreservedand actual depot halt prove collection.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe_chemical_pickup as observe
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
FILES = runpy.run_path(str(DIRECTORY / "surface_entry_salt_delivery_checks-1.py"))["FILES"]
PHASES = {"query": "staged-chemical-query", "order": "staged-chemical-order", "work": "staged-chemical-work"}


def execute(configuration, phase, receipt, recover_run=None, limit=3000):
    """Run only one separately admitted native chemical-pickup boundary.

    @param configuration Independently linked focused native application.
    @param phase Pure quote, original pickup/return orders or bounded actual work.
    @param receipt Immediately preceding original renewal, quote, order or partial work.
    @param recover_run Successful native invocation to inspect without repeating work.
    @param limit Freshly priced actual cap in1..3000ticks, preserving all previous progress.
    @return Native costs, physical Source15custody, actual clocks and unchanged original paid owners.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    expected = {"query": ("staged-chemical-renew", "staged-chemical-work"),
                "order": ("staged-chemical-query",), "work": ("staged-chemical-order", "staged-chemical-query")}
    assert predecessor["phase"] in expected[phase]
    # Every command/work interval depends on an exact funded fresh native
    # quote for this cap. A partial pickup requires another explicit admission.
    if phase in ("order", "work"):
        admitted, = predecessor["operation"]["rows"]["admission"]
        assert admitted["funded"] and admitted["ticks"] == limit and admitted["refund_counted"] == 0
    files = [Path(__file__)] + [DIRECTORY / name for name in FILES]
    files += [DIRECTORY / name for name in ("surface_entry_chemical_checks-4.py", "surface_propellant_ledger.py", "surface_regional_ledger.py")]
    operation = native(configuration, PHASES[phase], receipt, observe,
                       dict(admission="CHEMICAL_ADMISSION", progress="CHEMICAL_PROGRESS", invoices="INVOICE",
                            earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"), files,
                       recover_run=recover_run, chemical_limit=limit)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    old = predecessor["operation"]["final"]
    assert all(json.loads(json.dumps(first[key])) == value for key, value in old.items() if key != "chemical_carrier")
    assert all(json.loads(json.dumps(first["chemical_carrier"][key])) == value for key, value in old["chemical_carrier"].items())
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 12}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert last["company"]["cash"] >= 100000 and last["salt_delivery"]["delivered"] == 60
    admission, = rows["admission"]
    assert admission["ticks"] == limit and admission["reserve"] == 100000
    assert admission["refund_counted"] == admission["credit_counted"] == 0
    if phase == "query":
        assert first == last and not rows["invoices"] and not rows["earned"]
        assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
    elif phase == "order":
        assert first["company"] == last["company"] and first["salt_delivery"] == last["salt_delivery"]
        assert len(rows["invoices"]) == 3 and all(row["cost"] == 0 for row in rows["invoices"])
        assert not rows["earned"] and last["chemical_carrier"]["held"] == 0
        assert chunks(ROOT / operation["before"]["path"])[1][b"DATE"] == chunks(ROOT / operation["output"]["path"])[1][b"DATE"]
    else:
        earned, = rows["earned"]
        progress, = rows["progress"]
        carrier = last["chemical_carrier"]
        assert admission["funded"] and not rows["invoices"]
        assert 0 < earned["earned"] <= limit and progress["earned"] == earned["earned"]
        assert progress["held"] == carrier["held"] and progress["stored"] == carrier["stored"] and progress["reserved"] == carrier["reserved"]
        assert progress["collected"] == (carrier["home"] and carrier["tile"] == 2443 and carrier["stored"] == 27 and carrier["reserved"] == 0)
        assert last["company"]["cash"] == first["company"]["cash"] - earned["net_expense"]
        assert earned["net_expense"] == sum(row["amount"] for row in rows["expenses"])
        assert sum(row["amount"] for row in rows["expenses"] if row["amount"] > 0) <= admission["work_bound"]
    return operation


def main():
    """Publish one immutable genuine original chemical-pickup boundary."""
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
