"""Retain actual original Shield modal worlds and untouched complete native next opportunities.

@file shield_useful_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15EDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The borrowed writer reconstructs no machinery, packets, source stock, orders or accounts.
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
from shield_useful_checks import PREPARE_RECEIPT, PREPARE_SHA256, SOURCES as ROUTE_SOURCES, original_route_environment

PRODUCER_RECEIPT = "logs/tubes/shield-route/20261018-native-01/verification.json"
PRODUCER_SHA256 = "a086fcac59c4d09ef53a16d99633b57cc7704db7cd309481ac4c195778e0034f"
SOURCES = (*ROUTE_SOURCES, "src/tests/shield_useful_world_snapshot.h", "src/tests/tube_useful_tick_fixture.h",
           "src/tests/layer_save_fixture.h", "src/tests/multilayer/shield_useful_checks.py", "src/tests/multilayer/shield_useful_files.py",
           "src/saveload/vehicle_sl.cpp", "src/saveload/industry_sl.cpp", "src/saveload/company_sl.cpp", "src/saveload/cargopacket_sl.cpp")


def validate_points(points, prepared, useful):
    """Require six original real modal/final worlds with exactly their next full native opportunities.

    @param points Native writer observations and immutable full-world identities.
    @param prepared Accepted original paid ready-world participants and initial finite-source facts.
    @param useful Opaque accepted actual useful route metrics, without refreshing earlier nested binaries.
    @return None; altered participants, lost reserve/stock/packet/history or invented scene quantities refuse acceptance.
    """
    assert len(points) == 12
    grid = {(point["scene"], point["step"]): point for point in points}
    assert grid.keys() == {(scene, step) for scene in range(6) for step in range(2)}
    rare_label, refined_label = (int.from_bytes(label, "big") for label in (b"RARE", b"RFMT"))
    # All original/new parts, both industries and four real stations remain
    # present. Native reserve/queue/hold conservation is checked independently
    # of matching complete files; history never becomes another cargo store.
    for point in points:
        values = point["snapshot"]
        rare = next(int(key[5:-6]) for key, value in values.items() if key.startswith("cargo") and key.endswith("_label") and value == rare_label)
        refined = next(int(key[5:-6]) for key, value in values.items() if key.startswith("cargo") and key.endswith("_label") and value == refined_label)
        assert point["parts"] == prepared["parts"]
        assert point["references"] == prepared["industries"] + prepared["stations"] + [prepared["power"], values["deposit"]]
        assert point["tick"] == values["tick"] and values["calendar_year"] == 1985
        assert (values["vehicles"], values["industries"], values["stations"]) == (5, 2, 4)
        assert values["loan"] == 0 and values["part2_layer"] == 1
        assert [values[f"part{part}_id"] for part in range(5)] == prepared["parts"]
        assert values["source"] == values["binding_source"] == prepared["power"]
        physical = sum(values[f"part{part}_units"] for part in range(5))
        physical += sum(values[f"station{station}_cargo{rare}_units"] for station in range(4))
        receiver = None
        for industry in range(2):
            for kind, slots in (("input", "inputs"), ("output", "outputs")):
                for slot in range(values[f"industry{industry}_{slots}"]):
                    prefix = f"industry{industry}_{kind}{slot}_"
                    cargo, units = values[prefix + "cargo"], values[prefix + "waiting"]
                    if cargo == rare:
                        physical += units
                    # Only the original final receiver can earn useful intake.
                    # Its absent RFMT input prevents manufacturing Electronics.
                    if industry == 1 and kind == "input":
                        if cargo == rare:
                            receiver = units
                        if cargo == refined:
                            assert units == 0
                    if industry == 1 and kind == "output":
                        assert units == 0
        assert receiver == values["total_delivered"]
        assert values["reserve"] + physical == prepared["reserve"] + prepared["stock"]
        assert values["mine_extracted"] == prepared["reserve"] + prepared["stock"] - values["reserve"]
        assert values["mine_extracted"] == values["mine_dispatched"] + values["industry0_output0_waiting"]
        assert values[f"industry1_consumed{rare_label}"] == 0 and values["industry1_dispatched"] == 0
        packets = [key[:-6] for key in values if key.startswith("packet") and key.endswith("_units")]
        assert len(packets) == values["packets"]
        for prefix in packets:
            assert values[prefix + "_source"] == prepared["industries"][0]
            assert values[prefix + "_station"] == prepared["stations"][0] and values[prefix + "_purpose"] == 0
    # Each baseline follows one shared original opportunity. Real partial
    # loads remain observed quantities; final queues remain genuine production.
    for scene in range(6):
        original, baseline = grid[scene, 0], grid[scene, 1]
        assert baseline["tick"] == original["tick"] + 1
        assert original["parts"] == baseline["parts"] and original["references"] == baseline["references"]
        values = original["snapshot"]
        if scene < 5:
            part = (1, 1, 2, 4, 4)[scene]
            # Stable native layer IDs retain Space2 and DeepUnderground3;
            # actual Deep freight cannot be validated as an orbital scene.
            assert values[f"part{part}_units"] > 0 and values[f"part{part}_layer"] == (3, 1, 1, 1, 0)[scene]
            if scene == 2:
                assert values["part2_speed"] > 0
        else:
            assert values["tick"] == useful["tick"] and values["total_delivered"] == useful["delivered"]
            assert values["total_income"] == useful["income"] and values["cash"] == useful["cash"]
            assert values["consumed"] == useful["consumed"] and values["reserve"] == useful["reserve"]
            assert values["mine_extracted"] == useful["extracted"] and values["mine_dispatched"] == useful["dispatched"]
            assert values["packets"] == useful["packets"] and values["quarters"] == useful["quarter_history"]
            assert all(values[f"part{part}_units"] == 0 for part in range(5))


def main():
    """Write the exact opposite ready world through the actual full route and retain original/native-next boundaries.

    @return None; changed originals, impure saves or unequal accepted useful final worlds prevent a writer report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--prepare-report", type=Path, required=True)
    args = parser.parse_args()
    receipt, producer = ROOT / PREPARE_RECEIPT, ROOT / PRODUCER_RECEIPT
    assert identity(receipt)["sha256"] == PREPARE_SHA256
    assert identity(producer)["sha256"] == PRODUCER_SHA256
    accepted = json.loads(receipt.read_text(encoding="utf-8"))
    history = json.loads(producer.read_text(encoding="utf-8"))
    selected = identity(args.prepare_report)
    assert selected in [row["report"] for row in accepted["records"] if row.get("focus") == "route-prepare"]
    prepared = json.loads(args.prepare_report.read_text(encoding="utf-8"))
    assert prepared["configuration"] != args.config
    original = prepared["native"]["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    context = current_run_context("tubes", "shield-files")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15EDB; original Shield pure boundary writer.\nshield_useful_files_check\n", encoding="utf-8")
    environment = original_route_environment(prepared, accepted)
    environment["OTTD_SHIELD_ROUTE_INPUT"] = str(source)
    output, proof = native(context, args.config, "[.tube-shield-useful-files-native]", "writer", environment)
    pattern = r"SHIELD_WORLD scene=(\d+) step=(\d+) tick=(\d+) parts=([\d,]+) references=([\d,]+) snapshot=([^\s]+) path=([^\r\n]+)"
    points = []
    # Strict canonical decoding detects repeated/missing native field names;
    # every full original file independently retains all126 serialized chunks.
    for scene, step, tick, parts, references, snapshot, name in re.findall(pattern, output):
        values = parse_snapshot(snapshot)
        saved = Path(name)
        assert len(world_chunks(saved)) == 126
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
        "_documentation": "r2mlvs17-underground-transport-tubes.md15EDB; exact original useful Shield writer/untouched full-native next opportunities; readers/raster remain separate",
        "configuration": args.config, "phase": "write", "native": proof, "checkpoints": points,
        "prepare_report": selected, "prepare_receipt": identity(receipt), "producer_receipt": identity(producer),
        "historical_useful_output": historical, "historical_final_complete_chunks_exact": 126,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Exact accepted original five-part/two-industry/four-station paid ready world is loaded normally. Real full native clocks/fronts/industry work, continuing finite extraction, producer dispatch and earned service/orders/handling earn five actual loaded modal states, useful24RARE intake/payment and three whole empty returns. Six pure original saves plus each untouched next opportunity preserve all reserve/stock/packet/source/recipe/native quarter/power owners and both RNGs. Genuine final queues remain, with RFMT absent and Electronics output0. Prepared floor/machinery/Connected shafts remain boundaries. No reconstruction, source freeze, packet/rating/pose/order/clock/account repair, reader/raster or TBM/bootstrap claim.",
    }
    path = context.output("reports", f"shield-useful-files-{args.config}-write.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
