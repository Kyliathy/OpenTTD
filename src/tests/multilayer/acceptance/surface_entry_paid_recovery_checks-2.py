"""Bind only actually executed conditional recovery windows and current ordinary worlds.

@file surface_entry_paid_recovery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGJ.
Skipped intervals supply neither progress nor income. Actual current cash alone
can close the working-reserve gate; remaining module finance stays separate.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_paid_recovery_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Publish exact conditional recovery/ordinary evidence and truthful working cash."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "paid-recovery-static", __doc__)
    else:
        names = []
        # Bind only actual ordered windows/intervals. Both configurations must
        # have exactly the same accepted boundary sequence, including early stop.
        for window in range(1, 4):
            for stage in ("query", "short-query", "work-01", "work-02"):
                name = f"paid-recovery-{window:02}-{stage}"
                present = [(ROOT / f"logs/surface-entry-{name}-{config}.json").exists() for config in ("RelWithDebInfo", "Debug")]
                assert present[0] == present[1]
                if not present[0]:
                    if stage == "short-query":
                        continue
                    break
                names.append(name)
        assert names
        result = bind(ROOT / "logs/surface-entry-farm-cycle-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions" / name / "retained.json"
             for name in ("before-paid-recovery", "before-short-paid-recovery")],
            tuple(names), "paid-recovery-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/paid-recovery-products" if args.retain else None)
        restored = [row["observation"]["company"]["cash"] >= 100000 for row in result["configurations"]]
        assert restored[0] == restored[1]
        result.update(_documentation=__doc__, actual_reserve_restored=restored[0], actual_stages=names, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
