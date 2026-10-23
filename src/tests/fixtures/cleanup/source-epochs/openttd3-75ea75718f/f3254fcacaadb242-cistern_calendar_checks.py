"""Verify exact native capsule startup/saved launch dates and conventional engine isolation.

@file cistern_calendar_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DCBA.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every tag runs in an isolated native process; definition publication creates no world cargo, route or useful delivery.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native

SOURCES = ("src/engine.cpp", "src/transport_tube.h", "src/transport_tube_engine.cpp", "src/saveload/engine_sl.cpp",
           "src/tests/transport_tube_calendar.cpp", "src/tests/underground_engine.cpp", "src/tests/transport_tube_fixture.h")


def main():
    """Run each focused native date/isolation gate separately and retain exact current source/binary evidence.

    @return None; any early unlock, delayed canonical publication or changed conventional owner prevents the report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args(); context = current_run_context("tubes", "cistern-calendar")
    proofs = []
    # Isolated tags prevent fixture state from another engine family from
    # supplying apparent availability. Each model's before/on-date generation
    # remains a single bounded native calendar-owner case.
    for tag in ("transport-tube-calendar-startup", "transport-tube-calendar-monthly", "transport-tube-calendar-restore", "transport-tube-calendar-isolation", "underground-engine-startup"):
        output, proof = native(context, args.config, f"[{tag}]", tag); proofs.append(proof)
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DCBA; all-five canonical native startup/saved launch dates and conventional isolation",
        "configuration": args.config, "focus": "capsule-calendar", "native": proofs,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"cistern-calendar-{args.config}.json"); path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
