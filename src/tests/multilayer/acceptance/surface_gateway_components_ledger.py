"""Observe additional genuine factory pickups while preserving the first-Core cohort.

@file surface_gateway_components_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CF.
Only packets actually held by five bounded original carriers extend procurement.
Earlier default consumers still require their original fixed material totals.
"""
from surface_gateway_construction_ledger import observe as construction
from surface_regional_ledger import TABLE

CAPACITIES = {"Surface Steel reserve 1": 18, "Surface Steel reserve 2": 18,
              "Surface Goods reserve": 14, "Surface Launch Goods reserve 1": 18,
              "Surface Launch Goods reserve 2": 18}


def observe(path):
    """@param path Actual original native component checkpoint. @return Sole native cargo custody, bounded real acquisition and unchanged operational Space owners."""
    result = construction(path, allow_components=True)
    assert result["operational"] and result["fuel_burned"] == 30
    assert result["orbital_consumed"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    assert result["flight"]["started"] == result["flight"]["completed"] == 2
    shipments = result["shipments"]
    for name, capacity in CAPACITIES.items():
        assert shipments[name]["capacity"] == capacity
    for name in ("Surface Propellant reserve", "Surface Propellant refill"):
        assert shipments[name]["held"] == 0 and shipments[name]["home"] and shipments[name]["tile"] == 2443
    # No new material may appear outside the selected actual pickup vehicles.
    # In particular the original center stock and both paid Space holds stay
    # unchanged while this explicitly bounded acquisition is in progress.
    other = {label: 0 for label in ("STEL", "GOOD", "PROP")}
    for packet in result["cohort"]:
        if tuple(packet["owner"])[0] != "vehicle":
            assert tuple(packet["owner"]) == ("station", 21)
            other[packet["label"]] += packet["count"]
    assert other == {"STEL": 4, "GOOD": 2, "PROP": 12} and result["goods_holds"] == {54: 0, 38: 0}
    vehicles = TABLE(path, b"VEHS"); assert sum(row["type"] == 0 for row in vehicles.values()) == 35
    result["components_collected"] = all(shipments[name]["held"] == capacity and shipments[name]["home"] and shipments[name]["tile"] == 2443
                                          for name, capacity in CAPACITIES.items())
    return result
