"""Freeze current heavy mining after focused, native, ordinary and refusal twins agree.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C
Paid9B inputs remain immutable dated boundaries; no processor, Surface descent or heavy service acceptance is inferred.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_mining_runtime_checks-1.py")))
AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
SOURCES = (
    "src/space_mining_tuning.h", "src/space_mining_tuning.cpp", "src/space_vehicle.h", "src/space_vehicle.cpp", "src/space_orders.cpp",
    "src/space_structure.cpp", "src/fios.h", "src/saveload/space_order_sl.cpp", "src/saveload/vehicle_sl.cpp", "src/saveload/saveload.h",
    "src/tests/heavy_mining_runtime.cpp", "src/tests/heavy_mining_runtime_native.cpp", "src/tests/probe_shell.cpp", "src/tests/CMakeLists.txt",
    "docs/zz-rhr/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md",
) + tuple(f"src/tests/multilayer/acceptance/heavy_mining_runtime_checks-{number}.py" for number in range(1, 4))


def configuration(name, selectors, references):
    """Audit one complete current configuration's explicit original, resumed and refusal evidence.

    @param name Actual completed configuration.
    @param selectors Exact native, reload and refusal selectors in that order.
    @param references Shared direct path/hash audit; dated upstream graphs are not reopened.
    @return Accepted configuration row and exact physical checkpoint mapping.
    """
    reports = []; processes = {}; states = {}; totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0, preview_assertions=0)
    # Fixed complete membership prevents a failed stage, unrelated successful process or duplicated checkpoint from silently filling a missing native boundary.
    for scope, selector, count in zip(("native", "reloads", "rejects"), selectors, (5, 4, 6)):
        selected = SHARED["read"](selector); assert selected["configuration"] == name and selected["scope"] == scope and selected["complete"] and len(selected["reports"]) == count
        AUDIT["inspect_references"](identity(selector), references)
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["read"](path); assert proof["configuration"] == name; reports.append(identity(path))
            AUDIT["inspect_references"](reports[-1], references); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                native = SHARED["read"](ROOT / child["path"]); assert native["configuration"] == name and child["path"] not in processes
                AUDIT["inspect_references"](native, references)
                intended = scope == "rejects" and native["scenario"].startswith("heavy-mining-runtime-full-")
                assert (native["exit_status"] != 0) == intended; processes[child["path"]] = dict(report=child, intended_refusal=intended)
            if scope == "native":
                if proof["phase"] == "focused":
                    totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]
                else:
                    assert proof["procurement_acceptance"] is False; states["native", proof["phase"]] = proof["after"]; totals["native_assertions"] += proof["assertions"]
            elif scope == "reloads":
                assert proof["exact_owner_families"] == len(EXACT); states["ordinary", proof["phase"]] = proof["ordinary"]
                totals["resumed_assertions"] += proof["assertions"]
                if "resumed" in proof:
                    states["resumed", proof["phase"]] = proof["resumed"]
            else:
                totals["preview_assertions"] += proof["assertions"]
    assert len(processes) == 24 and sum(value["intended_refusal"] for value in processes.values()) == 6 and len(states) == 11
    assert totals["focused_cases"] >= 3 and totals["focused_assertions"] > 0
    return dict(configuration=name, save_version=432, content_version=3, reports=reports, selectors=[identity(path) for path in selectors], processes=list(processes.values()),
                ordinary_checkpoints=4, resumed_continuations=3, refusal_pairs=6, **totals), states


def main():
    """Publish one new immutable dual-build index after actual native owners and directly retained artifacts agree."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=3, type=Path, required=True)
    parser.add_argument("--debug", nargs=3, type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references)
    debug, right = configuration("Debug", [path.resolve() for path in args.debug], references); assert left.keys() == right.keys(); comparisons = 0
    # Compare complete actual physical families across configurations, including own-resave next actions; matching scalar mission summaries are insufficient.
    for key in left:
        before = chunks(ROOT / left[key]["path"])[1]; after = chunks(ROOT / right[key]["path"])[1]
        for family in EXACT:
            assert before[family] == after[family], (key, family); comparisons += 1
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9C; actual paid heavy finite mining and sustainable explicit orbital return, exact ordinary/resumed owner custody and independently rejected malformed states. Surface pod return/heavy service remain9CB/9CC; joined disclosures and integrated network/paid-cycle acceptance remain9CD/9D. Inherited starting content is not procurement acceptance.",
                  configurations=[release, debug], sources=[identity(ROOT / path) for path in SOURCES], exact_owner_families=len(EXACT), cross_configuration_comparisons=comparisons,
                  direct_references=list(references.values()), entry_index=identity(ROOT / "logs/heavy-commissioning-verification.json"), procurement_acceptance=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(json.dumps(dict(index=identity(target), owners=len(EXACT), comparisons=comparisons, configurations=[release["configuration"], debug["configuration"]])), flush=True)


if __name__ == "__main__":
    main()
