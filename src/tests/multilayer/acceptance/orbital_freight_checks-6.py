"""Recover one saved missing module destination explicitly, with the original payload aboard.

@file orbital_freight_checks-6.py Once-only cancelled native recovery and exact paused resave.
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
    """Consume the exact saved old incarnation and return the same loaded rocket without another burn or replacement delivery."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--loss", type=Path, required=True, help="Exact preceding orbital-module-target-lost.json")
    args = parser.parse_args()
    predecessor = args.loss.resolve()
    assert predecessor.is_relative_to(ROOT)
    previous = json.loads(predecessor.read_text(encoding="utf-8"))
    assert previous["configuration"] == args.configuration
    source = ROOT / previous["lost"]["path"]
    assert identity(source) == previous["lost"]
    context = current_run_context("space", "module-lost-recovery-" + previous["phase"] + "-verify")
    complete = native(context.for_feature("space", "module-lost-recovery-" + previous["phase"]), args.configuration, "lost-continue", source)
    paused = context.for_feature("space", "module-lost-recovered-ordinary-" + previous["phase"])
    ordinary(paused, {"name": "module-cancelled-recovery", "snapshot": identity(complete)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(complete)[1], chunks(resaved)[1]
    # Recovery retains every original freight/part packet and the retired/reused target's distinct paid history; no repeated callback grants a second trip.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == previous["lost"]
    report = context.reports / "orbital-module-lost-recovery.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group11C; original loaded hull's explicit cancelled recovery.",
        "configuration": args.configuration, "phase": previous["phase"], "loss": identity(predecessor), "source": identity(source),
        "complete": identity(complete), "ordinary_resave": identity(resaved), "exact_owner_families": len(EXACT),
        "original_payload_retained": True, "no_second_burn": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Recovered lost module flight/{previous['phase']}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
