"""Run bounded actual useful tube-route producers in native introduction-date worlds.

@file tube_useful_route_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15BB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Entry producer checks are separate from autonomous-route and runtime-file acceptance.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native

TAGS = {"ordered-producer": "[transport-tube-ordered-freight]", "mine-producer": "[layer-mine-production]", "factory-producer": "[layer-processor]", "mole": "[tube-useful-mole-route]"}
SOURCES = (
    "src/tests/transport_tube_interchange.cpp", "src/tests/layer_mine_production.cpp", "src/tests/layer_processor.cpp",
    "src/multilayer_industry.h", "src/layer_industry_mine.cpp",
    "src/layer_industry_processor.cpp", "src/multilayer_recipe.cpp", "src/multilayer_vehicle_catalogue.cpp",
    "src/transport_tube_power.cpp", "src/transport_tube_runtime.cpp", "src/economy.cpp", "src/train_cmd.cpp", "src/vehicle.cpp", "src/command.cpp",
    "src/timer/timer_game_calendar.cpp", "src/timer/timer_game_economy.cpp", "src/timer/timer_game_tick.cpp",
    "src/tests/transport_tube_useful_routes.cpp", "src/tests/CMakeLists.txt",
)


def main():
    """Run one isolated actual producer before a dependent useful-route mutation.

    @return None; failed/empty selection prevents the producer report and any acceptance claim.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=TAGS, required=True)
    args = parser.parse_args()
    context = current_run_context("tubes", "useful-routes")
    context.prepare_runtime()
    output, proof = native(context, args.config, TAGS[args.focus], args.focus)
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15BB; actual focused producer; local arrival and prepared-input limitations remain disclosed until autonomous-route acceptance",
        "configuration": args.config, "focus": args.focus, "native": proof,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    if args.focus == "mole":
        match = re.search(r"MOLE_USEFUL delivered=20 source=finite_mine year=1970 autonomous=true original_packets=(\d+) empty_returns=2 income=(\d+) consumed=(\d+) ticks=(\d+) tick=(\d+)", output)
        assert match is not None
        route = dict(zip(("original_packets", "income", "consumed_j", "route_ticks", "final_tick"), map(int, match.groups())))
        assert all(value > 0 for value in route.values())
        transitions = [tuple(map(int, row)) for row in re.findall(r"MOLE_ROUTE_STOCK tick=(\d+) origin=(\d+) capsule=(\d+) interchange=(\d+) wagon=(\d+) factory=(\d+)", output)]
        assert [row[1:] for row in transitions] == [(0, 20, 0, 0, 0), (0, 0, 20, 0, 0), (0, 0, 0, 20, 0), (0, 0, 0, 0, 20)]
        assert [row[0] for row in transitions] == sorted(row[0] for row in transitions)
        report.update(route=route, cargo_transitions=transitions)
    path = context.output("reports", f"tube-useful-{args.config}-{args.focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
