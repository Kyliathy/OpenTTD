"""Bind actual construction-cash recovery, full costs and ordinary saved-world convergence.

@file surface_entry_construction_cash_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEH.
The capital milestone threshold is restored only by actual banked native money.
"""
import argparse
import json
from pathlib import Path
import runpy
import time

from space_preview_checks import ROOT
from surface_original_evidence import bind
from surface_evidence_identity_cache import IdentityCache

CALLER = Path(__file__).with_name("surface_entry_construction_cash_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
VERSIONS = CURRENT["VERSIONS"]


def main():
    """Bind consecutive actual service intervals and verify the real capital milestone result."""
    started = time.monotonic()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--windows", type=int, required=True)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
    names = ("construction-cash-query",) + tuple(f"construction-cash-work-{window:03d}" for window in range(1, args.windows+1))
    finals, flows = [], []
    # A paid event alone cannot close this scope. Every actual interval and
    # its first real cash/refusal/cap stop must occur in strict source order.
    for configuration in ("RelWithDebInfo", "Debug"):
        measured = dict(ticks=0, actual_fares=0, interest=0, other_operating_cost=0, cash_delta=0)
        for name in names:
            value = json.loads((ROOT / f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"]
            assert value["capital_scope"]["ended"] == (name == names[-1])
            for key in measured:
                measured[key] += value["measured_cash_flow"][key]
        finals.append(value)
        flows.append(measured)
    assert flows[0] == flows[1] and finals[0]["capital_scope"] == finals[1]["capital_scope"]
    cache = IdentityCache()
    result = bind(ROOT / "logs/surface-entry-construction-capital-verification.json", VERSIONS, names,
        "construction-cash-static", CURRENT["observe"],
        [Path(__file__), CALLER, CALLER.with_name("surface_entry_construction_cash_checks-2.py"),
         CALLER.with_name("surface_construction_cash_native.py")],
        ROOT / "test-cache/surface-entry/construction-cash-products" if args.retain else None, identity_reader=cache)
    cache.assert_unchanged()
    cash = finals[0]["final"]["company"]["cash"]
    assert cash == finals[1]["final"]["company"]["cash"] and cash-101093 == flows[0]["cash_delta"]
    assert flows[0]["actual_fares"]-flows[0]["interest"]-flows[0]["other_operating_cost"] == cash-101093
    restored = cash >= 650000
    assert restored == finals[0]["capital_milestone_reached"] == finals[1]["capital_milestone_reached"]
    result.update(_documentation=__doc__, actual_cash=cash, startup_reserve=40000, capital_scope=finals[0]["capital_scope"],
        measured_cash_flow=flows[0], capital_milestone_reached=restored, working_floor_policy=100000, capital_milestone=650000,
        capital_milestone_gap=max(0, 650000-cash), priced_minimum=617500, priced_minimum_gap=max(0,617500-cash), incomplete_module_base=610000, incomplete_module_base_gap=max(0, 610000-cash),
        module_capital_complete=False, future_builder=dict(capital=100000, steel=4, goods=8, work=148,
            launch_service_priced=False, funded=False),
        metadata_verification=dict(seconds=time.monotonic()-started, distinct_sha_reads=len(cache.entries),
            unchanged_reuses=cache.hits, every_authority_rechecked_at_close=True, game_boundaries_repeated=0))
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), result["capital_scope"], "cash", cash,
        "capital_milestone_reached", restored, result["metadata_verification"], flush=True)


if __name__ == "__main__":
    main()
