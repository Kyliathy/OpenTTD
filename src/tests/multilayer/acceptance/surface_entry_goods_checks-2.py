"""Bind current Goods quotes and genuine selected original collection receipts.

@file surface_entry_goods_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEF.
Read-only artifact validation supplies no simulation work, stock or capital.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_propellant_ledger import observe


def execute(stage, bursts):
    """@param stage Pure complete plan or actual collected stock.
    @param bursts Exact number of selected bounded collection invocations.
    @return Verified both-build native process, original lineage and complete gameplay comparisons.
    """
    foundation_path = ROOT / "logs/surface-entry-freight-refresh-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    retained_path = ROOT / "test-cache/surface-entry/goods-initial-products/retained.json"
    retained = json.loads(retained_path.read_text(encoding="utf-8"))
    aliases = {(row["original"]["path"], row["original"]["sha256"]): row["retained"] for row in retained["files"]}
    processes, builds, summaries, verified = set(), [], [], set()

    def verify(reference):
        """@param reference Immutable original artifact or exact currently tested source. Verify its bytes once."""
        key = reference["path"], reference["sha256"]
        if key not in verified:
            path = ROOT / reference["path"]
            if identity(path) != reference:
                alias = aliases[key]; assert alias["sha256"] == reference["sha256"]
                assert identity(ROOT / alias["path"]) == alias
            verified.add(key)

    for entry in foundation["configurations"]:
        configuration = entry["configuration"]
        previous = ROOT / f"logs/surface-entry-freight-refresh-continued-{configuration}-original.json"
        predecessor = json.loads(previous.read_text(encoding="utf-8"))["operation"]
        assert predecessor["output"] == entry["output"]
        physical, rows, receipts, ticks, expense, income, infrastructure = [], [], [], 0, 0, 0, 0
        selections = [("query", f"logs/surface-entry-goods-query-{configuration}.json")]
        if stage in ("access", "collected"):
            selections += [("buy", f"logs/surface-entry-goods-bought-{configuration}.json")]
            selections += [("collect", f"logs/surface-entry-goods-collected-{configuration}-1.json")]
            selections += [("access-query", f"logs/surface-entry-goods-access-query-{configuration}.json")]
        if stage == "collected":
            selections += [("access", f"logs/surface-entry-goods-access-bought-{configuration}.json")]
            selections += [("collect", f"logs/surface-entry-goods-collected-{configuration}-{index}.json") for index in range(2, bursts + 1)]
        # Only each selected predecessor supplies the next native state.
        # Alternative builds and replay branches contribute no money or stock.
        for phase, name in selections:
            path = ROOT / name; value = json.loads(path.read_text(encoding="utf-8")); operation = value["operation"]
            assert value["complete"] and value["phase"] == phase and value["configuration"] == configuration
            assert operation["receipt"] == identity(previous) and operation["source"] == predecessor["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                verify(operation[field])
            for reference in operation["sources"]:
                verify(reference)
            for reference in operation["native_processes"]:
                verify(reference); report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
                processes.add(reference["path"])
            assert observe(ROOT / operation["before"]["path"]) == operation["initial"]
            assert observe(ROOT / operation["output"]["path"]) == operation["final"]
            if phase == "collect":
                work, = operation["rows"]["earned"]
                ticks += work["earned"]; expense += work["net_expense"]; income += work["income"]
            if phase == "buy":
                purchase, = operation["rows"]["plan"]
            if phase == "access":
                access_plan, = operation["rows"]["access_plan"]; infrastructure += access_plan["gross"]
            physical.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"]); receipts.append(identity(path))
            previous, predecessor = path, operation
        if stage == "collected":
            goods = predecessor["final"]["additional_goods"]
            assert sum(row["held"] for row in goods) == purchase["useful"] >= 34
            assert all(row["held"] == row["capacity"] and row["stopped"] and row["depot"] and row["tile"] == 2443 for row in goods)
            assert predecessor["final"]["company"]["cash"] == entry["company"]["cash"] + purchase["credit"] - purchase["gross"] - infrastructure - expense
            assert predecessor["final"]["company"]["debt"] == entry["company"]["debt"] + purchase["credit"]
        builds.append((physical, rows))
        summaries.append(dict(configuration=configuration, receipts=receipts, receipt=identity(previous), output=predecessor["output"],
                              company=predecessor["final"]["company"], additional_goods=predecessor["final"]["additional_goods"],
                              earned_ticks=ticks, actual_income=income, net_work_expense=expense, actual_infrastructure_cost=infrastructure))
    assert builds[0][1] == builds[1][1]
    for first, second in zip(builds[0][0], builds[1][0], strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_collection_complete=stage == "collected", actual_launch_complete=False,
                foundation=identity(foundation_path), native_processes=len(processes), cross_build_gameplay_comparisons=len(builds[0][0]) * 120,
                configurations=summaries, retained_products=identity(retained_path), source=identity(Path(__file__)))


def main():
    """Rebuild only the selected useful Goods boundary from immutable receipts."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=("plan", "access", "collected"), required=True)
    parser.add_argument("--bursts", type=int, default=0); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    assert args.bursts >= 0 and (args.stage != "collected" or args.bursts >= 2)
    output.write_text(json.dumps(execute(args.stage, args.bursts), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
