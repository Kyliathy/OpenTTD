"""Renew five imminently due road services through their actual native producers.

@file surface_entry_imminent_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQE.
Explicit fleet selectors preserve all other useful service. Native commands,
empty arrival, actual order copies, payments and real work retain every owner.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from surface_regional_ledger import TABLE, observe, lives

DIRECTORY = Path(__file__).resolve().parent
BUS = runpy.run_path(str(DIRECTORY / "surface_entry_bus_checks-1.py"))["execute"]
ROAD = runpy.run_path(str(DIRECTORY / "surface_entry_road_renewal_checks-1.py"))["execute"]
ORDERS = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))["all_orders"]
POLICY = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))["route_policy"]
WAGONS = runpy.run_path(str(DIRECTORY / "surface_entry_coordinated_checks-1.py"))["wagons"]
FLEETS = dict(freight={"Surface ore delivery", "Surface Steel delivery"},
              regional={f"Surface Regional passenger {index}" for index in range(1, 4)},
              local={f"Surface Launch passenger {index}" for index in range(1, 4)})


def unselected(path, actor):
    """@param path Actual native world. @param actor Explicit fleet receiving native commands.
    @return Every other useful service identity, engine/capacity and commercial policy.
    """
    routes = POLICY(ORDERS(path)); service = {row["name"]: row for row in lives(path)}; vehicles = TABLE(path, b"VEHS"); result = {}
    # A selected fleet cannot disguise another asset through pool reuse;
    # all unselected service keeps its real name, owner, model and capacity.
    for name, route in routes.items():
        if name in FLEETS[actor]:
            continue
        row = vehicles[route["vehicle"]]; prefix = "train[0].common[0]." if row["type"] == 0 else "roadveh[0].common[0]."
        result[name] = dict(route=route, engine=service[name]["engine"], capacity=row[prefix + "cargo_cap"], cargo=row[prefix + "cargo_type"])
    assert len(result) == 11 - len(FLEETS[actor])
    return result


def execute(configuration, actor, phase, receipt, recover_run=None):
    """@param configuration Actual focused build. @param actor Two freight lorries, three regional buses or pure default-local regression.
    @param phase Native pure, recall, work, renewal or short proof boundary.
    @param receipt Exact immediate selected predecessor. @param recover_run Successful native invocation to inspect without repeating.
    @return Full native/material/financial evidence with every unselected useful service protected.
    """
    assert actor in FLEETS and phase in ("query", "recall", "work", "renew", "earn")
    if actor == "local":
        assert phase == "query"
    if actor == "freight":
        operation = ROAD(configuration, phase, receipt, recover_run, observe_fn=observe)
    else:
        operation = BUS(configuration, phase, receipt, recover_run, observe_fn=observe, service=actor)
    before, output = [ROOT / operation[field]["path"] for field in ("before", "output")]
    first, last = unselected(before, actor), unselected(output, actor); assert first == last
    old, new = ORDERS(before), ORDERS(output)
    assert WAGONS(before, operation["initial"]) == WAGONS(output, operation["final"])
    if phase not in ("work", "earn"):
        # A same-tick purchase/recall cannot move or rewrite any other native
        # vehicle or useful wagon. Real work alone may change their actual pose.
        excluded = {old[name]["vehicle"] for name in FLEETS[actor]}; a, b = TABLE(before, b"VEHS"), TABLE(output, b"VEHS")
        assert all(b[key] == row for key, row in a.items() if key not in excluded)
    if phase == "query":
        if actor != "freight":
            assert all(row["last"] == (18 if actor == "local" else 20) for row in operation["rows"]["recalls"])
    if phase == "renew":
        query = ROOT / f"logs/surface-entry-imminent-query-{actor}-{configuration}.json"
        baseline = json.loads(query.read_text(encoding="utf-8"))["operation"]
        originals = ORDERS(ROOT / baseline["output"]["path"])
        # Copy actual current lists exactly, then restore only original loading
        # flags and delete the temporary third depot halt; estimates stay native.
        for name in FLEETS[actor]:
            assert old[name]["count"] == 3 and new[name]["count"] == 2
            expected = json.loads(json.dumps(old[name]["orders"]))
            for index in range(2):
                expected[index]["flags"] = originals[name]["orders"][index]["flags"]
            assert expected == new[name]["orders"] and old[name]["vehicle"] != new[name]["vehicle"]
        assert all(row["age"] == 0 for row in lives(output) if row["name"] in FLEETS[actor])
    service = lives(output); assert len(service) == 11 and min(row["remaining"] for row in service) > 0
    native_harness = ROOT / operation["retained_harness"]["path"]; retained = native_harness.with_name("imminent-service-driver.py")
    if retained.exists():
        assert retained.read_bytes() == Path(__file__).read_bytes()
    else:
        retained.write_bytes(Path(__file__).read_bytes())
    operation["service_lives"] = service; operation["unselected_services"] = last; operation["retained_wrapper"] = identity(retained)
    operation["sources"] += [identity(Path(__file__)), identity(DIRECTORY / "surface_regional_ledger.py"),
                             identity(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"), identity(DIRECTORY / "surface_entry_regional_checks-3.py")]
    return operation


def main():
    """Publish one selected native imminent-service boundary without overwriting genuine work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--actor", choices=FLEETS, required=True); parser.add_argument("--phase", choices=("query", "recall", "work", "renew", "earn"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.actor, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, actor=args.actor, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
