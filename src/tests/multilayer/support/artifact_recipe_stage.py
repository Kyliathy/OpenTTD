"""Execute only implemented, explicitly registered cleanup recipe stage adapters.

@file artifact_recipe_stage.py The producer catalogue remains the sole recipe authority.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group21, bounded prior-stage and historical Tube adapters.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups14/16-22.
Adapters added by their supplying groups call original pure/native owners with
explicit current-run inputs and outputs. No source publication or paid-work
adapter is supplied by this cleanup dispatcher.
"""

import argparse
import json
from pathlib import Path
import sys

from artifact_paths import contained, identifier, repository_root
from artifact_inventory import file_identity
from artifact_recipes import load_catalogue, validate_selection

def space_checkpoint_stage(request, root):
    """Import the implemented pure-seal/current-loader adapter only during execution.

    @param request Exact enabled zero-tick Space recovery stage request.
    @param root Explicit selected checkout.
    @return Actual current checks from the existing native/static qualification owners.
    """
    sys.path.insert(0, str(root / "src/tests/multilayer/acceptance"))
    from space_checkpoint_recipe import execute_stage
    return execute_stage(request, root)


def m1_source_stage(request, root):
    """Import the original source-conversion owner only for a selected recipe stage.

    @param request Exact enabled M1 conversion or packing stage request.
    @param root Selected checkout containing the original media/converter sources.
    @return Actual measured source, palette, geometry and preservation checks.
    """
    sys.path.insert(0, str(root / "media/multilayer"))
    from regenerate_art_sources import execute_stage
    return execute_stage(request, root)


def space_art_source_stage(request, root):
    """Import actual Space source/packing adapters only for a selected stage.

    @param request Exact enabled source or packing recipe request.
    @param root Selected checkout containing immutable original media inputs.
    @return Actual original source and independent canonical byte checks.
    """
    sys.path.insert(0, str(root / "media/multilayer"))
    from regenerate_space_sources import execute_stage
    return execute_stage(request, root)


def ground_art_source_stage(request, root):
    """Import the actual Ground source adapter only for its selected recipe stage.

    @param request Registered current source request with exact generated prerequisites.
    @param root Selected checkout containing independently retained original media.
    @return Actual original source, physical pose and immutable preservation checks.
    """
    sys.path.insert(0, str(root / "media/multilayer"))
    from regenerate_ground_sources import execute_stage
    return execute_stage(request, root)


def sprite_expectation_stage(request, root):
    """Import actual original sprite expectation adapters for the selected family.

    @param request Exact registered current source-expectation prepare/verify request.
    @param root Selected checkout containing immutable original source expectations.
    @return Actual original payload and independent identity/preservation checks.
    """
    from sprite_expected_payload import execute_stage
    return execute_stage(request, root)


def sprite_native_stage(request, root):
    """Import actual original native sprite loaders for one registered family.

    @param request Exact current native configuration, source/build inputs and stage.
    @param root Selected checkout owning current native images and source expectations.
    @return Actual measured loader/process/provenance checks from the original case.
    """
    from sprite_native_loader import execute_stage
    return execute_stage(request, root)


def sprite_native_closure_stage(request, root):
    """Verify the explicit five-family current native closure without another decoder.

    @param request Current aggregate input identities, configuration and output contract.
    @param root Checkout owning every actual completed native witness.
    @return Actual complete five-family observation join through the existing recipe owner.
    """
    from sprite_native_loader import collect_stage
    return collect_stage(request, root)


def space_source_selector_stage(request, root):
    """Dispatch the current original graphics selector wrapper with explicit actual decoder input.

    @param request Exact registered current family/configuration/stage and selected inputs.
    @param root Checkout owning original native source selectors and independent policy.
    @return Actual current measured selector/process checks; no viewport or paid-world scope.
    """
    sys.path.insert(0, str(root / "src/tests/multilayer"))
    from space_source_review import execute_stage
    return execute_stage(request, root)


def launch_occupied_viewport_stage(request, root):
    """Dispatch the original actual occupied launch renderer with current selected inputs.

    @param request Exact registered capture stage, actual build and source/world input roles.
    @param root Checkout owning the original C++ producer and independent source expectations.
    @return Actual complete native pixel/frame/process checks, never ordinary paid-game acceptance.
    """
    sys.path.insert(0, str(root / "src/tests/multilayer"))
    from launch_viewport_review import execute_stage
    return execute_stage(request, root)


def ordinary_paused_viewport_stage(request, root):
    """Dispatch the original ordinary screenshot owner with current explicit paused inputs.

    @param request Registered Space/orbital read-only capture request and current build/source identities.
    @param root Checkout owning retained historical worlds and actual current application.
    @return Actual current native frame, source-pixel and unchanged owner/clock checks.
    """
    sys.path.insert(0, str(root / "src/tests/multilayer"))
    from ordinary_art_review import execute_stage
    return execute_stage(request, root)


def ground_viewport_stage(request, root):
    """Dispatch original bounded Ground native capture and independent pixel owners.

    @param request Registered current mode/configuration/build/source/world stage request.
    @param root Checkout owning actual current viewport target and retained original review inputs.
    @return Actual native/complete-raster/source/composition/provenance checks without progress grants.
    """
    sys.path.insert(0, str(root / "src/tests/multilayer"))
    from ground_viewport_review import execute_stage
    return execute_stage(request, root)


def prior_metadata_stage(request, root):
    """Dispatch one bounded metadata prerequisite through the shared native fixture.

    @param request Exact registered selected metadata reload stage.
    @param root Checkout owning the retained old-format save and current application.
    @return Actual native reload/semantic checks without replaying a historical chain.
    """
    from prior_stage_evidence import execute_stage
    return execute_stage(request, root)


def tube_history_stage(request, root):
    """Verify original terminal history without compiling provisional Tube work.

    @param request Exact registered historical-metadata stage with no native target.
    @param root Checkout containing both durable source epochs and model9 terminal facts.
    @return Historical preservation checks, granting no current native qualification.
    """
    sys.path.insert(0, str(root / "src/tests/multilayer"))
    from qualify_tube_terminals import verify_checkpoint
    result = verify_checkpoint(root)
    if request["stage"] == "verify":
        target = contained(root / request["outputs"]["manifest"], root / "mlm/reviews")
        with target.open("x", encoding="utf-8") as stream:
            stream.write(json.dumps(result, indent=2) + "\n")
    return dict(checks=["retained original24-case/eight-tag model9 terminal history verified",
                        "both source epochs and original media preserved; five-model work remains paused"])


def donor_input_stage(request, root):
    """Dispatch retained donor verification to its original canonical preservation owner.

    @param request Existing exact read-only selected recipe stage.
    @param root Acheckout holding all imported original payloads and namespaced aliases.
    @return Actual input verification checks, never donor/current gameplay acceptance.
    """
    import sys
    sys.path.insert(0, str(root / "src/tests/multilayer"))
    from verify_donor_inputs import execute_stage
    return execute_stage(request, root)


def space_continuation_input_stage(request, root):
    """Inspect the selected retained Space continuation through its existing public owner.

    @param request Exact registered read-only foundation input recipe stage.
    @param root Acheckout containing the complete exported continuation witnesses.
    @return Real closed-policy input checks; no native process or paid work is launched.
    """
    import sys
    sys.path.insert(0, str(root / "src/tests/multilayer/acceptance"))
    from space_checkpoint import inspect_foundation_stage
    return inspect_foundation_stage(request, root)


def merged_ground_stage(request, root):
    """Delegate a registered bounded recovery leaf to its actual acceptance owner.

    @param request Exact selected ground recovery stage and declared input roles.
    @param root Checkout containing current sources and preserved original worlds.
    @return Actual selected original migration, replay, controls or timing checks.
    """
    sys.path.insert(0, str(root / 'src/tests/multilayer/acceptance'))
    from merged_ground_recipe import execute_stage
    return execute_stage(request, root)


HANDLERS = {"space-checkpoint": space_checkpoint_stage,
            **{"merged-ground-" + name: merged_ground_stage for name in
               ("normal-start", "lifecycle", "useful-route", "performance")},
            "donor-underground-inputs": donor_input_stage,
            "space-continuation-inputs": space_continuation_input_stage,
            "state-prior-native": prior_metadata_stage,
            "tube-checkpoint": tube_history_stage,
            **{"ground-viewport-" + mode: ground_viewport_stage for mode in
               ("collector", "industry-states", "purchases", "rail", "road", "stations")},
            **{family + "-paused-viewports": ordinary_paused_viewport_stage for family in ("space", "orbital")},
            "launch-occupied-viewports": launch_occupied_viewport_stage,
            **{family + "-current-producers": space_source_selector_stage for family in ("launch", "orbital", "space", "ground")},
            **{family + "-native-loader": sprite_native_stage for family in ("m1", "space", "orbital", "launch", "ground")},
            "sprite-native-loaders": sprite_native_closure_stage,
            **{family + "-expected-payload": sprite_expectation_stage for family in ("m1", "space", "orbital", "launch", "ground")},
            **{"m1-source-" + stage: m1_source_stage for stage in
               ("base", "terrain", "portal", "vehicle", "purchase", "facility", "control", "hardness", "collector")},
            "m1-derived-inputs": m1_source_stage,
            "merged-art-candidates": m1_source_stage,
            **{"space-source-" + asset: space_art_source_stage for asset in
               ("vehicle-model-15", "vehicle-model-19", "vehicle-model-20", "vehicle-model-18",
                "asteroid-nickel", "asteroid-ice", "asteroid-rare-earth")},
            "space-derived-inputs": space_art_source_stage,
            **{"orbital-source-vehicle-model-" + model: space_art_source_stage for model in ("21", "16", "17")},
            **{"orbital-source-module-" + module: space_art_source_stage for module in
               ("nucleus", "dock", "storage", "power", "fuel-store", "workshop", "processor")},
            "orbital-derived-inputs": space_art_source_stage,
            "launch-source-facility-launch-center": space_art_source_stage,
            "launch-source-vehicle-model-14": space_art_source_stage,
            "launch-derived-inputs": space_art_source_stage,
            **{"ground-source-mine-" + resource: ground_art_source_stage for resource in
               ("coal", "iore", "core", "salt", "lith", "cobt", "rare")},
            **{"ground-source-facility-" + facility: ground_art_source_stage for facility in
               ("metal-refinery", "chemical-works", "battery-works", "electronics-works", "rail-station", "road-terminal",
                "tube-terminal", "tube-service", "tube-interchange", "vent-powerhouse", "magma-powerhouse", "utility-riser")},
            **{"ground-source-vehicle-model-" + str(model).zfill(2): ground_art_source_stage for model in range(5, 14)},
            **{"ground-source-tube-" + shape: ground_art_source_stage for shape in
               ("straight", "bend45", "junction", "end", "crossing", "conduit")},
            "ground-source-geothermal-vent": ground_art_source_stage,
            "ground-source-geothermal-conduit": ground_art_source_stage,
            "ground-source-control-tube": ground_art_source_stage,
            "ground-derived-inputs": ground_art_source_stage}


def validated_prerequisite_runs(request, recipe, root):
    """Bind a fresh viewport's precise inputs to actual current supplying runs.

    @param request Registered capture request with an explicit completed pure source result.
    @param recipe Exact enabled catalogue declaration already checked by the dispatcher.
    @param root Checkout owning the source graph, current selectors and actual images.
    @return Exact selected prerequisite path/hash to verified supplier invocation map.
    """
    handler = HANDLERS.get(recipe["id"])
    # Only the existing prepared capture owners have a separate output run.
    # Other adapters retain the ordinary same-invocation dependency boundary.
    if request.get("prepared_source") is None or handler not in (launch_occupied_viewport_stage, ground_viewport_stage):
        return {}
    from artifact_rebuild import identity, load_recipe, snapshot, verify_passed_stage
    reference = request["prepared_source"]
    path = contained(root / reference["path"], root / "mlm/reviews/rebuild")
    if identity(path, root) != reference:
        raise ValueError("prepared viewport source result changed")
    source = json.loads(path.read_text(encoding="utf-8"))
    # A copied old success label cannot authorize foreign generated files.
    # Current declarations, original inputs and producing code must still match.
    if (source.get("kind") != "artifact_rebuild_result" or source.get("complete") is not True
            or source.get("status") != "verified" or source["signature"]["configuration"] is not None):
        raise ValueError("prepared viewport requires a completed pure source graph")
    supplier_run = identifier(source["run_id"])
    expected_path = root / "mlm/reviews/rebuild" / identifier(source["signature"]["recipe"]) / supplier_run / "reports/rebuild.json"
    if path != expected_path.resolve():
        raise ValueError("prepared viewport source graph recipe/run changed")
    selection = load_recipe(source["signature"]["recipe"], None, root)
    # Native branches cannot be borrowed as source-only qualification. The
    # actual current selector is checked independently below against its build.
    if (not selection["executable"] or any(row["target"] is not None or row["execution"] != "pure_conversion"
            or row["configuration_reuse"] != "source_bytes_only" for row in selection["recipes"].values())):
        raise ValueError("prepared viewport supplier is not an executable pure source graph")
    current = snapshot(selection, root)
    if any(source["signature"].get(key) != value for key, value in current.items()):
        raise ValueError("prepared viewport source graph inputs/code/declarations changed")
    sys.path.insert(0, str(root / "src/tests/multilayer"))
    from space_source_review import checked_result, selected_recipe
    family = "launch" if handler is launch_occupied_viewport_stage else "ground"
    selector_recipe = selected_recipe(family)
    result = {}
    # Bind each declared role to its actual producing leaf, including exact
    # run/feature/scenario/output placement. A whole old run is never exempted.
    for requirement in recipe["requires"]:
        supplied = request["inputs"][requirement["as"]]
        if requirement["as"] == "source-selector":
            if requirement["recipe"] != selector_recipe["id"] or requirement["role"] != "manifest":
                raise ValueError("prepared viewport source selector declaration changed")
            selector = checked_result(supplied, family, request["configuration"], root)
            decoder = json.loads((root / selector["decoder_prerequisite"]["path"]).read_text(encoding="utf-8"))
            # Current selector/process/candidate checks must agree with the
            # exact source root and current capture build, not just equal pixels.
            if (selector["run_id"] != supplier_run or selector["build_result"] != request["build"]["result"]
                    or decoder.get("source_generation") != reference):
                raise ValueError("prepared viewport selector/source/build handoff disagrees")
        else:
            declaration = source["signature"]["declarations"].get(requirement["recipe"])
            node = source["nodes"].get(requirement["recipe"])
            # A source-only role must have a real passed pure leaf; neither a
            # native result nor a hand-edited node/output can replace its supplier.
            if (declaration is None or node is None or node["status"] != "verified"
                    or declaration["execution"] != "pure_conversion"
                    or declaration["configuration_reuse"] != "source_bytes_only"
                    or node["stages"]["verify"]["status"] != "passed"):
                raise ValueError("prepared viewport lacks a verified pure prerequisite")
            output = next(row for row in declaration["outputs"] if row["role"] == requirement["role"])
            expected = root / "mlm/reviews" / declaration["feature"] / declaration["scenario"] / supplier_run / output["path"].format(configuration=None)
            if supplied != node["outputs"].get(requirement["role"]) or (root / supplied["path"]).resolve() != expected.resolve():
                raise ValueError("prepared viewport pure prerequisite recipe/role/run changed")
            verify_passed_stage(node["stages"]["verify"], root)
            stage = json.loads((root / node["stages"]["verify"]["result"]["path"]).read_text(encoding="utf-8"))
            if stage["outputs"].get(requirement["role"]) != supplied:
                raise ValueError("prepared viewport pure prerequisite differs from its passed stage")
        result[(supplied["path"], supplied["sha256"])] = supplier_run
    return result


def execute_request(path, root=None):
    """Dispatch one exact executor-owned stage request to an implemented adapter.

    @param path Explicit current-run request JSON, never latest-run discovery.
    @param root Optional selected source checkout.
    @return Adapter result after actual selected verification, or an explicit refusal.
    """
    root = repository_root(root)
    path = contained(path, root / "mlm/reviews")
    request = json.loads(path.read_text(encoding="utf-8"))
    # Only an exact current request with a known stage/adapter can execute.
    # A declaration or old complete flag cannot invent an implemented producer.
    if request["schema_version"] != 1 or request["kind"] != "artifact_recipe_stage_request" or request["stage"] not in ("prepare", "verify") or request["publishes_source"] is not False or request["paid_work_authorized"] is not False:
        raise ValueError("unsupported/unsafe recipe stage request")
    # The supervisor validates the whole catalogue before selecting any work
    # and binds its exact bytes in sources below. A child rechecks its complete
    # selected closure, avoiding unrelated original-file scans for every leaf.
    recipes = validate_selection(load_catalogue(root), request["recipe"], root)
    recipe = recipes.get(request["recipe"])
    # Direct dispatcher invocation follows the same registered/enabled boundary
    # as the supervisor. Editing a request cannot enable a pending producer.
    if recipe is None or not recipe["enabled"] or request["declaration"] != recipe:
        raise ValueError("recipe stage declaration is unregistered, changed or pending")
    from artifact_recipes import check_reference_roles
    expected_roles = {row["role"] for row in recipe["retained_inputs"]} | {row["as"] for row in recipe["requires"]}
    if set(request["inputs"]) != expected_roles:
        raise ValueError("recipe stage input roles differ from selected declaration")
    run = contained(root / "mlm/reviews" / recipe["feature"] / recipe["scenario"] / identifier(request["run_id"]), root / "mlm/reviews")
    expected_outputs = {row["role"]: (run / row["path"].format(configuration=request["configuration"])).relative_to(root).as_posix()
                        for row in recipe["outputs"]}
    if request.get("prepared_attempt") is not None:
        # Repeated native art observations keep earlier failures/results in
        # their own declared role folders; only existing native art leaves own
        # this bounded attempt namespace, never arbitrary converters/publishers.
        from sprite_native_loader import prepared_outputs, selected_recipe
        from artifact_paths import RunContext
        family = recipe["original_parameters"].get("family")
        if family is None or selected_recipe(family) != recipe:
            raise ValueError("prepared attempt is not an existing native art leaf")
        expected_outputs = prepared_outputs(RunContext(root, recipe["feature"], recipe["scenario"], request["run_id"]),
                                            recipe, request["configuration"], request["prepared_attempt"])
    if request["outputs"] != expected_outputs:
        raise ValueError("recipe stage output is outside its selected recipe/run")
    prerequisites = [request["inputs"][row["as"]] for row in recipe["requires"]]
    supplier_runs = validated_prerequisite_runs(request, recipe, root)
    check_reference_roles(request, root, prerequisites, request["run_id"], verify_bytes=False, generated_runs=supplier_runs)
    witnesses = [*request["inputs"].values(), *request["sources"]]
    # The checked source graph authorizes these precise cross-run inputs.
    # Keep that authority byte-exact before and after the capture adapter too.
    if supplier_runs:
        witnesses.append(request["prepared_source"])
    for reference in witnesses:
        if file_identity(contained(root / reference["path"], root))[0] != reference["sha256"]:
            raise ValueError("recipe stage input/source changed")
    handler = HANDLERS.get(request["recipe"])
    if handler is None:
        raise ValueError("recipe stage adapter is not implemented; complete its supplying group")
    facts = handler(request, root)
    # A zero child status alone is insufficient: the actual adapter must name
    # measured checks, then bind its completed result to this exact request.
    if not isinstance(facts, dict) or not facts.get("checks"):
        raise ValueError("recipe adapter did not return actual measured checks")
    destination = contained(root / request["result_path"], path.parent)
    outputs = {}
    # Only actual completed file outputs become stage witnesses. Directory
    # inventories are explicit files; an existing empty folder cannot pass.
    for role, relative in request["outputs"].items():
        actual = contained(root / relative, root / "mlm/reviews")
        if actual.is_file():
            outputs[role] = dict(path=relative, sha256=file_identity(actual)[0])
    if request["stage"] == "verify" and outputs.keys() != request["outputs"].keys():
        raise ValueError("verification did not produce every declared output role")
    # Close actual retained/source identities after the adapter returns. A
    # publisher or concurrent edit cannot become a successful conversion proof.
    for reference in witnesses:
        if file_identity(contained(root / reference["path"], root))[0] != reference["sha256"]:
            raise ValueError("recipe stage changed its retained input/source")
    result = dict(_documentation="Owner: r2fc-file-cleanup3.md14; actual completed adapter checks.",
        schema_version=1, kind="artifact_recipe_stage_result", complete=True,
        recipe=request["recipe"], stage=request["stage"], run_id=request["run_id"], configuration=request["configuration"],
        request=dict(path=path.relative_to(root).as_posix(), sha256=file_identity(path)[0]),
        inputs=request["inputs"], sources=request["sources"], outputs=outputs, facts=facts)
    with destination.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(result, indent=2) + "\n")
    return result


def main():
    """Run one structured executor request; no adapter is selected implicitly.

    @return None; adapter failures propagate as unsuccessful actual child status.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--request", required=True, type=Path)
    arguments = parser.parse_args()
    execute_request(arguments.request)


if __name__ == "__main__":
    main()
