"""Verify one actual TBM support/final-use boundary without an aggregate regression.

@file tbm_nonrail_return.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md Group15B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Paid native clearing remains distinct from initial generated-world/logistics preconditions.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from construction_provider_return import ROOT, identity, native
from runner import current_run_context

CASES = {
    "paid": "[tbm-nonrail-reuse]",
    "active-support": "Tube conversion cannot bypass an unfinished or unreleased TBM support owner",
    "occupied-support": "Released support stays while a train stands on it",
    "deep-date": "A Deep cutter commissions at an Underground portal only from 1 January 1980",
}
SOURCES = (
    "src/tbm.h", "src/tbm_support.cpp", "src/tbm_work.cpp", "src/tbm_project.cpp",
    "src/transport_tube.h", "src/transport_tube_construction.cpp", "src/rail_cmd.cpp",
    "src/station_cmd.cpp", "src/industry_cmd.cpp", "src/layer_industry_mine.cpp", "src/multilayer_industry.h",
    "src/road_cmd.cpp", "src/tests/tbm_reuse.cpp", "src/tests/underground_workflow_fixture.h",
    "src/tests/underground_workflow.cpp", "src/tests/underground_depot_fixture.h",
    "src/tests/transport_tube_construction.cpp", "src/tests/tbm_corridor.cpp", "src/tests/tbm_commission.cpp",
)


def run_one(configuration, focus):
    """Run one actual paid-construction or focused safe-support consumer obligation.

    @param configuration Exact current native Debug or optimized reader.
    @param focus One isolated paid workflow or active/occupied/date boundary.
    @return Current report path only after native success and required real-work metrics.
    """
    context = current_run_context("tbm", "nonrail-reuse")
    raw, proof = native(context, configuration, CASES[focus], focus)
    assert proof["cases"] == 1
    metrics = None
    if focus == "paid":
        match = re.search(r"TBM_NONRAIL_REUSE ticks=(\d+) cuts=(\d+) earth=(\d+) rock=(\d+) work=(\d+) replacement=(\d+) protected_retreat=1 withdrawn=1 terminal=(\d+) depot=(\d+) mine=(\d+) road=(\d+) open_chamber=(\d+)", raw)
        assert match is not None, "actual paid construction did not publish its native completion observations"
        metrics = dict(zip(("ticks", "cuts", "earth", "rock", "work", "replacement", "terminal", "depot", "mine", "road", "open_chamber"), map(int, match.groups())))
        assert min(metrics.values()) > 0
    report = {
        "_documentation": "r2mlvs4-tunnel-boring-machines.md15B; actual paid wide native excavation/spoil disposal/withdrawal and tube infrastructure;generated initial world/logistics are disclosed fixture preconditions,not normal-newgame or whole curved useful-route acceptance",
        "configuration": configuration, "focus": focus, "tag": CASES[focus], "native": proof,
        "metrics": metrics, "controller": identity(__file__),
        "boundary_runner": identity(ROOT / "src/tests/multilayer/construction_provider_return.py"),
        "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES], "native_version": 434,
    }
    path = context.output("reports", f"tbm-nonrail-{configuration}-{focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select one current reader and bounded paid-support/final-use focus.

    @return None; the native child retains its independent110-second timeout.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=CASES, required=True)
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
