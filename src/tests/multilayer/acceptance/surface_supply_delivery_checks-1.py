"""Bounded original native producer and purchased-construction delivery checks.

@file surface_supply_delivery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CD.
Prepared floor/capital are explicit scenario inputs; this does not prove an
unassisted excavation or final Surface factory bootstrap.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import create_run_context
from runner import run_native


def identity(path):
    """Hash immutable evidence bytes.

    @param path Existing repository-owned input or output.
    @return Exact path, byte count and digest.
    """
    raw = path.read_bytes()
    return dict(path=path.relative_to(ROOT).as_posix(), bytes=len(raw),
                sha256=hashlib.sha256(raw).hexdigest())


def execute(configuration, phase):
    """Execute one focused native process with a hard elapsed-time bound.

    @param configuration Actual independently linked native configuration.
    @param phase Producer gate or driven delivery, never an inferred latest run.
    @return Bound process report and actual native output identities.
    """
    context = create_run_context("space", "surface-supply-delivery-" + phase)
    context.prepare_runtime()
    executable = ROOT / "build" / configuration / (
        "openttd_test.exe" if phase == "producers" else "openttd_surface_supply_test.exe")
    tags = ("[layer-recipe-batch],[layer-mine-production],[underground-fleet],[geothermal-collector],[geothermal-riser]"
            if phase == "producers" else "[surface-supply-driven]")
    log = context.logs / "native.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), tags, "--durations", "yes"],
                            cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), timeout=110)
    terminal = log.read_text(errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match, log
    # A selector ignored by an older binary cannot prove a newly authored
    # transport boundary. Require the native completion marker as well.
    if phase == "drive":
        assert "SURFACE_SUPPLY_DRIVEN_COMPLETE" in terminal, log
    else:
        assert "Recipe" in terminal and "Native trucks" in terminal and "Geothermal" in terminal, log
    sources = ("src/tests/surface_supply_delivery.cpp", "src/tests/underground_workflow_fixture.h",
               "src/tests/underground_depot_fixture.h", "src/surface_supply_order_runtime.cpp",
               "src/surface_supply.cpp", "src/economy.cpp", "src/cargoaction.cpp",
               "src/geothermal_collector.cpp", "src/geothermal_riser_construction.cpp",
               "src/underground_vehicle.cpp", "src/layer_portal_work.cpp", "src/train_cmd.cpp",
               "media/multilayer/build_content.py", "src/cargotype.cpp")
    if phase == "producers":
        sources = ("src/multilayer_industry.h", "src/tests/layer_recipe_batch.cpp",
                   "src/tests/layer_mine_production.cpp", "src/tests/underground_fleet.cpp",
                   "src/tests/geothermal_collector.cpp", "src/tests/geothermal_riser.cpp")
    report = dict(_documentation=__doc__, configuration=configuration, phase=phase,
                  assertions=int(match[1]), cases=int(match[2]), executable=identity(executable),
                  terminal=identity(log), harness=identity(Path(__file__)),
                  sources=[identity(ROOT / name) for name in sources],
                  outputs=[identity(path) for path in sorted(context.saves.glob("*.sav"))],
                  native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                  prepared_floor=True, prepared_capital=True, unassisted=False)
    target = context.reports / "verification.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)
    return report


def main():
    """Select one independently bounded verification burst from explicit arguments."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("producers", "drive"), required=True)
    args = parser.parse_args()
    execute(args.configuration, args.phase)


if __name__ == "__main__":
    main()
