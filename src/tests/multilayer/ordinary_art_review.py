"""Bind original paused Space/orbital capture owners to explicit current recipe inputs.

@file ordinary_art_review.py Actual ordinary native frames, original source pixels and unchanged owners.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19C.
"""
import argparse
import json
from pathlib import Path
import runpy
import sys
import uuid

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "media/multilayer"))
from artifact_paths import RunContext, contained, create_run_context, identifier
from artifact_rebuild import (identity, load_recipe, snapshot, verify_current_build,
                              verify_passed_stage, verify_retained_inputs)
from artifact_recipes import load_catalogue
from evidence import EvidenceSet, bind_capture, selection_document
from space_source_review import checked_result

CONTRACT = "src/tests/fixtures/graphics/paused-review-worlds.json"


def selected_recipe(family):
    """Select the sole actual ordinary application declaration for this family.

    @param family Explicit Space or orbital original screenshot mechanism.
    @return Enabled current bounded declaration with no paid/live behavior.
    """
    if family not in ("space", "orbital"):
        raise ValueError("unknown paused ordinary capture family")
    recipe = next(row for row in load_catalogue(ROOT)["recipes"] if row["id"] == family + "-paused-viewports")
    # An ordinary field is supplied by its existing GUI target and real startup
    # arguments; a Catch decoder or paid fixture cannot qualify this observer.
    if (not recipe["enabled"] or recipe["execution"] != "native_application"
            or recipe["target"] != "openttd" or recipe["original_parameters"]["live"] is not False):
        raise ValueError("paused capture requires its enabled ordinary application contract")
    return recipe


def checked_request(request, root):
    """Close the selected current build/decoder/selectors and original retained world/source bytes.

    @param request Exact registered read-only stage and input/output identities.
    @param root Checkout owning the original sources and actual current application.
    @return Original declaration, family, selector, world contract, source and actual application.
    """
    family = request["declaration"]["original_parameters"]["family"]
    recipe = selected_recipe(family)
    # Cleanup may inspect already-earned history. It cannot advance the active
    # continuation, publish art or invoke the paid construction/manufacture cases.
    if (root.resolve() != ROOT or request["declaration"] != recipe
            or request["stage"] not in ("prepare", "verify")
            or request["paid_work_authorized"] is not False or request["publishes_source"] is not False):
        raise ValueError("ordinary capture requires its explicit paused read-only scope")
    configuration = request["configuration"]
    selector = checked_result(request["inputs"]["source-selector"], family, configuration, root)
    verify_current_build(request["build"], configuration, recipe, root)
    if selector["build_result"] != request["build"]["result"]:
        raise ValueError("ordinary capture and native source checks require the same actual current build")
    build = json.loads((root / request["build"]["result"]["path"]).read_text(encoding="utf-8"))
    application = root / build["targets"]["openttd"]["path"]
    contract_reference = request["inputs"]["world-contract"]
    if identity(root / CONTRACT, root) != contract_reference:
        raise ValueError("ordinary capture changed its independent retained-world contract")
    contract = json.loads((root / CONTRACT).read_text(encoding="utf-8"))
    source = contained(root / request["inputs"]["world"]["path"], root)
    original = contract["worlds"][family]["source"]
    # Exact historical bytes must come from a durable fixture owner; neither a
    # convenient newer world nor a still-present disposable alias substitutes.
    if (identity(source, root) != request["inputs"]["world"]
            or request["inputs"]["world"]["sha256"] != original["sha256"]
            or not any(source.is_relative_to((root / owner).resolve())
                       for owner in ("savegames/retained", "src/tests/fixtures"))):
        raise ValueError("ordinary capture requires its exact retained original world")
    packing = request["inputs"]["packing"]
    manifest = json.loads((root / packing["path"]).read_text(encoding="utf-8"))
    # Source output can be shared across configurations only with the original
    # completed pure leaf and independent canonical bytes. Native proof stays fresh.
    if (identity(root / packing["path"], root) != packing
            or manifest["kind"] != "selected_" + family + "_packing"
            or manifest["run_id"] != selector["run_id"]):
        raise ValueError("ordinary capture selected another source packing invocation")
    for reference in [*manifest["outputs"], *manifest["source_reviews"].values()]:
        if identity(root / reference["path"], root) != reference:
            raise ValueError("ordinary capture packed/source input changed")
    sheet = request["inputs"]["sheet"]
    canonical = root / "media/baseset/openttd" / (family + "_final.png")
    if sheet not in manifest["outputs"] or identity(canonical, root)["sha256"] != sheet["sha256"]:
        raise ValueError("ordinary capture source sheet differs from original canonical packed pixels")
    return recipe, family, selector, contract, source, application


def execute_stage(request, root):
    """Run and seal the actual original ordinary screenshot and exact pixel/owner checks.

    @param request Registered current prepare/verify stage with explicit source and output run.
    @param root Checkout owning current native images, retained originals and selected packing.
    @return Measured native provenance, exact original pixels and zero-progress observations.
    """
    recipe, family, selector, contract, source, application = checked_request(request, root)
    if request["stage"] == "prepare":
        return dict(checks=["current same-build native decoder/source selectors verified",
                            "independent original retained world and packed source bytes verified"])
    context = RunContext(root, recipe["feature"], recipe["scenario"], request["run_id"],
                         "ordinary-art", request["configuration"].lower())
    output = contained(root / request["outputs"]["manifest"], context.reports)
    if output.exists():
        raise ValueError("ordinary capture output exists; select a fresh explicit run")
    wrapper_path = root / recipe["producer"]
    wrapper = runpy.run_path(str(wrapper_path))
    actual = wrapper["capture"](context, request["configuration"], application, source, contract)
    pixels = runpy.run_path(str(wrapper_path.with_name(family + "_final_art_checks-2.py")))
    manifest = json.loads((root / request["inputs"]["packing"]["path"]).read_text(encoding="utf-8"))
    expected = contract["worlds"][family]["expected"]
    with Image.open(root / actual["frames"][0]["path"]) as native, Image.open(root / request["inputs"]["sheet"]["path"]) as sheet:
        if family == "space":
            matches = pixels["native_matches"](native, manifest, sheet, expected["asset_id"], expected["state"])
        else:
            matches = pixels["native_matches"](native, manifest, sheet, expected["asset_ids"])
    if not matches:
        raise ValueError("required genuine original native artwork is absent")
    checked_request(request, root)
    semantic = dict(_documentation=__doc__, schema_version=1, kind="native_capture_semantics",
        producer=recipe["producer"], feature=context.feature, scenario=context.scenario,
        run_id=context.run_id, configuration=request["configuration"],
        source=request["inputs"]["world"]["path"], source_sha256=request["inputs"]["world"]["sha256"],
        frames=actual["frames"], live=False, simulation_ticks=0, ordinary_game_reviewed=False)
    semantic_path = context.reports / "native-capture.json"
    semantic_path.write_text(json.dumps(semantic, indent=2) + "\n", encoding="utf-8")
    capture = bind_capture(context, semantic_path, request["configuration"])
    capture_path = root / request["outputs"]["capture-index"]
    capture_path.write_text(json.dumps(capture, indent=2) + "\n", encoding="utf-8")
    selection = selection_document(context, request["configuration"])
    selection.update(outputs={}, reports={"paused": identity(capture_path, root)}, evidence={"paused": capture["terminal"]})
    selection_path = root / request["outputs"]["selection"]
    selection_path.write_text(json.dumps(selection, indent=2) + "\n", encoding="utf-8")
    EvidenceSet(selection_path, context=context, root=root).report("paused")
    document = dict(_documentation=__doc__, schema_version=1, kind="native_paused_viewport_result",
        complete=True, family=family, configuration=request["configuration"], run_id=context.run_id,
        actual_native_viewports_verified=True, live=False, simulation_ticks=0, paid_work_authorized=False,
        ordinary_game_reviewed=False, personal_review_complete=False, publishes_source=False,
        source_selector=request["inputs"]["source-selector"], build_result=request["build"]["result"],
        source_inputs=request["inputs"], producer_sources=request["sources"], exact_native_matches=matches,
        selected_capture=identity(capture_path, root), selection=identity(selection_path, root), **actual)
    with output.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(document, indent=2) + "\n")
    return dict(manifest=identity(output, root), checks=["actual original ordinary native screenshot process exited successfully",
        "original physical/metadata owners and all saved DATE/SRND clock/RNG fields unchanged",
        "required genuine original packed source pixels matched actual indexed native field",
        "current process/frame/source/build/configuration/transcript provenance closed; no paid or human acceptance"])


def execute_prepared(family, configuration, selector_reference, build_reference, source_reference, run_id):
    """Prepare a fresh paused capture from actual current selectors and an explicit pure packing witness.

    @param family Original Space or orbital paused capture mechanism.
    @param configuration Actual independent current application configuration.
    @param selector_reference Exact current family selector result, including its actual decoder.
    @param build_reference Same actual current build containing ordinary and isolated art targets.
    @param source_reference Explicit completed supplying root; only its pure packing leaf is reused.
    @param run_id Fresh configuration-specific capture invocation.
    @return Completed actual dispatcher verification result.
    """
    from artifact_recipe_stage import execute_request
    identifier(run_id)
    recipe = selected_recipe(family)
    selector = checked_result(selector_reference, family, configuration, ROOT)
    path = contained(ROOT / source_reference["path"], ROOT / "mlm/reviews/rebuild")
    if identity(path, ROOT) != source_reference:
        raise ValueError("ordinary capture pure source root changed")
    state = json.loads(path.read_text(encoding="utf-8"))
    # Reuse only a completed independently compared source-byte leaf. Old native
    # results or the source root's configuration cannot qualify the current GUI.
    if (state.get("kind") != "artifact_rebuild_result" or state.get("complete") is not True
            or state.get("status") != "verified" or state.get("run_id") != selector["run_id"]):
        raise ValueError("ordinary capture requires its completed explicit pure source root")
    name = family + "-derived-inputs"
    declaration, node = state["signature"]["declarations"][name], state["nodes"][name]
    if (node["status"] != "verified" or declaration["execution"] != "pure_conversion"
            or declaration["configuration_reuse"] != "source_bytes_only"
            or declaration["comparison"] != "independent_source_bytes"):
        raise ValueError("ordinary capture packing supplier is not an independently checked pure leaf")
    verify_passed_stage(node["stages"]["verify"], ROOT)
    inputs = dict(verify_retained_inputs(recipe, ROOT), **{"source-selector": selector_reference,
                  "packing": node["outputs"]["manifest"], "sheet": node["outputs"]["png"]})
    selection = load_recipe(recipe["id"], configuration, ROOT)
    if not selection["executable"]:
        raise ValueError("ordinary capture has pending/missing prerequisites")
    sources = snapshot(selection, ROOT)["sources"]
    base = dict(stage="prepare", declaration=recipe, configuration=configuration, inputs=inputs,
                build=dict(status="passed", result=build_reference), paid_work_authorized=False, publishes_source=False)
    checked_request(base, ROOT)
    context = create_run_context(recipe["feature"], recipe["scenario"], run_id=run_id, root=ROOT,
                                 profile="ordinary-art", process=configuration.lower())
    last = None
    # Real shared dispatcher stages bind the original current request/results;
    # completed source conversion is never relabelled as current native capture.
    for stage in ("prepare", "verify"):
        token = "prepared-" + stage + "-" + uuid.uuid4().hex
        request = dict(base, _documentation=__doc__, schema_version=1, kind="artifact_recipe_stage_request",
            recipe=recipe["id"], stage=stage, run_id=run_id, feature=recipe["feature"], scenario=recipe["scenario"],
            sources=sources, outputs={row["role"]: (context.directory / row["path"].format(configuration=configuration)).relative_to(ROOT).as_posix()
                                     for row in recipe["outputs"]},
            result_path=(context.reports / (token + "-result.json")).relative_to(ROOT).as_posix())
        request_path = context.reports / (token + ".json")
        request_path.write_text(json.dumps(request, indent=2) + "\n", encoding="utf-8")
        last = execute_request(request_path, ROOT)
    return last


def main(default_family=None):
    """Run one original bounded ordinary capture with explicit current prerequisites.

    @param default_family Original wrapper's fixed family, or an explicit CLI selection.
    @return None; actual native/owner/pixel failures propagate with their retained current evidence.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--family", choices=("space", "orbital"), default=default_family, required=default_family is None)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--selector-result", type=Path, required=True)
    parser.add_argument("--build-result", type=Path, required=True)
    parser.add_argument("--source-result", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    args = parser.parse_args()
    result = execute_prepared(args.family, args.configuration, identity(args.selector_result.resolve(), ROOT),
        identity(args.build_result.resolve(), ROOT), identity(args.source_result.resolve(), ROOT), args.run_id)
    print(json.dumps(dict(recipe=result["recipe"], complete=result["complete"], outputs=result["outputs"]), indent=2))


if __name__ == "__main__":
    main()
