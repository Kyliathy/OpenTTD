"""Conserve the original128launch units through physical shipment and native handling.

@file surface_gateway_material_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEQL/15BIEQM.
Only actual vehicle, station and LHND packet references own inventory. This
observer does not admit assembly consumption or alter earlier closed observers.
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


def observe(path, phase="delivery"):
    """@param path Actual original world before/during/after native shipment and handling.
    @param phase Explicit delivery or paid-assembly contract; default retains all original128physical units and empty hull slots.
    @return Complete sole packet custody, exact selected36/50/42cohort, original processors, useful services and native books.
    """
    assert phase in ("delivery", "assembly"); assembly = phase == "assembly"
    current = facility(path, allow_craft=assembly); assert current["center"] is not None
    paid = []
    if assembly:
        from surface_gateway_assembly_ledger import production
        paid = production(path, current)
    factory = FACTORY["observe_factory"](path); labels = factory["cargo_labels"]
    vehicles, stations, packets, industries = [TABLE(path, key) for key in (b"VEHS", b"STNN", b"CAPA", b"INDY")]
    transfers = handling(path, labels, current); owners, packet_labels = {}, {}

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
            assert key in {craft["vehicle"] for craft in paid} and not row[prefix + "cargo.packets"]
        for reference in row[prefix + "cargo.packets"]:
            claim(reference - 1, ("vehicle", key), row[prefix + "cargo_type"])
    for job in transfers["jobs"]:
        for key, hop in job["references"]:
            claim(key, ("handling", job["id"]), job["cargo"])
        held = sum(packets[key]["count"] for key, _ in job["references"])
        assert 0 < held <= 64 and job["remaining"] == (held + 7) // 8
    assert owners.keys() == packets.keys()
    roles = dict(SHIPMENTS)
    roles.update({"Surface ore delivery": ("IORE", 9), "Surface Steel delivery": ("STEL", 10), "Surface Chemical reserve": ("CHEM", 15)})
    roles.update({f"Surface Launch passenger {index}": ("PASS", 0) for index in range(1, 4)})
    roles.update({f"Surface Regional passenger {index}": ("PASS", None) for index in range(1, 4)})
    roads, selected = {}, set()
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
        else:
            assert all(packet["source_type"] == 0 and packet["source_id"] == source for packet in native)
        held = sum(packet["count"] for packet in native)
        assert held == sum(row[ROAD + "cargo.action_counts"]) <= row[ROAD + "cargo_cap"]
        roads[name] = dict(vehicle=key, label=label, held=held, packets=native, tile=row[ROAD + "tile"],
                           home=row["roadveh[0].state"] == 254 and bool(row[ROAD + "vehstatus"] & 2), capacity=row[ROAD + "cargo_cap"])
        if name in SHIPMENTS:
            selected.update(packet["packet"] for packet in native)
    assert roads.keys() == roles.keys()
    # Only these two actual storage owners and the explicit reverse jobs may
    # add custody to the selected seven carriers. All other service stock is
    # excluded, including waiting IOREat member0 and commercial Steelat2.
    for key, owner in owners.items():
        if packet_labels[key] not in REQUIRED:
            continue
        eligible = owner[0] == "handling" or owner in (("station", 0), ("station", current["center"]))
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
    assert {label: totals[label] + embodied[label] for label in REQUIRED} == REQUIRED
    plant = industries[6]; assert plant["location.tile"] == 1639 and plant["type"] == 25 and plant["founder"] == 0
    history = counters(path, 6); state, = [row for row in TABLE(path, b"LPCR").values() if row["industry"] == 6]
    assert state["controller"] == 0 and state["recipe"] == int.from_bytes(b"PCHM", "little") and state["dispatched"] == 42
    assert history[1]["CHEM"] == history[2]["PROP"] == 2 * history[0]["PCHM"] == 42
    assert all(plant[f"accepted[{index}].waiting"] == 0 for index in range(plant["accepted.count"]))
    assert all(plant[f"produced[{index}].waiting"] == 0 for index in range(plant["produced.count"]))
    assert roads["Surface Chemical reserve"]["held"] == 0
    assert factory["counters"][1]["SALT"] == factory["counters"][1]["OIL_"] == factory["counters"][2]["CHEM"] == 120
    assert factory["inputs"]["SALT"] == factory["inputs"]["CHEM"] == factory["outputs"]["CHEM"] == 0
    mine, = TABLE(path, b"LMNR").values(); deposit = TABLE(path, b"GDEP")[4]
    assert mine["extracted"] + deposit["remaining"] == 24000
    assert mine["extracted"] == industries[14]["produced[0].waiting"] + mine["dispatched"]
    service = lives(path); assert len(service) == 11 and min(row["remaining"] for row in service) > 0
    company, = TABLE(path, b"PLYR").values()
    result = dict(company=dict(cash=company["money"], debt=company["current_loan"]), facility=current, totals=totals, cohort=cohort,
                shipments={name: roads[name] for name in SHIPMENTS}, handling=transfers, service_lives=service,
                plant=dict(consumed=42, produced=42, dispatched=42, batches=21), chemical_history=factory["counters"],
                finite_salt=dict(extracted=mine["extracted"], remaining=deposit["remaining"]),
                surviving_packets=len(packets), sole_packet_owners=len(owners))
    if assembly:
        result["paid_craft"] = paid; result["embodied_inputs"] = embodied
    return result
