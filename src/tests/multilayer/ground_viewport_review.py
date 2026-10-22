"""Select and seal original bounded Ground native viewport recipes with explicit current inputs.

@file ground_viewport_review.py Current native/source/provenance qualification, no gameplay or human grant.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB.
"""
import argparse
import json
from pathlib import Path
import runpy
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
from ground_review_sources import GroundSourceViews, WORLD_CONTRACT, retained_world
from ground_viewport_capture import MODES, verify_original_rasters
from space_source_review import checked_result as checked_selectors

SOURCE_ROLES = ("ground-packing", "m1-source-vehicle", "m1-source-purchase", "m1-source-collector")


def selected_recipe(mode):
    """Select one sole-catalogue original native capture mechanism.

    @param mode Explicit original collector, industry-state, purchase, body or station burst.
    @return Enabled bounded declaration with its actual original target/tag and immutable scope.
    """
    if mode not in MODES:
        raise ValueError("unknown original Ground viewport mode")
    recipe = next(row for row in load_catalogue(ROOT)["recipes"] if row["id"] == "ground-viewport-" + mode)
    # The actual native renderer is separate from decoders/source selectors.
    # No empty case, another target or live operation can qualify its viewports.
    if (not recipe["enabled"] or recipe["target"] != "openttd_ground_viewport_test"
            or recipe["test_tags"] != [MODES[mode]["tag"]]
            or recipe["original_parameters"]["mode"] != mode):
        raise ValueError("Ground viewport requires its implemented original native mechanism")
    return recipe


def checked_request(request, root):
    """Validate current same-build selectors and retained original world before native capture.

    @param request Exact registered read-only prepare/verify stage with current explicit identities.
    @param root Checkout owning current sources, original retained fixtures and actual native images.
    @return Original declaration/mode, checked selectors, independent world, source and application.
    """
    mode = request["declaration"]["original_parameters"]["mode"]
    recipe = selected_recipe(mode)
    # These isolated historical art fixtures do not authorize paid progress,
    # publication, active Space continuation or the paused five-model Tube work.
    if (root.resolve() != ROOT or request["declaration"] != recipe
            or request["stage"] not in ("prepare", "verify")
            or request["paid_work_authorized"] is not False or request["publishes_source"] is not False):
        raise ValueError("Ground viewport requires its explicit read-only current scope")
    configuration = request["configuration"]
    selector = checked_selectors(request["inputs"]["source-selector"], "ground", configuration, root)
    verify_current_build(request["build"], configuration, recipe, root)
    if selector["build_result"] != request["build"]["result"]:
        raise ValueError("Ground viewport and source consumers require the same actual current build")
    build = json.loads((root / request["build"]["result"]["path"]).read_text(encoding="utf-8"))
    application = root / build["targets"]["openttd_ground_viewport_test"]["path"]
    if identity(root / WORLD_CONTRACT, root) != request["inputs"]["worlds-contract"]:
        raise ValueError("Ground viewport independent original expectation contract changed")
    world, source = retained_world(mode, root)
    if identity(source, root) != request["inputs"]["world"]:
        raise ValueError("Ground viewport selected another original retained world")
    return recipe, mode, selector, world, source, application


def checked_result(reference, mode, configuration, root=ROOT):
    """Close actual current native/source/complete-raster/evidence witnesses for a later selected closure.

    @param reference Explicit completed current viewport result identity, never a latest-run lookup.
    @param mode Exact original bounded mechanism required by the consumer.
    @param configuration Matching independently executed current native configuration.
    @param root Checkout owning every selected current input, source and actual process artifact.
    @return Current result after complete physical/native/configuration/source provenance validation.
    """
    path = contained(root / reference["path"], root / "mlm/reviews")
    if identity(path, root) != reference:
        raise ValueError("current Ground viewport result changed")
    document = json.loads(path.read_text(encoding="utf-8"))
    # Art fixtures remain technical drawing evidence. Even an actual complete
    # view cannot promote deferred gameplay, paid progress or human approval.
    if (document.get("kind") != "native_ground_viewport_result" or document.get("complete") is not True
            or document.get("mode") != mode or document.get("configuration") != configuration
            or document.get("actual_native_viewports_verified") is not True
            or document.get("paid_progress_claimed") is not False or document.get("human_acceptance") is not False):
        raise ValueError("selected closure requires its exact current Ground viewport scope")
    recipe = selected_recipe(mode)
    request = dict(stage="verify", declaration=recipe, configuration=configuration, inputs=document["source_inputs"],
                   build=dict(result=document["build_result"]), paid_work_authorized=False, publishes_source=False)
    _recipe, _mode, selector, world, _source, application = checked_request(request, root)
    references = [*document["producer_sources"], *document["source_inputs"].values(), document["native_process"],
        document["actual_execution"], document["source_pixels"], document["selected_capture"], document["selection"],
        document["runtime_config"], document["runtime_payload"], document["startup"], document["application"], document["native_terminal"]]
    for witness in references:
        if identity(root / witness["path"], root) != witness:
            raise ValueError("current Ground viewport source/process/pixel witness changed")
    process = json.loads((root / document["native_process"]["path"]).read_text(encoding="utf-8"))
    # Actual scene metadata and retained stdout are required observations too;
    # unchanged rasters cannot hide a removed/replaced native owner report.
    for witness in [*process["artifacts"], *process["inputs"], *process["transcripts"]]:
        if identity(root / witness["path"], root) != witness:
            raise ValueError("current Ground native artifact/input/transcript changed")
    # Exact source and executing image distinguish this actual native capture
    # from an upstream decoder, a source-only test or another fixture mode.
    if (process["exit_status"] != 0 or process["executable"] != identity(application, root)
            or process["inputs"] != [document["source"]]
            or process["argv"][1:] != [MODES[mode]["tag"], "--reporter", "console", "--durations", "yes"]):
        raise ValueError("current Ground viewport actual native process disagrees")
    context = RunContext(root, recipe["feature"], recipe["scenario"], identifier(document["run_id"]),
                         "ground-art", mode + "-" + configuration.lower())
    verify_original_rasters(context, process, world)
    with GroundSourceViews(root, selector["run_id"], {role: request["inputs"][role] for role in SOURCE_ROLES}) as views:
        views.verify_current()
    selection = EvidenceSet(root / document["selection"]["path"], context=context, root=root)
    if selection.document["reports"]["ground"] != document["selected_capture"]:
        raise ValueError("current Ground viewport selected another capture report")
    capture = selection.report("ground")
    if capture["terminal"] != document["native_terminal"]:
        raise ValueError("current Ground viewport terminal differs from its actual native capture")
    return document


def execute_stage(request, root):
    """Execute and seal the original actual bounded native drawing/source pixel mechanism.

    @param request Registered current prepare/verify request and exact selected input/output identities.
    @param root Checkout owning current native targets, original source media and retained review worlds.
    @return Measured native/complete-raster/source/composition/provenance checks with no progress grant.
    """
    recipe, mode, selector, world, source, application = checked_request(request, root)
    inputs = {role: request["inputs"][role] for role in SOURCE_ROLES}
    if request["stage"] == "prepare":
        with GroundSourceViews(root, selector["run_id"], inputs) as views:
            views.verify_current()
        return dict(checks=["current same-build Ground decoder/consumer prerequisites verified",
                            "all50original source views and exact retained original world/raster expectations verified"])
    context = RunContext(root, recipe["feature"], recipe["scenario"], request["run_id"],
                         "ground-art", mode + "-" + request["configuration"].lower())
    for directory in (context.images, context.reports, context.evidence):
        directory.mkdir(parents=True, exist_ok=True)
    output = contained(root / request["outputs"]["manifest"], context.reports)
    if output.exists():
        raise ValueError("Ground viewport output exists; select a fresh explicit capture run")
    wrapper = runpy.run_path(str(root / recipe["producer"]))
    with GroundSourceViews(root, selector["run_id"], inputs) as views:
        actual = wrapper["capture"](context, mode, request["configuration"], application, source, world, views)
    checked_request(request, root)
    semantic = dict(_documentation=__doc__, schema_version=1, kind="native_capture_semantics",
        producer=recipe["producer"], feature=context.feature, scenario=context.scenario, run_id=context.run_id,
        configuration=request["configuration"], source=actual["source"]["path"], source_sha256=actual["source"]["sha256"],
        frames=actual["frames"], draw_owners_unchanged=True, ordinary_game_reviewed=False, paid_progress_claimed=False)
    semantic_path = context.reports / "native-capture.json"
    semantic_path.write_text(json.dumps(semantic, indent=2) + "\n", encoding="utf-8")
    capture = bind_capture(context, semantic_path, request["configuration"])
    capture_path = root / request["outputs"]["capture-index"]
    capture_path.write_text(json.dumps(capture, indent=2) + "\n", encoding="utf-8")
    selection = selection_document(context, request["configuration"])
    selection.update(outputs={}, reports={"ground": identity(capture_path, root)}, evidence={"ground": capture["terminal"]})
    selection_path = root / request["outputs"]["selection"]
    selection_path.write_text(json.dumps(selection, indent=2) + "\n", encoding="utf-8")
    EvidenceSet(selection_path, context=context, root=root).report("ground")
    document = dict(_documentation=__doc__, schema_version=1, kind="native_ground_viewport_result",
        complete=True, mode=mode, configuration=request["configuration"], run_id=context.run_id,
        actual_native_viewports_verified=True, publishes_source=False, public_runtime_activated=False,
        source_selector=request["inputs"]["source-selector"], build_result=request["build"]["result"],
        source_inputs=request["inputs"], producer_sources=request["sources"],
        selected_capture=identity(capture_path, root), selection=identity(selection_path, root), **actual)
    with output.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(document, indent=2) + "\n")
    checked_result(identity(output, root), mode, request["configuration"], root)
    return dict(manifest=identity(output, root), checks=["actual original bounded native drawing case passed",
        "complete independent original indexed/RGB container fingerprints matched",
        "original source pixels/composition/geometry/occlusion boundaries verified",
        "current process/source/build/configuration/frame/transcript provenance closed; no operating or human acceptance"])


def execute_prepared(mode, configuration, selector_reference, build_reference, source_reference, run_id):
    """Run a fresh original native capture from explicit current prerequisites and pure source bytes.

    @param mode Exact original registered bounded capture mechanism.
    @param configuration Actual independent current native configuration.
    @param selector_reference Completed actual current Ground consumer result, including its decoder.
    @param build_reference Same current build containing Ground decoder/consumer/viewport targets.
    @param source_reference Explicit completed pure source supplier; its old native outcomes are never reused.
    @param run_id Fresh explicit output invocation, separate from source and other configurations.
    @return Actual completed shared dispatcher verification result.
    """
    from artifact_recipe_stage import execute_request
    identifier(run_id)
    recipe = selected_recipe(mode)
    selector = checked_selectors(selector_reference, "ground", configuration, ROOT)
    path = contained(ROOT / source_reference["path"], ROOT / "mlm/reviews/rebuild")
    if identity(path, ROOT) != source_reference:
        raise ValueError("Ground viewport pure source root changed")
    state = json.loads(path.read_text(encoding="utf-8"))
    # Reuse only completed independently compared source bytes; no old native
    # receipt/configuration or copied application can qualify a current camera.
    if (state.get("kind") != "artifact_rebuild_result" or state.get("complete") is not True
            or state.get("status") != "verified" or state.get("run_id") != selector["run_id"]):
        raise ValueError("Ground viewport requires its exact completed pure source supplier")
    inputs = dict(verify_retained_inputs(recipe, ROOT), **{"source-selector": selector_reference})
    for role, name in (("ground-packing", "ground-derived-inputs"),
                       *((name, name) for name in SOURCE_ROLES[1:])):
        declaration, node = state["signature"]["declarations"][name], state["nodes"][name]
        if (node["status"] != "verified" or declaration["execution"] != "pure_conversion"
                or declaration["configuration_reuse"] != "source_bytes_only"
                or declaration["comparison"] != "independent_source_bytes"):
            raise ValueError("Ground viewport supplier is not an independently checked pure source leaf")
        verify_passed_stage(node["stages"]["verify"], ROOT)
        inputs[role] = node["outputs"]["manifest"]
    selection = load_recipe(recipe["id"], configuration, ROOT)
    if not selection["executable"]:
        raise ValueError("Ground viewport has pending/missing prerequisites")
    sources = snapshot(selection, ROOT)["sources"]
    base = dict(stage="prepare", declaration=recipe, configuration=configuration, inputs=inputs,
                build=dict(status="passed", result=build_reference), paid_work_authorized=False, publishes_source=False)
    checked_request(base, ROOT)
    context = create_run_context(recipe["feature"], recipe["scenario"], run_id=run_id, root=ROOT,
                                 profile="ground-art", process=mode + "-" + configuration.lower())
    last = None
    # Shared requests/results record actual preparation and verification; pure
    # source reuse never substitutes for fresh native drawing or pixel checks.
    for stage in ("prepare", "verify"):
        token = "prepared-" + stage + "-" + uuid.uuid4().hex
        request = dict(base, _documentation=__doc__, schema_version=1, kind="artifact_recipe_stage_request",
            recipe=recipe["id"], stage=stage, run_id=run_id, feature=recipe["feature"], scenario=recipe["scenario"],
            sources=sources, outputs={row["role"]: (context.directory / row["path"].format(configuration=configuration)).relative_to(ROOT).as_posix()
                                     for row in recipe["outputs"]},
            result_path=(context.reports / (token + "-result.json")).relative_to(ROOT).as_posix(), prepared_source=source_reference)
        request_path = context.reports / (token + ".json")
        request_path.write_text(json.dumps(request, indent=2) + "\n", encoding="utf-8")
        last = execute_request(request_path, ROOT)
    return last


def main(default_mode=None, allowed_modes=None):
    """Run one current native review through its original explicitly scoped producer.

    @param default_mode Optional original wrapper's sole default bounded capture mechanism.
    @param allowed_modes Original producer's allowed modes, or all registered modes for the common CLI.
    @return None; actual native/source/pixel/provenance failures retain evidence and propagate nonzero status.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=allowed_modes or tuple(MODES), default=default_mode, required=default_mode is None)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--selector-result", type=Path, required=True)
    parser.add_argument("--build-result", type=Path, required=True)
    parser.add_argument("--source-result", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    args = parser.parse_args()
    result = execute_prepared(args.mode, args.configuration, identity(args.selector_result.resolve(), ROOT),
        identity(args.build_result.resolve(), ROOT), identity(args.source_result.resolve(), ROOT), args.run_id)
    print(json.dumps(dict(recipe=result["recipe"], complete=result["complete"], outputs=result["outputs"]), indent=2))


if __name__ == "__main__":
    main()
