"""Observe physical external CHEM consumption and genuinely collected PROP.

@file surface_propellant_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BID/15BIEB/15BIEF.
Industry buffers, original packets and company books retain sole ownership.
LPCC counters describe lifetime committed history, never a second inventory.
"""
import re

from surface_income_ledger import FACTORY, TABLE, TRAIN

ROAD = "roadveh[0].common[0]."


def counters(path, industry):
    """@param path Complete native save. @param industry Actual allocated industry ID. @return Stable-label lifetime counters."""
    result = {0: {}, 1: {}, 2: {}}
    for row in TABLE(path, b"LPCC").values():
        if row["industry"] != industry:
            continue
        label = row["label"].to_bytes(4, "little").decode("ascii")
        assert label not in result[row["kind"]]
        result[row["kind"]][label] = row["count"]
    return result


def observe(path):
    """Reconcile the selected original chemicals, useful new fuel and every surviving native packet.

    @param path Actual first native output before/after traffic, transfer, processing or collection.
    @return Exact material custody, lifetime conversion, original finances and real crossing protection.
    """
    factory = FACTORY["observe_factory"](path); labels = factory["cargo_labels"]
    vehicles, stations, packets, industries = [TABLE(path, family) for family in (b"VEHS", b"STNN", b"CAPA", b"INDY")]
    plant_id, = [key for key, row in industries.items() if row["location.tile"] == 1639]
    plant = industries[plant_id]; assert plant_id != 15 and plant["type"] == 25 and plant["founder"] == 0
    state, = [row for row in TABLE(path, b"LPCR").values() if row["industry"] == plant_id]
    assert state["controller"] == 0 and state["recipe"] == int.from_bytes(b"PCHM", "little")
    history = counters(path, plant_id)
    inputs = {labels[plant[f"accepted[{index}].cargo"]]: plant[f"accepted[{index}].waiting"] for index in range(plant["accepted.count"])}
    outputs = {labels[plant[f"produced[{index}].cargo"]]: plant[f"produced[{index}].waiting"] for index in range(plant["produced.count"])}
    consumed, produced, batches = history[1]["CHEM"], history[2]["PROP"], history[0]["PCHM"]
    assert consumed == produced == 2 * batches <= 20
    assert history[0]["CHEM"] == 0 and inputs["SALT"] == inputs["OIL_"] == outputs["CHEM"] == 0
    owners, cargo_labels = {}, {}

    def claim(reference, owner, cargo):
        """@param reference Original one-based packet reference. @param owner Actual native container. @param cargo Native cargo slot. Assert sole ownership."""
        key = reference - 1
        assert key in packets and key not in owners, (key, owner)
        owners[key] = owner; cargo_labels[key] = labels[cargo]

    # Follow every native container reference rather than inventing a cargo
    # registry from nearby vehicles or treating historical dispatch as stock.
    for key, station in stations.items():
        for name, references in station.items():
            match = re.fullmatch(r"normal\[0\]\.goods\[(\d+)\]\.cargo\[\d+\]\.second", name)
            if match:
                for reference in references:
                    claim(reference, ["station", key], int(match.group(1)))
    for key, vehicle in vehicles.items():
        if vehicle["type"] not in (0, 1):
            continue
        prefix = TRAIN if vehicle["type"] == 0 else ROAD
        for reference in vehicle[prefix + "cargo.packets"]:
            claim(reference, ["vehicle", key], vehicle[prefix + "cargo_type"])
    assert owners.keys() == packets.keys()
    roads, trains, names = [], [], set()
    sources = {"IORE": 9, "STEL": 10, "GOOD": 11, "OIL_": 8, "CHEM": 15, "PROP": plant_id}
    road_roles = {"Surface ore delivery": "IORE", "Surface Steel delivery": "STEL", "Surface Steel reserve 1": "STEL",
                  "Surface Steel reserve 2": "STEL", "Surface Goods reserve": "GOOD", "Surface Oil delivery 1": "OIL_",
                  "Surface Oil delivery 2": "OIL_", "Surface Chemical reserve": "CHEM", "Surface Propellant reserve": "PROP"}
    road_roles.update({f"Surface Launch passenger {index}": "PASS" for index in range(1, 4)})
    road_roles.update({f"Surface Launch Goods reserve {index}": "GOOD" for index in range(1, 9)})
    for key, row in vehicles.items():
        if row["type"] != 1:
            continue
        name = bytes.fromhex(row[ROAD + "name"]).decode("utf-8")
        assert name in road_roles and name not in names and row[ROAD + "owner"] == 0
        names.add(name); label = labels[row[ROAD + "cargo_type"]]; assert label == road_roles[name]
        native = [dict(packet=reference - 1, **packets[reference - 1]) for reference in row[ROAD + "cargo.packets"]]
        # New buses carry actual town-generated passengers. Freight retains
        # its exact original industry; Town0passengers cannot become stock.
        if label == "PASS":
            assert all(packet["source_type"] == 1 and packet["source_id"] == 0 for packet in native)
        else:
            assert all(packet["source_type"] == 0 and packet["source_id"] == sources[label] for packet in native)
        held = sum(packet["count"] for packet in native)
        assert held == sum(row[ROAD + "cargo.action_counts"]) <= row[ROAD + "cargo_cap"]
        roads.append(dict(id=key, name=name, label=label, engine=row[ROAD + "engine_type"], capacity=row[ROAD + "cargo_cap"],
                          tile=row[ROAD + "tile"], stopped=bool(row[ROAD + "vehstatus"] & 2), depot=row["roadveh[0].state"] == 254,
                          held=held, packets=native))
    reserves = [row for row in roads if row["name"] in ("Surface Steel reserve 1", "Surface Steel reserve 2", "Surface Goods reserve")]
    assert len(reserves) == 3 and all(row["stopped"] and row["depot"] and row["tile"] == 2443 for row in reserves)
    assert sum(row["held"] for row in reserves if row["label"] == "STEL") == 36
    assert sum(row["held"] for row in reserves if row["label"] == "GOOD") == 14
    feed, = [row for row in roads if row["name"] == "Surface Chemical reserve"]
    prop = [row for row in roads if row["name"] == "Surface Propellant reserve"]
    assert len(prop) <= 1
    # These are the very21original held external chemical units. Partial
    # unloading and processing may move their sole owner, but cannot create
    # new inputs or hide consumption in a thirteen-month acceptance window.
    assert feed["held"] + inputs["CHEM"] + consumed == 21
    prop_packets = {key: packet for key, packet in packets.items() if packet["source_type"] == 0 and
                    packet["source_id"] == plant_id and cargo_labels[key] == "PROP"}
    assert produced == outputs["PROP"] + state["dispatched"]
    assert state["dispatched"] == sum(packet["count"] for packet in prop_packets.values())
    # Pool slot6previously belonged to a different natural industry. Stable
    # cargo meaning as well as actual source ID identifies this new PROP;
    # another historical commodity cannot be relabeled as useful fuel.
    assert all(owners[key][0] == "station" or (prop and owners[key] == ["vehicle", prop[0]["id"]]) for key in prop_packets)
    for head_id, row in vehicles.items():
        if row["type"] != 0:
            continue
        name = bytes.fromhex(row[TRAIN + "name"]).decode("utf-8")
        if not name.startswith("Surface Income "):
            continue
        assert name in ("Surface Income farm", "Surface Income goods", "Surface Income coal")
        chain, current = [], head_id
        # Actual native linked parts alone establish the commercial train;
        # parking cannot replace a paid wagon or silently discard its freight.
        while current is not None:
            assert current not in chain; chain.append(current)
            part = vehicles[current]; assert part["type"] == 0 and part[TRAIN + "owner"] == 0
            label = labels[part[TRAIN + "cargo_type"]]
            assert all(packets[reference - 1]["source_type"] == 0 and packets[reference - 1]["source_id"] == {
                "GRAI": 7, "LVST": 7, "GOOD": 5, "COAL": 0}[label] for reference in part[TRAIN + "cargo.packets"])
            current = part[TRAIN + "next"] - 1 if part[TRAIN + "next"] else None
        depot = {"Surface Income farm": 1638, "Surface Income goods": 3224, "Surface Income coal": 3161}[name]
        # Original dual26has two native engine bodies; paid single24has one.
        # Preserve all12Farm/12Goods/8Coalwagons rather than inventing another
        # rear body or accepting a silently shortened useful freight consist.
        engine = row[TRAIN + "engine_type"]; assert engine in (24, 26)
        assert len(chain) == (8 if name.endswith("coal") else 12) + (2 if engine == 26 else 1)
        trains.append(dict(head=head_id, name=name, chain=chain, tile=row[TRAIN + "tile"],
                           halted=bool(row[TRAIN + "vehstatus"] & 2) and row["train[0].track"] == 128 and row[TRAIN + "tile"] == depot))
    assert len(trains) == 3
    mine, = TABLE(path, b"LMNR").values(); deposit = TABLE(path, b"GDEP")[4]; mine_industry = industries[14]
    assert mine["extracted"] + deposit["remaining"] == 24000
    assert mine["extracted"] == mine_industry["produced[0].waiting"] + mine["dispatched"]
    salt_packets = {key: packet for key, packet in packets.items() if packet["source_type"] == 0 and packet["source_id"] == 14}
    assert all(owners[key] == ["station", 5] and cargo_labels[key] == "SALT" for key in salt_packets)
    assert factory["inputs"]["SALT"] + factory["counters"][1]["SALT"] == 60
    salt_loss = mine["dispatched"] - 60 - sum(packet["count"] for packet in salt_packets.values()); assert salt_loss >= 0
    chemical_packets = {key: packet for key, packet in packets.items() if packet["source_type"] == 0 and packet["source_id"] == 15}
    assert all(cargo_labels[key] == "CHEM" for key in chemical_packets)
    chemical_loss = factory["processor"]["dispatched"] - sum(packet["count"] for packet in chemical_packets.values()) - inputs["CHEM"] - consumed
    assert chemical_loss >= 0 and factory["counters"][2]["CHEM"] == factory["outputs"]["CHEM"] + factory["processor"]["dispatched"]
    company, = TABLE(path, b"PLYR").values()
    return dict(company=dict(cash=company["money"], debt=company["current_loan"]), reserves=dict(steel=36, goods=14), reserve_trucks=reserves,
                chemical_feed=feed, propellant_truck=prop[0] if prop else None, trains=trains, traffic_halted=all(row["halted"] for row in trains),
                passenger_buses=[row for row in roads if row["label"] == "PASS"],
                additional_goods=[row for row in roads if row["name"].startswith("Surface Launch Goods reserve ")],
                plant=dict(industry=plant_id, recipe="PCHM", chemicals=inputs["CHEM"], consumed=consumed, produced=produced,
                           buffered=outputs["PROP"], dispatched=state["dispatched"], batches=batches),
                mine=dict(extracted=mine["extracted"], remaining=deposit["remaining"], ordinary_loss=salt_loss),
                original_chemical=dict(produced=factory["counters"][2]["CHEM"], dispatched=factory["processor"]["dispatched"], ordinary_loss=chemical_loss),
                surviving_packets=len(packets), sole_packet_owners=len(owners))


def reserve_identity(snapshot):
    """@param snapshot Actual native world. @return Protected original Steel/Goods identity excluding naturally accrued packet age."""
    result = []
    for truck in snapshot["reserve_trucks"]:
        observed = dict(truck)
        observed["packets"] = [{key: value for key, value in packet.items() if key != "periods_in_transit"} for packet in truck["packets"]]
        result.append(observed)
    return result
