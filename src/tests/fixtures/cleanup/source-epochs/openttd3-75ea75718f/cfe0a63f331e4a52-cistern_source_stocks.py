"""Earn actual stopped Oil/Salt stock in an accepted ordinary Cistern carrier world.

@file cistern_source_stocks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All raw cargo comes from native producers; physical input carriage and chemical output remain15DBC.
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
from cistern_source_checks import SOURCES as CARRIER_SOURCES

CARRIER_RECEIPT = "logs/tubes/cistern-source/20261018-native-01/carrier-verification-2.json"
SOURCES = (*CARRIER_SOURCES, "src/tests/tube_useful_tick_fixture.h",
           "src/layer_industry_mine.cpp", "src/layer_industry_processor.cpp", "src/industry_cmd.cpp")


def main():
    """Load the exact opposite accepted carrier, prepare native participants and retain genuinely produced stopped stocks.

    @return None; a changed original, source/quantity mismatch or native departure prevents this stock-world report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--carrier-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / CARRIER_RECEIPT
    assert identity(receipt)["sha256"] == "f94b72837c882e8b261fefa798d774225ae32b27c3c9b93a98228c054af663c0"
    history = json.loads(receipt.read_text(encoding="utf-8"))
    selected = identity(args.carrier_report)
    assert selected in [row["report"] for row in history["records"] if row["focus"] == "carrier-contract"]
    carrier = json.loads(args.carrier_report.read_text(encoding="utf-8"))
    assert carrier["configuration"] != args.config
    point, contracts = carrier["carrier"], carrier["source_contracts"]
    original = carrier["native"][1]["output"]; source = ROOT / original["path"]
    assert identity(source) == original
    context = current_run_context("tubes", "cistern-source")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15DBB; genuine stopped precursor stock only.\ncistern_stocks_check\n", encoding="utf-8")
    environment = {
        "OTTD_CISTERN_SOURCE_INPUT": str(source), "OTTD_CISTERN_SOURCE_HEAD": str(point["head"]),
        "OTTD_CISTERN_SOURCE_TANK": str(point["tanker"]), "OTTD_CISTERN_SOURCE_ORIGINAL_ENGINE": str(point["original_engine"]),
        "OTTD_CISTERN_SOURCE_OIL_TYPE": str(contracts["oil_type"]), "OTTD_CISTERN_SOURCE_CHEM_TYPE": str(contracts["chemical_type"]),
        "OTTD_CISTERN_SOURCE_DEPOSIT": str(contracts["salt_deposit"]),
    }
    output, proof = native(context, args.config, "[.tube-cistern-source-stocks-native]", "source-stocks", environment)
    pattern = r"CISTERN_STOCKS tick=(\d+) oil=(\d+) salt=(\d+) oil_opportunities=(\d+) salt_opportunities=(\d+) reserve_before=(\d+) reserve_after=(\d+) parts=([\d,]+) industries=([\d,]+) stations=([\d,]+) source_income=0 moving=0 chemical_output=0 path=([^\r\n]+)"
    match = re.search(pattern, output); assert match is not None
    stock = dict(zip(("tick", "oil", "salt", "oil_opportunities", "salt_opportunities", "reserve_before", "reserve_after"), map(int, match.groups()[:7])))
    stock.update(parts=list(map(int, match[8].split(","))), industries=list(map(int, match[9].split(","))), stations=list(map(int, match[10].split(","))))
    assert stock["parts"][:2] == [point["head"], point["tanker"]] and len(set(stock["parts"])) == 4
    assert stock["salt"] == stock["reserve_before"] - stock["reserve_after"] == 20
    assert stock["oil"] >= 20 and stock["salt_opportunities"] == 5 and stock["oil_opportunities"] > 0
    saved = Path(match[11]); assert len(world_chunks(saved)) == 126
    packets = [dict(zip(("id", "units", "source", "station", "age", "purpose"), map(int, row))) for row in re.findall(r"CISTERN_STOCK_PACKET id=(\d+) units=(\d+) source=(\d+) station=(\d+) age=(\d+) purpose=(\d+)", output)]
    assert packets and len({row["id"] for row in packets}) == len(packets)
    assert sum(row["units"] for row in packets) == stock["oil"] + stock["salt"]
    assert all(row["purpose"] == 0 for row in packets)
    finance = re.search(r"CISTERN_STOCK_FINANCE cash=(\d+) loan=(\d+) income=0", output); assert finance is not None
    stock.update(cash=int(finance[1]), loan=int(finance[2]))
    assert identity(source) == original
    proof.update(input=original, output=identity(saved))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DBB; actual native Oil/finite Salt stock with original carrier and stopped source trains; chemical input haul/output remains15DBC",
        "configuration": args.config, "focus": "source-stocks", "native": proof, "stock": stock, "packets": packets,
        "carrier_report": selected, "carrier_receipt": identity(receipt), "controller": identity(__file__),
        "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Native ordinary loading retains the original purchased carrier. Real commands fund/construct the source and rail. Prepared rock floor, completed native machinery and Connected conventional shaft are explicit boundaries. Bounded actual original Oil and finite Salt opportunities own all cargo. No source input carriage, chemical output, Cistern movement or TBM/bootstrap claim.",
    }
    path = context.output("reports", f"cistern-source-stocks-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
