"""Conserve the genuine first Core's Goods through the same rocket's second flight.

@file surface_gateway_resupply_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CC/15CIE/15CK/15CL/15CM/15CN/15CP/15CSGB.
The job retains16Steel exclusively; actual24Goods change native station/rocket/
constructor owners once. Thirty burned fuel plus twelve remaining conserve42.
"""
import struct

from space_preview_checks import chunks
from surface_gateway_core_ledger import orders
from surface_gateway_material_ledger import observe as material


def observe(path, allow_construction=False, allow_components=False, allow_component_delivery=False, allow_component_abandonment=False, allow_salt_delivery=False, allow_chemical_pickup=False, allow_chemical_delivery=False, allow_propellant_pickup=False, allow_propellant_delivery=False, allow_capital_service=False, allow_goods_income=False):
    """@param path Actual original Goods resupply checkpoint. @param allow_construction Explicit later real Core delivery/work/activation consumer; the default stays unworked.
    @param allow_components Explicit later factory collection on original bounded carriers; requires completed construction.
    @param allow_component_delivery Explicit delivery of the already acquired fixed36/50; earlier collection remains based on current actual holds.
    @param allow_component_abandonment Explicit actual ordinary-offer loss after its native transport boundary. @return Sole packet/craft/job owners and exact native second-flight receipts.
    @param allow_salt_delivery Explicit subsequent original finite-Salt chemical delivery; default retains historical factory counters.
    @param allow_chemical_pickup Explicit actual renewed Source15CHEM pickup without any new PCHM/fuel production.
    @param allow_chemical_delivery Explicit actual later27CHEMdelivery/PCHMproduction, retaining original first-Core/launch fuel receipts.
    @param allow_propellant_pickup Explicit actual Source6pickup on original reserve2, adding only current real packet custody to acquired PROP.
    @param allow_propellant_delivery Explicit subsequent transport of the completed fixed26PROPacquisition; historical first-Core fuel receipts stay unchanged.
    @param allow_capital_service Explicit later single paid Town0PASSbus; every historical default keeps its exact original roles.
    @param allow_goods_income Explicit later reuse of only real empty140/cap18reserves3/25; new Source11trade stays outside original fiftyGoods and old defaults remain strict.
    """
    assert not (allow_components and allow_component_delivery)
    assert not (allow_components or allow_component_delivery) or allow_construction
    phase = "component-delivery" if allow_component_delivery else "components" if allow_components else "construction" if allow_construction else "resupply"
    result = material(path, phase=phase, allow_component_abandonment=allow_component_abandonment, allow_salt_delivery=allow_salt_delivery, allow_chemical_pickup=allow_chemical_pickup, allow_chemical_delivery=allow_chemical_delivery, allow_propellant_pickup=allow_propellant_pickup, allow_propellant_delivery=allow_propellant_delivery, allow_capital_service=allow_capital_service, allow_goods_income=allow_goods_income); queues = orders(path, allow_construction=allow_construction)
    core = result["core"]; assert core is not None
    assert core["row"]["inputs"][0]["quantity"] + core["row"]["consumed"][0] == 16
    rocket, = [craft for craft in result["paid_craft"] if craft["model"] == 14]
    constructor, = [craft for craft in result["paid_craft"] if craft["model"] == 20]
    assert (rocket["vehicle"], rocket["generation"]) == (54, 1) and (constructor["vehicle"], constructor["generation"]) == (38, 2)
    assert constructor["placement"] == 0 and constructor["deployed"]
    holds = {}
    for craft in (rocket, constructor):
        freight = [packet for packet in result["cohort"] if tuple(packet["owner"]) == ("vehicle", craft["vehicle"])]
        assert all(packet["label"] == "GOOD" for packet in freight)
        holds[craft["vehicle"]] = sum(packet["count"] for packet in freight)
    assert sum(holds.values()) in (0, 24)
    assert result["embodied_inputs"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    manifest = result["manifests"][54]; burned = manifest["fuel_burned"]
    acquired = result.get("additional_collected", {"STEL": 0, "GOOD": 0, "PROP": 0})
    abandoned = result.get("cargo_abandoned", {"STEL": 0, "GOOD": 0, "PROP": 0})
    assert result["totals"] == {"STEL": 20 - core["row"]["consumed"][0] + acquired["STEL"] - abandoned["STEL"], "GOOD": 26 - core["row"]["consumed"][1] + acquired["GOOD"] - abandoned["GOOD"], "PROP": 42 - burned + acquired["PROP"]}
    raw = chunks(path)[1][b"SPFL"][3:]; assert len(raw) == 85 and raw[0] == 1
    values = struct.unpack_from(">IQBBBHQHQBIQiiHBBQQQ", raw, 1)
    vehicle, generation, phase, blocked, layer, source, source_generation, recovery, recovery_generation, kind, target, target_generation, x, y, remaining, remainder, flags, started, completed, canceled = values
    assert (vehicle, generation) == (54, 1) and phase in range(6) and blocked == canceled == 0 and remainder < 74
    assert started in (1, 2) and completed == (started if phase == 0 else 1)
    if phase == 0:
        assert (source, source_generation, recovery, recovery_generation) == (65535, 0, 65535, 0)
        assert kind == 0 and target == 0xFFFFFFFF and target_generation == x == y == remaining == flags == 0
        assert rocket["placement"] == 2 and layer == 0 and burned == (15 if completed == 1 else 30)
    else:
        assert started == 2 and (source, source_generation) == (recovery, recovery_generation) == (21, 1)
        assert (kind, target, target_generation) == (4, 38, 2) and rocket["placement"] == 5
        assert layer == (2 if phase in (3, 4) else 0)
        assert burned == (15 if phase == 1 else 25 if phase in (2, 3) else 30)
        assert flags == (0 if phase == 1 else 1 if phase in (2, 3) else 11)
        assert holds[54] == (24 if phase in (1, 2, 3) else 0)
        assert holds[38] == (0 if phase in (1, 2, 3) else 24)
    if started == 2:
        if not allow_construction:
            assert not queues["queues"][38]["entries"]
    if completed == 2:
        assert holds[38] + core["row"]["inputs"][1]["quantity"] + core["row"]["consumed"][1] == 24 and holds[54] == 0
    result["space_orders"] = queues; result["fuel_burned"] = burned
    result["flight"] = dict(vehicle=vehicle, generation=generation, phase=phase, blocked=blocked, layer=layer,
                            source=(source, source_generation), recovery=(recovery, recovery_generation), destination=(kind, target, target_generation),
                            departure=(x, y), remaining=remaining, pad_remainder=remainder, flags=flags, started=started, completed=completed, canceled=canceled)
    result["goods_holds"] = holds
    return result
