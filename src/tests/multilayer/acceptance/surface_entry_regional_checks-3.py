"""Verify ordinary regional-service restoration, exact continuation and retained products.

@file surface_entry_regional_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQA.
The selected original branch alone earns funds. Ordinary copies prove identical
native behavior and preserve all eleven actual useful services and launch stock.
"""
import argparse
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE, observe, lives

DIRECTORY = Path(__file__).resolve().parent
RELOAD = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))
DRIVER = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-1.py"))["execute"]
ORIGINAL_ORDERS = runpy.run_path(str(DIRECTORY / "surface_entry_coordinated_checks-3.py"))["service_orders"]
TOWNS = runpy.run_path(str(DIRECTORY / "surface_entry_treasury_checks-1.py"))["towns"]


def route_policy(services):
    """@param services Full native service orders. @return Commercial/timetabled policy, excluding only untimetabled native estimates.
    UpdateVehicleTimetable updates measured wait/travel values during real service;
    Order::IsWaitTimetabled/IsTravelTimetabled flags3/7 retain explicit constraints.
    Full ordinary and cross-build gameplay comparisons still include every byte.
    """
    result = json.loads(json.dumps(services))
    # Only genuine ordinary station orders may use these native estimate
    # semantics; conditional or explicitly timetabled fields remain exact.
    for service in result.values():
        for order in service["orders"]:
            assert order["type"] & 31 == 1
            if not order["flags"] & 8:
                del order["wait_time"]
            if not order["flags"] & 128:
                del order["travel_time"]
    return result


def service_orders(path):
    """@param path Actual native save. @return Exact first-two native orders of all eleven genuinely owned useful services."""
    result = ORIGINAL_ORDERS(path); orders, vehicles = TABLE(path, b"ORDL"), TABLE(path, b"VEHS")
    snapshot = observe(path); assert len(snapshot["regional_buses"]) == 3
    expected = route_policy(result)["Surface Launch passenger 1"]["orders"]
    for bus in snapshot["regional_buses"]:
        row = vehicles[bus["id"]]; prefix = "roadveh[0].common[0]."; reference = row[prefix + "orders"]
        assert reference > 0 and not row[prefix + "vehstatus"] & 128; native = orders[reference - 1]; assert native["orders.count"] == 2
        route = []
        # The genuine station IDs and every native commercial order field
        # must survive reload. New buses use the original normal PASS policy.
        for index, destination in enumerate((17, 20)):
            field = f"orders[{index}]."; current = {name.removeprefix(field): value for name, value in native.items() if name.startswith(field)}
            assert current["dest"] == destination
            policy = route_policy({"new": dict(orders=[current])})["new"]["orders"][0]
            assert {k: v for k, v in policy.items() if k != "dest"} == {k: v for k, v in expected[index].items() if k != "dest"}
            route.append(current)
        result[bus["name"]] = dict(vehicle=bus["id"], count=2, orders=route)
    assert len(result) == 11
    return result


def reload(configuration):
    """@param configuration Actual ordinary/focused native build pair.
    @return Preserved whole-world ordinary owners and exact3000tick selected/copy continuation.
    """
    predecessor = ROOT / f"logs/surface-entry-regional-earn-{configuration}.json"
    value = json.loads(predecessor.read_text(encoding="utf-8")); operation = value["operation"]
    source = ROOT / operation["output"]["path"]; assert identity(source) == operation["output"]
    profit, = operation["rows"]["profit"]; earned, = operation["rows"]["earned"]
    assert profit["amount"] > 0 and all(row["amount"] > 0 for row in operation["rows"]["vehicle_profit"])
    assert earned["earned"] == 27000 and earned["cash"] > earned["cash_before"]
    towns = TOWNS(source); assert towns[1]["passenger_transported"] > 0
    old_path = ROOT / f"logs/surface-entry-coordinated-continued-{configuration}-original.json"
    old = json.loads(old_path.read_text(encoding="utf-8"))["operation"]
    old_routes = ORIGINAL_ORDERS(ROOT / old["output"]["path"])
    assert route_policy(old_routes) == route_policy(ORIGINAL_ORDERS(source))
    restored, native = RELOAD["ORDINARY"](configuration, source, "surface-regional-restore-" + configuration.lower())
    RELOAD["REVISION"](source, restored); pause = RELOAD["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observe(source) == observe(restored) and service_orders(source) == service_orders(restored)
    restore = ROOT / f"logs/surface-entry-regional-restored-{configuration}.json"
    RELOAD["publish"](restore, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore", operation=native, original_pause=pause))
    branches, rows, receipts = [], [], []
    # Only the selected original continuation contributes capital. A separate
    # ordinary continuation must reproduce every native gameplay owner exactly.
    for receipt, suffix in ((predecessor, "original"), (restore, "ordinary")):
        continuation = DRIVER(configuration, "short", receipt)
        output = ROOT / continuation["output"]["path"]
        assert route_policy(service_orders(source)) == route_policy(service_orders(output))
        target = ROOT / f"logs/surface-entry-regional-continued-{configuration}-{suffix}.json"
        RELOAD["publish"](target, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="short", operation=continuation))
        receipts.append(identity(target)); branches.append(chunks(output)[1]); rows.append(continuation["rows"])
        print(configuration, suffix, continuation["rows"]["earned"], flush=True)
    assert rows[0] == rows[1] and branches[0].keys() == branches[1].keys()
    assert all(branches[0][key] == branches[1][key] for key in branches[0] if key != b"GLOG")
    selected = json.loads((ROOT / receipts[0]["path"]).read_text(encoding="utf-8"))["operation"]
    return dict(_documentation=__doc__, complete=True, configuration=configuration, global_period=18, partial_continuation_ticks=3000,
                restore=identity(restore), continuations=receipts, output=selected["output"], company=selected["final"]["company"],
                service_lives=lives(ROOT / selected["output"]["path"]), towns=towns, rows=rows[0], native_processes=3,
                exact_native_replay_gameplay_comparisons=120)


def final():
    """@return Complete measured useful regional service and exact current retained native products, without closing unfunded launch work."""
    foundation_path = ROOT / "logs/surface-entry-regional-annual-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    summaries, worlds, processes, versions = [], [], set(), {}
    for configuration in ("RelWithDebInfo", "Debug"):
        path = ROOT / f"logs/surface-entry-regional-reload-{configuration}.json"
        value = json.loads(path.read_text(encoding="utf-8")); assert value["complete"] and value["global_period"] == 18
        restore = json.loads((ROOT / value["restore"]["path"]).read_text(encoding="utf-8"))["operation"]
        original, resaved = [chunks(ROOT / restore[field]["path"])[1] for field in ("source", "output")]
        normalized = dict(resaved); normalized[b"DATE"] = original[b"DATE"]; stages = [normalized]
        processes.update(reference["path"] for reference in restore["native_processes"])
        # Bind every actual paid/earning predecessor; pure civic refusal did
        # not authorize spending and its preserved world remains in this chain.
        for phase in ("civic-query", "query", "infrastructure", "buy", "earn"):
            receipt = ROOT / f"logs/surface-entry-regional-{phase}-{configuration}.json"
            previous = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
            for reference in previous["native_processes"]:
                assert identity(ROOT / reference["path"]) == reference; processes.add(reference["path"])
            retained = previous["retained_harness"]; assert identity(ROOT / retained["path"]) == retained; versions[retained["sha256"]] = retained
        comparable = []
        for reference in value["continuations"]:
            assert identity(ROOT / reference["path"]) == reference
            current = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))["operation"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                assert identity(ROOT / current[field]["path"]) == current[field]
            stages.append(chunks(ROOT / current["output"]["path"])[1]); comparable.append(current["rows"])
            processes.update(item["path"] for item in current["native_processes"])
            retained = current["retained_harness"]; versions[retained["sha256"]] = retained
        assert comparable[0] == comparable[1] and all(stages[1][key] == stages[2][key] for key in stages[1] if key != b"GLOG")
        company = value["company"]
        summaries.append(dict(configuration=configuration, receipt=value["continuations"][0], output=value["output"], company=company,
                              global_period=18, partial_continuation_ticks=3000, spendable=company["cash"] + 5000000 - company["debt"],
                              service_lives=value["service_lives"], towns=value["towns"], reload_receipt=identity(path)))
        worlds.append(stages)
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121 and all(first[key] == second[key] for key in first if key != b"GLOG")
    retained = ROOT / "test-cache/surface-entry/regional-service-products"; assert not retained.exists(); retained.mkdir(parents=True)
    products = [ROOT / "src/tests" / name for name in ("surface_entry_regional.cpp", "surface_regional_layout.h", "surface_passenger_layout.h", "surface_chemical_layout.h", "surface_entry_work.h", "CMakeLists.txt")]
    products += [ROOT / "CMakeLists.txt"] + [DIRECTORY / f"surface_entry_regional_checks-{index}.py" for index in (1, 2, 3)]
    products += [DIRECTORY / name for name in ("surface_regional_ledger.py", "surface_fuel_refill_ledger.py", "surface_propellant_ledger.py")]
    products += [ROOT / "build" / configuration / "openttd_surface_supply_test.exe" for configuration in ("RelWithDebInfo", "Debug")]
    copies = []
    # Preserve the exact accepted binaries, route planner and explicit observer
    # before financial continuation or any later native producer can change them.
    for index, source in enumerate(products):
        target = retained / f"{index:02d}-{source.name}"; shutil.copyfile(source, target)
        assert identity(source)["sha256"] == identity(target)["sha256"]; copies.append(dict(source=identity(source), retained=identity(target)))
    manifest = retained / "retained.json"
    manifest.write_text(json.dumps(dict(_documentation=__doc__, products=copies, invocation_driver_versions=list(versions.values())), indent=2) + "\n", encoding="utf-8")
    return dict(_documentation=__doc__, complete=True, actual_regional_service_complete=True, actual_capital_complete=False,
                foundation=identity(foundation_path), configurations=summaries, native_processes=len(processes), products=identity(manifest),
                cross_build_gameplay_comparisons=360, exact_replay_gameplay_comparisons=240, source=identity(Path(__file__)))


def main():
    """Publish focused ordinary/replay proof or the complete measured-service handoff."""
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
