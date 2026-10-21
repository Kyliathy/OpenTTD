"""Bind actual direct Goods orders/service and genuine feeder-origin rail final delivery.

@file surface_entry_factory_goods_work_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGU.
New pickup, preserved old-load delivery and actual new rail payment are separate
facts; none substitutes for the still-unrestored working/module capital.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_factory_goods_work_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain exact original order/work boundaries and ordinary physical/financial owners."""
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
        result = restore(args.configuration,ROOT / args.receipt,observe,"factory-goods-work-static",__doc__)
    else:
        versions = ROOT / "test-cache/surface-entry/salt-window-source-versions"
        result = bind(ROOT / "logs/surface-entry-factory-goods-verification.json",
            [versions / name / "retained.json" for name in ("before-factory-goods-work","before-factory-goods-order-observer")],
            ("factory-goods-orders","factory-goods-work-01"),"factory-goods-work-static",observe,[Path(__file__),CALLER],
            ROOT / "test-cache/surface-entry/factory-goods-work-products" if args.retain else None)
        flows, first_direct = [], []
        # Both original worlds must distinguish3's new pickup from25's old
        # loaded delivery. Native final rail quantity/counters corroborate
        # payment of the new feeder cohort during these same real ticks.
        for configuration in ("RelWithDebInfo","Debug"):
            value = json.loads((ROOT / f"logs/surface-entry-factory-goods-work-01-{configuration}.json").read_text(encoding="utf-8"))["operation"]
            flows.append(value["measured_cash_flow"])
            body3, = [r for r in value["rows"]["vehicles"] if r["vehicle"] == 3]
            first_direct.append(body3["delivered"])
            if value["measured_cash_flow"]["rail_delivered"] > 0:
                assert sum(r["amount"] for r in value["rows"]["delivered"] if r["label"] == "LVST") >= value["measured_cash_flow"]["rail_delivered"]
                earned, = value["rows"]["earned"]
                assert earned["income"] > 0
        assert flows[0] == flows[1] and first_direct[0] == first_direct[1]
        result.update(_documentation=__doc__, actual_first_direct_delivery=first_direct[0],
                      actual_feeder_rail_final_delivery=flows[0]["rail_delivered"] > 0,
                      actual_reserve_restored=all(r["observation"]["company"]["cash"] >= 100000 for r in result["configurations"]),
                      module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
