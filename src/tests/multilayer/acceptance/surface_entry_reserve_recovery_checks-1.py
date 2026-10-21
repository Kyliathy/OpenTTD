"""Recover actual working cash through freshly bounded original native service.

@file surface_entry_reserve_recovery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CS.
An actual100000cash stop is a measured result, never a projected funding source.
Partial native endpoints are retained without repeating their earned work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(DIRECTORY / "surface_entry_capital_recovery_checks-1.py"))
observe = PREVIOUS["observe"]
PHASE = "staged-chemical-capital-recovery-work"


def execute(configuration, receipt, recover_run=None):
    """Run one freshly costed native recovery interval from its exact real predecessor.

    @param configuration Independently linked focused native application.
    @param receipt Qualified original recovery quote or preceding actual partial work.
    @param recover_run Successful invocation to inspect without replaying paid progress.
    @return Actual cash restoration or preserved partial endpoint, native expense classes and strict owners.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    assert predecessor["phase"] in ("staged-chemical-capital-recovery-query", PHASE)
    if predecessor["phase"] == PHASE:
        previous, = predecessor["operation"]["rows"]["progress"]
        assert previous["partial"] and not previous["restored"]
    assert predecessor["operation"]["final"]["company"]["cash"] < 100000
    operation = native(configuration, PHASE, receipt, observe,
        dict(admission="RECOVERY_ADMISSION", progress="RECOVERY_PROGRESS", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        PREVIOUS["caller_files"]() + [Path(__file__)], recover_run=recover_run, chemical_limit=3000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == predecessor["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert last["new_fuel_at_center"] and not last["handling"]["jobs"]
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert len(first["service_lives"]) == len(last["service_lives"]) == 11
    assert first["capital_service"]["vehicle"] == last["capital_service"]["vehicle"] == 9
    admission, = rows["admission"]
    progress, = rows["progress"]
    earned, = rows["earned"]
    assert admission["funded"] and admission["target"] <= first["company"]["cash"]
    assert admission["future_income_counted"] == admission["refunds_counted"] == admission["credit"] == admission["purchases"] == 0
    assert progress["restored"] == (last["company"]["cash"] >= 100000) and progress["partial"] != progress["restored"]
    assert progress["cash"] == last["company"]["cash"] >= 40000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    assert 0 < elapsed == earned["earned"] == progress["ticks"] <= 3000
    assert progress["restored"] or elapsed == 3000
    assert earned["cash_before"] - earned["cash"] == sum(row["amount"] for row in rows["expenses"])
    assert earned["cash_before"] == first["company"]["cash"] and earned["cash"] == last["company"]["cash"]
    assert not progress["module_capital_complete"]
    return operation


def main():
    """Publish one immutable actual cash recovery or partial native work receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=PHASE, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
