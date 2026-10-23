"""Run one actual thermal lifecycle hook boundary for the selected owner return.

@file native_thermal_lifecycle.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md15D.
Prepared native unit inputs remain separate from the retained original paid plant chain.
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

CASES = {
    "claim": "[geothermal-site-claim]",
    "site-loss": "[geothermal-site-lifecycle]",
    "collector-loss": "[geothermal-collector-lifecycle]",
    "heat-history": "[geothermal-heat-accounting]",
    "heat-path": "[geothermal-heat-topology]",
    "heat-company": "[geothermal-heat-topology-owner]",
    "physical-owner": "[geothermal-utility-owner]",
    "vent-removal": "[geothermal-generator-operating-removal]",
    "vent-company": "[geothermal-generator-operating-company]",
    "magma-lifecycle": "[geothermal-magma-lifecycle]",
    "both-source-loss": "[geothermal-magma-contention-loss]",
    "riser-lifecycle": "[geothermal-riser-lifecycle]",
    "riser-link": "[geothermal-riser-link]",
    "riser-repair": "[geothermal-service-riser-repair]",
    "source-replacement": "[geothermal-service-source-replacement]",
    "recovery-refusal": "[geothermal-service-recovery-refusal]",
}
SOURCES = (
    "src/industry_cmd.cpp", "src/economy.cpp", "src/utility_power.cpp", "src/geothermal_site.cpp",
    "src/geothermal_collector.cpp", "src/geothermal_generator.cpp", "src/geothermal_riser.cpp",
    "src/geothermal_riser_construction.cpp", "src/geothermal_utility.cpp", "src/geothermal_service_cmd.cpp",
    "src/tests/geothermal_site.cpp", "src/tests/geothermal_collector.cpp", "src/tests/geothermal_heat.cpp",
    "src/tests/geothermal_topology.cpp", "src/tests/geothermal_utility.cpp", "src/tests/geothermal_generator_operating.cpp",
    "src/tests/geothermal_generator_magma.cpp", "src/tests/geothermal_riser_lifecycle.cpp", "src/tests/geothermal_riser_link.cpp",
    "src/tests/geothermal_town_recovery.cpp", "src/tests/geothermal_generator_operating_fixture.h", "src/tests/geothermal_town_service_fixture.h",
)


def run_one(configuration, focus):
    """Execute one current native producer obligation in an isolated bounded process.

    @param configuration Current Debug or RelWithDebInfo test executable.
    @param focus Exact selected lifecycle obligation from CASES.
    @return Current report path after a nonempty successful native execution.
    """
    context = current_run_context("lifecycle", "thermal-owner-return")
    _, proof = native(context, configuration, CASES[focus], focus)
    report = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md15D.2;actual native claim/machinery/route/grant/company lifecycle;prepared unit boundaries,not replacement paid-bootstrap proof",
              "configuration": configuration, "focus": focus, "tag": CASES[focus], "native": proof,
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "sources": [identity(ROOT / name) for name in SOURCES], "native_version": 434}
    target = context.output("reports", f"thermal-lifecycle-{configuration}-{focus}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Select one actual compiled reader and bounded thermal lifecycle tag.

    @return None; shared native runner enforces an independent110second limit.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=CASES, required=True)
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
