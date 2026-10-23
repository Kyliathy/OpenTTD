"""Drive the exact paid original Cistern ready world into useful native Propellant output and whole empty returns.

@file cistern_useful_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No construction/source fixture, packet grant, arrival/order skip, clock or account repair runs in this reader.
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
from geothermal_tube_binding_files import world_chunks
from cistern_route_prepare import SOURCES as PREPARE_SOURCES

PREPARE_RECEIPT = "logs/tubes/cistern-route/20261018-native-01/prepare-verification.json"
SOURCES = (*PREPARE_SOURCES, "src/transport_tube_stock.cpp", "src/vehicle.cpp", "src/station_cmd.cpp")


def original_route_environment(prepared, receipt):
    """Resolve only accepted original source meanings/roles and actual paid route identities for normal loading.

    @param prepared Exact original preparation report retained by its immutable receipt.
    @param receipt Accepted paid preparation receipt with opaque original source lineage.
    @return Native observer environment preserving original participants and independently observed source/ready quantities.
    """
    source_path = ROOT / receipt["source_receipt"]["path"]; assert identity(source_path) == receipt["source_receipt"]
    source_receipt = json.loads(source_path.read_text(encoding="utf-8")); source = source_receipt["source"]
    cursor = source_receipt
    # Follow exact opaque original receipts to the observed native tanker and
    # geological reserve, without fresh-comparing dated nested binaries or
    # guessing a cargo/engine/pool ID from another fixture.
    for key in ("source_lineage", "stock_receipt", "carrier_receipt"):
        reference = cursor[key][0] if key == "source_lineage" else cursor[key]
        path = ROOT / reference["path"]; assert identity(path) == reference; cursor = json.loads(path.read_text(encoding="utf-8"))
    point = prepared["prepared"]
    values = (point["tick"], point["cash"], point["loan"], point["source_income"], source["chemicals"], source["unused_oil"], source["reserve"], source["oil"], source["salt"])
    return {
        "OTTD_CISTERN_STOCK_PARTS": ",".join(map(str, point["original_parts"])),
        "OTTD_CISTERN_STOCK_INDUSTRIES": ",".join(map(str, point["industries"][:3])),
        "OTTD_CISTERN_STOCK_STATIONS": ",".join(map(str, point["stations"][:3])),
        "OTTD_CISTERN_STOCK_DEPOSIT": str(cursor["source_contracts"]["salt_deposit"]),
        "OTTD_CISTERN_STOCK_TANKER_ENGINE": str(cursor["carrier"]["original_engine"]),
        "OTTD_CISTERN_ROUTE_PARTS": ",".join(map(str, point["parts"])),
        "OTTD_CISTERN_ROUTE_REFERENCES": ",".join(map(str, (*point["stations"][3:], point["industries"][3], point["power"]))),
        "OTTD_CISTERN_ROUTE_VALUES": ",".join(map(str, values)),
    }


def main():
    """Load an exact opposite original paid route, then require useful native modal delivery, conserved owners and whole empty returns.

    @return None; changed originals, lost provenance, missing useful output/payment or incomplete physical returns prevent a route report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--prepare-report", type=Path, required=True)
    args = parser.parse_args(); receipt = ROOT / PREPARE_RECEIPT
    assert identity(receipt)["sha256"] == "9d3d8bbe1ae9509bfc9555b47f9aaad9b3753bbb8e8280effff8b1b641c49561"
    accepted = json.loads(receipt.read_text(encoding="utf-8")); selected = identity(args.prepare_report)
    assert selected in [row["report"] for row in accepted["records"]]
    prepared = json.loads(args.prepare_report.read_text(encoding="utf-8")); assert prepared["configuration"] != args.config
    original = prepared["native"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    context = current_run_context("tubes", "cistern-route"); context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15DCC; original paid Cistern useful native route.\ncistern_useful_check\n", encoding="utf-8")
    environment = original_route_environment(prepared, accepted); environment["OTTD_CISTERN_ROUTE_INPUT"] = str(source)
    output, proof = native(context, args.config, "[.tube-cistern-useful-native]", "cistern-useful", environment)
    names = ("year", "original_tick", "tick", "route_ticks", "delivered", "propellant", "unused_oil", "reserve", "income", "total_income", "cash", "loan", "consumed", "quarter_history")
    pattern = "CISTERN_USEFUL " + " ".join(name + r"=(\d+)" for name in names) + r" scenes=5 returns=3 empty_ticks=148 path=([^\r\n]+)"
    match = re.search(pattern, output); assert match is not None
    result = dict(zip(names, map(int, match.groups()[:14])))
    assert result["year"] == 1980 and result["original_tick"] == prepared["prepared"]["tick"]
    assert 0 < result["route_ticks"] < 24000 and result["tick"] == result["original_tick"] + result["route_ticks"] + 148
    assert result["delivered"] == result["propellant"] == 20 and result["unused_oil"] == 6 and result["reserve"] == 31980
    assert result["income"] > 0 and result["total_income"] == prepared["prepared"]["source_income"] + result["income"]
    assert result["cash"] > 0 and result["loan"] == 0 and result["consumed"] > 0
    scenes = [dict(zip(("scene", "tick", "cargo", "tile"), map(int, row))) for row in re.findall(r"CISTERN_ROUTE_SCENE scene=(\d+) tick=(\d+) cargo=(\d+) tile=(\d+)", output)]
    returned = [dict(zip(("role", "tick"), map(int, row))) for row in re.findall(r"CISTERN_ROUTE_RETURN role=(\d+) tick=(\d+)", output)]
    assert len(scenes) == 5 and {row["scene"] for row in scenes} == set(range(5)) and all(row["cargo"] > 0 for row in scenes)
    assert len(returned) == 3 and {row["role"] for row in returned} == set(range(3))
    saved = Path(match[15]); assert len(world_chunks(saved)) == 126 and identity(source) == original
    proof.update(input=original, output=identity(saved))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DCC; exact original paid Cistern modal shipment/real useful PCHM output/whole empty returns",
        "configuration": args.config, "focus": "cistern-useful", "native": proof, "result": result, "scenes": scenes, "returned": returned,
        "prepare_report": selected, "prepare_receipt": identity(receipt), "controller": identity(__file__),
        "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Normal loading retains the exact original paid ready world and all upstream owners. Original CHEM producer dispatch and normal service/orders/loading earn both conventional legs, layer-local paid Cistern work and useful PCHM intake/output. Full native clocks/accounts preserve raw histories, six unused Oil/reserve and whole empty returns. Inherited prepared floor/machinery/shafts and explicit saved multi-industry option remain boundaries. No source reconstruction, packet/rating grant, pose/order/clock/account repair or paid TBM/bootstrap claim.",
    }
    path = context.output("reports", f"cistern-useful-{args.config}.json"); path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
