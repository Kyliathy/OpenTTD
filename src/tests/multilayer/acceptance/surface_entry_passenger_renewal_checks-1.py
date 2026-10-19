"""Bind sequential native passenger renewal and ordinary replay evidence.

@file surface_entry_passenger_renewal_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEN.
Only the original continuation belongs to selected genuine funding. Ordinary
resaved branches prove deterministic ownership without contributing any cash.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import observe

DIRECTORY = Path(__file__).resolve().parent
RELOAD = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))
DRIVER = runpy.run_path(str(DIRECTORY / "surface_entry_bus_checks-1.py"))["execute"]


def bind(kind):
    """@param kind Pure current plan or actual paid custody boundary.
    @return Exact original lineage and complete cross-build gameplay comparison.
    """
    assert kind in ("plan", "custody")
    foundation_path = ROOT / "logs/surface-entry-savings-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    processes, worlds, summaries, comparable = set(), [], [], []
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]
        predecessor = ROOT / f"logs/surface-entry-savings-continued-{configuration}-original.json"
        phases = [("query", "query")]
        if kind == "custody":
            phases += [("recall", "recall")]
            index = 1
            # Actual empty return determines the number of bounded native
            # work bursts. No assumed time or planned pose authorizes renewal.
            while (ROOT / f"logs/surface-entry-passenger-renewal-work-{configuration}-{index}.json").exists():
                phases.append(("work", "work-" + str(index))); index += 1
            phases.append(("renew", "renew"))
        rows, stages = [], []
        for phase, suffix in phases:
            path = ROOT / f"logs/surface-entry-passenger-renewal-{suffix}-{configuration}.json"
            # Work invocations carry an index after the configuration so
            # repeated real progress never overwrites a prior successful run.
            if phase == "work":
                path = ROOT / f"logs/surface-entry-passenger-renewal-work-{configuration}-{suffix.removeprefix('work-')}.json"
            receipt = json.loads(path.read_text(encoding="utf-8")); operation = receipt["operation"]
            assert receipt["complete"] and receipt["configuration"] == configuration and receipt["phase"] == phase
            assert operation["receipt"] == identity(predecessor)
            prior = json.loads(predecessor.read_text(encoding="utf-8"))["operation"]
            assert operation["source"] == prior["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / operation[field]["path"]) == operation[field]
            for reference in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / reference["path"]) == reference
            processes.update(reference["path"] for reference in operation["native_processes"])
            assert json.loads(json.dumps(observe(ROOT / operation["output"]["path"]))) == operation["final"]
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"]); predecessor = path
        worlds.append(stages); comparable.append(rows)
        summaries.append(dict(configuration=configuration, receipt=identity(predecessor), output=operation["output"],
                              company=operation["final"]["company"], passenger_buses=operation["final"]["passenger_buses"], rows=rows))
    assert comparable[0] == comparable[1] and len(worlds[0]) == len(worlds[1])
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_bus_renewal_complete=kind == "custody", actual_launch_complete=False,
                foundation=identity(foundation_path), native_processes=len(processes), cross_build_gameplay_comparisons=len(worlds[0]) * 120,
                configurations=summaries, sources=[identity(Path(__file__)), identity(DIRECTORY / "surface_entry_bus_checks-1.py")])


def reload(configuration):
    """@param configuration Actual ordinary/focused build pair.
    @return Whole native restore and exact3000tick original/resaved continuations.
    """
    predecessor = ROOT / f"logs/surface-entry-passenger-renewal-renew-{configuration}.json"
    entry = json.loads(predecessor.read_text(encoding="utf-8")); assert entry["complete"]
    source = ROOT / entry["operation"]["output"]["path"]; assert identity(source) == entry["operation"]["output"]
    restored, operation = RELOAD["ORDINARY"](configuration, source, "surface-passenger-renewal-restore-" + configuration.lower())
    RELOAD["REVISION"](source, restored); pause = RELOAD["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored)
    restore_path = ROOT / f"logs/surface-entry-passenger-renewal-restored-{configuration}.json"
    RELOAD["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", operation=operation, original_pause=pause))
    branches, rows, receipts = [], [], []
    # Exact native replay compares the selected original with an ordinary
    # resave. Only the first continuation may fund future launch work.
    for path, suffix in ((predecessor, "original"), (restore_path, "ordinary")):
        continuation = DRIVER(configuration, "earn", path)
        receipt = ROOT / f"logs/surface-entry-passenger-renewal-continued-{configuration}-{suffix}.json"
        RELOAD["publish"](receipt, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="earn", operation=continuation))
        receipts.append(identity(receipt)); branches.append(chunks(ROOT / continuation["output"]["path"])[1]); rows.append(continuation["rows"])
    assert rows[0] == rows[1] and branches[0].keys() == branches[1].keys()
    assert all(branches[0][key] == branches[1][key] for key in branches[0] if key != b"GLOG")
    selected = json.loads((ROOT / receipts[0]["path"]).read_text(encoding="utf-8"))["operation"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, restore=identity(restore_path), continuations=receipts,
                output=selected["output"], company=selected["final"]["company"], passenger_buses=selected["final"]["passenger_buses"], rows=rows[0],
                native_processes=3, exact_native_replay_gameplay_comparisons=120)


def main():
    """Retain pure admission, paid custody or one focused ordinary restoration without overwriting receipts."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "custody", "reload"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    result = reload(args.configuration) if args.kind == "reload" else bind(args.kind)
    RELOAD["publish"](output, result); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
