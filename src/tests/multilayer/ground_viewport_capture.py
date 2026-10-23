"""Execute original bounded Ground native observers with current explicit inputs.

@file ground_viewport_capture.py Actual native rasters and independent original pixel obligations.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group31 cold runtime preparation.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import contained
from artifact_rebuild import identity
from runner import run_native, save_native_capture

MODES = {
    "collector": dict(tag="[.ground-industry-runtime]", command="ground_industry_art", hook="OTTD_GROUND_INDUSTRY_SOURCE",
        scene=None, marker="GROUND_INDUSTRY_BOOTSTRAP actual_application=true actual_viewport=true new_industry_coverage=false"),
    "industry-states": dict(tag="[.ground-industry-runtime]", command="ground_industry_states", hook="OTTD_GROUND_INDUSTRY_SOURCE",
        scene="industry-fixture.json", marker="GROUND_INDUSTRY_STATES industries=11 viewports=110 native_statuses=true owners_unchanged=true source_pixels=pending"),
    "purchases": dict(tag="[.ground-transport-runtime]", command="ground_transport_art", hook="OTTD_GROUND_TRANSPORT_SOURCE",
        scene="transport-purchases.json", marker="GROUND_TRANSPORT_PURCHASES models=5 actual_engine_views=10 company_date_guards=true map_coverage=pending"),
    "rail": dict(tag="[.ground-transport-runtime]", command="ground_transport_art", hook="OTTD_GROUND_TRANSPORT_SOURCE",
        scene="transport-rail.json", marker="GROUND_TRANSPORT_BODIES scope=rail frames=289 packet_owners=true"),
    "road": dict(tag="[.ground-transport-runtime]", command="ground_transport_art", hook="OTTD_GROUND_TRANSPORT_SOURCE",
        scene="transport-road.json", marker="GROUND_TRANSPORT_BODIES scope=road frames=210 packet_owners=true"),
    "stations": dict(tag="[.ground-station-runtime]", command="ground_station_art", hook="OTTD_GROUND_STATION_SOURCE",
        scene="station-modules.json", marker="GROUND_STATION_MODULES rail_graphics=8 terminal_graphics=6 frames=210 actual_loading_owner=true"),
}


def verify_original_rasters(context, process, world):
    """Require every actual native raster to retain the independent original complete byte stream.

    @param context Exact feature/scenario/native capture invocation.
    @param process Actual successful shared-runner process record, including its real artifacts.
    @param world Independently retained original complete raster fingerprints for this bounded mode.
    @return Actual raw native raster paths after complete domain/content validation.
    """
    actual = {}
    # A native result cannot inherit another mode's frames or hide missing,
    # duplicate or additional rasters behind a plausible source-pixel count.
    for reference in process["artifacts"]:
        if Path(reference["path"]).suffix not in (".idx", ".ppm"):
            continue
        path = contained(context.root / reference["path"], context.images)
        if path.parent != context.images or path.name in actual:
            raise ValueError("Ground native raster has a foreign or duplicate capture owner")
        if identity(path, context.root) != reference:
            raise ValueError("Ground native raster changed after its actual process")
        actual[path.name] = path
    if set(actual) != set(world["expected_captures"]):
        raise ValueError("Ground native complete raster domain differs from original bounded scope")
    # These fingerprints come from pre-cleanup actual accepted executions, not
    # this renderer. Compare complete RGB/indexed containers as well as the
    # independent source/composition algorithms that follow this gate.
    for name, path in actual.items():
        expected = world["expected_captures"][name]
        if (path.stat().st_size != expected["bytes"]
                or identity(path, context.root)["sha256"] != expected["sha256"]):
            raise ValueError("Ground native raster differs from original complete capture: " + name)
    return actual


def verify_pixels(mode, execution, source_view, context):
    """Call original exact pixel/composition owners with explicit selected current source views.

    @param mode Original bounded native collector/industry/purchase/body/station mechanism.
    @param execution Actual current wrapper execution report, using its original proof schema.
    @param source_view Independent checked original Ground/M1 source poses and atlases.
    @param context Current output invocation owning the actual native observations.
    @return Original exact source/pixel/composition result, without human or operating acceptance.
    """
    relative = execution.relative_to(context.root).as_posix()
    if mode == "collector":
        from ground_selected_closure_checks import collector_pixels
        return dict(passed=True, **collector_pixels(context.images / "bootstrap-collector.idx", source_view))
    if mode == "industry-states":
        from ground_industry_pixel_checks import verify
    elif mode == "purchases":
        from ground_transport_pixel_checks import verify_purchases as verify
    elif mode in ("rail", "road"):
        from ground_transport_body_pixel_checks import verify
    elif mode == "stations":
        from ground_station_pixel_checks import verify
    else:
        raise ValueError("unknown original Ground pixel mechanism")
    return verify(relative, source_view)


def run_capture(context, mode, configuration, application, source, world, source_view, producer):
    """Run one actual original native observer and seal its independent raster/source obligations.

    @param context Fresh explicit feature/scenario/configuration-specific capture/runtime owner.
    @param mode Exactly one original bounded capture mechanism from MODES.
    @param configuration Actual independently built current native configuration.
    @param application Explicit current original viewport executable from the selected build result.
    @param source Exact hash-verified retained original review world, never an active continuation.
    @param world Independently retained original source/raster comparison contract for this mode.
    @param source_view Current checked original Ground/M1 source view with complete independent poses.
    @param producer Original registered Python wrapper actually delegating this observer.
    @return Actual native process/frame/input/pixel identities and scope disclosures.
    """
    specification = MODES[mode]
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    # Cold runtimes need their private script owner before the observer can start.
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2fc-file-cleanup3.md20BB; isolated original art observer only.\n"
                       + specification["command"] + "\n", encoding="utf-8")
    source_before = identity(source, context.root)
    application_before = identity(application, context.root)
    canonical = identity(context.root / "media/baseset/openttd.grf", context.root)
    runtime = identity(context.runtime_dir / "baseset/openttd.grf", context.root)
    if runtime["sha256"] != canonical["sha256"]:
        raise ValueError("Ground viewport runtime differs from current canonical content")
    environment = context.environment()
    # Each real observer consumes exactly its own retained world hook. A stale
    # sibling source cannot become an extra claimed native input in this run.
    for hook in ("OTTD_GROUND_INDUSTRY_SOURCE", "OTTD_GROUND_TRANSPORT_SOURCE", "OTTD_GROUND_STATION_SOURCE"):
        environment.pop(hook, None)
    environment[specification["hook"]] = str(source)
    environment["OTTD_TEST_CONFIGURATION"] = configuration
    if mode in ("purchases", "rail", "road"):
        environment["OTTD_GROUND_TRANSPORT_MODE"] = mode
    terminal = context.evidence / "native-viewport.txt"
    started = time.monotonic()
    with terminal.open("x", encoding="utf-8") as output:
        result = run_native(context, [str(application), specification["tag"], "--reporter", "console", "--durations", "yes"],
            native_producer=producer, cwd=context.root / "build", env=environment,
            stdout=output, stderr=subprocess.STDOUT, timeout=60, check=False)
    elapsed = round(time.monotonic() - started, 3)
    text = terminal.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
    # An empty filter, another successful case or partial fixture cannot
    # qualify this original native loading/drawing/owner boundary.
    if result.returncode != 0 or passed is None or specification["marker"] not in text:
        raise ValueError("actual original Ground viewport case failed: " + str(terminal))
    process = json.loads((context.root / result.native_report["path"]).read_text(encoding="utf-8"))
    if (process["producer"] != producer or process["executable"] != application_before
            or process["configuration"] != configuration or process["inputs"] != [source_before]):
        raise ValueError("Ground viewport actual process/application/source provenance disagrees")
    requested_terminal = identity(terminal, context.root)
    # Shared runner retains the exact stdout segment under its own process ID.
    # Seal that actual transcript identity; the requested stream remains the
    # declared evidence output and must contain exactly the same native bytes.
    if len(process["transcripts"]) != 1 or process["transcripts"][0]["sha256"] != requested_terminal["sha256"]:
        raise ValueError("Ground viewport retained stdout differs from its actual native stream")
    rasters = verify_original_rasters(context, process, world)
    if specification["scene"] is not None:
        scene = context.reports / specification["scene"]
        if identity(scene, context.root) not in process["artifacts"]:
            raise ValueError("Ground native scene metadata is not an actual child artifact")
    artifacts = [row for row in process["artifacts"] if Path(row["path"]).suffix in (".idx", ".ppm", ".json")]
    observed = dict(source=source_before, application=application_before, exit_code=0, artifacts=artifacts,
        captures=artifacts, terminal=identity(terminal, context.root), actual_purchase_views=10 if mode == "purchases" else 0,
        native_body_frames=289 if mode == "rail" else 210 if mode == "road" else 0)
    execution = context.reports / "actual-execution.json"
    execution.write_text(json.dumps(dict(_documentation=__doc__, passed=True, mode=mode, configuration=configuration,
        application=application_before, observation=observed, native_process=result.native_report), indent=2) + "\n", encoding="utf-8")
    pixels = verify_pixels(mode, execution, source_view, context)
    if pixels.get("passed") is not True:
        raise ValueError("original Ground source/composition verifier did not pass")
    source_view.verify_current()
    pixel_path = context.reports / "source-pixels.json"
    pixel_path.write_text(json.dumps(pixels, indent=2) + "\n", encoding="utf-8")
    frames = []
    # PNG is a lossless native-container conversion. Raw indexed buffers stay
    # actual child artifacts, and full original PPM/IDX bytes remain available
    # in this cleanable run for independent re-verification until cleanup.
    for name, path in sorted(rasters.items()):
        if path.suffix != ".ppm":
            continue
        target = path.with_suffix(".png")
        with Image.open(path) as raster:
            save_native_capture(context, raster, path, target)
            frame = dict(identity(target, context.root), feature=context.feature, scenario=context.scenario,
                         run_id=context.run_id, dimensions=list(raster.size),
                         rgb_sha256=hashlib.sha256(raster.convert("RGB").tobytes()).hexdigest())
        indices = path.with_suffix(".idx")
        if indices.name in rasters:
            frame["native_indices"] = identity(indices, context.root)
        frames.append(frame)
    # Drawing cannot publish art, rewrite the original review world, replace
    # its current executable or alter the canonical/shared runtime content.
    for path, expected in ((source, source_before), (application, application_before),
                           (context.root / "media/baseset/openttd.grf", canonical),
                           (context.runtime_dir / "baseset/openttd.grf", runtime)):
        if identity(path, context.root) != expected:
            raise ValueError("Ground viewport changed a protected original/native/content input")
    return dict(native_process=result.native_report, native_terminal=process["transcripts"][0],
        requested_terminal=requested_terminal,
        actual_execution=identity(execution, context.root), source_pixels=identity(pixel_path, context.root),
        frames=frames, source=source_before, runtime_payload=runtime, runtime_config=identity(context.config_path, context.root),
        startup=identity(startup, context.root), application=application_before,
        native_assertions=int(passed.group(1).replace(",", "")), native_elapsed_seconds=elapsed,
        complete_original_rasters=len(rasters), exact_source_pixels_verified=True,
        draw_owners_unchanged=True, art_fixture_staging=mode != "collector", paid_progress_claimed=False,
        ordinary_game_reviewed=False, human_acceptance=False)
