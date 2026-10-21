"""Restore the exact final actual construction-cash original through the ordinary game.

@file surface_entry_construction_cash_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEH.
Paused native load/resave repeats no fare, cargo transfer or earned tick.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import restore

CALLER = Path(__file__).with_name("surface_entry_construction_cash_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore only the actual finite cash scope's final original endpoint."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--windows", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
    receipt = ROOT / f"logs/surface-entry-construction-cash-work-{args.windows:03d}-{args.configuration}.json"
    assert json.loads(receipt.read_text(encoding="utf-8"))["operation"]["capital_scope"]["ended"]
    result = restore(args.configuration, receipt, CURRENT["observe"], "construction-cash-static", __doc__)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
