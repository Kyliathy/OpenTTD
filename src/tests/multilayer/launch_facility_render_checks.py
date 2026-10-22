"""Qualify actual native occupied viewport rasters against frozen launch sources.

@file launch_facility_render_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JB.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19B.
The retained paid host is immutable. Explicit component pose inputs claim no
ordinary paid progress; the native renderer runs with its actual 8bpp cache.
"""
import argparse
from collections import Counter
import copy
import json
from pathlib import Path
import subprocess
import re
import sys
from PIL import Image, ImageDraw
sys.path.insert(0, str(Path(__file__).resolve().parent / "support"))
from artifact_paths import contained
from artifact_rebuild import identity
from runner import run_native, save_native_capture
import launch_facility_selector_checks as component

ROOT = component.ROOT
HOST = "mlm/reviews/space/paid-probe-phase-native-preparing/20261014-122357-441519346a/saves/probe-flight-preparing-before-423.sav"
HOST_SHA = "ce2a813ee83b867133e63e25917221fa6ceec054c2b1020796b9b01a630fa460"


def bound_image(reference):
    """Read an immutable accepted indexed source after its recorded hash passes.

    @param reference Exact path/SHA from the frozen source handoff or report.
    @return Detached original image including its native DOS palette.
    @throws ValueError On stale or substituted source bytes.
    """
    path = contained(ROOT / reference["path"], ROOT)
    if identity(path, ROOT) != reference:
        raise ValueError("frozen raster source changed: " + reference["path"])
    with Image.open(path) as image:
        return image.copy()


def expected_scene(entry, facility, rockets, atlas):
    """Compose original per-zone cuts independently of native sorting bounds.

    @param entry Native observation identifying pose, physical contacts and visibility.
    @param facility Immutable complete facility source report with original ownership maps.
    @param rockets Immutable complete rocket source report.
    @param atlas Original qualified rocket indexed atlas.
    @return Expected indexed source scene and its placement in the actual viewport.
    @throws ValueError On a changed source mask or an unqualified pose.
    """
    art = component.art
    direction = ("NE", "SE", "SW", "NW")[entry["rotation"]]
    state = {"workshop-working": "working", "workshop-blocked": "blocked"}.get(entry["scene"], "ready")
    whole = bound_image(facility["whole_artifacts"][direction][state])
    masks = {}
    # Accepted original component/cut maps own the expected physical order;
    # current C++ sort boxes cannot define their own passing pixel fixture.
    for name in ("components", "foreground"):
        reference = facility["ownership"][direction][name]
        path = contained(ROOT / reference["path"], ROOT)
        if identity(path, ROOT) != reference:
            raise ValueError("frozen occupied ownership map changed")
        masks[name] = path.read_bytes()
    row = next(row for row in rockets["sprites"] if row["direction"] == direction and row["state"] == art.STATES[entry["state"]] and row["frame"] == entry["frame"])
    x, y, width, height = row["box"]
    actor = atlas.crop((x, y, x + width, y + height))
    geometry = facility["geometry"][direction]
    offset = [entry["facility_contact"][i] - geometry["floor_origin"][i] for i in range(2)]
    contact = [entry["rocket_contact"][i] - offset[i] for i in range(2)]
    zone = "recovery" if entry["scene"] == "recovery-turnaround" else "pad"
    scene = Image.new("P", whole.size)
    scene.putpalette(whole.getpalette())
    opaque = Image.frombytes("L", actor.size, bytes(255 if value else 0 for value in actor.tobytes()))
    # Reproduce only the accepted source physical partition. The visitor goes
    # between its own zone's back/front; other zones keep original depth order.
    for owner, name in enumerate(art.facility.DEPTH_ORDERS[direction], 1):
        passes = (False, True) if name == zone and entry["rocket_visible"] else (None,)
        for front in passes:
            if front is True:
                scene.paste(actor, (contact[0] - 32, contact[1] - 32), opaque)
            for index, value in enumerate(whole.get_flattened_data()):
                if masks["components"][index] != owner:
                    continue
                # Only original pixels belonging to this pass may obscure its
                # native visitor; no global foreground overpaint is admitted.
                if front is not None and bool(masks["foreground"][index]) != front:
                    continue
                scene.putpixel((index % whole.width, index // whole.width), value)
    return scene, offset


def qualified_observations(document):
    """Require every original view/scene exactly once before raster acceptance.

    @param document Actual native scene observations and explicit component scope.
    @return Complete ordered native observations.
    @throws ValueError On missing/repeated views, scenes or misleading gameplay scope.
    """
    scenes = ("empty", "pad-ready", "pad-preparing", "pad-ascent0", "pad-ascent1", "recovery-turnaround", "workshop-working", "workshop-blocked", "stale-pad")
    expected = Counter((rotation, scene) for rotation in range(4) for scene in scenes)
    actual = Counter((row["rotation"], row["scene"]) for row in document["observations"])
    # A partial matrix or a repeated successful image cannot stand for another
    # physical view; component poses also cannot claim earned ordinary progress.
    if actual != expected or document["simulation_ticks"] != 0 or document["ordinary_game_reviewed"] or not document["component_pose_inputs"]:
        raise ValueError("incomplete or misleading occupied native matrix")
    # Each logical scene must name its own native frame. A matching scene
    # count cannot substitute another viewport or an out-of-owner file.
    for row in document["observations"]:
        expected_name = "view-" + str(row["rotation"]) + "-" + row["scene"] + ".ppm"
        if row["image"] != expected_name:
            raise ValueError("occupied scene names another native frame")
    return document["observations"]


def compare_pixels(actual, expected):
    """Require exact original palette indices across the complete real viewport.

    @param actual Native indexed raster, including transparent-source terrain regions.
    @param expected Independent frozen source composite on the real empty viewport.
    @return Complete compared screen-pixel count.
    @throws ValueError On any wrong source, ordering, pose, stale frame or extra pixel.
    """
    if actual != expected:
        raise ValueError("native occupied raster differs from frozen source pixels")
    return len(actual)


def verify_rasters(context, facility, rockets, atlas):
    """Compare complete native facility/occupied viewports with frozen source pixels.

    @param context Actual native invocation with its declared image/report outputs.
    @param facility Explicit current selected facility report with original source/ownership maps.
    @param rockets Explicit current selected rocket source report, separate from native observations.
    @param atlas Detached approved indexed rocket source image.
    @return Per-scene observations and explicit detached malformed-evidence refusals.
    @throws ValueError On missing scenes, source drift or any changed viewport pixel.
    """
    art = component.art
    document = art.read_json(context.reports / "native-occupied-matrix.json")
    entries = qualified_observations(document)
    results = []
    for entry in entries:
        scene, offset = expected_scene(entry, facility, rockets, atlas)
        actual_bytes = (context.images / (entry["image"] + ".indexed")).read_bytes()
        baseline = (context.images / ("view-" + str(entry["rotation"]) + "-empty.ppm.indexed")).read_bytes()
        expected = Image.frombytes("P", (640, 360), baseline)
        # Frozen original source pixels override the independently captured empty
        # world. Compare the whole viewport, so leaked transparent pixels, engine
        # remnants and hidden stale actors cannot escape a source-mask-only test.
        opaque = Image.frombytes("L", scene.size, bytes(255 if value else 0 for value in scene.tobytes()))
        expected.paste(scene, tuple(offset), opaque)
        count = compare_pixels(actual_bytes, expected.tobytes())
        results.append(dict(rotation=entry["rotation"], scene=entry["scene"], source_pixels=sum(bool(value) for value in scene.tobytes()), screen_pixels=count, mismatches=0))
    space = [(context.images / ("view-" + str(rotation) + "-space.indexed")).read_bytes() for rotation in range(4)]
    for raster in space[1:]:
        compare_pixels(raster, space[0])
    refusals = 0
    # Detached evidence mutations must fail without rewriting source fixtures,
    # actual native observations or a paid save. Each tests a different gate.
    for mutation in ("missing", "repeated", "false-paid-scope", "wrong-pixel"):
        altered = copy.deepcopy(document)
        try:
            if mutation == "missing":
                altered["observations"].pop()
            elif mutation == "repeated":
                altered["observations"].append(altered["observations"][0])
            elif mutation == "false-paid-scope":
                altered["ordinary_game_reviewed"] = True
            else:
                damaged = bytearray(space[0]); damaged[0] ^= 1
                compare_pixels(bytes(damaged), space[0])
            qualified_observations(altered)
        except ValueError:
            refusals += 1
        else:
            raise ValueError("malformed occupied render evidence was accepted")
    print("qualified", len(results), "complete native occupied viewports; detached refusals", refusals, flush=True)
    return dict(scenes=results, wrong_layer_comparisons=3, detached_refusals=refusals)


def contact_sheet(context):
    """Keep reproducible actual viewport views together for physical art review.

    @param context Exact native invocation supplying the already qualified PNG captures.
    @return Newly written review image, containing only native pixels and labels.
    """
    sheet = Image.new("RGB", (1408, 744), "#29323c")
    drawing = ImageDraw.Draw(sheet)
    # Compare the same genuine contact across views and phases. Cropping only
    # selects the review camera area; complete viewport bytes qualified above.
    for row, scene in enumerate(("pad-ready", "pad-ascent0", "recovery-turnaround")):
        for rotation, heading in enumerate(("NE", "SE", "SW", "NW")):
            with Image.open(context.images / ("view-" + str(rotation) + "-" + scene + ".png")) as image:
                sheet.paste(image.crop((144, 100, 496, 324)), (rotation * 352, row * 248 + 24))
            drawing.text((rotation * 352 + 5, row * 248 + 5), heading + " " + scene + " / native viewport", fill="white")
    output = context.images / "native-occupied-contact.png"
    sheet.save(output)
    return output


def run(context, configuration, application, host, facility, rockets, atlas, terminal):
    """Capture the original occupied matrix using explicit current source/world/build inputs.

    @param context Fresh capture invocation and configuration-specific native process home.
    @param configuration Actual independently built Debug or RelWithDebInfo application.
    @param application Exact current isolated Space-art executable closed by the stage owner.
    @param host Hash-verified retained original fixture world, never the current GZVEEH save.
    @param facility Selected current facility source/ownership report with approved original pixels.
    @param rockets Selected current rocket source report with independent pose expectations.
    @param atlas Detached original approved indexed rocket source image.
    @param terminal Fresh required current native transcript in this invocation's evidence owner.
    @return Actual native process, complete raster comparisons, frames and derived review sheet.
    """
    observation = context.reports / "native-occupied-matrix.json"
    # Fixed C++ frame/report names belong to one fresh capture run. Refuse
    # existing observations before any runtime or source world can be touched.
    if observation.exists() or terminal.exists() or any(context.images.iterdir()):
        raise ValueError("occupied native capture already exists; select a fresh capture run")
    before = identity(ROOT / "media/baseset/openttd.grf", ROOT)
    config = context.prepare_runtime()
    runtime = context.runtime_dir / "baseset/openttd.grf"
    if identity(runtime, ROOT)["sha256"] != before["sha256"]:
        raise ValueError("occupied viewport runtime GRF differs from canonical shipping bytes")
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# @see r2fc-file-cleanup3.md19B; original zero-tick occupied component observer.\nlaunch_facility_render\n", encoding="utf-8")
    environment = context.environment()
    environment.update(OTTD_LAUNCH_RENDER_SOURCE=str(host), OTTD_TEST_CONFIGURATION=configuration)
    with terminal.open("x", encoding="utf-8") as output:
        result = run_native(context, [str(application), "[.launch-facility-render]", "--reporter", "console", "--durations", "yes"],
                            native_producer=Path(__file__).resolve().relative_to(ROOT).as_posix(),
                            cwd=ROOT / "build", env=environment, stdout=output,
                            stderr=subprocess.STDOUT, timeout=60, check=False)
    text = terminal.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \(([\d,]+) assertions in 1 test case\)", text)
    # Actual full native drawing, source restoration and zero-tick checks
    # must complete; an empty filter or partial process cannot qualify pixels.
    if result.returncode != 0 or passed is None:
        raise ValueError("actual occupied native viewport failed: " + str(terminal))
    verification = verify_rasters(context, facility, rockets, atlas)
    process = component.art.read_json(ROOT / result.native_report["path"])
    if identity(observation, ROOT) not in process["artifacts"]:
        raise ValueError("native occupied observations lack actual child provenance")
    frames = []
    # Select only containers actually written by this exact child. The
    # shared conversion owner binds unchanged PNG pixels to their real process;
    # the later contact sheet is a separate derived review product.
    for reference in process["artifacts"]:
        path = contained(ROOT / reference["path"], ROOT)
        if path.suffix != ".ppm":
            continue
        contained(path, context.images)
        target = path.with_suffix(".png")
        with Image.open(path) as raster:
            save_native_capture(context, raster, path, target)
        frames.append(dict(identity(target, ROOT), feature=context.feature, scenario=context.scenario,
                           run_id=context.run_id, name=path.stem))
    sheet = contact_sheet(context)
    # Rendering cannot replace canonical content or rewrite its runtime copy;
    # the immutable input host is closed by the caller and shared runner too.
    if identity(ROOT / "media/baseset/openttd.grf", ROOT) != before or identity(runtime, ROOT)["sha256"] != before["sha256"]:
        raise ValueError("occupied viewport changed canonical/runtime content")
    return dict(native_process=result.native_report, native_observations=identity(observation, ROOT),
                native_terminal=identity(terminal, ROOT), assertions=int(passed.group(1).replace(",", "")),
                verification=verification, frames=frames, derived_review_sheet=identity(sheet, ROOT),
                runtime_config=identity(config, ROOT), runtime_payload=identity(runtime, ROOT))


def main():
    """Dispatch one current occupied viewport using explicit source/selector/build inputs.

    @return None; legacy executable-copy finalization is not an active producer.
    """
    from launch_viewport_review import main as selected_main
    selected_main()


if __name__ == "__main__":
    main()
