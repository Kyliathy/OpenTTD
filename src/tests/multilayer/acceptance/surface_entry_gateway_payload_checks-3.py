"""Prove ordinary original ground payload/fuel and retain exact tested preflight products.

@file surface_entry_gateway_payload_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQN.
Copied worlds prove equality; only original service supplies actual funds.
The real first flight remains unstarted through this entire ground boundary.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_payload_ledger import observe

DIRECTORY = Path(__file__).resolve().parent
DRIVER = runpy.run_path(str(DIRECTORY / "surface_entry_gateway_payload_checks-1.py"))
BIND = runpy.run_path(str(DIRECTORY / "surface_entry_gateway_payload_checks-2.py"))
RESTORE = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))
STEM = "surface-entry-gateway-payload"
FOUNDATION = ROOT / "logs/surface-entry-gateway-payload-ready-verification.json"
PRODUCTS = ROOT / "test-cache/surface-entry/gateway-assembly-products/retained.json"


def load(path):
    """@param path Accepted immutable receipt. @return Complete receipt object."""
    value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"]
    return value


def reload(configuration):
    """@param configuration Actual independently built ordinary/focused pair. @return Exact paused restore and original/copy3000tick ground continuation."""
    predecessor = ROOT / f"logs/{STEM}-flight-query-{configuration}.json"
    original = load(predecessor)["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    restored, operation = RESTORE["ORDINARY"](configuration, source, STEM + "-" + configuration.lower())
    RESTORE["REVISION"](source, restored); pause = RESTORE["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored)
    restore_path = ROOT / f"logs/{STEM}-restored-{configuration}.json"
    RESTORE["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", operation=operation, original_pause=pause))
    worlds, rows, receipts = [], [], []
    # Both real native branches retain reciprocal original carry references,
    # the constructor's freight and rocket fuel without beginning any flight.
    for receipt, suffix in ((predecessor, "original"), (restore_path, "ordinary")):
        current = DRIVER["execute"](configuration, "payload-service", receipt)
        target = ROOT / f"logs/{STEM}-continued-{configuration}-{suffix}.json"
        RESTORE["publish"](target, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="payload-service", operation=current))
        receipts.append(identity(target)); worlds.append(chunks(ROOT / current["output"]["path"])[1]); rows.append(current["rows"])
        print(configuration, suffix, current["rows"]["earned"], flush=True)
    assert rows[0] == rows[1] and worlds[0].keys() == worlds[1].keys()
    assert all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    selected = load(ROOT / receipts[0]["path"])["operation"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, source_receipt=identity(predecessor), restore=identity(restore_path),
                continuations=receipts, output=selected["output"], company=selected["final"]["company"], additional_continuation_ticks=3000,
                source=identity(Path(__file__)))


def final():
    """@return Complete original loaded ground payload, ordinary/exact useful service and immutable currently tested native/observer products."""
    foundation = load(FOUNDATION); current = BIND["bind"]("ready")
    assert current["configurations"] == foundation["configurations"] and current["native_processes"] == 6
    summaries, worlds, rows, processes, applications = [], [], [], set(), {}
    # Whole ordinary restoration and both exact continuations independently
    # retain every real paid hull and packet owner, including variable SPSU
    # compartments and the already issued native carry generations.
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; path = ROOT / f"logs/{STEM}-reload-{configuration}.json"; value = load(path)
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
            output = ROOT / operation["output"]["path"]; snapshot = observe(output)
            assert json.loads(json.dumps(snapshot)) == operation["final"] and DRIVER["loaded"](snapshot)
            assert snapshot["totals"] == {"STEL": 20, "GOOD": 26, "PROP": 42} and snapshot["embodied_inputs"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
            stages.append(chunks(output)[1]); comparable.append(operation["rows"])
            processes.update(item["path"] for item in operation["native_processes"])
            applications[operation["application"]["path"]] = operation["application"]
        assert comparable[0] == comparable[1] and min(row["remaining"] for row in snapshot["service_lives"]) > 732
        company = snapshot["company"]
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company, global_period=40,
                              spendable=company["cash"] + 5000000 - company["debt"], totals=snapshot["totals"], embodied_inputs=snapshot["embodied_inputs"],
                              paid_craft=snapshot["paid_craft"], manifests=snapshot["manifests"], facility=snapshot["facility"], service_lives=snapshot["service_lives"],
                              original_wagons=32, additional_continuation_ticks=3000, reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == 6 and len(applications) == 2
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
    previous = json.loads(PRODUCTS.read_text(encoding="utf-8")); copies = []
    changed = {"src/tests/surface_entry_gateway.cpp", "src/tests/multilayer/acceptance/surface_gateway_material_ledger.py",
               "src/tests/multilayer/acceptance/surface_gateway_assembly_ledger.py"}
    replaced = set(applications) | changed
    # Replace only the current tested producer, its two focused applications
    # and explicitly extended shared observers. Earlier strict modes and all
    # other current products must remain exact, with every old copy retained.
    for product in previous["products"]:
        assert identity(ROOT / product["retained"]["path"]) == product["retained"]
        if product["source"]["path"] not in replaced:
            assert identity(ROOT / product["source"]["path"]) == product["source"]; copies.append(product)
    retained = ROOT / "test-cache/surface-entry/gateway-payload-products"; assert not retained.exists(); retained.mkdir(parents=True)
    sources = [ROOT / path for path in applications] + [ROOT / path for path in sorted(changed)]
    sources += [DIRECTORY / name for name in ("surface_gateway_native.py", "surface_gateway_payload_ledger.py")]
    sources += [DIRECTORY / f"surface_entry_gateway_payload_checks-{index}.py" for index in (1, 2, 3)]
    for index, source in enumerate(sources):
        target = retained / f"{index:02d}-{source.parent.name}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, predecessor=identity(PRODUCTS)), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_ground_payload_complete=True, actual_flight_admitted=True, actual_launch_complete=False,
                foundation=identity(FOUNDATION), configurations=summaries, native_processes=current["native_processes"] + len(processes), products=identity(manifest),
                cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240, source=identity(Path(__file__)))


def main():
    """Retain an actual ordinary preflight boundary or its complete current both-build handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    value = reload(args.configuration) if args.kind == "reload" else final()
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
