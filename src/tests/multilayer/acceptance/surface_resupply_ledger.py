"""Observe the explicit finite Salt and chemical refill without weakening closed proofs.

@file surface_resupply_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEG.
All quantities come from actual native industry, packet, vehicle and financial
owners. This named producer preserves original plant20PROP/1CHEM unchanged.
"""
from surface_propellant_ledger import TABLE, observe as observe_phase, reserve_identity


def observe(path):
    """@param path Actual complete native save. @return Explicit chemical-refill custody and finite-source reconciliation."""
    snapshot = observe_phase(path, phase="chemical-refill")
    assert snapshot["plant"]["consumed"] == snapshot["plant"]["produced"] == snapshot["plant"]["dispatched"] == 20
    assert snapshot["plant"]["chemicals"] == 1 and snapshot["plant"]["buffered"] == 0
    assert snapshot["propellant_truck"]["held"] == 20 and snapshot["propellant_truck"]["stopped"] and snapshot["propellant_truck"]["depot"]
    assert sum(row["held"] for row in snapshot["additional_goods"]) == 36
    assert all(row["held"] == 18 and row["stopped"] and row["depot"] and row["tile"] == 2443 for row in snapshot["additional_goods"])
    return snapshot
