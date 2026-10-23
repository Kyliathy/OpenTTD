"""Reconnect paid original rebuilt machinery and earn both models' fresh useful town service.

@file first_plant_rebuilt_service.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups1CCD/1CD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only ordinary native frames and synchronized paid utility/town commands act on the accepted original world.
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

DELIVERY = "logs/geothermal/replacement-delivery/20261018-native-01/verification.json"
DELIVERY_SHA = "c69b48f271812ea2163de76a041782547fa89f250fc2d03acb7b42a69d213503"
SOURCES = ("src/tests/geothermal_first_plant_rebuilt_service.cpp", "src/tests/plant_rebuild_service_fixture.h",
           "src/tests/plant_rebuild_delivery_fixture.h", "src/tests/plant_rebuild_material_fixture.h",
           "src/tests/multilayer/utilities/first_plant_rebuilt_service.py", "src/geothermal_utility.cpp",
           "src/geothermal_generator.cpp", "src/geothermal_collector.cpp", "src/geothermal_service_cmd.cpp",
           "src/utility_power.cpp", "src/openttd.cpp")
CASES = ("Native powerhouse requires its actual installed boundary ports",
         "Selected replacement settles original native service grants atomically",
         "Both native generator models share one actual original vent budget")


def validate_points(points, delivered):
    """Require real paid ports, fresh typed identities, both native useful consumers and a single unchanged vent budget.

    @param points Nineteen original/repeated complete source/connection/service/untouched-next observations.
    @param delivered Exact opposite accepted whole empty-return original, with actual paid replacement bills fully consumed.
    @return Unique scene/step grid; any historical reuse, material loss, account/thermal mismatch or missing useful output refuses acceptance.
    """
    grid = {(point["scene"], point["step"]): point for point in points}
    expected = {(-1, 0)} | {(scene, step) for scene in range(9) for step in range(2)}
    assert len(grid) == len(points) == 19 and grid.keys() == expected
    initial = grid[-1, 0]["snapshot"]; paid = delivered["snapshot"]
    assert world_chunks(ROOT / grid[-1, 0]["input"]["path"]) == world_chunks(ROOT / delivered["input"]["path"])
    for key, value in paid.items():
        if key != "invoices":  # This phase observes its own actual port invoices; the sealed cargo purchase remains historical.
            assert initial[key] == value, key
    assert initial["invoices"] == 0 and initial["power_next_producer"] == 4
    assert initial["producer3_generated"] > 0 and initial["producer3_consumed"] == 0
    original_links = range(initial["power_next_link"])
    for point in points:
        assert compare_worlds(point["input"], point["repeat"]) == 126
        current = point["snapshot"]
        for key in ("geology", "origin", "carrier", "industries", "orders", "next_order", "carrier_parts", "loan", "gross_income", "town_consumer", "town_id", "town_terminal"):
            assert current[key] == initial[key], (point["scene"], point["step"], key)
        assert current["whole_empty_home"] == 1 and current["aboard"] == 0 and current["tick"] >= initial["tick"]
        for part in range(5):
            for suffix in ("id", "first", "engine", "owner", "cargo", "capacity"):
                assert current[f"part{part}_{suffix}"] == initial[f"part{part}_{suffix}"]
        # Pure command-funded repair retains consumed old/new bills and every
        # natural/retired source; no old material becomes physical inventory.
        for machine, ticks in enumerate((296, 592, 888)):
            assert current[f"work{machine}"] == ticks and current[f"committed{machine}"] == 1
            assert current[f"machine{machine}_site"] == initial[f"machine{machine}_site"]
        for material in range(4):
            for field in ("bought", "dispatched", "retired", "live_consumed"):
                assert current[f"{field}{material}"] == initial[f"{field}{material}"]
            assert current[f"physical{material}"] == current[f"source{material}"] == 0
        for producer in range(3):
            for suffix in ("active", "generated", "consumed", "reserved", "domain", "input", "efficiency", "conversion_remainder"):
                assert current[f"producer{producer}_{suffix}"] == initial[f"producer{producer}_{suffix}"]
            assert current[f"producer{producer}_active"] == current[f"producer{producer}_reserved"] == 0
        # All retained identities keep their original endpoints. Only native
        # active source-edge supersession is permitted; tombstones stay terminal.
        for link in original_links:
            for suffix in ("kind", "source", "destination", "tile"):
                assert current[f"link{link}_{suffix}"] == initial[f"link{link}_{suffix}"]
            if initial[f"link{link}_active"] == 0:
                assert current[f"link{link}_active"] == 0
        elapsed = current["tick"] - initial["tick"]
        first = current["machine1_service_ticks"]; second = current["machine2_service_ticks"]
        assert current["producer3_generated"] == initial["producer3_generated"] + elapsed * 27000
        assert current["producer3_consumed"] == (first + second) * 27000 <= elapsed * 27000
        fees = first * 20 // 74 + second * 35 // 74
        assert current["machine1_service_remainder"] == first * 20 % 74
        assert current["machine2_service_remainder"] == second * 35 % 74
        assert initial["cash"] - current["cash"] >= current["invoices"] + fees
        # Native acquisition precedes every fresh conversion; all new thermal
        # attribution refers solely to naturally earned collector3, never0..2.
        for machine, efficiency, output_per_tick in ((1, 15, 4050), (2, 35, 9450)):
            producer = current[f"machine{machine}_electrical_producer"]
            if producer == 2 ** 32 - 1:
                assert point["scene"] in (-1, 0) and current[f"machine{machine}_service_ticks"] == 0
                continue
            consumer = current[f"machine{machine}_heat_consumer"]; prefix = f"producer{producer}_"
            ticks = current[f"machine{machine}_service_ticks"]
            assert producer >= initial["power_next_producer"] and consumer >= initial["power_next_consumer"]
            assert current[prefix + "input"] == consumer and current[prefix + "efficiency"] == efficiency
            assert current[prefix + "generated"] == ticks * output_per_tick
            origins = {key: value for key, value in current.items() if re.fullmatch(prefix + r"origin\d+", key)}
            assert sum(origins.values()) == ticks * 27000
            assert not origins or origins.keys() == {prefix + "origin3"}
    for scene in range(9):
        assert grid[scene, 1]["snapshot"]["tick"] == grid[scene, 0]["snapshot"]["tick"] + 1
    ports = grid[0, 0]["snapshot"]
    assert ports["tick"] == initial["tick"] and ports["invoices"] == initial["cash"] - ports["cash"] == 5000
    assert ports["power_next_link"] == initial["power_next_link"] + 5
    bound = grid[1, 0]["snapshot"]
    assert bound["power_next_producer"] == 6 and bound["power_next_consumer"] == initial["power_next_consumer"] + 2
    assert bound["machine1_service_ticks"] == 1 and bound["machine2_service_ticks"] == 0
    for scene, machine, uses in ((3, 1, 37), (4, 1, 74), (7, 2, 37), (8, 2, 74)):
        current = grid[scene, 0]["snapshot"]; producer = current[f"machine{machine}_electrical_producer"]
        assert current[f"producer{producer}_consumed"] == uses * 135
        assert current["town_applied"] == 1 and current["town_bonus"] == 25 and current["town_population"] > 0
    first_connection = grid[2, 0]["snapshot"]; switched = grid[6, 0]["snapshot"]
    assert first_connection["town_supply"] != initial["town_supply"] and switched["town_supply"] != first_connection["town_supply"]
    assert switched[f"consumer{first_connection['town_supply']}_active"] == 0
    assert grid[5, 0]["snapshot"]["invoices"] == 5250
    final = grid[8, 0]["snapshot"]
    assert final["machine1_service_ticks"] == switched["machine1_service_ticks"] > 0
    assert final["machine2_service_ticks"] > 0 and final["invoices"] == 5250
    return grid


def operate(configuration, delivery_report):
    """Restore actual paid ports and earn real service on the exact opposite original reconstructed world.

    @param configuration Current native Debug or optimized test executable.
    @param delivery_report Exact sealed opposite writer report; its original scene8/step0 is the sole mutation input.
    @return None; print an original service report only after native producers, full-world purity and actual two-model service pass.
    """
    receipt = ROOT / DELIVERY; assert identity(receipt)["sha256"] == DELIVERY_SHA
    history = json.loads(receipt.read_text(encoding="utf-8")); selected = identity(delivery_report)
    record = next(row for row in history["records"] if row["report"] == selected); assert record["configuration"] != configuration
    delivered = next(point for point in record["checkpoints"] if point["scene"] == 8 and point["step"] == 0)
    source = ROOT / delivered["input"]["path"]; assert identity(source) == delivered["input"]
    context = current_run_context("geothermal", "rebuilt-service"); context.prepare_runtime(); processes = []
    # Native physical admission/source replacement/contention run in isolated
    # processes before the actual original, without leaking prepared fixture state.
    for index, case in enumerate(CASES):
        _, proof = native(context, configuration, case, f"producer-{index}"); processes.append({"case": case, **proof})
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md1CCD; original rebuilt machinery and fresh native service.\nplant_rebuilt_service\n", encoding="utf-8")
    output, proof = native(context, configuration, "[.geothermal-first-plant-rebuilt-service]", "original-service", {"OTTD_PLANT_SERVICE_INPUT": str(source)})
    points = []
    # Observed native fields remain evidence only; this controller cannot
    # assign a port, account, generation rate, service time or town benefit.
    for scene, step, text, path, repeat in re.findall(r"PLANT_SERVICE_WORLD scene=(-?\d+) step=(\d+) snapshot=([^\s]+) path=([^\r\n]+) repeat=([^\r\n]+)", output):
        snapshot = parse_snapshot(text); assert len(snapshot) == text.count("=")
        points.append({"scene": int(scene), "step": int(step), "snapshot": snapshot, "input": identity(path), "repeat": identity(repeat)})
    grid = validate_points(points, delivered); assert identity(source) == delivered["input"]
    complete = re.search(r"PLANT_SERVICE_COMPLETE original=(\d+) final=(\d+) frames=(\d+) original_vent=1 models=2 uses=74,74 ports_paid=(\d+)", output); assert complete is not None
    start, end, frames, invoices = map(int, complete.groups()); assert start == grid[0, 0]["snapshot"]["tick"] and end == grid[8, 0]["snapshot"]["tick"]
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CCD; original paid physical repair and fresh useful both-model service",
              "configuration": configuration, "focus": "operate", "native": processes + [proof], "checkpoints": points,
              "result": {"start_tick": start, "final_tick": end, "native_ticks": end - start, "presentation_frames": frames, "port_invoices": invoices, "useful_original_uses": [74, 74]},
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES],
              "delivery_receipt": identity(receipt), "delivery_report": selected, "delivery_checkpoint": delivered,
              "fixture_classification": "Exact original whole-return world. Five paid fresh domain ports restore actual collector/both machines; ordinary complete frames acquire new typed identities. Earlier VentTap receives the single shared vent budget. Actual paid Heat isolation and synchronized town-source replacement then give Magma fresh intervals. The same original paid risers/terminal/populated town earn37/74 interrupted/completed useful135J opportunities from each native model. Retired source/history/material owners stay intact. Every original has a complete untouched next opportunity and a pure repeated whole world. Ordinary retired/cold/interrupted/rebuilt reader acceptance remains1CD."}
    path = context.output("reports", f"first-plant-rebuilt-service-{configuration}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def accept(reports, build_logs, output):
    """Accept both current original useful-service writers and every complete native boundary.

    @param reports Two exact-opposite-delivery current native service reports.
    @param build_logs Successful current Debug/optimized test target diagnostics.
    @param output Absent immutable service receipt below repository logs.
    @return None; stale references or any complete-world/quantity mismatch prevent task closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, records = {}, {}; summary = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    # Only current service code/binaries are freshly verified. Exact sealed
    # delivery reports remain opaque historical authority after later rebuilds.
    for name in reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in summary and cfg not in records and report["focus"] == "operate"
        validate_points(report["checkpoints"], report["delivery_checkpoint"])
        for proof in report["native"]:
            count_native(proof, summary[cfg])
        records[cfg] = {"report": identity(path), **report}
    assert records.keys() == summary.keys() and summary["Debug"] == summary["RelWithDebInfo"]
    for key in ("start_tick", "final_tick", "native_ticks", "port_invoices", "useful_original_uses"):
        assert records["Debug"]["result"][key] == records["RelWithDebInfo"]["result"][key]
    left = {(point["scene"], point["step"]): point for point in records["Debug"]["checkpoints"]}
    right = {(point["scene"], point["step"]): point for point in records["RelWithDebInfo"]["checkpoints"]}; assert left.keys() == right.keys(); pairs = []
    # All126 complete native chunks, including original accounts/vehicles/
    # utility lineage and genuine saved effects, compare without exclusions.
    for key in sorted(left):
        first, second = left[key], right[key]; assert first["snapshot"] == second["snapshot"]
        assert compare_worlds(first["input"], second["input"]) == compare_worlds(first["repeat"], second["repeat"]) == 126
        pairs.append({"scene": key[0], "step": key[1], "left": first["input"], "right": second["input"], "complete_chunks_exact": 126})
    builds = []
    # Actual compile/link outputs establish the executables that produced the
    # accepted service, rather than an uncompiled declaration or old receipt.
    for name in build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CCD/original1CC; fresh useful original-vent reconstruction acceptance",
              "controller": identity(__file__), "records": list(records.values()), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "complete_cross_build_world_pairs": pairs, "pure_repeat_pairs": 38, "delivery_receipt": identity(ROOT / DELIVERY), "builds": builds,
              "native_version": 434, "exact_identities_verified": len(cache),
              "remaining": "1CD ordinary original retired/cold/interrupted/rebuilt preview/readers/full continuation remains before1.3B/original1.3. Master10.7/runtime-art/9.5B/later TBM-to-tube/human/operator/network gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(pairs)}), flush=True)


def main():
    """Select one bounded original useful-service run or its complete independent two-build acceptance.

    @return None; incomplete arguments or missing actual native evidence stop before acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("operate", "accept"), required=True)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--delivery-report", type=Path)
    parser.add_argument("--reports", nargs=2); parser.add_argument("--build-logs", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "operate":
        assert args.config is not None and args.delivery_report is not None; operate(args.config, args.delivery_report)
    else:
        assert args.reports is not None and args.build_logs is not None and args.output is not None
        accept(args.reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
