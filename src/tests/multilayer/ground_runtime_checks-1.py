"""Build, bootstrap or construct the isolated current selected-industry viewport fixture.

@file ground_runtime_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Bootstrap qualifies real application loading/capture only. Fixture mode adds
actual native construction scenes; complete pixel/state/foreground acceptance remains pending.
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

BASELINE = "logs/graphics/selected-ground-runtime/20261021-industry-01/before.json"
SOURCE = ROOT / "src/tests/fixtures/geothermal/fixture389-collector-funded-read.sav"


def main():
    """Build or execute exactly one configuration with immutable input and application provenance.

    @return None; retain actual command/terminal/capture references in the explicit repository logs run.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("build", "bootstrap", "fixture", "reload", "states"), required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--run", required=True, help="Explicit repository logs directory")
    parser.add_argument("--source", help="Exact generated scene checkpoint required only for native reload")
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("ground runtime diagnostics belong in repository logs")
    output.mkdir(parents=True, exist_ok=True)
    transitions, count = preserved(BASELINE)
    application = ROOT / "build" / args.configuration / "openttd_ground_viewport_test.exe"
    commands, observed = [], None
    if args.mode == "build":
        cache = dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))
        commands.append(execute([cache["CMAKE_COMMAND"], "-S", str(ROOT), "-B", str(ROOT / "build")], ROOT, output / "configure.log"))
        commands.append(execute([cache["CMAKE_COMMAND"], "--build", str(ROOT / "build"), "--config", args.configuration,
                                 "--target", "openttd_ground_viewport_test", "baseset_files", "--parallel", "2", "--",
                                 "/nodeReuse:false", "/p:CL_MPCount=2"], ROOT, output / "build.log"))
    else:
        source = SOURCE if args.source is None else handoff.bound(args.source)
        # Reload must name one actual current scene; another source or a
        # convenient latest-run search cannot stand in for native persistence.
        if (args.mode in ("reload", "states")) != (args.source is not None):
            raise ValueError("reload/states modes require an explicit native scene --source")
        source_before = reference(source)
        application_before = reference(application)
        context = create_run_context("selected-ground", "industries", run_id="ground-industry-" + str(time.time_ns()),
                                     root=ROOT, profile="ground-industry-art", process=args.configuration.lower())
        context.prepare_runtime()
        startup = context.runtime_dir / "scripts/game_start.scr"
        original_startup = startup.read_bytes() if startup.exists() else None
        command = {"fixture": "ground_industry_fixture", "reload": "ground_industry_reload",
                   "states": "ground_industry_states"}.get(args.mode, "ground_industry_art")
        startup.write_text("# @see r2mlvs14-remaining-assets-and-runtime-integration.md79FHB isolated native observer.\n" + command + "\n", encoding="utf-8")
        terminal = output / "native.log"
        attempt = {"_documentation": __doc__, "mode": args.mode, "configuration": args.configuration,
                   "invocation": context.run_id, "application": application_before, "source": source_before,
                   "reports": context.reports.relative_to(ROOT).as_posix(), "images": context.images.relative_to(ROOT).as_posix()}
        (output / "attempt.json").write_text(json.dumps(attempt, indent=2) + "\n", encoding="utf-8")
        started = time.monotonic()
        environment = context.environment()
        environment.pop("OTTD_GROUND_INDUSTRY_CHECKPOINT", None)
        if args.mode in ("reload", "states"):
            environment["OTTD_GROUND_INDUSTRY_CHECKPOINT"] = str(source)
        try:
            with terminal.open("w", encoding="utf-8") as sink:
                result = subprocess.run([str(application), "[.ground-industry-runtime]"], cwd=ROOT / "build", env=environment,
                                        stdout=sink, stderr=subprocess.STDOUT, timeout=60, check=False)
        finally:
            # Restore this isolated runtime's copied distribution hook, rather
            # than modifying the shared build scripts or another invocation.
            if original_startup is None:
                startup.unlink()
            else:
                startup.write_bytes(original_startup)
        text = terminal.read_text(encoding="utf-8", errors="replace")
        passed = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
        marker = "GROUND_INDUSTRY_BOOTSTRAP actual_application=true actual_viewport=true new_industry_coverage=false"
        if args.mode in ("fixture", "reload"):
            marker = "GROUND_INDUSTRY_FIXTURE industries=11 construction_viewports=11 owners_unchanged=true source_pixel_verification=pending"
        if args.mode == "states":
            marker = "GROUND_INDUSTRY_STATES industries=11 viewports=110 native_statuses=true owners_unchanged=true source_pixels=pending"
        # A loaded collector bootstrap does not qualify new industry imagery;
        # require its exact bounded process result and preserve that distinction.
        if result.returncode != 0 or passed is None or marker not in text:
            raise ValueError("ground native " + args.mode + " failed; inspect " + str(terminal))
        if reference(source) != source_before or reference(application) != application_before:
            raise ValueError("native bootstrap changed its immutable input or executing image")
        paths = [context.images / "bootstrap-collector.ppm", context.images / "bootstrap-collector.idx"]
        scene = None
        if args.mode in ("fixture", "reload", "states"):
            scene_path = context.reports / "industry-fixture.json"
            scene = handoff.read_json(scene_path.relative_to(ROOT).as_posix())
            # Saved native bindings and construction flags must survive actual
            # application reload without rebuilding any displayed facility.
            if args.mode == "reload" and (scene.get("loaded_checkpoint") is not True or
                    "GROUND_INDUSTRY_RELOAD actual_saved_mines=7 actual_saved_processors=4 replacement_funding=false" not in text):
                raise ValueError("native selected industry checkpoint was not actually reloaded")
            if args.mode == "states" and scene.get("full_state_coverage") is not True:
                raise ValueError("native current selected industry state coverage is incomplete")
            dispatch = handoff.read_json("media/multilayer/contracts/ground-consumer-dispatch.json")
            selected = {row["asset_id"] for row in dispatch["families"] if row.get("binding_header") in
                        ("src/ground_mine_sprite.h", "src/ground_processor_sprite.h")}
            # Require every actually bound selected industry, rather than a
            # hand-shortened list or the retained collector standing in for it.
            expected_frames = 110 if args.mode == "states" else 11
            if len(scene["frames"]) != expected_frames or {row["asset_id"] for row in scene["frames"]} != selected:
                raise ValueError("native current selected industry scene is incomplete")
            for frame in scene["frames"]:
                paths.extend(context.images / (frame["basename"] + suffix) for suffix in (".ppm", ".idx"))
            paths.append(scene_path)
            if args.mode != "states":
                paths.append(context.saves / "selected-industries-fixture.sav")
        for path in paths:
            if not path.is_file():
                raise ValueError("ground actual native capture is missing: " + str(path))
        context.write_report("native-" + args.mode + ".json", "src/tests/multilayer/ground_runtime_checks-1.py", paths,
                             [source, application], args.configuration,
                             "openttd_main/CaptureGroundIndustryFixtures/CaptureNativeArtViewport/SaveWithFilter" if scene else "CaptureNativeArtViewport")
        observed = {"invocation": context.run_id, "configuration": args.configuration, "source": source_before,
                    "application": application_before, "terminal": reference(terminal), "exit_code": result.returncode,
                    "seconds": round(time.monotonic() - started, 3), "assertions": int(passed.group(1).replace(",", "")),
                    "captures": [reference(path) for path in paths], "actual_application": True, "actual_viewport": True,
                    "native_construction_industries": len({row["asset_id"] for row in scene["frames"]}) if scene else 0,
                    "native_scenes": len(scene["frames"]) if scene else 0,
                    "source_pixel_verification": "pending", "full_state_coverage": scene.get("full_state_coverage", False) if scene else False,
                    "new_industry_coverage": False, "transported_bootstrap_claimed": False}
        runtime_payload = reference(context.runtime_dir / "baseset/openttd.grf")
        published_payload = reference(ROOT / "media/baseset/openttd.grf")
        if runtime_payload["sha256"] != published_payload["sha256"]:
            raise ValueError("actual native viewport payload differs from qualified current publication")
        observed.update(runtime_payload=runtime_payload, published_payload=published_payload)
    transitions, count = preserved(BASELINE)
    proof = {"_documentation": __doc__, "passed": True, "configuration": args.configuration, "mode": args.mode,
             "application": reference(application), "commands": commands, "observation": observed,
             "baseline": reference(handoff.bound(BASELINE)), "preserved_file_count": count, "transitions": transitions,
             "inputs": [reference(path) for path in (ROOT / "src/tests/ground_industry_runtime.cpp", ROOT / "CMakeLists.txt", SOURCE)]}
    (output / (args.mode + ".json")).write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    summary = None if observed is None else {key: observed[key] for key in
               ("invocation", "assertions", "seconds", "native_construction_industries", "source_pixel_verification", "full_state_coverage")}
    print(json.dumps({"passed": True, "configuration": args.configuration, "mode": args.mode, "observation": summary}))


if __name__ == "__main__":
    main()
