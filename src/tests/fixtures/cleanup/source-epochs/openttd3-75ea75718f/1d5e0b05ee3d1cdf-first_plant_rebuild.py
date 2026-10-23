"""Fund fresh native machinery in the exact retained original retired first-plant world.

@file first_plant_rebuild.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CCB/operating1.3B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Fresh capital and native claims are real; replacement material transport, labour and useful power remain1CCC/1CCD.
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
from first_plant_lifecycle import PROTECTED

RETIREMENT = "logs/geothermal/source-lifecycle/20261018-native-01/retirement-verification.json"
RETIREMENT_SHA = "56d4c47f64f984b4eb9d049b5b19c1da4ca52d28f7033dd2934254e18aaa3232"
SOURCES = ("src/tests/geothermal_first_plant_rebuild.cpp", "src/tests/multilayer/utilities/first_plant_rebuild.py",
           "src/geothermal_site_cmd.cpp", "src/geothermal_claim.cpp", "src/geothermal_collector.cpp",
           "src/geothermal_generator.cpp", "src/industry_cmd.cpp", "src/multilayer_bootstrap.cpp", "src/utility_power.cpp")
CASES = ("Native geothermal planning commands preserve quote and cancellation custody",
         "Native collector funding consumes a synchronized original plan",
         "Native powerhouse funding settles original capital without construction or output")


def validate_points(points, retired):
    """Verify fresh claims/full funding while every original natural/retired/physical owner remains intact.

    @param points Original0, native footprint preparation1, fresh-cold2 and actual-paid-empty3 native observed worlds.
    @param retired Exact accepted opposite-writer original retired checkpoint and native owner observations.
    @return None; changed originals, free capital/work/stock/power or inherited old claims reject acceptance.
    """
    assert [point["stage"] for point in points] == [0, 1, 2, 3]
    first = points[0]["snapshot"]; old = retired["snapshot"]; initial = world_chunks(ROOT / points[0]["input"]["path"])
    assert len(initial) == 126 and initial == world_chunks(ROOT / retired["input"]["path"])
    assert first["tick"] == old["tick"] and first["cash"] == old["cash"] and first["year"] == old["year"] == 2000
    assert first["geology"] == old["geology"] and first["vent"] == old["vent"] and first["capacity"] == old["capacity"] == 1000000
    assert first["industries"] == old["industries"] and first["old_producers"] == 3 and first["next_producer"] == 3
    expected_sites = [old["collector_site"], old["site0"], old["site1"]]
    expected_tiles = [old["collector_tile"], old["tile0"], old["tile1"]]
    expected_industries = [old["collector"], old["generator0"], old["generator1"]]
    protected = {name.encode() for name in PROTECTED}; assert protected <= initial.keys()
    prepared = points[1]["snapshot"]; assert prepared["utility"] != first["utility"] and prepared["preparation"] >= 0
    # Every stage retains source geometry, old meter history and unrelated
    # paid physical owners. Native Industry allocation may reuse the old pool
    # IDs, but must acquire fresh claims and zero actual construction state.
    for point in points:
        stage, current = point["stage"], point["snapshot"]
        for key in ("tick", "year", "vent", "capacity", "geology", "projects", "exchange", "supply", "risers", "vehicles", "packets",
                    "old_producers", "next_producer", "next_consumer", "next_link", "utility_links", "old_source0_generated", "old_source0_consumed",
                    "old_source1_generated", "old_source1_consumed", "old_source2_generated", "old_source2_consumed"):
            assert current[key] == first[key], (stage, key)
        assert current["claims"] == (3 if stage < 2 else 6)
        assert current["capital"] == (720000 if stage == 3 else 0)
        assert current["preparation"] == (0 if stage == 0 else prepared["preparation"])
        assert current["invoices"] == current["capital"] + current["preparation"]
        assert current["cash"] == first["cash"] - current["invoices"]
        assert current["industries"] == first["industries"] + (3 if stage == 3 else 0)
        assert current["collectors"] == int(stage == 3) and current["generators"] == (2 if stage == 3 else 0)
        if stage >= 1:
            assert current["utility"] == prepared["utility"]
        for index in range(3):
            assert current[f"old_site{index}"] == expected_sites[index]
            assert current[f"tile{index}"] == expected_tiles[index] and current[f"kind{index}"] == (0, 1, 3)[index]
            assert current[f"stock{index}"] == current[f"accepted{index}"] == current[f"work{index}"] == current[f"committed{index}"] == 0
            assert current[f"input{index}"] == current[f"output{index}"] == 4294967295
            if stage >= 2:
                assert current[f"fresh_site{index}"] > max(expected_sites)
            if stage == 3:
                assert current[f"industry{index}"] == expected_industries[index]
        if stage >= 2:
            assert len({current[f"fresh_site{index}"] for index in range(3)}) == 3
        chunks = world_chunks(ROOT / point["input"]["path"]); assert chunks.keys() == initial.keys()
        for name in protected:
            assert chunks[name] == initial[name], (stage, name)
    # Consumed fresh planning rows remain the same native identities when
    # funding creates machinery; old consumed rows never acquire new rights.
    for index in range(3):
        assert points[2]["snapshot"][f"fresh_site{index}"] == points[3]["snapshot"][f"fresh_site{index}"]
    assert first["old_source0_generated"] == old["heat_generated"] and first["old_source0_consumed"] == old["heat_consumed"]
    for index in range(2):
        assert first[f"old_source{index + 1}_generated"] == old[f"generated{index}"]
        assert first[f"old_source{index + 1}_consumed"] == old[f"consumed{index}"]


def fund(configuration, retired_report):
    """Load the exact opposite accepted retired world and execute fresh planning/funding commands.

    @param configuration Actual current native Debug or optimized test binary.
    @param retired_report Exact sealed opposite configuration's retirement report.
    @return None; print a current funding report only after all native/original-owner checks pass.
    """
    receipt = ROOT / RETIREMENT; assert identity(receipt)["sha256"] == RETIREMENT_SHA
    history = json.loads(receipt.read_text(encoding="utf-8")); selected = identity(retired_report)
    record = next(row for row in history["records"] if row["report"] == selected)
    assert record["configuration"] != configuration
    retired = next(point for point in record["checkpoints"] if point["stage"] == 3)
    source = ROOT / retired["input"]["path"]; assert identity(source) == retired["input"]
    context = current_run_context("geothermal", "source-rebuild"); context.prepare_runtime(); processes = []
    # Exact isolated planner/funding cases verify the command producers before
    # their dependent mutation of the retained actual paid-world original.
    for index, case in enumerate(CASES):
        _, proof = native(context, configuration, case, f"producer-{index}"); processes.append({"case": case, **proof})
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md1CCB; fresh command-funded native original-site machinery.\nplant_rebuild_funding\n", encoding="utf-8")
    output, proof = native(context, configuration, "[.geothermal-first-plant-rebuild-funding]", "original-funding", {"OTTD_PLANT_REBUILD_INPUT": str(source)})
    points = []
    # Native flat observations retain original and new identities separately;
    # parsing these scalars never creates or repairs gameplay participants.
    for text, path in re.findall(r"PLANT_REBUILD_WORLD snapshot=([^\s]+) path=([^\r\n]+)", output):
        snapshot = parse_snapshot(text); points.append({"stage": snapshot["stage"], "snapshot": snapshot, "input": identity(path)})
    validate_points(points, retired); assert identity(source) == retired["input"]
    proof.update(input=retired["input"], output=points[-1]["input"])
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CCB; actual fresh original-site funding, paid material carriage/labour/service remain1CCC/1CCD",
              "configuration": configuration, "focus": "fund", "native": processes + [proof], "checkpoints": points,
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "sources": [identity(ROOT / name) for name in SOURCES], "retirement_receipt": identity(receipt),
              "retired_report": selected, "retired_checkpoint": retired, "protected_complete_chunks": sorted(PROTECTED),
              "fixture_classification": "Exact opposite accepted original retired stage3 is ordinary-loaded. Native removal commands prepare only old utility cells inside the original vacant machinery footprints, retaining their identities/history and surrounding paths. Native planning/funding then creates three fresh claims and charges full720000 capital plus exact preparation invoices for actual collector/VentTap/Magma machinery on unchanged paid floor/vent. Reused Industry IDs retain zero new stock/work/receipt/service and unissued new Heat/electric identities. All original retired source/meter/claim histories and protected physical/Surface/spoil/riser owners remain. No material, labour, energy, floor, cash or clock grant; actual replacement transport and useful service remain1CCC/1CCD."}
    path = context.output("reports", f"first-plant-rebuild-funding-{configuration}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def accept(reports, build_logs, output):
    """Accept both independent fresh funding proofs with exact complete native stage comparisons.

    @param reports Two current opposite-original native funding reports.
    @param build_logs Successful current test-target build diagnostics for both configurations.
    @param output Absent immutable funding receipt under repository logs.
    @return None; stale originals, unequal owner/world state or incomplete native funding prevents acceptance.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, records = {}, {}; summary = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    # Current sources/binaries are checked directly. The exact sealed old
    # retirement report remains opaque history rather than a current build claim.
    for name in reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in summary and cfg not in records and report["focus"] == "fund"
        validate_points(report["checkpoints"], report["retired_checkpoint"])
        for proof in report["native"]:
            count_native(proof, summary[cfg])
        records[cfg] = {"report": identity(path), **report}
    assert records.keys() == summary.keys() and summary["Debug"] == summary["RelWithDebInfo"]
    pairs = []
    # Whole native worlds retain every command-produced tile, Industry,
    # claim, capital expense and RNG result across independent builds.
    for first, second in zip(records["Debug"]["checkpoints"], records["RelWithDebInfo"]["checkpoints"]):
        assert first["stage"] == second["stage"] and first["snapshot"] == second["snapshot"]
        assert compare_worlds(first["input"], second["input"]) == 126
        pairs.append({"stage": first["stage"], "left": first["input"], "right": second["input"], "complete_chunks_exact": 126})
    builds = []
    # Native test/link diagnostics support the current executable identities;
    # declarations or a copied historical receipt cannot replace these targets.
    for name in build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CCB; fresh actual original-site capital/machinery acceptance",
              "controller": identity(__file__), "records": list(records.values()), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "complete_cross_build_world_pairs": pairs, "retirement_receipt": identity(ROOT / RETIREMENT), "builds": builds,
              "native_version": 434, "exact_identities_verified": len(cache),
              "remaining": "1CCC actual paid replacement transport/native labour/empty return,1CCD fresh original-vent useful service and1CD ordinary lifecycle files must pass before1CC/1.3B/original1.3. Master10.7/runtime art/9.5B/later TBM-to-tube/human/operator/network gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(pairs)}), flush=True)


def main():
    """Select one bounded native original-site funding run or its two-build acceptance.

    @return None; missing phase arguments or failed actual native evidence stop before acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("fund", "accept"), required=True)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--retired-report", type=Path)
    parser.add_argument("--reports", nargs=2); parser.add_argument("--build-logs", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "fund":
        assert args.config is not None and args.retired_report is not None
        fund(args.config, args.retired_report)
    else:
        assert args.reports is not None and args.build_logs is not None and args.output is not None
        accept(args.reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
