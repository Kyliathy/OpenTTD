"""Run isolated native tube refusal boundaries without claiming useful-route acceptance.

@file tube_refusal_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15A.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared floor/content/stock inputs are explicit adversarial boundary fixtures.
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

TAGS = {
    "construction": "[transport-tube-construction]",
    "conversion": "[transport-tube-conversion]",
    "domain": "[transport-tube-prototype]",
    "purchase": "[transport-tube-purchase]",
    "dates": "[transport-tube-admission-date]",
    "power": "[transport-tube-power]",
    "meter": "[transport-tube-meter]",
    "cross-layer": "[tube-refusal-cross-layer]",
    "portal-freight": "[portal-movement]",
    "portal-date": "[portal-fleet-date]",
    "tbm-travel": "[tbm-movement]",
}
SOURCES = (
    "src/transport_tube.h", "src/transport_tube_engine.cpp", "src/transport_tube_cmd.cpp",
    "src/transport_tube_power.cpp", "src/transport_tube_runtime.cpp", "src/transport_tube_topology.cpp",
    "src/train_cmd.cpp", "src/rail_cmd.cpp", "src/layer_portal_cmd.cpp", "src/pathfinder/follow_track.hpp", "src/utility_power.cpp",
    "src/tests/transport_tube_construction.cpp", "src/tests/transport_tube_prototype.cpp",
    "src/tests/transport_tube_admission.cpp", "src/tests/transport_tube_power.cpp", "src/tests/transport_tube_meter.cpp",
    "src/tests/transport_tube_refusal.cpp",
    "src/tests/portal_movement.cpp", "src/tests/tbm_movement.cpp",
    "src/tests/transport_tube_fixture.h", "src/tests/CMakeLists.txt",
)


def main():
    """Run one actual native refusal tag and retain its exact current owners and process evidence.

    @return None; failed or empty selection leaves the focused acceptance report absent.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=TAGS, required=True)
    args = parser.parse_args()
    context = current_run_context("tubes", "refusal-boundaries")
    context.prepare_runtime()
    _, proof = native(context, args.config, TAGS[args.focus], args.focus)
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15A; actual command/native domain/refusal evidence; explicit prepared inputs do not establish useful-route/bootstrap acceptance",
        "configuration": args.config, "focus": args.focus, "native": proof,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"tube-refusal-{args.config}-{args.focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
