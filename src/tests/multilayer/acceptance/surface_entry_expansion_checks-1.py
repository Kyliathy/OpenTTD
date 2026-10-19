"""Characterize genuine current Core expansion inputs before dependent purchases.

@file surface_entry_expansion_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CE.1.
This read-only membership report supplies no owner, packet, money or progress.
Native layout/current price quotes remain the next focused boundary, and missing
physical supplies or unpriced logistics cannot become funded procurement.
"""
import json
from pathlib import Path
import re

from space_preview_checks import ROOT, identity
from surface_gateway_construction_ledger import observe
from surface_propellant_ledger import FACTORY, TABLE, TRAIN, ROAD

FOUNDATION = ROOT / "logs/surface-entry-core-construction-reload-verification.json"
LABELS = {"IORE", "STEL", "GOOD", "SALT", "OIL_", "CHEM", "PROP", "RBPT", "ELEC", "RFMT"}


def station_stock(path, labels):
    """@param path Actual current native world. @param labels Same-file cargo meanings.
    @return Relevant native waiting packets grouped by actual station, label and original source; this metadata owns no inventory.
    """
    packets = TABLE(path, b"CAPA"); result = []
    # Current factory pickup feasibility depends on real waiting packets, not
    # historical output counters. Preserve original provenance/reference IDs
    # rather than treating expired or already consumed production as stock.
    for station, row in TABLE(path, b"STNN").items():
        for field, references in row.items():
            match = re.fullmatch(r"normal\[0\]\.goods\[(\d+)\]\.cargo\[\d+\]\.second", field)
            if match is None:
                continue
            label = labels[int(match.group(1))]
            if label not in LABELS:
                continue
            selected = [dict(packet=reference - 1, **packets[reference - 1]) for reference in references]
            if selected:
                result.append(dict(station=station, tile=row["normal[0].base[0].xy"], label=label,
                                   quantity=sum(packet["count"] for packet in selected), packets=selected))
    return result


def characterize(entry):
    """@param entry Exact original operational-Core summary for one accepted build.
    @return Current physical buffers/freight/service/books and explicit known expansion deficits, without predicting future income.
    """
    receipt = ROOT / entry["receipt"]["path"]; assert identity(receipt) == entry["receipt"]
    operation = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
    path = ROOT / operation["output"]["path"]; assert identity(path) == operation["output"] == entry["output"]
    snapshot = observe(path); assert json.loads(json.dumps(snapshot)) == operation["final"] and snapshot["operational"]
    factory = FACTORY["observe_factory"](path); labels = factory["cargo_labels"]; industries = []
    # Actual industry buffers remain their own counter-based physical owners;
    # station packet stock is reported separately so neither can be counted twice.
    for industry, row in TABLE(path, b"INDY").items():
        slots = {}
        for field in ("accepted", "produced"):
            slots[field] = {labels[row[f"{field}[{index}].cargo"]]: row[f"{field}[{index}].waiting"] for index in range(row[field + ".count"])}
        industries.append(dict(industry=industry, type=row["type"], tile=row["location.tile"], founder=row["founder"], **slots))
    freight = []
    for vehicle, row in TABLE(path, b"VEHS").items():
        if row["type"] not in (0, 1):
            continue
        prefix = TRAIN if row["type"] == 0 else ROAD
        name = bytes.fromhex(row[prefix + "name"]).decode("utf-8")
        freight.append(dict(vehicle=vehicle, type=row["type"], name=name, engine=row[prefix + "engine_type"],
                            label=labels[row[prefix + "cargo_type"]], capacity=row[prefix + "cargo_cap"], tile=row[prefix + "tile"],
                            stopped=bool(row[prefix + "vehstatus"] & 2)))
    company, = TABLE(path, b"PLYR").values()
    stock = snapshot["totals"]; modules = {"STEL": 16, "GOOD": 28}; missing = {label: modules[label] - stock[label] for label in modules}
    known = dict(module_capital=340000, missions=10000, pad=3000, canonical_work=7000, reserve=150000)
    base = sum(known.values()); spendable = company["money"] + 5000000 - company["current_loan"]
    return dict(configuration=entry["configuration"], receipt=entry["receipt"], output=operation["output"],
                company=snapshot["company"], spendable=spendable, original_totals=stock, original_core=snapshot["core"], panel=snapshot["panel"],
                module_inputs=modules, missing_common_materials=missing, minimum_two_flight_fuel=30, existing_fuel=stock["PROP"],
                additional_fuel=30 - stock["PROP"], full_two_unit_batches=(30 - stock["PROP"] + 1) // 2,
                rounded_refill_fuel=2 * ((30 - stock["PROP"] + 1) // 2), known_bill=known, known_bill_total=base,
                known_bill_shortfall=max(0, base - spendable), full_logistics_bill_complete=False,
                unpriced_prerequisites=["Actual additional Steel/Goods collection through existing owners", "Actual additional Propellant/CHEM/raw-source delivery and any required transport"],
                native_layout_admitted=False, native_money_granted=False, native_reruns=0,
                station_stock=station_stock(path, labels), industries=industries, conventional_bodies=freight,
                service_lives=snapshot["service_lives"], yearly_expenses=company["yearly_expenses"],
                clock=TABLE(path, b"DATE")[0])


def main():
    """Publish actual current original expansion membership while preserving all existing world and native receipts."""
    foundation = json.loads(FOUNDATION.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["original_operational_core"]
    configurations = [characterize(entry) for entry in foundation["configurations"]]
    first, second = [{key: value for key, value in entry.items() if key not in ("configuration", "receipt", "output")} for entry in configurations]
    assert first == second
    output = ROOT / "logs/surface-entry-expansion-membership-verification.json"; assert not output.exists()
    result = dict(_documentation=__doc__, complete=True, native_expansion_admitted=False, funded_expansion=False,
                  foundation=identity(FOUNDATION), configurations=configurations, source=identity(Path(__file__)),
                  sources=[identity(ROOT / name) for name in ("src/multilayer_space_content.cpp", "src/orbital_freight.cpp", "src/orbital_station.cpp", "src/space_manifest.cpp")])
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
