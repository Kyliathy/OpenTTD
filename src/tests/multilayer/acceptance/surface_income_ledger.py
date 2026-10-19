"""Observe genuine Surface reserves, finite sources and actual commercial trains.

@file surface_income_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.
Rolling annual/quarterly histories are not lifetime accounts. Original native
inventories, packets, company money and finite mine counters retain ownership.
"""
import re

from surface_chemical_ledger import FACTORY, TABLE, TRAIN


def road_products(path, labels):
    """Observe original road roles through native renewal rather than historical pool slots.

    @param path Complete original native save.
    @param labels Same-file runtime cargo meanings.
    @return Original road material custody and sole company money/debt owners.
    """
    company, = TABLE(path, b"PLYR").values()
    vehicles, packets = [TABLE(path, family) for family in (b"VEHS", b"CAPA")]
    roles = {"Surface ore delivery": "IORE", "Surface Steel delivery": "STEL", "Surface Steel reserve 1": "STEL",
             "Surface Steel reserve 2": "STEL", "Surface Goods reserve": "GOOD", "Surface Oil delivery 1": "OIL_",
             "Surface Oil delivery 2": "OIL_", "Surface Chemical reserve": "CHEM"}
    prefix = "roadveh[0].common[0]."
    old, tankers, names = [], [], set()
    # Native renewal can replace a moving truck's pool ID; ordinary accidents
    # can remove active freight. Names, same-file cargo meaning and original
    # packet provenance identify the roles. Selected depot reserves must remain.
    for key, row in vehicles.items():
        if row["type"] != 1:
            continue
        name = bytes.fromhex(row[prefix + "name"]).decode("utf-8")
        assert name in roles and name not in names and row[prefix + "owner"] == 0
        names.add(name)
        label = labels[row[prefix + "cargo_type"]]
        assert roles[name] == label
        native = []
        for reference in row[prefix + "cargo.packets"]:
            packet = packets[reference - 1]
            assert packet["source_type"] == 0 and packet["source_id"] == {"IORE": 9, "STEL": 10, "GOOD": 11, "OIL_": 8, "CHEM": 15}[label]
            native.append(dict(packet=reference - 1, **packet))
        held = sum(packet["count"] for packet in native)
        assert held == sum(row[prefix + "cargo.action_counts"]) and held <= row[prefix + "cargo_cap"]
        observed = dict(id=key, name=name, label=label, engine=row[prefix + "engine_type"], capacity=row[prefix + "cargo_cap"],
                        tile=row[prefix + "tile"], stopped=bool(row[prefix + "vehstatus"] & 2), state=row["roadveh[0].state"], held=held, packets=native)
        (tankers if label in ("OIL_", "CHEM") else old).append(observed)
    reserves = [row for row in old if "reserve" in row["name"]]
    assert {row["name"] for row in reserves} == {"Surface Steel reserve 1", "Surface Steel reserve 2", "Surface Goods reserve"}
    assert all(row["stopped"] and row["state"] == 254 and row["tile"] == 2443 for row in reserves)
    steel = sum(row["held"] for row in reserves if row["label"] == "STEL")
    goods = sum(row["held"] for row in reserves if row["label"] == "GOOD")
    assert steel == 36 and goods == 14
    return dict(cash=company["money"], debt=company["current_loan"], parked_steel=steel, parked_goods=goods, original_trucks=old, tankers=tankers)


def observe(path):
    """Reconcile actual long-running commerce without assuming sold Salt vehicles exist.

    @param path Complete original native save before/after actual purchases or earnings.
    @return Original company/material owners, commercial consists and finite source history.
    """
    factory = FACTORY["observe_factory"](path)
    vehicles, packets, stations = [TABLE(path, family) for family in (b"VEHS", b"CAPA", b"STNN")]
    labels = factory["cargo_labels"]
    fleet = road_products(path, labels)
    owners, packet_cargo = {}, {}

    def claim(reference, owner, cargo):
        """@param reference Original one-based packet reference. @param owner Native container. @param cargo Original cargo slot. Check sole ownership."""
        key = reference - 1
        assert key in packets and key not in owners, (key, owner)
        owners[key] = owner
        packet_cargo[key] = labels[cargo]

    # Every surviving packet must belong to an actual native station or vehicle.
    # Industry buffers are separate integer owners, never another copy of CAPA.
    for key, station in stations.items():
        for name, references in station.items():
            match = re.fullmatch(r"normal\[0\]\.goods\[(\d+)\]\.cargo\[\d+\]\.second", name)
            if match:
                for reference in references:
                    claim(reference, ["station", key], int(match.group(1)))
    for key, vehicle in vehicles.items():
        if vehicle["type"] not in (0, 1):
            continue
        prefix = TRAIN if vehicle["type"] == 0 else "roadveh[0].common[0]."
        for reference in vehicle[prefix + "cargo.packets"]:
            claim(reference, ["vehicle", key], vehicle[prefix + "cargo_type"])
    assert owners.keys() == packets.keys()
    trains = []
    for head_id, head in vehicles.items():
        if head["type"] != 0:
            continue
        name = bytes.fromhex(head[TRAIN + "name"]).decode("utf-8")
        if not name.startswith("Surface Income "):
            continue
        role = {"Surface Income farm": 0, "Surface Income goods": 1, "Surface Income coal": 2}[name]
        chain, capacity, held, current = [], {}, {}, head_id
        # Follow the actual native next references; vehicle ID order or nearby
        # map poses cannot invent an attached wagon or a carrying capacity.
        while current is not None:
            assert current not in chain
            chain.append(current)
            part = vehicles[current]
            assert part["type"] == 0 and part[TRAIN + "owner"] == 0
            label = labels[part[TRAIN + "cargo_type"]]
            capacity[label] = capacity.get(label, 0) + part[TRAIN + "cargo_cap"]
            count = 0
            for reference in part[TRAIN + "cargo.packets"]:
                packet = packets[reference - 1]
                assert packet["source_type"] == 0
                assert packet["source_id"] == {"GRAI": 7, "LVST": 7, "GOOD": 5, "COAL": 0}[label]
                count += packet["count"]
            assert count == sum(part[TRAIN + "cargo.action_counts"])
            held[label] = held.get(label, 0) + count
            current = part[TRAIN + "next"] - 1 if part[TRAIN + "next"] else None
        trains.append(dict(role=role, head=head_id, name=name, chain=chain, capacity=capacity, held=held,
                           engines=[vehicles[key][TRAIN + "engine_type"] for key in chain],
                           tile=head[TRAIN + "tile"], stopped=bool(head[TRAIN + "vehstatus"] & 2), speed=head[TRAIN + "cur_speed"]))
    def route_order(row):
        """@param row Actual named consist. @return Stable commercial role for cross-build comparison."""
        return row["role"]

    trains.sort(key=route_order)
    if trains:
        assert [row["role"] for row in trains] == [0, 1, 2]
        # A verified original electric renewal keeps all old freight wagons,
        # removes the old head's small cargo compartment and adds four Coal
        # wagons. The dedicated purchase receipt proves each changed identity;
        # this observer still demands the whole exact native consist shape.
        modern = [row["engines"][0] == 26 for row in trains]
        assert not any(modern) or all(modern)
        assert [len(row["chain"]) for row in trains] == ([14, 14, 10] if all(modern) else [14, 14, 6])
        assert trains[0]["capacity"]["GRAI"] == (180 if all(modern) else 184) and trains[0]["capacity"]["LVST"] == 150
        assert trains[1]["capacity"]["GOOD"] == (300 if all(modern) else 308)
        if all(modern):
            assert trains[2]["capacity"]["COAL"] == 240
            assert all(row["engines"][-1] == 26 for row in trains)
        # Service commands may legitimately exchange Coal wagons. Their actual
        # capacity is verified against that phase's original purchase quote;
        # the generic packet observer cannot impose a second model registry.
        assert trains[2]["capacity"]["COAL"] > 0
    reserve, = [row for row in fleet["tankers"] if row["name"] == "Surface Chemical reserve"]
    assert reserve["held"] == 21 and reserve["stopped"] and reserve["state"] == 254 and reserve["tile"] == 2443
    mine, = TABLE(path, b"LMNR").values()
    deposit = TABLE(path, b"GDEP")[4]
    mine_industry = TABLE(path, b"INDY")[14]
    assert mine["extracted"] + deposit["remaining"] == 24000
    assert mine["extracted"] == mine_industry["produced[0].waiting"] + mine["dispatched"]
    salt_packets = {key: row for key, row in packets.items() if row["source_type"] == 0 and row["source_id"] == 14}
    assert all(owners[key] == ["station", 5] and packet_cargo[key] == "SALT" for key in salt_packets)
    delivered_salt = factory["inputs"]["SALT"] + factory["counters"][1]["SALT"]
    assert delivered_salt == 60
    salt_loss = mine["dispatched"] - delivered_salt - sum(row["count"] for row in salt_packets.values())
    assert salt_loss >= 0
    chemical_packets = {key: row for key, row in packets.items() if row["source_type"] == 0 and row["source_id"] == 15}
    assert all(packet_cargo[key] == "CHEM" for key in chemical_packets)
    chemical_loss = factory["processor"]["dispatched"] - sum(row["count"] for row in chemical_packets.values())
    assert chemical_loss >= 0
    assert factory["counters"][2]["CHEM"] == factory["outputs"]["CHEM"] + factory["processor"]["dispatched"]
    return dict(company=dict(cash=fleet["cash"], debt=fleet["debt"]), reserves=dict(steel=fleet["parked_steel"], goods=fleet["parked_goods"], chemicals=21),
                reserve_trucks=[row for row in fleet["original_trucks"] if "reserve" in row["name"]] + [reserve], trains=trains,
                mine=dict(extracted=mine["extracted"], remaining=deposit["remaining"], dispatched=mine["dispatched"], ordinary_loss=salt_loss),
                chemical=dict(inputs=factory["inputs"], produced=factory["counters"][2]["CHEM"], dispatched=factory["processor"]["dispatched"], ordinary_loss=chemical_loss),
                surviving_packets=len(packets), sole_packet_owners=len(owners))


def reserve_identity(snapshot):
    """@param snapshot Original observed world. @return Physical reserve identity excluding naturally accrued packet age."""
    result = []
    for truck in snapshot["reserve_trucks"]:
        packets = [{key: value for key, value in packet.items() if key not in ("periods_in_transit",)} for packet in truck["packets"]]
        result.append(dict(id=truck["id"], label=truck["label"], held=truck["held"], tile=truck["tile"], stopped=truck["stopped"], packets=packets))
    return result
