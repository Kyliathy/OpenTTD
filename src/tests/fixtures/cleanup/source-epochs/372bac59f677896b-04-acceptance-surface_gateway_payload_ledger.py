"""Conserve original loaded constructor freight, rocket fuel and reciprocal native carry custody.

@file surface_gateway_payload_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEQN/15C.
Actual native Space bodies and SPSUcompartments retain sole packet ownership.
No flight, fuel burn, orbital work or new hull belongs to this ground contract.
"""
import struct

from space_preview_checks import chunks
from surface_regional_ledger import TABLE


def supplies(path, craft, allow_mission=False):
    """@param path Complete actual original ground-payload save. @param craft Independently decoded paid native hull receipts.
    @return Bounded native compartment/carry records, cross-checked against their actual generation-qualified paid bodies.
    @param allow_mission Explicit first mission consumer; default refuses every fuel burn and deployed placement.
    """
    models = {row["model"]: row for row in craft}; assert models.keys() == {14, 20}
    by_id = {row["vehicle"]: row for row in craft}; data = chunks(path)[1][b"SPSU"][3:]
    count, offset, records = data[0], 1, {}; assert count in (1, 2)
    # Decode each actual manifest's original variable packet references.
    # History, a passenger link and compartment quantities cannot substitute
    # for the single native packet-owning list saved in each supply slot.
    for _ in range(count):
        values = struct.unpack_from(">IQIQIQHHBBBBQQB", data, offset); offset += 61
        vehicle, generation, passenger, passenger_generation, carrier, carrier_generation, age, service, returns, paused, pending, location, burned, completed, pods = values
        assert vehicle in by_id and vehicle not in records and generation == by_id[vehicle]["generation"]
        assert age < 185 and service == paused == pending == location == completed == pods == 0
        assert burned in ((0, 10, 15) if allow_mission and by_id[vehicle]["model"] == 14 else (0,))
        assert returns == (2 if by_id[vehicle]["model"] == 20 else 0)
        compartments = []
        for index in range(3):
            size = data[offset]; offset += 1
            references = [struct.unpack_from(">IH", data, offset + item * 6) for item in range(size)]; offset += 6 * size
            assert index == 0 or size == 0
            compartments.append(references)
        records[vehicle] = dict(generation=generation, passenger=(passenger, passenger_generation), carrier=(carrier, carrier_generation),
                                age=age, returns=returns, fuel_burned=burned, supplies=compartments)
    assert offset == len(data) and models[20]["vehicle"] in records
    vehicles, packets = TABLE(path, b"VEHS"), TABLE(path, b"CAPA"); rocket, constructor = models[14], models[20]
    assert rocket["placement"] in ((2, 5) if allow_mission else (2,))
    assert constructor["placement"] in ((0, 4) if allow_mission else (2, 4))
    empty = (0xFFFFF, 0); rocket_manifest = records.get(rocket["vehicle"])
    constructor_manifest = records[constructor["vehicle"]]
    assert constructor_manifest["passenger"] == empty and all(not slot for slot in constructor_manifest["supplies"])
    if rocket_manifest is not None:
        assert rocket_manifest["carrier"] == empty
        fuel = sum(packets[key]["count"] for key, _ in rocket_manifest["supplies"][0])
        if allow_mission:
            assert fuel + rocket_manifest["fuel_burned"] == 30
        else:
            assert fuel in (0, 30)
    if constructor["placement"] == 4:
        assert rocket_manifest is not None
        assert rocket_manifest["passenger"] == (constructor["vehicle"], constructor["generation"])
        assert constructor_manifest["carrier"] == (rocket["vehicle"], rocket["generation"])
    else:
        assert constructor_manifest["carrier"] == empty
        assert rocket_manifest is None or rocket_manifest["passenger"] == empty
    # This first flight carries the construction materials inside the paid
    # constructor. Loose rocket freight cannot become a substitute passenger.
    assert not vehicles[rocket["vehicle"]]["space[0].common[0].cargo.packets"]
    held = sum(packets[reference - 1]["count"] for reference in vehicles[constructor["vehicle"]]["space[0].common[0].cargo.packets"])
    assert held in (0, 16)
    return records


def observe(path):
    """@param path Actual original preflight ground world. @return Strict20Steel/26Goods/42PROPphysical plus original paid embodied material and native carry/fuel owners."""
    from surface_gateway_material_ledger import observe as material
    result = material(path, phase="payload")
    assert result["totals"] == {"STEL": 20, "GOOD": 26, "PROP": 42}
    assert result["embodied_inputs"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    constructor, = [craft for craft in result["paid_craft"] if craft["model"] == 20]
    cargo = [packet for packet in result["cohort"] if tuple(packet["owner"]) == ("vehicle", constructor["vehicle"])]
    assert all(packet["label"] == "STEL" for packet in cargo)
    if constructor["placement"] == 4:
        assert sum(packet["count"] for packet in cargo) == 16
    return result
