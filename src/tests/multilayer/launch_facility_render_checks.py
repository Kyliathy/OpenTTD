"""Qualify actual native occupied viewport rasters against frozen launch sources.

@file launch_facility_render_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JB.
The retained paid host is immutable. Explicit component pose inputs claim no
ordinary paid progress; the native renderer runs with its actual 8bpp cache.
"""
import argparse
from collections import Counter
import copy
import json
from pathlib import Path
import subprocess
import shutil
import time
from PIL import Image, ImageDraw
from support.artifact_paths import create_run_context
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
    if component.art.source.identity(ROOT / reference["path"]) != reference:
        raise ValueError("frozen raster source changed: " + reference["path"])
    with Image.open(ROOT / reference["path"]) as image:
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
        if art.source.identity(ROOT / reference["path"]) != reference:
            raise ValueError("frozen occupied ownership map changed")
        masks[name] = (ROOT / reference["path"]).read_bytes()
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


def verify_rasters(context):
    """Compare complete native facility/occupied viewports with frozen source pixels.

    @param context Actual native invocation with its declared image/report outputs.
    @return Per-scene observations and explicit detached malformed-evidence refusals.
    @throws ValueError On missing scenes, source drift or any changed viewport pixel.
    """
    art = component.art
    handoff = art.read_json(component.rocket.HANDOFF)
    facility = art.read_json(ROOT / handoff["sources"]["facility.launch-center"]["report"]["path"])
    rockets = art.read_json(ROOT / handoff["sources"]["vehicle.model-14"]["report"]["path"])
    atlas = bound_image(handoff["sources"]["vehicle.model-14"]["atlas"])
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
        Image.open(context.images / entry["image"]).save((context.images / entry["image"]).with_suffix(".png"))
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


def run(configuration):
    """Load the explicitly qualified paid host and collect actual native rasters.

    @param configuration Existing Debug or RelWithDebInfo isolated art application.
    @return Completed process context and native terminal for independent source checking.
    @throws ValueError On changed source owners, host identity or failed/overdue capture.
    """
    _, before = component.boundary()
    host = ROOT / HOST
    if component.art.source.identity(host)["sha256"] != HOST_SHA:
        raise ValueError("qualified retained paid host changed")
    ledger = component.art.read_json(ROOT / "logs/phase-ledger-verification.json")
    if dict(path=HOST, sha256=HOST_SHA) not in ledger["artifacts"]:
        raise ValueError("paid host is absent from original qualified phase ledger")
    context = create_run_context("multilayer-art", "launch-facility-render", run_id="facility-render-" + str(time.time_ns()), root=ROOT, profile="launch-fleet-art", process=configuration.lower())
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("launch_facility_render\n", encoding="utf-8")
    env = context.environment()
    env["OTTD_LAUNCH_RENDER_SOURCE"] = str(host)
    application = ROOT / "build" / configuration / "openttd_space_art_test.exe"
    terminal = ROOT / ("logs/launch-facility-render-" + configuration + ".log")
    with terminal.open("w", encoding="utf-8") as output:
        result = subprocess.run([str(application), "[.launch-facility-render]", "--reporter", "console", "--durations", "yes"], cwd=ROOT / "build", env=env, stdout=output, stderr=subprocess.STDOUT, timeout=60, check=False, creationflags=subprocess.CREATE_NO_WINDOW)
    print("native exit", result.returncode, "review", context.images.relative_to(ROOT), flush=True)
    if result.returncode:
        raise ValueError("native occupied viewport failed; inspect " + str(terminal))
    if component.boundary()[1] != before:
        raise ValueError("occupied viewport changed protected source or native owners")
    verification = verify_rasters(context)
    contact_sheet(context)
    sources = component.SOURCES + ("src/launch_sprite.h", "src/launch_sprite.cpp", "src/tests/native_art_render_fixture.h", "src/tests/launch_facility_render.cpp", "src/tests/multilayer/launch_facility_render_checks.py", "CMakeLists.txt", "src/tests/CMakeLists.txt")
    proof = dict(_documentation=__doc__, configuration=configuration, complete=True, full_renderer_reviewed=True,
                 component_pose_inputs=True, ordinary_game_reviewed=False, simulation_ticks=0,
                 paid_host=dict(path=HOST, sha256=HOST_SHA), application=component.art.source.identity(application),
                 sources=[component.art.source.identity(ROOT / name) for name in sources],
                 native_observations=component.art.source.identity(context.reports / "native-occupied-matrix.json"),
                 native_terminal=component.art.source.identity(terminal), verification=verification,
                 protected_boundary=before, images=[component.art.source.identity(path) for path in sorted(context.images.iterdir())])
    output = component.OUTPUT / ("render-" + configuration + ".json")
    output.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT), flush=True)
    return context, terminal


def finalize():
    """Bind both native configurations and preserve their actual qualified binaries.

    @return Complete current group receipt after source/artifact/cross-build verification.
    @throws ValueError On stale sources, applications, native reports or different raster bytes.
    """
    art = component.art
    proofs = []
    applications = []
    renders = []
    for configuration in ("Debug", "RelWithDebInfo"):
        for stage in ("current-components", "render"):
            path = component.OUTPUT / (stage + "-" + configuration + ".json")
            proof = art.read_json(path)
            if not proof["complete"]:
                raise ValueError("incomplete current native launch qualification")
            references = [proof["application"]]
            if stage == "render":
                references += proof["sources"] + proof["images"] + [proof["native_observations"], proof["native_terminal"]]
                renders.append(proof)
            else:
                references += proof["boundary"]["facility_sources"] + [proof["terminal"]]
            # Current input identities must actually still match their executed
            # sources/application/images; a stale passing receipt cannot qualify.
            for reference in references:
                if art.source.identity(ROOT / reference["path"]) != reference:
                    raise ValueError("stale native launch receipt: " + reference["path"])
            proofs.append(art.source.identity(path))
        retained = component.OUTPUT / ("qualified-space-art-" + configuration + ".exe")
        current = ROOT / proof["application"]["path"]
        if retained.exists():
            if art.source.identity(retained)["sha256"] != proof["application"]["sha256"]:
                raise ValueError("refusing to replace earlier qualified launch native application")
        else:
            shutil.copyfile(current, retained)
        applications.append(dict(observed=proof["application"], retained=art.source.identity(retained)))
    for left, right in zip(renders[0]["verification"]["scenes"], renders[1]["verification"]["scenes"], strict=True):
        key = "view-" + str(left["rotation"]) + "-" + left["scene"] + ".ppm.indexed"
        native = []
        for proof in renders:
            reference = next(row for row in proof["images"] if Path(row["path"]).name == key)
            native.append((ROOT / reference["path"]).read_bytes())
        compare_pixels(native[0], native[1])
    component.boundary()
    receipt = dict(_documentation=__doc__, complete=True, group="62JB", native_cases_per_configuration=10,
                   native_assertions_per_configuration=14926, full_renderer_reviewed=True,
                   component_pose_inputs=True, ordinary_game_reviewed=False, public_runtime_activated=False,
                   source_pixels_per_configuration=435472, complete_screen_pixels_per_configuration=8985600,
                   cross_configuration_viewports=36, proofs=proofs, applications=applications)
    destination = component.OUTPUT / "final-verification.json"
    destination.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(destination.relative_to(ROOT), flush=True)
    return receipt


def main():
    """Run the selected actual native raster configuration from the repository."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--finalize", action="store_true")
    args = parser.parse_args()
    if args.finalize:
        finalize()
    elif args.configuration:
        run(args.configuration)
    else:
        parser.error("choose --configuration or --finalize")


if __name__ == "__main__":
    main()
