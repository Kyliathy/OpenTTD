"""Conserve the actual additional factory batch through native launch handling.

@file surface_gateway_component_delivery_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CG.
Completed15CFowns fixed acquired36Steel/50Goods. Original native containers own
their transport; this observer never treats an emptied carrier as lost supply.
"""
from surface_gateway_construction_ledger import observe as construction
from surface_gateway_components_ledger import CAPACITIES
from surface_regional_ledger import TABLE


def observe(path):
    """@param path Actual original additional-material delivery checkpoint. @return Fixed whole-batch conservation, actual member/job/center custody and original paid Space owners."""
    result = construction(path, allow_component_delivery=True)
    assert result["operational"] and result["fuel_burned"] == 30
    assert result["flight"]["started"] == result["flight"]["completed"] == 2
    assert result["orbital_consumed"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    assert result["totals"] == {"STEL": 40, "GOOD": 52, "PROP": 12} and result["goods_holds"] == {54: 0, 38: 0}
    stocks = {station: {label: 0 for label in ("STEL", "GOOD", "PROP")} for station in (0, 21)}
    # Read quantities only from actual globally unique packet owners. The
    # original twelve fuel units stay in center storage throughout delivery.
    for packet in result["cohort"]:
        owner = tuple(packet["owner"])
        if owner[0] == "station":
            stocks[owner[1]][packet["label"]] += packet["count"]
        if packet["label"] == "PROP":
            assert owner == ("station", 21)
    shipments = result["shipments"]
    for name, capacity in CAPACITIES.items():
        assert shipments[name]["capacity"] == capacity
    for name in ("Surface Propellant reserve", "Surface Propellant refill"):
        assert shipments[name]["held"] == 0 and shipments[name]["home"] and shipments[name]["tile"] == 2443
    returned = all(shipments[name]["held"] == 0 and shipments[name]["home"] and shipments[name]["tile"] == 2443 for name in CAPACITIES)
    result["delivery_stocks"] = stocks
    result["component_carriers_returned"] = returned
    result["components_at_member"] = returned and not result["handling"]["jobs"] and stocks[0] == {"STEL": 36, "GOOD": 50, "PROP": 0}
    result["components_at_center"] = returned and not result["handling"]["jobs"] and stocks[21] == {"STEL": 40, "GOOD": 52, "PROP": 12}
    assert sum(row["type"] == 0 for row in TABLE(path, b"VEHS").values()) == 35
    return result
