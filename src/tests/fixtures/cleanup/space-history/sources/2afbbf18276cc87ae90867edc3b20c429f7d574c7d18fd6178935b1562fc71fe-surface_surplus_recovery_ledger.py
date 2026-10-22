"""Observe only native retirement of the two additional surplus Livestock bodies.

@file surface_surplus_recovery_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEB.
Every original wagon and all active mixed capacity retain physical ownership.
"""
from surface_farm_backhaul_ledger import observe as mixed


def observe(path):
    """Select only the later actual surplus retirement, preserving the exact predecessor.

    @param path Actual original native save with sole cargo/vehicle owners.
    @return Current owned bodies and real material custody; no prospective refund or income.
    """
    return mixed(path, allow_surplus_retirement=True)
