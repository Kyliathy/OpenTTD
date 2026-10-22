"""Bind each sequential local-bus renewal prerequisite in both genuine regional worlds.

@file surface_entry_regional_renewal_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQC.
All actual phases follow their selected predecessor, preserve eight healthy
services and compare every gameplay family before the next paid/native action.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import observe, lives


def bind(kind, work_count=1):
    """@param kind Current pure plan, actual empty custody or paid purchase boundary.
    @param work_count Actual sequential bounded empty-return intervals, never forced work.
    @return Exact both-build lineage, selected funds and useful service ages.
    """
    assert kind in ("query", "custody", "purchase") and 1 <= work_count <= 4
    phases = ["query"]
    if kind != "query":
        phases += ["recall"] + [f"work-{index}" for index in range(1, work_count + 1)]
    if kind == "purchase":
        phases.append("renew")
    summaries, worlds, comparable, processes = [], [], [], set()
    for configuration in ("RelWithDebInfo", "Debug"):
        previous = ROOT / f"logs/surface-entry-capital-boundary-26-continued-{configuration}-original.json"
        receipts, stages, rows = [], [], []
        for phase in phases:
            path = ROOT / f"logs/surface-entry-local-renewal-{phase}-{configuration}.json"
            value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
            current = value["operation"]; predecessor = json.loads(previous.read_text(encoding="utf-8"))["operation"]
            assert current["receipt"] == identity(previous) and current["source"] == predecessor["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness", "retained_wrapper"):
                assert identity(ROOT / current[field]["path"]) == current[field]
            for reference in current["sources"] + current["native_processes"]:
                assert identity(ROOT / reference["path"]) == reference
            source = ROOT / current["output"]["path"]
            assert json.loads(json.dumps(observe(source))) == current["final"]
            stages.append(chunks(source)[1]); rows.append(current["rows"]); receipts.append(identity(path))
            processes.update(reference["path"] for reference in current["native_processes"]); previous = path
        if kind == "custody":
            assert all(row["home"] and row["held"] == 0 and row["tile"] == 2984 for row in current["rows"]["custody"])
        company = current["final"]["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=current["output"], company=company,
                              global_period=26, spendable=company["cash"] + 5000000 - company["debt"], receipts=receipts,
                              service_lives=lives(ROOT / current["output"]["path"]), actual_rows=current["rows"]))
        worlds.append(stages); comparable.append(rows)
    assert comparable[0] == comparable[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, phase=kind, configurations=summaries, work_count=work_count,
                native_processes=len(processes), cross_build_gameplay_comparisons=len(phases) * 120,
                actual_launch_complete=False, source=identity(Path(__file__)))


def main():
    """Publish only the completed next local-renewal boundary, without repeating actual work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("query", "custody", "purchase"), required=True)
    parser.add_argument("--work-count", type=int, choices=range(1, 5), default=1); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind, args.work_count), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
