"""Bind actual mixed farm backhaul, whole cash flow and ordinary native reloads.

@file surface_entry_farm_backhaul_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVD.
Only exact consecutive original paid/work boundaries can complete this scope.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_farm_backhaul_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore one real final original or bind a closed finite actual mixed-service sequence."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--windows", type=int, required=True)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
    names = ("farm-backhaul-query", "farm-backhaul-pay") + tuple(
        f"farm-backhaul-work-{window:03d}" for window in range(1, args.windows+1))
    if args.phase == "restore":
        assert args.configuration
        receipt = ROOT / f"logs/surface-entry-{names[-1]}-{args.configuration}.json"
        assert json.loads(receipt.read_text(encoding="utf-8"))["operation"]["backhaul_scope"]["ended"]
        result = restore(args.configuration, receipt, CURRENT["observe"], "farm-backhaul-static", __doc__)
    else:
        finals, flows = [], []
        # Each configuration must end at its first actual gate. Later windows
        # cannot extend a closed scope or contribute fictional unpaid income.
        for configuration in ("RelWithDebInfo", "Debug"):
            measured = dict(ticks=0, actual_fares=0, interest=0, other_operating_cost=0, gross_purchases=0, cash_delta=0)
            for name in names:
                receipt = ROOT / f"logs/surface-entry-{name}-{configuration}.json"
                value = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
                assert value["backhaul_scope"]["ended"] == (name == names[-1])
                for key in measured:
                    measured[key] += value["measured_cash_flow"][key]
            paid = json.loads((ROOT / f"logs/surface-entry-farm-backhaul-pay-{configuration}.json").read_text(encoding="utf-8"))["operation"]
            assert paid["old_default_rejected"] and len(paid["actual_paid_goods_ids"]) == 2
            finals.append(value)
            flows.append(measured)
        assert flows[0] == flows[1] and finals[0]["backhaul_scope"] == finals[1]["backhaul_scope"]
        assert finals[0]["actual_goods"] == finals[1]["actual_goods"]
        result = bind(ROOT / "logs/surface-entry-farm-two-return-verification.json", CURRENT["VERSIONS"],
            names, "farm-backhaul-static", CURRENT["observe"],
            [Path(__file__), CALLER, CALLER.with_name("surface_farm_backhaul_native.py"),
             CALLER.with_name("surface_gateway_construction_ledger.py"),
             CALLER.with_name("surface_gateway_resupply_ledger.py"),
             CALLER.with_name("surface_gateway_material_ledger.py"),
             ROOT / "logs/surface-entry-farm-backhaul-failed-query-RelWithDebInfo.json",
             ROOT / "logs/surface-entry-farm-backhaul-query-correction-versions.json"],
            ROOT / "test-cache/surface-entry/farm-backhaul-products" if args.retain else None)
        cash = finals[0]["final"]["company"]["cash"]
        assert cash == finals[1]["final"]["company"]["cash"]
        assert cash-43368 == flows[0]["cash_delta"]
        assert flows[0]["actual_fares"]-flows[0]["interest"]-flows[0]["other_operating_cost"]-2226 == cash-43368
        result.update(_documentation=__doc__, backhaul_scope=finals[0]["backhaul_scope"],
            startup_reserve=40000, final_cash=cash, measured_cash_flow=flows[0],
            cumulative_boarded=finals[0]["cumulative_boarded"], cumulative_accepted=finals[0]["cumulative_accepted"],
            original_wagons_owned=32, active_capacity=dict(livestock=200, grain=90, goods=50),
            old_default_rejected=True, failed_pure_query=identity(ROOT / "logs/surface-entry-farm-backhaul-failed-query-RelWithDebInfo.json"),
            working_gap=max(0, 100000-cash), incomplete_module_base_gap=max(0, 510000-cash),
            module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
