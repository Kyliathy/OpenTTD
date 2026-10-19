"""Characterize current literal fuel and finance prerequisites from original owners.

@file surface_entry_supply_admission_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CH.1.
This read-only membership supplies no cargo, money, progress or hypothetical
income. Fresh native prices and complete work/traffic capital remain required.
"""
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe
from surface_propellant_ledger import FACTORY, TABLE, TRAIN, ROAD

DIRECTORY = Path(__file__).resolve().parent
FOUNDATION = ROOT / "logs/surface-entry-component-delivery-reload-verification.json"
STOCK = runpy.run_path(str(DIRECTORY / "surface_entry_expansion_checks-1.py"))["station_stock"]


def characterize(entry):
    """@param entry Exact accepted original stored-component continuation for one build.
    @return Actual producer buffers, body ages/capacities/profits, station packets and current known funding/fuel deficits without simulated work.
    """
    receipt = ROOT / entry["receipt"]["path"]; assert identity(receipt) == entry["receipt"]
    operation = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
    path = ROOT / operation["output"]["path"]; assert identity(path) == operation["output"] == entry["output"]
    current = observe(path); assert json.loads(json.dumps(current)) == operation["final"] and current["components_at_center"]
    labels = FACTORY["observe_factory"](path)["cargo_labels"]; industries, bodies = [], []
    # Each actual industry buffer owns its still-available production. Past
    # output counters and station packets are separate and cannot be added
    # again as chemical or fuel inputs to the next literal factory producer.
    for key, row in TABLE(path, b"INDY").items():
        buffers = {}
        for field in ("accepted", "produced"):
            buffers[field] = {labels[row[f"{field}[{index}].cargo"]]: row[f"{field}[{index}].waiting"] for index in range(row[field + ".count"])}
        industries.append(dict(industry=key, type=row["type"], tile=row["location.tile"], founder=row["founder"], **buffers))
    # Include parked reserves as well as useful services: an old chemical
    # tanker or fuel carrier may require actual paid renewal before transport.
    # Fixed-point native profits are observations, never projected revenue.
    for key, row in TABLE(path, b"VEHS").items():
        if row["type"] not in (0, 1, 6):
            continue
        prefix = TRAIN if row["type"] == 0 else ROAD if row["type"] == 1 else "space[0].common[0]."
        bodies.append(dict(vehicle=key, type=row["type"], name=bytes.fromhex(row[prefix + "name"]).decode("utf-8"),
                           engine=row[prefix + "engine_type"], label=labels.get(row[prefix + "cargo_type"], "INVALID"), capacity=row[prefix + "cargo_cap"],
                           held=sum(row[prefix + "cargo.action_counts"]), tile=row[prefix + "tile"], stopped=bool(row[prefix + "vehstatus"] & 2),
                           age=row[prefix + "age"], life=row[prefix + "max_age"], remaining=row[prefix + "max_age"] - row[prefix + "age"],
                           profit_this_year_fixed_256=row[prefix + "profit_this_year"], profit_last_year_fixed_256=row[prefix + "profit_last_year"]))
    company, = TABLE(path, b"PLYR").values(); funding, = operation["rows"]["funding"]
    assert funding["ceiling"] == 5000000 and company["current_loan"] == 5000000
    known = dict(modules=340000, missions=10000, pad=3000, work=7000, future_module_reserve=150000); base = sum(known.values())
    return dict(configuration=entry["configuration"], receipt=entry["receipt"], output=operation["output"], company=current["company"],
                original_totals=current["totals"], additional_collected=current["additional_collected"], actual_unavailable_abandonment=current["cargo_abandoned"],
                core=current["core"], panel=current["panel"], module_inputs={"STEL": 16, "GOOD": 28}, missing_common_materials={"STEL": 0, "GOOD": 0},
                minimum_flight_fuel=30, current_fuel=12, minimum_additional_fuel=18, minimum_two_unit_batches=9,
                known_module_bill=known, known_module_bill_total=base, last_native_ceiling=funding["ceiling"],
                known_module_bill_shortfall=base - company["money"], full_fuel_logistics_bill_complete=False, native_prices_currently_queried=False,
                collection_working_reserve=100000, industries=industries, conventional_and_space_bodies=bodies,
                station_stock=STOCK(path, labels), service_lives=current["service_lives"], clock=TABLE(path, b"DATE")[0],
                yearly_expenses=company["yearly_expenses"], native_economy={key: value for key, value in company.items() if "income" in key or "expenses" in key})


def main():
    """Publish exact both-build current prerequisite membership without native reruns or edits to any accepted artifact."""
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["retained_products"] == 94
    configurations = [characterize(entry) for entry in foundation["configurations"]]
    first, second = [{key: value for key, value in row.items() if key not in ("configuration", "receipt", "output")} for row in configurations]
    assert first == second
    output = ROOT / "logs/surface-entry-supply-admission-membership-verification.json"; assert not output.exists()
    result = dict(_documentation=__doc__, complete=True, admission_only=True, funded_expansion=False, fuel_procured=False,
                  foundation=identity(FOUNDATION), configurations=configurations, source=identity(Path(__file__)),
                  sources=[identity(DIRECTORY / "surface_entry_expansion_checks-1.py")] + [identity(ROOT / name) for name in
                           ("src/tests/surface_entry_resupply.cpp", "src/tests/surface_entry_gateway.cpp", "src/space_vehicle.cpp", "src/multilayer_space_content.cpp")])
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
