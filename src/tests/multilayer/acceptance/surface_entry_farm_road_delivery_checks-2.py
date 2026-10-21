"""Bind the real first farm-road delivery followthrough and exact ordinary originals.

@file surface_entry_farm_road_delivery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGO.
Only actual payment/cash can close its respective gate; a late delivery remains
an explicit partial outcome rather than a forecast or replayed native interval.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_farm_road_delivery_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain exact followthrough/ordinary products and literal first-payment/reserve results."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "farm-road-delivery-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-farm-road-work-verification.json", [],
            ("farm-road-delivery-work-01",), "farm-road-delivery-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/farm-road-delivery-products" if args.retain else None)
        paid = [json.loads((ROOT / f"logs/surface-entry-farm-road-delivery-work-01-{config}.json").read_text(encoding="utf-8"))["operation"]["measured_cash_flow"]["first_delivery"]
                for config in ("RelWithDebInfo", "Debug")]
        restored = [row["observation"]["company"]["cash"] >= 100000 for row in result["configurations"]]
        assert paid[0] == paid[1] and restored[0] == restored[1]
        result.update(_documentation=__doc__, actual_first_delivery=paid[0], actual_reserve_restored=restored[0], module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
