"""Observe explicit additional Propellant from the actual refilled chemical feed.

@file surface_fuel_refill_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEH.
Actual native buffers, packets, lifetime counters and books retain all owners.
Default original and completed chemical-refill contracts remain unchanged.
"""
from surface_propellant_ledger import TABLE, observe as observe_phase, reserve_identity


def observe(path, phase="propellant-refill"):
    """@param path Actual complete native save. @param phase Explicit closed fuel or additional regional-service contract.
    @return Explicit42external-CHEMconversion and original/new fuel custody; default original bounds remain strict.
    """
    assert phase in ("propellant-refill", "regional-service")
    snapshot = observe_phase(path, phase=phase)
    old = snapshot["propellant_truck"]
    assert old["held"] == 20 and old["stopped"] and old["depot"] and old["tile"] == 2443
    assert sum(row["held"] for row in snapshot["additional_goods"]) == 36
    assert all(row["held"] == 18 and row["stopped"] and row["depot"] and row["tile"] == 2443 for row in snapshot["additional_goods"])
    assert snapshot["plant"]["produced"] >= 20
    return snapshot
