"""Bind original three-rail admission, empty custody and paid wagon-preserving renewal.

@file surface_entry_rail_only_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQG.
Every stage follows the selected genuine predecessor. Current native matching
credit is recomputed before purchase; future refunds and fares fund no admission.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE, observe, lives


def bind(kind, work_count=1):
    """@param kind Current pure plan, genuine empty custody or paid head-only purchase.
    @param work_count Actual sequential bounded native depot-return intervals.
    @return Exact cross-build lineage, current finances and every preserved useful service/material owner.
    """
    assert kind in ("plan", "custody", "purchase") and 1 <= work_count <= 5
    phases = ["query"]
    if kind != "plan":
        phases += ["recall"] + [f"work-{index}" for index in range(1, work_count + 1)]
    if kind == "purchase":
        phases.append("buy")
    summaries, worlds, comparable, processes = [], [], [], set()
    for configuration in ("RelWithDebInfo", "Debug"):
        previous = ROOT / f"logs/surface-entry-treasury-boundary-37-continued-{configuration}-original.json"
        receipts, stages, rows = [], [], []; quoted = None
        for phase in phases:
            path = ROOT / f"logs/surface-entry-rail-only-{phase}-{configuration}.json"
            value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
            operation = value["operation"]; predecessor = json.loads(previous.read_text(encoding="utf-8"))["operation"]
            assert operation["receipt"] == identity(previous) and operation["source"] == predecessor["output"] and not operation["with_steel"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness", "retained_wrapper"):
                assert identity(ROOT / operation[field]["path"]) == operation[field]
            for reference in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / reference["path"]) == reference
            source = ROOT / operation["output"]["path"]
            assert json.loads(json.dumps(observe(source))) == operation["final"]
            if phase == "query":
                quoted, = operation["rows"]["plan"]
                assert quoted["heads"] == 3 and quoted["road_gross"] == 0 and quoted["whole_gross"] == quoted["gross"]
            elif phase == "buy":
                fresh, = operation["rows"]["plan"]; paid, = operation["rows"]["purchase"]
                assert fresh["gross"] == quoted["gross"] and fresh["road_gross"] == 0
                required = max(0, (fresh["gross"] + 150000 - paid["cash_before"] + 9999) // 10000 * 10000)
                assert paid["credit"] == fresh["credit"] == required
            receipts.append(identity(path)); stages.append(chunks(source)[1]); rows.append(operation["rows"])
            processes.update(item["path"] for item in operation["native_processes"]); previous = path
        if kind == "custody":
            assert all(row["stopped"] and row["tile"] == row["depot"] for row in operation["rows"]["arrival"])
            vehicles = TABLE(source, b"VEHS"); prefix = "train[0].common[0]."
            # Every original wagon and head must actually be empty, with all
            # three native depot arrivals complete before head-only sale.
            for train in operation["final"]["trains"]:
                for key in train["chain"]:
                    assert not vehicles[key][prefix + "cargo.packets"] and sum(vehicles[key][prefix + "cargo.action_counts"]) == 0
        company = operation["final"]["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=operation["output"], company=company,
                              global_period=37, spendable=company["cash"] + 5000000 - company["debt"], receipts=receipts,
                              service_lives=lives(source), actual_rows=operation["rows"]))
        worlds.append(stages); comparable.append(rows)
    assert comparable[0] == comparable[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, phase=kind, configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=len(phases) * 120, work_count=work_count,
                actual_launch_complete=False, source=identity(Path(__file__)))


def main():
    """Publish only the completed real next rail-only boundary, preserving all successful work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "custody", "purchase"), required=True)
    parser.add_argument("--work-count", type=int, choices=range(1, 6), default=1); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind, args.work_count), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
