"""Bind original additional component transport and paid center handling.

@file surface_entry_component_delivery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CG.
Every selected original phase retains exact native sources, actual cargo owners
and both-build equality. Ordinary replay cannot supply a second material batch.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe, observe_retained
from surface_gateway_replay import load, validate

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-components-reload-verification.json"
CHANGED = {"src/tests/surface_entry_gateway.cpp"} | {"src/tests/multilayer/acceptance/" + name for name in
          ("surface_gateway_material_ledger.py", "surface_gateway_resupply_ledger.py", "surface_gateway_construction_ledger.py")}
STEM = "surface-entry-component-delivery"
VERSIONS = ROOT / "test-cache/surface-entry/component-delivery-source-versions/before-native-abandonment.json"


def verify_reference(reference):
    """@param reference Exact successful native source/application identity. Verify current bytes or its explicitly retained pre-abandonment copy."""
    if identity(ROOT / reference["path"]) == reference:
        return
    products = json.loads(VERSIONS.read_text(encoding="utf-8"))["products"]
    exact, = [row for row in products if row["source"] == reference["path"] and row["sha256"] == reference["sha256"]]
    retained = identity(ROOT / exact["retained"])
    assert {key: value for key, value in retained.items() if key != "path"} == {key: value for key, value in reference.items() if key != "path"}


def bind(kind, count, retain):
    """@param kind Pure query, physical member arrival, paid center storage or final ordinary-restored handoff.
    @param count Actual sequential original road-work bursts. @param retain Whether to retain current tested products.
    @return Both-build original receipt chain, physical custody, actual fees and current product identities.
    """
    foundation = load(FOUNDATION); previous_path = ROOT / foundation["products"]["path"]
    assert identity(previous_path) == foundation["products"]; previous = json.loads(previous_path.read_text(encoding="utf-8"))
    replaced = CHANGED | {f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")}
    # Completed collection products remain exact; only the explicitly named
    # new transport consumers replace current copies, never historical bytes.
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]
    names = ["query"]
    if kind != "query":
        assert count > 0; names += ["dispatch"] + [f"work-{index:02d}" for index in range(1, count + 1)]
    if kind in ("stored", "reload"):
        names += ["handling-query", "handling", "handling-work"]
    worlds, comparable, summaries, processes, applications, sources = [], [], [], set(), {}, {}
    # Each configuration follows its own original source chain. Printed
    # equality cannot substitute a copied save or a different cargo owner.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; predecessor = ROOT / entry["receipt"]["path"]; stages, rows = [], []
        for name in names:
            path = ROOT / f"logs/{STEM}-{name}-{configuration}.json"; operation = load(path)["operation"]
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
            abandoned = operation["final"].get("cargo_abandoned", {"STEL": 0, "GOOD": 0, "PROP": 0})
            snapshot = observe(ROOT / operation["output"]["path"], allow_abandonment=abandoned["STEL"] == 8)
            comparable_snapshot = json.loads(json.dumps(snapshot))
            if "cargo_abandoned" not in operation["final"]:
                # Earlier exact consumers predate only this explicit loss
                # observation field. Their retained bytes and lossless saved
                # quantities remain unchanged; remove no other comparison.
                assert comparable_snapshot.pop("cargo_abandoned") == {"STEL": 0, "GOOD": 0, "PROP": 0}
            assert comparable_snapshot == operation["final"]
            if "native_abandonment" in operation:
                loss = operation["native_abandonment"]; initial = observe(ROOT / operation["before"]["path"])
                assert json.loads(json.dumps(initial)) == operation["initial"]
                assert loss["before"] == operation["before"] and loss["output"] == operation["output"]
                assert loss["lost"] == snapshot["cargo_abandoned"] == {"STEL": 8, "GOOD": 2, "PROP": 0}
                assert {label: initial["totals"][label] - snapshot["totals"][label] for label in initial["totals"]} == loss["lost"]
                assert snapshot["component_carriers_returned"] and len(loss["authority"]) == 4
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"])
            applications[operation["application"]["path"]] = operation["application"]; predecessor = path
        if kind == "arrived":
            assert snapshot["components_at_member"]
        if kind in ("stored", "reload"):
            assert snapshot["components_at_center"]
        if kind == "reload":
            summary = load(ROOT / f"logs/{STEM}-reload-{configuration}.json")
            operation, resumed, replay_rows, native_processes, focused = validate(summary, dict(receipt=identity(predecessor)), observe_retained)
            stages += resumed; rows += replay_rows; processes.update(native_processes); applications.update(focused)
            predecessor = ROOT / summary["continuations"][0]["path"]; snapshot = observe_retained(ROOT / operation["output"]["path"])
        worlds.append(stages); comparable.append(rows)
        summaries.append(dict(configuration=configuration, receipt=identity(predecessor), output=operation["output"], company=snapshot["company"],
                              totals=snapshot["totals"], additional_collected=snapshot["additional_collected"], delivery_stocks=snapshot["delivery_stocks"],
                              cargo_abandoned=snapshot["cargo_abandoned"],
                              components_at_member=snapshot["components_at_member"], components_at_center=snapshot["components_at_center"],
                              core=snapshot["core"], panel=snapshot["panel"], service_lives=snapshot["service_lives"]))
    assert comparable[0] == comparable[1]
    # Independently linked builds must agree on every saved gameplay family;
    # GLOG alone retains their expected different executable revision strings.
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, boundary=kind, foundation=identity(FOUNDATION), configurations=summaries,
                  native_processes=len(processes), cross_build_gameplay_comparisons=120 * len(worlds[0]), historical_versions=identity(VERSIONS), source=identity(Path(__file__)))
    if retain:
        copies = [product for product in previous["products"] if product["source"]["path"] not in replaced]
        target_dir = ROOT / "test-cache/surface-entry/component-delivery-products"; assert not target_dir.exists(); target_dir.mkdir(parents=True)
        sources.update(applications)
        for name in ("surface_entry_component_delivery_checks-2.py", "surface_entry_component_delivery_checks-3.py", "surface_gateway_replay.py"):
            reference = identity(DIRECTORY / name); sources[reference["path"]] = reference
        existing = {product["source"]["path"] for product in copies}
        # Retain every current tested product once, preserving the immutable
        # predecessor copies and exact consumer/binary identities for handoff.
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
    """Publish only the selected actual transport/handling boundary, preserving every earlier receipt."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("query", "arrived", "stored", "reload"), required=True)
    parser.add_argument("--work-count", type=int, default=0); parser.add_argument("--retain", action="store_true"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    result = bind(args.kind, args.work_count, args.retain); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
