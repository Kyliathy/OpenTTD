"""Bind the original factory acquisition chain and its exact current products.

@file surface_entry_components_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CF.
Pure query acceptance precedes dispatch; genuine collection and ordinary replay
are separately accepted boundaries. No copied branch supplies original cargo.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_components_ledger import observe
from surface_gateway_replay import load, validate

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-expansion-verification.json"
CHANGED = {"src/tests/surface_entry_gateway.cpp"} | {"src/tests/multilayer/acceptance/" + name for name in
          ("surface_gateway_material_ledger.py", "surface_gateway_resupply_ledger.py", "surface_gateway_construction_ledger.py")}
VERSIONS = [ROOT / ("test-cache/surface-entry/components-source-versions/" + name) for name in
            ("before-current-work-funding.json", "before-crossing-clear-restore.json", "before-short-work-admission.json", "before-monthly-interest-budget.json")]


def verify_reference(reference):
    """@param reference Exact successful native source/application identity. Verify current bytes or its explicitly retained pre-funding version."""
    path = ROOT / reference["path"]
    if identity(path) == reference:
        return
    products = [row for manifest in VERSIONS for row in json.loads(manifest.read_text(encoding="utf-8"))["products"]]
    exact, = {row["retained"]: row for row in products if row["source"] == reference["path"] and row["sha256"] == reference["sha256"]}.values()
    retained = identity(ROOT / exact["retained"])
    assert {key: value for key, value in retained.items() if key != "path"} == {key: value for key, value in reference.items() if key != "path"}


def bind(kind, count, traffic_count, retain):
    """@param kind Pure query, actual collection or ordinary-restored final boundary. @param count Actual sequential bounded work bursts.
    @param traffic_count Actual sequential genuine train-arrival bursts. @param retain Whether to retain exact tested products at this boundary. @return Original source links, both-build equality and actual physical acquisition.
    """
    foundation = load(FOUNDATION); previous_path = ROOT / foundation["products"]["path"]
    assert identity(previous_path) == foundation["products"]; previous = json.loads(previous_path.read_text(encoding="utf-8"))
    replaced = CHANGED | {f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")}
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]
    names = ["query-current"]
    if kind != "query":
        assert traffic_count > 0
        names += ["recall"] + [f"traffic-work-{index:02d}" for index in range(1, traffic_count + 1)]
        if kind != "protected":
            assert count > 0; names += ["dispatch-query", "dispatch"]
            assert count >= 3
            names += [f"work-{index:02d}" for index in range(1, 4)] + ["restore"]
            names += [f"work-{index:02d}" for index in range(4, min(5, count) + 1)]
            if count > 5:
                names += ["short-work-query"] + [f"work-{index:02d}" for index in range(6, min(7, count) + 1)]
            if count > 7:
                names += ["current-work-query"] + [f"work-{index:02d}" for index in range(8, count + 1)]
    worlds, comparable, summaries, processes, applications, sources = [], [], [], set(), {}, {}
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; predecessor = ROOT / entry["receipt"]["path"]; stages, rows = [], []
        for name in names:
            path = ROOT / f"logs/surface-entry-components-{name}-{configuration}.json"; operation = load(path)["operation"]
            assert operation["receipt"] == identity(predecessor) and operation["source"] == load(predecessor)["operation"]["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                reference = operation[field]; verify_reference(reference)
            for reference in operation["sources"] + operation["native_processes"]:
                verify_reference(reference)
            for reference in operation["sources"]:
                sources[reference["path"]] = reference
            for reference in operation["native_processes"]:
                report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
                processes.add(reference["path"])
            snapshot = observe(ROOT / operation["output"]["path"]); assert json.loads(json.dumps(snapshot)) == operation["final"]
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"])
            applications[operation["application"]["path"]] = operation["application"]; predecessor = path
        if kind in ("collected", "reload"):
            assert snapshot["components_collected"] and snapshot["additional_collected"] == {"STEL": 36, "GOOD": 50, "PROP": 0}
        else:
            assert snapshot["additional_collected"] == {"STEL": 0, "GOOD": 0, "PROP": 0}
            if kind == "protected":
                protection, = operation["rows"]["protection"]; assert protection["complete"]
        if kind == "reload":
            replay_path = ROOT / f"logs/surface-entry-components-reload-{configuration}.json"; summary = load(replay_path)
            operation, resumed, replay_rows, native_processes, focused = validate(summary, dict(receipt=identity(predecessor)), observe)
            stages += resumed; rows += replay_rows; processes.update(native_processes); applications.update(focused)
            predecessor = ROOT / summary["continuations"][0]["path"]; snapshot = observe(ROOT / operation["output"]["path"])
        worlds.append(stages); comparable.append(rows)
        summaries.append(dict(configuration=configuration, receipt=identity(predecessor), output=operation["output"], company=snapshot["company"],
                              additional_collected=snapshot["additional_collected"], totals=snapshot["totals"], core=snapshot["core"], panel=snapshot["panel"],
                              service_lives=snapshot["service_lives"], components_collected=snapshot["components_collected"]))
    assert comparable[0] == comparable[1]
    # Every paired original stage must preserve all121native families. GLOG
    # contains the expected independently built application revision strings.
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, boundary=kind, foundation=identity(FOUNDATION), configurations=summaries,
                  native_processes=len(processes), cross_build_gameplay_comparisons=120 * len(worlds[0]), historical_versions=[identity(path) for path in VERSIONS], source=identity(Path(__file__)))
    if retain:
        copies = [product for product in previous["products"] if product["source"]["path"] not in replaced]
        target_dir = ROOT / "test-cache/surface-entry/components-products"; assert not target_dir.exists(); target_dir.mkdir(parents=True)
        sources.update(applications)
        for name in ("surface_entry_components_checks-2.py", "surface_entry_components_checks-3.py", "surface_gateway_replay.py"):
            reference = identity(DIRECTORY / name); sources[reference["path"]] = reference
        existing = {product["source"]["path"] for product in copies}
        # Retain each current source or binary once, keeping all historical
        # copies available without counting superseded products as current.
        for index, reference in enumerate(sources.values()):
            if reference["path"] in existing:
                continue
            source = ROOT / reference["path"]; target = target_dir / f"{index:02d}-{source.parent.name}-{source.name}"
            shutil.copyfile(source, target); assert identity(target)["sha256"] == reference["sha256"]
            copies.append(dict(source=reference, retained=identity(target))); existing.add(reference["path"])
        manifest = target_dir / "retained.json"
        manifest.write_text(json.dumps(dict(_documentation=__doc__, predecessor=identity(previous_path), products=copies), indent=2) + "\n", encoding="utf-8")
        result["products"] = identity(manifest); result["retained_products"] = len(copies)
    return result


def main():
    """Publish an immutable actual query, collection or ordinary-restored handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("query", "protected", "collected", "reload"), required=True)
    parser.add_argument("--work-count", type=int, default=0); parser.add_argument("--retain", action="store_true"); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--traffic-count", type=int, default=0)
    args = parser.parse_args(); output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    result = bind(args.kind, args.work_count, args.traffic_count, args.retain); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
