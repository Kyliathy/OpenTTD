"""Bind exact original ground payload/fuel admission before the first flight.

@file surface_entry_gateway_payload_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQN.
Actual generation-qualified paid bodies and native packet compartments retain
all custody; an admitted flight quote does not begin or pay for a mission.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_assembly_ledger import observe as assembly
from surface_gateway_payload_ledger import observe

FOUNDATION = ROOT / "logs/surface-entry-gateway-assembly-reload-verification.json"
STEM = "surface-entry-gateway-payload"


def load(path):
    """@param path Complete immutable accepted receipt. @return Accepted object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def bind(kind):
    """@param kind Completed pure plan, actual loading or fresh first-flight quote. @return Exact both-build source/custody/payment/native equality."""
    phases = ["query"]
    if kind != "plan":
        phases.append("payload")
    if kind == "ready":
        phases.append("flight-query")
    foundation = load(FOUNDATION); summaries, worlds, rows, processes = [], [], [], set()
    # Compare independent original paid worlds, never pooling copied cargo or
    # loan capacity. The previous strict assembly snapshot must remain exact
    # before explicit payload permission changes native custody.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        baseline = load(previous)["operation"]; assert json.loads(json.dumps(assembly(ROOT / baseline["output"]["path"]))) == baseline["final"]
        stages, comparable, references = [], [], []
        for phase in phases:
            path = ROOT / f"logs/{STEM}-{phase}-{configuration}.json"; value = load(path); operation = value["operation"]
            assert value["configuration"] == configuration and operation["receipt"] == identity(previous)
            assert operation["source"] == load(previous)["operation"]["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                reference = operation[field]; assert identity(ROOT / reference["path"]) == reference
            for reference in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / reference["path"]) == reference
            for reference in operation["native_processes"]:
                report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
                processes.add(reference["path"])
            output = ROOT / operation["output"]["path"]; snapshot = observe(output)
            assert json.loads(json.dumps(snapshot)) == operation["final"]
            stages.append(chunks(output)[1]); comparable.append(operation["rows"]); references.append(identity(path)); previous = path
        if kind != "plan":
            constructor, = [craft for craft in snapshot["paid_craft"] if craft["model"] == 20]
            rocket, = [craft for craft in snapshot["paid_craft"] if craft["model"] == 14]
            assert constructor["placement"] == 4 and rocket["placement"] == 2
            assert sum(packet["count"] for packet in snapshot["cohort"] if tuple(packet["owner"]) == ("vehicle", constructor["vehicle"])) == 16
            assert sum(packet["count"] for packet in snapshot["cohort"] if tuple(packet["owner"]) == ("supply", rocket["vehicle"], 0)) == 30
            # Ground cargo/carry permission cannot weaken the older empty-hold
            # assembly consumer. It must continue to reject this loaded state.
            try:
                assembly(output)
            except AssertionError:
                pass
            else:
                raise AssertionError("Original assembly contract admitted loaded/carried payload")
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=operation["output"], company=company, global_period=40,
                              spendable=company["cash"] + 5000000 - company["debt"], totals=snapshot["totals"], paid_craft=snapshot["paid_craft"],
                              receipts=references, actual_rows=operation["rows"], service_lives=snapshot["service_lives"]))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_ground_payload_complete=kind != "plan", actual_flight_admitted=kind == "ready",
                actual_launch_complete=False, foundation=identity(FOUNDATION), stage=kind, configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=len(phases) * 120, source=identity(Path(__file__)))


def main():
    """Retain the completed current original ground-payload stage before any flight mutation."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "loaded", "ready"), required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
