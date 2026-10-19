"""Bind an explicit current treasury foundation and genuine subsequent income.

@file surface_entry_treasury_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQ.
Native books, original cargo and actual service histories remain authoritative.
An explicit foundation/start period prevents stale prior-cohort assumptions.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import TABLE, observe
from surface_propellant_ledger import FACTORY

LIVES = runpy.run_path(str(Path(__file__).resolve().with_name("surface_entry_capital_checks-1.py")))["lives"]


def towns(path):
    """@param path Complete original save. @return Actual passenger production/transport in twelve completed native months."""
    labels = FACTORY["observe_factory"](path)["cargo_labels"]; result = []
    for key, row in TABLE(path, b"CITY").items():
        matches = [index for index in range(row["supplied.count"]) if labels[row[f"supplied[{index}].cargo"]] == "PASS"]
        assert len(matches) == 1; prefix = f"supplied[{matches[0]}]."; assert row[prefix + "history.count"] >= 13
        # Native THIS_MONTH=0is incomplete; LAST_MONTH=1starts the completed
        # monthly history. Forecast demand or future fares cannot fund assets.
        produced = sum(row[prefix + f"history[{month}].production"] for month in range(1, 13))
        transported = sum(row[prefix + f"history[{month}].transported"] for month in range(1, 13))
        assert 0 <= transported <= produced
        result.append(dict(town=key, tile=row["xy"], passenger_produced=produced, passenger_transported=transported,
                           passenger_untransported=produced - transported, completed_months=12))
    return result


def plan(foundation_path, start_period, products_path, observe_fn=observe, lives_fn=LIVES):
    """@param foundation_path Exact accepted current original-service handoff.
    @param start_period Completed native annual index, including explicit service proof years.
    @param products_path Immutable accepted current compiled/source products.
    @param observe_fn Explicit current sole-owner material observer. @param lives_fn Current useful-service age observer.
    @return Actual funds, matching repayment and measured current demand; no native pure command query is claimed.
    """
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    products = json.loads(products_path.read_text(encoding="utf-8"))
    for product in products["products"]:
        assert identity(ROOT / product["source"]["path"]) == product["source"]
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
    worlds, summaries = [], []
    for entry in foundation["configurations"]:
        source = ROOT / entry["output"]["path"]; assert identity(source) == entry["output"]
        snapshot = observe_fn(source); company = snapshot["company"]; service = lives_fn(source)
        assert company == entry["company"] and min(row["remaining"] for row in service) > 732
        assert entry.get("global_period", start_period) == start_period
        amount = min(company["debt"], max(0, (company["cash"] - 150000) // 10000 * 10000))
        summaries.append(dict(configuration=entry["configuration"], receipt=entry["receipt"], output=entry["output"], company=company,
                              spendable=company["cash"] + 5000000 - company["debt"], target=1870000, reserve=150000,
                              matching_repayment=amount, service_lives=service, towns=towns(source)))
        worlds.append(chunks(source)[1])
    assert worlds[0].keys() == worlds[1].keys() and all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_capital_complete=False, native_commands_queried=False,
                foundation=identity(foundation_path), products=identity(products_path), start_period=start_period,
                cross_build_gameplay_comparisons=120, configurations=summaries, source=identity(Path(__file__)))


def bind(plan_path, last_period, observe_fn=observe, lives_fn=LIVES):
    """@param plan_path Explicit current foundation/annual-index contract. @param last_period Last actually earned period.
    @param observe_fn Explicit current material observer. @param lives_fn Current useful-service age observer.
    @return Continuous native bank/income lineage, including optional repayments decided only by actual available cash.
    """
    foundation = json.loads(plan_path.read_text(encoding="utf-8")); assert foundation["complete"]
    start = foundation["start_period"]; assert last_period > start
    worlds, comparable, summaries, processes = [], [], [], set()
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        stages, rows, references = [], [], []
        for period in range(start, last_period):
            prior = json.loads(previous.read_text(encoding="utf-8"))["operation"]; company = prior["final"]["company"]
            # A repayment exists only when actual prior cash can preserve
            # the reserve. Never invent a bank boundary or count future fares.
            phases = [("repaid", period)] if company["cash"] >= 160000 else []
            phases.append(("earned", period + 1))
            for phase, index in phases:
                path = ROOT / f"logs/surface-entry-capital-{phase}-{configuration}-{index}.json"
                value = json.loads(path.read_text(encoding="utf-8")); operation = value["operation"]
                assert value["complete"] and value["configuration"] == configuration and operation["receipt"] == identity(previous)
                prior = json.loads(previous.read_text(encoding="utf-8"))["operation"]; assert operation["source"] == prior["output"]
                for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                    assert identity(ROOT / operation[field]["path"]) == operation[field]
                for reference in operation["sources"] + operation["native_processes"]:
                    assert identity(ROOT / reference["path"]) == reference
                processes.update(reference["path"] for reference in operation["native_processes"])
                assert json.loads(json.dumps(observe_fn(ROOT / operation["output"]["path"]))) == operation["final"]
                stages.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"]); references.append(identity(path)); previous = path
        company = operation["final"]["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=operation["output"], company=company,
                              spendable=company["cash"] + 5000000 - company["debt"], receipts=references, service_lives=lives_fn(ROOT / operation["output"]["path"])))
        worlds.append(stages); comparable.append(rows)
    assert comparable[0] == comparable[1] and len(worlds[0]) == len(worlds[1])
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    assert len(processes) == len(worlds[0]) * 2
    return dict(_documentation=__doc__, complete=True, actual_capital_complete=all(entry["spendable"] >= 1870000 for entry in summaries), actual_launch_complete=False,
                foundation=identity(plan_path), start_period=start, last_period=last_period, native_processes=len(processes),
                cross_build_gameplay_comparisons=len(worlds[0]) * 120, configurations=summaries, source=identity(Path(__file__)))


def main():
    """Publish an explicit current treasury/demand contract or a completed native annual boundary."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "bind"), required=True)
    parser.add_argument("--foundation", type=Path); parser.add_argument("--products", type=Path); parser.add_argument("--start-period", type=int)
    parser.add_argument("--plan", type=Path); parser.add_argument("--last-period", type=int); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--service", choices=("original", "regional"), default="original")
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    callbacks = {}
    if args.service == "regional":
        from surface_regional_ledger import observe, lives
        callbacks = dict(observe_fn=observe, lives_fn=lives)
    value = plan((ROOT / args.foundation).resolve(), args.start_period, (ROOT / args.products).resolve(), **callbacks) if args.kind == "plan" else bind((ROOT / args.plan).resolve(), args.last_period, **callbacks)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
