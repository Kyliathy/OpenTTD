"""Prepare one actual target-removal/reuse phase from the genuine418 paid world.

@file orbital_freight_checks-5.py Empty paid Storage retirement preserves the original loaded rocket and issued old destination.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11C.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

shared = runpy.run_path(str(Path(__file__).with_name("orbital_freight_checks-4.py")))
native, EXACT = shared["native"], shared["EXACT"]


def main():
    """Activate actual paid Storage, depart its builders and retire/reuse only its original empty native owner."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", required=True, choices=("preparing", "ascent", "delivery", "approach"))
    args = parser.parse_args()
    source = ROOT / "src/tests/fixtures/demos/space/space-module-frame-418.sav"
    original = identity(source)
    assert original["sha256"] == "0f10c5e085846c1ee2fca8c3d8945296e3b5366243ddaadff6d09e9250e00dca"
    context = current_run_context("space", "module-lost-" + args.phase + "-verify")
    lost = native(context.for_feature("space", "module-lost-" + args.phase), args.configuration, "lost-" + args.phase, source)
    paused = context.for_feature("space", "module-lost-paused-" + args.phase)
    ordinary(paused, {"name": "module-target-lost", "snapshot": identity(lost)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(lost)[1], chunks(resaved)[1]
    # The paused missing incarnation must remain explicitly lost with all original payload aboard, even after its numeric ID is reused by a paid inert blueprint.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == original
    report = context.reports / "orbital-module-target-lost.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group11C; real empty Storage retirement/reuse and retained flight payload.",
        "configuration": args.configuration, "phase": args.phase, "source": original, "lost": identity(lost),
        "ordinary_resave": identity(resaved), "exact_owner_families": len(EXACT), "original_payload_retained": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Module target lost/{args.phase}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
