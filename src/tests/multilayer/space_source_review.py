"""Run selected original graphics source selectors with current decoder prerequisites.

@file space_source_review.py Component source review, without paid worlds or viewport claims.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups19/19B/20.
The existing producer catalogue owns target/tag selection. Actual18C loader
results remain separate prerequisites, never synthesized by component tests.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import time
import uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))

from artifact_paths import RunContext, contained, identifier
from artifact_rebuild import identity, load_recipe, snapshot, verify_current_build, verify_retained_inputs
from artifact_recipes import load_catalogue
from runner import run_native
from sprite_native_loader import prepared_outputs


def selected_recipe(family):
    """Select one existing original producer through the sole recipe catalogue.

    @param family Explicit launch, orbital, space or ground presentation family.
    @return Enabled current source-selector declaration, with no hidden gameplay observer.
    """
    if family not in ("launch", "orbital", "space", "ground"):
        raise ValueError("unknown current graphics source-selector family")
    recipe = next(row for row in load_catalogue(ROOT)["recipes"] if row["id"] == family + "-current-producers")
    # Hidden native observers can purchase or construct. This source-only
    # component boundary owns only the explicitly registered ordinary cases.
    if (not recipe["enabled"] or not recipe["test_tags"]
            or any(tag.startswith("[.") for tag in recipe["test_tags"])
            or recipe["target"] != ("openttd_ground_consumer_test" if family == "ground" else "openttd_space_art_test")):
        raise ValueError("current source selectors require their enabled isolated component cases")
    return recipe


def validate_native_manifest(document, family, configuration, run_id):
    """Refuse historical, source-only or unrelated decoder outcomes before selecting an image.

    @param document Explicit upstream actual18C native result, not an arbitrary complete flag.
    @param family Current source-selector family; launch uses the launch-fleet binary protocol.
    @param configuration Actual independent current native configuration.
    @param run_id Explicit invocation supplying the source/decoder prerequisites.
    @return None; physical source/build/process identities are checked separately afterward.
    """
    protocol = "launch-fleet" if family == "launch" else family
    # Component success cannot invent decoder acceptance or acquire a paid
    # world, publication scope, another family or another configuration's proof.
    if (document.get("kind") != "native_sprite_loader_result" or document.get("complete") is not True
            or document.get("native_loading_verified") is not True or document.get("family") != protocol
            or document.get("recipe") != family + "-native-loader"
            or document.get("configuration") != configuration or document.get("run_id") != run_id
            or document.get("paid_world_loaded") is not False or document.get("publishes_source") is not False):
        raise ValueError("source selector requires its exact actual current native decoder prerequisite")


def checked_request(request, root):
    """Close actual upstream decoder/build/source identities before any selector process.

    @param request Exact registered current prepare/verify stage and explicit input roles.
    @param root Checkout owning this producer and actual native images.
    @return Current recipe, actual upstream decoder and independently built application.
    """
    if request.get("prepared_attempt") is not None:
        identifier(request["prepared_attempt"])
    # Only these isolated source queries are authorized. Captures, paid work
    # and source publication require their own later plan groups and producers.
    if (root.resolve() != ROOT or request["stage"] not in ("prepare", "verify")
            or request["configuration"] not in ("Debug", "RelWithDebInfo")
            or request["paid_work_authorized"] is not False or request["publishes_source"] is not False):
        raise ValueError("current source selectors require explicit read-only configuration scope")
    recipe = request["declaration"]
    family = recipe["original_parameters"]["family"]
    if recipe != selected_recipe(family):
        raise ValueError("source-selector declaration changed")
    reference = request["inputs"]["native-loader"]
    path = contained(root / reference["path"], root / "mlm/reviews")
    if identity(path, root) != reference:
        raise ValueError("current decoder prerequisite identity changed")
    native = json.loads(path.read_text(encoding="utf-8"))
    validate_native_manifest(native, family, request["configuration"], request["run_id"])
    verify_current_build(request["build"], request["configuration"], recipe, root)
    build = json.loads((root / request["build"]["result"]["path"]).read_text(encoding="utf-8"))
    application = contained(root / build["targets"][recipe["target"]]["path"], root)
    # A prior real decoder pass still cannot qualify a replaced executable.
    # The current component and decoder processes must use the same exact build.
    if (native["build_result"] != request["build"]["result"]
            or native["application"] != build["targets"]["openttd_ground_art_test" if family == "ground" else recipe["target"]]
            or native["source_epoch"] != build["source_epoch"]
            or application.parent.name != request["configuration"]):
        raise ValueError("source selectors and decoder used different actual current builds")
    # Close measured evidence and independently generated inputs, rather than
    # accepting a detached successful flag with absent process/source witnesses.
    for witness in (native["native_process"], native["terminal"], native["expected_manifest"],
                    native["expected_payload"], native["runtime_payload"], native["runtime_config"]):
        if identity(root / witness["path"], root) != witness:
            raise ValueError("current native decoder witness changed")
    process = json.loads((root / native["native_process"]["path"]).read_text(encoding="utf-8"))
    text = (root / native["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
    validate_native_process(native, process, text)
    # Runner-retained stdout/stderr remain actual measured evidence too; an
    # intact top-level report cannot conceal a removed execution transcript.
    for witness in process["transcripts"]:
        if identity(root / witness["path"], root) != witness:
            raise ValueError("current native process transcript changed")
    if family == "ground":
        from run_ground_consumer_checks import verify_consumer_inputs
        verify_consumer_inputs(request, root)
    return recipe, native, application


def checked_result(reference, family, configuration, root):
    """Validate an actual current selector result before a later independent viewport.

    @param reference Explicit measured selector result identity, never latest-run selection.
    @param family Exact source family whose component/owner boundaries must have passed.
    @param configuration Same actual independent build configuration as the later observer.
    @param root Checkout owning current producers and physical process/source witnesses.
    @return Current selector document after actual upstream/build/process/policy closure.
    """
    path = contained(root / reference["path"], root / "mlm/reviews")
    if identity(path, root) != reference:
        raise ValueError("current source-selector prerequisite changed")
    document = json.loads(path.read_text(encoding="utf-8"))
    # A capture cannot borrow incomplete or unrelated component results, and
    # current producer success grants neither paid-world nor publication scope.
    if (document.get("kind") != "native_source_selector_result" or document.get("complete") is not True
            or document.get("source_selectors_verified") is not True or document.get("family") != family
            or document.get("configuration") != configuration or document.get("paid_world_loaded") is not False
            or document.get("viewport_captured") is not False or document.get("publishes_source") is not False):
        raise ValueError("viewport requires its exact current source-selector prerequisite")
    recipe = selected_recipe(family)
    request = dict(stage="verify", declaration=recipe, configuration=configuration,
        run_id=identifier(document["run_id"]), inputs=document["selected_inputs"],
        build=dict(result=document["build_result"]), paid_work_authorized=False, publishes_source=False)
    # Close the complete producer chain and selected source contracts. A
    # plausible old result cannot certify a newer wrapper or native image.
    for witness in [*document["producer_sources"], *document["selected_inputs"].values(),
                    document["native_process"], document["terminal"]]:
        if identity(root / witness["path"], root) != witness:
            raise ValueError("current selector source/process witness changed")
    checked_request(request, root)
    process = json.loads((root / document["native_process"]["path"]).read_text(encoding="utf-8"))
    text = (root / document["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
    summary = re.search(r"All tests passed \(([\d,]+) assertions in (\d+) test cases?\)", text)
    # Exact case selection and an input-free component process keep actual
    # saved-world/capture observers from masquerading as a source selector.
    if (process.get("exit_status") != 0 or process.get("input_integrity_passed", True) is not True
            or process.get("configuration") != configuration or process.get("run_id") != document["run_id"]
            or process.get("executable") != document["application"] or process.get("inputs") != []
            or process.get("argv", [])[1:] != [",".join(recipe["test_tags"]), "--reporter", "console", "--durations", "yes"]
            or summary is None or int(summary.group(1).replace(",", "")) != document["assertions"]
            or int(summary.group(2)) != document["cases"] or document["cases"] != recipe["original_parameters"]["cases"]):
        raise ValueError("current selector process/case/terminal evidence disagrees")
    # Keep the runner's exact retained stdout/stderr available for consumers;
    # the producer's selected terminal alone is not the whole process record.
    for witness in process["transcripts"]:
        if identity(root / witness["path"], root) != witness:
            raise ValueError("current selector retained transcript changed")
    if family == "launch":
        from launch_fleet_runtime_checks import rocket
        policy = launch_policy(request)
        if rocket.verify_observations(text, policy) != document["source_policy_comparisons"]:
            raise ValueError("current launch policy observation coverage changed")
        if rocket.verify_refusals(text, policy) != document["detached_policy_refusals"]:
            raise ValueError("current launch policy refusal coverage changed")
    return document


def validate_native_process(native, process, text):
    """Match an upstream decoder outcome to its actual process, source binding and terminal.

    @param native Explicit actual decoder result whose physical witnesses were already closed.
    @param process Actual runner process report, never a component-selector substitute.
    @param text Exact actual decoder transcript from that result.
    @return None; inconsistent process/configuration/input/success evidence refuses.
    """
    binding = dict(role="expected_sprites", family=native["family"], configuration=native["configuration"],
                   **native["expected_payload"], source_manifest=native["expected_manifest"])
    summary = re.search(r"All tests passed \(([\d,]+) assertions in 1 test case\)", text)
    # A successful unrelated process, stale input binding or source-only flag
    # cannot become actual current decoder qualification for these selectors.
    if (process.get("exit_status") != 0 or process.get("input_integrity_passed") is not True
            or process.get("configuration") != native["configuration"] or process.get("run_id") != native["run_id"]
            or process.get("executable") != native["application"]
            or process.get("argv", [])[1:] != [native["tag"], "--reporter", "console", "--durations", "yes"]
            or process.get("inputs") != [native["expected_payload"]]
            or binding not in process.get("bound_inputs", []) or summary is None
            or int(summary.group(1).replace(",", "")) != native["assertions"] or native["marker"] not in text):
        raise ValueError("actual decoder process/source/terminal evidence disagrees")


def run_native_selectors(context, recipe, configuration, application, terminal):
    """Execute the original bounded component cases and retain their exact real process.

    @param context Explicit family/scenario/source invocation with isolated evidence ownership.
    @param recipe Registered actual target/tags and complete expected case count.
    @param configuration Actual independently built Debug or RelWithDebInfo image.
    @param application Exact current executable already closed against its build result.
    @param terminal New selected transcript path; an earlier outcome is never overwritten.
    @return Real process/result counts, with no source, viewport or gameplay publication.
    """
    started = time.monotonic()
    environment = context.environment()
    environment["OTTD_TEST_CONFIGURATION"] = configuration
    with terminal.open("x", encoding="utf-8") as output:
        result = run_native(context, [str(application), ",".join(recipe["test_tags"]),
            "--reporter", "console", "--durations", "yes"], cwd=ROOT / "build",
            native_producer=recipe["producer"],
            env=environment, stdout=output,
            stderr=subprocess.STDOUT, timeout=recipe["native_timeout_seconds"], check=False)
    text = terminal.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \(([\d,]+) assertions in (\d+) test cases?\)", text)
    # A partial filter, empty successful process or wrong case count cannot
    # certify the entire declared family/owner boundary.
    if result.returncode != 0 or passed is None or int(passed.group(2)) != recipe["original_parameters"]["cases"]:
        raise ValueError("actual current source-selector cases failed: " + str(terminal))
    return dict(native_process=result.native_report, cases=int(passed.group(2)),
                assertions=int(passed.group(1).replace(",", "")), elapsed_seconds=round(time.monotonic() - started, 3))


def launch_policy(request):
    """Read only the immutable original source model, without its historical proof chain.

    @param request Exact current launch inputs containing the original owner fixture and policy.
    @return Original independent phase/cadence policy, retaining its accepted byte identity.
    """
    fixture = json.loads((ROOT / request["inputs"]["launch-owner-contract"]["path"]).read_text(encoding="utf-8"))
    reference = request["inputs"]["launch-policy"]
    if fixture["source_era_policy"] != reference:
        raise ValueError("current launch source policy differs from its original independent owner fixture")
    return json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))


def execute_stage(request, root):
    """Run current source selectors with a separate actual decoder prerequisite.

    @param request Registered current family/configuration stage and explicit generated inputs.
    @param root Checkout owning current source policies and measured native images.
    @return Actual source-selector checks and exact completed result identity.
    """
    recipe, native, application = checked_request(request, root)
    family = recipe["original_parameters"]["family"]
    policy = launch_policy(request) if family == "launch" else None
    if request["stage"] == "prepare":
        return dict(checks=["actual same-build decoder/source/process prerequisite verified",
                            "explicit current selector tags, independent policy and configuration verified"])
    context = RunContext(root, recipe["feature"], recipe["scenario"], request["run_id"],
                         "source-selectors", family + "-" + request["configuration"].lower()
                         + ("-" + request["prepared_attempt"] if request.get("prepared_attempt") is not None else ""))
    context.reports.mkdir(parents=True, exist_ok=True)
    context.evidence.mkdir(parents=True, exist_ok=True)
    terminal = contained(root / request["outputs"]["native-evidence"], context.evidence)
    output = contained(root / request["outputs"]["manifest"], context.reports)
    # Completed observations are immutable. Another current build needs a new
    # selected invocation rather than overwriting its earlier successful proof.
    if terminal.exists() or output.exists():
        raise ValueError("source-selector output already exists; select a fresh invocation")
    # Named repeats retain each actual observation in its original role while
    # preserving all completed earlier proof and process directories.
    for destination in (terminal, output):
        destination.parent.mkdir(parents=True, exist_ok=True)
    if family == "launch":
        from launch_fleet_runtime_checks import run
        actual = run(context, recipe, request["configuration"], application, policy, terminal)
    else:
        actual = run_native_selectors(context, recipe, request["configuration"], application, terminal)
    checked_request(request, root)
    document = dict(_documentation=__doc__, schema_version=1, kind="native_source_selector_result",
        complete=True, recipe=recipe["id"], family=family, configuration=request["configuration"],
        run_id=context.run_id, source_selectors_verified=True, decoder_prerequisite=request["inputs"]["native-loader"],
        build_result=request["build"]["result"], application=identity(application, root), source_epoch=native["source_epoch"],
        selected_inputs=request["inputs"], producer_sources=request["sources"], test_tags=recipe["test_tags"],
        prepared_attempt=request.get("prepared_attempt"),
        terminal=identity(terminal, root), paid_world_loaded=False, viewport_captured=False,
        ordinary_game_reviewed=False, public_runtime_activated=False, publishes_source=False, **actual)
    with output.open("x", encoding="utf-8") as stream:
        json.dump(document, stream, indent=2)
        stream.write("\n")
    # Ground's decoder and selector are separate images in one build. Close
    # the actual selector process against the just-written semantic result so
    # an upstream image identity can never masquerade as the executing consumer.
    checked_result(identity(output, root), family, request["configuration"], root)
    return dict(manifest=identity(output, root), checks=["all original actual source-selector cases passed",
        "independent original source-policy comparisons/refusals preserved",
        "same current decoder/image/configuration and exact process/transcript closed",
        "no paid world, viewport, human acceptance or source/runtime publication"])


def execute_prepared(family, configuration, native_reference, build_reference, attempt=None):
    """Dispatch fresh selected stages using one explicit current actual decoder result.

    @param family Explicit original source-selector family.
    @param configuration Independently built actual native configuration.
    @param native_reference Exact current actual18C decoder result identity.
    @param build_reference Exact actual current build result identity, never latest lookup.
    @param attempt Optional named fresh selector observation preserving earlier proof/runtime homes.
    @return Actual dispatcher verification result after both selected stages pass.
    """
    from artifact_recipe_stage import execute_request
    if attempt is not None:
        identifier(attempt)
    recipe = selected_recipe(family)
    native_path = contained(ROOT / native_reference["path"], ROOT / "mlm/reviews")
    if identity(native_path, ROOT) != native_reference:
        raise ValueError("selected prepared decoder result changed")
    native = json.loads(native_path.read_text(encoding="utf-8"))
    run_id = identifier(native["run_id"])
    validate_native_manifest(native, family, configuration, run_id)
    selection = load_recipe(recipe["id"], configuration, ROOT)
    if not selection["executable"]:
        raise ValueError("current source-selector recipe has pending/missing prerequisites")
    sources = snapshot(selection, ROOT)["sources"]
    context = RunContext(ROOT, recipe["feature"], recipe["scenario"], run_id)
    inputs = dict(verify_retained_inputs(recipe, ROOT), **{"native-loader": native_reference})
    last = None
    # Both requests are fresh real dispatcher stages. Only the explicit actual
    # decoder outcome supplies this family; no old selector/native flag is reused.
    for stage in ("prepare", "verify"):
        token = "prepared-" + configuration.lower() + "-" + stage + "-" + uuid.uuid4().hex
        request = dict(_documentation=__doc__, schema_version=1, kind="artifact_recipe_stage_request",
            recipe=recipe["id"], stage=stage, declaration=recipe, configuration=configuration,
            run_id=run_id, feature=recipe["feature"], scenario=recipe["scenario"], inputs=inputs,
            outputs=prepared_outputs(context, recipe, configuration, attempt),
            sources=sources, build=dict(status="passed", result=build_reference), prepared_attempt=attempt,
            result_path=(context.reports / (token + "-result.json")).relative_to(ROOT).as_posix(),
            publishes_source=False, paid_work_authorized=False)
        checked_request(request, ROOT)
        context.reports.mkdir(parents=True, exist_ok=True)
        path = context.reports / (token + ".json")
        path.write_text(json.dumps(request, indent=2) + "\n", encoding="utf-8")
        last = execute_request(path, ROOT)
    return last


def main(default_family=None):
    """Run one separately invocable bounded selector family with explicit prerequisites.

    @param default_family Optional original launch wrapper's fixed family selection.
    @return None; a failed actual process remains a nonzero command outcome.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--family", choices=("launch", "orbital", "space", "ground"), default=default_family, required=default_family is None)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--native-result", type=Path, required=True)
    parser.add_argument("--build-result", type=Path, required=True)
    parser.add_argument("--attempt", help="Named fresh selector proof preserving earlier observations")
    args = parser.parse_args()
    result = execute_prepared(args.family, args.configuration,
        identity(args.native_result.resolve(), ROOT), identity(args.build_result.resolve(), ROOT), args.attempt)
    print(json.dumps(dict(recipe=result["recipe"], complete=result["complete"], outputs=result["outputs"]), indent=2))


if __name__ == "__main__":
    main()
