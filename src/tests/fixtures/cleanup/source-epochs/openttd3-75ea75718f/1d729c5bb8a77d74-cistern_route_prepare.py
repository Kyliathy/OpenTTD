"""Prepare a paid native Cistern modal route in the exact original completed chemical-source world.

@file cistern_route_prepare.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All source owners remain intact; useful route movement and receiver production belong to15DCC.
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
from cistern_source_ready import SOURCES as SOURCE_SOURCES

SOURCE_RECEIPT = "logs/tubes/cistern-source/20261018-native-01/verification-2.json"
SOURCES = (*SOURCE_SOURCES, "src/tests/cistern_useful_route_fixture.h", "src/tests/tube_cistern_route.cpp",
           "src/transport_tube.cpp", "src/transport_tube_engine.cpp", "src/transport_tube_power.cpp", "src/transport_tube_runtime.cpp")


def main():
    """Continue an exact opposite accepted useful-source world through real paid stopped route construction.

    @return None; changed originals, native command/date/carrier refusal or altered source/output owners prevent preparation evidence.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--haul-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / SOURCE_RECEIPT
    assert identity(receipt)["sha256"] == "27d2c1e9bde125da41a67f15de58be119cadd0518b8737337c766555b27cfc98"
    accepted = json.loads(receipt.read_text(encoding="utf-8"))
    history_path = ROOT / accepted["source_lineage"][0]["path"]; assert identity(history_path) == accepted["source_lineage"][0]
    history = json.loads(history_path.read_text(encoding="utf-8")); selected = identity(args.haul_report)
    assert selected in [row["report"] for row in history["records"]]
    haul = json.loads(args.haul_report.read_text(encoding="utf-8")); assert haul["configuration"] != args.config
    original = haul["native"]["output"]; assert original in accepted["original_source_worlds"]
    source = ROOT / original["path"]; assert identity(source) == original
    result = haul["result"]; assert result == accepted["source"]
    stock_path = ROOT / history["stock_receipt"]["path"]; assert identity(stock_path) == history["stock_receipt"]
    stock = json.loads(stock_path.read_text(encoding="utf-8"))
    carrier_path = ROOT / stock["carrier_receipt"]["path"]; assert identity(carrier_path) == stock["carrier_receipt"]
    carrier = json.loads(carrier_path.read_text(encoding="utf-8"))
    context = current_run_context("tubes", "cistern-route"); context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15DCB; native stopped paid Cistern route acquisition.\ncistern_route_prepare_check\n", encoding="utf-8")
    environment = {
        "OTTD_CISTERN_SOURCE_INPUT": str(source),
        "OTTD_CISTERN_STOCK_PARTS": ",".join(map(str, result["parts"])),
        "OTTD_CISTERN_STOCK_INDUSTRIES": ",".join(map(str, result["industries"])),
        "OTTD_CISTERN_STOCK_STATIONS": ",".join(map(str, result["stations"])),
        "OTTD_CISTERN_STOCK_DEPOSIT": str(carrier["source_contracts"]["salt_deposit"]),
        "OTTD_CISTERN_STOCK_TANKER_ENGINE": str(carrier["carrier"]["original_engine"]),
        "OTTD_CISTERN_READY_VALUES": ",".join(str(result[key]) for key in ("tick", "oil", "salt", "chemicals", "unused_oil", "unused_salt", "income", "cash", "loan", "reserve", "quarter_history")),
    }
    output, proof = native(context, args.config, "[.tube-cistern-route-prepare-native]", "route-prepare", environment)
    pattern = r"CISTERN_ROUTE_PREPARED tick=(\d+) cash=(\d+) loan=(\d+) source_chemicals=20 unused_oil=6 source_income=(\d+) consumed=0 chemical_deliveries=0 multiple_industry_per_town=true original_parts=([\d,]+) parts=([\d,]+) industries=([\d,]+) stations=([\d,]+) power=(\d+) path=([^\r\n]+)"
    match = re.search(pattern, output); assert match is not None
    prepared = dict(zip(("tick", "cash", "loan", "source_income"), map(int, match.groups()[:4])))
    for index, key in enumerate(("original_parts", "parts", "industries", "stations"), start=5):
        prepared[key] = list(map(int, match[index].split(",")))
    prepared["power"] = int(match[9])
    prepared["multiple_industry_per_town"] = True
    assert prepared["tick"] == result["tick"] and 0 < prepared["cash"] < result["cash"] and prepared["loan"] == result["loan"]
    assert prepared["source_income"] == result["income"] and prepared["original_parts"] == result["parts"]
    assert len(set(prepared["original_parts"] + prepared["parts"])) == 9
    assert len(prepared["parts"]) == 5 and len(set(prepared["industries"])) == 4 and len(set(prepared["stations"])) == 6
    assert prepared["industries"][:3] == result["industries"] and prepared["stations"][:3] == result["stations"]
    saved = Path(match[10]); assert len(world_chunks(saved)) == 126 and identity(source) == original
    proof.update(input=original, output=identity(saved))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DCB; original source preserved, command-funded actual stopped Cistern modal fleet/route/PCHM receiver and paid power",
        "configuration": args.config, "focus": "route-prepare", "native": proof, "prepared": prepared,
        "source_report": selected, "source_receipt": identity(receipt), "controller": identity(__file__),
        "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Exact original source input retains all upstream participants and six unused Oil. The synchronized native setting command explicitly enables saved multiple-industry-per-town for the second actual Chemical Works in this one-town route scenario. Native commands fund its PCHM alternative, nine-part fleet, modal stations/rail and paid tube power. Floor/completed machinery/Connected shafts remain prepared boundaries. Every front remains stopped/empty; original20CHEM is undispatched. No route movement, useful intake, loaded repair or TBM/bootstrap claim.",
    }
    path = context.output("reports", f"cistern-route-prepare-{args.config}.json"); path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
