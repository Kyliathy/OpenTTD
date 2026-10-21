"""Restore actual original builder home navigation through the ordinary application.

@file surface_entry_constructor_home_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEC.
An actual home arrival does not bank resale value or remove the builder's upkeep.
The separately bounded metadata bind lives in surface_entry_constructor_home_checks-3.py.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import restore

CALLER = Path(__file__).with_name("surface_entry_constructor_home_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore a real final original without repeating any native command or earned tick."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore",), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--windows", type=int, required=True)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
    names = ("constructor-home-order",) + tuple(f"constructor-home-work-{window:03d}" for window in range(1, args.windows+1))
    assert args.configuration
    receipt = ROOT / f"logs/surface-entry-{names[-1]}-{args.configuration}.json"
    assert json.loads(receipt.read_text(encoding="utf-8"))["operation"]["home_scope"]["ended"]
    result = restore(args.configuration, receipt, CURRENT["observe"], "constructor-home-static", __doc__)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
