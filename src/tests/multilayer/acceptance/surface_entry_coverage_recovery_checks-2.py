"""Bind actual new-catchment receipts and physical freight without future fares.

@file surface_entry_coverage_recovery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSDF.
Existing paid station, vehicles and cargo remain the only native producers.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

DIRECTORY = Path(__file__).resolve().parent
CALLER = DIRECTORY / "surface_entry_passenger_coverage_checks-1.py"
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]
FREIGHT = runpy.run_path(str(DIRECTORY / "surface_entry_capital_recovery_checks-1.py"))


def main():
    """Publish exact actual capital recovery, ordinary reloads and original loaded-freight admission."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--windows", type=int, choices=(1, 2), default=1)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration, ROOT / args.receipt, observe, "coverage-recovery-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-passenger-coverage-verification.json", [],
            tuple(f"coverage-recovery-work-{index:02d}" for index in range(1, args.windows + 1)), "coverage-recovery-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/coverage-recovery-products" if args.retain else None)
        restored = []
        # Decode the original loaded bodies independently. Current cargo can
        # justify a bounded trial but never contributes to its starting cash.
        for row in result["configurations"]:
            operation = json.loads((ROOT / row["receipt"]["path"]).read_text(encoding="utf-8"))["operation"]
            first = json.loads((ROOT / f"logs/surface-entry-coverage-recovery-work-01-{row['configuration']}.json").read_text(encoding="utf-8"))["operation"]
            before = ROOT / first["before"]["path"]
            freight = FREIGHT["saved_consists"](before)
            assert freight[42, "COAL"] == (240, 116) and freight[52, "LVST"] == (150, 150) and freight[52, "GRAI"] == (180, 63)
            assert first["initial"]["company"]["cash"] == 84525
            assert CURRENT["BASE"]["passenger_state"](before)["stops"][2]["waiting"] == 75
            restored.append(row["observation"]["company"]["cash"] >= 100000)
        assert restored[0] == restored[1]
        result.update(_documentation=__doc__, actual_reserve_restored=restored[0], module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
