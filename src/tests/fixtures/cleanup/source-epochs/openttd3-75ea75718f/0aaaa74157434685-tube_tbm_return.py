"""Observe one bounded actual paid TBM-to-tube final-use phase.

@file tube_tbm_return.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md Group21.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Generated initial world/logistics preconditions remain distinct from actual paid clearing.
"""

import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from construction_provider_return import ROOT, identity, native
from runner import current_run_context

CASES = {"clearing": "[tube-tbm-clearing]"}
SOURCES = (
    "src/tests/tube_tbm_clearance_fixture.h", "src/tests/tbm_reuse.cpp",
    "src/tests/underground_workflow.cpp", "src/tests/underground_workflow_fixture.h",
    "src/tests/underground_depot_fixture.h", "src/tbm_support.cpp", "src/tbm_work.cpp",
    "src/tbm_project.cpp", "src/tbm_replanning.h", "src/transport_tube_construction.cpp",
    "src/rail_cmd.cpp", "src/station_cmd.cpp", "src/industry_cmd.cpp",
)


def run_one(configuration, focus):
    """Run one isolated native phase and retain only earned original paid observations.

    @param configuration Current Debug or optimized native test executable.
    @param focus Explicit bounded actual clearing/construction phase.
    @return Report path after its exact one-case native success and real positive metrics.
    """
    context = current_run_context("tubes", "tbm-final-use")
    raw, proof = native(context, configuration, CASES[focus], focus)
    assert proof["cases"] == 1
    match = re.search(r"TUBE_TBM_CLEAR ticks=(\d+) cuts=(\d+) bends=(\d+) extensions=(\d+) earth=(\d+) rock=(\d+) work=(\d+) construction=(\d+) origin=(\d+) interchange=(\d+)", raw)
    assert match is not None, "actual paid non-rectangular clearing did not complete"
    metrics = dict(zip(("ticks", "cuts", "bends", "extensions", "earth", "rock", "work", "construction", "origin", "interchange"), map(int, match.groups())))
    assert min(metrics.values()) > 0
    assert metrics["extensions"] == 1
    owner_match = re.search(r"TUBE_TBM_OWNERS site=(\d+) portal=(\d+) cutter=(\d+) hauler=(\d+) earth_receiver=(\d+) rock_receiver=(\d+) job=(\d+) depot0=(\d+) depot1=(\d+) cash=(\d+)", raw)
    assert owner_match is not None
    owners = dict(zip(("site", "portal", "cutter", "hauler", "earth_receiver", "rock_receiver", "job", "depot0", "depot1", "cash"), map(int, owner_match.groups())))
    saved = re.search(r"TUBE_TBM_SAVE path=([^\r\n]+)", raw)
    assert saved is not None
    saved_path = Path(saved[1]).resolve()
    assert saved_path == context.output("saves", "tube-tbm-cleared-ready.sav").resolve()
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16B;actual paid native cutting/physical spoil disposal/whole withdrawal before curved tube construction on the same non-rectangular earned floor;initial world/company/commissioned logistics remain disclosed inputs",
        "configuration": configuration, "focus": focus, "tag": CASES[focus], "native": proof,
        "metrics": metrics, "owners": owners, "original_ready": identity(saved_path), "controller": identity(__file__),
        "boundary_runner": identity(ROOT / "src/tests/multilayer/construction_provider_return.py"),
        "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES], "native_version": 434,
    }
    path = context.output("reports", f"tube-tbm-{configuration}-{focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select the actual paid native final-use phase and independent110-second child.

    @return None; failed or incomplete physical work cannot publish a passing report.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=CASES, required=True)
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
