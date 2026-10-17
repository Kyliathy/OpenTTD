"""Freeze current typed action evidence after both bounded native configurations pass.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B
Historical producer reports remain immutable boundaries. Current proofs bind their own original saves, native processes and executable identities.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

EXACT = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-1.py")))["EXACT"]
SOURCES = (
    "src/space_order_actions.h", "src/space_order_actions.cpp", "src/space_orders.cpp", "src/space_manifest.cpp",
    "src/launch_center.cpp", "src/fios.h", "src/fios_gui.cpp", "src/saveload/station_sl.cpp",
    "src/saveload/space_manifest_sl.cpp", "src/saveload/saveload.h", "src/CMakeLists.txt", "src/tests/CMakeLists.txt",
    "src/tests/probe_order_actions.cpp", "src/tests/probe_order_actions_native.cpp",
) + tuple(f"src/tests/multilayer/acceptance/probe_role_checks-{number}.py" for number in range(1, 6))


def read(path):
    """Read one explicitly selected accepted JSON owner.

    @param path Exact repository path, without directory guessing.
    @return Parsed original JSON object.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def inspect_references(value, references):
    """Validate every directly retained artifact identity without reopening dated producers' implementation graphs.

    @param value One current proof object or nested list.
    @param references Shared path-to-identity audit for the current final index.
    """
    if isinstance(value, dict):
        # An exact accepted producer report/save is a dated input boundary. Its overwritten old build paths cannot redefine the current physical original.
        if "path" in value and "sha256" in value:
            path = ROOT / value["path"]; assert identity(path) == value, value
            old = references.setdefault(value["path"], value); assert old == value
        for item in value.values():
            inspect_references(item, references)
    elif isinstance(value, list):
        for item in value:
            inspect_references(item, references)


def configuration(name, selectors, references):
    """Audit all explicit accepted scopes and the actual processes that produced their physical outputs.

    @param name Actual completed Release or Debug configuration.
    @param selectors Exact native, reload and rejection selector paths in that order.
    @param references Shared exact artifact audit.
    @return Index row and authoritative checkpoints keyed for cross-configuration comparison.
    """
    selectors = [path.resolve() for path in selectors]
    reports = []; states = {}; processes = {}; totals = {"focused_assertions": 0, "native_assertions": 0, "resumed_assertions": 0}
    # Each scope owns its own explicit membership. A partial selector, duplicated phase or unrelated successful process cannot supply missing acceptance.
    for scope, selector, expected in zip(("native", "reloads", "rejects"), selectors, (16, 13, 10)):
        content = read(selector); assert content["configuration"] == name and content["scope"] == scope and content["complete"]
        assert len(content["reports"]) == expected; inspect_references(identity(selector), references)
        for member in content["reports"]:
            path = ROOT / member; proof = read(path); assert proof["configuration"] == name
            reports.append(identity(path)); inspect_references(reports[-1], references); inspect_references(proof, references)
            for child in proof["native_reports"]:
                native = read(ROOT / child["path"]); assert native["configuration"] == name
                inspect_references(native, references); assert child["path"] not in processes
                # Full malformed-load children intentionally fail before the observer; every ordinary/resumed/focused/preview child must succeed independently.
                expected_failure = scope == "rejects" and native["scenario"].startswith("probe-order-full-")
                assert (native["exit_status"] != 0) == expected_failure
                processes[child["path"]] = dict(report=child, intended_refusal=expected_failure)
            if scope == "native":
                if proof["phase"] == "focused":
                    assert proof["cases"] == 4; totals["focused_assertions"] += proof["assertions"]
                else:
                    totals["native_assertions"] += proof["assertions"]
                    states["native", proof["branch"], proof["phase"]] = proof["after"]
            elif scope == "reloads":
                assert proof["exact_owner_families"] == len(EXACT)
                states["ordinary", proof["branch"], proof["phase"]] = proof["ordinary"]
                if "resumed" in proof:
                    totals["resumed_assertions"] += proof["assertions"]
                    states["resumed", proof["branch"], proof["phase"]] = proof["resumed"]
    assert len(processes) == 57 and sum(item["intended_refusal"] for item in processes.values()) == 10
    assert len(states) == 36
    row = dict(configuration=name, save_version=430, reports=reports, selectors=[identity(path) for path in selectors],
               processes=list(processes.values()), ordinary_checkpoints=13, resumed_continuations=8, refusal_pairs=10, **totals)
    return row, states


def main():
    """Freeze both current action configurations only after all original/native/ordinary/refusal memberships and owner comparisons pass."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", nargs=3, type=Path, required=True, metavar=("NATIVE", "RELOADS", "REJECTS"))
    parser.add_argument("--debug", nargs=3, type=Path, required=True, metavar=("NATIVE", "RELOADS", "REJECTS"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", args.release, references)
    debug, right = configuration("Debug", args.debug, references); assert left.keys() == right.keys(); comparisons = 0
    # The same paid originals must converge across compilers at every native, ordinary and resumed boundary, including real packet provenance and company money.
    for key in left:
        before, after = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in EXACT:
            assert before[family] == after[family], (key, family); comparisons += 1
    sources = [identity(ROOT / name) for name in SOURCES]
    result = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md task8B.2; actual whole freight and once-only paid ordered service. Survey/network task8B.4 remains separate.",
                  configurations=[release, debug], sources=sources, references=list(references.values()),
                  exact_owner_families=[family.decode("ascii") for family in EXACT], cross_configuration_comparisons=comparisons,
                  historical_inputs="Exact dated Plan10 shell424 and Plan11 replacement429 report/save boundaries; their overwritten old build paths are not requalified.")
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], comparisons, flush=True)


if __name__ == "__main__":
    main()
