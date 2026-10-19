"""Bind sequential actual launch-supply custody before every dependent native action.

@file surface_entry_gateway_supply_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQL.
Only selected original128unit transport and native reverse handling count.
All gameplay families must agree across independent builds before extending.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_material_ledger import observe, REQUIRED

FOUNDATION = ROOT / "logs/surface-entry-paid-center-reload-verification.json"
STEM = "surface-entry-gateway-supply"


def load(path):
    """@param path Existing immutable successful receipt. @return Its complete accepted object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def bind(kind, work_count=1):
    """@param kind Completed pure, empty custody, handling query/reservation or stored boundary.
    @param work_count Actual sequential bounded shipment intervals, never supplied work.
    @return Exact source/process/material lineage, physical custody and cross-build gameplay equality.
    """
    phases = ["query"]
    if kind != "plan":
        phases += ["dispatch"] + [f"ship-work-{index}" for index in range(1, work_count + 1)]
    if kind in ("handling-plan", "handling", "stored"):
        phases.append("handling-query")
    if kind in ("handling", "stored"):
        phases.append("handling")
    if kind == "stored":
        phases.append("handle-work-1")
    foundation = load(FOUNDATION); summaries, worlds, rows, processes = [], [], [], set()
    # Both independently linked builds must follow the exact original paid
    # facility chain, preserving every packet rather than importing fixture
    # cargo or treating virtual transfer income as a second physical owner.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
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
            snapshot = observe(ROOT / operation["output"]["path"])
            assert json.loads(json.dumps(snapshot)) == operation["final"] and snapshot["totals"] == REQUIRED
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); comparable.append(operation["rows"])
            references.append(identity(path)); previous = path
        if kind != "plan":
            assert all(row["home"] and row["tile"] == 2443 and row["held"] == 0 for row in snapshot["shipments"].values())
        if kind in ("custody", "handling-plan"):
            assert all(tuple(packet["owner"]) == ("station", 0) for packet in snapshot["cohort"])
        if kind == "handling":
            assert len(snapshot["handling"]["jobs"]) == 3
            assert all(packet["owner"][0] == "handling" for packet in snapshot["cohort"])
        if kind == "stored":
            assert not snapshot["handling"]["jobs"] and snapshot["handling"]["remainder"] == 0
            assert all(tuple(packet["owner"]) == ("station", snapshot["facility"]["center"]) for packet in snapshot["cohort"])
            earned, = operation["rows"]["earned"]; assert earned["reached"] and earned["earned"] == 16
            assert sum(row["amount"] for row in operation["rows"]["expenses"] if row["type"] == 12) >= 103
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=operation["output"], company=company, global_period=40,
                              spendable=company["cash"] + 5000000 - company["debt"], totals=snapshot["totals"], facility=snapshot["facility"],
                              receipts=references, actual_rows=operation["rows"], service_lives=snapshot["service_lives"]))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_material_delivery_complete=kind == "stored", actual_launch_complete=False,
                foundation=identity(FOUNDATION), stage=kind, work_count=work_count, configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=len(phases) * 120, source=identity(Path(__file__)))


def main():
    """Retain only the currently completed bounded supply stage before further native action."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", choices=("plan", "custody", "handling-plan", "handling", "stored"), required=True)
    parser.add_argument("--work-count", type=int, choices=range(1, 5), default=1); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind, args.work_count), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
