"""Accept original operating retirement/reconstruction files in focused ordinary reader batches.

@file plant_lifecycle_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md15D; exact native build-revision diagnostic migration.
Exact sealed original inputs remain immutable; no producer or fixture builder is rerun.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
from runner import current_run_context, run_native, console_executable
from local_electrical_feed_checks import identity, native
from geothermal_tube_binding_files import world_chunks
from tube_sprite_report import compare_worlds, verify_references
from mole_useful_raster_report import count_native
from first_plant_checks import saved_table
from lifecycle_revision_log import compare_original

PRODUCERS = {
    "retired": ("logs/geothermal/source-lifecycle/20261018-native-01/retirement-verification.json", "56d4c47f64f984b4eb9d049b5b19c1da4ca52d28f7033dd2934254e18aaa3232"),
    "funded": ("logs/geothermal/source-rebuild/20261018-native-01/funding-verification.json", "4c0f3b708a90f56ceb5cfd2111bd2bf9007cc83c14efd73f6ef073146f31032e"),
    "stock": ("logs/geothermal/replacement-stock/20261018-native-01/verification.json", "5423e9ff985f15be4051c35918f4d61b05ebcf114260530004f51a38309ecc4c"),
    "delivery": ("logs/geothermal/replacement-delivery/20261018-native-01/verification.json", "c69b48f271812ea2163de76a041782547fa89f250fc2d03acb7b42a69d213503"),
    "service": ("logs/geothermal/rebuilt-service/20261018-native-01/verification.json", "ce965450a66cb43b0c624bcb3e244cdcee8113c5b241e8f30a64dc17b11bd3b6"),
}
SOURCES = ("src/tests/geothermal_plant_lifecycle_files.cpp", "src/tests/multilayer/utilities/plant_lifecycle_files.py",
           "src/tests/multilayer/lifecycle_revision_log.py", "src/gamelog.cpp", "src/gamelog.h", "src/saveload/gamelog_sl.cpp",
           "src/saveload/geothermal_collector_sl.cpp", "src/saveload/geothermal_generator_sl.cpp",
           "src/saveload/geothermal_site_sl.cpp", "src/saveload/utility_power_sl.cpp")
CONFIGURATIONS = ("Debug", "RelWithDebInfo")


def manifest(output):
    """Select all distinct original lifecycle states, preserving exact producer receipts and uninterrupted next baselines.

    @param output Absent immutable manifest under repository logs.
    @return None; missing sealed history or unequal complete writer worlds prevents manifest publication.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cases, receipts = {}, []
    # Stage0 of funding/stock and scene-1 of delivery/service duplicate the
    # preceding accepted final. Keep26 distinct states without rerunning owners.
    for family, (name, sha) in PRODUCERS.items():
        path = ROOT / name; assert identity(path)["sha256"] == sha; receipts.append(identity(path))
        receipt = json.loads(path.read_text(encoding="utf-8"))
        for record in receipt["records"]:
            cfg = record["configuration"]; assert cfg in CONFIGURATIONS
            points = record["checkpoints"]
            for point in points:
                stage = point.get("stage", point.get("scene"))
                if "scene" in point and (stage < 0 or point["step"] != 0):
                    continue  # Next1 remains the original baseline, not another input replacing its useful/work original0.
                if family in ("funded", "stock") and stage == 0:
                    continue  # Previous owner already supplied this exact boundary; duplication cannot strengthen a lifecycle claim.
                label = f"{family}-{stage}"; selected = cases.setdefault(label, {"family": family, "writers": {}})
                assert cfg not in selected["writers"]
                original = point["input"]; assert identity(ROOT / original["path"]) == original
                baseline = None
                if "scene" in point:
                    baseline = next(row["input"] for row in points if row["scene"] == stage and row["step"] == 1)
                    assert identity(ROOT / baseline["path"]) == baseline
                selected["writers"][cfg] = {"input": original, "baseline": baseline, "report": record["report"], "receipt": identity(path)}
    assert len(cases) == 26
    for case in cases.values():
        assert set(case["writers"]) == set(CONFIGURATIONS)
        assert compare_worlds(case["writers"]["Debug"]["input"], case["writers"]["RelWithDebInfo"]["input"]) == 126
    result = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CD; all26 distinct original lifecycle states and18 untouched next boundaries",
              "controller": identity(__file__), "producer_receipts": receipts, "cases": cases}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output, flush=True)


def read_original(context, configuration, source, label, baseline):
    """Ordinary-load one immutable actual lifecycle state, copied-preview/resave it and optionally earn its complete native next opportunity.

    @param context Invocation-owned runtime/output/diagnostic namespace.
    @param configuration Actual compiled current test executable.
    @param source Exact original or first genuine ordinary resave.
    @param label Unique case/iteration prefix preventing outputs from overwriting their supplied inputs.
    @param baseline Exact untouched original full next world, or None for a pure paused lifecycle read.
    @return Original/native output identities and nonzero native evidence; any complete-owner difference refuses acceptance.
    """
    context.prepare_runtime(); startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md1CD; ordinary original lifecycle file acceptance.\nplant_lifecycle_file\n", encoding="utf-8")
    before = identity(source)
    output, proof = native(context, configuration, "[.geothermal-plant-lifecycle-file]", label,
                           {"OTTD_PLANT_FILE_INPUT": str(source), "OTTD_PLANT_FILE_LABEL": label, "OTTD_PLANT_FILE_ACTION": "next" if baseline else "inspect"})
    points = {}
    for stage, snapshot, path in re.findall(r"PLANT_LIFECYCLE_FILE stage=([^\s]+) snapshot=([^\s]+) path=([^\r\n]+)", output):
        assert stage not in points; points[stage] = {"snapshot": snapshot, "input": identity(path)}
    assert points.keys() == ({"read", "repeat", "next", "next-repeat"} if baseline else {"read", "repeat"})
    # A rebuilt executable can append its actual new revision to native GLOG.
    # Every old diagnostic byte and all125 other complete chunks remain exact;
    # repeated saves of the same accepted build still require all126 bytes.
    load_tick = saved_table(source, b"DATE")[0]["tick_counter"]
    read_comparison = compare_original(before, points["read"]["input"], load_tick)
    assert compare_worlds(points["read"]["input"], points["repeat"]["input"]) == 126
    next_comparison = None
    if baseline is not None:
        next_comparison = compare_original(baseline, points["next"]["input"], load_tick)
        assert compare_worlds(points["next"]["input"], points["next-repeat"]["input"]) == 126
    assert identity(source) == before
    return {"input": before, "baseline": baseline, "outputs": points, "native": proof,
            "original_read_comparison": read_comparison, "original_next_comparison": next_comparison}


def ordinary(context, configuration, executable, source, label):
    """Read/resave an actual lifecycle original with the unchanged normal production game before a simulation frame.

    @param context Current bounded batch runtime/output owner, shared serially to avoid unnecessary executable copies.
    @param configuration Actual normal production game configuration.
    @param executable Invocation-local console projection of the exact existing production executable.
    @param source Immutable original or preceding genuine ordinary production resave.
    @param label Unique case/iteration basename.
    @return Native production/input/output/log identities and field-checked transient startup pause; every other complete owner stays exact.
    """
    output = context.saves / f"production-{label}.sav"; assert not output.exists()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md1CD; normal production lifecycle read.\n"
                       f'set savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    before = identity(source); log = context.logs / f"production-{label}.log"
    with log.open("wb") as sink:
        process = run_native(context, [executable, "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                                       "-c", context.config_path, "-g", source], cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=90, check=False)
    assert process.returncode == 0 and output.is_file(), log
    first, second = world_chunks(source), world_chunks(output); assert first.keys() == second.keys() and len(first) == 126
    changed = [name.decode() for name in first if first[name] != second[name]]
    assert set(changed) <= {"DATE"}, changed
    before_clock, after_clock = saved_table(source, b"DATE"), saved_table(output, b"DATE")
    assert before_clock.keys() == after_clock.keys()
    # Current434 lifecycle files need no old-format/company initialization.
    # Every complete nonclock owner stays exact; only the native transient
    # CargoDist pause bit may be added, with every other DATE field unchanged.
    for row, fields in before_clock.items():
        assert fields.keys() == after_clock[row].keys()
        for name, value in fields.items():
            if name == "pause_mode":
                assert after_clock[row][name] in (value, value | 64)
            else:
                assert after_clock[row][name] == value, (row, name)
    assert identity(source) == before
    return {"input": before, "output": identity(output), "log": identity(log), "game": identity(ROOT / f"build/{configuration}/openttd.exe"),
            "changed_complete_chunks": changed, "all_other_complete_chunks_exact": True, "startup_pause_field_checked": True}


def batch(configuration, phase, manifest_path, labels):
    """Execute at most four distinct lifecycle states in a focused current-reader or normal-production burst.

    @param configuration Current reader configuration; every input is selected from the opposite accepted writer.
    @param phase Read includes copied preview/repetition/native-next; production uses the unchanged normal game twice.
    @param manifest_path Exact retained26-state manifest.
    @param labels One to four distinct selected original state labels, keeping native bursts below two minutes.
    @return None; print a current bounded report only after every selected original/repeated/native-next world passes.
    """
    assert 1 <= len(labels) <= 4 and len(set(labels)) == len(labels)
    manifest_path = (ROOT / manifest_path).resolve(); selected = json.loads(manifest_path.read_text(encoding="utf-8"))
    context = current_run_context("geothermal", "lifecycle-files"); context.prepare_runtime(); results = []
    opposite = next(cfg for cfg in CONFIGURATIONS if cfg != configuration)
    executable = console_executable(context, ROOT / f"build/{configuration}/openttd.exe") if phase == "production" else None
    # Every native process starts from an exact original or its genuine first
    # ordinary resave. No retirement/funding/purchase/carriage/service builder runs.
    for label in labels:
        original = selected["cases"][label]["writers"][opposite]; source = ROOT / original["input"]["path"]; assert identity(source) == original["input"]
        if phase == "read":
            first = read_original(context, configuration, source, label + "-first", original["baseline"])
            second = read_original(context, configuration, ROOT / first["outputs"]["read"]["input"]["path"], label + "-second", original["baseline"])
        else:
            first = ordinary(context, configuration, executable, source, label + "-first")
            second = ordinary(context, configuration, executable, ROOT / first["output"]["path"], label + "-second")
        assert identity(source) == original["input"]
        results.append({"case": label, "original": original, "first": first, "second": second})
        print(f"{configuration}/{phase}/{label}: complete ordinary readers passed", flush=True)
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CD; bounded actual original lifecycle readers/preview/native-next",
              "configuration": configuration, "phase": phase, "manifest": identity(manifest_path), "controller": identity(__file__), "results": results,
              "sources": [identity(ROOT / name) for name in SOURCES], "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe") if phase == "read" else identity(ROOT / f"build/{configuration}/openttd.exe")}
    path = context.output("reports", f"plant-lifecycle-{configuration}-{phase}-{labels[0]}-{labels[-1]}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def accept(manifest_path, reports, build_logs, output):
    """Accept all opposite-reader lifecycle cases, repeated ordinary loads and exact original full next opportunities.

    @param manifest_path Exact original26-state/five-producer manifest.
    @param reports Explicit bounded read/production reports covering each state in both configurations exactly once.
    @param build_logs Successful final current native test target compile/link diagnostics.
    @param output Absent immutable lifecycle file/handoff receipt below repository logs.
    @return None; missing/stale original references or any absent whole-world/native-next proof prevents1.3B closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    manifest_path = (ROOT / manifest_path).resolve(); selected = json.loads(manifest_path.read_text(encoding="utf-8"))
    expected = {(cfg, phase, case) for cfg in CONFIGURATIONS for phase in ("read", "production") for case in selected["cases"]}
    cache, seen, records, native_pairs = {}, set(), [], {}
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in CONFIGURATIONS}; production_processes = 0; next_comparisons = 0
    # Exact producer receipts/reports/inputs are opaque historical references;
    # current reader source/binary/log/output identities are verified freshly.
    verify_references(selected, cache)
    for name in reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        assert report["manifest"] == identity(manifest_path); cfg, phase = report["configuration"], report["phase"]
        for result in report["results"]:
            label = result["case"]; key = cfg, phase, label; assert key in expected and key not in seen; seen.add(key)
            opposite = next(other for other in CONFIGURATIONS if other != cfg); assert result["original"] == selected["cases"][label]["writers"][opposite]
            if phase == "read":
                for iteration in ("first", "second"):
                    proof = result[iteration]; count_native(proof["native"], summary[cfg])
                    tick = saved_table(ROOT / proof["input"]["path"], b"DATE")[0]["tick_counter"]
                    assert proof["original_read_comparison"] == compare_original(proof["input"], proof["outputs"]["read"]["input"], tick)
                    assert compare_worlds(proof["outputs"]["read"]["input"], proof["outputs"]["repeat"]["input"]) == 126
                    if proof["baseline"] is not None:
                        assert proof["original_next_comparison"] == compare_original(proof["baseline"], proof["outputs"]["next"]["input"], tick)
                        assert compare_worlds(proof["outputs"]["next"]["input"], proof["outputs"]["next-repeat"]["input"]) == 126
                        next_comparisons += 1
                native_pairs.setdefault(label, {})[cfg] = result
            else:
                production_processes += 2
                for iteration in ("first", "second"):
                    proof = result[iteration]; assert proof["startup_pause_field_checked"] and proof["all_other_complete_chunks_exact"]
        records.append({"report": identity(path), **report})
    assert seen == expected and summary["Debug"] == summary["RelWithDebInfo"]
    assert production_processes == 104 and next_comparisons == 72
    # Opposite current readers independently retain every original complete
    # world and its actual full next baseline, without chunk exclusions.
    cross_pairs = 0
    for pair in native_pairs.values():
        for iteration in ("first", "second"):
            left, right = pair["Debug"][iteration], pair["RelWithDebInfo"][iteration]
            assert compare_worlds(left["outputs"]["read"]["input"], right["outputs"]["read"]["input"]) == 126; cross_pairs += 1
            if left["baseline"] is not None:
                assert compare_worlds(left["outputs"]["next"]["input"], right["outputs"]["next"]["input"]) == 126; cross_pairs += 1
    builds = []
    for name in build_logs:  # Actual final compile/link diagnostics establish the current native reader, rather than a header declaration alone.
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CD/original1.3B/1.3; complete original operating lifecycle file acceptance",
              "controller": identity(__file__), "manifest": identity(manifest_path), "records": records, "summary": summary, "builds": builds,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "ordinary_production_processes": production_processes, "original_native_next_comparisons": next_comparisons,
              "complete_cross_reader_world_pairs": cross_pairs, "complete_native_chunks": 126, "exact_identities_verified": len(cache), "native_version": 434,
              "remaining": "Master10.7 selected Plan14-84.3 runtime integration,9.5B/later TBM-to-tube final-use and human/operator/network gates remain pending. Independent file replay is not an actual host/client join."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "production_processes": production_processes, "full_native_next": next_comparisons, "cross_reader_pairs": cross_pairs}), flush=True)


def main():
    """Select immutable input discovery, one bounded current-reader/production burst or complete lifecycle acceptance.

    @return None; missing phase arguments or failed actual whole-world evidence stop before closure.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("manifest", "read", "production", "accept"), required=True)
    parser.add_argument("--config", choices=CONFIGURATIONS); parser.add_argument("--manifest"); parser.add_argument("--cases", nargs="+")
    parser.add_argument("--reports", nargs="+"); parser.add_argument("--build-logs", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "manifest":
        assert args.output is not None; manifest(args.output)
    elif args.phase in ("read", "production"):
        assert args.config is not None and args.manifest is not None and args.cases is not None; batch(args.config, args.phase, args.manifest, args.cases)
    else:
        assert args.manifest is not None and args.reports is not None and args.build_logs is not None and args.output is not None
        accept(args.manifest, args.reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
