"""Admit actual finite-Salt delivery, bounded return and genuine temporary-asset sale.

@file surface_entry_salt_delivery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CIE.
Only native orders and earned simulation ticks deliver the original60Salt.
The scoped90000working floor covers actual charges; a later real empty sale
must restore100000before any chemical-carrier purchase. No resale is prepaid.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe_salt_delivery as observe
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
FILES = runpy.run_path(str(DIRECTORY / "surface_entry_component_delivery_checks-1.py"))["FILES"]
PHASES = {"query": "staged-delivery-query", "order": "staged-delivery-order",
          "work": "staged-delivery-work", "sell": "staged-empty-sale"}


def execute(configuration, phase, receipt, recover_run=None, limit=900):
    """Run only the next admitted actual original Salt boundary.

    @param configuration Independently linked native configuration.
    @param phase Pure quote, original order replacement, bounded work or real empty sale.
    @param receipt Immediately preceding genuine original native receipt.
    @param recover_run Existing successful invocation to inspect without replaying paid progress.
    @param limit Separately priced actual delivery cap in1..900ticks.
    @return Exact native books, conserved finite Salt, original paid owners and process identities.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    expected = {"query": ("staged-return-work", "staged-delivery-work"),
                "order": ("staged-delivery-query",), "work": ("staged-delivery-order", "staged-delivery-query"),
                "sell": ("staged-delivery-work",)}
    assert predecessor["phase"] in expected[phase]
    # Commands and work depend on an explicit funded native quote for this
    # exact cap. Later partial intervals must receive a fresh independent quote.
    if phase in ("order", "work"):
        admission, = predecessor["operation"]["rows"]["admission"]
        assert admission["funded"] and admission["ticks"] == limit and admission["refund_counted"] == 0
    prefixes = dict(admission="DELIVERY_ADMISSION", progress="DELIVERY_PROGRESS", sale="EMPTY_SALE",
                    earned="EARNED", expenses="EXPENSE", delivered="DELIVERED", invoices="INVOICE")
    files = [Path(__file__)] + [DIRECTORY / name for name in FILES]
    files += [DIRECTORY / name for name in ("surface_entry_chemical_checks-4.py", "surface_propellant_ledger.py", "surface_regional_ledger.py")]
    current = native(configuration, PHASES[phase], receipt, observe, prefixes, files,
                     recover_run=recover_run, delivery_limit=limit)
    initial, final, rows = current["initial"], current["final"], current["rows"]
    old = predecessor["operation"]["final"]
    assert all(json.loads(json.dumps(initial[key])) == value for key, value in old.items())
    assert initial["totals"] == final["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 12}
    assert initial["company"]["debt"] == final["company"]["debt"] == 5000000
    assert final["company"]["cash"] >= 90000
    assert initial["salt_delivery"]["delivered"] <= final["salt_delivery"]["delivered"] <= 60
    if phase == "query":
        assert initial == final and not rows["invoices"] and not rows["earned"]
        assert chunks(ROOT / current["before"]["path"])[1] == chunks(ROOT / current["output"]["path"])[1]
    elif phase == "order":
        assert initial["company"] == final["company"] and initial["salt_delivery"] == final["salt_delivery"]
        assert len(rows["invoices"]) == 5 and all(row["cost"] == 0 for row in rows["invoices"])
        assert not rows["earned"] and final["salt_consist"]["held"] == 60
        assert chunks(ROOT / current["before"]["path"])[1][b"DATE"] == chunks(ROOT / current["output"]["path"])[1][b"DATE"]
    elif phase == "work":
        admission, = rows["admission"]
        earned, = rows["earned"]
        progress, = rows["progress"]
        assert admission["funded"] and admission["ticks"] == limit and admission["reserve"] == 90000
        assert admission["refund_counted"] == admission["credit_counted"] == 0
        assert 0 < earned["earned"] <= limit and progress["earned"] == earned["earned"] and not rows["invoices"]
        assert progress["delivered"] == final["salt_delivery"]["delivered"]
        assert progress["held"] == final["salt_consist"]["held"]
        assert progress["empty_home"] == (final["salt_consist"]["home"] and final["salt_consist"]["held"] == 0)
        assert final["company"]["cash"] == initial["company"]["cash"] - earned["net_expense"]
        assert earned["net_expense"] == sum(row["amount"] for row in rows["expenses"])
        assert sum(row["amount"] for row in rows["expenses"] if row["amount"] > 0) <= admission["work_bound"]
    else:
        sale, = rows["sale"]
        invoice, = rows["invoices"]
        assert initial["salt_consist"]["home"] and initial["salt_consist"]["held"] == 0
        assert initial["salt_delivery"]["delivered"] == final["salt_delivery"]["delivered"] == 60
        assert "salt_consist" not in final and not rows["earned"]
        assert sale["reserve_restored"] and invoice["cost"] == sale["cost"] < 0
        assert final["company"]["cash"] == initial["company"]["cash"] - sale["cost"] >= 100000
    return current


def main():
    """Publish one actual bounded original Salt boundary without rewriting any predecessor."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=tuple(PHASES), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--limit", type=int, default=900)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    assert 0 < args.limit <= 900
    operation = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run, args.limit)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                      phase=PHASES[args.phase], operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
