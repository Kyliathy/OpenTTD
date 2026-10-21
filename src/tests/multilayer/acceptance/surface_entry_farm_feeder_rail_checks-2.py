"""Bind actual feeder-to-rail boarding/delivery results and exact ordinary originals.

@file surface_entry_farm_feeder_rail_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGS.
Earlier train cargo payments never close this new cohort's delivery gate.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_farm_feeder_rail_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain exact native/ordinary evidence and only literal new-cohort final-delivery outcomes."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration, ROOT / args.receipt, observe, "farm-feeder-rail-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-farm-feeder-follow-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-farm-feeder-rail/retained.json"],
            ("farm-feeder-rail-01",), "farm-feeder-rail-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/farm-feeder-rail-products" if args.retain else None)
        actual = [json.loads((ROOT / f"logs/surface-entry-farm-feeder-rail-01-{config}.json").read_text(encoding="utf-8"))["operation"]["measured_cash_flow"]
                  for config in ("RelWithDebInfo", "Debug")]
        assert actual[0] == actual[1]
        result.update(_documentation=__doc__, actual_new_cohort_boarded=actual[0]["feeder_cohort_boarded"],
                      actual_new_cohort_delivered=actual[0]["feeder_cohort_delivered"],
                      actual_first_final_delivery=actual[0]["feeder_cohort_delivered"] > 0,
                      actual_reserve_restored=all(row["observation"]["company"]["cash"] >= 100000 for row in result["configurations"]),
                      feeder_credit_is_cash=False, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
