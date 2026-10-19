"""Accept actual head-only renewal, exact ordinary annual service and current products.

@file surface_entry_rail_only_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQG.
The original32wagons and eight healthy roads retain their real owners and
commercial roles. Only original native annual service contributes capital.
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
    """@param configuration Actual ordinary/focused pair. @return Exact ordinary restore and27000tick genuine useful service."""
    result = replay(configuration, ROOT / f"logs/surface-entry-rail-only-buy-{configuration}.json", "surface-entry-rail-only-service", phase="earn")
    earned, = result["rows"]["earned"]; assert earned["earned"] == 27000 and earned["net_expense"] < 0
    delivered = {row["label"] for row in result["rows"]["delivered"] if row["amount"] > 0}
    assert {"PASS", "COAL", "LVST", "GRAI", "GOOD", "IORE", "STEL"} <= delivered
    result["global_period"] = 38
    return result


def final():
    """@return Both-build paid three-head/original-wagon renewal and exact ordinary annual ownership with current tested products."""
    foundation_path = ROOT / "logs/surface-entry-rail-only-purchase-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    summaries, worlds, rows, processes, versions = [], [], [], set(), {}
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]
        for reference in entry["receipts"]:
            assert identity(ROOT / reference["path"]) == reference
            operation = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness", "retained_wrapper"):
                assert identity(ROOT / operation[field]["path"]) == operation[field]
            for item in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / item["path"]) == item
            for field in ("retained_harness", "retained_wrapper"):
                item = operation[field]; versions[item["sha256"]] = item
            processes.update(item["path"] for item in operation["native_processes"])
        path = ROOT / f"logs/surface-entry-rail-only-service-reload-{configuration}.json"
        value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["global_period"] == 38
        assert value["source_receipt"] == entry["receipt"] and value["additional_continuation_ticks"] == 27000
        restore = json.loads((ROOT / value["restore"]["path"]).read_text(encoding="utf-8"))["operation"]
        original, restored = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(restored); normalized[b"DATE"] = original[b"DATE"]; stages = [normalized]
        processes.update(item["path"] for item in restore["native_processes"]); comparable = []
        for reference in value["continuations"]:
            assert identity(ROOT / reference["path"]) == reference
            operation = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / operation[field]["path"]) == operation[field]
            for item in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / item["path"]) == item
            stages.append(chunks(ROOT / operation["output"]["path"])[1]); comparable.append(operation["rows"])
            processes.update(item["path"] for item in operation["native_processes"])
            item = operation["retained_harness"]; versions[item["sha256"]] = item
        assert comparable[0] == comparable[1]
        source = ROOT / value["output"]["path"]; assert observe(source)["company"] == value["company"]
        service = lives(source); assert len(service) == 11 and min(row["remaining"] for row in service) > 732
        company = value["company"]
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              global_period=38, additional_continuation_ticks=27000, spendable=company["cash"] + 5000000 - company["debt"],
                              service_lives=service, annual_rows=value["rows"], reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == foundation["native_processes"] + 6
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    previous_manifest = ROOT / "test-cache/surface-entry/treasury-boundary-37-products/retained.json"
    previous = json.loads(previous_manifest.read_text(encoding="utf-8")); unique = {}
    # Identical metadata-controller copies describe the same current source;
    # retain one current product per path while preserving the complete old
    # manifest as history. Every source and retained copy must still match exactly.
    for product in previous["products"]:
        assert identity(ROOT / product["source"]["path"]) == product["source"]
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        key = product["source"]["path"]
        if key in unique:
            assert unique[key]["source"] == product["source"]
        else:
            unique[key] = product
    retained = ROOT / "test-cache/surface-entry/rail-only-service-products"; assert not retained.exists(); retained.mkdir(parents=True)
    copies = list(unique.values())
    products = [ROOT / "src/tests/surface_entry_renewal.cpp", DIRECTORY / "surface_entry_coordinated_checks-1.py"]
    products += [DIRECTORY / f"surface_entry_rail_only_checks-{index}.py" for index in (1, 2, 3)]
    for index, source in enumerate(products):
        target = retained / f"{index:02d}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(previous_manifest),
                                      invocation_driver_versions=list(versions.values())), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_rail_renewal_complete=True, actual_capital_complete=False, actual_launch_complete=False,
                foundation=identity(foundation_path), configurations=summaries, native_processes=len(processes), products=identity(manifest),
                cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240, source=identity(Path(__file__)))


def main():
    """Publish current exact annual replay or the genuine rail-only/healthy-road handoff."""
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
