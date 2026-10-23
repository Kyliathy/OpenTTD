"""Run isolated canonical tube company custody and original permission lifecycle checks.

@file tube_company_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared physical/material/stock inputs remain separate from useful-route/bootstrap acceptance.
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
    "takeover": "[tube-company-loaded-takeover]",
    "removal": "[tube-company-terminal-removal]",
    "external-recovery": "[transport-tube-power-recovery]",
    "native-source": "[tube-geothermal-source-lifecycle]",
    "binding-retirement": "[tube-geothermal-binding-retirement]",
    "utility-company": "[utility-power-company]",
    "heat-company": "[utility-heat-company]",
    "feed-company": "[utility-local-feed-recipient-company]",
    "path-company": "[utility-power-path-lifecycle]",
}
SOURCES = (
    "src/economy.cpp", "src/vehicle.cpp", "src/transport_tube.h", "src/transport_tube_power.cpp",
    "src/transport_tube_runtime.cpp", "src/transport_tube_geothermal.cpp", "src/transport_tube_cmd.cpp",
    "src/utility_power.cpp", "src/geothermal_generator.cpp", "src/tests/transport_tube_company_lifecycle.cpp",
    "src/tests/transport_tube_recovery.cpp", "src/tests/transport_tube_geothermal.cpp",
    "src/tests/transport_tube_geothermal_binding.cpp", "src/tests/geothermal_tube_switch_fixture.h",
    "src/tests/geothermal_tube_work_fixture.h", "src/tests/CMakeLists.txt",
    "src/tests/utility_power_company.cpp", "src/tests/utility_heat.cpp",
    "src/tests/utility_power_local_feed_registration.cpp", "src/tests/utility_power_transfer.cpp",
)


def main():
    """Run one bounded actual company lifecycle tag and retain exact native owners.

    @return None; native failure or an empty selection leaves acceptance absent.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=TAGS, required=True)
    args = parser.parse_args()
    context = current_run_context("tubes", "company-lifecycle")
    context.prepare_runtime()
    _, proof = native(context, args.config, TAGS[args.focus], args.focus)
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md14D; actual canonical company event, surviving native custody and retired original permission/history; prepared inputs remain boundary fixtures",
        "configuration": args.config, "focus": args.focus, "native": proof,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"tube-company-{args.config}-{args.focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
