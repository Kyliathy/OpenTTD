"""Bind current original first-flight admission and actual native phase receipts.

@file surface_entry_first_mission_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15C.
The accepted entry lineage remains immutable. Exact old products survive in
their retained copies while explicitly extended producers are freshly tested.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_mission_ledger import observe

FOUNDATION = ROOT / "logs/surface-entry-gateway-entry-verification.json"
STEM = "surface-entry-first-mission"
CHANGED = {"src/tests/surface_entry_gateway.cpp"} | {
    "src/tests/multilayer/acceptance/" + name for name in
    ("surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_payload_ledger.py")}


def load(path):
    """@param path Complete accepted immutable receipt. @return Existing receipt object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def bind(kind):
    """@param kind Pure admission or whole actual first flight. @return Exact source/custody/native phase comparisons in both builds."""
    foundation = load(FOUNDATION); assert foundation["genuine_surface_and_launch_entry_complete"]
    phases = ["query"] if kind == "plan" else ["query", "begin", "ascent", "orbit", "deploy", "recover"]
    manifest = json.loads((ROOT / foundation["products"]["path"]).read_text(encoding="utf-8"))
    assert identity(ROOT / foundation["products"]["path"]) == foundation["products"]
    # Only explicit current producer/observer extensions and their two newly
    # built applications replace prior versions. Every old retained byte stays
    # available; unrelated inherited products must still match their sources.
    replaced = CHANGED | {f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")}
    for product in manifest["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]
    summaries, worlds, rows, processes = [], [], [], set()
    # Each original build uses its own selected exact entry, then only the
    # immediately preceding actual paid/native output. Cross-build equality
    # checks simulation owners without combining copied funds or inventory.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; previous = ROOT / entry["receipt"]["path"]
        assert identity(previous) == entry["receipt"]
        stages, comparable, references = [], [], []
        for phase in phases:
            path = ROOT / f"logs/{STEM}-{phase}-{configuration}.json"; value = load(path); current = value["operation"]
            assert current["receipt"] == identity(previous) and current["source"] == load(previous)["operation"]["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                reference = current[field]; assert identity(ROOT / reference["path"]) == reference
            for reference in current["sources"] + current["native_processes"]:
                assert identity(ROOT / reference["path"]) == reference
            for reference in current["native_processes"]:
                report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [current["source"]] and report["executable"] == current["application"]
                processes.add(reference["path"])
            output = ROOT / current["output"]["path"]; snapshot = observe(output)
            assert json.loads(json.dumps(snapshot)) == current["final"]
            stages.append(chunks(output)[1]); comparable.append(current["rows"]); references.append(identity(path)); previous = path
        if kind == "flight":
            assert snapshot["flight"]["phase"] == 0 and snapshot["flight"]["started"] == snapshot["flight"]["completed"] == 1
            assert snapshot["fuel_burned"] == 15 and snapshot["totals"] == {"STEL": 20, "GOOD": 26, "PROP": 27}
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=current["output"], company=company,
                              spendable=company["cash"] + 5000000 - company["debt"], receipts=references,
                              totals=snapshot["totals"], fuel_burned=snapshot["fuel_burned"], flight=snapshot["flight"], paid_craft=snapshot["paid_craft"],
                              service_lives=snapshot["service_lives"], native_rows=comparable))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, first_actual_mission_complete=kind == "flight", core_constructed=False,
                foundation=identity(FOUNDATION), stage=kind, configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=120 * len(phases), source=identity(Path(__file__)))


def main():
    """Publish only the currently completed first-flight boundary without overwriting accepted evidence."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "flight"), required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
