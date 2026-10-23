"""Retain actual original Forge modal worlds and untouched complete native next opportunities.

@file forge_useful_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FEB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The borrowed writer reconstructs no source machinery, native packets, stock, orders, accounts or power owners.
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
from geothermal_tube_work_files import compare_chunks
from shield_source_files import parse_snapshot
from forge_useful_checks import PREPARE_RECEIPT, PREPARE_SHA256, SOURCES as ROUTE_SOURCES, original_route_environment

PRODUCER_RECEIPT = "logs/tubes/forge-route/20261018-native-01/verification.json"
PRODUCER_SHA256 = "351772ad22fc7af32fcdb8414375deef15f0c7cd6618696aedbd428336cfd577"
SOURCES = (*ROUTE_SOURCES, "src/tests/forge_useful_world_snapshot.h", "src/tests/tube_useful_tick_fixture.h",
           "src/tests/layer_save_fixture.h", "src/tests/multilayer/forge_useful_files.py", "src/source_type.h",
           "src/effectvehicle_base.h", "src/saveload/vehicle_sl.cpp", "src/saveload/industry_sl.cpp", "src/saveload/company_sl.cpp", "src/saveload/cargopacket_sl.cpp")


def industry_stock(values, industry, cargo, kind):
    """Read one actual physical native industry's matching input/output slots without using history as inventory.

    @param values Canonical native owner observations.
    @param industry Actual original industry role0..3.
    @param cargo Actual runtime meaning retained by native loading.
    @param kind Input or output slot family.
    @return Whole stock of that meaning in the actual native slot family.
    """
    count = values[f"industry{industry}_{'inputs' if kind == 'input' else 'outputs'}"]
    return sum(values[f"industry{industry}_{kind}{slot}_waiting"] for slot in range(count)
               if values[f"industry{industry}_{kind}{slot}_cargo"] == cargo)


def validate_points(points, prepared, useful):
    """Require six genuine original modal/final worlds with exactly their next complete native opportunities.

    @param points Native writer observations and immutable full-world identities.
    @param prepared Exact accepted paid ready report with actual original/raw/new participant roles.
    @param useful Opaque accepted useful route report and real first-scene/material/account facts.
    @return None; altered roles, lost stock/history or invented loaded scenes prevent acceptance.
    """
    assert len(points) == 12
    grid = {(point["scene"], point["step"]): point for point in points}
    assert grid.keys() == {(scene, step) for scene in range(6) for step in range(2)}
    ready, original = prepared["prepared"], prepared["source_checkpoint"]["snapshot"]
    parts = [original[f"part{index}_id"] for index in range(4)] + ready["new_parts"]
    industries = [original[f"industry{index}_id"] for index in range(3)] + [ready["receiver"]]
    stations = [original[f"raw_station{index}_id"] for index in range(3)] + ready["stations"][2:]
    references = industries + stations + [ready["power"], original["deposit0"], original["deposit1"]]
    labels = [int.from_bytes(label, "big") for label in (b"CORE", b"COAL", b"RFMT")]
    # Every native slot/queue/hold has one physical owner. Recipe history and
    # company receipts independently reconcile those units without becoming a
    # second store; each finite raw reserve retains its own exact equation.
    for point in points:
        values = point["snapshot"]
        cargos = [values["part1_cargo"], values["part3_cargo"], values["part6_cargo"]]
        assert point["tick"] == values["tick"] and values["calendar_year"] == 2005
        assert (values["train_vehicles"], values["industries"], values["stations"]) == (9, 4, 5)
        assert values["vehicles"] == values["train_vehicles"] + values["effect_vehicles"]
        assert len([key for key in values if key.startswith("effect") and key.endswith("_id")]) == values["effect_vehicles"]
        assert point["parts"] == parts and point["references"] == references
        assert [values[f"part{index}_id"] for index in range(9)] == parts
        assert values["source"] == values["binding_source"] == ready["power"] and values["loan"] == 0
        assert values["part6_capacity"] == 80 and values["part6_layer"] == 1
        assert values[f"industry2_batch{int.from_bytes(b'CFMT', 'big')}"] == ready["rfmt"] // 2
        for index, cargo in enumerate(cargos):
            physical = sum(values[f"part{part}_units"] for part in range(9) if values[f"part{part}_cargo"] == cargo)
            physical += sum(values[f"station{station}_cargo{cargo}_units"] for station in stations)
            physical += sum(industry_stock(values, industry, cargo, kind) for industry in range(4) for kind in ("input", "output"))
            if index < 2:
                consumed = values[f"industry2_consumed{labels[index]}"]
                assert values[f"reserve{index}"] + physical + consumed == values[f"initial_reserve{index}"] == 32000
                assert values[f"mine{index}_extracted"] == 32000 - values[f"reserve{index}"]
                assert values[f"mine{index}_extracted"] == values[f"mine{index}_dispatched"] + values[f"industry{index}_output0_waiting"]
                assert values[f"total_delivered{index}"] == industry_stock(values, 2, cargo, "input") + consumed
                assert consumed == ready["rfmt"] // 2 * (4 if index == 0 else 1)
                assert industry_stock(values, 2, cargo, "input") == (0 if index == 0 else ready["unused_coal"])
            else:
                assert physical == values[f"industry2_made{labels[2]}"] == ready["rfmt"]
                assert industry_stock(values, 2, cargo, "output") + values["industry2_dispatched"] == ready["rfmt"]
                assert values["total_delivered2"] == industry_stock(values, 3, cargo, "input")
                assert values[f"industry3_consumed{labels[2]}"] == 0
        # Native ELEC has no RARE input, so no manufactured Electronics or
        # consumed RFMT may be hidden behind matching final freight receipts.
        for slot in range(values["industry3_inputs"]):
            if values[f"industry3_input{slot}_cargo"] != cargos[2]:
                assert values[f"industry3_input{slot}_waiting"] == 0
        assert all(values[f"industry3_output{slot}_waiting"] == 0 for slot in range(values["industry3_outputs"]))
        packets = [key[:-6] for key in values if key.startswith("packet") and key.endswith("_units")]
        assert len(packets) == values["packets"]
        for prefix in packets:
            source = values[prefix + "_source"]
            assert source in industries[:3] and values[prefix + "_source_type"] == 0
            assert values[prefix + "_first"] == stations[industries.index(source)] and values[prefix + "_purpose"] == 0
    # Native next baselines follow one actual opportunity. The original first
    # scenes must retain the exact accepted route ticks and observed partial
    # quantities; final source queues/unused inputs remain genuine inventory.
    scenes = {row["scene"]: row for row in useful["scenes"]}
    for scene in range(6):
        point, baseline = grid[scene, 0], grid[scene, 1]
        assert baseline["tick"] == point["tick"] + 1
        values = point["snapshot"]
        if scene < 5:
            part = (5, 5, 6, 8, 8)[scene]
            assert values[f"part{part}_units"] == scenes[scene]["cargo"] > 0
            assert values[f"part{part}_layer"] == (0, 1, 1, 1, 0)[scene] and values["tick"] == scenes[scene]["tick"]
            if scene == 2:
                assert values["part6_speed"] > 0
        else:
            result = useful["result"]
            assert values["tick"] == result["tick"] and values["total_delivered2"] == result["delivered"]
            assert values["total_income"] == result["total_income"] and values["cash"] == result["cash"] and values["consumed"] == result["consumed"]
            assert values["packets"] == result["packets"] and values["quarters"] == result["quarters"]
            assert all(values[f"part{part}_units"] == 0 for part in range(9))
            for row in useful["materials"]:
                index = row["role"]
                assert values[f"reserve{index}"] == row["reserve"] and values[f"mine{index}_extracted"] == row["extracted"] and values[f"mine{index}_dispatched"] == row["dispatched"]


def main():
    """Run the exact opposite ready original through the actual route while retaining pure loaded/final native worlds.

    @return None; altered originals, impure saves or changed complete useful final state prevent a writer report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--prepare-report", type=Path, required=True)
    args = parser.parse_args()
    receipt, producer = ROOT / PREPARE_RECEIPT, ROOT / PRODUCER_RECEIPT
    assert identity(receipt)["sha256"] == PREPARE_SHA256 and identity(producer)["sha256"] == PRODUCER_SHA256
    accepted, history = (json.loads(path.read_text(encoding="utf-8")) for path in (receipt, producer))
    selected = identity(args.prepare_report)
    assert selected in [row["report"] for row in accepted["records"] if row.get("focus") == "route-prepare"]
    prepared = json.loads(args.prepare_report.read_text(encoding="utf-8"))
    assert prepared["configuration"] != args.config
    original = prepared["native"]["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    useful = next(row for row in history["records"] if row["configuration"] == args.config and row.get("focus") == "forge-useful")
    context = current_run_context("tubes", "forge-files")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15FEB; original Forge pure boundary writer.\nforge_useful_files_check\n", encoding="utf-8")
    environment = original_route_environment(prepared, accepted)
    environment["OTTD_FORGE_ROUTE_INPUT"] = str(source)
    output, proof = native(context, args.config, "[.tube-forge-useful-files-native]", "writer", environment)
    points = []
    # Strict canonical decoding rejects repeated/missing native field names;
    # complete original chunks independently retain every actual saved owner.
    for scene, step, tick, snapshot, name in re.findall(r"FORGE_WORLD scene=(\d+) step=(\d+) tick=(\d+) snapshot=([^\s]+) path=([^\r\n]+)", output):
        values = parse_snapshot(snapshot)
        saved = Path(name)
        assert len(world_chunks(saved)) == 126
        parts = [values[f"part{index}_id"] for index in range(9)]
        references = [values[f"industry{index}_id"] for index in range(4)] + [values[f"station{index}_id"] for index in range(5)] + [values["source"], values["deposit0"], values["deposit1"]]
        points.append(dict(scene=int(scene), step=int(step), tick=int(tick), parts=parts, references=references, snapshot=values, input=identity(saved)))
    validate_points(points, prepared, useful)
    final = next(point for point in points if (point["scene"], point["step"]) == (5, 0))
    historical = useful["native"]["output"]
    assert identity(ROOT / historical["path"]) == historical and compare_chunks(ROOT / final["input"]["path"], ROOT / historical["path"], None) == 126
    assert identity(source) == original and "scenes=5 returns=3 empty_ticks=148" in output
    proof.update(input=original)
    report = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FEB; actual original useful Forge writer/untouched complete next opportunities; ordinary readers/raster remain separate",
        configuration=args.config, phase="write", native=proof, checkpoints=points,
        prepare_report=selected, prepare_receipt=identity(receipt), producer_receipt=identity(producer), historical_useful_output=historical,
        historical_final_complete_chunks_exact=126, controller=identity(__file__), binary=identity(ROOT / f"build/{args.config}/openttd_test.exe"), sources=[identity(ROOT / name) for name in SOURCES],
        fixture_classification="Exact original nine-part/four-industry/five-station paid ready world is loaded normally. The same complete native clocks/fronts/source/recipe driver earns five real loaded modal states,32 genuine RFMT intake/payment and three whole empty returns. Six pure original saves and each untouched next opportunity retain both finite sources, unused Coal, exact CFMT/ELEC histories, all real native queues/packets/source IDs/aging/purpose, all native quarters and paid electrical owners. Native simulation RNG is canonical and unsaved client RNG independently pure. Prepared floor/machinery/Connected shafts remain boundaries. No reconstruction, producer freeze, quantity/rating/pose/order/clock/account repair, reader/raster or paid TBM/bootstrap claim.",
    )
    path = context.output("reports", f"forge-useful-files-{args.config}-write.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
