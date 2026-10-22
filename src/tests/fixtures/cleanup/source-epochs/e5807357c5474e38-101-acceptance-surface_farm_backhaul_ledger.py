"""Observe actual paid farm backhaul without broadening earlier material contracts.

@file surface_farm_backhaul_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVD.
Native packets and train links remain the sole inventory and capacity owners.
"""
from surface_gateway_component_delivery_ledger import observe_propellant_delivery
from surface_regional_ledger import TABLE


def observe(path):
    """Select only the genuine mixed farm extension of the current paid world.

    @param path Actual original native checkpoint, never manufactured save data.
    @return Conserved original materials and exact current service/reserve bodies.
    """
    result = observe_propellant_delivery(path, allow_capital_service=True, allow_goods_income=True,
        allow_farm_capacity=True, allow_farm_balance=True, allow_farm_road=True,
        allow_factory_goods=True, allow_capital_return=True, allow_retired_refill=True, allow_farm_backhaul=True)
    assert result["components_at_center"] and result["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    factory = TABLE(path, b"INDY")[5]
    assert factory["type"] == 6 and factory["location.tile"] == 3344 and factory["produced[0].cargo"] == 5
    return result
