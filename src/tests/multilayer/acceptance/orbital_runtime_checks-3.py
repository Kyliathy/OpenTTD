"""Qualify bounded native processing work, ordinary files and independent full/copy refusals.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C
Processor fixture inputs are explicitly isolated; no paid native processor procurement claim is made here.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("orbital_runtime_checks-1.py")))
PHASES = {"partial", "complete", "cancelled", "unpowered", "retired", "fractional"}
FAULTS = {"paid", "history", "residue", "future-opportunity", "consumer", "duplicate-owner", "missing"}
PHYSICAL = (b"SPST", b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF",
            b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND",
            b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"TUPH", b"TUPD", b"TUVH", b"TUVD",
            b"ENGN", b"EIDS", b"ERNW", b"GRPS")
METADATA = (b"CDIM", b"SPRC")
SOURCES = ("src/CMakeLists.txt", "src/tests/CMakeLists.txt", "src/saveload/CMakeLists.txt",
           "src/layer_recipe_work.h", "src/layer_recipe_work.cpp", "src/layer_industry_recipe.cpp", "src/multilayer_industry.h",
           "src/multilayer_recipe.h", "src/multilayer_recipe.cpp", "src/orbital_recipe.h", "src/orbital_recipe.cpp",
           "src/orbital_recipe_runtime.cpp", "src/orbital_recipe_record.cpp", "src/orbital_station.h", "src/orbital_station.cpp",
           "src/space_structure.h", "src/space_structure.cpp", "src/space_structure_cmd.h", "src/orbital_service.cpp",
           "src/transport_tube_runtime.cpp", "src/utility_power.h", "src/utility_power.cpp", "src/company_cmd.cpp", "src/command_type.h",
           "src/fios.h", "src/fios_gui.cpp", "src/saveload/saveload.h", "src/saveload/saveload.cpp", "src/saveload/afterload.cpp",
           "src/saveload/orbital_recipe_sl.cpp", "src/saveload/space_structure_sl.cpp", "src/saveload/space_cargo_preview.cpp",
           "src/tests/orbital_recipe_inventory.cpp", "src/tests/orbital_recipe_files.cpp", "src/tests/excavation_fixture.h",
           "src/tests/layer_save_fixture.h", "src/tests/layer_recipe_work.cpp", "src/tests/heavy_joined_native.cpp",
           "src/tests/multilayer/acceptance/orbital_runtime_checks-1.py", "src/tests/multilayer/acceptance/orbital_runtime_checks-2.py",
           "src/tests/multilayer/acceptance/orbital_runtime_checks-3.py", "src/tests/multilayer/acceptance/space_preview_checks.py",
           "src/tests/multilayer/acceptance/launch_center_checks.py", "src/tests/multilayer/support/runner.py",
           "src/tests/multilayer/support/artifact_paths.py", "src/tests/test_artifact_paths.cpp")


def read(path):
    """Read one explicit original evidence file.

    @param path Exact retained selector/report path.
    @return Parsed unchanged evidence, never newest-file selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def inspect(value, references):
    """Independently verify every retained direct artifact identity.

    @param value Original report/identity tree.
    @param references Shared deduplicated checked direct identities.
    """
    if isinstance(value, dict):
        # A dated index/report is checked only as a direct immutable boundary. Its historical executables/source graphs are never recursively reopened or requalified.
        if "path" in value and "sha256" in value:
            actual = identity(ROOT / value["path"])
            assert actual == {key: value[key] for key in ("path", "sha256")}
            assert references.get(actual["path"], actual) == actual
            references[actual["path"]] = actual
        # Each direct nested identity participates in the exact current invocation's claimed evidence.
        for child in value.values():
            inspect(child, references)
    elif isinstance(value, list):
        for child in value:
            inspect(child, references)


def qualify(configuration, path, references):
    """Qualify an actual current process, including explicitly intended ordinary-reader refusal.

    @param configuration Exact current test executable build.
    @param path Explicit original scoped proof report.
    @param references Shared checked identity graph.
    @return Original qualified semantic proof.
    """
    proof = read(path)
    inspect(proof, references)
    assert proof["configuration"] == configuration and len(proof["native_reports"]) == 1
    process = read(ROOT / proof["native_reports"][0]["path"])
    inspect(process, references)
    assert process["configuration"] == configuration and process["executable"] == identity(ROOT / "build" / configuration / "openttd_test.exe")
    assert process["exit_status"] == (1 if proof["expected_full_refusal"] else 0)
    expected = SHARED["TAGS"] if proof["scope"] == "focused" else "[.heavy-joined-native]" if proof["scope"] == "legacy" else "[.orbital-recipe-" + proof["scope"] + "]"
    assert process["argv"][1:] == [expected]
    terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
    if proof["expected_full_refusal"]:
        assert "Broken savegame" in terminal and "Fatal test error" not in terminal and "Assertion failed" not in terminal
    else:
        assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
    # Exact positive saved outputs must retain the new required metadata, rather than merely carrying a successful process marker.
    for saved in proof["saves"]:
        assert chunks(ROOT / saved["path"])[0] == 438 and b"SPRC" in chunks(ROOT / saved["path"])[1]
    proof["_path"] = identity(path)
    return proof


def compare(left, right):
    """Require every original physical owner and configured processing metadata to remain byte-exact.

    @param left Exact immutable source/checkpoint identity.
    @param right Matching independently written ordinary checkpoint identity.
    """
    before, after = chunks(ROOT / left["path"])[1], chunks(ROOT / right["path"])[1]
    # All thirty physical families retain their original exclusive owners; processing adds metadata rather than another cargo inventory.
    for family in (*PHYSICAL, *METADATA):
        assert family in before and family in after and before[family] == after[family], (left["path"], right["path"], family)


def configuration(name, selectors, references):
    """Qualify one complete focused/ordinary/refusal/legacy acceptance set.

    @param name Exact current build configuration.
    @param selectors Explicit final positive and negative scope membership; abandoned diagnostics are excluded.
    @param references Shared direct identity graph.
    @return Qualified configuration summary and exact proof/process membership.
    """
    reports, faults = [], {}
    # Only explicit selected reports enter acceptance. Each malformed pair must retain both its independent actual copied and full reader processes.
    for selector in selectors:
        selected = read(selector)
        assert selected["configuration"] == name and selected["complete"]
        if "fault" in selected:
            assert selected["fault"] not in faults and len(selected["reports"]) == 2
            faults[selected["fault"]] = selected
        for report in selected["reports"]:
            reports.append(qualify(name, ROOT / report, references))
    focused = [p for p in reports if p["scope"] == "focused"]
    assert len({p["_path"]["path"] for p in reports}) == len(reports) == 34
    assert len(focused) == 1 and focused[0]["cases"] == 32 and focused[0]["assertions"] > 3549
    exports = {p["phase"]: p for p in reports if p["scope"] == "export"}
    assert set(exports) == PHASES and len([p for p in reports if p["scope"] == "export"]) == len(PHASES)
    assert all(p["source"] == identity(SHARED["BASE_SOURCE"]) and p["declared_unit_inputs"] for p in exports.values())
    ordinary = [p for p in reports if p["scope"] == "file" and not p["expected_full_refusal"] and p["copied_candidate"] is None]
    assert len(ordinary) == 2 * len(PHASES)
    # Each phase must first load its actual export, then independently load that process's own ordinary resave and repeat the same continuation.
    checkpoints = {}
    for phase in sorted(PHASES):
        rows = [p for p in ordinary if p["phase"] == phase]
        assert len(rows) == 2 and len(exports[phase]["saves"]) == 1
        first = next(p for p in rows if p["source"] == exports[phase]["saves"][0])
        own = next(s for s in first["saves"] if Path(s["path"]).name == "orbital-recipe-own.sav")
        second = next(p for p in rows if p is not first)
        assert second["source"] == own
        second_own = next(s for s in second["saves"] if Path(s["path"]).name == "orbital-recipe-own.sav")
        after = next(s for s in first["saves"] if Path(s["path"]).name == "orbital-recipe-continued.sav")
        repeated = next(s for s in second["saves"] if Path(s["path"]).name == "orbital-recipe-continued.sav")
        compare(exports[phase]["saves"][0], own)
        compare(own, second_own)
        compare(after, repeated)
        checkpoints.update({phase + "/" + stage: saved for stage, saved in
                            (("export", exports[phase]["saves"][0]), ("original/own", own), ("original/continued", after),
                             ("own/own", second_own), ("own/continued", repeated))})
    assert set(faults) == FAULTS
    qualified = {p["_path"]["path"]: p for p in reports}
    # A pair belongs to its own independently damaged original file; seven differently named selectors cannot reuse the same actual refusal as substitute coverage.
    for selected in faults.values():
        copied, full = (qualified[path] for path in selected["reports"])
        assert not copied["expected_full_refusal"] and full["expected_full_refusal"]
        assert copied["copied_candidate"] == full["source"] == selected["candidate"]
        assert copied["source"] == selected["source"] == exports["partial"]["saves"][0]
    malformed = [p for p in reports if p["copied_candidate"] is not None or p["expected_full_refusal"]]
    assert len(malformed) == 2 * len(FAULTS)
    legacy = [p for p in reports if p["scope"] == "legacy"]
    assert len(legacy) == 1 and legacy[0]["dated_producer"]["index"]["sha256"] == SHARED["LEGACY_INDEX_SHA"]
    return dict(configuration=name, selectors=[identity(p.resolve()) for p in selectors], reports=[p["_path"] for p in reports],
                focused_assertions=focused[0]["assertions"], focused_cases=focused[0]["cases"], processes=len(reports),
                ordinary_processing_loads=len(ordinary), isolated_exports=len(exports), independent_full_copied_pairs=len(faults), legacy_loads=1,
                physical_owner_comparisons=18 * len(PHYSICAL), metadata_comparisons=18 * len(METADATA),
                checkpoints=checkpoints, records=reports)


def main():
    """Freeze both current complete bounded acceptance sets with explicit declared-fixture provenance."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs="+", required=True)
    parser.add_argument("--debug", type=Path, nargs="+", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = args.output.resolve()
    assert not target.exists()
    references = {}
    release = configuration("RelWithDebInfo", args.release, references)
    debug = configuration("Debug", args.debug, references)
    assert (release["focused_assertions"], release["focused_cases"]) == (debug["focused_assertions"], debug["focused_cases"])
    assert release["checkpoints"].keys() == debug["checkpoints"].keys()
    # Every original export, ordinary resave and repeated continuation must converge between configurations, including physical packets, money, utility and processing history.
    for key, left in release["checkpoints"].items():
        compare(left, debug["checkpoints"][key])
    result = dict(_documentation=__doc__, save_version=438, configurations=[release, debug],
                  sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()),
                  dated_producer_indexes=[identity(ROOT / "logs/orbital-inventory-verification.json"), identity(ROOT / "logs/orbital-recipe-work-verification.json")],
                  physical_owner_families=[family.decode("ascii") for family in PHYSICAL],
                  cross_configuration_physical_comparisons=len(release["checkpoints"]) * len(PHYSICAL),
                  cross_configuration_metadata_comparisons=len(release["checkpoints"]) * len(METADATA), native_paid_processor_procurement=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], flush=True)


if __name__ == "__main__":
    main()
