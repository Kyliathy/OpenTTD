"""Observe delivered native capsule purchase, refit, orders and service controls.

@file tube_native_control_return.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md8C.
@see docs/zz-reach2/architecture/gui/archi-gui.md.
Original useful intermodal packets remain in the completed feature-owner proof.
"""

import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
from construction_provider_return import current_run_context, identity, native

ENTRY_CASES = {"purchase": "[transport-tube-admission-ui]",
               "refit": "Capsule loaded refit refuses packet loss before changing native cargo or RNG",
               "renewal": "Capsule clone and renewal preserve cargo and paid service on admission refusal",
               "service": "Tube actual depot service requires paid native work and preserves manual stop",
               "service-recovery": "Tube depot power restoration services the actual capsule without starting it",
               "orders": "[transport-tube-ordered-freight]", "domain": "[tube-refusal-cross-layer]",
               "terminal": "Tube public facility view retains native identity and exact capsule cargo admission"}
INPUT_CASES = {"purchase-input": "[tube-native-purchase-input]", "native-ui": "[tube-native-control-input]", "service-input": "[tube-native-service-input]"}
CASES = {**ENTRY_CASES, **INPUT_CASES}
SOURCES = ("src/vehicle_gui.cpp", "src/depot_gui.cpp", "src/build_vehicle_gui.cpp", "src/vehicle_cmd.cpp", "src/vehicle.cpp", "src/engine.cpp",
           "src/order_cmd.cpp", "src/transport_tube.h", "src/transport_tube_gui.cpp", "src/transport_tube_engine.cpp",
           "src/tests/transport_tube_controls.cpp", "src/tests/transport_tube_admission.cpp",
           "src/tests/transport_tube_meter.cpp", "src/tests/transport_tube_recovery.cpp",
           "src/tests/transport_tube_refusal.cpp", "src/tests/transport_tube_readiness.cpp",
           "src/tests/transport_tube_interchange.cpp", "src/tests/transport_tube_meter_fixture.h",
           "src/widgets/vehicle_widget.h", "src/widgets/depot_widget.h", "src/widgets/build_vehicle_widget.h", "src/lang/english.txt")


def run_one(configuration, focus):
    """Verify one original native command or actual capsule control obligation.

    @param configuration Current Debug or RelWithDebInfo native test executable.
    @param focus Exact single compiled case/tag, bounded to one native process.
    @return Current report path only after a nonempty actual native selection passes.
    """
    # Verify the named current owners before paying for a native run; a guessed
    # or missing source path cannot support this producer return.
    for name in SOURCES:
        assert (ROOT / name).is_file(), name
    context = current_run_context("construction", "tube-native-controls")
    _, proof = native(context, configuration, CASES[focus], focus)
    report = {"_documentation": "r2mlvs7-underground-construction-and-fleet.md8C;original native purchase/refit/order/service/ordinarydomain/custody and actual control input;prepared boundary inputs do not replace original useful intermodal cargo provenance",
              "configuration": configuration, "focus": focus, "tag": CASES[focus], "native": proof,
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "boundary_runner": identity(ROOT / "src/tests/multilayer/construction_provider_return.py"),
              "sources": [identity(ROOT / name) for name in SOURCES], "native_version": 434}
    path = context.output("reports", f"tube-native-control-{configuration}-{focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Choose an isolated current capsule command/input obligation.

    @return None; failed, empty or missing-owner selections cannot publish a pass.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=CASES, required=True)
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
