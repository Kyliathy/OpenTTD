"""Build or execute one bounded current native default station foreground observation.

@file ground_station_runtime_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Actual loading/appearance captures precede independent source-pixel qualification;
they establish no transported, financial or human acceptance.
"""

import argparse
import json
import re
import subprocess
import time

from ground_pack_check_support import ROOT, handoff
from ground_payload_build_checks import execute
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference
from support.artifact_paths import create_run_context

BASELINE = "logs/graphics/selected-ground-runtime/20261021-station-corrected-01/before.json"


def preserve_completed():
    """Protect completed body/industry source helpers as well as frozen executed images.

    @return Current predecessor-file transitions and count; only root CMake is authorized to change.
    """
    # The inherited freezer's glob covers new test .cpp files, not helper .h
    # files. Verify those explicit completed receipt inputs too, without adding
    # an exemption or pretending changed historical sources belong to this epoch.
    for name in ("transport-bodies", "industry-states"):
        proof = handoff.read_json("logs/proofs/mlm/native-art/ground/ground-consumers-v1/" + name + ".json")
        for row in proof["inputs"]:
            if row["path"] == "CMakeLists.txt":
                continue
            checked_reference(row)
        for row in proof["native"]:
            checked_reference(row["retained_application"])
    return preserved(BASELINE)


def main():
    """Compile or observe one exact current station configuration and immutable native source.

    @return None; native diagnostics and artifacts retain actual source/application/payload identities.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("build", "modules"), required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--run", required=True)
    parser.add_argument("--source", help="Exact qualified eleven-industry native scene; required for observation")
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("station runtime diagnostics belong beneath repository logs")
    output.mkdir(parents=True, exist_ok=True)
    preserve_completed()
    application = ROOT / "build" / args.configuration / "openttd_ground_viewport_test.exe"
    commands, observed = [], None
    if args.mode == "build":
        cache = dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))
        commands.append(execute([cache["CMAKE_COMMAND"], "-S", str(ROOT), "-B", str(ROOT / "build")], ROOT, output / "configure.log"))
        commands.append(execute([cache["CMAKE_COMMAND"], "--build", str(ROOT / "build"), "--config", args.configuration,
                                 "--target", "openttd_ground_viewport_test", "baseset_files", "--parallel", "2", "--",
                                 "/nodeReuse:false", "/p:CL_MPCount=2"], ROOT, output / "build.log"))
    else:
        if args.source is None:
            raise ValueError("native station observation requires an explicit qualified --source checkpoint")
        source = handoff.bound(args.source)
        source_before, application_before = reference(source), reference(application)
        context = create_run_context("selected-ground", "stations", run_id="ground-station-" + str(time.time_ns()),
                                     root=ROOT, profile="ground-station-art", process=args.configuration.lower())
        context.prepare_runtime()
        startup = context.runtime_dir / "scripts/game_start.scr"
        original = startup.read_bytes() if startup.exists() else None
        startup.write_text("# @see r2mlvs14-remaining-assets-and-runtime-integration.md79FHCC native station observer.\nground_station_art\n", encoding="utf-8")
        environment = context.environment()
        environment["OTTD_GROUND_STATION_SOURCE"] = str(source)
        terminal = output / "native.log"
        attempt = {"_documentation": __doc__, "invocation": context.run_id, "configuration": args.configuration,
                   "source": source_before, "application": application_before,
                   "reports": context.reports.relative_to(ROOT).as_posix(), "images": context.images.relative_to(ROOT).as_posix()}
        (output / "attempt.json").write_text(json.dumps(attempt, indent=2) + "\n", encoding="utf-8")
        started = time.monotonic()
        try:
            with terminal.open("w", encoding="utf-8") as sink:
                result = subprocess.run([str(application), "[.ground-station-runtime]"], cwd=ROOT / "build", env=environment,
                                        stdout=sink, stderr=subprocess.STDOUT, timeout=60, check=False)
        finally:
            if original is None:
                startup.unlink()
            else:
                startup.write_bytes(original)
        text = terminal.read_text(encoding="utf-8", errors="replace")
        complete = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
        marker = "GROUND_STATION_MODULES rail_graphics=8 terminal_graphics=6 frames=210 actual_loading_owner=true"
        # An empty native filter or incomplete module/loading run cannot qualify
        # a picture collection as actual station foreground observations.
        if result.returncode != 0 or complete is None or marker not in text:
            raise ValueError("native station observation failed; inspect " + str(terminal))
        report_path = context.reports / "station-modules.json"
        report = handoff.read_json(report_path.relative_to(ROOT).as_posix())
        expected = {(asset, graphics, state, phase, appearance)
                    for asset, count in (("facility.rail-station", 8), ("facility.road-terminal", 6))
                    for graphics in range(count) for state in ("ready", "working")
                    for phase in (range(4) if state == "working" else (0,)) for appearance in range(3)}
        actual = {(row["asset_id"], row["graphics"], row["state"], row["frame"], row["appearance"]) for row in report["frames"]}
        # Every native role, entrance, loading phase and transparency boundary
        # must be present exactly once; duplicate captures cannot fill a gap.
        if len(report["frames"]) != 210 or actual != expected:
            raise ValueError("native station capture matrix is incomplete")
        artifacts = [context.images / (row["basename"] + extension) for row in report["frames"] for extension in (".ppm", ".idx")] + [report_path]
        for artifact in artifacts:
            if not artifact.is_file():
                raise ValueError("actual native station artifact is absent")
        runtime_payload = reference(context.runtime_dir / "baseset/openttd.grf")
        if runtime_payload["sha256"] != reference(ROOT / "media/baseset/openttd.grf")["sha256"]:
            raise ValueError("native station loaded a different publication")
        # Observing presentation may not rewrite its source save or executing
        # image; those are distinct from deliberately staged in-memory fixtures.
        if reference(source) != source_before or reference(application) != application_before:
            raise ValueError("native station modified its source checkpoint or application")
        context.write_report("native-modules.json", "src/tests/multilayer/ground_station_runtime_checks.py", artifacts,
                             [source, application], args.configuration, "openttd_main/CaptureNativeArtViewport/native station and vehicle sorter")
        observed = {**attempt, "terminal": reference(terminal), "exit_code": result.returncode,
                    "assertions": int(complete.group(1).replace(",", "")), "seconds": round(time.monotonic() - started, 3),
                    "artifacts": [reference(path) for path in artifacts], "runtime_payload": runtime_payload,
                    "native_scenes": 210, "source_pixel_verification": "pending"}
    transitions, count = preserve_completed()
    inputs = ("src/tests/ground_station_runtime.cpp", "src/tests/ground_transport_runtime_fixture.h",
              "src/tests/ground_industry_runtime_fixture.h", "src/tests/native_art_render_fixture.h",
              "src/tests/multilayer/ground_station_runtime_checks.py", "CMakeLists.txt")
    proof = {"_documentation": __doc__, "passed": True, "mode": args.mode, "configuration": args.configuration,
             "application": reference(application), "commands": commands, "observation": observed,
             "baseline": reference(handoff.bound(BASELINE)), "transitions": transitions, "preserved_file_count": count,
             "inputs": [reference(ROOT / path) for path in inputs]}
    (output / (args.mode + ".json")).write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    summary = None if observed is None else {key: observed[key] for key in ("invocation", "assertions", "seconds", "native_scenes")}
    print(json.dumps({"passed": True, "mode": args.mode, "configuration": args.configuration, "observation": summary}))


if __name__ == "__main__":
    main()
