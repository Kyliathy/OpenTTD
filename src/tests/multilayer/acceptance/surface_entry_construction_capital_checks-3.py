"""Bind pure genuine construction quotes and retained ordinary original owners.

@file surface_entry_construction_capital_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEG.
Native queries and ordinary restores are retained boundaries, never replayed here.
"""
import argparse
import json
from pathlib import Path
import runpy
import time

from space_preview_checks import ROOT
from surface_original_evidence import bind
from surface_evidence_identity_cache import IdentityCache

CALLER = Path(__file__).with_name("surface_entry_construction_capital_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
VERSIONS = CURRENT["VERSIONS"]


def main():
    """Bind two actual pure proposals and ordinary restores without claiming funded modules."""
    started = time.monotonic()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    cache = IdentityCache()
    result = bind(ROOT / "logs/surface-entry-working-cash-verification.json", VERSIONS,
        ("construction-capital-query",), "construction-capital-static", CURRENT["observe"],
        [Path(__file__), CALLER, CALLER.with_name("surface_entry_construction_capital_checks-2.py"),
         CALLER.with_name("surface_construction_capital_native.py")],
        ROOT / "test-cache/surface-entry/construction-capital-products" if args.retain else None, identity_reader=cache)
    cache.assert_unchanged()
    values = [json.loads((ROOT / f"logs/surface-entry-construction-capital-query-{configuration}.json").read_text(encoding="utf-8"))["operation"]
              for configuration in ("RelWithDebInfo", "Debug")]
    assert values[0]["rows"] == values[1]["rows"]
    result.update(_documentation=__doc__, actual_cash=101093, working_floor_policy=100000,
        capital_milestone=650000, incomplete_module_base=610000, priced_minimum=617500,
        priced_minimum_gap=516407, propellant_missing=7, full_logistics_priced=False, module_capital_complete=False,
        minimum_current_life=values[0]["minimum_current_life"],
        metadata_verification=dict(seconds=time.monotonic()-started, distinct_sha_reads=len(cache.entries),
            unchanged_reuses=cache.hits, every_authority_rechecked_at_close=True, game_boundaries_repeated=0))
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), "minimum_life", result["minimum_current_life"], result["metadata_verification"], flush=True)


if __name__ == "__main__":
    main()
