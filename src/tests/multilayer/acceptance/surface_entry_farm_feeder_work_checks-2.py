"""Bind actual short-feeder payments/work and exact ordinary native originals.

@file surface_entry_farm_feeder_work_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGQ.
Only literal native cargo/cash closes its respective gate. A picked-up batch
or virtual transfer credit cannot close the final-delivery or capital gates.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_farm_feeder_work_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def earlier_trade_rejects(path):
    """Check the actual earlier Source11-only trade contract remains strict.

    @param path Genuine checkpoint containing native Source5commercial Goods.
    @return Whether the preceding explicit farm-road observer rejects that new provenance.
    """
    try:
        CURRENT["BASE"]["observe"](path)
    except AssertionError:
        return True
    return False


def main():
    """Retain all ordered native boundaries and literal pickup/transfer/capital outcomes."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "farm-feeder-work-static", __doc__)
    else:
        versions = ROOT / "test-cache/surface-entry/salt-window-source-versions"
        result = bind(ROOT / "logs/surface-entry-farm-feeder-verification.json",
            [versions / name / "retained.json" for name in ("before-farm-feeder-work", "before-farm-feeder-order-observer", "before-farm-feeder-factory-goods")],
            ("farm-feeder-road", "farm-feeder-stops", "farm-feeder-orders", "farm-feeder-work-01"),
            "farm-feeder-work-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/farm-feeder-work-products" if args.retain else None)
        transferred, restored = [], []
        # Both original worlds exercise the narrow Factory5trade allowance;
        # the previous explicit Source11-only observer must still reject it.
        for row in result["configurations"]:
            assert earlier_trade_rejects(ROOT / row["output"]["path"])
            body = row["observation"]["farm_road"]
            assert body["vehicle"] == 62 and all(p["source_type"] == 0 and p["source_id"] == 7 for p in body["packets"])
            restored.append(row["observation"]["company"]["cash"] >= 100000)
            receipt = json.loads((ROOT / f"logs/surface-entry-farm-feeder-work-01-{row['configuration']}.json").read_text(encoding="utf-8"))
            transferred.append(receipt["operation"]["measured_cash_flow"]["first_transfer"])
        assert restored[0] == restored[1] and transferred[0] == transferred[1]
        result.update(_documentation=__doc__, actual_first_transfer=transferred[0], actual_reserve_restored=restored[0],
                      strict_factory_goods_default_rejected=True, feeder_credit_is_cash=False, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
