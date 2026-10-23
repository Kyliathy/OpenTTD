"""Retain real operating first-plant retirement without reconstructing its paid source world.

@file first_plant_lifecycle.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CB/operating1.3B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native demolition is the only mutation. Retired/rebuilt ordinary acceptance and paid replacement construction remain later groups.
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

ORIGINAL = "logs/geothermal/first-plant/20261017-native-01/first-plant-power-verification.json"
ORIGINAL_SHA = "905afa4cff910932cd87a5009617c2e699448c30bf5d65333161926cdf27e666"
SOURCES = ("src/tests/geothermal_first_plant_lifecycle.cpp", "src/tests/multilayer/utilities/first_plant_lifecycle.py",
           "src/industry_cmd.cpp", "src/geothermal_collector.cpp", "src/geothermal_generator.cpp", "src/geothermal_claim.cpp",
           "src/utility_power.cpp", "src/utility_power_transfer.h", "src/saveload/afterload.cpp")
CASES = ("Geothermal actual collector lifecycle retains paid work and retires only machinery",
         "Native operating powerhouse demolition retains retired utility history",
         "Natural vent generation preserves ten minerals and has no free cavity or heat grant")
# Native demolition must not change any geological/excavation/haulage owner,
# retained physical train/order/packet or the original Surface supply/riser
# construction histories. Utility and removed Industry/native tile records
# change legitimately and remain fully compared between independent builds.
PROTECTED = set("GEOS GDEP GSPN GVNT EXJS XJOB XJRT XJTL XPAS LPWH LPWK TBMH TBMP TBMS SPRH SPRR LMNH LMNR LPCH LPCR LPCC TBXH TBXA TBXS TBXW SSOH SSOR GRSH GRSD VEHS DEPT ORDL CAPA".split())


def validate_points(points):
    """Check actual original machinery retirement and immutable paid/natural owners at every stage.

    @param points Four native observed worlds: original, first model gone, both gone, collector gone.
    @return None; missing stages, altered source/history or nonterminal old generation rejects acceptance.
    """
    assert [point["stage"] for point in points] == list(range(4))
    first = points[0]["snapshot"]; before = world_chunks(ROOT / points[0]["input"]["path"])
    assert len(before) == 126 and PROTECTED <= {name.decode() for name in before}
    assert first["year"] == 2000 and first["capacity"] == 1000000 and first["packets"] == 0
    assert first["model0"] == 0 and first["model1"] == 1 and first["service0"] > 0 and first["service1"] > 0
    assert first["generated0"] > 0 and first["generated1"] > 0 and first["heat_consumed"] > 0
    previous = first
    # Every original native stage keeps the same paid floor/spoil/Surface
    # stock/cargo and consumed energy. Only the expected machinery disappears,
    # and its ordinary native clearing invoices account for the entire cash delta.
    for point in points:
        stage, current = point["stage"], point["snapshot"]
        for key in ("tick", "year", "vent", "capacity", "geology", "projects", "exchange", "supply", "risers", "vehicles", "packets",
                    "generator0", "generator1", "site0", "site1", "tile0", "tile1", "model0", "model1", "work0", "work1", "service0", "service1",
                    "heat_consumer0", "heat_consumer1", "electrical_source0", "electrical_source1", "generated0", "generated1", "consumed0", "consumed1",
                    "collector", "collector_site", "collector_tile", "heat_source", "heat_generated", "heat_consumed"):
            assert current[key] == first[key], (stage, key)
        assert current["generators"] == max(2 - stage, 0) and current["collectors"] == int(stage < 3)
        assert current["industries"] == first["industries"] - stage
        assert current["cash"] == first["cash"] - current["invoices"] and current["invoices"] >= previous["invoices"]
        if stage >= 1:
            assert current["active0"] == 0
        if stage >= 2:
            assert current["active1"] == 0
        if stage == 3:
            assert current["heat_active"] == 0
        chunks = world_chunks(ROOT / point["input"]["path"]); assert chunks.keys() == before.keys()
        for name in PROTECTED:
            assert chunks[name.encode()] == before[name.encode()], (stage, name)
        previous = current
    # These original native machinery definitions currently quote zero for
    # demolition. Preserve the exact command result/cash equation; retirement
    # cannot invent a fee or erase their already-paid construction history.
    assert previous["invoices"] >= 0


def retire(configuration):
    """Run prerequisite native callbacks and ordinary-load the exact accepted complete paid world.

    @param configuration Actual current Debug or optimized test executable.
    @return None; print one current retirement report only after every native and original-owner check passes.
    """
    receipt = ROOT / ORIGINAL; assert identity(receipt)["sha256"] == ORIGINAL_SHA
    history = json.loads(receipt.read_text(encoding="utf-8")); original = history["same_world_ready"]
    source = ROOT / original["path"]; assert identity(source) == original
    context = current_run_context("geothermal", "source-lifecycle"); context.prepare_runtime(); processes = []
    # Cross-plan industry/utility/geology producer boundaries are isolated,
    # so an unrelated fixture cannot supply state to the real afterload proof.
    for index, case in enumerate(CASES):
        _, proof = native(context, configuration, case, f"producer-{index}")
        processes.append({"case": case, **proof})
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md1CB; original operating machinery retirement.\nplant_lifecycle_retire\n", encoding="utf-8")
    output, proof = native(context, configuration, "[.geothermal-first-plant-lifecycle-retire]", "original-retirement",
                           {"OTTD_PLANT_LIFECYCLE_INPUT": str(source)})
    points = []
    # Metadata is observed, not an expected-state assignment. Native bools
    # decode explicitly; neither a quantity nor a gameplay reference is repaired.
    for stage, text, name in re.findall(r"PLANT_LIFECYCLE_WORLD stage=(\d+) snapshot=([^\s]+) path=([^\r\n]+)", output):
        pairs = [part.split("=") for part in text.rstrip(",").split(",")]
        snapshot = {key: (int(value) if value not in ("true", "false") else int(value == "true")) for key, value in pairs}
        assert len(snapshot) == len(pairs)
        points.append({"stage": int(stage), "snapshot": snapshot, "input": identity(name)})
    validate_points(points); assert identity(source) == original
    proof.update(input=original, output=points[-1]["input"])
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CB; actual original operating paid first-plant retirement, reconstruction remains1CC",
              "configuration": configuration, "focus": "retire", "native": processes + [proof], "checkpoints": points,
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "sources": [identity(ROOT / name) for name in SOURCES], "source_receipt": identity(receipt),
              "source_world": original, "protected_complete_chunks": sorted(PROTECTED),
              "fixture_classification": "Ordinary native afterload of the exact accepted431 paid-TBM/Surface-carrier/operating-both-model world. Native current afterload migration supplies definitions only; no original input bytes, clock/cash/material/floor/work/energy quantities or participants are rewritten by this fixture. Ordinary native industry clearing retires both plant incarnations and collector while original natural vent/floor, completed spoil and Surface supply/riser histories and consumed energy remain. This proves retirement, not actual reconstruction or retired/rebuilt file acceptance."}
    path = context.output("reports", f"first-plant-retirement-{configuration}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def accept(reports, build_logs, output):
    """Bind both current retirement proofs and every complete cross-build original stage.

    @param reports Two current independently run native retirement reports.
    @param build_logs Successful current test-target build diagnostics for both configurations.
    @param output Absent immutable retirement receipt beneath repository logs.
    @return None; stale identities, failed callbacks or unequal actual native worlds prevent acceptance.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, records = {}, {}; summary = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    # Each report binds current code/executable identities; its source receipt
    # remains an opaque historical identity, never a fresh old-binary claim.
    for name in reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in summary and cfg not in records and report["focus"] == "retire"
        validate_points(report["checkpoints"])
        for proof in report["native"]:
            count_native(proof, summary[cfg])
        records[cfg] = {"report": identity(path), **report}
    assert records.keys() == summary.keys() and summary["Debug"] == summary["RelWithDebInfo"]
    pairs = []
    # Every chunk is compared across independent actual loads/commands, not
    # just geological digests or selected retired meter counters.
    for first, second in zip(records["Debug"]["checkpoints"], records["RelWithDebInfo"]["checkpoints"]):
        assert first["stage"] == second["stage"] and first["snapshot"] == second["snapshot"]
        assert compare_worlds(first["input"], second["input"]) == 126
        pairs.append({"stage": first["stage"], "left": first["input"], "right": second["input"], "complete_chunks_exact": 126})
    builds = []
    # Build diagnostics retain the real target/link result; no compile-only
    # source assumption substitutes for the native afterload evidence above.
    for name in build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CB; original operating machinery retirement acceptance",
              "controller": identity(__file__), "records": list(records.values()), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "complete_cross_build_world_pairs": pairs, "source_receipt": identity(ROOT / ORIGINAL), "builds": builds,
              "native_version": 434, "exact_identities_verified": len(cache),
              "remaining": "1CC actual command-funded paid reconstruction/fresh service and1CD ordinary retired/interrupted/rebuilt files must pass before1.3B/original1.3. Master10.7/runtime art/9.5B and later paid TBM-to-tube conversion/human/operator/network gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(pairs)}), flush=True)


def main():
    """Select one bounded native retirement run or its exact two-build acceptance.

    @return None; missing phase arguments or incomplete actual evidence stop before a receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("retire", "accept"), required=True)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--reports", nargs=2)
    parser.add_argument("--build-logs", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "retire":
        assert args.config is not None; retire(args.config)
    else:
        assert args.reports is not None and args.build_logs is not None and args.output is not None
        accept(args.reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
