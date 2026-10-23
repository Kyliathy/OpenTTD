"""Run isolated paid native geothermal conduit topology/owner presentation boundaries.

@file geothermal_conduit_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared floor is explicit unit input; actual retained paid-world pixels remain separate84BC.2/84C acceptance.
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

CASES = ("[geothermal-conduit-art-masks]", "[geothermal-conduit-art-domains]", "[geothermal-utility-build]", "[geothermal-utility-path]", "[geothermal-utility-owner]", "[geothermal-electrical]", "[geothermal-utility-coexist]")
SOURCES = ("src/geothermal_conduit_sprite.h", "src/geothermal_conduit_sprite.cpp", "src/geothermal_utility.h", "src/geothermal_utility.cpp", "src/utility_power.cpp",
           "src/viewport.cpp", "src/industry_cmd.cpp", "src/rail_cmd.cpp", "src/road_cmd.cpp", "src/station_cmd.cpp", "src/layer_portal_cmd.cpp",
           "src/tests/geothermal_conduit_sprite.cpp", "src/tests/excavation_fixture.h", "src/tests/utility_power_fixture.h", "src/tests/multilayer/geothermal_conduit_checks.py")


def focused(configuration):
    """Check current actual installed topology and native owner boundaries, one isolated tag at a time.

    @param configuration Current compiled Debug or optimized native test executable.
    @return None; any failed native pose/payment/geometry/cache/purity boundary prevents a passing report.
    """
    context = current_run_context("graphics", "geothermal-conduits"); proofs = []
    # Every tag starts with its own original native map/company/utility owners;
    # no completed pipe or prior cache state can supply another case's input.
    for index, case in enumerate(CASES):
        _, proof = native(context, configuration, case, f"conduits-{index}"); proofs.append({"case": case, **proof})
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BC; actual paid topology boundary fixtures, not original paid-world pixel acceptance",
              "configuration": configuration, "native": proofs, "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"), "controller": identity(__file__),
              "sources": [identity(ROOT / name) for name in SOURCES], "remaining": "84BC.2 actual original paid source/branch/domain/occlusion pixels and both-config/four-target acceptance;84BD/84B.2/84C/84D remain pending."}
    path = context.output("reports", f"geothermal-conduits-{configuration}.json"); path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def main():
    """Select one bounded current native conduit burst.

    @return None; required current configuration and every actual native case must pass.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args(); focused(args.config)


if __name__ == "__main__":
    main()
