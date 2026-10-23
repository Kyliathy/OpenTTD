"""Earn genuine RFMT through original CORE/Coal carriage and native CFMT manufacturing.

@file forge_raw_delivery.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Finite sources continue normally; prepared floor and Connected shafts remain disclosed boundaries.
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
from forge_source_files import source_environment
from shield_source_files import parse_snapshot
from forge_raw_route_prepare import SOURCES as PREPARE_SOURCES

PREPARE_RECEIPT = "logs/tubes/forge-raw-route/20261018-native-01/prepare-verification.json"
PREPARE_SHA256 = "6677531f46cc146b26e0fd68d561a2c0c4c7937321c09c4180a727df3b38db22"
SOURCES = (*PREPARE_SOURCES, "src/tests/forge_raw_delivery_fixture.h", "src/tests/tube_forge_raw_delivery.cpp",
           "src/tests/multilayer/forge_raw_delivery.py", "src/vehicle.cpp", "src/economy.cpp", "src/cargopacket.cpp")


def original_environment(prepared, receipt):
    """Transport exact original roles and independent ready facts without reconstructing participants.

    @param prepared Exact opposite accepted paid preparation report.
    @param receipt Immutable preparation receipt retaining the original finite source lineage.
    @return Borrowed-loader environment for the existing four parts, three industries and three stations.
    """
    original = ROOT / receipt["source_receipt"]["path"]
    assert identity(original) == receipt["source_receipt"]
    source = json.loads(original.read_text(encoding="utf-8"))["source"]
    point = prepared["prepared"]
    values = [point[key] for key in ("tick", "cash", "loan", "copper", "coal", "copper_reserve", "coal_reserve")] + [20]
    environment = source_environment(source)
    environment.update(
        OTTD_FORGE_RAW_STATIONS=",".join(map(str, point["stations"])),
        OTTD_FORGE_RAW_PORTALS=",".join(map(str, point["portals"])),
        OTTD_FORGE_RAW_DELIVERY_VALUES=",".join(map(str, values)),
    )
    return environment


def validate_result(result, materials, scenes, returned, recipe, snapshot, point):
    """Independently verify genuine manufacture, physical scenes, returns and each separate native material balance.

    @param result Final real native clock, recipe, payment and pool facts.
    @param materials Both actual finite reserve/stock/intake/consumption summaries.
    @param scenes First actual loaded CORE/Coal wagons in Underground and Surface.
    @param returned Both whole empty home arrivals after useful RFMT manufacture.
    @param recipe First actual native CFMT output observation.
    @param snapshot Canonical original owner observations at the pure-produced save boundary.
    @param point Accepted original paid still-stopped source facts.
    @return None; lost inventory, fabricated output or missing physical carriage prevents acceptance.
    """
    assert result["year"] == 2005 and result["original_tick"] == point["tick"]
    assert 0 < result["route_ticks"] < 24000
    assert result["tick"] == point["tick"] + result["route_ticks"] + 148
    assert result["rfmt"] == result["batches"] * 2 >= 20
    assert result["income"] > 0 and result["cash"] > 0 and result["loan"] == 0
    assert snapshot["tick"] == result["tick"] and snapshot["cash"] == result["cash"] and snapshot["loan"] == 0
    assert snapshot["vehicles"] == 4 and snapshot["industries"] == snapshot["stations"] == 3
    assert snapshot["packets"] == result["packets"] and snapshot["quarters"] == result["quarters"]
    assert len(materials) == 2 and {row["role"] for row in materials} == {0, 1}
    # Each raw meaning conserves its original finite reserve independently.
    # Recipe history is a debit and native paid intake equals unused input
    # plus consumption, rather than another physical copy of delivered cargo.
    for row in materials:
        index = row["role"]
        name = ("copper", "coal")[index]
        assert row["reserve"] + row["extracted"] == point[name] + point[name + "_reserve"]
        assert row["extracted"] == row["dispatched"] + row["buffer"]
        assert row["extracted"] == row["buffer"] + row["waiting"] + row["unused"] + row["consumed"]
        assert row["delivered"] == row["unused"] + row["consumed"] > 0
        assert row["consumed"] == result["batches"] * (4 if index == 0 else 1)
        assert snapshot[f"reserve{index}"] == row["reserve"]
        assert snapshot[f"mine{index}_extracted"] == row["extracted"] and snapshot[f"mine{index}_dispatched"] == row["dispatched"]
        assert all(snapshot[f"part{part}_units"] == 0 for part in (index * 2, index * 2 + 1))
    assert len(scenes) == 4 and {row["scene"] for row in scenes} == set(range(4))
    # First loaded layer observations must be real positive native holds;
    # initial home storage cannot stand in for a useful whole empty return.
    for row in scenes:
        assert row["cargo"] > 0 and point["tick"] < row["tick"] < result["tick"] - 148
        assert row["layer"] == (1 if row["scene"] % 2 == 0 else 0)
    assert len(returned) == 2 and {row["role"] for row in returned} == {0, 1}
    assert point["tick"] < recipe["tick"] < result["tick"] - 148
    assert recipe["rfmt"] == recipe["batches"] * 2 > 0
    for row in returned:
        assert max(recipe["tick"], *(scene["tick"] for scene in scenes)) < row["tick"] <= result["tick"] - 148


def main():
    """Load an exact opposite paid original world and earn native input carriage, RFMT and empty returns.

    @return None; native failures or altered original identities/balances prevent a report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--prepare-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / PREPARE_RECEIPT
    assert identity(receipt)["sha256"] == PREPARE_SHA256
    accepted = json.loads(receipt.read_text(encoding="utf-8"))
    selected = identity(args.prepare_report)
    assert selected in [row["report"] for row in accepted["records"] if row.get("focus") == "raw-route-prepare"]
    prepared = json.loads(args.prepare_report.read_text(encoding="utf-8"))
    assert prepared["configuration"] != args.config
    original = prepared["native"]["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    environment = original_environment(prepared, accepted)
    environment["OTTD_FORGE_RAW_ROUTE_INPUT"] = str(source)
    context = current_run_context("tubes", "forge-raw-route")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FCC; native original raw delivery and CFMT manufacture.\nforge_raw_delivery_check\n", encoding="utf-8")
    output, proof = native(context, args.config, "[.tube-forge-raw-delivery-native]", "raw-delivery", environment)
    names = ("year", "original_tick", "tick", "route_ticks", "rfmt", "batches", "income", "cash", "loan", "packets", "quarters")
    pattern = "FORGE_RFMT " + " ".join(name + r"=(\d+)" for name in names)
    pattern += r" scenes=4 returns=2 empty_ticks=148 snapshot=([^\s]+) path=([^\r\n]+)"
    match = re.search(pattern, output)
    assert match is not None
    result = dict(zip(names, map(int, match.groups()[:len(names)])))
    snapshot = parse_snapshot(match[len(names) + 1])
    scenes = [dict(zip(("scene", "tick", "cargo", "tile", "layer"), map(int, row))) for row in
              re.findall(r"FORGE_RAW_SCENE scene=(\d+) tick=(\d+) cargo=(\d+) tile=(\d+) layer=(\d+)", output)]
    returned = [dict(zip(("role", "tick"), map(int, row))) for row in re.findall(r"FORGE_RAW_RETURN role=(\d+) tick=(\d+)", output)]
    recipe_match = re.search(r"FORGE_RAW_RECIPE tick=(\d+) rfmt=(\d+) batches=(\d+)", output)
    assert recipe_match is not None
    recipe = dict(zip(("tick", "rfmt", "batches"), map(int, recipe_match.groups())))
    raw_names = ("role", "reserve", "extracted", "dispatched", "buffer", "waiting", "delivered", "unused", "consumed")
    raw_pattern = "FORGE_RAW_MATERIAL " + " ".join(name + r"=(\d+)" for name in raw_names)
    materials = [dict(zip(raw_names, map(int, row))) for row in re.findall(raw_pattern, output)]
    validate_result(result, materials, scenes, returned, recipe, snapshot, prepared["prepared"])
    saved = Path(match[len(names) + 2])
    assert len(world_chunks(saved)) == 126 and identity(source) == original
    proof.update(input=original, output=identity(saved))
    report = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FCC; original actual CORE/Coal carriage, native RFMT manufacture and whole empty raw returns",
        configuration=args.config, focus="raw-delivery", native=proof, result=result, materials=materials,
        scenes=scenes, returned=returned, recipe=recipe, snapshot=snapshot, prepare_report=selected, prepare_receipt=identity(receipt),
        controller=identity(__file__), binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES],
        fixture_classification="Exact accepted original paid native world retains both finite Underground mines, original CORE/Coal4+5 carriers and actual Surface CFMT. One common native clock/front/industry opportunity plus real mine dispatch and earned station service physically carry both raw inputs across native Connected shafts. Native4CORE+1Coal->2RFMT batches retain every unused input, continuing reserve/buffer/queue/packet owner and current/old company receipt. Both whole empty home returns and148 further opportunities forbid replayed intake/payment while real source/recipe work continues. Prepared floor/Connected shafts remain disclosed boundaries; no source freeze, cargo/rating/pose/order/clock/cash repair, downstream Forge route, profit or paid TBM/bootstrap claim.",
    )
    path = context.output("reports", f"forge-raw-delivery-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
