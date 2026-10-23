"""Complete actual module freight and verify an ordinary native resave.

@file orbital_freight_checks-2.py Whole direct unload and same original rocket recovery.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

shared = runpy.run_path(str(Path(__file__).with_name("orbital_freight_checks-1.py")))
native, EXACT = shared["native"], shared["EXACT"]


def main():
    """Consume one exact paid departure report and retain every recovery checkpoint identity."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--preparation", type=Path, required=True, help="Exact preceding orbital-module-flight-prepare.json")
    args = parser.parse_args()
    predecessor = args.preparation.resolve()
    assert predecessor.is_relative_to(ROOT)
    prepared = json.loads(predecessor.read_text(encoding="utf-8"))
    assert prepared["configuration"] == args.configuration
    source = ROOT / prepared["delivery"]["path"]
    assert identity(source) == prepared["delivery"]
    context = current_run_context("space", "module-flight-complete-verify")
    completion = context.for_feature("space", "module-flight-complete")
    complete = native(completion, args.configuration, "continue", source)
    version, expected = chunks(complete)
    assert version == 423
    ordinary_context = context.for_feature("space", "module-flight-ordinary")
    ordinary(ordinary_context, {"name": "module-flight", "snapshot": identity(complete)}, args.configuration)
    resaved = ordinary_context.saves / "ordinary-resave.sav"
    actual = chunks(resaved)[1]
    # Ordinary loading must preserve all original inventory, spent receipts, native fleet, money and utility authority without delivering again.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == prepared["delivery"]
    report = context.reports / "orbital-module-flight-complete.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group11; real whole module unload and original hull recovery.",
        "configuration": args.configuration, "save_version": version, "preparation": identity(predecessor), "source": identity(source),
        "complete": identity(complete), "ordinary_resave": identity(resaved), "exact_owner_families": len(EXACT),
        "approach": identity(completion.saves / "module-flight-approach-423.sav"),
        "unloaded": identity(completion.saves / "module-flight-unloaded-423.sav"),
        "turnaround": identity(completion.saves / "module-flight-turnaround-423.sav"),
        "whole_delivery": True, "same_original_rocket": True, "fuel_burned_once": 15, "no_second_recipe": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Completed module flight: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
