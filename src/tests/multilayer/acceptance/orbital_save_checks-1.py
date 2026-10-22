"""Decode original orbital construction/facility receipts for independent save comparisons.

@file orbital_save_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group14
Version gates follow the native SPST owner. Reading creates no gameplay object,
packet, utility grant or work; absent legacy fields have their explicit native defaults.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group8.0; pure reader ownership.
"""
from space_saved_state import structures




def construction_summary(records, kind):
    """Expose an actual kind's reservation, spent bill and facility readiness.

    @param records Independently decoded complete structure records.
    @param kind Exact native module kind. @return The unique delivered site's original economic state.
    """
    # Reservation scenarios deliberately retain another unfunded competing
    # blueprint of the same kind. Only the site holding delivered or already
    # consumed material supplies this matrix's economic milestone.
    (site, row), = [(site, row) for site, row in records["rows"].items()
                   if row["kind"] == kind and (any(row["consumed"]) or any(slot["quantity"] for slot in row["inputs"]))]
    return dict(site=site, **row)
