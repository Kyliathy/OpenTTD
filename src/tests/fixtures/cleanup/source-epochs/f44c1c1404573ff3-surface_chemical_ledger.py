"""Observe actual Salt custody, chemical input settlement and ordinary surplus loss.

@file surface_chemical_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.
Native mine counters are history. Industry, CAPA, STNN and VEHS retain sole
material ownership. Ordinary unserved station offers may decay; held wagon
stock and delivered/consumed chemical inputs must never be counted as losses.
"""
from pathlib import Path
import re
import runpy

from space_preview_checks import chunks

FACTORY = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-4.py")))
TABLE = FACTORY["OWNERS"]["saved_table"]
TRAIN = "train[0].common[0]."


def salt_ledger(path):
    """Reconcile finite extraction with explicit actual packet/input owners.

    @param path Complete genuine save containing the real chemical factory15.
    @return Mine history, sole surviving packet custody and ordinary surplus losses.
    """
    factory = FACTORY["observe_factory"](path)
    mine, = TABLE(path, b"LMNR").values()
    deposit = TABLE(path, b"GDEP")[4]
    industry = TABLE(path, b"INDY")[14]
    cargo = industry["produced[0].cargo"]
    assert mine["industry"] == 14 and mine["deposit"] == 4 and mine["company"] == 0
    assert deposit["claimant"] == 0 and deposit["resource"].to_bytes(4, "little") == b"SALT"
    assert mine["extracted"] + deposit["remaining"] == 24000
    assert mine["extracted"] == industry["produced[0].waiting"] + mine["dispatched"]
    packets = {key: row for key, row in TABLE(path, b"CAPA").items() if row["source_type"] == 0 and row["source_id"] == 14}
    station = TABLE(path, b"STNN")[5]
    prefix = "normal[0].goods[" + str(cargo) + "]."
    references = [reference for name, values in station.items() if re.fullmatch(re.escape(prefix) + r"cargo\[\d+\]\.second", name)
                  for reference in values]
    station_packets = {reference - 1 for reference in references}
    vehicles = TABLE(path, b"VEHS")
    wagon, head = vehicles[19], vehicles[18]
    assert wagon[TRAIN + "cargo_type"] == cargo and wagon[TRAIN + "cargo_cap"] == 60
    wagon_packets = {reference - 1 for reference in wagon[TRAIN + "cargo.packets"]}
    # Every surviving mine14packet has exactly one actual owner: the original
    # pickup station or the original paid wagon. Unknown packets/duplicate
    # references cannot be hidden in a derived surplus-loss counter.
    assert len(station_packets) == len(references)
    assert station_packets.isdisjoint(wagon_packets) and station_packets | wagon_packets == packets.keys()
    held = sum(packets[key]["count"] for key in wagon_packets)
    assert held == sum(wagon[TRAIN + "cargo.action_counts"])
    waiting = sum(packets[key]["count"] for key in station_packets)
    delivered = factory["inputs"]["SALT"] + factory["counters"][1]["SALT"]
    ordinary_loss = mine["dispatched"] - waiting - held - delivered
    assert ordinary_loss >= 0
    # Historical dispatch includes native station rating abandonment. Its
    # only unowned remainder is recorded separately, never useful cargo. The
    # Surface wagon and actual processor input/commit counters stay explicit.
    assert mine["dispatched"] == waiting + held + delivered + ordinary_loss
    surface = wagon[TRAIN + "tile"] == head[TRAIN + "tile"] == 1373
    surface = surface and bool(head[TRAIN + "vehstatus"] & 2) and wagon["train[0].track"] == head["train[0].track"] == 128
    return dict(mine=mine, reserve=deposit["remaining"], mine_waiting=industry["produced[0].waiting"],
                source_packets=packets, station_packets=sorted(station_packets), wagon_packets=sorted(wagon_packets),
                station_waiting=waiting, station_rating=station[prefix + "rating"], wagon_held=held, delivered=delivered,
                ordinary_station_loss=ordinary_loss, surface_custody=surface, supplied_stock=False)


def products(path):
    """Read original reserved products and new source-eligible tanker cargo.

    @param path Complete native world before or after real tanker purchases/work.
    @return Actual finance, protected old packets and distinct new physical carriers.
    """
    company, = TABLE(path, b"PLYR").values()
    vehicles, packets = [TABLE(path, family) for family in (b"VEHS", b"CAPA")]
    labels = FACTORY["observe_factory"](path)["cargo_labels"]
    prefix = "roadveh[0].common[0]."
    old, tankers, claimed = [], [], set()
    # Original vehicle IDs2..6retain the previously purchased production chain.
    # New tanker IDs are discovered by actual native type/name, so free pool
    # reuse after real machinery sales cannot overwrite material provenance.
    for key, row in vehicles.items():
        if row["type"] != 1:
            continue
        label = labels[row[prefix + "cargo_type"]]
        name = bytes.fromhex(row[prefix + "name"]).decode("utf-8")
        native = []
        for reference in row[prefix + "cargo.packets"]:
            packet = reference - 1
            assert packet in packets and packet not in claimed
            claimed.add(packet)
            value = packets[packet]
            source = {"IORE": 9, "STEL": 10, "GOOD": 11, "OIL_": 8, "CHEM": 15}[label]
            assert value["source_type"] == 0 and value["source_id"] == source
            native.append(dict(packet=packet, **value))
        held = sum(value["count"] for value in native)
        assert held == sum(row[prefix + "cargo.action_counts"])
        observed = dict(id=key, name=name, label=label, engine=row[prefix + "engine_type"], capacity=row[prefix + "cargo_cap"],
                        tile=row[prefix + "tile"], stopped=bool(row[prefix + "vehstatus"] & 2), state=row["roadveh[0].state"],
                        held=held, packets=native)
        if key in range(2, 7):
            old.append(observed)
        else:
            assert observed["engine"] == 132 and observed["capacity"] == 21
            assert label in ("OIL_", "CHEM") and name.startswith("Surface ")
            tankers.append(observed)
    assert {row["id"] for row in old} == set(range(2, 7))
    reserves = [row for row in old if "reserve" in row["name"]]
    steel = sum(row["held"] for row in reserves if row["label"] == "STEL")
    goods = sum(row["held"] for row in reserves if row["label"] == "GOOD")
    assert steel == 36 and goods == 14 and all(row["stopped"] and row["state"] == 254 and row["tile"] == 2443 for row in reserves)
    return dict(cash=company["money"], debt=company["current_loan"], expense=sum(company["yearly_expenses"]),
                parked_steel=steel, parked_goods=goods, original_trucks=old, tankers=tankers)
