"""Fund actual finite CORE/Coal sources and earn undispatched native stocks in the accepted2005 world.

@file forge_source_stock.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FBB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Whole stopped raw carriers and an unfed CFMT refinery remain original paid owners; useful carriage follows later.
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
from forge_source_prerequisites import SOURCES as PREREQUISITE_SOURCES

PREREQUISITE_RECEIPT = "logs/tubes/forge-source/20261018-native-01/prerequisite-verification.json"
PREREQUISITE_SHA256 = "69dca512e78f1db6ec89d2e5d80c8a37d377048ccb2df8eda33422e5966c0fa7"
SOURCES = (*PREREQUISITE_SOURCES, "src/tests/forge_source_world_fixture.h", "src/tests/multilayer/forge_source_stock.py",
           "src/rail_cmd.cpp", "src/vehicle_cmd.cpp", "src/train_cmd.cpp")


def main():
    """Load the exact opposite accepted original world, buy native participants and earn conserved finite raw stocks.

    @return None; changed inputs, rejected paid commands or lost native reserve/quantity ownership prevent a stock report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--prerequisite-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / PREREQUISITE_RECEIPT
    assert identity(receipt)["sha256"] == PREREQUISITE_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    selected = identity(args.prerequisite_report)
    assert selected in [row["report"] for row in history["records"] if row.get("focus") == "native"]
    original = json.loads(args.prerequisite_report.read_text(encoding="utf-8"))
    assert original["configuration"] != args.config
    input_identity = original["native"][1]["output"]
    source = ROOT / input_identity["path"]
    assert identity(source) == input_identity
    deposits = {row["label"]: row for row in original["deposits"]}
    expected = [deposits[label]["deposit"] for label in ("CORE", "COAL")]
    expected.extend(deposits[label]["reserve"] for label in ("CORE", "COAL"))
    context = current_run_context("tubes", "forge-source")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FBB; actual finite Forge raw stock production.\nforge_source_stock_check\n", encoding="utf-8")
    output, proof = native(context, args.config, "[.tube-forge-source-stock-native]", "raw-stock",
                           {"OTTD_FORGE_SOURCE_INPUT": str(source), "OTTD_FORGE_RAW_ORIGINAL": ",".join(map(str, expected))})
    names = ("year", "tick", "opportunities", "copper", "coal", "copper_reserve", "coal_reserve", "cash", "loan", "income")
    pattern = "FORGE_STOCK " + " ".join(name + r"=(\d+)" for name in names)
    pattern += r" parts=([\d,]+) industries=([\d,]+) deposits=([\d,]+) homes=([\d,]+) cargos=([\d,]+) path=([^\r\n]+)"
    match = re.search(pattern, output)
    assert match is not None
    values = dict(zip(names, map(int, match.groups()[:len(names)])))
    for index, key in enumerate(("parts", "industries", "deposits", "homes", "cargos"), len(names) + 1):
        values[key] = list(map(int, match[index].split(",")))
    assert values["year"] == 2005 and values["tick"] == values["opportunities"] and 0 < values["tick"] < 2048
    assert values["copper"] == values["coal"] == 40
    assert values["copper_reserve"] + values["copper"] == expected[2]
    assert values["coal_reserve"] + values["coal"] == expected[3]
    assert values["deposits"] == expected[:2] and values["loan"] == values["income"] == 0
    assert 0 < values["cash"] < original["definitions"]["cash"]
    assert len(values["parts"]) == len(set(values["parts"])) == 4
    assert len(values["industries"]) == len(set(values["industries"])) == 3
    assert len(values["homes"]) == len(set(values["homes"])) == 2
    assert values["cargos"] == [original["definitions"][key] for key in ("copper", "coal", "refined")]
    saved = Path(match[len(names) + 6])
    assert len(world_chunks(saved)) == 126 and identity(source) == input_identity
    proof.update(input=input_identity, output=identity(saved))
    report = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FBB; actual original finite CORE/Coal stocks, command-paid stopped carriers and unfed real CFMT",
        configuration=args.config, focus="raw-stock", native=proof, source=values,
        prerequisite_report=selected, prerequisite_receipt=identity(receipt), controller=identity(__file__),
        binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES],
        fixture_classification="Exact opposite ordinary-loaded2005 prerequisite world retains generated CORE/Coal deposits and initial32m funds. Native commands buy both actual finite mines, Surface CFMT and two original4+5 raw carriers with real underground depots. Completed machinery/prepared floor remain explicit boundaries. One common complete native clock/front/industry opportunity earns each original40-unit undispatched stock with exact reserve/history conservation while every carrier stays whole stopped/empty. CFMT has no inputs/output/batches; no source freeze, stock/packet/rating grant, native definition/date/account repair, RFMT manufacture, useful freight or TBM/bootstrap claim.",
    )
    path = context.output("reports", f"forge-source-stock-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
