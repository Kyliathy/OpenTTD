"""Bind actual retirement capital and permanently consumed embodied inputs.

@file surface_entry_constructor_retirement_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVED.
Exact current and retained authorities are SHA-read with unchanged-file reuse;
native sale/ordinary reloads already exist and are never repeated by this bind.
"""
import argparse
import json
from pathlib import Path
import runpy
import time

from space_preview_checks import ROOT
from surface_original_evidence import bind
from surface_evidence_identity_cache import IdentityCache

CALLER = Path(__file__).with_name("surface_entry_constructor_retirement_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
VERSIONS = CURRENT["VERSIONS"]


def main():
    """Bind two actual retirement originals, their ordinary copies and exact unchanged physical materials."""
    started = time.monotonic()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    cache = IdentityCache()
    result = bind(ROOT / "logs/surface-entry-constructor-home-verification.json", VERSIONS,
        ("constructor-retirement-pay",), "constructor-retirement-static", CURRENT["observe"],
        [Path(__file__), CALLER, CALLER.with_name("surface_entry_constructor_retirement_checks-2.py"),
         CALLER.with_name("surface_constructor_retirement_native.py")],
        ROOT / "test-cache/surface-entry/constructor-retirement-products" if args.retain else None, identity_reader=cache)
    cache.assert_unchanged()
    # Both exact native originals separately bind the same retired generation,
    # actual money and terminal embodied consumption. Stock is never refunded.
    for configuration in ("RelWithDebInfo", "Debug"):
        value = json.loads((ROOT / f"logs/surface-entry-constructor-retirement-pay-{configuration}.json").read_text(encoding="utf-8"))["operation"]
        assert value["historical_present_rejected"] and value["actual_cash"] == 71344 and value["refund_banked"] == 29927
        assert value["final"]["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
        assert value["earned_ticks"] == value["avoided_cost_realized"] == 0
    result.update(_documentation=__doc__, actual_cash=71344, startup_reserve=40000, refund_banked=29927,
        actual_retirement_complete=True, avoided_cost_realized=0, historical_present_rejected=True,
        retired_embodied_inputs=dict(STEL=4, GOOD=8, PROP=0), current_embodied_inputs=dict(STEL=12, GOOD=16, PROP=0),
        components_recovered=0, unused_returns_extinguished=2, working_gap=28656,
        incomplete_module_base=610000, incomplete_module_base_gap=538656, module_capital_complete=False,
        future_builder=dict(capital=100000, steel=4, goods=8, work=148, launch_service_priced=False, funded=False),
        metadata_verification=dict(seconds=time.monotonic()-started, distinct_sha_reads=len(cache.entries),
            unchanged_reuses=cache.hits, every_authority_rechecked_at_close=True, game_boundaries_repeated=0))
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), result["metadata_verification"], flush=True)


if __name__ == "__main__":
    main()
