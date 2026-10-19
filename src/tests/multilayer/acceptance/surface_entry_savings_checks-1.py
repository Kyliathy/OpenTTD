"""Bind continuing genuine savings from the actually renewed road-freight checkpoint.

@file surface_entry_savings_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEM.
Existing guarded native earnings/repayment drivers retain all original owners.
This read-only binder compares complete worlds before further annual work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import observe

LIVES = runpy.run_path(str(Path(__file__).resolve().with_name("surface_entry_capital_checks-1.py")))["lives"]


def plan():
    """@return Current actual treasury, native repayment eligibility and full useful service-life guards."""
    path = ROOT / "logs/surface-entry-road-renewal-reload-verification.json"
    foundation = json.loads(path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["actual_road_renewal_complete"]
    summaries, worlds = [], []
    for entry in foundation["configurations"]:
        source = ROOT / entry["output"]["path"]; assert identity(source) == entry["output"]
        snapshot, service = observe(source), LIVES(source); assert min(row["remaining"] for row in service) == 4237
        company = snapshot["company"]; spendable = company["cash"] + 5000000 - company["debt"]; assert spendable == 442271
        amount = (company["cash"] - 150000) // 10000 * 10000; assert amount == 50000 and company["cash"] - amount >= 150000
        summaries.append(dict(configuration=entry["configuration"], receipt=entry["receipt"], output=entry["output"], company=company,
                              spendable=spendable, target=1870000, matching_repayment=amount, reserve=150000, service_lives=service))
        worlds.append(chunks(source)[1])
    assert worlds[0].keys() == worlds[1].keys() and all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_capital_complete=False, native_commands_queried=False,
                foundation=identity(path), cross_build_gameplay_comparisons=120, configurations=summaries, source=identity(Path(__file__)))


def bind(last_period):
    """@param last_period Actual final earned global period, starting after completed period3.
    @return Continuous whole native savings lineage and full cross-build owner comparisons.
    """
    assert last_period > 3
    path = ROOT / "logs/surface-entry-savings-plan-verification.json"
    foundation = json.loads(path.read_text(encoding="utf-8")); assert foundation["complete"]
    worlds, summaries, comparable, processes = [], [], [], set()
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        stages, rows, references = [], [], []
        # Bind every actual bank and annual boundary in order; equal final
        # balances alone cannot establish the origin of useful launch capital.
        for period in range(3, last_period):
            phases = [("repaid", period), ("earned", period + 1)]
            for phase, index in phases:
                receipt = ROOT / f"logs/surface-entry-capital-{phase}-{configuration}-{index}.json"
                value = json.loads(receipt.read_text(encoding="utf-8")); operation = value["operation"]
                assert value["complete"] and value["configuration"] == configuration and operation["receipt"] == identity(previous)
                prior = json.loads(previous.read_text(encoding="utf-8"))["operation"]; assert operation["source"] == prior["output"]
                for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                    assert identity(ROOT / operation[field]["path"]) == operation[field]
                for reference in operation["sources"] + operation["native_processes"]:
                    assert identity(ROOT / reference["path"]) == reference
                processes.update(reference["path"] for reference in operation["native_processes"])
                assert json.loads(json.dumps(observe(ROOT / operation["output"]["path"]))) == operation["final"]
                stages.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"]); references.append(identity(receipt)); previous = receipt
        company = operation["final"]["company"]; service = LIVES(ROOT / operation["output"]["path"])
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=operation["output"], company=company,
                              spendable=company["cash"] + 5000000 - company["debt"], receipts=references, service_lives=service))
        worlds.append(stages); comparable.append(rows)
    assert comparable[0] == comparable[1] and len(worlds[0]) == len(worlds[1])
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    assert len(processes) == (last_period - 3) * 4
    funded = all(entry["spendable"] >= 1870000 for entry in summaries)
    renewal = any(min(row["remaining"] for row in entry["service_lives"]) <= 732 for entry in summaries)
    return dict(_documentation=__doc__, complete=True, actual_capital_complete=funded, actual_launch_complete=False, renewal_due=renewal,
                from_period=3, last_period=last_period, foundation=identity(path), native_processes=len(processes),
                cross_build_gameplay_comparisons=len(worlds[0]) * 120, configurations=summaries, source=identity(Path(__file__)))


def main():
    """Retain current eligibility or the actually completed continuation without overwriting accepted receipts."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "bind"), required=True)
    parser.add_argument("--last-period", type=int); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    value = plan() if args.kind == "plan" else bind(args.last_period)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
