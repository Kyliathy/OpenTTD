"""Load the corrected complete candidate through actual isolated native applications.

@file ground_station_offset_native_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCCB.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The candidate replaces only this process's runtime GRF. Ordinary published
media and all historical observer images remain unchanged until qualification.
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

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1"
BASELINE = "logs/graphics/selected-ground-runtime/20261021-station-offset-01/before.json"
MARKER = "GROUND_STATION_OFFSET_PAYLOAD_LOADED sprites=22018 protected=7764 new=14254 families=47 native_start=7791 native_bound=22045 full_pixel_bytes=exact"


def main():
    """Build or run one exact corrected source-loading configuration within a sixty-second native cap.

    @return None; retain actual tested images and candidate/runtime/source identities without publication.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("build", "run"), required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--run", required=True)
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("native offset diagnostics belong beneath repository logs")
    output.mkdir(parents=True, exist_ok=True)
    preserved(BASELINE)
    application = ROOT / "build" / args.configuration / "openttd_ground_viewport_test.exe"
    commands, observed = [], None
    if args.mode == "build":
        cache = dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))
        commands.append(execute([cache["CMAKE_COMMAND"], "-S", str(ROOT), "-B", str(ROOT / "build")], ROOT, output / "configure.log"))
        # Only the new observer relinks. Source qualification must not rebuild
        # or materialize a partly qualified container into ordinary game data.
        commands.append(execute([cache["CMAKE_COMMAND"], "--build", str(ROOT / "build"), "--config", args.configuration,
                                 "--target", "openttd_ground_viewport_test", "--parallel", "2", "--",
                                 "/nodeReuse:false", "/p:CL_MPCount=2"], ROOT, output / "build.log"))
    else:
        build_path = DIRECTORY / "build-proof.json"
        build = handoff.read_json(build_path.relative_to(ROOT).as_posix())
        # An independently decoded complete source/prefix proof precedes the
        # native observation; a plausible candidate hash alone grants nothing.
        if build.get("passed") is not True or build["total_native_sprites"] != 22018 or build["unchanged_native_sprites"] != 21586:
            raise ValueError("corrected native loading lacks complete source/prefix decoding")
        for row in build["inputs"] + [build["report"], build["staging"], build["fixture"]]:
            checked_reference(row)
        candidate = checked_reference(build["builds"][0]["payload"])
        fixture = checked_reference(build["fixture"])
        before_application = reference(application)
        before_published = reference(ROOT / "media/baseset/openttd.grf")
        context = create_run_context("selected-ground", "station-offset-load", run_id="ground-offset-" + str(time.time_ns()),
                                     root=ROOT, profile="ground-offset-art", process=args.configuration.lower())
        context.prepare_runtime()
        runtime_payload = context.runtime_dir / "baseset/openttd.grf"
        runtime_payload.write_bytes(candidate.read_bytes())
        environment = context.environment()
        environment["OTTD_GROUND_OFFSET_FIXTURE"] = str(fixture)
        terminal = output / "native.log"
        attempt = {"_documentation": __doc__, "invocation": context.run_id, "configuration": args.configuration,
                   "application": before_application, "fixture": reference(fixture), "candidate": reference(candidate),
                   "runtime_payload": reference(runtime_payload), "reports": context.reports.relative_to(ROOT).as_posix()}
        (output / "attempt.json").write_text(json.dumps(attempt, indent=2) + "\n", encoding="utf-8")
        started = time.monotonic()
        with terminal.open("w", encoding="utf-8") as sink:
            result = subprocess.run([str(application), "[.ground-station-offset-load]"], cwd=ROOT / "build", env=environment,
                                    stdout=sink, stderr=subprocess.STDOUT, timeout=60, check=False)
        text = terminal.read_text(encoding="utf-8", errors="replace")
        complete = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
        # A partial source loop or successful empty filter cannot authorize a
        # corrected publication containing unobserved native offsets or pixels.
        if result.returncode != 0 or complete is None or MARKER not in text:
            raise ValueError("corrected native source loading failed; inspect " + str(terminal))
        if reference(application) != before_application or reference(ROOT / "media/baseset/openttd.grf") != before_published:
            raise ValueError("native candidate observation changed its image or the ordinary publication")
        retained = DIRECTORY / "bin" / args.configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("retained offset loader has another executed identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        observed = {**attempt, "terminal": reference(terminal), "exit_code": result.returncode,
                    "assertions": int(complete.group(1).replace(",", "")), "seconds": round(time.monotonic() - started, 3),
                    "retained_application": reference(retained), "native_sprites": 22018, "exact_source_pixels_and_offsets": True,
                    "build_proof": reference(build_path), "published_payload_unchanged": before_published}
    transitions, count = preserved(BASELINE)
    proof = {"_documentation": __doc__, "passed": True, "mode": args.mode, "configuration": args.configuration,
             "application": reference(application), "commands": commands, "observation": observed,
             "baseline": reference(handoff.bound(BASELINE)), "preserved_file_count": count, "transitions": transitions,
             "inputs": [reference(ROOT / path) for path in ("src/tests/ground_station_offset_load.cpp",
                        "src/tests/multilayer/ground_station_offset_native_checks.py", "CMakeLists.txt")]}
    (output / (args.mode + ".json")).write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    summary = None if observed is None else {key: observed[key] for key in ("invocation", "assertions", "seconds", "native_sprites")}
    print(json.dumps({"passed": True, "mode": args.mode, "configuration": args.configuration, "observation": summary}))


if __name__ == "__main__":
    main()
