"""Freeze original paid commissioning after current, legacy, resave and refusal twins agree.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9B
Earlier producer report/save/index identities remain dated immutable boundaries.
This final audit binds current executable/source identities and exact physical owners.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_commissioning_checks-2.py")))
AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
SOURCES = (
    "src/space_mining_tuning.h", "src/space_mining_tuning.cpp", "src/multilayer_space_content.h", "src/multilayer_space_content.cpp",
    "src/multilayer_content.h", "src/multilayer_vehicle_catalogue.cpp", "src/space_production.cpp", "src/space_manifest.cpp",
    "src/space_vehicle.cpp", "src/engine.cpp", "src/space_structure.cpp", "src/orbital_manufacturing.cpp", "src/space_purchase.cpp",
    "src/space_clone.cpp", "src/space_replacement.cpp", "src/space_pod.cpp", "src/space_transition.cpp", "src/orbital_station.cpp",
    "src/orbital_service.cpp", "src/space_structure_gui.cpp", "src/lang/english.txt", "src/saveload/saveload.h",
    "src/saveload/multilayer_sl.cpp", "src/saveload/space_production_sl.cpp", "src/saveload/space_structure_sl.cpp", "src/saveload/space_pod_sl.cpp",
    "src/tests/heavy_commissioning.cpp", "src/tests/heavy_commissioning_native.cpp", "src/tests/space_content.cpp", "src/tests/CMakeLists.txt",
) + tuple(f"src/tests/multilayer/acceptance/heavy_commissioning_checks-{number}.py" for number in range(1, 5))


def read(path):
    """Read one explicitly retained JSON owner.

    @param path Exact retained proof path.
    @return Parsed original object.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def configuration(name, selectors, references):
    """Validate exact current native, ordinary/legacy and independent-refusal membership.

    @param name Matching completed native configuration.
    @param selectors Explicit original/reload/refusal selectors in that order.
    @param references Shared path-to-identity direct artifact audit.
    @return Final configuration row and current physical checkpoint mapping.
    """
    reports = []; processes = {}; states = {}; upgrade = None; totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0, preview_assertions=0)
    # Every scope has its own complete fixed membership; duplicated successful processes or an unrelated old report cannot fill a missing native milestone or reader refusal.
    for scope, selector, count in zip(("native", "reloads", "rejects"), selectors, (9, 11, 11)):
        content = read(selector); assert content["configuration"] == name and content["scope"] == scope and content["complete"] and len(content["reports"]) == count
        AUDIT["inspect_references"](identity(selector), references)
        for member in content["reports"]:
            path = ROOT / member; proof = read(path); assert proof["configuration"] == name
            reports.append(identity(path)); AUDIT["inspect_references"](reports[-1], references); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                native = read(ROOT / child["path"]); assert native["configuration"] == name and child["path"] not in processes
                AUDIT["inspect_references"](native, references)
                intended = scope == "rejects" and native["scenario"].startswith("heavy-commissioning-full-")
                assert (native["exit_status"] != 0) == intended; processes[child["path"]] = dict(report=child, intended_refusal=intended)
            if scope == "native":
                if proof["phase"] == "focused":
                    totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]
                else:
                    assert proof["procurement_acceptance"] is False; states["native", proof["phase"]] = proof["after"]; totals["native_assertions"] += proof["assertions"]
                    if proof["phase"] == "upgrade":
                        upgrade = SHARED["migrate_equal"](ROOT / proof["source"]["path"], ROOT / proof["after"]["path"], "original428-empty-upgrade", name)
                        AUDIT["inspect_references"](upgrade, references)
            elif scope == "reloads":
                assert proof["exact_owner_families"] == len(EXACT)
                states["legacy" if "upgraded" in proof else "ordinary", proof["phase"]] = proof.get("upgraded", proof.get("ordinary"))
                if "resumed" in proof:
                    states["resumed", proof["phase"]] = proof["resumed"]; totals["resumed_assertions"] += proof["assertions"]
            else:
                totals["preview_assertions"] += proof["assertions"]
    assert len(processes) == 48 and sum(item["intended_refusal"] for item in processes.values()) == 11 and len(states) == 25
    assert totals["focused_assertions"] > 0 and totals["focused_cases"] >= 4
    row = dict(configuration=name, save_version=431, content_version=3, reports=reports, selectors=[identity(path) for path in selectors],
               processes=list(processes.values()), ordinary_checkpoints=8, resumed_continuations=6, genuine_legacy_upgrades=4, entry_migration=upgrade, refusal_pairs=11, **totals)
    return row, states


def main():
    """Publish one new immutable dual-configuration commissioning index after exact owner/provenance checks pass."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=3, type=Path, required=True)
    parser.add_argument("--debug", nargs=3, type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references)
    debug, right = configuration("Debug", [path.resolve() for path in args.debug], references); assert left.keys() == right.keys(); comparisons = 0
    # Both builds must converge across every actual job/receipt, ordinary checkpoint, own-resave next action and genuine old pending producer.
    for key in left:
        comparisons += SHARED["compare"](ROOT / left[key]["path"], ROOT / right[key]["path"], ("cross-configuration", key))
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9B; actual independently paid full-bill heavy commissioning/content3/save431, genuine old2 receipt/pending-job conservation and independent native/copied refusals. Upstream Surface starting-content fixture is explicit; procurement acceptance remains false. Heavy Mine/service and processor work remain their later producers.",
                  configurations=[release, debug], sources=[identity(ROOT / name) for name in SOURCES], references=list(references.values()),
                  exact_owner_families=[family.decode("ascii") for family in EXACT], cross_configuration_comparisons=comparisons)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], comparisons, flush=True)


if __name__ == "__main__":
    main()
