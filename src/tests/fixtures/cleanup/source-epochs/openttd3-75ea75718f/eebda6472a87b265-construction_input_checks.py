"""Observe actual native input for delivered construction provider controls.

@file construction_input_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md4B.2.
Real hit testing is separate from callback-only producer entry and from paid bootstrap proof.
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

CASES = {"mouse": "[tube-provider-native-input]", "viewport": "[tube-provider-viewport-input]", "geothermal": "[tube-provider-geothermal-input]"}
SOURCES = ("src/underground_gui.cpp", "src/transport_tube_gui.cpp", "src/window.cpp", "src/widgets/underground_widget.h",
           "src/tests/transport_tube_controls.cpp", "src/tests/transport_tube_geothermal_controls.cpp", "src/tests/tube_input_view_fixture.h",
           "src/tests/native_input_fixture.h", "src/tests/underground_view_fixture.h", "src/tests/CMakeLists.txt")


def run_one(configuration, focus):
    """Drive one actual native input obligation through the bounded test runner.

    @param configuration Current Debug or RelWithDebInfo native test executable.
    @param focus Exact compiled input characterization/acceptance tag.
    @return Current report path after all native assertions pass.
    """
    context = current_run_context("construction", "tube-provider-input")
    _, proof = native(context, configuration, CASES[focus], focus)
    report = {"_documentation": "r2mlvs7-underground-construction-and-fleet.md4B.2;actual native provider input boundary;prepared geometry is not paid final-use/bootstrap acceptance",
              "configuration": configuration, "focus": focus, "native": proof, "controller": identity(__file__),
              "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES]}
    path = context.output("reports", f"construction-input-{configuration}-{focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select one explicit current reader and real native input obligation.

    @return None; native failure or an empty tag cannot produce acceptance metadata.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=CASES, default="mouse")
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
