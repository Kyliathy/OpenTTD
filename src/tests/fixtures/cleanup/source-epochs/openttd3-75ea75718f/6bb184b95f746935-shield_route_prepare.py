"""Prepare the actual paid Shield modal route in an exact accepted original1985 native source world.

@file shield_route_prepare.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15ECB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Stopped preparation retains original source/receiver/carrier; it earns no useful freight or Electronics output.
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
from shield_source_files import SOURCES as SOURCE_SOURCES, source_environment

SOURCE_RECEIPT = "logs/tubes/shield-source/20261018-native-01/verification.json"
SOURCES = (*SOURCE_SOURCES, "src/tests/shield_useful_route_fixture.h", "src/tests/tube_shield_route.cpp",
           "src/transport_tube_cmd.cpp", "src/transport_tube_cmd.h", "src/transport_tube_power.cpp", "src/transport_tube_stock.cpp",
           "src/layer_portal_map.h", "src/layer_portal_cmd.cpp", "src/station_cmd.cpp", "src/train_cmd.cpp", "src/rail_cmd.cpp")


def main():
    """Load the exact opposite accepted source and command-fund actual still-stopped modal participants and paid service.

    @return None; changed originals, rejected commands, altered source owners or premature useful work prevent a preparation report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--source-writer-report", type=Path, required=True)
    args = parser.parse_args(); receipt = ROOT / SOURCE_RECEIPT
    assert identity(receipt)["sha256"] == "57280f254c130b46841bf924ab72dafe5b13756652b7e502ce7d466165207d83"
    history = json.loads(receipt.read_text(encoding="utf-8")); source_result = history["source"]; selected = identity(args.source_writer_report)
    assert selected in [row["report"] for row in history["records"] if row.get("phase") == "write"]
    writer = json.loads(args.source_writer_report.read_text(encoding="utf-8")); assert writer["configuration"] != args.config
    point = next(row for row in writer["checkpoints"] if row["step"] == 0); source = ROOT / point["input"]["path"]; assert identity(source) == point["input"]
    values = (point["tick"], source_result["cash"], source_result["loan"], source_result["units"], source_result["reserve"], source_result["extracted"], source_result["dispatched"])
    assert point["tick"] == source_result["tick"] and point["snapshot"]["cash"] == source_result["cash"]
    environment = source_environment(source_result)
    environment.update(OTTD_SHIELD_ROUTE_INPUT=str(source), OTTD_SHIELD_ROUTE_SOURCE_VALUES=",".join(map(str, values)),
                       OTTD_SHIELD_ROUTE_SOURCE_SNAPSHOT=",".join(f"{key}={value}" for key, value in point["snapshot"].items()) + ",")
    context = current_run_context("tubes", "shield-route"); context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15ECB; actual original-world paid Shield preparation.\nshield_route_prepare_check\n", encoding="utf-8")
    output, proof = native(context, args.config, "[.tube-shield-route-prepare-native]", "prepare", environment)
    names = ("tick", "cash", "loan", "stock", "reserve", "extracted", "dispatched", "consumed")
    pattern = "SHIELD_ROUTE_PREPARED " + " ".join(name + r"=(\d+)" for name in names)
    pattern += r" parts=([\d,]+) industries=([\d,]+) stations=([\d,]+) power=(\d+) portals=([\d,]+) path=([^\r\n]+)"
    match = re.search(pattern, output); assert match is not None
    ready = dict(zip(names, map(int, match.groups()[:8])))
    ready.update(parts=list(map(int, match[9].split(","))), industries=list(map(int, match[10].split(","))), stations=list(map(int, match[11].split(","))),
                 power=int(match[12]), portals=list(map(int, match[13].split(","))))
    assert ready["tick"] == source_result["tick"] and 0 < ready["cash"] < source_result["cash"] and ready["loan"] == 0
    assert ready["stock"] == ready["extracted"] == 20 and ready["reserve"] == source_result["reserve"]
    assert ready["dispatched"] == ready["consumed"] == 0 and ready["parts"][:2] == source_result["parts"] and ready["industries"] == source_result["industries"]
    assert len(ready["parts"]) == len(set(ready["parts"])) == 5 and len(ready["stations"]) == len(set(ready["stations"])) == 4
    assert len(ready["portals"]) == len(set(ready["portals"])) == 3
    saved = Path(match[14]); assert len(world_chunks(saved)) == 126 and identity(source) == point["input"]
    proof.update(input=point["input"], output=identity(saved))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15ECB; exact original source command-paid still-stopped Shield/conventional Deep-UG-Surface preparation; useful movement remains15ECC",
        "configuration": args.config, "focus": "route-prepare", "native": proof, "prepared": ready,
        "source_writer": selected, "source_checkpoint": point, "source_receipt": identity(receipt), "controller": identity(__file__),
        "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Exact accepted ordinary source retains original mine/ELEC receiver/Deep4+6 sealed carrier,20RARE stock/31980reserve/work/history. Commands buy actual stations/track/depots/new Shield/delivery fleet and settle paid external source/binding through synchronized requests. Two conventional freight shafts plus a separate portal-side service housing are explicit Connected prepared-floor boundaries; all three real fronts remain whole stopped/empty with no energy use, station service/rating/packet grant or clock/cash/source repair. No useful delivery, Electronics manufacture or TBM/bootstrap claim.",
    }
    path = context.output("reports", f"shield-route-prepare-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


if __name__ == "__main__":
    main()
