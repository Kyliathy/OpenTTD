"""Conserve actual first-Core deliveries, paid portable work and native panel activation.

@file surface_gateway_construction_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CD/15CIE/15CK/15CL/15CM/15CN/15CP/15CSGB.
Reserved SPSTpackets and once-only consumed receipts are distinct quantities.
The actual UPLDsource is the sole energy owner; this observer grants none.
"""
from surface_gateway_resupply_ledger import observe as resupply
from surface_regional_ledger import TABLE


def observe(path, allow_components=False, allow_component_delivery=False, allow_component_abandonment=False, allow_salt_delivery=False, allow_chemical_pickup=False, allow_chemical_delivery=False, allow_propellant_pickup=False, allow_propellant_delivery=False, allow_capital_service=False, allow_goods_income=False, allow_farm_road=False, allow_factory_goods=False, allow_capital_return=False, allow_retired_refill=False, allow_farm_backhaul=False):
    """@param path Actual original first-Core construction/activation world. @param allow_components Explicit later real factory collection, with original Core and strict historical receipts retained.
    @param allow_component_delivery Explicit transport of the actually completed fixed36/50acquisition.
    @param allow_component_abandonment Explicit after-boundary actual ordinary-offer attrition, retaining all original Core receipts. @return Exact physical/reserved/consumed stock, original paid owners and real shared panel.
    @param allow_salt_delivery Explicit subsequent physical finite-Salt receiver/production; earlier construction defaults remain strict.
    @param allow_chemical_pickup Explicit subsequent renewed chemical pickup; original Core activation and receipts remain strict.
    @param allow_chemical_delivery Explicit subsequent PCHMdelivery/production; original paid Core and material ownership remain strict.
    @param allow_propellant_pickup Explicit actual current Source6reserve2pickup; historical Core fuel/launch receipts remain unchanged.
    @param allow_propellant_delivery Explicit completed26fuel receipt through actual member/job/center transport; original paid Core ownership remains strict.
    @param allow_capital_service Explicit later single paid Town0PASSbus; every historical default keeps its exact original roles.
    @param allow_goods_income Explicit later reuse of only real empty140/cap18reserves3/25; new Source11trade stays outside original fiftyGoods and old defaults remain strict.
    @param allow_farm_road Explicit later sole named137/cap18Roadbody with only real Industry7Livestock; all original role/material defaults stay strict.
    @param allow_factory_goods Explicit later actual3/25native Source5Goods picked up at realStation13/paid2572; all historical source defaults and original fiftyGoods remain strict.
    @param allow_capital_return Explicit later extra9/owner0/model119/cap40 Town1return packets first20; default and original local sources remain strict.
    @param allow_retired_refill Explicit later native retirement of only empty duplicate vehicle 39 after completed fuel delivery; retained carrier 2 and all historical defaults remain strict.
    @param allow_farm_backhaul Explicit Group15CSGZVD actual paid Goods reuse of retired39slot; historical defaults remain unchanged.
    """
    result = resupply(path, allow_construction=True, allow_components=allow_components, allow_component_delivery=allow_component_delivery, allow_component_abandonment=allow_component_abandonment, allow_salt_delivery=allow_salt_delivery, allow_chemical_pickup=allow_chemical_pickup, allow_chemical_delivery=allow_chemical_delivery, allow_propellant_pickup=allow_propellant_pickup, allow_propellant_delivery=allow_propellant_delivery, allow_capital_service=allow_capital_service, allow_goods_income=allow_goods_income, allow_farm_road=allow_farm_road, allow_factory_goods=allow_factory_goods, allow_capital_return=allow_capital_return, allow_retired_refill=allow_retired_refill, allow_farm_backhaul=allow_farm_backhaul); row = result["core"]["row"]; module = row["module"]
    assert result["fuel_burned"] == 30 and result["flight"]["completed"] == result["flight"]["started"] == 2
    sources = [value for value in TABLE(path, b"UPLD").values() if value["type"] == 0 and value["layer"] == 2]
    panel = None
    if module is None:
        assert not sources
    else:
        assert row["phase"] == 1 and row["work"] == 740 and module["stock"] == {} and 0 <= module["activation"] <= 74
        assert module["consumer"] == module["assembly_consumer"] == 0xFFFFFFFF and module["enabled"] == 1
        assert module["source"] == 65535 and module["source_generation"] == module["completed"] == module["manufacture_serial"] == 0
        assert module["manufacture"] is None
        for name in ("service", "dock_service"):
            job = module[name]
            assert job["craft"] == 0xFFFFF and job["generation"] == job["ticks"] == job["paused"] == job["interval"] == job["serial"] == 0
            assert job["inputs"] == [dict(quantity=0, references=[]), dict(quantity=0, references=[])]
        panel, = sources
        assert panel["id"] == module["producer"] and panel["owner"] == 0 and panel["layer"] == 2 and panel["kind"] == 5 and panel["active"] == 1 and panel["domain"] == 0
        assert panel["a"] in (0, 30000) and panel["c"] == panel["lifetime_consumed"] == 0
    result["panel"] = panel; result["operational"] = module is not None and module["activation"] == 0
    return result
