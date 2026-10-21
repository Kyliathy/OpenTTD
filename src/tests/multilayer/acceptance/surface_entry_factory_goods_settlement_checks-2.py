"""Bind actual posted Goods payment and exact ordinary native originals.

@file surface_entry_factory_goods_settlement_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGVB.
Only the native payment lifecycle posts the real fare; pending amounts and
virtual shares never enter company cash through this observer.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore
from surface_regional_ledger import TABLE

CALLER = Path(__file__).with_name("surface_entry_factory_goods_settlement_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain original settlement/ordinary evidence and literal posted-payment/capital results."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase",choices=("restore","bind"),required=True)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"))
    parser.add_argument("--receipt",type=Path)
    parser.add_argument("--retain",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration,ROOT / args.receipt,observe,"factory-goods-settlement-static",__doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-factory-goods-follow-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-factory-goods-settlement/retained.json"],
            ("factory-goods-settlement",),"factory-goods-settlement-static",observe,[Path(__file__),CALLER],
            ROOT / "test-cache/surface-entry/factory-goods-settlement-products" if args.retain else None)
        posted = []
        # Both original worlds must lose only the settled native owner and
        # retain the literal corresponding company cash/fare/profit evidence.
        for configuration in ("RelWithDebInfo","Debug"):
            value = json.loads((ROOT / f"logs/surface-entry-factory-goods-settlement-{configuration}.json").read_text(encoding="utf-8"))["operation"]
            flow = value["measured_cash_flow"]
            assert flow["actual_fares"] >= 572 and not flow["payment_granted"]
            assert not any(r["front"] == 4 for r in TABLE(ROOT / value["output"]["path"],b"CAPY").values())
            posted.append(flow["actual_first_direct_payment"])
        assert posted[0] == posted[1]
        result.update(_documentation=__doc__,actual_first_direct_payment=posted[0],actual_reserve_restored=False,module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
