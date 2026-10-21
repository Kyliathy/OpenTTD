"""Bind native paid farm balance and exact ordinary original reloads.

@file surface_entry_farm_rebalance_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGH.
Original32wagons remain owned; parked90Grain never counts as active capacity.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_farm_rebalance_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Publish retained actual balance/ordinary evidence and honest current reserve result."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "farm-rebalance-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-farm-fleet-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-farm-rebalance/retained.json"],
            ("farm-rebalance-recall", "farm-rebalance-return-01", "farm-rebalance-park", "farm-rebalance-pay", "farm-rebalance-service-01"),
            "farm-rebalance-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/farm-rebalance-products" if args.retain else None)
        restored = []
        # Both independently worked worlds must retain every old asset and
        # agree on actual reserve recovery, rather than projected capacity.
        for row in result["configurations"]:
            assert CURRENT["strict_default_rejects"](ROOT / row["output"]["path"])
            balance = row["observation"]["farm_balance"]
            assert balance["state"] == "balanced" and balance["original_wagons_owned"] == 32
            assert balance["grain_capacity"] == balance["parked_grain_capacity"] == 90
            restored.append(row["observation"]["company"]["cash"] >= 100000)
        assert restored[0] == restored[1]
        result.update(_documentation=__doc__, actual_reserve_restored=restored[0], strict_default_rejected=True, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
