"""Drive exact original RFMT through the paid Forge modal route and whole empty returns.

@file forge_useful_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Both finite sources continue; actual unused inputs, recipe, packets, quarters and paid electrical owners remain conserved.
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
from forge_produced_files import produced_environment
from forge_route_prepare import SOURCES as PREPARE_SOURCES

PREPARE_RECEIPT = "logs/tubes/forge-route/20261018-native-01/prepare-verification.json"
PREPARE_SHA256 = "a05932f139fe4d4150ece5c79709890c6785a374254bda1b60df430735e64992"
SOURCES = (*PREPARE_SOURCES, "src/tests/multilayer/forge_useful_checks.py", "src/vehicle.cpp", "src/economy.cpp", "src/cargopacket.cpp")


def original_route_environment(prepared, receipt):
    """Transport exact original source/route roles without constructing or repairing actual participants.

    @param prepared Exact opposite accepted paid Forge preparation report.
    @param receipt Immutable preparation receipt retaining the original produced-source lineage.
    @return Borrowed native loader environment and independent accepted ready facts.
    """
    source_receipt = ROOT / receipt["source_receipt"]["path"]
    assert identity(source_receipt) == receipt["source_receipt"]
    source_history = json.loads(source_receipt.read_text(encoding="utf-8"))
    writer_path = ROOT / prepared["source_writer"]["path"]
    assert identity(writer_path) == prepared["source_writer"]
    writer = json.loads(writer_path.read_text(encoding="utf-8"))
    delivery_receipt = ROOT / source_history["delivery_receipt"]["path"]
    assert identity(delivery_receipt) == source_history["delivery_receipt"]
    deliveries = json.loads(delivery_receipt.read_text(encoding="utf-8"))
    original = next(row for row in deliveries["records"] if row["report"] == writer["delivery_report"])
    environment = produced_environment(deliveries, original)
    point = prepared["prepared"]
    values = [point[key] for key in ("tick", "cash", "loan", "rfmt", "unused_coal", "copper_reserve", "coal_reserve", "income")]
    environment.update(
        OTTD_FORGE_ROUTE_PARTS=",".join(map(str, point["new_parts"])),
        OTTD_FORGE_ROUTE_REFERENCES=",".join(map(str, (point["receiver"], *point["stations"], point["power"]))),
        OTTD_FORGE_ROUTE_PORTALS=",".join(map(str, point["portals"])),
        OTTD_FORGE_USEFUL_VALUES=",".join(map(str, values)),
    )
    return environment


def validate_result(result, materials, scenes, returned, snapshot, point):
    """Independently check actual useful RFMT, paid movement, source conservation and whole empty arrivals.

    @param result Final native useful freight, actual electrical and account observations.
    @param materials Independent original CORE/Coal reserve/stock/intake/recipe summaries.
    @param scenes First actual loaded modal scenes and genuine native layer positions.
    @param returned Three complete empty home arrivals after useful native intake/payment.
    @param snapshot Canonical original/new native owner observations at the pure final save boundary.
    @param point Accepted original still-stopped paid world facts.
    @return None; lost cargo, invented payment, missing movement or failed whole returns prevents acceptance.
    """
    assert result["year"] == 2005 and result["original_tick"] == point["tick"]
    assert 0 < result["route_ticks"] < 24000 and result["tick"] == point["tick"] + result["route_ticks"] + 148
    assert result["rfmt"] == point["rfmt"] and result["delivered"] == result["receiver"] >= 20
    assert result["delivered"] + result["source_buffer"] + result["waiting"] == result["rfmt"]
    assert result["income"] > 0 and result["total_income"] == point["income"] + result["income"]
    assert result["cash"] > 0 and result["loan"] == 0 and result["consumed"] > 0
    assert snapshot["tick"] == result["tick"] and snapshot["cash"] == result["cash"] and snapshot["loan"] == 0
    assert snapshot["vehicles"] == 9 and snapshot["industries"] == 4 and snapshot["stations"] == 5
    assert snapshot["packets"] == result["packets"] and snapshot["quarters"] == result["quarters"]
    assert all(snapshot[f"route_part{index}_units"] == 0 for index in range(5))
    assert len(materials) == 2 and {row["role"] for row in materials} == {0, 1}
    # Raw queues still retain ongoing finite extraction. Neither RFMT payment
    # nor historical recipe debits create another copy of unused raw inventory.
    for row in materials:
        index = row["role"]
        assert row["reserve"] + row["extracted"] == 32000
        assert row["extracted"] == row["dispatched"] + row["buffer"]
        assert row["extracted"] == row["buffer"] + row["waiting"] + row["unused"] + row["consumed"]
        assert row["delivered"] == row["unused"] + row["consumed"]
        assert row["consumed"] == point["rfmt"] // 2 * (4 if index == 0 else 1)
        assert row["unused"] == (0 if index == 0 else point["unused_coal"])
        assert snapshot[f"reserve{index}"] == row["reserve"] and snapshot[f"mine{index}_extracted"] == row["extracted"]
    assert len(scenes) == 5 and {row["scene"] for row in scenes} == set(range(5))
    assert len(returned) == 3 and {row["role"] for row in returned} == set(range(3))
    # Real positive native holds/layers precede final whole empty homes;
    # initial storage cannot satisfy any useful return or modal transfer.
    for row in scenes:
        assert row["cargo"] > 0 and point["tick"] < row["tick"] <= result["tick"] - 148
        assert row["layer"] == (0, 1, 1, 1, 0)[row["scene"]]
    for row in returned:
        assert max(scene["tick"] for scene in scenes) < row["tick"] <= result["tick"] - 148


def main():
    """Load exact opposite paid originals and earn useful native freight while both finite sources continue.

    @return None; native failures or altered original identities/material/account/power owners prevent a useful report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--prepare-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / PREPARE_RECEIPT
    assert identity(receipt)["sha256"] == PREPARE_SHA256
    accepted = json.loads(receipt.read_text(encoding="utf-8"))
    selected = identity(args.prepare_report)
    assert selected in [row["report"] for row in accepted["records"] if row.get("focus") == "route-prepare"]
    prepared = json.loads(args.prepare_report.read_text(encoding="utf-8"))
    assert prepared["configuration"] != args.config
    original = prepared["native"]["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    environment = original_route_environment(prepared, accepted)
    environment["OTTD_FORGE_ROUTE_INPUT"] = str(source)
    context = current_run_context("tubes", "forge-route")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FDC; actual original Forge useful freight.\nforge_useful_check\n", encoding="utf-8")
    output, proof = native(context, args.config, "[.tube-forge-useful-native]", "forge-useful", environment)
    names = ("year", "original_tick", "tick", "route_ticks", "rfmt", "delivered", "receiver", "source_buffer", "waiting", "income", "total_income", "cash", "loan", "consumed", "packets", "quarters")
    pattern = "FORGE_USEFUL " + " ".join(name + r"=(\d+)" for name in names) + r" scenes=5 returns=3 empty_ticks=148 snapshot=([^\s]+) path=([^\r\n]+)"
    match = re.search(pattern, output)
    assert match is not None
    result = dict(zip(names, map(int, match.groups()[:len(names)])))
    snapshot = parse_snapshot(match[len(names) + 1])
    scenes = [dict(zip(("scene", "tick", "cargo", "tile", "layer"), map(int, row))) for row in re.findall(r"FORGE_ROUTE_SCENE scene=(\d+) tick=(\d+) cargo=(\d+) tile=(\d+) layer=(\d+)", output)]
    returned = [dict(zip(("role", "tick"), map(int, row))) for row in re.findall(r"FORGE_ROUTE_RETURN role=(\d+) tick=(\d+)", output)]
    raw_names = ("role", "reserve", "extracted", "dispatched", "buffer", "waiting", "delivered", "unused", "consumed")
    raw_pattern = "FORGE_USEFUL_RAW " + " ".join(name + r"=(\d+)" for name in raw_names)
    materials = [dict(zip(raw_names, map(int, row))) for row in re.findall(raw_pattern, output)]
    validate_result(result, materials, scenes, returned, snapshot, prepared["prepared"])
    saved = Path(match[len(names) + 2])
    assert len(world_chunks(saved)) == 126 and identity(source) == original
    proof.update(input=original, output=identity(saved))
    report = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FDC; original genuine RFMT useful Forge modal freight/whole empty returns with continuing finite sources",
        configuration=args.config, focus="forge-useful", native=proof, result=result, materials=materials, scenes=scenes, returned=returned, snapshot=snapshot,
        prepare_report=selected, prepare_receipt=identity(receipt), controller=identity(__file__),
        binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES],
        fixture_classification="Exact original paid native world carries actual manufactured RFMT through conventional Surface/UG feed, local model13 Forge and conventional Surface ELEC intake/payment. Both original raw keepers stay stopped while full common native clocks/fronts/source/recipe work and earned service retain finite reserves/queues, unused Coal, exact batches and original packet provenance. Native current/old quarters conserve receipts; actual paid service/binding retains energy/invoice owners. Three whole empty returns and148 further complete opportunities forbid replayed income/intake. RARE absent means no Electronics manufacture. Prepared floor/new receiver completion/Connected shafts remain disclosed boundaries. No producer freeze, quantity/rating/pose/order/clock/account repair, profit or paid TBM/bootstrap claim.",
    )
    path = context.output("reports", f"forge-useful-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
