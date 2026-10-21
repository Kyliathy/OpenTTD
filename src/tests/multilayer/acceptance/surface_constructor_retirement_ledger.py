"""Observe actual empty-home retirement without recovering consumed construction inputs.

@file surface_constructor_retirement_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVED.
Actual remaining bodies own packets; retired4Steel8Goodsremain consumed history.
The public native invoice and exact predecessor separately bind paid provenance.
"""
from surface_constructor_home_ledger import observe as home


def observe(path):
    """Select only original38/gen2retirement after its genuine completed Core and home arrival.

    @param path Actual original native checkpoint with live sole packet owners.
    @return Conserved current and retired embodied inputs, actual live hulls and original stock.
    """
    return home(path, allow_constructor_retirement=True)
