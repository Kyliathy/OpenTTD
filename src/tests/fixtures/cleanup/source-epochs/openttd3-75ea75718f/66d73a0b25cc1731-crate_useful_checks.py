"""Verify the actual1975 Crate route from earned original Goods to a real town receipt.

@file crate_useful_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md task15CBB.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Both shafts belong to actual conventional freight; the capsule remains on its paid Underground guideway.
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
from goods_source_checks import SOURCES


def main():
    """Require the accepted producer, then run one complete isolated same-world actual Crate route.

    @return None; failed original production, modal travel, payment or empty returns prevents the route report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args()
    receipt = ROOT / "logs/tubes/goods-source/20261018-native-01/verification-2.json"
    assert identity(receipt)["sha256"] == "5885ed12ed02ce80fafb0325102ef4df307c25286ba29eca0b18da7461cbf33c"
    context = current_run_context("tubes", "crate-route")
    context.prepare_runtime()
    output, proof = native(context, args.config, "[tube-useful-crate-route]", "crate")
    match = re.search(r"CRATE_USEFUL year=1975 delivered=(\d+) source=original_sawmill native_wood=39 fed=true carried=true surfaced=true returns=3 income=(\d+) consumed=(\d+) ticks=(\d+) tick=(\d+)", output)
    assert match is not None
    route = dict(zip(("delivered", "income", "consumed_j", "route_ticks", "final_tick"), map(int, match.groups())))
    assert route["delivered"] == 39 and all(value > 0 for value in route.values())
    history = re.search(r"quarter_history=(\d+)", output)
    assert history is not None and int(history[1]) > 0
    route["native_quarter_history"] = int(history[1])
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15CBB.1; actual1975 original Goods, Surface/Underground conventional feed, paid layer-local Crate and conventional Surface town receipt/empty returns",
        "configuration": args.config, "native": proof, "route": route,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in (*SOURCES, "src/tests/transport_tube_crate_routes.cpp", "src/tests/crate_useful_world_fixture.h", "src/transport_tube_runtime.cpp", "src/transport_tube_power.cpp", "src/tests/multilayer/goods_source_checks.py")],
        "source_receipt": identity(receipt),
        "fixture_classification": "Prepared completed machinery, generated funded town, excavated floor and commissioned native portals remain explicit inputs. Every Wood/Goods unit is earned by original production and physical native orders/loading; source train retirement is an actual empty-depot sale. No arrival, order skip, stock or purpose rewrite. Ordinary files/runtime silhouette and actual TBM/bootstrap remain separate.",
    }
    path = context.output("reports", f"crate-useful-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
