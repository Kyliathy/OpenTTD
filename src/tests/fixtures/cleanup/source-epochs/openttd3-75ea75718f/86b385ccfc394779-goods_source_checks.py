"""Verify an actual1975 original Goods source before dependent Crate transport.

@file goods_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md task15CB.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The native Forest/Wood train/Sawmill owns all material; town buildings supply real Goods demand.
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

SOURCES = (
    "src/tests/tube_goods_source.cpp", "src/tests/tube_goods_source_fixture.h", "src/tests/tube_useful_tick_fixture.h",
    "src/tests/CMakeLists.txt", "src/industry_cmd.cpp", "src/table/build_industry.h", "src/economy.cpp",
    "src/town_cmd.cpp", "src/train_cmd.cpp", "src/vehicle.cpp", "src/multilayer_vehicle_catalogue.cpp",
)


def main():
    """Run one isolated real source gate and retain its complete original diagnostics.

    @return None; missing real production/conversion/physical return or town demand prevents acceptance evidence.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args()
    context = current_run_context("tubes", "goods-source")
    context.prepare_runtime()
    output, proof = native(context, args.config, "[tube-useful-goods-source]", "source")
    match = re.search(r"GOODS_SOURCE year=1975 source=original_forest converted=(\d+) primary_opportunities=(\d+) route_ticks=(\d+) town=(\d+) houses=(\d+) berth=(\d+) wood_income=(\d+)", output)
    assert match is not None
    route = dict(zip(("converted", "primary_opportunities", "route_ticks", "town", "houses", "berth", "wood_income"), map(int, match.groups())))
    assert all(value > 0 for value in route.values())
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15CB.1; real original Forest production and driven Wood delivery earn native Sawmill Goods; actual town catchment is verified, Crate route remains pending",
        "configuration": args.config, "native": proof, "source": route,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Prepared completed original machinery and a company-funded generated town are boundary inputs. No Wood/Goods stock, arrival, order skip or acceptance flag is injected. Original primary batches earn the first native dispatch lot above the unchanged Forest minimum; upstream forestry/bootstrap and balance remain outside this transport gate.",
    }
    path = context.output("reports", f"goods-source-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
