"""Bind whole-five road admission and sequential actual empty/purchase custody.

@file surface_entry_imminent_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQE.
Pure default-local regression grants no spending. Independent native quotes from
the same original world admit only the two freight and three regional replacements.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE, observe, lives


def checked(path, configuration):
    """@param path Exact native stage receipt. @param configuration Actual focused build.
    @return Fully retained original stage with current source/binary and sole-owner material evidence.
    """
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
    operation = value["operation"]
    for field in ("source", "before", "output", "application", "terminal", "retained_harness", "retained_wrapper"):
        assert identity(ROOT / operation[field]["path"]) == operation[field]
    for reference in operation["sources"] + operation["native_processes"]:
        assert identity(ROOT / reference["path"]) == reference
    assert json.loads(json.dumps(observe(ROOT / operation["output"]["path"]))) == operation["final"]
    return operation


def bind(kind, freight_work=1, regional_work=1):
    """@param kind Whole pure admission, actual empty custody or paid purchase boundary.
    @param freight_work Actual bounded two-freight return intervals. @param regional_work Actual bounded regional return intervals.
    @return Exact cross-build whole-five capital/native ownership and the selected genuine continuation.
    """
    assert kind in ("plan", "custody", "purchase") and 1 <= freight_work <= 4 and 1 <= regional_work <= 4
    summaries, worlds, comparable, processes = [], [], [], set()
    for configuration in ("RelWithDebInfo", "Debug"):
        original = ROOT / f"logs/surface-entry-treasury-boundary-29-continued-{configuration}-original.json"
        original_output = json.loads(original.read_text(encoding="utf-8"))["operation"]["output"]
        queries, stages, rows, receipts = {}, [], [], []
        # All three pure selectors start from the identical real original;
        # only freight/regional gross contributes to the five-service bill.
        for actor in ("local", "freight", "regional"):
            path = ROOT / f"logs/surface-entry-imminent-query-{actor}-{configuration}.json"
            operation = checked(path, configuration); assert operation["receipt"] == identity(original) and operation["source"] == original_output
            assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
            queries[actor] = operation; receipts.append(identity(path)); rows.append(operation["rows"])
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); processes.update(item["path"] for item in operation["native_processes"])
        freight, = queries["freight"]["rows"]["plan"]; regional, = queries["regional"]["rows"]["plan"]
        company = queries["regional"]["final"]["company"]; gross = freight["gross"] + regional["gross"]
        assert freight["trucks"] == 2 and regional["buses"] == 3 and freight["credit"] == regional["credit"] == 0
        assert company["cash"] - gross >= 150000 and company["debt"] <= 5000000
        previous = ROOT / f"logs/surface-entry-imminent-query-regional-{configuration}.json"; operation = queries["regional"]
        phases = []
        if kind != "plan":
            phases += [("freight", "recall")] + [("freight", f"work-{index}") for index in range(1, freight_work + 1)]
            phases += [("regional", "recall")] + [("regional", f"work-{index}") for index in range(1, regional_work + 1)]
        if kind == "purchase":
            phases += [("freight", "renew"), ("regional", "renew")]
        for actor, phase in phases:
            path = ROOT / f"logs/surface-entry-imminent-{phase}-{actor}-{configuration}.json"
            operation = checked(path, configuration); predecessor = json.loads(previous.read_text(encoding="utf-8"))["operation"]
            assert operation["receipt"] == identity(previous) and operation["source"] == predecessor["output"]
            if phase == "renew":
                current, = operation["rows"]["plan"]; assert current["gross"] == (freight["gross"] if actor == "freight" else regional["gross"])
            receipts.append(identity(path)); stages.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"])
            processes.update(item["path"] for item in operation["native_processes"]); previous = path
        source = ROOT / operation["output"]["path"]
        if kind == "custody":
            vehicles = TABLE(source, b"VEHS"); prefix = "roadveh[0].common[0]."
            # All five real bodies must be empty and physically halted in
            # their own paid depots before a single old asset may be sold.
            for service in lives(source):
                name = service["name"]
                if name not in ("Surface ore delivery", "Surface Steel delivery") and not name.startswith("Surface Regional passenger "):
                    continue
                row = vehicles[service["vehicle"]]; depot = 2984 if name.startswith("Surface Regional passenger ") else 2443
                assert row[prefix + "tile"] == depot and row[prefix + "vehstatus"] & 2 and row["roadveh[0].state"] == 254
                assert not row[prefix + "cargo.packets"] and sum(row[prefix + "cargo.action_counts"]) == 0
        company = operation["final"]["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=operation["output"], company=company,
                              global_period=29, spendable=company["cash"] + 5000000 - company["debt"], receipts=receipts,
                              whole_gross=gross, whole_credit=0, reserve=150000, service_lives=lives(source), actual_rows=operation["rows"]))
        worlds.append(stages); comparable.append(rows)
    assert comparable[0] == comparable[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, phase=kind, configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=len(worlds[0]) * 120, freight_work=freight_work, regional_work=regional_work,
                build_logs=[identity(ROOT / f"logs/surface-entry-imminent-road-build-{configuration}.log") for configuration in ("RelWithDebInfo", "Debug")],
                actual_launch_complete=False, source=identity(Path(__file__)))


def main():
    """Publish the next real whole-five admission, custody or purchase boundary without repeating work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "custody", "purchase"), required=True)
    parser.add_argument("--freight-work", type=int, choices=range(1, 5), default=1); parser.add_argument("--regional-work", type=int, choices=range(1, 5), default=1)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind, args.freight_work, args.regional_work), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
