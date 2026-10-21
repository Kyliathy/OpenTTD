"""Build or observe one bounded current native conventional transport art scope.

@file ground_transport_runtime_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHC.
@see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
Actual native purchase observations do not establish map-body/foreground,
transported gameplay or human acceptance before those separate checks pass.
"""

import argparse
import json
import re
import subprocess
import time

from ground_pack_check_support import ROOT, handoff
from ground_payload_build_checks import execute
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import reference
from support.artifact_paths import create_run_context

BASELINE = "logs/graphics/selected-ground-runtime/20261021-transport-01/before.json"


def main():
    """Compile or observe one exact configuration with its actual source, application and runtime payload.

    @return None; retain explicit current native process/capture identities under repository logs/reviews.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("build", "purchases", "rail", "road"), required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--run", required=True, help="Explicit repository logs directory")
    parser.add_argument("--source", help="Exact qualified native scene save required for observation")
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("native transport diagnostics belong in repository logs")
    output.mkdir(parents=True, exist_ok=True)
    preserved(BASELINE)
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
            raise ValueError("native transport observation requires an explicit --source checkpoint")
        source = handoff.bound(args.source)
        source_before, application_before = reference(source), reference(application)
        context = create_run_context("selected-ground", "transport", run_id="ground-transport-" + str(time.time_ns()),
                                     root=ROOT, profile="ground-transport-art", process=args.configuration.lower())
        context.prepare_runtime()
        startup = context.runtime_dir / "scripts/game_start.scr"
        original = startup.read_bytes() if startup.exists() else None
        startup.write_text("# @see r2mlvs14-remaining-assets-and-runtime-integration.md79FHC actual native observer.\nground_transport_art\n", encoding="utf-8")
        environment = context.environment()
        environment["OTTD_GROUND_TRANSPORT_SOURCE"] = str(source)
        environment["OTTD_GROUND_TRANSPORT_MODE"] = args.mode
        terminal = output / "native.log"
        attempt = {"_documentation": __doc__, "invocation": context.run_id, "mode": args.mode,
                   "configuration": args.configuration, "source": source_before, "application": application_before,
                   "reports": context.reports.relative_to(ROOT).as_posix(), "images": context.images.relative_to(ROOT).as_posix()}
        (output / "attempt.json").write_text(json.dumps(attempt, indent=2) + "\n", encoding="utf-8")
        started = time.monotonic()
        try:
            with terminal.open("w", encoding="utf-8") as sink:
                result = subprocess.run([str(application), "[.ground-transport-runtime]"], cwd=ROOT / "build", env=environment,
                                        stdout=sink, stderr=subprocess.STDOUT, timeout=60, check=False)
        finally:
            if original is None:
                startup.unlink()
            else:
                startup.write_bytes(original)
        text = terminal.read_text(encoding="utf-8", errors="replace")
        complete = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
        body_frames = 289 if args.mode == "rail" else 210
        marker = ("GROUND_TRANSPORT_PURCHASES models=5 actual_engine_views=10 company_date_guards=true map_coverage=pending"
                  if args.mode == "purchases" else "GROUND_TRANSPORT_BODIES scope={} frames={} packet_owners=true".format(args.mode, body_frames))
        # A source-only matrix or successful empty process cannot qualify the
        # actual engine/company purchase renderer and its native locked cues.
        if result.returncode != 0 or complete is None or marker not in text:
            raise ValueError("native transport purchase observation failed; inspect " + str(terminal))
        report_path = context.reports / ("transport-" + args.mode + ".json")
        report = handoff.read_json(report_path.relative_to(ROOT).as_posix())
        if args.mode == "purchases" and {(row["model"], row["available"]) for row in report["frames"]} != {(model, value) for model in range(4, 9) for value in (False, True)}:
            raise ValueError("actual native transport purchase coverage is incomplete")
        artifacts = [context.images / (row["basename"] + ".ppm") for row in report["frames"]] + [report_path]
        if args.mode != "purchases":
            if len(report["frames"]) != body_frames:
                raise ValueError("actual native transport body matrix is incomplete")
            artifacts += [context.images / (row["basename"] + ".idx") for row in report["frames"] if row.get("context") != "list"]
        for artifact in artifacts:
            if not artifact.is_file():
                raise ValueError("actual native transport artifact is absent")
        runtime_payload = reference(context.runtime_dir / "baseset/openttd.grf")
        if runtime_payload["sha256"] != reference(ROOT / "media/baseset/openttd.grf")["sha256"]:
            raise ValueError("actual native transport payload differs from current qualified publication")
        if reference(source) != source_before or reference(application) != application_before:
            raise ValueError("native transport changed its immutable input or executing image")
        context.write_report("native-" + args.mode + ".json", "src/tests/multilayer/ground_transport_runtime_checks.py", artifacts,
                             [source, application], args.configuration, "openttd_main/native transport renderer")
        observed = {**attempt, "terminal": reference(terminal), "exit_code": result.returncode,
                    "assertions": int(complete.group(1).replace(",", "")), "seconds": round(time.monotonic() - started, 3),
                    "artifacts": [reference(path) for path in artifacts], "runtime_payload": runtime_payload,
                    "actual_purchase_views": 10 if args.mode == "purchases" else 0,
                    "native_body_frames": body_frames if args.mode != "purchases" else 0,
                    "map_body_coverage": args.mode != "purchases", "station_foreground_coverage": False}
    transitions, count = preserved(BASELINE)
    proof = {"_documentation": __doc__, "passed": True, "configuration": args.configuration, "mode": args.mode,
             "application": reference(application), "commands": commands, "observation": observed,
             "baseline": reference(handoff.bound(BASELINE)), "transitions": transitions, "preserved_file_count": count,
             "inputs": [reference(ROOT / path) for path in ("src/tests/ground_transport_runtime.cpp", "CMakeLists.txt")]}
    (output / (args.mode + ".json")).write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    summary = None if observed is None else {key: observed[key] for key in ("invocation", "assertions", "seconds", "actual_purchase_views", "native_body_frames")}
    print(json.dumps({"passed": True, "configuration": args.configuration, "mode": args.mode, "observation": summary}))


if __name__ == "__main__":
    main()
