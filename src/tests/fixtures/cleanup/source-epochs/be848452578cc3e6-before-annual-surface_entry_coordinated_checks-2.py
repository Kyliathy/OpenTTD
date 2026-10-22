"""Bind whole due rail/Steel admission and sequential real renewal custody.

@file surface_entry_coordinated_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEP.1-2.
Actual quotes jointly fund all four replacements; healthy service, original
wagons, exact native owners and preserved launch stock remain independently bound.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import observe


def source_identity(reference, operation):
    """@param reference Tested source identity. @param operation Exact native observation.
    Assert original source bytes, including the retained invocation copy before a documentation-only edit.
    """
    if identity(ROOT / reference["path"]) == reference:
        return
    # Each invocation retained the exact executed driver before native work.
    # A later business-comment correction may use that byte-identical source;
    # executable or unrelated dependency differences receive no exception.
    assert reference["path"] == "src/tests/multilayer/acceptance/surface_entry_coordinated_checks-1.py"
    retained = operation["retained_harness"]; assert identity(ROOT / retained["path"]) == retained
    assert retained["sha256"] == reference["sha256"]


def execute(kind):
    """@param kind Pure whole admission or completed original paid custody.
    @return Exact continuous both-build native lineage and complete gameplay comparisons.
    """
    assert kind in ("plan", "custody")
    foundation_path = ROOT / "logs/surface-entry-continuation-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["renewal_due"]
    worlds, comparable, summaries, processes = [], [], [], set()
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        sequence = [("rail", "query", None), ("steel", "query", None)]
        if kind == "custody":
            sequence += [("rail", "recall", None), ("steel", "recall", None)]
            for actor in ("rail", "steel"):
                index = 1
                # Actual arrival determines the number of short native work
                # bursts; an assumed depot pose cannot authorize empty sale.
                while (ROOT / f"logs/surface-entry-coordinated-{actor}-work-{configuration}-{index}.json").exists():
                    sequence.append((actor, "work", index)); index += 1
            sequence += [("rail", "buy", None), ("steel", "buy", None)]
        rows, stages, receipts, quotes = [], [], [], {}
        for actor, phase, index in sequence:
            suffix = "" if index is None else "-" + str(index)
            path = ROOT / f"logs/surface-entry-coordinated-{actor}-{phase}-{configuration}{suffix}.json"
            value = json.loads(path.read_text(encoding="utf-8")); operation = value["operation"]
            assert value["complete"] and value["configuration"] == configuration and value["actor"] == actor and value["phase"] == phase
            assert operation["receipt"] == identity(previous)
            prior = json.loads(previous.read_text(encoding="utf-8"))["operation"]; assert operation["source"] == prior["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / operation[field]["path"]) == operation[field]
            for reference in operation["sources"]:
                source_identity(reference, operation)
            for reference in operation["native_processes"]:
                assert identity(ROOT / reference["path"]) == reference; processes.add(reference["path"])
            assert json.loads(json.dumps(observe(ROOT / operation["output"]["path"]))) == operation["final"]
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"]); receipts.append(identity(path))
            if phase == "query":
                quotes[actor] = operation
                assert chunks(ROOT / operation["before"]["path"])[1] == stages[-1]
            previous = path
        rail, = quotes["rail"]["rows"]["plan"]; steel, = quotes["steel"]["rows"]["plan"]
        extra, = quotes["rail"]["rows"]["extra"]; selected, = quotes["steel"]["rows"]["selected"]
        assert rail["road_gross"] == steel["gross"] == extra["cost"] == selected["unit"]
        assert (extra["model"], extra["capacity"]) == (selected["model"], selected["capacity"])
        assert rail["whole_gross"] == rail["gross"] + steel["gross"]
        assert rail["credit"] == max(0, (rail["whole_gross"] + 150000 - rail["cash"] + 9999) // 10000 * 10000)
        assert rail["cash"] + rail["credit"] - rail["whole_gross"] >= 150000 and not rail["unsold_value_counted"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=operation["output"], company=operation["final"]["company"],
                              whole_admission=rail, steel=selected, receipts=receipts))
        worlds.append(stages); comparable.append(rows)
    assert comparable[0] == comparable[1] and len(worlds[0]) == len(worlds[1])
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    assert len(processes) == len(worlds[0]) * 2
    return dict(_documentation=__doc__, complete=True, actual_coordinated_service_complete=kind == "custody", actual_launch_complete=False,
                foundation=identity(foundation_path), native_processes=len(processes), cross_build_gameplay_comparisons=len(worlds[0]) * 120,
                configurations=summaries, source=identity(Path(__file__)))


def main():
    """Retain one whole original admission or completed paid custody without overwriting accepted evidence."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "custody"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(execute(args.kind), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
