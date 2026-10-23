"""Drive the exact paid replacement stock through original freight and actual native construction.

@file first_plant_replacement_delivery.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CCCC/operating1.3B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Ordinary complete game frames earn cargo movement/work; repeated complete worlds and untouched next opportunities remain original evidence.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native
from geothermal_tube_binding_files import world_chunks
from tube_sprite_report import verify_references, compare_worlds
from mole_useful_raster_report import count_native
from shield_source_files import parse_snapshot

STOCK = "logs/geothermal/replacement-stock/20261018-native-01/verification.json"
STOCK_SHA = "5423e9ff985f15be4051c35918f4d61b05ebcf114260530004f51a38309ecc4c"
SOURCES = ("src/tests/geothermal_first_plant_replacement_delivery.cpp", "src/tests/plant_rebuild_delivery_fixture.h",
           "src/tests/plant_rebuild_material_fixture.h", "src/tests/multilayer/utilities/first_plant_replacement_delivery.py",
           "src/surface_supply.cpp", "src/geothermal_collector.cpp", "src/geothermal_generator.cpp", "src/industry_cmd.cpp",
           "src/train_cmd.cpp", "src/cargopacket.cpp", "src/openttd.cpp")
CASES = ("Paid Surface collector bill arrives by native ordered conventional train",
         "Paid Surface riser bill arrives by native ordered conventional train",
         "Native powerhouse waits for its whole bill before592 prepaid work ticks")
BILLS = ((8, 4, 2, 0), (12, 8, 4, 2), (24, 16, 8, 4))
WORK = (296, 592, 888)


def observed_tick(point):
    """Read the actual native observation clock for chronological work validation.

    @param point Original native scene/step with its canonical observed snapshot.
    @return Actual simulation tick, independent of semantic scene numbering.
    """
    return point["snapshot"]["tick"]


def validate_points(points, stock):
    """Require actual three-layer carriage, every genuine new construction receipt and pure full-native boundaries.

    @param points Exact original input and nine original/untouched-next scene pairs, including repeated full native files.
    @param stock Exact accepted opposite-writer all-paid original input checkpoint.
    @return Unique scene/step grid; missing loaded/work/home states, lost material/history or any pure/next mismatch refuses acceptance.
    """
    grid = {(point["scene"], point["step"]): point for point in points}
    expected = {(-1, 0)} | {(scene, step) for scene in range(9) for step in range(2)}
    assert len(grid) == len(points) == 19 and grid.keys() == expected
    initial = grid[-1, 0]["snapshot"]; paid = stock["snapshot"]
    assert world_chunks(ROOT / grid[-1, 0]["input"]["path"]) == world_chunks(ROOT / stock["input"]["path"])
    for key, value in paid.items():
        assert initial[key] == value, key
    assert initial["whole_empty_home"] == 1 and initial["carrier_parts"] == 5 and initial["aboard"] == 0
    assert initial["power_next_producer"] == 3
    new_orders = set(range(paid["next_order"] - 4, paid["next_order"]))
    # Every complete world/repetition retains genuine saved effects and every
    # other native owner, independent of the explicit relevant-owner scalars.
    for point in points:
        assert compare_worlds(point["input"], point["repeat"]) == 126
        current = point["snapshot"]
        for key in ("invoices", "geology", "origin", "carrier", "industries", "orders", "next_order", "carrier_parts", "loan", "gross_income"):
            assert current[key] == initial[key], (point["scene"], point["step"], key)
        assert current["year"] >= initial["year"] == 2000 and current["tick"] >= initial["tick"]
        for part in range(5):
            for suffix in ("id", "first", "engine", "owner", "cargo", "capacity"):
                assert current[f"part{part}_{suffix}"] == initial[f"part{part}_{suffix}"]
        # The three old source meters remain terminal with identical lifetime
        # use/generation. Naturally earned fresh sources keep new native IDs.
        for producer in range(3):
            for suffix in ("active", "generated", "consumed", "reserved", "domain"):
                assert current[f"producer{producer}_{suffix}"] == initial[f"producer{producer}_{suffix}"]
            assert current[f"producer{producer}_active"] == current[f"producer{producer}_reserved"] == 0
        for machine, ticks in enumerate(WORK):
            assert current[f"industry{machine}"] == initial[f"industry{machine}"]
            assert current[f"machine{machine}_site"] == initial[f"machine{machine}_site"]
            assert 0 <= current[f"work{machine}"] <= ticks and current[f"committed{machine}"] in (0, 1)
            if current[f"work{machine}"] > 0:
                assert current[f"committed{machine}"] == 1
        # Native whole-bill commitment replaces physical stock once. All old
        # retired92 and retained-riser24 history remains in the same equation.
        for material, retired in enumerate((44, 28, 14, 6)):
            consumed = (8, 8, 4, 4)[material] + sum(BILLS[machine][material] * current[f"committed{machine}"] for machine in range(3))
            assert current[f"bought{material}"] == initial[f"bought{material}"]
            assert current[f"retired{material}"] == retired and current[f"live_consumed{material}"] == consumed
            assert current[f"bought{material}"] == current[f"physical{material}"] + retired + consumed
            assert 0 <= current[f"source{material}"] <= current[f"physical{material}"] <= retired
            assert current[f"dispatched{material}"] <= current[f"bought{material}"]
        for key, value in current.items():
            if re.fullmatch(r"packet\d+_order", key):
                assert value in new_orders
    for scene in range(9):
        assert grid[scene, 1]["snapshot"]["tick"] == grid[scene, 0]["snapshot"]["tick"] + 1
    ready = grid[0, 0]["snapshot"]; assert ready["tick"] == initial["tick"] and ready["cash"] == initial["cash"]
    # Positive-speed real wagon cargo on each explicit plane proves both
    # conventional crossings; neither shared catchment nor a sign can pass.
    for scene, layer in ((1, 0), (2, 1), (3, 3)):
        current = grid[scene, 0]["snapshot"]
        assert current["aboard"] > 0 and current["part0_layer"] == layer and current["part0_speed"] > 0
        assert any(current[f"part{part}_layer"] == layer and current[f"part{part}_units"] > 0 for part in range(5))
    assert grid[1, 0]["snapshot"]["tick"] < grid[2, 0]["snapshot"]["tick"] < grid[3, 0]["snapshot"]["tick"]
    for machine, ticks in enumerate(WORK):
        assert 0 < grid[machine + 4, 0]["snapshot"][f"work{machine}"] < ticks
    for scene in (7, 8):
        current = grid[scene, 0]["snapshot"]
        assert all(current[f"work{machine}"] == WORK[machine] and current[f"committed{machine}"] == 1 for machine in range(3))
    final = grid[8, 0]["snapshot"]; assert final["whole_empty_home"] == 1 and final["aboard"] == 0
    for material in range(4):
        assert final[f"source{material}"] == final[f"physical{material}"] == 0 and final[f"dispatched{material}"] == final[f"bought{material}"]
    assert not any(re.fullmatch(r"packet\d+_order", key) for key in final)
    # Native work cannot decrease between real observed ticks, regardless of
    # which model first received a whole bill or which label captured it.
    previous = initial
    for point in sorted(points, key=observed_tick):
        for machine in range(3):
            assert point["snapshot"][f"work{machine}"] >= previous[f"work{machine}"]
        previous = point["snapshot"]
    return grid


def drive(configuration, stock_report):
    """Earn actual native carriage/labour on the exact opposite accepted paid-stock original.

    @param configuration Actual current Debug or optimized test executable.
    @param stock_report Exact sealed opposite current paid replacement stock report.
    @return None; print a native original-route report only after every original/pure/movement/work/home obligation passes.
    """
    receipt = ROOT / STOCK; assert identity(receipt)["sha256"] == STOCK_SHA
    history = json.loads(receipt.read_text(encoding="utf-8")); selected = identity(stock_report)
    record = next(row for row in history["records"] if row["report"] == selected); assert record["configuration"] != configuration
    stock = next(point for point in record["checkpoints"] if point["stage"] == 1)
    source = ROOT / stock["input"]["path"]; assert identity(source) == stock["input"]
    context = current_run_context("geothermal", "replacement-delivery"); context.prepare_runtime(); processes = []
    # Existing actual loading/purpose/labour producers run separately before
    # dependent full-world mutation; no fixture state leaks into this original.
    for index, case in enumerate(CASES):
        _, proof = native(context, configuration, case, f"producer-{index}"); processes.append({"case": case, **proof})
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md1CCCC; actual original paid replacement travel/work.\nplant_replacement_delivery\n", encoding="utf-8")
    output, proof = native(context, configuration, "[.geothermal-first-plant-replacement-delivery]", "original-drive", {"OTTD_PLANT_REPLACEMENT_INPUT": str(source)})
    points = []
    # Every observation is native; fields never become expected values written
    # back into a carrier, receiver, clock, rating, company or utility owner.
    for scene, step, text, path, repeat in re.findall(r"PLANT_DELIVERY_WORLD scene=(-?\d+) step=(\d+) snapshot=([^\s]+) path=([^\r\n]+) repeat=([^\r\n]+)", output):
        snapshot = parse_snapshot(text); assert len(snapshot) == text.count("=")
        points.append({"scene": int(scene), "step": int(step), "snapshot": snapshot, "input": identity(path), "repeat": identity(repeat)})
    grid = validate_points(points, stock); assert identity(source) == stock["input"]
    complete = re.search(r"PLANT_DELIVERY_COMPLETE original=(\d+) final=(\d+) frames=(\d+) whole_empty=true models=3 original_units=92 no_freight_income=true", output); assert complete is not None
    start, end, frames = map(int, complete.groups()); assert start == grid[0, 0]["snapshot"]["tick"] and end == grid[8, 0]["snapshot"]["tick"] > start
    proof.update(input=stock["input"], output=grid[8, 0]["input"], untouched_final_next=grid[8, 1]["input"])
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CCCC; actual same original92 paid replacement carriage/native labour/whole return and full next boundaries",
              "configuration": configuration, "focus": "drive", "native": processes + [proof], "checkpoints": points,
              "result": {"start_tick": start, "final_tick": end, "native_ticks": end - start, "presentation_frames": frames, "fresh_bills": 3, "replacement_units": 92},
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES],
              "stock_receipt": identity(receipt), "stock_report": selected, "stock_checkpoint": stock,
              "fixture_classification": "Exact original all-paid stock world, original three-stop train and both actual conventional layer crossings. Complete ordinary native game/input/window frames earn loaded Surface/Underground/Deep travel, all three new whole bills and296/592/888 labour, then whole empty Surface depot return with zero freight income. Native retired92 and retained-riser24 histories remain; fresh92 are physically consumed once. Pure repeated complete worlds preserve genuine effects, all relevant native owners and independent client RNG. Each original has its untouched full next opportunity before further observer commands. Naturally earned fresh collector Heat may publish; actual fresh useful generator service/physical reconnection and ordinary reader acceptance remain1CCD/1CD."}
    path = context.output("reports", f"first-plant-replacement-delivery-{configuration}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def accept(reports, build_logs, output):
    """Accept both independent actual original route writers and all complete native boundaries.

    @param reports Two current exact-opposite-stock native delivery reports.
    @param build_logs Successful current Debug/optimized native test target diagnostics.
    @param output Absent immutable actual delivery/construction receipt under repository logs.
    @return None; stale originals, unequal whole worlds or missing carriage/work/home proof prevent aggregate closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, records = {}, {}; summary = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    # Current code/binaries are checked freshly; old exact paid-stock reports
    # remain opaque history rather than claims about historical binary freshness.
    for name in reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in summary and cfg not in records and report["focus"] == "drive"
        validate_points(report["checkpoints"], report["stock_checkpoint"])
        for proof in report["native"]:
            count_native(proof, summary[cfg])
        records[cfg] = {"report": identity(path), **report}
    assert records.keys() == summary.keys() and summary["Debug"] == summary["RelWithDebInfo"]
    for key in ("start_tick", "final_tick", "native_ticks", "fresh_bills", "replacement_units"):
        assert records["Debug"]["result"][key] == records["RelWithDebInfo"]["result"][key]
    left = {(row["scene"], row["step"]): row for row in records["Debug"]["checkpoints"]}
    right = {(row["scene"], row["step"]): row for row in records["RelWithDebInfo"]["checkpoints"]}; assert left.keys() == right.keys()
    pairs = []
    # Whole worlds compare every original clock/packet/position/receipt/claim,
    # old and naturally created source and genuine effect; no chunk exclusions.
    for key in sorted(left):
        first, second = left[key], right[key]; assert first["snapshot"] == second["snapshot"]
        assert compare_worlds(first["input"], second["input"]) == compare_worlds(first["repeat"], second["repeat"]) == 126
        pairs.append({"scene": key[0], "step": key[1], "left": first["input"], "right": second["input"], "complete_chunks_exact": 126})
    builds = []
    # Actual compile/link diagnostics retain the targets that produced these
    # normal-frame originals; source declarations alone cannot authorize them.
    for name in build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CCCC/original1CCC; actual paid replacement movement/work/whole return acceptance",
              "controller": identity(__file__), "records": list(records.values()), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "complete_cross_build_world_pairs": pairs, "pure_repeat_pairs": 38, "stock_receipt": identity(ROOT / STOCK), "builds": builds,
              "native_version": 434, "exact_identities_verified": len(cache),
              "remaining": "1CCD actual physical source reconnection/fresh useful service and1CD ordinary retired/interrupted/rebuilt reader acceptance must pass before1CC/1.3B/original1.3. Master10.7/runtime-art/9.5B/later TBM-to-tube/human/operator/network gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(pairs)}), flush=True)


def main():
    """Select one bounded real native delivery run or its exact independent two-build acceptance.

    @return None; incomplete arguments or failed actual owner/world evidence stop before acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("drive", "accept"), required=True)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--stock-report", type=Path)
    parser.add_argument("--reports", nargs=2); parser.add_argument("--build-logs", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "drive":
        assert args.config is not None and args.stock_report is not None; drive(args.config, args.stock_report)
    else:
        assert args.reports is not None and args.build_logs is not None and args.output is not None
        accept(args.reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
