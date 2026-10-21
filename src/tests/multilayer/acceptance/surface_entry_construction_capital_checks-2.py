"""Restore the exact pure native capital proposal through the untouched ordinary game.

@file surface_entry_construction_capital_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEG.
This paused load/resave never repeats a command, grants materials or earns work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import restore

CALLER = Path(__file__).with_name("surface_entry_construction_capital_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore one exact original pure proposal without changing its ownership or money."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / f"logs/surface-entry-construction-capital-query-{args.configuration}.json"
    assert not json.loads(receipt.read_text(encoding="utf-8"))["operation"]["module_capital_complete"]
    result = restore(args.configuration, receipt, CURRENT["observe"], "construction-capital-static", __doc__)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
