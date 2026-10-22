"""Observe actual Salt custody, chemical input settlement and ordinary surplus loss.

@file surface_chemical_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.
Native mine counters are history. Industry, CAPA, STNN and VEHS retain sole
material ownership. Ordinary unserved station offers may decay; held wagon
stock and delivered/consumed chemical inputs must never be counted as losses.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group8.0; pure reader ownership.
"""
from pathlib import Path
import re

from space_preview_checks import chunks

from space_saved_state import FACTORY, TABLE
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


def chemical_ledger(path):
    """Reconcile actually delivered Salt/Oil with complete native chemical batches.

    @param path Genuine native save after ordinary input delivery and recipe work.
    @return Sole factory stocks, exact histories and useful physical chemical custody.
    """
    factory = FACTORY["observe_factory"](path)
    fleet = products(path)
    industry = TABLE(path, b"INDY")[15]
    produced = factory["counters"][2]["CHEM"]
    dispatched = factory["processor"]["dispatched"]
    assert produced == factory["outputs"]["CHEM"] + dispatched
    accepted = {}
    # This newly funded producer's actual acceptance history must account for
    # every incoming ingredient before input stock or complete batches can count.
    for index in range(industry["accepted.count"]):
        prefix = "accepted[" + str(index) + "]."
        label = factory["cargo_labels"][industry[prefix + "cargo"]]
        accepted[label] = sum(value for name, value in industry.items() if name.startswith(prefix) and name.endswith(".accepted"))
    assert accepted["SALT"] == produced + factory["inputs"]["SALT"]
    assert accepted["OIL_"] == produced + factory["inputs"]["OIL_"]
    assert accepted["CHEM"] == 0
    packets = {key: row for key, row in TABLE(path, b"CAPA").items() if row["source_type"] == 0 and row["source_id"] == 15}
    cargo, = [key for key, label in factory["cargo_labels"].items() if label == "CHEM"]
    references = []
    # Resolve physical station and vehicle packet references independently.
    # Production/dispatch counters are history and cannot own a second CHEM unit.
    for station in TABLE(path, b"STNN").values():
        pattern = re.escape("normal[0].goods[" + str(cargo) + "].") + r"cargo\[\d+\]\.second"
        for name, values in station.items():
            if re.fullmatch(pattern, name):
                references.extend(reference - 1 for reference in values)
    for row in fleet["tankers"]:
        if row["label"] == "CHEM":
            references.extend(packet["packet"] for packet in row["packets"])
    assert len(references) == len(set(references)) and set(references) == packets.keys()
    held = sum(row["count"] for row in packets.values())
    ordinary_loss = dispatched - held
    assert ordinary_loss >= 0 and produced == factory["outputs"]["CHEM"] + held + ordinary_loss
    reserve, = [row for row in fleet["tankers"] if row["name"] == "Surface Chemical reserve"]
    useful = reserve["held"] == reserve["capacity"] == 21 and reserve["stopped"] and reserve["state"] == 254 and reserve["tile"] == 2443
    return dict(factory=factory, accepted=accepted, produced=produced, dispatched=dispatched, source_packets=packets,
                ordinary_station_loss=ordinary_loss, reserve=reserve, useful_custody=useful)
