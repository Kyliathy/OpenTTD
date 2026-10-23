"""Verify only the native finite/receiver/calendar producers required by the actual1985 Shield source.

@file shield_source_gate.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15EB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Each exact test case runs in its own native process; logical receiver names do not establish missing facilities.
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
from shield_source_checks import SOURCES

CASES = (("Every native mine produces exact finite output", "finite-producer"),
         ("Native Surface processors execute every frozen material alternative", "processor-producer"),
         ("Baseline receiver reuse validates actual cargo and production semantics", "receiver-boundary"),
         ("Native capsule startup honors all five exact launch dates", "launch-calendar"))


def main():
    """Run the exact source prerequisite cases separately against the current compiled native owner.

    @return None; failed/empty native cases or changed definitions cannot authorize source acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args(); context = current_run_context("tubes", "shield-source"); context.prepare_runtime()
    processes = []
    # A new process per exact case avoids the known combined-tag fixture
    # leakage while retaining all generated native resource/recipe meanings.
    for case, label in CASES:
        _, proof = native(context, args.config, case, label)
        processes.append(dict(case=case, **proof))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15EB; current native finite/source/receiver/calendar boundary gate; no source stock or useful route claim",
        "configuration": args.config, "focus": "producer-gate", "native": processes, "controller": identity(__file__),
        "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in (*SOURCES, "src/tests/layer_mine_production.cpp", "src/tests/layer_processor.cpp",
                                                       "src/tests/layer_baseline_receiver.cpp", "src/tests/transport_tube_calendar.cpp")],
    }
    path = context.output("reports", f"shield-producer-gate-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


if __name__ == "__main__":
    main()
