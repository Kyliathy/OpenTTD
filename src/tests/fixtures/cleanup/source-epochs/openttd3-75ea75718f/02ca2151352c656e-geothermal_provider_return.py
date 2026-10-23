"""Observe selected native geothermal controls before the Plan7 provider return.

@file geothermal_provider_return.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md4C.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md15B-15D.
@see docs/zz-reach2/architecture/gui/archi-gui.md.
Prepared floor and machinery are disclosed boundary inputs, not paid bootstrap.
"""

import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context
from construction_provider_return import identity, native

ENTRY_CASES = {"availability": "[geothermal-control-availability]", "inspection": "[geothermal-control-inspection]",
               "plan": "[geothermal-window-plan]", "collector": "[geothermal-window-collector]",
               "placement": "[geothermal-window-placement]", "policy": "[geothermal-window-policy]",
               "utilities": "[geothermal-window-utilities]", "riser": "[geothermal-window-riser]",
               "physical-refusal": "[geothermal-window-physical-refusal]", "survey": "[geothermal-window-survey-overlay]"}
INPUT_CASES = {"mouse": "[geothermal-provider-native-input]", "site-input": "[geothermal-provider-site-input]",
               "utility-input": "[geothermal-provider-utility-input]"}
CASES = {**ENTRY_CASES, **INPUT_CASES}
SOURCES = ("src/geothermal_controls.h", "src/geothermal_controls.cpp", "src/geothermal_gui.h", "src/geothermal_gui.cpp",
           "src/underground_gui.cpp", "src/widgets/underground_widget.h", "src/multilayer_bootstrap.h",
           "src/geothermal_collector.h", "src/geothermal_generator.h", "src/geothermal_riser.h",
           "src/tests/geothermal_controls.cpp", "src/tests/geothermal_window_sites.cpp",
           "src/tests/geothermal_window_utilities.cpp", "src/tests/geothermal_window_information.cpp",
           "src/tests/native_input_fixture.h", "src/tests/tube_input_view_fixture.h")


def run_one(configuration, focus):
    """Run one independent current native provider or actual input obligation.

    @param configuration Current Debug or RelWithDebInfo native test executable.
    @param focus Explicit selected provider/input tag, bounded to one native process.
    @return Current report path only after the actual compiled selection passes.
    """
    context = current_run_context("construction", "geothermal-provider-return")
    _, proof = native(context, configuration, CASES[focus], focus)
    report = {"_documentation": "r2mlvs7-underground-construction-and-fleet.md4C;selected native vent/collector/bothgenerator/riser/physicalutility controls;future directprocessing factory actions and paid bootstrap are separate",
              "configuration": configuration, "focus": focus, "tag": CASES[focus], "native": proof,
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "boundary_runner": identity(ROOT / "src/tests/multilayer/construction_provider_return.py"),
              "sources": [identity(ROOT / name) for name in SOURCES], "native_version": 434}
    path = context.output("reports", f"geothermal-provider-{configuration}-{focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select one actual native geothermal construction control obligation.

    @return None; failed or empty selections cannot publish acceptance evidence.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=CASES, required=True)
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
