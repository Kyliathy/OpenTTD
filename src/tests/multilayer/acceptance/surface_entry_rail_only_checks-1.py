"""Renew original due rail heads with every real road service and wagon intact.

@file surface_entry_rail_only_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQG.
The existing compiled original three-rail producer owns empty delivery, order
copies, head-only sales and recoupling. No healthy road carrier is replaced.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from surface_regional_ledger import TABLE, observe, lives

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_coordinated_checks-1.py"))
ORDERS = runpy.run_path(str(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"))["all_orders"]
POLICY = runpy.run_path(str(DIRECTORY / "surface_entry_regional_checks-3.py"))["route_policy"]


def roads(path):
    """@param path Actual native world. @return All eight healthy road service identities, models, cargo capacities and commercial policies."""
    routes = POLICY(ORDERS(path)); vehicles = TABLE(path, b"VEHS"); result = {}; prefix = "roadveh[0].common[0]."
    # Only actual train heads are selected. Every named passenger/freight road
    # body retains its independent original owner, useful capacity and policy.
    for name, route in routes.items():
        row = vehicles[route["vehicle"]]
        if row["type"] != 1:
            continue
        result[name] = dict(route=route, engine=row[prefix + "engine_type"], cargo=row[prefix + "cargo_type"], capacity=row[prefix + "cargo_cap"])
    assert len(result) == 8
    return result


def execute(configuration, phase, receipt, recover_run=None):
    """@param configuration Actual focused build. @param phase Original pure, recall, work, purchase or annual boundary.
    @param receipt Exact selected immediate predecessor. @param recover_run Successful existing native invocation to inspect without repeating.
    @return Complete real material/financial/native evidence with all32wagons and eight road services conserved.
    """
    operation = BASE["execute"](configuration, "rail", phase, receipt, recover_run, observe_fn=observe, with_steel=False)
    before, output = [ROOT / operation[field]["path"] for field in ("before", "output")]
    assert roads(before) == roads(output)
    assert BASE["wagons"](before, operation["initial"]) == BASE["wagons"](output, operation["final"])
    old, new = ORDERS(before), ORDERS(output); selected = [name for name in old if name.startswith("Surface Income ")]; assert len(selected) == 3
    if phase not in ("work", "annual"):
        # Same-tick rail commands cannot alter a single healthy road vehicle.
        # Actual road motion, age and unflagged estimates may evolve only during work.
        a, b = TABLE(before, b"VEHS"), TABLE(output, b"VEHS")
        assert all(b[key] == row for key, row in a.items() if row["type"] == 1)
    if phase == "buy":
        # CO_COPY preserves every actual commercial/timetable field; deleting
        # only the third temporary depot halt leaves original first-two orders exact.
        for name in selected:
            assert old[name]["count"] == 3 and new[name]["count"] == 2
            assert old[name]["orders"] == new[name]["orders"] and old[name]["vehicle"] != new[name]["vehicle"]
    service = lives(output); assert len(service) == 11 and min(row["remaining"] for row in service) > 0
    native = ROOT / operation["retained_harness"]["path"]; retained = native.with_name("rail-only-driver.py")
    if retained.exists():
        assert retained.read_bytes() == Path(__file__).read_bytes()
    else:
        retained.write_bytes(Path(__file__).read_bytes())
    operation["service_lives"] = service; operation["healthy_roads"] = roads(output); operation["retained_wrapper"] = identity(retained)
    operation["sources"] += [identity(Path(__file__)), identity(DIRECTORY / "surface_regional_ledger.py"),
                             identity(DIRECTORY / "surface_entry_regional_renewal_checks-1.py"), identity(DIRECTORY / "surface_entry_regional_checks-3.py")]
    return operation


def main():
    """Publish one actual original rail-only producer boundary without repeating successful native work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("query", "recall", "work", "buy", "annual"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
