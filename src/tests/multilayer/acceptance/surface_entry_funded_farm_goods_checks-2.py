"""Restore the actual final funded farm Goods original through the ordinary game.

@file surface_entry_funded_farm_goods_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEE.
Paused load/resave grants no extra payment, vehicle, cargo or earned tick.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import restore

CALLER = Path(__file__).with_name("surface_entry_funded_farm_goods_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore exactly the actual finite scope's last original, without repeating transport."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--windows", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
    receipt = ROOT / f"logs/surface-entry-funded-farm-goods-work-{args.windows:03d}-{args.configuration}.json"
    assert json.loads(receipt.read_text(encoding="utf-8"))["operation"]["goods_scope"]["ended"]
    result = restore(args.configuration, receipt, CURRENT["observe"], "funded-farm-goods-static", __doc__)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
