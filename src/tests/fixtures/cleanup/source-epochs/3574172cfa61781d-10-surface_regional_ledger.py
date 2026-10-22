"""Observe explicitly admitted regional service while retaining all original cargo contracts.

@file surface_regional_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQA.
Only named additional buses admit actual Town0/1 passengers. Original local
service and the complete36Steel/50Goods/42Propellant inventories remain strict.
"""
from pathlib import Path
import runpy

from surface_fuel_refill_ledger import TABLE, observe as observe_fuel, reserve_identity

ORIGINAL_LIVES = runpy.run_path(str(Path(__file__).with_name("surface_entry_capital_checks-1.py")))["lives"]


def observe(path):
    """@param path Complete actual native world. @return Explicit regional owners plus unchanged full launch-stock obligations."""
    result = observe_fuel(path, phase="regional-service")
    assert len(result["passenger_buses"]) == 3
    assert len(result["regional_buses"]) in (0, 3)
    return result


def lives(path):
    """@param path Actual complete save. @return Every original eight and every genuinely purchased regional service age."""
    result = ORIGINAL_LIVES(path); regional = []
    for key, row in TABLE(path, b"VEHS").items():
        if row["type"] != 1:
            continue
        prefix = "roadveh[0].common[0]."; name = bytes.fromhex(row[prefix + "name"]).decode("utf-8")
        if not name.startswith("Surface Regional passenger "):
            continue
        assert row[prefix + "owner"] == 0 and not row[prefix + "vehstatus"] & 128
        regional.append(dict(vehicle=key, name=name, engine=row[prefix + "engine_type"], age=row[prefix + "age"],
                             remaining=row[prefix + "max_age"] - row[prefix + "age"]))
    assert len(regional) in (0, 3)
    return result + regional
