"""Observe the five delivered native capsule models and actual fleet controls.

@file tube_fleet_provider_return.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md8B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md10-13.
@see docs/zz-reach2/architecture/gui/archi-gui.md.
Native boundary fixtures do not replace original paid useful-route/art evidence.
"""

import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
from construction_provider_return import current_run_context, identity, native

ENTRY_CASES = {"definitions": "[transport-tube-definitions]", "normal-registration": "[transport-tube-normal-registration]",
               "startup": "[transport-tube-calendar-startup]", "monthly": "[transport-tube-calendar-monthly]",
               "dates": "[transport-tube-admission-date]", "purchase-ui": "[transport-tube-admission-ui]",
               "art": "[transport-tube-model-art]", "fleet": "[transport-tube-controls-fleet]"}
INPUT_CASES = {"models-input": "[tube-provider-model-input]", "fleet-input": "[tube-provider-fleet-input]"}
CASES = {**ENTRY_CASES, **INPUT_CASES}
SOURCES = ("src/transport_tube.h", "src/transport_tube_engine.cpp", "src/transport_tube_gui.h", "src/transport_tube_gui.cpp",
           "src/underground_gui.h", "src/transport_tube_model.h", "src/multilayer_content.h",
           "src/tests/transport_tube_definitions.cpp", "src/tests/transport_tube_calendar.cpp",
           "src/tests/transport_tube_admission.cpp", "src/tests/transport_tube_controls.cpp",
           "src/tests/transport_tube_fixture.h", "src/tests/native_input_fixture.h", "src/dropdown.cpp", "src/widgets/dropdown_widget.h")


def run_one(configuration, focus):
    """Verify one current native identity/date/art/fleet or actual input boundary.

    @param configuration Current Debug or RelWithDebInfo native test executable.
    @param focus Exact isolated selected provider/input tag, never a full suite.
    @return Current report path after a nonempty real native selection passes.
    """
    context = current_run_context("construction", "tube-fleet-return")
    _, proof = native(context, configuration, CASES[focus], focus)
    report = {"_documentation": "r2mlvs7-underground-construction-and-fleet.md8B;five original native identities/canonical dates/cargos/art/fleet;prepared fixture inputs are separate from paid useful-route/runtime/human acceptance",
              "configuration": configuration, "focus": focus, "tag": CASES[focus], "native": proof,
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "boundary_runner": identity(ROOT / "src/tests/multilayer/construction_provider_return.py"),
              "sources": [identity(ROOT / name) for name in SOURCES], "native_version": 434}
    path = context.output("reports", f"tube-fleet-{configuration}-{focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Choose one current five-model native provider or actual input obligation.

    @return None; missing/failed compiled selections cannot produce acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=CASES, required=True)
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
