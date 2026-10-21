"""Bind strict refusal, actual commercial Goods mix admission and unchanged ordinary originals.

@file surface_entry_factory_goods_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGT.
Both pure proposals retain their own native authorities. Neither stock nor a
shorter static road route supplies funds or predicts a future payment.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_factory_goods_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain both exact pure cases and literal complete later-producer funding results."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "factory-goods-static", __doc__)
    else:
        versions = ROOT / "test-cache/surface-entry/salt-window-source-versions"
        result = bind(ROOT / "logs/surface-entry-farm-feeder-rail-verification.json",
            [versions / name / "retained.json" for name in ("before-factory-goods", "before-factory-goods-mixed")],
            ("factory-goods-query", "factory-goods-mixed-query"), "factory-goods-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/factory-goods-products" if args.retain else None)
        admitted = []
        # Preserve the first genuine refusal instead of silently replacing
        # its pure Source5assumption with the later explicitly quoted5/11mix.
        for configuration in ("RelWithDebInfo", "Debug"):
            strict = json.loads((ROOT / f"logs/surface-entry-factory-goods-query-{configuration}.json").read_text(encoding="utf-8"))["operation"]
            mixed = json.loads((ROOT / f"logs/surface-entry-factory-goods-mixed-query-{configuration}.json").read_text(encoding="utf-8"))["operation"]
            assert not strict["factory_goods_admitted"] and strict["initial"] == strict["final"] == mixed["initial"] == mixed["final"]
            source, = mixed["rows"]["source"]
            assert source["factory5"] == 213 and source["factory11"] == 6 and source["physical"] == source["available"] == 219
            assert source["old_available"] == 0 and source["old_distance"] == 88 and source["distance"] == 82
            admitted.append(mixed["factory_goods_admitted"])
        assert admitted[0] == admitted[1]
        result.update(_documentation=__doc__, strict_source5_refused=True, factory_goods_admitted=admitted[0],
                      actual_reserve_restored=False, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
