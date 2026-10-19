"""Bind genuine Core Goods loading and the same rocket's actual second mission.

@file surface_entry_core_resupply_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CC.
Every intermediate phase retains actual paid hull/job/packet identities; old
strict consumers retain their exact qualified source and executable versions.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_resupply_ledger import observe

FOUNDATION = ROOT / "logs/surface-entry-first-core-reload-verification.json"
STEM = "surface-entry-core-resupply"
CHANGED = {"src/tests/surface_entry_gateway.cpp"} | {
    "src/tests/multilayer/acceptance/" + name for name in
    ("surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_payload_ledger.py")}


def load(path):
    """@param path Complete accepted immutable receipt. @return Existing receipt object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def bind(kind):
    """@param kind Pure admission, loaded/ready flight or actual completed resupply. @return Exact native phase/job/cargo/payment evidence in both builds."""
    foundation = load(FOUNDATION); assert foundation["paid_steel_backed_core"] and not foundation["core_constructed"]
    phases = ["query"]
    if kind != "plan":
        phases += ["load", "flight-query"]
    if kind == "flight":
        phases += ["flight"]
    manifest_path = ROOT / foundation["products"]["path"]; assert identity(manifest_path) == foundation["products"]
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    replaced = CHANGED | {f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")}
    # Explicit current resupply extensions replace only their tested versions;
    # earlier retained copies and every unrelated current product stay exact.
    for product in manifest["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]
    summaries, worlds, rows, processes = [], [], [], set()
    # Each build starts from its own exact original paid Core checkpoint.
    # Follow actual output-to-input identities and saved inner flight phases;
    # a copied world's material or funds never enter the selected branch.
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
            for inner in current.get("stages", []):
                saved = inner["saved"]; assert identity(ROOT / saved["path"]) == saved
                assert json.loads(json.dumps(observe(ROOT / saved["path"]))) == inner["observed"]
                stages.append(chunks(ROOT / saved["path"])[1])
            output = ROOT / current["output"]["path"]; snapshot = observe(output)
            assert json.loads(json.dumps(snapshot)) == current["final"]
            stages.append(chunks(output)[1]); comparable.append(current["rows"]); references.append(identity(path)); previous = path
        if kind == "flight":
            assert snapshot["flight"]["phase"] == 0 and snapshot["flight"]["started"] == snapshot["flight"]["completed"] == 2
            assert snapshot["goods_holds"] == {54: 0, 38: 24} and snapshot["fuel_burned"] == 30
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=current["output"], company=company,
                              spendable=company["cash"] + 5000000 - company["debt"], receipts=references,
                              totals=snapshot["totals"], core=snapshot["core"], goods_holds=snapshot["goods_holds"], flight=snapshot["flight"],
                              paid_craft=snapshot["paid_craft"], service_lives=snapshot["service_lives"], native_rows=comparable))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_goods_resupply_complete=kind == "flight", core_constructed=False,
                foundation=identity(FOUNDATION), stage=kind, configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=120 * len(worlds[0]), source=identity(Path(__file__)))


def main():
    """Publish only the completed original resupply boundary without overwriting accepted evidence."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "ready", "flight"), required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
