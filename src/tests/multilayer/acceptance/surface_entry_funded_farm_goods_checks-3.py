"""Bind actual first Goods52payment, full costs and original current/retired material owners.

@file surface_entry_funded_farm_goods_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEE.
Only consecutive exact native originals and their existing ordinary copies count.
"""
import argparse
import json
from pathlib import Path
import runpy
import time

from space_preview_checks import ROOT
from surface_original_evidence import bind
from surface_evidence_identity_cache import IdentityCache

CALLER = Path(__file__).with_name("surface_entry_funded_farm_goods_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
VERSIONS = CURRENT["VERSIONS"]


def main():
    """Bind the finite actual post-retirement service and both exact ordinary reloads."""
    started = time.monotonic()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--windows", type=int, required=True)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
    names = ("funded-farm-goods-query",) + tuple(f"funded-farm-goods-work-{window:03d}" for window in range(1, args.windows+1))
    finals, flows = [], []
    # A later original may not extend the first genuine posted payment,
    # working goal, funding refusal or cap. Every exact interval is accounted.
    for configuration in ("RelWithDebInfo", "Debug"):
        measured = dict(ticks=0, actual_fares=0, interest=0, other_operating_cost=0, cash_delta=0)
        for name in names:
            value = json.loads((ROOT / f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"]
            assert value["goods_scope"]["ended"] == (name == names[-1])
            for key in measured:
                measured[key] += value["measured_cash_flow"][key]
        finals.append(value)
        flows.append(measured)
    assert flows[0] == flows[1] and finals[0]["goods_scope"] == finals[1]["goods_scope"]
    assert finals[0]["actual_goods"] == finals[1]["actual_goods"]
    cache = IdentityCache()
    result = bind(ROOT / "logs/surface-entry-constructor-retirement-verification.json", VERSIONS, names,
        "funded-farm-goods-static", CURRENT["observe"],
        [Path(__file__), CALLER, CALLER.with_name("surface_entry_funded_farm_goods_checks-2.py"),
         CALLER.with_name("surface_funded_farm_goods_native.py")],
        ROOT / "test-cache/surface-entry/funded-farm-goods-products" if args.retain else None, identity_reader=cache)
    cache.assert_unchanged()
    cash = finals[0]["final"]["company"]["cash"]
    assert cash == finals[1]["final"]["company"]["cash"] and cash-71344 == flows[0]["cash_delta"]
    assert flows[0]["actual_fares"]-flows[0]["interest"]-flows[0]["other_operating_cost"] == cash-71344
    result.update(_documentation=__doc__, actual_cash=cash, startup_reserve=40000, goods_scope=finals[0]["goods_scope"],
        measured_cash_flow=flows[0], cumulative_boarded=finals[0]["cumulative_boarded"], cumulative_accepted=finals[0]["cumulative_accepted"],
        payment_posted=finals[0]["payment_posted"], actual_goods=finals[0]["actual_goods"], working_gap=max(0, 100000-cash),
        incomplete_module_base=610000, incomplete_module_base_gap=max(0, 610000-cash), module_capital_complete=False,
        future_builder=dict(capital=100000, steel=4, goods=8, work=148, launch_service_priced=False, funded=False),
        metadata_verification=dict(seconds=time.monotonic()-started, distinct_sha_reads=len(cache.entries),
            unchanged_reuses=cache.hits, every_authority_rechecked_at_close=True, game_boundaries_repeated=0))
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), result["goods_scope"], result["metadata_verification"], flush=True)


if __name__ == "__main__":
    main()
