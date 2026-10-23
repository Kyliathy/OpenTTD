"""Drive exact original stopped Oil/Salt packets into native CHEM and earn whole empty returns.

@file cistern_source_haul.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Actual Cistern movement, chemical output dispatch and final source acceptance remain later groups.
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
from cistern_source_stocks import SOURCES as STOCK_SOURCES

STOCK_RECEIPT = "logs/tubes/cistern-source/20261018-native-01/stock-verification.json"
SOURCES = (*STOCK_SOURCES, "src/tests/cistern_source_world_fixture.h", "src/tests/tube_cistern_haul.cpp",
           "src/company_cmd.cpp", "src/economy.cpp", "src/train_cmd.cpp", "src/cargopacket.cpp")


def main():
    """Continue only an exact opposite accepted stock world through native input carriage and actual all-input processing.

    @return None; changed originals, failed native conservation or incomplete physical returns prevent a useful source report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--stock-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / STOCK_RECEIPT
    assert identity(receipt)["sha256"] == "798650fee1b3da68d41987e4a53dd09d0d63a307698714ddfdb495fee43c87a6"
    history = json.loads(receipt.read_text(encoding="utf-8"))
    selected = identity(args.stock_report); assert selected in [row["report"] for row in history["records"]]
    stock_report = json.loads(args.stock_report.read_text(encoding="utf-8")); assert stock_report["configuration"] != args.config
    stock, packets = stock_report["stock"], stock_report["packets"]
    carrier_receipt = ROOT / history["carrier_receipt"]["path"]; assert identity(carrier_receipt) == history["carrier_receipt"]
    carrier = json.loads(carrier_receipt.read_text(encoding="utf-8"))
    original = stock_report["native"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    context = current_run_context("tubes", "cistern-source"); context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15DBC; exact original raw input haulage and CHEM production.\ncistern_haul_check\n", encoding="utf-8")
    environment = {
        "OTTD_CISTERN_SOURCE_INPUT": str(source),
        "OTTD_CISTERN_STOCK_PARTS": ",".join(map(str, stock["parts"])),
        "OTTD_CISTERN_STOCK_INDUSTRIES": ",".join(map(str, stock["industries"])),
        "OTTD_CISTERN_STOCK_STATIONS": ",".join(map(str, stock["stations"])),
        "OTTD_CISTERN_STOCK_DEPOSIT": str(carrier["source_contracts"]["salt_deposit"]),
        "OTTD_CISTERN_STOCK_TANKER_ENGINE": str(carrier["carrier"]["original_engine"]),
        "OTTD_CISTERN_STOCK_VALUES": ",".join(str(stock[key]) for key in ("tick", "oil", "salt", "reserve_after", "cash", "loan")),
        "OTTD_CISTERN_STOCK_PACKET_COUNT": str(len(packets)),
        "OTTD_CISTERN_STOCK_PACKETS": ",".join(str(row[key]) for row in packets for key in ("id", "units", "source", "station", "age", "purpose")),
    }
    output, proof = native(context, args.config, "[.tube-cistern-source-haul-native]", "source-haul", environment)
    fields = ("year", "original_tick", "tick", "route_ticks", "oil", "salt", "chemicals", "unused_oil", "unused_salt")
    pattern = "CISTERN_CHEMICAL_SOURCE " + " ".join(name + r"=(\d+)" for name in fields)
    pattern += r" returns=2 empty_ticks=148 surfaced=true income=(\d+) cash=(\d+) loan=(\d+) quarter_history=(\d+) reserve=(\d+) parts=([\d,]+) industries=([\d,]+) stations=([\d,]+) path=([^\r\n]+)"
    match = re.search(pattern, output); assert match is not None
    result = dict(zip((*fields, "income", "cash", "loan", "quarter_history", "reserve"), map(int, match.groups()[:14])))
    result.update(parts=list(map(int, match[15].split(","))), industries=list(map(int, match[16].split(","))), stations=list(map(int, match[17].split(","))))
    assert result["year"] == 1980 and result["original_tick"] == stock["tick"]
    assert result["tick"] == stock["tick"] + result["route_ticks"] + 148
    assert result["oil"] == stock["oil"] and result["salt"] == stock["salt"] == result["chemicals"] == 20
    assert result["unused_oil"] == stock["oil"] - 20 and result["unused_salt"] == 0
    assert result["reserve"] == stock["reserve_after"] and result["income"] > 0 and result["cash"] > 0 and result["loan"] == stock["loan"]
    for key in ("parts", "industries", "stations"):
        assert result[key] == stock[key]
    saved = Path(match[18]); assert len(world_chunks(saved)) == 126 and identity(source) == original
    carried = [dict(zip(("role", "tick", "units", "tile"), map(int, row))) for row in re.findall(r"CISTERN_HAUL_LOADED role=(\d+) tick=(\d+) units=(\d+) tile=(\d+)", output)]
    returned = [dict(zip(("role", "tick", "delivered"), map(int, row))) for row in re.findall(r"CISTERN_HAUL_RETURN role=(\d+) tick=(\d+) delivered=(\d+)", output)]
    assert {row["role"] for row in carried} == {row["role"] for row in returned} == {0, 1}
    proof.update(input=original, output=identity(saved))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DBC; original physical raw input haulage, real CHEM output and whole empty returns; Cistern movement remains15DC",
        "configuration": args.config, "focus": "source-haul", "native": proof, "result": result, "carried": carried, "returned": returned,
        "stock_report": selected, "stock_receipt": identity(receipt), "controller": identity(__file__),
        "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Ordinary loading retains exact original raw packets/participants. Full native clocks/vehicle/loading/payment/aging plus actual CHEM work earn output and complete empty home returns. Prepared floor/completed machinery/Connected conventional shaft remain inherited disclosed boundaries; no raw production replay, output dispatch, Cistern movement or paid TBM/bootstrap claim.",
    }
    path = context.output("reports", f"cistern-source-haul-{args.config}.json"); path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
