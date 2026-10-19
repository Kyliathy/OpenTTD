"""Bind the pure complete current finite-Salt and chemical-refill admission.

@file surface_entry_resupply_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEG.1.
Every cost and physical route comes from original native current-world queries.
No financial or material owner changes during this boundary.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_resupply_ledger import observe


def execute():
    """@return Exact both-build source lineage, unchanged native owners and complete current admission."""
    foundation_path = ROOT / "logs/surface-entry-goods-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["actual_collection_complete"]
    processes, builds, rows, summaries = set(), [], [], []
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; path = ROOT / f"logs/surface-entry-resupply-query-{configuration}.json"
        receipt = json.loads(path.read_text(encoding="utf-8")); assert receipt["complete"] and receipt["configuration"] == configuration and receipt["phase"] == "query"
        operation = receipt["operation"]; assert operation["source"] == entry["output"]
        assert operation["receipt"] == identity(ROOT / f"logs/surface-entry-goods-continued-{configuration}-original.json")
        for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
            assert identity(ROOT / operation[field]["path"]) == operation[field]
        for reference in operation["sources"]:
            assert identity(ROOT / reference["path"]) == reference
        for reference in operation["native_processes"]:
            assert identity(ROOT / reference["path"]) == reference
            report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
            assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
            processes.add(reference["path"])
        first, last = [chunks(ROOT / operation[field]["path"])[1] for field in ("before", "output")]
        assert first == last and len(first) == 121 and operation["unchanged_families"] == 121
        # Native history counter kinds are integer keys in memory; the
        # retained JSON receipt preserves them as strings. Compare the same
        # serialized representation without discarding any counter or owner.
        assert json.loads(json.dumps(observe(ROOT / operation["output"]["path"]))) == operation["initial"] == operation["final"]
        builds.append(last); rows.append(operation["rows"])
        summaries.append(dict(configuration=configuration, receipt=identity(path), output=operation["output"], plan=operation["rows"]["plan"],
                              rail=operation["rows"]["rail"], crossings=operation["rows"]["crossings"], company=operation["final"]["company"]))
    assert rows[0] == rows[1] and builds[0].keys() == builds[1].keys()
    assert all(builds[0][key] == builds[1][key] for key in builds[0] if key != b"GLOG")
    assert len(processes) == 2
    return dict(_documentation=__doc__, complete=True, actual_refill_complete=False, actual_launch_complete=False,
                foundation=identity(foundation_path), native_processes=2, cross_build_gameplay_comparisons=120,
                unchanged_families_per_query=121, configurations=summaries, source=identity(Path(__file__)))


def main():
    """Retain the admitted whole actual plan before any original purchase or transport work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(execute(), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
