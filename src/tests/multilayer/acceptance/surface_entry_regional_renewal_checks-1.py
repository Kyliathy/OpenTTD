"""Renew only due local buses in the genuine eleven-service regional world.

@file surface_entry_regional_renewal_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQC.
The existing compiled native command producer owns recall, empty arrival,
purchase, sale and order restoration. All eight healthy services remain useful.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from surface_regional_ledger import TABLE, observe, lives

DIRECTORY = Path(__file__).resolve().parent
BUS = runpy.run_path(str(DIRECTORY / "surface_entry_bus_checks-1.py"))["execute"]
REGIONAL = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))


def all_orders(path):
    """@param path Actual native world, including temporary local recall orders.
    @return Every first-two order field for all eleven useful services, without borrowing a temporary local policy for regional buses.
    """
    vehicles, orders, result = TABLE(path, b"VEHS"), TABLE(path, b"ORDL"), {}
    # Follow each real owned order list independently. During recall the local
    # pickup flags differ intentionally from the healthy regional bus policy.
    for service in lives(path):
        row = vehicles[service["vehicle"]]; prefix = "train[0].common[0]." if row["type"] == 0 else "roadveh[0].common[0]."
        assert not row[prefix + "vehstatus"] & 128 and row[prefix + "owner"] == 0
        reference = row[prefix + "orders"]; assert reference > 0; native = orders[reference - 1]; assert native["orders.count"] >= 2
        route = []
        for index in range(2):
            field = f"orders[{index}]."; route.append({name.removeprefix(field): value for name, value in native.items() if name.startswith(field)})
        result[service["name"]] = dict(vehicle=service["vehicle"], count=native["orders.count"], orders=route)
    assert len(result) == 11
    return result


def healthy(path):
    """@param path Actual native world. @return All eight healthy service identities and commercial policies, excluding due local buses."""
    routes = REGIONAL["route_policy"](all_orders(path))
    result = {name: row for name, row in routes.items() if not name.startswith("Surface Launch passenger ")}
    assert len(result) == 8
    return result


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Independently linked actual focused build.
    @param phase Original pure, recall, work, renewal or short service boundary.
    @param receipt Exact selected predecessor. @param recover_run Successful existing native run to inspect without repeating.
    @return Exact complete material/financial/native evidence and preserved healthy services.
    """
    operation = BUS(configuration, phase, receipt, recover_run, observe_fn=observe)
    before, output = [ROOT / operation[field]["path"] for field in ("before", "output")]
    first, last = healthy(before), healthy(output); assert first == last
    old, new = [all_orders(path) for path in (before, output)]
    local = [name for name in old if name.startswith("Surface Launch passenger ")]; assert len(local) == 3
    if phase not in ("work", "earn"):
        # Same-tick local commands cannot move or rewrite any healthy native
        # vehicle or wagon. Real work may change their poses and time estimates.
        excluded = {old[name]["vehicle"] for name in local}; a, b = TABLE(before, b"VEHS"), TABLE(output, b"VEHS")
        assert all(b[key] == row for key, row in a.items() if key not in excluded)
    if phase == "renew":
        query = ROOT / f"logs/surface-entry-local-renewal-query-{configuration}.json"
        baseline = json.loads(query.read_text(encoding="utf-8"))["operation"]
        originals = all_orders(ROOT / baseline["output"]["path"])
        # CO_COPY retains every actual current order/timetable field. Only the
        # temporary depot halt is deleted and original pickup flags restored.
        for name in local:
            assert old[name]["count"] == 3 and new[name]["count"] == 2
            expected = json.loads(json.dumps(old[name]["orders"]))
            for index in range(2):
                expected[index]["flags"] = originals[name]["orders"][index]["flags"]
            assert expected == new[name]["orders"] and old[name]["vehicle"] != new[name]["vehicle"]
        assert all(row["age"] == 0 for row in lives(output) if row["name"] in local)
    service = lives(output); assert len(service) == 11 and min(row["remaining"] for row in service) > 0
    retained = ROOT / operation["retained_harness"]["path"]; wrapper = retained.with_name("regional-renewal-driver.py")
    if wrapper.exists():
        assert wrapper.read_bytes() == Path(__file__).read_bytes()
    else:
        wrapper.write_bytes(Path(__file__).read_bytes())
    operation["service_lives"] = service; operation["healthy_services"] = last; operation["retained_wrapper"] = identity(wrapper)
    operation["sources"] += [identity(Path(__file__)), identity(DIRECTORY / "surface_regional_ledger.py"), identity(DIRECTORY / "surface_entry_regional_checks-3.py")]
    return operation


def main():
    """Publish one genuinely selected local renewal boundary with explicit regional observation."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("query", "recall", "work", "renew", "earn"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
