"""Prepare command-paid conventional CORE/Coal routes in exact accepted original Forge raw worlds.

@file forge_raw_route_prepare.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Preparation preserves stopped carriers and raw stock; physical delivery/manufactured RFMT remain the next task.
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
from forge_source_files import SOURCES as SOURCE_SOURCES, source_environment

SOURCE_RECEIPT = "logs/tubes/forge-source/20261018-native-01/verification.json"
SOURCE_SHA256 = "3213729e82ceb9ef5c7892617576b78df649bbb8ea6980950e6017d62126fc5b"
SOURCES = (*SOURCE_SOURCES, "src/tests/forge_raw_route_fixture.h", "src/tests/tube_forge_raw_route.cpp",
           "src/tests/multilayer/forge_raw_route_prepare.py", "src/layer_portal_map.h", "src/layer_portal_cmd.cpp",
           "src/layer_portal_work.cpp", "src/station_cmd.cpp", "src/rail_cmd.cpp", "src/order_cmd.cpp")


def main():
    """Continue the exact opposite original stock boundary with paid real facilities and unchanged native source owners.

    @return None; changed originals, rejected commands, altered quantities/poses or premature delivery prevent a ready report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--source-writer-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / SOURCE_RECEIPT
    assert identity(receipt)["sha256"] == SOURCE_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    source_result = history["source"]
    selected = identity(args.source_writer_report)
    assert selected in [row["report"] for row in history["records"] if row.get("phase") == "write"]
    writer = json.loads(args.source_writer_report.read_text(encoding="utf-8"))
    assert writer["configuration"] != args.config
    point = next(row for row in writer["checkpoints"] if row["step"] == 0)
    source = ROOT / point["input"]["path"]
    assert identity(source) == point["input"]
    values = (point["tick"], source_result["cash"], source_result["loan"], source_result["copper"], source_result["coal"], source_result["copper_reserve"], source_result["coal_reserve"])
    environment = source_environment(source_result)
    environment.update(OTTD_FORGE_RAW_ROUTE_INPUT=str(source), OTTD_FORGE_RAW_PREPARE_VALUES=",".join(map(str, values)),
                       OTTD_FORGE_RAW_SOURCE_SNAPSHOT=",".join(f"{key}={value}" for key, value in point["snapshot"].items()) + ",")
    context = current_run_context("tubes", "forge-raw-route")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FCB; original Forge paid raw-route preparation.\nforge_raw_route_prepare_check\n", encoding="utf-8")
    output, proof = native(context, args.config, "[.tube-forge-raw-route-prepare-native]", "prepare", environment)
    names = ("tick", "cash", "loan", "copper", "coal", "copper_reserve", "coal_reserve")
    pattern = "FORGE_RAW_PREPARED " + " ".join(name + r"=(\d+)" for name in names)
    pattern += r" parts=([\d,]+) industries=([\d,]+) stations=([\d,]+) portals=([\d,]+) path=([^\r\n]+)"
    match = re.search(pattern, output)
    assert match is not None
    ready = dict(zip(names, map(int, match.groups()[:len(names)])))
    for index, key in enumerate(("parts", "industries", "stations", "portals"), len(names) + 1):
        ready[key] = list(map(int, match[index].split(",")))
    assert ready["tick"] == source_result["tick"] and 0 < ready["cash"] < source_result["cash"] and ready["loan"] == 0
    assert all(ready[key] == source_result[key] for key in ("copper", "coal", "copper_reserve", "coal_reserve", "parts", "industries"))
    assert len(ready["stations"]) == len(set(ready["stations"])) == 3
    assert len(ready["portals"]) == len(set(ready["portals"])) == 2
    saved = Path(match[len(names) + 5])
    assert len(world_chunks(saved)) == 126 and identity(source) == point["input"]
    proof.update(input=point["input"], output=identity(saved))
    report = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FCB; exact original native raw-source paid preparation, no shipment/manufactured RFMT",
        configuration=args.config, focus="raw-route-prepare", native=proof, prepared=ready,
        source_writer=selected, source_checkpoint=point, source_receipt=identity(receipt), controller=identity(__file__),
        binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES],
        fixture_classification="Exact accepted original ordinary source retains both finite mine claims/reserves/history, original4+5 raw carriers and actual Surface CFMT. Native commands pay for conventional stations/rail/far depots and actual normal pickup/delivery/service orders. Two reciprocal Connected Underground/Surface shaft housings plus prepared floor remain disclosed boundaries, not paid TBM excavation. Distinct Surface platforms share one actual CFMT station/acceptance owner. Original tick740,40CORE/40Coal,31960reserves, all source/processor state and every carrier pose remain unchanged; all chains whole stopped/empty, no packets/queues/service/rating grant, work/clock/account repair, raw delivery or RFMT manufacture.",
    )
    path = context.output("reports", f"forge-raw-route-prepare-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
