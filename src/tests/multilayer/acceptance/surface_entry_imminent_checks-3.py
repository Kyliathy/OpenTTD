"""Accept real five-road renewal, ordinary exact annual service and fresh tested products.

@file surface_entry_imminent_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQE.
The changed regional native selector needs current tested source/binary products.
Exact old retained bytes remain history; only specifically observed replacements
enter the new manifest. No gameplay family or accounting comparison is waived.
"""
import argparse
import json
from pathlib import Path
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import observe, lives
from surface_regional_replay import replay

DIRECTORY = Path(__file__).resolve().parent


def reload(configuration):
    """@param configuration Actual ordinary/focused pair.
    @return Exact ordinary restoration and actual27000tick original/copy service, selecting only original income.
    """
    result = replay(configuration, ROOT / f"logs/surface-entry-imminent-renew-regional-{configuration}.json", "surface-entry-imminent-service", phase="earn")
    earned, = result["rows"]["earned"]; profit, = result["rows"]["profit"]
    assert earned["earned"] == 27000 and earned["net_expense"] < 0 and profit["amount"] > 0
    assert len(result["rows"]["vehicle_profit"]) == 3
    delivered = {row["label"] for row in result["rows"]["delivered"] if row["amount"] > 0}
    assert {"PASS", "COAL", "LVST", "GRAI", "GOOD", "IORE", "STEL"} <= delivered
    result["global_period"] = 30
    return result


def final():
    """@return Both-build real paid fleet/annual replay, complete launch custody and explicitly verified fresh current products."""
    foundation_path = ROOT / "logs/surface-entry-imminent-purchase-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    summaries, worlds, rows, processes, tested, versions = [], [], [], set(), {}, {}
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]
        for reference in entry["receipts"]:
            assert identity(ROOT / reference["path"]) == reference
            operation = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness", "retained_wrapper"):
                assert identity(ROOT / operation[field]["path"]) == operation[field]
            for item in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / item["path"]) == item
            for item in operation["sources"] + [operation["application"]]:
                tested[item["path"]] = item
            for field in ("retained_harness", "retained_wrapper"):
                item = operation[field]; versions[item["sha256"]] = item
            processes.update(item["path"] for item in operation["native_processes"])
        path = ROOT / f"logs/surface-entry-imminent-service-reload-{configuration}.json"
        value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["global_period"] == 30 and value["additional_continuation_ticks"] == 27000
        assert value["source_receipt"] == entry["receipt"]
        assert identity(ROOT / value["source"]["path"]) == value["source"]; tested[value["source"]["path"]] = value["source"]
        restore = json.loads((ROOT / value["restore"]["path"]).read_text(encoding="utf-8"))["operation"]
        original, restored = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(restored); normalized[b"DATE"] = original[b"DATE"]; stages = [normalized]
        processes.update(item["path"] for item in restore["native_processes"])
        comparable = []
        for reference in value["continuations"]:
            assert identity(ROOT / reference["path"]) == reference
            operation = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / operation[field]["path"]) == operation[field]
            for item in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / item["path"]) == item
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); comparable.append(operation["rows"])
            processes.update(item["path"] for item in operation["native_processes"])
            retained = operation["retained_harness"]; versions[retained["sha256"]] = retained
        assert comparable[0] == comparable[1]
        source = ROOT / value["output"]["path"]; assert observe(source)["company"] == value["company"]
        company = value["company"]; service = lives(source); assert len(service) == 11 and min(row["remaining"] for row in service) > 732
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              global_period=30, additional_continuation_ticks=27000, spendable=company["cash"] + 5000000 - company["debt"],
                              service_lives=service, annual_rows=value["rows"], reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == 28
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    previous_manifest = ROOT / "test-cache/surface-entry/treasury-boundary-29-products/retained.json"
    previous = json.loads(previous_manifest.read_text(encoding="utf-8")); retained = ROOT / "test-cache/surface-entry/imminent-service-products"
    assert not retained.exists(); retained.mkdir(parents=True); copies, replacements = [], []
    allowed = {"src/tests/surface_entry_bus_renewal.cpp", "src/tests/multilayer/acceptance/surface_entry_bus_checks-1.py",
               "src/tests/multilayer/acceptance/surface_regional_replay.py"}
    allowed.update(f"build/{configuration}/openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug"))
    # Replace only these five explicitly delivered products, each observed in
    # current native/replay receipts. Old retained source/binary bytes stay valid
    # history; arbitrary dependency or executable changes cannot pass this gate.
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        source = ROOT / product["source"]["path"]; current = identity(source)
        if current == product["source"]:
            copies.append(product); continue
        assert current["path"] in allowed and tested[current["path"]] == current
        target = retained / f"changed-{len(replacements):02d}-{source.name}"; shutil.copyfile(source, target)
        assert identity(target)["sha256"] == current["sha256"]
        copies.append(dict(source=current, retained=identity(target), replaces=product)); replacements.append(current)
    assert {item["path"] for item in replacements} == allowed
    products = [DIRECTORY / "surface_entry_road_renewal_checks-1.py"] + [DIRECTORY / f"surface_entry_imminent_checks-{index}.py" for index in (1, 2, 3)]
    for index, source in enumerate(products):
        target = retained / f"new-{index:02d}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(previous_manifest),
                                      explicit_tested_replacements=replacements, invocation_driver_versions=list(versions.values())), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_imminent_renewal_complete=True, actual_capital_complete=False, actual_launch_complete=False,
                foundation=identity(foundation_path), configurations=summaries, native_processes=len(processes), products=identity(manifest),
                cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240, source=identity(Path(__file__)))


def main():
    """Publish focused whole-world annual replay or the exact current fresh-product fleet handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    if args.kind == "reload":
        assert args.configuration is not None; value = reload(args.configuration)
    else:
        value = final()
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
