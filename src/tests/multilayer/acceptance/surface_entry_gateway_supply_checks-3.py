"""Prove ordinary original launch-stock storage and retain exact physical supply products.

@file surface_entry_gateway_supply_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQL.
Copied worlds prove equality and supply no funds or material. Original native
storage and every paid commercial train/wagon retain their sole ownership.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_material_ledger import TABLE, observe, REQUIRED

DIRECTORY = Path(__file__).resolve().parent
DRIVER = runpy.run_path(str(DIRECTORY / "surface_entry_gateway_supply_checks-1.py"))
BIND = runpy.run_path(str(DIRECTORY / "surface_entry_gateway_supply_checks-2.py"))
RESTORE = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))
STEM = "surface-entry-gateway-supply"
FOUNDATION = ROOT / "logs/surface-entry-gateway-supply-stored-verification.json"
PRODUCTS = ROOT / "test-cache/surface-entry/paid-center-products/retained.json"


def load(path):
    """@param path Accepted immutable receipt. @return Complete accepted object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def trains(path):
    """@param path Complete actual native world. @return All35original rail identities, chain links, models, capacities and cargo meanings."""
    result = {}
    # Real work may alter pose, loading and native time estimates. None of
    # those changes authorizes replacing an original wagon or its capacity.
    for key, row in TABLE(path, b"VEHS").items():
        if row["type"] != 0:
            continue
        prefix = "train[0].common[0]."
        result[key] = {field: row[prefix + field] for field in ("next", "owner", "engine_type", "cargo_type", "cargo_cap")}
    assert len(result) == 35
    return result


def reload(configuration):
    """@param configuration Independently built ordinary/focused pair. @return Exact ordinary restore and selected original/copy3000tick continuation."""
    source_receipt = ROOT / f"logs/{STEM}-handle-work-1-{configuration}.json"
    original = load(source_receipt)["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    restored, operation = RESTORE["ORDINARY"](configuration, source, STEM + "-" + configuration.lower())
    RESTORE["REVISION"](source, restored); pause = RESTORE["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored) and trains(source) == trains(restored)
    restore_path = ROOT / f"logs/{STEM}-restored-{configuration}.json"
    RESTORE["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", operation=operation, original_pause=pause))
    worlds, rows, receipts = [], [], []
    # Each branch independently earns the same actual native service. The
    # first original branch alone becomes the next selected construction world.
    for predecessor, suffix in ((source_receipt, "original"), (restore_path, "ordinary")):
        current = DRIVER["execute"](configuration, "service", predecessor)
        target = ROOT / f"logs/{STEM}-continued-{configuration}-{suffix}.json"
        RESTORE["publish"](target, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="service", operation=current))
        assert trains(ROOT / current["output"]["path"]) == trains(source)
        receipts.append(identity(target)); worlds.append(chunks(ROOT / current["output"]["path"])[1]); rows.append(current["rows"])
        print(configuration, suffix, current["rows"]["earned"], flush=True)
    assert rows[0] == rows[1] and worlds[0].keys() == worlds[1].keys()
    assert all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    selected = load(ROOT / receipts[0]["path"])["operation"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, source_receipt=identity(source_receipt), restore=identity(restore_path),
                continuations=receipts, output=selected["output"], company=selected["final"]["company"], additional_continuation_ticks=3000,
                source=identity(Path(__file__)))


def final():
    """@return Complete current physical supply, original rail identities, ordinary/exact continuation and immutable tested products."""
    foundation = load(FOUNDATION); current = BIND["bind"]("stored")
    assert current["configurations"] == foundation["configurations"] and current["native_processes"] == 12
    summaries, worlds, rows, processes, applications = [], [], [], set(), {}
    paid = load(BIND["FOUNDATION"])
    # Revalidate every intermediate rail identity against the actual paid
    # facility predecessor; ordinary proof cannot hide an earlier wagon swap.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]
        baseline, = [row for row in paid["configurations"] if row["configuration"] == configuration]
        expected = trains(ROOT / baseline["output"]["path"])
        for reference in entry["receipts"]:
            assert identity(ROOT / reference["path"]) == reference
            operation = load(ROOT / reference["path"])["operation"]
            assert trains(ROOT / operation["output"]["path"]) == expected
        path = ROOT / f"logs/{STEM}-reload-{configuration}.json"; value = load(path)
        assert value["source_receipt"] == entry["receipt"] and value["additional_continuation_ticks"] == 3000
        restore = load(ROOT / value["restore"]["path"])["operation"]
        original, restored = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(restored); normalized[b"DATE"] = original[b"DATE"]; stages, comparable = [normalized], []
        processes.update(reference["path"] for reference in restore["native_processes"])
        for reference in value["continuations"]:
            assert identity(ROOT / reference["path"]) == reference
            operation = load(ROOT / reference["path"])["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                item = operation[field]; assert identity(ROOT / item["path"]) == item
            for item in operation["sources"] + operation["native_processes"]:
                assert identity(ROOT / item["path"]) == item
            output = ROOT / operation["output"]["path"]
            assert json.loads(json.dumps(observe(output))) == operation["final"] and trains(output) == expected
            stages.append(chunks(output)[1]); comparable.append(operation["rows"])
            processes.update(item["path"] for item in operation["native_processes"])
            applications[operation["application"]["path"]] = operation["application"]
        assert comparable[0] == comparable[1]
        snapshot = observe(ROOT / value["output"]["path"]); assert snapshot["totals"] == REQUIRED and not snapshot["handling"]["jobs"]
        assert all(tuple(packet["owner"]) == ("station", snapshot["facility"]["center"]) for packet in snapshot["cohort"])
        assert min(row["remaining"] for row in snapshot["service_lives"]) > 732
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company, global_period=40,
                              spendable=company["cash"] + 5000000 - company["debt"], facility=snapshot["facility"], totals=snapshot["totals"],
                              service_lives=snapshot["service_lives"], original_wagons=32, additional_continuation_ticks=3000, reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == 6 and len(applications) == 2
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    previous = json.loads(PRODUCTS.read_text(encoding="utf-8")); copies = []
    replaced = set(applications) | {"src/tests/surface_entry_gateway.cpp"}
    # This tested producer alone replaces its two focused binaries and native
    # source. All other inherited current products and prior retained bytes
    # must remain exact; earlier closed observers receive no exceptions.
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]; copies.append(product)
    retained = ROOT / "test-cache/surface-entry/gateway-supply-products"; assert not retained.exists(); retained.mkdir(parents=True)
    sources = [ROOT / path for path in applications] + [ROOT / "src/tests/surface_entry_gateway.cpp", DIRECTORY / "surface_gateway_material_ledger.py"]
    sources += [DIRECTORY / f"surface_entry_gateway_supply_checks-{index}.py" for index in (1, 2, 3)]
    for index, source in enumerate(sources):
        target = retained / f"{index:02d}-{source.parent.name}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(PRODUCTS)), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_material_delivery_complete=True, actual_launch_complete=False, foundation=identity(FOUNDATION),
                configurations=summaries, native_processes=current["native_processes"] + len(processes), products=identity(manifest),
                cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240, source=identity(Path(__file__)))


def main():
    """Retain the completed original ordinary storage proof or its fully tested physical supply handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    value = reload(args.configuration) if args.kind == "reload" else final()
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
