"""Bind current unchanged module and literal-refill admission with exact products.

@file surface_entry_supply_admission_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CH.3.
Current prices and gaps remain admission only. No replacement, raw input,
Propellant, working capital or orbital module is supplied by this handoff.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe
from surface_gateway_replay import load

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-component-delivery-reload-verification.json"
MEMBERSHIP = ROOT / "logs/surface-entry-supply-admission-membership-verification.json"
VERSIONS = ROOT / "test-cache/surface-entry/supply-admission-source-versions/before-current-admission.json"
CHANGED = {"src/tests/surface_entry_resupply.cpp"}


def verify_reference(reference):
    """@param reference Exact current or pre-edit characterization authority. Verify only current bytes or the explicitly hash-qualified original producer copy."""
    if identity(ROOT / reference["path"]) == reference:
        return
    products = json.loads(VERSIONS.read_text(encoding="utf-8"))["products"]
    exact, = [row for row in products if row["source"] == reference["path"] and row["sha256"] == reference["sha256"]]
    retained = identity(ROOT / exact["retained"])
    assert {key: value for key, value in retained.items() if key != "path"} == {key: value for key, value in reference.items() if key != "path"}


def bind(retain):
    """@param retain Whether to retain exact current tested products. @return Both-build pure actual module/refill boundaries and explicitly unfulfilled funding/logistics/renewal prerequisites."""
    foundation, membership = load(FOUNDATION), load(MEMBERSHIP); assert membership["foundation"] == identity(FOUNDATION)
    previous_path = ROOT / foundation["products"]["path"]; assert identity(previous_path) == foundation["products"]
    previous = json.loads(previous_path.read_text(encoding="utf-8"))
    replaced = CHANGED | {f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")}
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]
    for reference in membership["sources"]:
        verify_reference(reference)
    worlds, comparable, summaries, processes, products = [], [], [], set(), {}
    # Each actual original source must remain unchanged through both pure
    # native boundaries. Equal diagnostics alone cannot admit changed cargo,
    # money, ages or RNG in either independently linked configuration.
    for entry, member in zip(foundation["configurations"], membership["configurations"], strict=True):
        configuration = entry["configuration"]; assert member["receipt"] == entry["receipt"] and member["output"] == entry["output"]
        predecessor = ROOT / entry["receipt"]["path"]; stages, rows = [], []
        for boundary in ("modules", "refill"):
            path = ROOT / f"logs/surface-entry-supply-admission-{boundary}-{configuration}.json"; current = load(path)["operation"]
            assert current["receipt"] == identity(predecessor) and current["source"] == load(predecessor)["operation"]["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                reference = current[field]; verify_reference(reference)
            for reference in current["sources"] + current["native_processes"]:
                verify_reference(reference)
            for reference in current["sources"] + [current["application"]]:
                products[reference["path"]] = reference
            for reference in current["native_processes"]:
                report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [current["source"]] and report["executable"] == current["application"]
                processes.add(reference["path"])
            first, last = [chunks(ROOT / current[field]["path"])[1] for field in ("before", "output")]
            assert first == last and json.loads(json.dumps(observe(ROOT / current["output"]["path"]))) == current["final"]
            assert current["initial"] == current["final"]
            stages.append(last); rows.append(current["rows"]); predecessor = path
        assert current["final"]["company"] == member["company"] == {"cash": 143673, "debt": 5000000}
        worlds.append(stages); comparable.append(rows)
        summaries.append(dict(configuration=configuration, receipt=identity(predecessor), output=current["output"], company=current["final"]["company"],
                              totals=current["final"]["totals"], cargo_abandoned=current["final"]["cargo_abandoned"], service_lives=current["final"]["service_lives"],
                              modules=rows[0], refill=rows[1]))
    assert len(processes) == 4 and comparable[0] == comparable[1]
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, admission_only=True, funded_expansion=False, fuel_procured=False, replacement_owned=False,
                  full_logistics_bill_complete=False, foundation=identity(FOUNDATION), membership=identity(MEMBERSHIP), historical_versions=identity(VERSIONS),
                  configurations=summaries, native_processes=4, cross_build_gameplay_comparisons=240, source=identity(Path(__file__)))
    if retain:
        copies = [product for product in previous["products"] if product["source"]["path"] not in replaced]
        retained = ROOT / "test-cache/surface-entry/supply-admission-products"; assert not retained.exists(); retained.mkdir(parents=True)
        for index in (1, 2, 3):
            reference = identity(DIRECTORY / f"surface_entry_supply_admission_checks-{index}.py"); products[reference["path"]] = reference
        existing = {product["source"]["path"] for product in copies}
        # Preserve each current authority once and keep all predecessor and
        # pre-edit copies exact; superseded bytes never become current prices.
        for index, reference in enumerate(products.values()):
            if reference["path"] in existing:
                continue
            source = ROOT / reference["path"]; target = retained / f"{index:02d}-{source.parent.name}-{source.name}"
            shutil.copyfile(source, target); assert identity(target)["sha256"] == reference["sha256"]
            copies.append(dict(source=reference, retained=identity(target))); existing.add(reference["path"])
        manifest = retained / "retained.json"
        manifest.write_text(json.dumps(dict(_documentation=__doc__, predecessor=identity(previous_path), products=copies), indent=2) + "\n", encoding="utf-8")
        result["products"] = identity(manifest); result["retained_products"] = len(copies)
    return result


def main():
    """Publish current exact pure prerequisite admission without overwriting previous evidence."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--retain", action="store_true"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.retain), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
