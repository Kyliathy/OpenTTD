"""Bind short renewed-fleet economic batches and actual current fuel affordability.

@file surface_entry_fuel_checks-14.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIB.2.
All prior construction, finite Salt, chemicals and renewal remain accepted
ancestors. Only the newly earned continuous financial lineage is audited here.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]


def bind(through, require_affordable):
    """@param through Last completed actual annual period. @param require_affordable Require the entire fresh fuel bill.
    @return Exact both-build continuous native finance/material proof, never a forecast or supplied balance.
    """
    foundation_path = ROOT / "logs/surface-entry-income-fleet-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["actual_reliable_income"]
    checked_ids, processes, physical, build_rows, summaries = set(), set(), [], [], []

    def checked(reference):
        """@param reference Exact immutable immediate artifact. @return Verified original repository path."""
        path = ROOT / reference["path"]
        key = reference["path"], reference["sha256"]
        if key not in checked_ids:
            assert identity(path) == reference, path
            checked_ids.add(key)
        return path

    def walk(value):
        """@param value Immediate process/receipt tree. Verify every artifact once without replaying accepted ancestors."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    for configuration in ("RelWithDebInfo", "Debug"):
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        source_receipt = ROOT / f"logs/surface-entry-income-earned-{configuration}-27.json"
        starting = json.loads(source_receipt.read_text(encoding="utf-8"))["operation"]["final"]["company"]
        snapshots, rows, receipts = [], [], []
        net_expense, paid_income, repaid, earned_ticks = 0, 0, 0, 0
        # Follow only the selected first-output chain. A saved branch with a
        # better balance or a resaved proof replay cannot finance this company.
        for period in range(27, through):
            initial = json.loads(source_receipt.read_text(encoding="utf-8"))["operation"]["final"]
            paths = []
            paid = ROOT / f"logs/surface-entry-income-renewed-repaid-{configuration}-{period}.json"
            if initial["company"]["cash"] >= 160000:
                assert paid.is_file(); paths.append(paid)
            else:
                assert not paid.exists()
            paths.append(ROOT / f"logs/surface-entry-income-earned-{configuration}-{period + 1}.json")
            for path in paths:
                value = json.loads(path.read_text(encoding="utf-8")); walk(value)
                assert value["complete"] and value["configuration"] == configuration
                operation = value["operation"]
                assert operation["receipt"] == identity(source_receipt) and operation["source"] == previous
                assert operation["initial"]["reserves"] == operation["final"]["reserves"] == dict(steel=36, goods=14, chemicals=21)
                source, before, output = [checked(operation[name]) for name in ("source", "before", "output")]
                old, first, last = [chunks(item)[1] for item in (source, before, output)]
                assert old.keys() == first.keys() == last.keys()
                assert all(old[key] == first[key] for key in old if key != b"GLOG")
                if old[b"GLOG"] != first[b"GLOG"]:
                    REVISION(source, before)
                assert first[b"GLOG"] == last[b"GLOG"]
                if value["phase"] == "repay":
                    bank, = operation["rows"]["repayment"]
                    assert bank["buffer"] == 150000 and 150000 <= bank["cash"] < 160000
                    assert bank["ceiling"] == 5000000 and bank["amount"] > 0 and bank["amount"] % 10000 == 0
                    assert bank["spendable"] == bank["cash"] + bank["ceiling"] - bank["debt"]
                    assert all(first[key] == last[key] for key in first if key != b"PLYR")
                    repaid += bank["amount"]
                else:
                    assert value["phase"] == "earn"
                    earned, = operation["rows"]["earned"]
                    assert 0 < earned["earned"] <= 27000 and earned["debt"] == operation["initial"]["company"]["debt"]
                    assert earned["cash"] == operation["final"]["company"]["cash"]
                    assert sum(row["amount"] for row in operation["rows"]["expenses"]) == earned["cash_before"] - earned["cash"]
                    net_expense += earned["net_expense"]; paid_income += earned["income"]; earned_ticks += earned["earned"]
                    if period + 1 < through:
                        assert not earned["affordable"] and earned["earned"] == 27000
                for reference in operation["native_processes"]:
                    process = json.loads(checked(reference).read_text(encoding="utf-8")); walk(process)
                    assert process["exit_status"] == 0; processes.add(reference["path"])
                snapshots.extend((first, last)); rows.append(operation["rows"]); receipts.append(identity(path))
                previous = operation["output"]; source_receipt = path
        earned, = operation["rows"]["earned"]
        company = operation["final"]["company"]
        assert company["cash"] == starting["cash"] - net_expense - repaid
        assert company["debt"] == starting["debt"] - repaid
        spendable = company["cash"] + 5000000 - company["debt"]
        assert spendable == starting["cash"] + 5000000 - starting["debt"] - net_expense
        assert earned["affordable"] == (spendable >= earned["requirement"])
        if require_affordable:
            assert earned["affordable"]
        physical.append(snapshots); build_rows.append(rows)
        summaries.append(dict(configuration=configuration, receipts=receipts, output=previous, through_period=through,
                              company=company, spendable_capital=spendable, fresh_requirement=earned["requirement"],
                              actual_earned_ticks=earned_ticks, actual_freight_income=paid_income, actual_net_earned=-net_expense,
                              matching_native_repayments=repaid,
                              full_propellant_affordability=earned["affordable"]))
    assert build_rows[0] == build_rows[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    return dict(_documentation=__doc__, complete=True, foundation=identity(foundation_path), configurations=summaries,
                native_processes=len(processes), exact_cross_build_gameplay_comparisons=comparisons,
                full_propellant_affordability=summaries[0]["full_propellant_affordability"], source=identity(Path(__file__)))


def main():
    """Publish the exact current capital boundary; partial batches cannot close full finance."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--through-period", type=int, required=True)
    parser.add_argument("--require-affordable", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert args.through_period > 27 and output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.through_period, args.require_affordable), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
