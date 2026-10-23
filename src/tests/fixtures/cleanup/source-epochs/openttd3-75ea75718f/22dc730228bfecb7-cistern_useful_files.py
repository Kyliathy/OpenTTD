"""Retain actual original1980 Cistern modal worlds and untouched next full-native opportunities.

@file cistern_useful_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Normal application loading acquires all original source/route owners; this writer reconstructs no participant.
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
from cistern_useful_checks import PREPARE_RECEIPT, SOURCES as ROUTE_SOURCES, original_route_environment

PRODUCER_RECEIPT = "logs/tubes/cistern-route/20261018-native-01/verification.json"
SOURCES = (*ROUTE_SOURCES, "src/tests/cistern_useful_world_snapshot.h", "src/tests/tube_useful_tick_fixture.h",
           "src/tests/layer_save_fixture.h", "src/tests/CMakeLists.txt", "src/tests/multilayer/cistern_useful_checks.py",
           "src/tests/multilayer/cistern_useful_files.py", "src/saveload/vehicle_sl.cpp", "src/saveload/industry_sl.cpp",
           "src/saveload/company_sl.cpp", "src/saveload/cargopacket_sl.cpp")


def validate_points(points, prepared, useful):
    """Require six original loaded/final states and precisely their next shared native opportunities.

    @param points Actual native writer observations and complete file identities.
    @param prepared Accepted original paid ready-world participants and original scalar facts.
    @param useful Opaque accepted original useful route metrics, without refreshing its dated nested sources/binaries.
    @return None; changed participants, lost raw/recipe/account owners or invented scene quantities refuse evidence.
    """
    assert len(points) == 12
    grid = {(point["scene"], point["step"]): point for point in points}
    assert grid.keys() == {(scene, step) for scene in range(6) for step in range(2)}
    parts = prepared["original_parts"] + prepared["parts"]
    references = prepared["industries"] + prepared["stations"] + [prepared["power"]]
    oil, salt, chemical, propellant = (int.from_bytes(label, "big") for label in (b"OIL_", b"SALT", b"CHEM", b"PROP"))
    # Every boundary retains all original upstream participants and the exact
    # two real recipe histories. Native split packets may change pool size;
    # their original production meaning and first station cannot change.
    for point in points:
        values = point["snapshot"]
        assert point["parts"] == parts and point["references"][:11] == references and len(point["references"]) == 12
        assert point["tick"] == values["tick"] and values["calendar_year"] == 1980
        assert (values["vehicles"], values["industries"], values["stations"]) == (9, 4, 6)
        assert values["reserve"] == 31980 and values["mine_extracted"] == values["mine_dispatched"] == 20
        assert values["multiple_industry_per_town"] == 1 and values["loan"] == 0
        assert values[f"industry1_consumed{oil}"] == values[f"industry1_consumed{salt}"] == 20
        assert values[f"industry1_made{chemical}"] == 20
        assert values[f"industry3_consumed{chemical}"] == values[f"industry3_made{propellant}"] <= 20
        assert values["industry3_dispatched"] == 0 and values["source"] == values["binding_source"] == prepared["power"]
        assert [values[f"part{part}_id"] for part in range(9)] == parts
        assert all(values[f"part{part}_units"] == 0 for part in range(4))
        assert values["part6_layer"] == 1
    # The baseline is the next actual shared opportunity, not an extra private
    # movement tick taken to stage another scene. Partial native loads remain
    # their real first observations rather than a fixed full-load substitute.
    for scene in range(6):
        original, baseline = grid[scene, 0], grid[scene, 1]
        assert baseline["tick"] == original["tick"] + 1
        assert original["parts"] == baseline["parts"] and original["references"] == baseline["references"]
        values = original["snapshot"]
        if scene < 5:
            part = (5, 5, 6, 8, 8)[scene]
            assert 0 < values[f"part{part}_units"] <= 20 and values[f"part{part}_layer"] == (0, 1, 1, 1, 0)[scene]
            if scene == 2:
                assert values["part6_speed"] > 0
        else:
            assert values["tick"] == useful["tick"] and values["total_delivered"] == 20 and values["packets"] == 0
            assert values["total_income"] == useful["total_income"] and values["cash"] == useful["cash"]
            assert values["consumed"] == useful["consumed"] and values["quarters"] > 0
            propellant_slot = next(slot for slot in range(values["industry3_produced"])
                                   if values[f"cargo{values[f'industry3_output{slot}_cargo']}_label"] == propellant)
            assert values[f"industry3_made{propellant}"] == values[f"industry3_output{propellant_slot}_waiting"] == 20
            assert all(values[f"part{part}_units"] == 0 for part in range(9))


def main():
    """Write exact opposite prepared worlds through the actual useful driver and retain complete native baselines.

    @return None; changed originals, incomplete points or any native save-purity failure prevent a writer report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--prepare-report", type=Path, required=True)
    args = parser.parse_args()
    receipt, producer = ROOT / PREPARE_RECEIPT, ROOT / PRODUCER_RECEIPT
    assert identity(receipt)["sha256"] == "9d3d8bbe1ae9509bfc9555b47f9aaad9b3753bbb8e8280effff8b1b641c49561"
    assert identity(producer)["sha256"] == "63753266d3c0c11297f7402ce5a78c8a040974cd81da6398034d6bc69053d14e"
    accepted = json.loads(receipt.read_text(encoding="utf-8")); history = json.loads(producer.read_text(encoding="utf-8"))
    selected = identity(args.prepare_report); assert selected in [row["report"] for row in accepted["records"]]
    prepared = json.loads(args.prepare_report.read_text(encoding="utf-8")); assert prepared["configuration"] != args.config
    original = prepared["native"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    context = current_run_context("tubes", "cistern-files"); context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15DDB; original useful-world pure writer.\ncistern_useful_files_check\n", encoding="utf-8")
    environment = original_route_environment(prepared, accepted); environment["OTTD_CISTERN_ROUTE_INPUT"] = str(source)
    output, proof = native(context, args.config, "[.tube-cistern-useful-files-native]", "writer", environment)
    pattern = r"CISTERN_WORLD scene=(\d+) step=(\d+) tick=(\d+) parts=([\d,]+) references=([\d,]+) snapshot=([^\s]+) path=([^\r\n]+)"
    points = []
    # Each canonical CSV must round-trip exactly. Repeated/missing field names
    # cannot silently replace another original owner's observation in JSON.
    for scene, step, tick, parts, references, snapshot, name in re.findall(pattern, output):
        values = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+),", snapshot)}
        assert ",".join(f"{key}={value}" for key, value in values.items()) + "," == snapshot
        saved = Path(name); assert len(world_chunks(saved)) == 126
        points.append(dict(scene=int(scene), step=int(step), tick=int(tick), parts=list(map(int, parts.split(","))),
                           references=list(map(int, references.split(","))), snapshot=values, input=identity(saved)))
    validate_points(points, prepared["prepared"], history["result"])
    final = next(point for point in points if (point["scene"], point["step"]) == (5, 0))
    historical = next(row["native"]["output"] for row in history["records"] if row["configuration"] == args.config)
    assert identity(ROOT / historical["path"]) == historical
    assert compare_chunks(ROOT / final["input"]["path"], ROOT / historical["path"], None) == 126
    assert identity(source) == original and "scenes=5 returns=3 empty_ticks=148" in output
    proof.update(input=original)
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DDB; actual ordinary-loaded original useful Cistern writer/untouched next native opportunities; reader/raster remain separate",
        "configuration": args.config, "phase": "write", "native": proof, "checkpoints": points,
        "prepare_report": selected, "prepare_receipt": identity(receipt), "producer_receipt": identity(producer),
        "historical_useful_output": historical, "historical_final_complete_chunks_exact": 126,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Exact original nine-part/four-industry/six-station paid ready world is loaded normally. Real native dispatch/orders/handling and shared clocks/vehicles/power/processor work earn all five loaded modal scenes, useful20PROP and three whole empty returns. Six pure original saves and each untouched next opportunity retain raw histories/reserve/unused Oil, native quarters, packets, original definitions, paid root and both RNGs. Prepared floor/machinery/Connected shafts and saved multi-industry option remain boundaries. No reconstruction, quantity/pose/order/clock/account repair, ordinary reader/raster or paid TBM/bootstrap claim.",
    }
    path = context.output("reports", f"cistern-useful-files-{args.config}-write.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


if __name__ == "__main__":
    main()
