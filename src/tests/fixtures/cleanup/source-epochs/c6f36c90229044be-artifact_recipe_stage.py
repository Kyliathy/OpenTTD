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

from artifact_paths import contained, repository_root
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


HANDLERS = {"space-checkpoint": space_checkpoint_stage,
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
    for reference in [*request["inputs"].values(), *request["sources"]]:
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
    for reference in [*request["inputs"].values(), *request["sources"]]:
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
