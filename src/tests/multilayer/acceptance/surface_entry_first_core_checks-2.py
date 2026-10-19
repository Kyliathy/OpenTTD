"""Bind actual original Core funding, typed delivery and exclusive material ownership.

@file surface_entry_first_core_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CB.
No inherited fixture or copied packet can satisfy the saved native input job.
The remaining Goods/resupply/completion steps retain their own later gates.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_core_ledger import observe

FOUNDATION = ROOT / "logs/surface-entry-first-mission-reload-verification.json"
STEM = "surface-entry-first-core"
CHANGED = {"src/tests/surface_entry_gateway.cpp"} | {
    "src/tests/multilayer/acceptance/" + name for name in
    ("surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_mission_ledger.py")}


def load(path):
    """@param path Complete accepted immutable receipt. @return Existing receipt object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def bind(kind):
    """@param kind Pure Core admission or paid Steel-backed job. @return Exact current sources/native owners and both-build comparisons."""
    foundation = load(FOUNDATION); assert foundation["first_actual_mission_complete"] and not foundation["core_constructed"]
    phases = ["query"] if kind == "plan" else ["query", "buy", "order-query", "order", "steel-work"]
    manifest_path = ROOT / foundation["products"]["path"]; assert identity(manifest_path) == foundation["products"]
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    replaced = CHANGED | {f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")}
    # Actual Core consumers are explicit extensions; older retained source and
    # executable copies remain exact. All unrelated inherited products must
    # still match current sources rather than receiving a blanket hash waiver.
    for product in manifest["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]
    summaries, worlds, rows, processes = [], [], [], set()
    # Each independent build follows only its original paid flight checkpoint
    # and immediate native successors. Exact physical equality never pools the
    # two copies' funds or duplicates the delivered Steel's sole job ownership.
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
        if kind == "prepared":
            assert snapshot["core"]["row"]["inputs"][0]["quantity"] == 16 and snapshot["core"]["row"]["work"] == 0
            assert snapshot["space_orders"]["queues"][38]["blocked"] == 9
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=current["output"], company=company,
                              spendable=company["cash"] + 5000000 - company["debt"], receipts=references,
                              totals=snapshot["totals"], core=snapshot["core"], space_orders=snapshot["space_orders"], paid_craft=snapshot["paid_craft"],
                              service_lives=snapshot["service_lives"], native_rows=comparable))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, paid_steel_backed_core=kind == "prepared", core_constructed=False,
                foundation=identity(FOUNDATION), stage=kind, configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=120 * len(phases), source=identity(Path(__file__)))


def main():
    """Publish only the completed original Core boundary without overwriting accepted evidence."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "prepared"), required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
