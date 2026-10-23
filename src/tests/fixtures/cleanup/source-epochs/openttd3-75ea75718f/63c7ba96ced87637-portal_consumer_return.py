"""Run one delivered portal/tube/riser consumer boundary in an isolated native process.

@file portal_consumer_return.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md Groups12B-12C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared native boundary worlds remain distinct from retained original paid/useful history.
"""

import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from construction_provider_return import ROOT, identity, native
from runner import current_run_context

TUBE_CASES = {
    "catalogue": "[portal-contract]",
    "borer-domain": "An acquired TBM consist refuses tube guideways independently of compatibility masks",
    "capsule-shafts": "[tube-refusal-cross-layer]",
    "conventional-domain": "Tube and conventional trains refuse foreign movement domains despite broad masks",
    "portal-conversion": "Tube conversion refuses a complete portal pair without changing either endpoint",
    "tube-floor": "Tube terminals and utility footprints require every actual floor and approach",
    "interchange": "Tube interchange transfers actual native packets to a conventional carrier",
    "station-custody": "Tube interchange requires the actual joined station cargo identity",
}
RISER_CASES = {
    "riser-quote": "Riser admission quotes both actual connected housings without granting construction",
    "direct-elevator": "Direct Deep riser admission leaves intervening Underground solid",
    "relay": "Underground relay riser admission resolves its actual Surface endpoint",
    "riser-refusal": "Riser admission refuses foreign sealed locked and incomplete sites",
    "riser-funding": "Native riser funding refuses incomplete capital dates and physical participants atomically",
    "riser-bill": "Native riser consumes four real construction inputs and296 once-only ticks",
}
CASES = {**TUBE_CASES, **RISER_CASES}
SOURCES = (
    "src/map_layer.h", "src/layer_portal_cmd.h", "src/layer_portal_cmd.cpp", "src/layer_portal_work.cpp",
    "src/transport_tube.h", "src/transport_tube.cpp", "src/transport_tube_engine.cpp", "src/transport_tube_construction.cpp",
    "src/transport_tube_stock.cpp", "src/transport_tube_cmd.cpp", "src/transport_tube_power.cpp",
    "src/geothermal_riser.h", "src/geothermal_riser.cpp", "src/geothermal_riser_construction.cpp",
    "src/geothermal_site_cmd.cpp", "src/geothermal_utility.cpp", "src/utility_power.cpp", "src/multilayer_content.h",
    "src/tests/portal_work_state.cpp", "src/tests/transport_tube_refusal.cpp",
    "src/tests/transport_tube_prototype.cpp", "src/tests/transport_tube_construction.cpp",
    "src/tests/transport_tube_interchange.cpp", "src/tests/geothermal_riser.cpp",
    "src/tests/geothermal_riser_construction.cpp", "src/tests/geothermal_riser_fixture.h",
)


def run_one(configuration, focus):
    """Verify one current native consumer without combining unrelated fixture lifetimes.

    @param configuration Exact Debug or optimized native reader configuration.
    @param focus Named single-case commissioning/domain/construction/interchange obligation.
    @return Actual report path after nonempty native success; no public owner is modified.
    """
    context = current_run_context("portals", "consumer-return")
    _, proof = native(context, configuration, CASES[focus], focus)
    assert proof["cases"] == 1, "portal consumer focuses must remain isolated native cases"
    record = {
        "_documentation": "r2mlvs3-sealed-portals-and-deep-rail.md12B-12C; actual delivered native consumer boundaries;prepared floor/material worlds do not replace paid bootstrap/final-use proof",
        "configuration": configuration, "focus": focus, "tag": CASES[focus], "native": proof,
        "controller": identity(__file__), "boundary_runner": identity(ROOT / "src/tests/multilayer/construction_provider_return.py"),
        "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES], "native_version": 434,
    }
    path = context.output("reports", f"portal-consumer-{configuration}-{focus}.json")
    path.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select one explicit delivered native reader and single consumer obligation.

    @return None; the shared native runner enforces its independent110-second limit.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=CASES, required=True)
    args = parser.parse_args()
    print(run_one(args.config, args.focus), flush=True)


if __name__ == "__main__":
    main()
