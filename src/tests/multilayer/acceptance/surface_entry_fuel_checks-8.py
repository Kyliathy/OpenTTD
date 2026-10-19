"""Bind genuine post-service earnings, normal renewal and matching native debt repayment.

@file surface_entry_fuel_checks-8.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIB.1.
Real current money plus remaining bank headroom is conserved by repayment.
Quoted interest savings are not received income or factory affordability.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def bind():
    """@return Verified both-build year14-19 and actual original bank settlement checkpoint."""
    foundation_path = ROOT / "logs/surface-entry-income-coal-replacement-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["actual_replacement_complete"]
    aliases = {}
    for name in ("income-coal-replacement-products", "income-lifecycle-products"):
        manifest = json.loads((ROOT / "test-cache/surface-entry" / name / "retained.json").read_text(encoding="utf-8"))
        for row in manifest["retained"]:
            aliases[row["sha256"]] = ROOT / row["path"]

    def checked(reference):
        """@param reference Exact original input/output/source. @return Verified original or explicitly retained alias."""
        path = aliases.get(reference["sha256"], ROOT / reference["path"])
        assert identity(path)["sha256"] == reference["sha256"], path
        return path

    def walk(value):
        """@param value Immediate receipt/process tree. Check original retained membership without replaying prior groups."""
        if isinstance(value, dict):
            if set(value) == {"path", "sha256"}:
                checked(value)
            else:
                for child in value.values():
                    walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    snapshots, field_rows, summaries, processes = [], [], [], set()
    for configuration in ("RelWithDebInfo", "Debug"):
        previous, = [row["output"] for row in foundation["configurations"] if row["configuration"] == configuration]
        paths = [ROOT / f"logs/surface-entry-income-earned-{configuration}-{period}.json" for period in range(14, 20)]
        paths.append(ROOT / f"logs/surface-entry-income-repaid-{configuration}-19.json")
        physical, rows, total_income = [], [], 0
        for index, path in enumerate(paths):
            value = json.loads(path.read_text(encoding="utf-8")); walk(value)
            op = value["operation"]
            assert value["complete"] and value["configuration"] == configuration and op["source"] == previous
            previous = op["output"]
            assert op["initial"]["reserves"] == op["final"]["reserves"] == dict(steel=36, goods=14, chemicals=21)
            if index < 6:
                earned, = op["rows"]["earned"]
                assert value["phase"] == "earn" and earned["earned"] == 27000 and earned["debt"] == 4650000 and not earned["affordable"]
                total_income += earned["income"]
            else:
                bank, = op["rows"]["repayment"]
                assert value["phase"] == "repay" and bank["amount"] == 230000
                assert bank["cash_before"] == 335143 and bank["debt_before"] == 4650000
                assert bank["cash"] == 105143 and bank["debt"] == 4420000 and bank["ceiling"] == 5000000
                assert bank["spendable"] == 685143 and bank["annual_interest_reduction"] == 4600
                before, after = [chunks(checked(op[name]))[1] for name in ("before", "output")]
                assert all(before[key] == after[key] for key in before if key != b"PLYR")
            for reference in op["native_processes"]:
                process = json.loads(checked(reference).read_text(encoding="utf-8")); walk(process)
                assert process["exit_status"] == 0; processes.add(reference["path"])
            # The later parser adds an empty repayment family to ordinary
            # earning receipts. Its earlier absence means the same no-bank-action
            # observation; every actual native row and all gameplay remain exact.
            normalized = dict(op["rows"]); normalized.setdefault("repayment", [])
            physical.extend(chunks(checked(op[name]))[1] for name in ("before", "output")); rows.append(normalized)
        snapshots.append(physical); field_rows.append(rows)
        summaries.append(dict(configuration=configuration, receipts=[identity(path) for path in paths], output=previous,
                              actual_earned_ticks=162000, actual_freight_income=total_income, cash=105143, debt=4420000, spendable_capital=685143))
    assert field_rows[0] == field_rows[1]
    comparisons = 0
    for first, second in zip(*snapshots, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert len(processes) == 14 and comparisons == 1680
    return dict(_documentation=__doc__, complete=True, foundation=identity(foundation_path), configurations=summaries,
                native_processes=14, exact_cross_build_gameplay_comparisons=comparisons, remaining_current_shortfall=123806,
                full_propellant_affordability=False, source=identity(Path(__file__)))


def main():
    """Retain the exact genuine treasury checkpoint without closing PCHM funding."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
