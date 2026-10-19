"""Bind the genuine first Core's original material/work/activation progression.

@file surface_entry_core_construction_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CD.
Actual saved intermediate jobs, frames and shared panel identities must agree
in both builds; inherited fixture construction supplies no predecessor.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_construction_ledger import observe

FOUNDATION = ROOT / "logs/surface-entry-core-resupply-reload-verification.json"
STEM = "surface-entry-core-construction"
CHANGED = {"src/tests/surface_entry_gateway.cpp"} | {
    "src/tests/multilayer/acceptance/" + name for name in
    ("surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_core_ledger.py", "surface_gateway_resupply_ledger.py")}


def load(path):
    """@param path Complete accepted immutable receipt. @return Existing receipt object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def bind(kind):
    """@param kind Pure native construction admission or actual operational Core. @return Exact original source/job/work/module/panel evidence in both builds."""
    foundation = load(FOUNDATION); assert foundation["actual_goods_resupply_complete"] and not foundation["core_constructed"]
    phases = ["query"] if kind == "plan" else ["query", "build"]
    manifest_path = ROOT / foundation["products"]["path"]; assert identity(manifest_path) == foundation["products"]
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    replaced = CHANGED | {f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")}
    # Explicit current construction consumers replace their own tested copies;
    # every earlier retained byte and unrelated current product remains exact.
    for product in manifest["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]
    summaries, worlds, rows, processes = [], [], [], set()
    # Follow each actual original source-to-output chain independently, then
    # inspect every native saved material/work/activation checkpoint. Matching
    # copied histories cannot replace an original paid site or physical input.
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
        if kind == "built":
            assert snapshot["operational"] and snapshot["orbital_consumed"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
            assert snapshot["totals"] == {"STEL": 4, "GOOD": 2, "PROP": 12}
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=identity(previous), output=current["output"], company=company,
                              spendable=company["cash"] + 5000000 - company["debt"], receipts=references, totals=snapshot["totals"],
                              core=snapshot["core"], panel=snapshot["panel"], operational=snapshot["operational"], orbital_consumed=snapshot["orbital_consumed"],
                              paid_craft=snapshot["paid_craft"], service_lives=snapshot["service_lives"], native_rows=comparable))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, original_operational_core=kind == "built", core_constructed=kind == "built",
                foundation=identity(FOUNDATION), stage=kind, configurations=summaries, native_processes=len(processes),
                cross_build_gameplay_comparisons=120 * len(worlds[0]), source=identity(Path(__file__)))


def main():
    """Publish only the actual completed first-Core boundary without overwriting accepted evidence."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("plan", "built"), required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.kind), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
