"""Prepare the actual paid Forge modal route in the exact original manufactured-RFMT world.

@file forge_route_prepare.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Stopped preparation retains both finite mines, CFMT, raw keepers, genuine unused inputs and packet/account histories.
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
from shield_source_files import parse_snapshot
from forge_produced_files import SOURCES as SOURCE_SOURCES, produced_environment

SOURCE_RECEIPT = "logs/tubes/forge-produced/20261018-native-01/verification.json"
SOURCE_SHA256 = "1ba8a636d4f5eaa0763eeb54856675c701161d2423af3ad88123d14f5195097e"
SOURCES = (*SOURCE_SOURCES, "src/tests/forge_useful_route_fixture.h", "src/tests/tube_forge_route.cpp",
           "src/tests/multilayer/forge_route_prepare.py", "src/transport_tube_cmd.cpp", "src/transport_tube_cmd.h",
           "src/transport_tube_power.cpp", "src/transport_tube_stock.cpp", "src/industry_cmd.cpp", "src/train_cmd.cpp")


def validate_prepared(ready, snapshot, point, source):
    """Check original manufactured stock, actual paid participants and a still-stopped native ready boundary.

    @param ready Real native preparation result and actual new participant references.
    @param snapshot Native original/new source/vehicle/station/electrical observations after paid preparation.
    @param point Exact original source0 file boundary selected from the opposite accepted writer.
    @param source Accepted manufactured result/unused inputs/reserves/account facts.
    @return None; stock loss, free preparation, invalid participant roles or premature work prevents acceptance.
    """
    original = point["snapshot"]
    assert ready["tick"] == point["tick"] == source["result"]["tick"]
    assert 0 < ready["cash"] < original["cash"] and ready["loan"] == ready["consumed"] == 0
    assert ready["rfmt"] == source["result"]["rfmt"] == 32 and ready["unused_coal"] == source["materials"][1]["unused"] == 112
    assert ready["copper_reserve"] == source["materials"][0]["reserve"] and ready["coal_reserve"] == source["materials"][1]["reserve"]
    assert ready["income"] == source["result"]["income"] and snapshot["cash"] == ready["cash"] and snapshot["tick"] == ready["tick"]
    assert snapshot["vehicles"] == 9 and snapshot["industries"] == 4 and snapshot["stations"] == 5 and snapshot["packets"] == 2
    assert len(ready["new_parts"]) == len(set(ready["new_parts"])) == 5
    assert not set(ready["new_parts"]) & {original[f"part{index}_id"] for index in range(4)}
    assert len(ready["stations"]) == len(set(ready["stations"])) == 4 and ready["stations"][:2] == [original["raw_station2_id"], original["raw_station1_id"]]
    assert len(ready["portals"]) == len(set(ready["portals"])) == 4
    assert ready["receiver"] == snapshot["receiver"] and ready["power"] == snapshot["route_source"]
    assert all(snapshot[f"route_part{index}_units"] == 0 for index in range(5))
    assert snapshot["route_part2_capacity"] == 80


def main():
    """Load exact opposite original RFMT and pay native commands for actual still-stopped Forge modal owners.

    @return None; changed originals, rejected native commands or altered original source/material/history owners prevent a ready report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--source-writer-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / SOURCE_RECEIPT
    assert identity(receipt)["sha256"] == SOURCE_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    selected = identity(args.source_writer_report)
    assert selected in [row["report"] for row in history["records"] if row.get("phase") == "write"]
    writer = json.loads(args.source_writer_report.read_text(encoding="utf-8"))
    assert writer["configuration"] != args.config
    point = next(row for row in writer["checkpoints"] if row["step"] == 0)
    source = ROOT / point["input"]["path"]
    assert identity(source) == point["input"]
    delivery_receipt = ROOT / history["delivery_receipt"]["path"]
    assert identity(delivery_receipt) == history["delivery_receipt"]
    delivery_history = json.loads(delivery_receipt.read_text(encoding="utf-8"))
    original_delivery = next(row for row in delivery_history["records"] if row["report"] == writer["delivery_report"])
    environment = produced_environment(delivery_history, original_delivery)
    values = (point["tick"], point["snapshot"]["cash"], point["snapshot"]["loan"], history["result"]["rfmt"], history["materials"][1]["unused"],
              history["materials"][0]["reserve"], history["materials"][1]["reserve"], history["result"]["income"])
    environment.update(OTTD_FORGE_ROUTE_INPUT=str(source), OTTD_FORGE_ROUTE_SOURCE_VALUES=",".join(map(str, values)),
                       OTTD_FORGE_ROUTE_SOURCE_SNAPSHOT=",".join(f"{key}={value}" for key, value in point["snapshot"].items()) + ",")
    context = current_run_context("tubes", "forge-route")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FDB; actual original-world paid Forge modal preparation.\nforge_route_prepare_check\n", encoding="utf-8")
    output, proof = native(context, args.config, "[.tube-forge-route-prepare-native]", "prepare", environment)
    names = ("tick", "cash", "loan", "rfmt", "unused_coal", "copper_reserve", "coal_reserve", "income", "consumed")
    pattern = "FORGE_ROUTE_PREPARED " + " ".join(name + r"=(\d+)" for name in names)
    pattern += r" new_parts=([\d,]+) receiver=(\d+) stations=([\d,]+) power=(\d+) portals=([\d,]+) snapshot=([^\s]+) path=([^\r\n]+)"
    match = re.search(pattern, output)
    assert match is not None
    ready = dict(zip(names, map(int, match.groups()[:len(names)])))
    ready.update(new_parts=list(map(int, match[10].split(","))), receiver=int(match[11]), stations=list(map(int, match[12].split(","))), power=int(match[13]), portals=list(map(int, match[14].split(","))))
    snapshot = parse_snapshot(match[15])
    validate_prepared(ready, snapshot, point, history)
    saved = Path(match[16])
    assert len(world_chunks(saved)) == 126 and identity(source) == point["input"]
    proof.update(input=point["input"], output=identity(saved))
    report = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FDB; exact original genuine RFMT command-paid still-stopped Forge/conventional modal preparation",
        configuration=args.config, focus="route-prepare", native=proof, prepared=ready, snapshot=snapshot,
        source_writer=selected, source_checkpoint=point, source_receipt=identity(receipt), controller=identity(__file__),
        binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES],
        fixture_classification="Exact accepted original32RFMT/112unusedCoal world retains both finite mines/CFMT/raw4+5 keepers,31476reserves/460CORE+396Coal queues/two genuine packets and original recipe/account histories. Native commands buy a NEW4+5 RFMT feed using the existing Coal Surface/UG corridor, actual model13 Forge, final4+5 delivery and Surface ELEC. Native12-row station spread admits the real joined Coal/Forge interchange. Actual synchronized service/binding commands pay the external source invoice; zero lifetime energy and all five fronts whole stopped/empty prove preparation only. Two existing and two new disclosed Connected pairs, prepared floor and newly completed paid ELEC machinery remain boundaries. No useful RFMT freight, Electronics manufacture, source freeze, cargo/rating/pose/order/date/account repair, operating profit or paid TBM/bootstrap claim.",
    )
    path = context.output("reports", f"forge-route-prepare-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
