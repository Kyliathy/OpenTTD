"""Bind actual direct shipment acceptance separately from pending native cash settlement.

@file surface_entry_factory_goods_follow_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGV.
The receiver has accepted the shipment, but CAPYstill owns its572payment.
Neither empty cargo nor accepted-cargo statistics count as spendable cash.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore
from surface_regional_ledger import TABLE

CALLER = Path(__file__).with_name("surface_entry_factory_goods_work_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def pending_payment(path):
    """Inspect the actual native payment owner for accepted Goods from vehicle3.

    @param path Genuine original checkpoint or ordinary resave after acceptance.
    @return Exact native payment record, whose one-based front4identifies actual3.
    """
    payment, = [row for row in TABLE(path,b"CAPY").values() if row["front"] == 4]
    assert payment["route_profit"] == payment["visual_profit"] == 572 and payment["visual_transfer"] == 0
    return payment


def main():
    """Retain exact shipment/ordinary evidence without promoting pending payment into cash."""
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
        result = restore(args.configuration,ROOT / args.receipt,observe,"factory-goods-follow-static",__doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-factory-goods-work-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-factory-goods-follow-caller/retained.json"],
            ("factory-goods-follow-01",),"factory-goods-follow-static",observe,[Path(__file__),CALLER],
            ROOT / "test-cache/surface-entry/factory-goods-follow-products" if args.retain else None)
        pending = []
        # Actual vehicle3emptying at20proves acceptance. CAPYand unchanged
        # vehicle profit prove its final cash payment has not yet settled.
        for configuration in ("RelWithDebInfo","Debug"):
            value = json.loads((ROOT / f"logs/surface-entry-factory-goods-follow-01-{configuration}.json").read_text(encoding="utf-8"))["operation"]
            body3, = [r for r in value["rows"]["vehicles"] if r["vehicle"] == 3]
            assert body3["delivered"] and body3["held"] == 0 and body3["profit_delta_256"] < 0
            pending.append(pending_payment(ROOT / value["output"]["path"]))
        assert pending[0] == pending[1]
        result.update(_documentation=__doc__, actual_first_direct_acceptance=True, actual_first_direct_payment=False,
                      pending_payment=pending[0], actual_reserve_restored=False, module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
