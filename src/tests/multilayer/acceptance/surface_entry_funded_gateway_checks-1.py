"""Bind complete current funded launch admission and the unchanged unfunded default.

@file surface_entry_funded_gateway_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQJ.
Every geometry, price, recipe and current owner comes from real pure commands.
Financial resources are native cash plus unused unchanged bank capacity.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import observe, lives

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-local-refresh-reload-verification.json"
PRODUCTS = ROOT / "test-cache/surface-entry/local-refresh-products/retained.json"


def execute():
    """@return Both current funded and original unfunded pure proofs, exact current compiled products and next selected original worlds."""
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["actual_capital_complete"]
    summaries, processes, tested_applications = [], set(), {}
    # Two explicitly different real worlds exercise the same native producer.
    # The old unfunded world is compatibility evidence and supplies no capital.
    for funded, stem, count in ((True, "surface-entry-funded-gateway", 11), (False, "surface-entry-unfunded-compat", 8)):
        worlds, rows = [], []
        for entry in foundation["configurations"]:
            configuration = entry["configuration"]; path = ROOT / f"logs/{stem}-query-{configuration}.json"
            value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
            operation = value["operation"]; assert operation["funded"] == funded and operation["service_count"] == count
            expected = entry["receipt"] if funded else identity(ROOT / f"logs/surface-entry-fuel-refill-continued-{configuration}-original.json")
            assert operation["receipt"] == expected
            predecessor = json.loads((ROOT / expected["path"]).read_text(encoding="utf-8"))
            assert operation["source"] == predecessor["operation"]["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                reference = operation[field]; assert identity(ROOT / reference["path"]) == reference
            for reference in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / reference["path"]) == reference
            for reference in operation["native_processes"]:
                report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]] and report["executable"] == operation["application"]
                processes.add(reference["path"])
            first, last = [chunks(ROOT / operation[field]["path"])[1] for field in ("before", "output")]
            assert first == last and len(first) == operation["unchanged_families"] == 121
            plan, = operation["rows"]["plan"]
            assert plan["complete_capital"] == funded and plan["target"] == 1870000
            if funded:
                snapshot = observe(ROOT / operation["output"]["path"]); company = snapshot["company"]
                assert json.loads(json.dumps(snapshot)) == operation["initial"] == operation["final"]
                assert company == entry["company"] and plan["spendable"] == entry["spendable"]
                assert plan["gap"] == 0 and plan["spendable"] >= plan["planned"] > 0
                service = lives(ROOT / operation["output"]["path"]); assert len(service) == 11 and min(row["remaining"] for row in service) > 732
                summaries.append(dict(configuration=configuration, receipt=identity(path), output=operation["output"], company=company,
                                      global_period=40, spendable=plan["spendable"], full_arrangement=operation["rows"], service_lives=service))
                tested_applications[operation["application"]["path"]] = operation["application"]
            else:
                assert plan["gap"] > 0 and plan["spendable"] < plan["target"]
            worlds.append(last); rows.append(operation["rows"])
        assert rows[0] == rows[1] and worlds[0].keys() == worlds[1].keys()
        assert all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    assert len(processes) == 4 and len(tested_applications) == 2
    previous = json.loads(PRODUCTS.read_text(encoding="utf-8")); copies = []
    # Only the two newly tested focused applications replace retained bytes.
    # Every other inherited current source remains exact; old binaries retain
    # their immutable prior copies and cannot silently qualify a new producer.
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] in tested_applications:
            assert identity(ROOT / product["source"]["path"]) == tested_applications[product["source"]["path"]]
        else:
            assert identity(ROOT / product["source"]["path"]) == product["source"]
            copies.append(product)
    retained = ROOT / "test-cache/surface-entry/funded-gateway-products"; assert not retained.exists(); retained.mkdir(parents=True)
    sources = [ROOT / path for path in tested_applications]
    sources += [ROOT / "src/tests/surface_entry_gateway.cpp", DIRECTORY / "surface_entry_gateway_checks-1.py", Path(__file__)]
    for index, source in enumerate(sources):
        target = retained / f"{index:02d}-{source.parent.name}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(PRODUCTS)), indent=2) + "\n", encoding="utf-8")
    ordinary = {configuration: identity(ROOT / f"build/{configuration}/openttd.exe") for configuration in ("RelWithDebInfo", "Debug")}
    assert ordinary["RelWithDebInfo"]["sha256"] == "e4903e67ad32de7c971963cf08c50e2024cc772deb0646ed18de929d01e301b3"
    assert ordinary["Debug"]["sha256"] == "cc8b57ecba5cba2e84ad2b21bb7d250e37e3758553c059c514087335ab88476f"
    return dict(_documentation=__doc__, complete=True, actual_capital_complete=True, actual_current_arrangement_admitted=True,
                actual_launch_complete=False, foundation=identity(FOUNDATION), configurations=summaries, native_processes=4,
                cross_build_gameplay_comparisons=240, unchanged_families_per_query=121, products=identity(manifest), ordinary_applications=ordinary,
                build_logs=[identity(ROOT / f"logs/surface-entry-funded-gateway-build-{configuration}.log") for configuration in ordinary], source=identity(Path(__file__)))


def main():
    """Retain complete admitted current launch geometry and funding before actual paid construction."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.write_text(json.dumps(execute(), indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
