"""Bind unchanged productive freight admission and ordinary original reloads.

@file surface_entry_productive_capital_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGC.
Actual first-delivery and working-reserve goals remain explicitly open.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_productive_capital_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Publish ordinary reload or exact full remaining-work funding acceptance."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "productive-capital-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-goods-trade-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-productive-query/retained.json"],
            ("productive-capital-query",), "productive-capital-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/productive-capital-products" if args.retain else None)
        for row in result["configurations"]:
            receipt = json.loads((ROOT / row["receipt"]["path"]).read_text(encoding="utf-8"))["operation"]
            assert receipt["current_producers"] == CURRENT["current_producers"](ROOT / row["output"]["path"])
        result.update(_documentation=__doc__, actual_reserve_restored=False, module_capital_complete=False, purchases=0)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
