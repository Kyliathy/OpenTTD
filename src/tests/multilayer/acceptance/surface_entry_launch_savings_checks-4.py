"""Bind the original native initial-capital lineage, excluding discarded work.

@file surface_entry_launch_savings_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIED.2.
Every accepted boundary names its immediate immutable predecessor and actual
native process. Retained source versions qualify earlier compiled products;
failed unsaved attempts and ordinary replay branches supply no useful capital.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_propellant_ledger import TABLE, TRAIN, observe

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]


def execute(last_period):
    """@param last_period Final actual selected annual receipt. @return Verified continuous payments, debt and physical ownership in both builds."""
    foundation_path = ROOT / "logs/surface-entry-launch-savings-plan-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    aliases = {}
    for name in ("launch-savings-annual-products", "launch-site-reload-products", "launch-savings-capital-products"):
        path = ROOT / "test-cache/surface-entry" / name / "retained.json"
        if not path.exists():
            continue
        for row in json.loads(path.read_text(encoding="utf-8"))["files"]:
            aliases[(row["original"]["path"], row["original"]["sha256"])] = row["retained"]
    verified = {}

    def verify(reference):
        """@param reference Actual current or explicitly retained original identity. @return Validated exact source/artifact reference."""
        key = (reference["path"], reference["sha256"])
        if key in verified:
            return reference
        target = ROOT / reference["path"]
        # A current file qualifies only while it still matches the original
        # receipt; immutable compiled-source copies own historical evidence.
        if target.is_file() and identity(target) == reference:
            verified[key] = True
            return reference
        retained = aliases[key]; assert identity(ROOT / retained["path"]) == retained
        verified[key] = retained
        return reference

    build_rows, summaries, processes = [], [], set()
    for configuration in ("RelWithDebInfo", "Debug"):
        previous = ROOT / f"logs/surface-entry-launch-savings-query-{configuration}.json"
        predecessor = json.loads(previous.read_text(encoding="utf-8"))["operation"]
        initial_cash = predecessor["final"]["company"]["cash"]; initial_debt = predecessor["final"]["company"]["debt"]
        total_expense = total_income = total_repaid = total_ticks = total_profit = 0
        delivered, receipts, physical, rows, continuations = {}, [], [], [], []

        def accept(path, phase):
            """@param path Exact original native boundary. @param phase Expected bank/work action. @return Complete actual boundary after source/process validation."""
            value = json.loads(path.read_text(encoding="utf-8"))
            assert value["complete"] and value["configuration"] == configuration and value["phase"] == phase
            operation = value["operation"]
            assert operation["receipt"] == identity(previous) and operation["source"] == predecessor["output"]
            for name in ("source", "before", "output", "application", "terminal", "retained_harness"):
                verify(operation[name])
            for reference in operation["sources"]:
                verify(reference)
            for reference in operation["native_processes"]:
                verify(reference); report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
                assert report["configuration"] == configuration; processes.add(reference["path"])
            assert observe(ROOT / operation["before"]["path"]) == operation["initial"]
            assert observe(ROOT / operation["output"]["path"]) == operation["final"]
            native = chunks(ROOT / operation["output"]["path"])[1]
            assert native[b"MAPT"][3 + 2826] >> 4 == 6 and (native[b"MAPO"][3 + 2826] >> 5) & 3 == 1
            physical.append(native); rows.append(operation["rows"]); receipts.append(identity(path))
            return operation

        # Repayments debit actual cash/debt together. Only the following
        # original earned-work output advances usable capital; every boundary
        # must name the immediately preceding selected save and receipt.
        for period in range(1, last_period + 1):
            if predecessor["final"]["company"]["cash"] >= 160000:
                path = ROOT / f"logs/surface-entry-launch-savings-repaid-{configuration}-{period - 1}.json"
                operation = accept(path, "repay"); paid, = operation["rows"]["repayment"]
                total_repaid += paid["amount"]; previous = path; predecessor = operation
            path = ROOT / f"logs/surface-entry-launch-savings-earned-{configuration}-{period}.json"
            operation = accept(path, "earn"); earned, = operation["rows"]["earned"]; profit, = operation["rows"]["profit"]
            assert earned["earned"] == 27000
            total_ticks += earned["earned"]; total_income += earned["income"]; total_expense += earned["net_expense"]; total_profit += profit["amount"]
            for row in operation["rows"]["delivered"]:
                delivered[row["label"]] = delivered.get(row["label"], 0) + row["amount"]
            # Native source ages, rather than retirement of a purchase model,
            # establish that every actually run interval fits engine life.
            for vehicle in TABLE(ROOT / operation["before"]["path"], b"VEHS").values():
                if vehicle["type"] != 0:
                    continue
                if bytes.fromhex(vehicle[TRAIN + "name"]).decode("utf-8").startswith("Surface Income "):
                    assert vehicle[TRAIN + "age"] + 366 < vehicle[TRAIN + "max_age"]
            terminal = (ROOT / operation["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
            continuations.extend(FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_SAVINGS_ENDGAME "))
            previous = path; predecessor = operation
        company = predecessor["final"]["company"]
        assert company["cash"] == initial_cash - total_expense - total_repaid and company["debt"] == initial_debt - total_repaid
        assert total_ticks == last_period * 27000 and company["cash"] + 5000000 - company["debt"] >= 325000
        assert len(continuations) == 1 and continuations[0]["year"] == continuations[0]["ending_year"] + 1
        assert continuations[0]["windows"] == 2 and continuations[0]["native_close"]
        build_rows.append((physical, rows))
        summaries.append(dict(configuration=configuration, receipts=receipts, last_receipt=identity(previous), output=predecessor["output"], company=company,
                              earned_ticks=total_ticks, actual_income=total_income, net_earned=-total_expense, matching_repaid=total_repaid,
                              bus_profit_fractional=total_profit, bus_profit_denominator=256, delivered=delivered, native_endgame=continuations[0]))
    assert build_rows[0][1] == build_rows[1][1]
    for first, second in zip(build_rows[0][0], build_rows[1][0], strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, initial_milestone_reached=True, complete_launch=False, foundation=identity(foundation_path),
                native_processes=len(processes), cross_build_gameplay_comparisons=len(build_rows[0][0]) * 120,
                configurations=summaries, sources=[identity(Path(__file__))], retained_aliases=[value for value in verified.values() if isinstance(value, dict)])


def main():
    """Rebuild the focused financial proof from original receipts without running another native work/payment process."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--last-period", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert args.last_period > 0 and output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(execute(args.last_period), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
