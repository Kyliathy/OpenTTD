"""Run one bounded native owner check for the selected lifecycle audit return.

@file native_lifecycle_return.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Groups15C-15E.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md9.5B.
Prepared unit boundaries remain distinct from retained original paid-world files.
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

TUBE_CASES = {
    "schema": "[multilayer-state-foundation]",
    "track": "[tube-component-track-edit]",
    "unrelated": "[tube-component-unrelated-edit]",
    "interchange": "[tube-component-interchange-edit]",
    "depot": "[tube-component-depot-edit]",
    "occupied-depot": "[tube-lifecycle-occupied-depot]",
    "occupied-platform": "[tube-lifecycle-occupied-platform]",
    "recovery": "[transport-tube-power-recovery]",
    "native-loss": "[tube-geothermal-work-loss]",
    "late-loss": "[tube-geothermal-work-late-loss]",
    "binding-topology": "[tube-geothermal-binding-topology]",
    "takeover": "[tube-company-loaded-takeover]",
    "removal": "[tube-company-terminal-removal]",
}
SOURCES = (
    "src/multilayer_state.h", "src/multilayer_state.cpp", "src/saveload/saveload.h", "src/map_layer.h",
    "src/transport_tube.h", "src/transport_tube_topology.cpp", "src/transport_tube_power.cpp",
    "src/transport_tube_geothermal.cpp", "src/transport_tube_runtime.cpp", "src/transport_tube_cmd.cpp",
    "src/rail_cmd.cpp", "src/station_cmd.cpp", "src/economy.cpp", "src/utility_power.cpp",
    "src/tests/multilayer_state.cpp", "src/tests/transport_tube_component_edits.cpp",
    "src/tests/transport_tube_recovery.cpp", "src/tests/transport_tube_company_lifecycle.cpp",
)


def run_one(configuration, focus):
    """Observe one actual native hook boundary in its own process and artifact home.

    @param configuration Current Debug or RelWithDebInfo test executable.
    @param focus Exact bounded schema/tube lifecycle tag declared above.
    @return Current report path after a nonempty successful native selection.
    """
    context = current_run_context("lifecycle", "tube-owner-return")
    _, proof = native(context, configuration, TUBE_CASES[focus], focus)
    report = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md15C;actual isolated native owner boundary;prepared units do not replace original paid/useful files",
              "configuration": configuration, "focus": focus, "tag": TUBE_CASES[focus], "native": proof,
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "sources": [identity(ROOT / name) for name in SOURCES], "native_version": 434}
    path = context.output("reports", f"native-lifecycle-{configuration}-{focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select an explicit compiled reader and one focused native lifecycle obligation.

    @return None; each native subprocess is independently bounded to110seconds.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=TUBE_CASES, required=True)
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
