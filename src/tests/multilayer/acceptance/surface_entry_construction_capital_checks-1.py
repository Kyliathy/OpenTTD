"""Price the genuine missing builder and module capital from actual restored cash.

@file surface_entry_construction_capital_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEG.
Pure native estimates create no capital, packet, owner, refund or earned work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_original_evidence import verify
from surface_constructor_retirement_ledger import observe
from surface_construction_capital_native import execute as native
from surface_evidence_identity_cache import IdentityCache

HERE = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(HERE / "surface_entry_working_cash_checks-3.py"))
VERSIONS = PREVIOUS["VERSIONS"] + [ROOT / "logs/surface-entry-construction-capital-before-edit.json"]


def execute(configuration, recover_run=None):
    """Query one exact current original without spending or changing native owners.

    @param configuration Independently linked isolated native application.
    @param recover_run Exact successful native invocation to inspect without repetition.
    @return Actual unchanged owners and explicit native capital/material/cost proposals.
    """
    receipt = ROOT / f"logs/surface-entry-working-cash-work-004-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    prior = previous["operation"]
    assert prior["reserve_scope"]["end_reason"] == "working_floor" and prior["working_floor_restored"]
    assert prior["final"]["company"] == dict(cash=101093, debt=5000000)
    cache = IdentityCache()
    # Every consumed original authority is verified before deduplication;
    # archived epochs qualify changes without rebasing historical raw hashes.
    for reference in prior["sources"] + [prior["application"]]:
        verify(reference, VERSIONS, identity_reader=cache)
    attempt = ROOT / f"logs/surface-entry-construction-capital-query-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the exact native invocation"
        attempt.write_text(json.dumps(dict(_documentation=__doc__, receipt=identity(receipt), paid=0, elapsed=0), indent=2)+"\n", encoding="utf-8")
    else:
        assert json.loads(attempt.read_text(encoding="utf-8"))["receipt"] == identity(receipt)
    files = [ROOT / reference["path"] for reference in prior["sources"] if verify(reference, VERSIONS, identity_reader=cache)] + [
        Path(__file__), ROOT / "src/tests/surface_entry_construction_capital.h", ROOT / "src/space_structure.cpp",
        ROOT / "src/space_structure_cmd.h", ROOT / "src/launch_operations.cpp", ROOT / "src/orbital_station.cpp",
        ROOT / "src/multilayer_space_content.cpp", ROOT / "src/multilayer_space_content.h"] + VERSIONS
    operation = native(configuration, "construction-capital-query", receipt, observe,
        dict(modules="CONSTRUCTION_CAPITAL_MODULE", inputs="CONSTRUCTION_CAPITAL_INPUT",
             budget="CONSTRUCTION_CAPITAL_BUDGET", savings="CONSTRUCTION_CAPITAL_SAVINGS", services="CAPITAL_SERVICE"),
        list(dict.fromkeys(files)), recover_run=recover_run, work_limit=64)
    cache.assert_unchanged()
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"] and first == last
    before, output = [ROOT / operation[name]["path"] for name in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    assert len(rows["modules"]) == len(rows["inputs"]) == 2
    assert [row["capital"] for row in rows["modules"]] == [160000, 180000]
    assert all(row["admitted"] and not row["actual_created"] for row in rows["modules"])
    assert [row["needed"] for row in rows["inputs"]] == [20, 36]
    assert [row["held"] for row in rows["inputs"]] == [32, 50]
    assert all(row["missing"] == row["recovered_retired"] == 0 for row in rows["inputs"])
    budget, = rows["budget"]
    savings, = rows["savings"]
    assert budget["cash"] == 101093 and budget["debt"] == 5000000 and budget["working_floor"] == 100000
    assert budget["builder"] == 100000 and budget["builder_work"] == 148 and budget["base"] == 610000
    assert budget["minimum"] == 617500 and budget["gap"] == 516407 and budget["cash_milestone"] == 650000
    assert budget["fuel"] == 45 and budget["held_fuel"] == 38 and budget["fuel_missing"] == 7
    assert not budget["full_logistics_priced"] and not budget["funded"] and not budget["spending_permission"]
    assert savings["funded"] and savings["reserve"] == 40000 and savings["target"] == savings["operating"]+40000
    assert savings["future_income_counted"] == 0 and not savings["spending_permission"]
    assert rows["services"] and all(row["remaining"] > 0 for row in rows["services"])
    operation.update(earned_ticks=0, actual_cash=101093, working_floor_policy=100000, capital_milestone=650000,
        incomplete_module_base=610000, priced_minimum=617500, full_logistics_priced=False, module_capital_complete=False,
        minimum_current_life=min(row["remaining"] for row in rows["services"]),
        historical_versions=[identity(path) for path in VERSIONS])
    return operation


def main():
    """Retain one exact pure capital proposal and its independently linked native process."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    output = ROOT / f"logs/surface-entry-construction-capital-query-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase="construction-capital-query", operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["rows"]["budget"], "minimum_life", operation["minimum_current_life"], flush=True)


if __name__ == "__main__":
    main()
