"""Bind the existing occupied launch viewport producer to explicit current inputs.

@file launch_viewport_review.py Real native drawing and independent source pixels; no publication.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19B.
"""

import argparse
import json
from pathlib import Path
import sys
import uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "media/multilayer"))

from artifact_paths import RunContext, contained, create_run_context, identifier
from artifact_rebuild import (identity, load_recipe, snapshot, verify_current_build,
                              verify_passed_stage, verify_retained_inputs)
from artifact_recipes import load_catalogue
from evidence import EvidenceSet, bind_capture, selection_document
from regenerate_space_sources import bind_sources, read_scope, selected_path
import launch_facility_render_checks as renderer
from space_source_review import checked_result


def selected_recipe():
    """Read the sole registered actual occupied viewport declaration.

    @return Enabled original isolated target/tag, never a fabricated viewport writer.
    """
    recipe = next(row for row in load_catalogue(ROOT)["recipes"] if row["id"] == "launch-occupied-viewports")
    # Actual native drawing is a separate bounded obligation from selectors;
    # another target, pending leaf or component filter cannot stand in for it.
    if (not recipe["enabled"] or recipe["target"] != "openttd_space_art_test"
            or recipe["test_tags"] != ["[.launch-facility-render]"]):
        raise ValueError("occupied launch viewport requires its implemented original native case")
    return recipe


def source_views(inputs, source_run):
    """Bind the original independent source/ownership matrices through their shared owner.

    @param inputs Explicit facility and rocket selected source report identities.
    @param source_run Source invocation supplying both pure reports, separate from capture output.
    @return Current approved facility/rocket documents and detached original rocket atlas.
    """
    scope = read_scope("launch")
    context = RunContext(ROOT, "space", "launch-source-inputs", source_run)
    manifests = {"facility.launch-center": inputs["facility-source"], "vehicle.model-14": inputs["rocket-source"]}
    public = renderer.component.art.read_json(renderer.component.art.PUBLIC)
    _public, _references, documents = bind_sources(public, scope, context, manifests)
    index = {row["id"]: row for row in load_catalogue(ROOT)["recipes"]}
    # Each actual current report keeps the original source producer and exact
    # authored pixels. Historical approval is separate from this new drawing.
    for asset, reference in manifests.items():
        document = documents[reference["path"]]
        name = "launch-source-" + asset.replace(".", "-")
        if document["source_producer"]["path"] != index[name]["producer"]:
            raise ValueError("occupied viewport source producer belongs to another asset")
        if identity(ROOT / document["source_producer"]["path"], ROOT) != document["source_producer"]:
            raise ValueError("occupied viewport source producer changed")
    facility = documents[manifests["facility.launch-center"]["path"]]
    rockets = documents[manifests["vehicle.model-14"]["path"]]
    # Complete independent whole-art and ownership maps are real source inputs,
    # not pixels inferred from the current occupied native viewport.
    for direction in facility["directions"]:
        for reference in facility["whole_artifacts"][direction].values():
            selected_path(reference, context, "images")
        for reference in facility["ownership"][direction].values():
            selected_path(reference, context, "data")
    return facility, rockets, renderer.bound_image(rockets["native_source"])


def checked_request(request, root):
    """Validate current selector/build and original retained/source inputs before native drawing.

    @param request Exact registered current prepare/verify capture stage.
    @param root Explicit source checkout owning this producer.
    @return Actual declaration, current selector, retained host, application and source views.
    """
    # Cleanup may observe this original historical fixture, never acquire paid
    # work/publication scope or load the active GZVEEH continuation world.
    if (root.resolve() != ROOT or request["stage"] not in ("prepare", "verify")
            or request["configuration"] not in ("Debug", "RelWithDebInfo")
            or request["paid_work_authorized"] is not False or request["publishes_source"] is not False):
        raise ValueError("occupied launch viewport requires its read-only current configuration scope")
    recipe = selected_recipe()
    if request["declaration"] != recipe:
        raise ValueError("occupied viewport declaration changed")
    selector = checked_result(request["inputs"]["source-selector"], "launch", request["configuration"], root)
    verify_current_build(request["build"], request["configuration"], recipe, root)
    if selector["build_result"] != request["build"]["result"]:
        raise ValueError("occupied viewport and source selectors used different actual builds")
    host_reference = request["inputs"]["host"]
    host = contained(root / host_reference["path"], root)
    # Original historical fixture identity is fixed independently of the new
    # producer. Another convenient paid world cannot substitute for this host.
    if identity(host, root) != host_reference or host_reference["sha256"] != renderer.HOST_SHA:
        raise ValueError("occupied viewport retained original fixture changed")
    allowed = (root / "savegames/retained", root / "src/tests/fixtures")
    if not any(host.is_relative_to(owner.resolve()) for owner in allowed):
        raise ValueError("occupied viewport host must use its retained immutable owner")
    facility, rockets, atlas = source_views(request["inputs"], selector["run_id"])
    application = contained(root / selector["application"]["path"], root)
    return recipe, selector, host, application, facility, rockets, atlas


def execute_stage(request, root):
    """Execute and seal actual complete occupied viewports through the original producer.

    @param request Registered current capture stage and explicit input/output identities.
    @param root Checkout owning retained host, independent sources and actual build.
    @return Actual source/native pixel, process, conversion and scope checks.
    """
    recipe, selector, host, application, facility, rockets, atlas = checked_request(request, root)
    if request["stage"] == "prepare":
        return dict(checks=["current actual same-build source selectors/decoder verified",
                            "original retained host and independent approved source/ownership maps verified"])
    context = RunContext(root, recipe["feature"], recipe["scenario"], request["run_id"],
                         "launch-fleet-art", request["configuration"].lower())
    # C++ owns fixed native scene names. One explicit output run/configuration
    # keeps captures isolated from source inputs and previous native observers.
    for directory in (context.images, context.reports, context.evidence):
        directory.mkdir(parents=True, exist_ok=True)
    output = contained(root / request["outputs"]["manifest"], context.reports)
    if output.exists():
        raise ValueError("occupied viewport result already exists; select a fresh capture run")
    terminal = contained(root / request["outputs"]["native-evidence"], context.evidence)
    actual = renderer.run(context, request["configuration"], application, host, facility, rockets, atlas, terminal)
    checked_request(request, root)
    semantic = dict(_documentation=__doc__, schema_version=1, kind="native_capture_semantics",
        producer=recipe["producer"], feature=context.feature, scenario=context.scenario, run_id=context.run_id,
        configuration=request["configuration"], source=request["inputs"]["host"]["path"],
        source_sha256=request["inputs"]["host"]["sha256"], frames=actual["frames"],
        component_pose_inputs=True, simulation_ticks=0, ordinary_game_reviewed=False)
    semantic_path = context.reports / "native-capture.json"
    semantic_path.write_text(json.dumps(semantic, indent=2) + "\n", encoding="utf-8")
    capture = bind_capture(context, semantic_path, request["configuration"])
    capture_path = contained(root / request["outputs"]["capture-index"], context.reports)
    capture_path.write_text(json.dumps(capture, indent=2) + "\n", encoding="utf-8")
    selection = selection_document(context, request["configuration"])
    selection.update(outputs={}, reports={"occupied": identity(capture_path, root)},
                     evidence={"occupied": capture["terminal"]})
    selection_path = contained(root / request["outputs"]["selection"], context.reports)
    selection_path.write_text(json.dumps(selection, indent=2) + "\n", encoding="utf-8")
    EvidenceSet(selection_path, context=context, root=root).report("occupied")
    # Only fully validated actual process/frames/conversions and independent
    # whole-viewport comparisons can publish current technical qualification.
    document = dict(_documentation=__doc__, schema_version=1, kind="native_launch_viewport_result",
        complete=True, configuration=request["configuration"], run_id=context.run_id,
        actual_native_viewports_verified=True, component_pose_inputs=True, simulation_ticks=0,
        ordinary_game_reviewed=False, public_runtime_activated=False, publishes_source=False,
        retained_host=request["inputs"]["host"], source_selector=request["inputs"]["source-selector"],
        source_inputs={name: request["inputs"][name] for name in ("facility-source", "rocket-source")},
        build_result=request["build"]["result"], application=selector["application"],
        producer_sources=request["sources"], selected_capture=identity(capture_path, root),
        selection=identity(selection_path, root), source_generation=request.get("prepared_source"), **actual)
    with output.open("x", encoding="utf-8") as stream:
        json.dump(document, stream, indent=2)
        stream.write("\n")
    return dict(manifest=identity(output, root), checks=["actual original occupied viewport case passed",
        "all36complete indexed scene views and3wrong-layer comparisons matched independent original sources",
        "all4original detached malformed-evidence refusals passed",
        "actual native process/frame/container-conversion/source/build/configuration provenance closed",
        "derived contact sheet kept separate; zero simulation ticks and no ordinary/human/public acceptance"])


def execute_prepared(configuration, selector_reference, build_reference, source_reference, run_id):
    """Run a fresh capture using explicit current selectors and completed pure source leaves.

    @param configuration Actual independent current native configuration.
    @param selector_reference Exact actual current launch selector result identity.
    @param build_reference Same actual current build identity used by its decoder/selectors.
    @param source_reference Explicit completed supplying recipe root; only its pure source leaves are reused.
    @param run_id New explicit output invocation, separate for each configuration's fixed C++ frame names.
    @return Actual completed dispatcher verification result.
    """
    from artifact_recipe_stage import execute_request
    identifier(run_id)
    recipe = selected_recipe()
    selector = checked_result(selector_reference, "launch", configuration, ROOT)
    path = contained(ROOT / source_reference["path"], ROOT / "mlm/reviews/rebuild")
    if identity(path, ROOT) != source_reference:
        raise ValueError("occupied viewport pure source root changed")
    state = json.loads(path.read_text(encoding="utf-8"))
    # Reuse only completed independently checked source bytes. The source
    # root's historical native result/configuration cannot qualify this capture.
    if (state.get("kind") != "artifact_rebuild_result" or state.get("complete") is not True
            or state.get("status") != "verified" or state.get("run_id") != selector["run_id"]):
        raise ValueError("occupied viewport requires its explicit completed pure source root")
    inputs = dict(verify_retained_inputs(recipe, ROOT), **{"source-selector": selector_reference})
    # Both source families must have their own passed pure verification witness
    # before the fresh output run is reserved or any saved world is loaded.
    for name, role in (("launch-source-facility-launch-center", "facility-source"),
                       ("launch-source-vehicle-model-14", "rocket-source")):
        declaration = state["signature"]["declarations"][name]
        node = state["nodes"][name]
        if (node["status"] != "verified" or declaration["execution"] != "pure_conversion"
                or declaration["configuration_reuse"] != "source_bytes_only"
                or declaration["comparison"] != "independent_source_bytes"):
            raise ValueError("occupied viewport supplier is not a verified pure source leaf")
        verify_passed_stage(node["stages"]["verify"], ROOT)
        inputs[role] = node["outputs"]["manifest"]
    selection = load_recipe(recipe["id"], configuration, ROOT)
    if not selection["executable"]:
        raise ValueError("occupied viewport recipe has pending/missing prerequisites")
    sources = snapshot(selection, ROOT)["sources"]
    base = dict(stage="prepare", declaration=recipe, configuration=configuration, inputs=inputs,
                build=dict(status="passed", result=build_reference), paid_work_authorized=False, publishes_source=False)
    checked_request(base, ROOT)
    context = create_run_context(recipe["feature"], recipe["scenario"], run_id=run_id, root=ROOT,
                                 profile="launch-fleet-art", process=configuration.lower())
    last = None
    # Actual shared dispatcher stages retain their exact requests/results;
    # no historical producer success or current-source flag substitutes for drawing.
    for stage in ("prepare", "verify"):
        token = "prepared-" + stage + "-" + uuid.uuid4().hex
        request = dict(base, _documentation=__doc__, schema_version=1, kind="artifact_recipe_stage_request",
            recipe=recipe["id"], stage=stage, run_id=run_id, feature=recipe["feature"], scenario=recipe["scenario"],
            sources=sources, outputs={row["role"]: (context.directory / row["path"].format(configuration=configuration)).relative_to(ROOT).as_posix()
                                     for row in recipe["outputs"]},
            result_path=(context.reports / (token + "-result.json")).relative_to(ROOT).as_posix(),
            prepared_source=source_reference)
        request_path = context.reports / (token + ".json")
        request_path.write_text(json.dumps(request, indent=2) + "\n", encoding="utf-8")
        last = execute_request(request_path, ROOT)
    return last


def compare_configurations(references):
    """Compare both actual configuration matrices without retaining executable copies.

    @param references Two explicit completed current viewport result identities.
    @return Measured comparison of all36complete indexed scene views.
    """
    results = {}
    # Both configurations must independently pass their actual original
    # renderer/source/process checks; one image cannot qualify its counterpart.
    for reference in references:
        path = contained(ROOT / reference["path"], ROOT / "mlm/reviews")
        if identity(path, ROOT) != reference:
            raise ValueError("occupied viewport comparison input changed")
        document = json.loads(path.read_text(encoding="utf-8"))
        configuration = document.get("configuration")
        if (document.get("kind") != "native_launch_viewport_result" or document.get("complete") is not True
                or document.get("actual_native_viewports_verified") is not True
                or document.get("simulation_ticks") != 0 or document.get("component_pose_inputs") is not True
                or document.get("ordinary_game_reviewed") is not False or document.get("publishes_source") is not False
                or configuration not in ("Debug", "RelWithDebInfo") or configuration in results):
            raise ValueError("occupied comparison requires two independent completed current configurations")
        verify_current_build(dict(result=document["build_result"]), configuration, selected_recipe(), ROOT)
        checked_result(document["source_selector"], "launch", configuration, ROOT)
        # Every original producer, physical source and measured observation
        # stays current before comparing bytes from the independently run cases.
        for witness in [*document["producer_sources"], *document["source_inputs"].values(),
                        document["retained_host"], document["native_process"], document["native_observations"]]:
            if identity(ROOT / witness["path"], ROOT) != witness:
                raise ValueError("occupied comparison producer/source/native witness changed")
        selection = EvidenceSet(ROOT / document["selection"]["path"], root=ROOT)
        if identity(selection.path, ROOT) != document["selection"]:
            raise ValueError("occupied comparison selection changed")
        if selection.document["reports"]["occupied"] != document["selected_capture"]:
            raise ValueError("occupied comparison selected another capture")
        selection.report("occupied")
        process = json.loads((ROOT / document["native_process"]["path"]).read_text(encoding="utf-8"))
        observation = json.loads((ROOT / document["native_observations"]["path"]).read_text(encoding="utf-8"))
        entries = renderer.qualified_observations(observation)
        indexed = {}
        # Indexed buffers are actual child artifacts too. Do not accept a
        # detached replacement merely because it has an expected scene name.
        for entry in entries:
            raster = contained(selection.context.images / (entry["image"] + ".indexed"), selection.context.images)
            if identity(raster, ROOT) not in process["artifacts"]:
                raise ValueError("occupied indexed comparison lacks actual native output provenance")
            indexed[(entry["rotation"], entry["scene"])] = raster.read_bytes()
        results[configuration] = dict(document=document, indexed=indexed)
    if set(results) != {"Debug", "RelWithDebInfo"}:
        raise ValueError("occupied comparison requires Debug and RelWithDebInfo")
    left, right = results["Debug"], results["RelWithDebInfo"]
    if left["document"]["retained_host"] != right["document"]["retained_host"]:
        raise ValueError("occupied configurations used different original fixture worlds")
    # Compare complete current viewports, preserving the original cross-build
    # obligation without archiving executables or rebasing historical receipts.
    for key, pixels in left["indexed"].items():
        renderer.compare_pixels(pixels, right["indexed"][key])
    return dict(configurations=["Debug", "RelWithDebInfo"], matched_native_views=len(left["indexed"]),
                component_pose_inputs=True, simulation_ticks=0, ordinary_game_reviewed=False)


def main():
    """Execute one real occupied capture with explicit current input/build/output identities.

    @return None; failures retain actual current diagnostics and do not publish acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--selector-result", type=Path)
    parser.add_argument("--build-result", type=Path)
    parser.add_argument("--source-result", type=Path)
    parser.add_argument("--run-id")
    parser.add_argument("--compare", nargs=2, type=Path, help="Two explicit completed current configuration results")
    args = parser.parse_args()
    if args.compare:
        print(json.dumps(compare_configurations([identity(path.resolve(), ROOT) for path in args.compare]), indent=2))
        return
    # Explicit current input/build/output identities are required together;
    # the former implicit finalization and fixed cache lookups stay unavailable.
    if not all((args.configuration, args.selector_result, args.build_result, args.source_result, args.run_id)):
        parser.error("capture requires --configuration, --selector-result, --build-result, --source-result and --run-id")
    result = execute_prepared(args.configuration, identity(args.selector_result.resolve(), ROOT),
        identity(args.build_result.resolve(), ROOT), identity(args.source_result.resolve(), ROOT), args.run_id)
    print(json.dumps(dict(recipe=result["recipe"], complete=result["complete"], outputs=result["outputs"]), indent=2))


if __name__ == "__main__":
    main()
