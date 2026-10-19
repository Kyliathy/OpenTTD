"""Prove renewed useful service through ordinary reload and exact annual continuation.

@file surface_entry_coordinated_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEP.3.
Every useful route earns original deliveries. Exact orders, all32original wagons,
healthy vehicles and complete launch stock survive native renewal and replay.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import TABLE, observe

DIRECTORY = Path(__file__).resolve().parent
RELOAD = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))
DRIVER = runpy.run_path(str(DIRECTORY / "surface_entry_coordinated_checks-1.py"))["execute"]
LIVES = runpy.run_path(str(DIRECTORY / "surface_entry_capital_checks-1.py"))["lives"]


def service_orders(path):
    """@param path Complete native save. @return Actual eight useful vehicle identities and their first two native orders."""
    orders = TABLE(path, b"ORDL"); vehicles = TABLE(path, b"VEHS"); result = {}
    for service in LIVES(path):
        key = service["vehicle"]; vehicle = vehicles[key]
        prefix = "train[0].common[0]." if vehicle["type"] == 0 else "roadveh[0].common[0]."
        assert not vehicle[prefix + "vehstatus"] & 128  # Native VehState::Crashed must not hide behind a retained name.
        reference = vehicle[prefix + "orders"]; assert reference > 0
        row = orders[reference - 1]; assert row["orders.count"] >= 2
        route = []
        # Follow the native list, preserving every serialized first-two-order
        # field rather than reconstructing service from an old station number.
        for index in range(2):
            field = f"orders[{index}]."
            route.append({name.removeprefix(field): value for name, value in row.items() if name.startswith(field)})
        result[service["name"]] = dict(vehicle=key, count=row["orders.count"], orders=route)
    assert len(result) == 8
    return result


def purchase_routes(configuration):
    """@param configuration Actual focused build. Verify native copies and only the explicitly restored pickup loading policy."""
    original_path = ROOT / f"logs/surface-entry-coordinated-rail-query-{configuration}.json"
    original = json.loads(original_path.read_text(encoding="utf-8"))["operation"]
    baseline = service_orders(ROOT / original["source"]["path"])
    for actor in ("rail", "steel"):
        receipt = json.loads((ROOT / f"logs/surface-entry-coordinated-{actor}-buy-{configuration}.json").read_text(encoding="utf-8"))["operation"]
        first, last = [service_orders(ROOT / receipt[field]["path"]) for field in ("before", "output")]
        names = [name for name in first if name.startswith("Surface Income ")] if actor == "rail" else ["Surface Steel delivery"]
        for name in names:
            old, new = first[name], last[name]; assert old["count"] == 3 and new["count"] == 2
            expected = json.loads(json.dumps(old["orders"]))
            if actor == "steel":
                # NoLoad suppressed new pickup only for genuine empty return;
                # restore exactly the original pickup flags, retaining all
                # actual timetable, receiver, refit and stopping fields.
                expected[0]["flags"] = baseline[name]["orders"][0]["flags"]
            assert expected == new["orders"]


def reload(configuration):
    """@param configuration Actual ordinary/focused build pair.
    @return Ordinary ownership restoration and exact27000tick original/resaved useful-service proof.
    """
    purchase_routes(configuration)
    predecessor = ROOT / f"logs/surface-entry-coordinated-steel-buy-{configuration}.json"
    value = json.loads(predecessor.read_text(encoding="utf-8")); source = ROOT / value["operation"]["output"]["path"]
    assert identity(source) == value["operation"]["output"] and all(row["remaining"] > 732 for row in LIVES(source))
    restored, operation = RELOAD["ORDINARY"](configuration, source, "surface-coordinated-restore-" + configuration.lower())
    RELOAD["REVISION"](source, restored); pause = RELOAD["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored) and service_orders(source) == service_orders(restored)
    restore_path = ROOT / f"logs/surface-entry-coordinated-restored-{configuration}.json"
    RELOAD["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", operation=operation, original_pause=pause))
    branches, rows, receipts = [], [], []
    # An entire actual year lets all three commercial consists complete
    # their full-load cycles. Only the selected original branch earns capital;
    # the independent ordinary branch proves identical native behavior.
    for path, suffix in ((predecessor, "original"), (restore_path, "ordinary")):
        continuation = DRIVER(configuration, "rail", "annual", path)
        delivered = {row["label"] for row in continuation["rows"]["delivered"] if row["amount"] > 0}
        assert {"PASS", "COAL", "GOOD", "GRAI", "LVST", "IORE", "STEL"} <= delivered
        assert all(row["count"] == 2 for row in service_orders(ROOT / continuation["output"]["path"]).values())
        receipt = ROOT / f"logs/surface-entry-coordinated-continued-{configuration}-{suffix}.json"
        RELOAD["publish"](receipt, dict(_documentation=__doc__, complete=True, configuration=configuration, actor="rail", phase="annual", operation=continuation))
        receipts.append(identity(receipt)); branches.append(chunks(ROOT / continuation["output"]["path"])[1]); rows.append(continuation["rows"])
        print(configuration, suffix, continuation["rows"]["earned"], flush=True)
    assert rows[0] == rows[1] and branches[0].keys() == branches[1].keys()
    assert all(branches[0][key] == branches[1][key] for key in branches[0] if key != b"GLOG")
    selected = json.loads((ROOT / receipts[0]["path"]).read_text(encoding="utf-8"))["operation"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, global_period=17, restore=identity(restore_path), continuations=receipts,
                output=selected["output"], company=selected["final"]["company"], service_lives=LIVES(ROOT / selected["output"]["path"]), rows=rows[0],
                native_processes=3, exact_native_replay_gameplay_comparisons=120)


def final():
    """@return Complete both-build paid service, exact annual replay and immutable current tested products."""
    foundation_path = ROOT / "logs/surface-entry-coordinated-custody-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["actual_coordinated_service_complete"]
    worlds, rows, summaries, processes, driver_versions = [], [], [], set(), {}
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; path = ROOT / f"logs/surface-entry-coordinated-reload-{configuration}.json"
        value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["global_period"] == 17
        restore = json.loads((ROOT / value["restore"]["path"]).read_text(encoding="utf-8"))["operation"]; assert restore["source"] == entry["output"]
        original, resaved = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(resaved); normalized[b"DATE"] = original[b"DATE"]
        stages, comparable = [normalized], []
        processes.update(reference["path"] for reference in restore["native_processes"])
        for reference in entry["receipts"] + value["continuations"]:
            receipt = ROOT / reference["path"]; assert identity(receipt) == reference
            current = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
            retained = current["retained_harness"]; assert identity(ROOT / retained["path"]) == retained
            driver_versions[retained["sha256"]] = retained
        for reference in value["continuations"]:
            continuation = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / continuation[field]["path"]) == continuation[field]
            stages.append(chunks(ROOT / continuation["output"]["path"])[1]); comparable.append(continuation["rows"])
            processes.update(reference["path"] for reference in continuation["native_processes"])
        assert comparable[0] == comparable[1] and all(stages[1][key] == stages[2][key] for key in stages[1] if key != b"GLOG")
        company = value["company"]
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              global_period=17, spendable=company["cash"] + 5000000 - company["debt"], service_lives=value["service_lives"],
                              delivered=value["rows"]["delivered"], annual_work=value["rows"]["earned"], reload_receipt=identity(path)))
        worlds.append(stages); rows.append(comparable)
    assert rows[0] == rows[1] and len(processes) == 6
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    retained = ROOT / "test-cache/surface-entry/coordinated-service-products"; assert not retained.exists(); retained.mkdir(parents=True)
    products = [ROOT / "src/tests" / name for name in ("surface_entry_renewal.cpp", "surface_entry_road_renewal.cpp", "surface_entry_work.h", "CMakeLists.txt")]
    products += [ROOT / "CMakeLists.txt"] + [DIRECTORY / f"surface_entry_coordinated_checks-{index}.py" for index in (1, 2, 3)]
    products += [DIRECTORY / name for name in ("surface_fuel_refill_ledger.py", "surface_propellant_ledger.py", "surface_entry_fuel_refill_checks-3.py")]
    products += [ROOT / "build" / configuration / "openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")]
    copies = []
    # Keep the exact current native binaries and sources before any later
    # producer changes; earlier invocation copies retain every tested driver version.
    for index, source in enumerate(products):
        target = retained / f"{index:02d}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    binding_source = ROOT / "test-cache/surface-entry/coordinated-source-versions/before-annual-surface_entry_coordinated_checks-2.py"
    assert identity(binding_source)["sha256"] == foundation["source"]["sha256"]
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, invocation_driver_versions=list(driver_versions.values()),
                                        custody_binding_source=identity(binding_source)), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_coordinated_service_complete=True, actual_capital_complete=False, actual_launch_complete=False,
                foundation=identity(foundation_path), native_processes=6, cross_build_gameplay_comparisons=360, exact_native_replay_gameplay_comparisons=240,
                configurations=summaries, retained=identity(manifest), source=identity(Path(__file__)))


def main():
    """Publish one focused ordinary annual replay pair or its final both-build handoff."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--kind", choices=("reload", "final"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug")); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    value = reload(args.configuration) if args.kind == "reload" else final()
    RELOAD["publish"](output, value); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
