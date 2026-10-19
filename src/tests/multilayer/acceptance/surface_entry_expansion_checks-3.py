"""Bind current original expansion admission and retain its exact tested products.

@file surface_entry_expansion_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CE.2/.3.
Native pure geometry can pass while whole funding and supplies remain missing.
This boundary never accepts a purchased Dock/Assembly or completed later chain.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_construction_ledger import observe

FOUNDATION = ROOT / "logs/surface-entry-core-construction-reload-verification.json"
MEMBERSHIP = ROOT / "logs/surface-entry-expansion-membership-verification.json"
DIRECTORY = Path(__file__).resolve().parent
CHANGED = {"src/tests/surface_entry_gateway.cpp", "src/tests/multilayer/acceptance/surface_gateway_material_ledger.py"}


def load(path):
    """@param path Complete accepted immutable receipt. @return Existing receipt object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def bind(retain=False):
    """@param retain Whether to additionally retain the current exactly tested products. @return Both-build pure admission and explicit still-missing genuine prerequisites."""
    foundation, membership = load(FOUNDATION), load(MEMBERSHIP); assert membership["foundation"] == identity(FOUNDATION)
    previous_path = ROOT / foundation["products"]["path"]; assert identity(previous_path) == foundation["products"]
    previous = json.loads(previous_path.read_text(encoding="utf-8")); applications, summaries, worlds, rows, processes = {}, [], [], [], set()
    replaced = CHANGED | {f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")}
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]
    # Current independent original worlds must quote the same actual layout,
    # bills and refusals. Exact native source/hash/owner comparisons preserve
    # all prior paid Core material, hull, power, service and packet receipts.
    for entry, member in zip(foundation["configurations"], membership["configurations"], strict=True):
        configuration = entry["configuration"]; path = ROOT / f"logs/surface-entry-expansion-query-{configuration}.json"; current = load(path)["operation"]
        assert member["receipt"] == entry["receipt"] == current["receipt"] and member["output"] == entry["output"] == current["source"]
        for field in ("receipt", "source", "before", "output", "application", "terminal", "retained_harness"):
            reference = current[field]; assert identity(ROOT / reference["path"]) == reference
        for reference in current["sources"] + current["native_processes"]:
            assert identity(ROOT / reference["path"]) == reference
        for reference in current["native_processes"]:
            native = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
            assert native["exit_status"] == 0 and native["inputs"] == [current["source"]] and native["executable"] == current["application"]
            processes.add(reference["path"])
        output = ROOT / current["output"]["path"]; snapshot = observe(output)
        assert json.loads(json.dumps(snapshot)) == current["final"]
        assert chunks(ROOT / current["before"]["path"])[1] == chunks(output)[1]
        applications[current["application"]["path"]] = current["application"]; worlds.append(chunks(output)[1]); rows.append(current["rows"])
        summaries.append(dict(configuration=configuration, receipt=identity(path), output=current["output"], company=snapshot["company"], spendable=member["spendable"],
                              layout=current["rows"]["layout"], budget=current["rows"]["budget"], inputs=current["rows"]["inputs"], fuel=current["rows"]["fuel"],
                              original_core=snapshot["core"], service_lives=snapshot["service_lives"], membership=member["receipt"]))
    assert len(processes) == 2 and rows[0] == rows[1] and worlds[0].keys() == worlds[1].keys() and len(worlds[0]) == 121
    assert all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    result = dict(_documentation=__doc__, complete=True, native_layout_admitted=True, funded_expansion=False, supplies_complete=False,
                  full_logistics_bill_complete=False, paid_expansion_complete=False, foundation=identity(FOUNDATION), membership=identity(MEMBERSHIP),
                  configurations=summaries, native_processes=2, ordinary_reruns=0, cross_build_gameplay_comparisons=120, source=identity(Path(__file__)))
    if retain:
        copies = [product for product in previous["products"] if product["source"]["path"] not in replaced]
        retained = ROOT / "test-cache/surface-entry/expansion-products"; assert not retained.exists(); retained.mkdir(parents=True)
        sources = [ROOT / path for path in applications] + [ROOT / path for path in sorted(CHANGED)]
        sources += [DIRECTORY / f"surface_entry_expansion_checks-{index}.py" for index in (1, 2, 3)]
        sources += [ROOT / item["path"] for item in membership["sources"]]
        existing = {product["source"]["path"] for product in copies}
        # Retain each tested/source-authority product once, keeping every older
        # immutable copy available while excluding only declared replacements.
        for index, source in enumerate(sources):
            reference = identity(source)
            if reference["path"] in existing:
                continue
            target = retained / f"{index:02d}-{source.parent.name}-{source.name}"; shutil.copyfile(source, target)
            assert reference["sha256"] == identity(target)["sha256"]; copies.append(dict(source=reference, retained=identity(target))); existing.add(reference["path"])
        manifest = retained / "retained.json"
        manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(previous_path)), indent=2) + "\n", encoding="utf-8")
        result["products"] = identity(manifest); result["retained_products"] = len(copies)
    return result


def main():
    """Publish actual current original expansion query acceptance or its exact retained-product handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--retain", action="store_true"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(bind(args.retain), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
