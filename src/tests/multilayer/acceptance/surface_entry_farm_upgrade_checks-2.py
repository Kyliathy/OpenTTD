"""Bind the real single-wagon farm upgrade and exact ordinary original reloads.

@file surface_entry_farm_upgrade_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGF.
Only the actual one native tail joins the existing original Farm service.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_farm_upgrade_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Publish exact upgrade/ordinary evidence and the truthful current reserve result."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "farm-upgrade-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-farm-capacity-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-farm-upgrade/retained.json"],
            ("farm-upgrade-recall", "farm-upgrade-return-01", "farm-upgrade-pay", "farm-upgrade-service-01"), "farm-upgrade-static", observe,
            [Path(__file__), CALLER], ROOT / "test-cache/surface-entry/farm-upgrade-products" if args.retain else None)
        restored = []
        for row in result["configurations"]:
            assert CURRENT["strict_default_rejects"](ROOT / row["output"]["path"])
            assert row["observation"]["farm_capacity"]["added"] == 1
            restored.append(row["observation"]["company"]["cash"] >= 100000)
        assert restored[0] == restored[1]
        result.update(_documentation=__doc__, actual_reserve_restored=restored[0], strict_default_rejected=True, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
