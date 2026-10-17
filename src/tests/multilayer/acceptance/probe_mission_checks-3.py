"""Freeze current mission acceptance after native, ordinary and real network twins agree.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md task8B.4
This is a read-only evidence audit until its final new index write. Dated paid producer and action reports remain immutable input boundaries.
"""
import argparse
import json
from pathlib import Path
import runpy
from PIL import Image

from space_preview_checks import ROOT, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("probe_mission_checks-2.py")))
AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
SOURCES = (
    "src/tests/probe_mission_native.cpp", "src/tests/CMakeLists.txt", "src/space_orders.cpp", "src/space_orders.h",
    "src/asteroid.cpp", "src/asteroid.h", "src/asteroid_gui.cpp", "src/space_vehicle.cpp", "src/space_vehicle.h",
    "src/network/network_command.cpp", "src/network/network_internal.h", "src/command_func.h", "src/saveload/asteroid_sl.cpp",
) + tuple(f"src/tests/multilayer/acceptance/probe_mission_checks-{number}.py" for number in range(1, 4))


def read(path):
    """Read one explicitly retained JSON proof.

    @param path Exact repository proof path.
    @return Original parsed proof object.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def capture_audit(path, references):
    """Revalidate each actual native image's exact producer and unchanged pixels across container conversion.

    @param path Exact current conversion log.
    @param references Shared exact artifact audit.
    @return Number of verified current native captures.
    """
    count = 0
    # Every conversion must retain the actual process-written raw image; a matching PNG filename or visual similarity cannot substitute for native provenance.
    for line in path.read_text(encoding="utf-8").splitlines():
        conversion = json.loads(line); assert conversion["pixels_unchanged"]
        target = {key: conversion[key] for key in ("path", "sha256")}
        AUDIT["inspect_references"]([target, conversion["source_container"], conversion["native_reports"]], references)
        assert conversion["native_reports"]
        for member in conversion["native_reports"]:
            assert conversion["source_container"] in read(ROOT / member["path"])["artifacts"]
        with Image.open(ROOT / target["path"]) as png, Image.open(ROOT / conversion["source_container"]["path"]) as raw:
            assert png.size == raw.size and png.convert("RGB").tobytes() == raw.convert("RGB").tobytes()
        count += 1
    return count


def configuration(name, native_selector, reload_selector, references):
    """Bind one complete native configuration's original, ordinary and resumed evidence.

    @param name Actual current build configuration.
    @param native_selector Exact completed original native membership.
    @param reload_selector Exact completed ordinary/resumed membership.
    @param references Shared exact artifact audit.
    @return Final index row and authoritative checkpoint mapping.
    """
    selectors = [native_selector.resolve(), reload_selector.resolve()]; reports = []; states = {}; processes = {}; images = 0; native_assertions = 0; resumed_assertions = 0
    # Both scopes need their own complete explicit membership. Reusing an unrelated successful native process or an uninterrupted input cannot fill a missing reconstruction.
    for scope, selector in zip(("native", "ordinary"), selectors):
        content = read(selector); assert content["configuration"] == name and content["complete"] and len(content["reports"]) == 13
        AUDIT["inspect_references"](identity(selector), references)
        for member in content["reports"]:
            path = ROOT / member; proof = read(path); assert proof["configuration"] == name
            reports.append(identity(path)); AUDIT["inspect_references"](reports[-1], references); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                native = read(ROOT / child["path"]); assert native["configuration"] == name and native["exit_status"] == 0
                assert child["path"] not in processes; processes[child["path"]] = child; AUDIT["inspect_references"](native, references)
            for child in proof["conversions"]:
                images += capture_audit(ROOT / child["path"], references)
            key = scope, proof["branch"], proof.get("action")
            assert key not in states; states[key] = proof["after"] if scope == "native" else proof["ordinary"]
            if scope == "native":
                native_assertions += proof["assertions"]
            if "resumed" in proof:
                states["resumed", proof["branch"], proof["action"]] = proof["resumed"]; resumed_assertions += proof["assertions"]
    assert len(states) == 32 and len(processes) == 32 and images == 14
    comparisons = 0
    # Original server queues must produce the same physical owners as local posting, including after ordinary reconstruction and the next earned scan/extraction step.
    for key, checkpoint in states.items():
        scope, branch, action = key
        if branch != "network":
            continue
        comparisons += SHARED["compare"](ROOT / states[scope, "local", action]["path"], ROOT / checkpoint["path"], (name, "local-network", scope, action))
    return dict(configuration=name, save_version=430, reports=reports, selectors=[identity(path) for path in selectors], processes=list(processes.values()),
                native_assertions=native_assertions, ordinary_checkpoints=13, resumed_continuations=6, resumed_assertions=resumed_assertions,
                inspector_captures=images, local_network_comparisons=comparisons), states


def main():
    """Freeze both completed native mission configurations and their actual authoritative comparisons in a new final index."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", nargs=2, type=Path, required=True)
    parser.add_argument("--debug", nargs=2, type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", *args.release, references)
    debug, right = configuration("Debug", *args.debug, references); assert left.keys() == right.keys(); comparisons = 0
    # Match every actual paid original, old-world upgrade, native next action and reconstructed checkpoint across both compilers.
    for key in left:
        comparisons += SHARED["compare"](ROOT / left[key]["path"], ROOT / right[key]["path"], ("cross-configuration", key))
    result = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md task8B.4; actual paid Visit(A)/Mine(B), scan cancellation/completion/company lifecycle, pre-knowledge408 migration and original server command dispatch/own-resave continuation. Full multiplayer socket integration remains with its later owning acceptance gate.",
                  configurations=[release, debug], sources=[identity(ROOT / name) for name in SOURCES], references=list(references.values()),
                  disclosure=SHARED["disclosure_audit"](), exact_owner_families=[family.decode("ascii") for family in EXACT], cross_configuration_comparisons=comparisons)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], comparisons, flush=True)


if __name__ == "__main__":
    main()
