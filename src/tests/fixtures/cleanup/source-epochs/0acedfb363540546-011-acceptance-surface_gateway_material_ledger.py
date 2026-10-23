"""Conserve the original128launch units through physical shipment and native handling.

@file surface_gateway_material_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEQL/15BIEQM/15BIEQN/15C/15CIE/15CK/15CL/15CM/15CN/15CP/15CSGB.
Only actual vehicle, station and LHND packet references own inventory. This
default delivery contract retains all physical stock. Explicit assembly/payload
consumers alone admit actual paid embodied inputs and native craft compartments.
"""
import re
import struct

from space_preview_checks import chunks
from surface_propellant_ledger import FACTORY, TABLE, TRAIN, ROAD, counters
from surface_regional_ledger import lives
from surface_gateway_facility_ledger import facility

SHIPMENTS = {
    "Surface Steel reserve 1": ("STEL", 10), "Surface Steel reserve 2": ("STEL", 10),
    "Surface Goods reserve": ("GOOD", 11), "Surface Launch Goods reserve 1": ("GOOD", 11),
    "Surface Launch Goods reserve 2": ("GOOD", 11), "Surface Propellant reserve": ("PROP", 6),
    "Surface Propellant refill": ("PROP", 6),
}
REQUIRED = {"STEL": 36, "GOOD": 50, "PROP": 42}


def handling(path, labels, center):
    """@param path Complete native save. @param labels Same-file cargo meanings. @param center Actual generation-qualified facility.
    @return Independently decoded sole packet-owning reverse jobs and actual fractional fee remainder.
    """
    data = chunks(path)[1][b"LHND"][3:]; next_id, count, cost_count = struct.unpack_from(">QHB", data)
    offset, jobs = 11, []
    # Every job must retain the selected exact center/member generations and
    # explicit reverse supply direction. An unrelated or stale job owns none
    # of this cohort, even if its numeric station IDs happen to match.
    for _ in range(count):
        values = struct.unpack_from(">QHQHQBBHHB", data, offset); offset += 35
        job_id, station, generation, member, member_generation, cargo, reverse, remaining, age, packet_count = values
        assert 0 < job_id < next_id and (station, generation) == (center["center"], center["generation"])
        assert (member, member_generation) == (center["member"], center["member_generation"])
        assert reverse == 1 and labels[cargo] in REQUIRED and 0 < remaining <= 8 and 0 < packet_count <= 64
        references = [struct.unpack_from(">IH", data, offset + index * 6) for index in range(packet_count)]; offset += packet_count * 6
        jobs.append(dict(id=job_id, cargo=cargo, label=labels[cargo], remaining=remaining, age=age, references=references))
    assert len({job["id"] for job in jobs}) == len(jobs) <= 3 and cost_count <= 1
    remainder = 0
    if cost_count:
        station, generation, remainder = struct.unpack_from(">HQB", data, offset); offset += 11
        assert (station, generation) == (center["center"], center["generation"]) and 0 < remainder < 16
    assert offset == len(data) and next_id > 0
    return dict(next_id=next_id, jobs=jobs, remainder=remainder)


def observe(path, phase="delivery", allow_component_abandonment=False, allow_salt_delivery=False, allow_chemical_pickup=False, allow_chemical_delivery=False, allow_propellant_pickup=False, allow_propellant_delivery=False, allow_capital_service=False, allow_goods_income=False, allow_farm_road=False, allow_factory_goods=False, allow_capital_return=False):
    """@param path Actual original world before/during/after native shipment and handling.
    @param phase Explicit native custody boundary; components admits bounded original carrier pickups, component-delivery alone conserves the fixed already acquired36/50through subsequent transport. Default retains128physical units and empty hull slots.
    @param allow_component_abandonment Explicit after-boundary receipt for actual eightSteel/twoGoods ordinary-offer loss; valid only for component-delivery, never a material credit.
    @param allow_salt_delivery Explicit later physical delivery of at most60new finite Salt to the original chemical factory; historical defaults still require its original120only.
    @param allow_chemical_pickup Explicit renewed27capacity carrier's Source15pickup only; PCHM and original Propellant remain unchanged.
    @param allow_chemical_delivery Explicit later27CHEMdelivery to PCHM6and at most26new produced PROP; original launch-stock cohort remains unchanged.
    @param allow_propellant_pickup Explicit actual Source6PROPpackets on original reserve2only; historical acquired PROP remains zero.
    @param allow_propellant_delivery Explicit transport of the completed fixed26Source6acquisition through member/job/center; empty present holds never erase that real receipt.
    @return Complete sole packet custody, physical-plus-embodied/burned/consumed/actually abandoned conservation, original processors, useful services and native books.
    @param allow_capital_service Explicit later single paid Town0PASSbus; every historical default keeps its exact original roles.
    @param allow_goods_income Explicit later reuse of only real empty140/cap18reserves3/25; new Source11trade stays outside original fiftyGoods and old defaults remain strict.
    @param allow_farm_road Explicit later sole named137/cap18Roadbody with only real Industry7Livestock; all original role/material defaults stay strict.
    @param allow_factory_goods Explicit later actual3/25native Source5Goods picked up at realStation13/paid2572; all historical source defaults and original fiftyGoods remain strict.
    @param allow_capital_return Explicit later extra9/owner0/model119/cap40 Town1return packets first20; default and original local sources remain strict.
    """
    assert phase in ("delivery", "assembly", "payload", "mission", "core", "resupply", "construction", "components", "component-delivery")
    assert not allow_component_abandonment or phase == "component-delivery"
    # New chemical production belongs only to the explicit later original
    # component world; earlier material contracts remain exact and unchanged.
    assert not allow_salt_delivery or (phase == "component-delivery" and allow_component_abandonment)
    assert not allow_chemical_pickup or allow_salt_delivery
    assert not allow_chemical_delivery or allow_chemical_pickup
    assert not allow_propellant_pickup or allow_chemical_delivery
    assert not allow_propellant_delivery or allow_propellant_pickup
    # Only the completed original Core/material/fuel world can reassign these
    # two empty reserves to income. Earlier delivery contracts remain exact.
    assert not allow_goods_income or (phase == "component-delivery" and allow_propellant_delivery and allow_capital_service)
    # Later explicit consumers include only their delivered prerequisite
    # custody. Earlier defaults retain exactly their original strict bounds.
    building = phase in ("construction", "components", "component-delivery")
    resupply = building or phase == "resupply"; core = resupply or phase == "core"
    mission = core or phase == "mission"; payload = mission or phase == "payload"; assembly = payload or phase == "assembly"
    current = facility(path, allow_craft=assembly); assert current["center"] is not None
    paid = []
    if assembly:
        from surface_gateway_assembly_ledger import production
        paid = production(path, current, allow_payload=payload, allow_mission=mission, allow_core=core, allow_resupply=resupply, allow_construction=building)
    manifests = {}
    if payload:
        from surface_gateway_payload_ledger import supplies
        manifests = supplies(path, paid, allow_mission=mission, allow_resupply=resupply)
    factory = FACTORY["observe_factory"](path); labels = factory["cargo_labels"]
    vehicles, stations, packets, industries = [TABLE(path, key) for key in (b"VEHS", b"STNN", b"CAPA", b"INDY")]
    transfers = handling(path, labels, current); owners, packet_labels = {}, {}
    construction = None
    if core:
        from surface_gateway_core_ledger import blueprint
        construction = blueprint(path, allow_construction=building)

    def claim(key, owner, cargo):
        """@param key Actual zero-based packet ID. @param owner Sole native container. @param cargo Same-file cargo slot."""
        assert key in packets and key not in owners and packets[key]["count"] > 0
        owners[key] = owner; packet_labels[key] = labels[cargo]

    # Follow all native packet references, including other commercial freight;
    # quantity histories and observed snapshot dictionaries own no inventory.
    for key, row in stations.items():
        for field, references in row.items():
            match = re.fullmatch(r"normal\[0\]\.goods\[(\d+)\]\.cargo\[\d+\]\.second", field)
            if match:
                for reference in references:
                    claim(reference - 1, ("station", key), int(match.group(1)))
    for key, row in vehicles.items():
        assert row["type"] in ((0, 1, 4, 6) if assembly else (0, 1, 4))
        if row["type"] == 4:
            continue
        prefix = TRAIN if row["type"] == 0 else "space[0].common[0]." if row["type"] == 6 else ROAD
        assert row[prefix + "owner"] == 0 and not row[prefix + "vehstatus"] & 128
        if row["type"] == 6:
            # Paid Earth assembly has no loose freight or supply stock yet.
            # Every Space body must have the independently decoded paid receipt.
            assert key in {craft["vehicle"] for craft in paid}
            if not payload:
                assert not row[prefix + "cargo.packets"]
        for reference in row[prefix + "cargo.packets"]:
            claim(reference - 1, ("vehicle", key), row[prefix + "cargo_type"])
    for job in transfers["jobs"]:
        for key, hop in job["references"]:
            claim(key, ("handling", job["id"]), job["cargo"])
        held = sum(packets[key]["count"] for key, _ in job["references"])
        assert 0 < held <= 64 and job["remaining"] == (held + 7) // 8
    # Native manifest compartments are independent sole owners of the very
    # same original packets. Only explicit ground-payload mode may claim them;
    # current service parts remain empty and propellant keeps its source6label.
    for vehicle, manifest in manifests.items():
        for index, references in enumerate(manifest["supplies"]):
            assert index == 0 or not references
            for key, hop in references:
                claim(key, ("supply", vehicle, index), next(slot for slot, label in labels.items() if label == "PROP"))
    if construction is not None:
        # The saved native blueprint job becomes the sole owner when delivered
        # packets leave the constructor. Its quantity metadata is checked
        # against actual CAPAreferences, never counted as another inventory.
        for index, slot in enumerate(construction["row"]["inputs"]):
            cargo = next(slot for slot, label in labels.items() if label == ("STEL", "GOOD")[index])
            for key, hop in slot["references"]:
                claim(key, ("construction", construction["site"], index), cargo)
            assert sum(packets[key]["count"] for key, _ in slot["references"]) == slot["quantity"]
    assert owners.keys() == packets.keys()
    roles = dict(SHIPMENTS)
    roles.update({"Surface ore delivery": ("IORE", 9), "Surface Steel delivery": ("STEL", 10), "Surface Chemical reserve": ("CHEM", 15)})
    roles.update({f"Surface Launch passenger {index}": ("PASS", 0) for index in range(1, 4)})
    roles.update({f"Surface Regional passenger {index}": ("PASS", None) for index in range(1, 4)})
    # Only the explicitly admitted later service may add its actual named body;
    # original defaults still reject every extra road vehicle.
    if allow_capital_service:
        if any(row["type"] == 1 and bytes.fromhex(row[ROAD + "name"]).decode("utf-8") == "Surface Capital passenger 1" for row in vehicles.values()):
            roles["Surface Capital passenger 1"] = ("PASS", None if allow_capital_return else 0)
    roads, selected, goods_income = {}, set(), {}
    # This one actual paid Livestock body may appear only in the explicit
    # later producer. Its absence before purchase never invents a vehicle.
    if allow_farm_road:
        assert phase == "component-delivery" and allow_capital_service and allow_goods_income
        if any(row["type"] == 1 and bytes.fromhex(row[ROAD + "name"]).decode("utf-8") == "Surface Farm livestock income" for row in vehicles.values()):
            roles["Surface Farm livestock income"] = ("LVST", 7)
    goods_names = {"Surface Launch Goods reserve 1", "Surface Launch Goods reserve 2"} if allow_goods_income else set()
    # Discovery follows actual names/roles, never recycled vehicle slots.
    # Commercial Steel has the same industry source but stays outside the
    # selected parked-reserve shipment cohort throughout this producer.
    for key, row in vehicles.items():
        if row["type"] != 1:
            continue
        name = bytes.fromhex(row[ROAD + "name"]).decode("utf-8"); assert name in roles and name not in roads
        label, source = roles[name]; assert labels[row[ROAD + "cargo_type"]] == label
        native = [dict(packet=reference - 1, **packets[reference - 1]) for reference in row[ROAD + "cargo.packets"]]
        if label == "PASS":
            allowed = (0, 1) if source is None else (source,)
            assert all(packet["source_type"] == 1 and packet["source_id"] in allowed for packet in native)
            # Only the explicitly admitted paid extra bus may collect Town1.
            # Original local buses retain SourceTown0, and regional services
            # keep their independent existing two-town contract. First20proves
            # real receiver-side pickup rather than a widened source default.
            if allow_capital_return and name == "Surface Capital passenger 1":
                assert phase == "component-delivery" and allow_capital_service
                assert key == 9 and row[ROAD + "owner"] == 0 and row[ROAD + "engine_type"] == 119 and row[ROAD + "cargo_cap"] == 40
                assert all(packet["source_id"] != 1 or packet["source"] == 20 for packet in native)
        # Only the explicit later trade observer may admit actual natural
        # Factory5Goods picked up at newly paid Station13stop2572 by3/25.
        # Native implicit visits create real commercial cargo, while all
        # historical defaults and the original fiftyGoods cohort stay strict.
        elif allow_factory_goods and name in goods_names:
            assert phase == "component-delivery" and allow_goods_income and allow_farm_road
            assert all(packet["source_type"] == 0 and packet["source_id"] in (5, 11) for packet in native)
            assert all(packet["source_id"] != 5 or (packet["source"] == 13 and packet["source_xy"] == 2572) for packet in native)
        else:
            assert all(packet["source_type"] == 0 and packet["source_id"] == source for packet in native)
        held = sum(packet["count"] for packet in native)
        assert held == sum(row[ROAD + "cargo.action_counts"]) <= row[ROAD + "cargo_cap"]
        roads[name] = dict(vehicle=key, label=label, held=held, packets=native, tile=row[ROAD + "tile"],
                           home=row["roadveh[0].state"] == 254 and bool(row[ROAD + "vehstatus"] & 2), capacity=row[ROAD + "cargo_cap"])
        if name in goods_names:
            expected = 3 if name.endswith("1") else 25
            assert key == expected and row[ROAD + "engine_type"] == 140 and row[ROAD + "cargo_cap"] == 18
            assert row[ROAD + "age"] < row[ROAD + "max_age"] and row[ROAD + "owner"] == 0
            goods_income[name] = dict(roads[name], engine=140, remaining=row[ROAD + "max_age"] - row[ROAD + "age"],
                stopped=bool(row[ROAD + "vehstatus"] & 2), profit256=row[ROAD + "profit_this_year"],
                previous_profit256=row[ROAD + "profit_last_year"], orders=row[ROAD + "orders"])
        # Only newly produced trade on the two explicitly reassigned carriers
        # leaves this historical cohort. Original center stock stays selected,
        # so loading any of its fiftyGoods would fail exact conservation.
        if name in SHIPMENTS and name not in goods_names:
            selected.update(packet["packet"] for packet in native)
    assert roads.keys() == roles.keys()
    # Only these two actual storage owners and the explicit reverse jobs may
    # add custody to the selected seven carriers. All other service stock is
    # excluded, including waiting IOREat member0 and commercial Steelat2.
    for key, owner in owners.items():
        if packet_labels[key] not in REQUIRED:
            continue
        # Explicit payload custody additionally follows the actual paid Space
        # body holds and fuel compartments; default delivery counts neither.
        eligible = owner[0] == "handling" or owner in (("station", 0), ("station", current["center"]))
        if payload:
            eligible = eligible or owner[0] == "supply" or owner in {("vehicle", craft["vehicle"]) for craft in paid}
        if core:
            eligible = eligible or owner[0] == "construction"
        if eligible:
            selected.add(key)
    sources = {"STEL": 10, "GOOD": 11, "PROP": 6}; totals = {label: 0 for label in REQUIRED}
    cohort = []
    for key in sorted(selected):
        label = packet_labels[key]; packet = packets[key]
        assert packet["source_type"] == 0 and packet["source_id"] == sources[label]
        totals[label] += packet["count"]; cohort.append(dict(packet=key, label=label, owner=owners[key], **packet))
    embodied = {label: 0 for label in REQUIRED}
    for craft in paid:
        embodied["STEL"] += 12 if craft["model"] == 14 else 4
        embodied["GOOD"] += 16 if craft["model"] == 14 else 8
    burned = sum(manifest["fuel_burned"] for manifest in manifests.values()) if mission else 0
    consumed = {"STEL": 0, "GOOD": 0, "PROP": 0}
    if building:
        consumed.update(zip(("STEL", "GOOD"), construction["row"]["consumed"]))
    acquired = {label: 0 for label in REQUIRED}
    if phase == "components":
        # Only actual newly loaded source packets on the five original empty
        # reserves extend this cohort. Existing center stock, burned fuel and
        # embodied/consumed receipts must still conserve the original batch.
        for name, (label, source) in SHIPMENTS.items():
            if label == "PROP":
                continue
            acquired[label] += roads[name]["held"]
        assert acquired["STEL"] <= 36 and acquired["GOOD"] <= 50
    elif phase == "component-delivery":
        # Completed15CFproved actual whole pickups. Their fixed quantities
        # must survive carrier/member/job/center transfers, even after all
        # reserve vehicles empty; present carrier holds cannot redefine supply.
        acquired.update(STEL=36, GOOD=50)
    if allow_propellant_pickup:
        # Actual original reserve2packets alone extend the twelve launch units.
        # An Industry production counter, refill39or a prospective pickup
        # supplies no acquired quantity; partial physical/reserved custody is
        # separately disclosed and cannot pass the stored-depot completion gate.
        fuel_carrier = roads["Surface Propellant reserve"]
        assert fuel_carrier["vehicle"] == 2 and fuel_carrier["capacity"] == 27 and fuel_carrier["held"] <= 26
        acquired["PROP"] = 26 if allow_propellant_delivery else fuel_carrier["held"]
    abandoned = {"STEL": 8, "GOOD": 2, "PROP": 0} if allow_component_abandonment else {label: 0 for label in REQUIRED}
    assert {label: totals[label] + embodied[label] + consumed[label] + abandoned[label] + (burned if label == "PROP" else 0) - acquired[label] for label in REQUIRED} == REQUIRED
    plant = industries[6]; assert plant["location.tile"] == 1639 and plant["type"] == 25 and plant["founder"] == 0
    history = counters(path, 6); state, = [row for row in TABLE(path, b"LPCR").values() if row["industry"] == 6]
    assert state["controller"] == 0 and state["recipe"] == int.from_bytes(b"PCHM", "little")
    assert history[1]["CHEM"] == history[2]["PROP"] == 2 * history[0]["PCHM"]
    plant_receipt = dict(consumed=history[1]["CHEM"], produced=history[2]["PROP"], dispatched=state["dispatched"], batches=history[0]["PCHM"])
    if allow_chemical_delivery:
        # Only this later original27unit truck load may feed additional PCHM
        # batches. Original42unit history remains separate; actual unconsumed
        # inputs, undispatched output and dispatched history conserve the new
        # receipt without placing any of it in launch storage by assumption.
        inputs = {labels[plant[f"accepted[{index}].cargo"]]: plant[f"accepted[{index}].waiting"] for index in range(plant["accepted.count"])}
        outputs = {labels[plant[f"produced[{index}].cargo"]]: plant[f"produced[{index}].waiting"] for index in range(plant["produced.count"])}
        extra = history[1]["CHEM"] - 42
        dispatched = state["dispatched"] - 42
        assert 0 <= extra <= 26 and 0 <= dispatched <= extra
        assert all(quantity == 0 for label, quantity in inputs.items() if label != "CHEM")
        assert all(quantity == 0 for label, quantity in outputs.items() if label != "PROP")
        assert outputs["PROP"] + dispatched == extra
        delivered_chemical = inputs["CHEM"] + extra
        assert 0 <= delivered_chemical <= 27 and roads["Surface Chemical reserve"]["held"] + delivered_chemical == 27
        plant_receipt.update(delivered=delivered_chemical, additional_consumed=extra, additional_produced=extra,
                             additional_dispatched=dispatched, chemical_input=inputs["CHEM"], propellant_output=outputs["PROP"])
        if allow_propellant_delivery:
            # Every original/new Source6unit must still exist in a sole native
            # packet or actual Industry output, except the original30burned by
            # the paid rocket. No outside waiting packet can hide duplication
            # or replace a missing part of the fixed26transport receipt.
            physical = sum(packet["count"] for key, packet in packets.items()
                           if packet_labels[key] == "PROP" and packet["source_type"] == 0 and packet["source_id"] == 6)
            assert physical + outputs["PROP"] + burned == history[2]["PROP"] == 68
    else:
        assert state["dispatched"] == history[1]["CHEM"] == 42
        assert all(plant[f"accepted[{index}].waiting"] == 0 for index in range(plant["accepted.count"]))
        assert all(plant[f"produced[{index}].waiting"] == 0 for index in range(plant["produced.count"]))
    if allow_chemical_pickup:
        assert roads["Surface Chemical reserve"]["held"] <= 27
    else:
        assert roads["Surface Chemical reserve"]["held"] == 0
    chemical = factory["counters"][1]["SALT"]
    assert chemical == factory["counters"][1]["OIL_"] == factory["counters"][2]["CHEM"]
    salt_delivery = None
    if allow_salt_delivery:
        # Only the additional original60Salt can feed new equal Salt/Oil
        # batches. Counters describe consumption; Industry buffers and actual
        # station packets remain the sole owners of unconsumed/new product.
        delivered_salt = factory["inputs"]["SALT"] + chemical - 120
        assert 120 <= chemical <= 180 and 0 <= delivered_salt <= 60
        assert factory["inputs"]["CHEM"] == 0 and 0 <= factory["outputs"]["CHEM"] <= chemical - 120
        salt_delivery = dict(delivered=delivered_salt, consumed=chemical - 120,
                             salt_input=factory["inputs"]["SALT"], chemical_output=factory["outputs"]["CHEM"],
                             oil_input=factory["inputs"]["OIL_"], processor=factory["processor"])
    else:
        assert chemical == 120
        assert factory["inputs"]["SALT"] == factory["inputs"]["CHEM"] == factory["outputs"]["CHEM"] == 0
    mine, = TABLE(path, b"LMNR").values(); deposit = TABLE(path, b"GDEP")[4]
    assert mine["extracted"] + deposit["remaining"] == 24000
    assert mine["extracted"] == industries[14]["produced[0].waiting"] + mine["dispatched"]
    service = lives(path); assert len(service) == 11 and min(row["remaining"] for row in service) > 0
    company, = TABLE(path, b"PLYR").values()
    result = dict(company=dict(cash=company["money"], debt=company["current_loan"]), facility=current, totals=totals, cohort=cohort,
                shipments={name: roads[name] for name in SHIPMENTS}, handling=transfers, service_lives=service,
                plant=plant_receipt, chemical_history=factory["counters"],
                finite_salt=dict(extracted=mine["extracted"], remaining=deposit["remaining"]),
                surviving_packets=len(packets), sole_packet_owners=len(owners))
    if allow_goods_income:
        assert goods_income.keys() == goods_names and totals["GOOD"] == 50
        result["goods_income"] = goods_income
    if allow_capital_service:
        added = roads.get("Surface Capital passenger 1")
        if added is not None:
            row = vehicles[added["vehicle"]]
            assert row[ROAD + "owner"] == 0 and row[ROAD + "engine_type"] == 119 and row[ROAD + "cargo_cap"] == 40
            assert row[ROAD + "age"] < row[ROAD + "max_age"]
            added = dict(added, engine=119, remaining=row[ROAD + "max_age"] - row[ROAD + "age"],
                         profit256=row[ROAD + "profit_this_year"], age=row[ROAD + "age"],
                         orders=row[ROAD + "orders"], stopped=bool(row[ROAD + "vehstatus"] & 2))
        result["capital_service"] = added
    if allow_farm_road:
        added = roads.get("Surface Farm livestock income")
        if added is not None:
            row = vehicles[added["vehicle"]]
            assert row[ROAD + "owner"] == 0 and row[ROAD + "engine_type"] == 137 and row[ROAD + "cargo_cap"] == 18
            assert row[ROAD + "age"] < row[ROAD + "max_age"]
            added = dict(added, engine=137, remaining=row[ROAD + "max_age"] - row[ROAD + "age"],
                         profit256=row[ROAD + "profit_this_year"], previous_profit256=row[ROAD + "profit_last_year"],
                         orders=row[ROAD + "orders"], stopped=bool(row[ROAD + "vehstatus"] & 2))
        result["farm_road"] = added
    if assembly:
        result["paid_craft"] = paid; result["embodied_inputs"] = embodied
    if payload:
        result["manifests"] = manifests
    if core:
        result["core"] = construction
    if building:
        result["orbital_consumed"] = consumed
    if phase in ("components", "component-delivery"):
        result["additional_collected"] = acquired
    if phase == "component-delivery":
        result["cargo_abandoned"] = abandoned
    if allow_salt_delivery:
        result["salt_delivery"] = salt_delivery
    if allow_chemical_pickup:
        selected_chemical = roads["Surface Chemical reserve"]
        row = vehicles[selected_chemical["vehicle"]]
        assert row[ROAD + "engine_type"] == 134 and row[ROAD + "cargo_cap"] == 27
        assert row[ROAD + "age"] < row[ROAD + "max_age"]
        reserved = row[ROAD + "cargo.action_counts"][3]
        result["chemical_carrier"] = dict(selected_chemical, engine=134, age=row[ROAD + "age"], life=row[ROAD + "max_age"],
                                           stored=selected_chemical["held"] - reserved, reserved=reserved)
    if allow_propellant_pickup:
        row = vehicles[fuel_carrier["vehicle"]]
        assert row[ROAD + "engine_type"] == 134 and row[ROAD + "age"] < row[ROAD + "max_age"]
        reserved = row[ROAD + "cargo.action_counts"][3]
        result["propellant_carrier"] = dict(fuel_carrier, engine=134, age=row[ROAD + "age"], life=row[ROAD + "max_age"],
                                            stored=fuel_carrier["held"] - reserved, reserved=reserved)
    return result
