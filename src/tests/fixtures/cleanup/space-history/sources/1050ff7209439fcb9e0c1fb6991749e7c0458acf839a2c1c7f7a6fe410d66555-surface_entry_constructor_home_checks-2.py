"""Bind actual original builder home navigation and ordinary saved-world reloads.

@file surface_entry_constructor_home_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEC.
An actual home arrival does not bank resale value or remove the builder's upkeep.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_constructor_home_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore a real final original or bind exactly its finite ordered home-work sequence."""
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
    names = ("constructor-home-order",) + tuple(f"constructor-home-work-{window:03d}" for window in range(1, args.windows+1))
    if args.phase == "restore":
        assert args.configuration
        receipt = ROOT / f"logs/surface-entry-{names[-1]}-{args.configuration}.json"
        assert json.loads(receipt.read_text(encoding="utf-8"))["operation"]["home_scope"]["ended"]
        result = restore(args.configuration, receipt, CURRENT["observe"], "constructor-home-static", __doc__)
    else:
        finals, flows = [], []
        # Every original interval must follow its actual predecessor, stopping
        # at the first actual home/refusal/cap rather than repeating old work.
        for configuration in ("RelWithDebInfo", "Debug"):
            measured = dict(ticks=0, actual_fares=0, interest=0, other_operating_cost=0, cash_delta=0)
            for name in names:
                value = json.loads((ROOT / f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"]
                assert value["home_scope"]["ended"] == (name == names[-1])
                assert value["old_queue_default_rejected"] and value["builder_refund_banked"] == value["avoided_cost_realized"] == 0
                for key in measured:
                    measured[key] += value["measured_cash_flow"][key]
            finals.append(value)
            flows.append(measured)
        assert flows[0] == flows[1] and finals[0]["home_scope"] == finals[1]["home_scope"]
        result = bind(ROOT / "logs/surface-entry-surplus-recovery-verification.json", CURRENT["VERSIONS"], names,
            "constructor-home-static", CURRENT["observe"],
            [Path(__file__), CALLER, CALLER.with_name("surface_constructor_home_native.py")],
            ROOT / "test-cache/surface-entry/constructor-home-products" if args.retain else None)
        cash = finals[0]["final"]["company"]["cash"]
        assert cash == finals[1]["final"]["company"]["cash"] and cash-41666 == flows[0]["cash_delta"]
        assert flows[0]["actual_fares"]-flows[0]["interest"]-flows[0]["other_operating_cost"] == cash-41666
        result.update(_documentation=__doc__, actual_cash=cash, startup_reserve=40000, home_scope=finals[0]["home_scope"],
            measured_cash_flow=flows[0], arrived_stopped=finals[0]["arrived_stopped"], old_queue_default_rejected=True,
            builder_refund_banked=0, avoided_cost_realized=0, builder_idle_per_day=100,
            unbanked_sale=finals[0]["rows"]["sale"], working_gap=max(0, 100000-cash),
            incomplete_module_base_gap=max(0, 510000-cash), module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
