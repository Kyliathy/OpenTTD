"""Query genuine current adjacent expansion geometry, missing stock and bank bounds.

@file surface_entry_expansion_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CE.2.
All actual121native families must remain unchanged. An admitted legal footprint
cannot grant its missing materials, future income or unaffordable capital.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_construction_ledger import observe
from surface_gateway_native import execute as native

DIRECTORY = Path(__file__).resolve().parent


def execute(configuration, receipt, recover_run=None):
    """@param configuration Actual focused native build. @param receipt Exact original operational-Core predecessor.
    @param recover_run Successful native invocation to inspect without repetition. @return Pure actual layout/stock/bank/flight boundaries and exact native owners.
    """
    prefixes = dict(layout="EXPANSION_LAYOUT", inputs="EXPANSION_INPUT", fuel="EXPANSION_FUEL", budget="EXPANSION_BUDGET", flight="EXPANSION_FLIGHT", invoices="INVOICE")
    files = [Path(__file__)] + [DIRECTORY / name for name in ("surface_gateway_construction_ledger.py", "surface_gateway_resupply_ledger.py", "surface_gateway_core_ledger.py", "orbital_save_checks-1.py", "surface_gateway_mission_ledger.py", "surface_gateway_payload_ledger.py", "surface_gateway_material_ledger.py", "surface_gateway_assembly_ledger.py", "surface_gateway_facility_ledger.py")]
    current = native(configuration, "expansion-query", receipt, observe, prefixes, files, recover_run)
    before, output = [ROOT / current[field]["path"] for field in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1] and current["initial"] == current["final"] and not current["rows"]["invoices"]
    parent = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
    assert json.loads(json.dumps(current["initial"])) == parent["final"]
    rows = current["rows"]; layout, = rows["layout"]; budget, = rows["budget"]; fuel, = rows["fuel"]; flight, = rows["flight"]
    assert layout["core"] == 0 and layout["generation"] == 1 and layout["dock_tile"] == 11277 and layout["assembly_tile"] == 11467
    assert layout["dock_cost"] == 160000 and layout["assembly_cost"] == 180000
    assert layout["adjacent"] and layout["distinct"] and layout["admitted"]
    assert len(rows["inputs"]) == 2
    for row, needed, held, missing, mass in zip(rows["inputs"], (16, 28), (4, 2), (12, 26), (256, 224), strict=True):
        assert (row["needed"], row["held"], row["missing"], row["mass"]) == (needed, held, missing, mass) and not row["load_admitted"]
    assert (fuel["needed"], fuel["held"], fuel["missing"]) == (30, 12, 18) and not fuel["fuel_load_admitted"]
    assert budget["modules"] == 340000 and budget["missions"] == 10000 and budget["pad"] == 3000 and budget["work"] == 7000 and budget["reserve"] == 150000
    assert budget["base"] == 510000 and budget["cash"] == 164570 and budget["debt"] == 4960000 and budget["ceiling"] == 5000000
    assert budget["spendable"] == 204570 and budget["credit"] == 350000
    assert not budget["bank_admitted"] and not budget["funded"] and not budget["logistics_costed"]
    assert flight["target"] == 0 and flight["generation"] == 1 and flight["empty_freight_block"] == 5
    assert not flight["flight_admitted"] and flight["payload_missing"] and flight["fuel_missing"]
    return current


def main():
    """Retain actual pure expansion admission without overwriting any accepted source or native output."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    current = execute(args.configuration, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase="expansion-query", operation=current), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
