"""Run the five original actual sprite loaders with explicit source/build inputs.

@file sprite_native_loader.py Bounded registered native leaves; no source publication or paid world.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups18C/19.
Prepared mode explicitly reuses only independently checked source-byte products,
never another configuration's native result or a latest/cached executable.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import time
import uuid

from artifact_paths import RunContext, contained, identifier
from artifact_rebuild import identity, load_recipe, snapshot, verify_current_build, verify_passed_stage, verify_retained_inputs
from artifact_recipes import load_catalogue
from runner import run_native, _run_owned_process
from sprite_expected_payload import ROOT, checked_payload, checked_expectation_bundle, load_facts, recipe_family
from sprite_native_inputs import bind_expected_payload, bind_candidate_baseset


def selected_recipe(family):
    """Read the actual existing family/target/tag declaration from the sole catalogue.

    @param family Original source binary protocol family, including launch-fleet.
    @return Actual native recipe declaration; disabled/unimplemented selectors refuse.
    """
    name = recipe_family(family) + "-native-loader"
    recipe = next(row for row in load_catalogue(ROOT)["recipes"] if row["id"] == name)
    # Every separately invocable native leaf owns one real original loader case,
    # one explicit configuration and a stopped bounded child before completion.
    if not recipe["enabled"] or len(recipe["test_tags"]) != 1 or not 0 < recipe["native_timeout_seconds"] <= 60:
        raise ValueError("selected native sprite leaf is pending or lacks its bounded actual case")
    return recipe


def prepare_native_context(request, root, suffix=""):
    """Create a family/configuration-specific runtime without loading any saved paid world.

    @param request Actual registered stage request with explicit source invocation/configuration.
    @param root Actual checkout owning current distribution and selected sources.
    @param suffix Optional deliberate refusal scenario/process suffix, never a source selector.
    @return Prepared context/config, actual candidate runtime identity and its separate binding manifest.
    """
    family = request["declaration"]["original_parameters"]["family"]
    configuration = request["configuration"]
    attempt = request.get("prepared_attempt")
    process_attempt = "-" + identifier(attempt) if attempt is not None else ""
    context = RunContext(root, request["feature"], request["scenario"] + suffix, request["run_id"],
                         "packed-art", family + "-" + configuration.lower() + process_attempt + suffix)
    # Every family/configuration owns separate data and actual process evidence;
    # a prior child's scripts, settings or captures cannot supply this observer.
    for directory in (context.images, context.data, context.reports, context.evidence):
        directory.mkdir(parents=True, exist_ok=True)
    config = context.prepare_runtime()
    scripts = context.runtime_dir / "scripts"
    scripts.mkdir(parents=True, exist_ok=True)
    (scripts / "game_start.scr").write_text("# @see r2fc-file-cleanup3.md18C; isolated sprite startup; no saved world or work commands.\n", encoding="utf-8")
    binding = context.prepare_candidate_baseset(request["inputs"]["candidate-grf"], request["inputs"]["candidate-checksum"], configuration)
    return context, config, identity(context.runtime_dir / "baseset/openttd.grf", root), binding


def checked_request(request, root):
    """Validate current source/build roles before any real native application is launched.

    @param request Exact selected current native prepare/verify request.
    @param root Explicit current checkout.
    @return Current family, configuration, native application and actual build result document.
    """
    if request.get("prepared_attempt") is not None:
        identifier(request["prepared_attempt"])
    # Native qualification cannot acquire paid-work/source-publication authority
    # or replace one supported independent configuration with another result label.
    if (root.resolve() != ROOT or request["stage"] not in ("prepare", "verify")
            or request["configuration"] not in ("Debug", "RelWithDebInfo")
            or request["paid_work_authorized"] is not False or request["publishes_source"] is not False):
        raise ValueError("native sprite loader requires its exact read-only current recipe scope")
    recipe = request["declaration"]
    family = recipe["original_parameters"]["family"]
    if recipe != selected_recipe(family):
        raise ValueError("native sprite recipe declaration changed")
    configuration = request["configuration"]
    verify_current_build(request["build"], configuration, recipe, root)
    build = json.loads((root / request["build"]["result"]["path"]).read_text(encoding="utf-8"))
    application = contained(root / build["targets"][recipe["target"]]["path"], root)
    if application.parent.name != configuration:
        raise ValueError("native sprite executable path disagrees with its actual configuration")
    payload = checked_payload(family, request["inputs"]["expected-manifest"],
                              RunContext(root, request["feature"], request["scenario"], request["run_id"]))
    if identity(payload, root) != request["inputs"]["expected-payload"]:
        raise ValueError("native sprite payload role disagrees with its exact typed source report")
    if request.get("prepared_expectations") is not None:
        bundle = checked_expectation_bundle(request["prepared_expectations"], RunContext(root, request["feature"], request["scenario"], request["run_id"]))
        # The source packing ledger and expectation phase stay distinct. Their
        # actual candidate/expected roles cannot be swapped or forged into nodes.
        if (bundle["families"][family]["manifest"] != request["inputs"]["expected-manifest"]
                or bundle["actual_candidate"] != request["inputs"]["candidate-grf"]
                or bundle["actual_checksum"] != request["inputs"]["candidate-checksum"]
                or bundle["source_result"] != request.get("prepared_source")):
            raise ValueError("prepared candidate/expectation phase inputs disagree")
    return family, configuration, application, build


def execute_stage(request, root):
    """Run the original actual GRF/cache loader and seal only its measured current outcome.

    @param request Registered current native stage with exact source/build/configuration inputs.
    @param root Checkout owning the original independent expectations and actual native image.
    @return Measured real native checks and exact result/process/evidence identities.
    """
    family, configuration, application, build = checked_request(request, root)
    if request["stage"] == "prepare":
        return dict(checks=["current actual native image/configuration/source epoch verified",
                            "explicit independent source payload/report identities verified"])
    context = RunContext(root, request["feature"], request["scenario"], request["run_id"])
    terminal = contained(root / request["outputs"]["native-evidence"], context.evidence)
    output = contained(root / request["outputs"]["manifest"], context.reports)
    # Exact output names identify one current observation. A duplicate command
    # cannot overwrite earlier evidence or turn its old successful flag into reuse.
    if terminal.exists() or output.exists():
        raise ValueError("native sprite output already exists; choose a fresh selected invocation")
    context, config, runtime, candidate_binding = prepare_native_context(request, root)
    # Named attempts preserve earlier proofs and own distinct process homes;
    # their nested evidence/report directories stay within the declared roles.
    for destination in (terminal, output):
        destination.parent.mkdir(parents=True, exist_ok=True)
    before = identity(root / "media/baseset/openttd.grf", root)
    environment = bind_expected_payload(context, family, configuration, request["inputs"]["expected-manifest"])
    environment = bind_candidate_baseset(context, candidate_binding, configuration, environment)
    started = time.monotonic()
    command = [str(application), request["declaration"]["test_tags"][0], "--reporter", "console", "--durations", "yes"]
    with terminal.open("x", encoding="utf-8") as sink:
        result = run_native(context, command, env=environment, cwd=context.runtime_dir,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=request["declaration"]["native_timeout_seconds"], check=False)
    text = terminal.read_text(encoding="utf-8")
    complete = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
    expected = load_facts()[family]
    prefix = family.upper().replace("-", "_") + "_PACKED_ART_LOADED"
    marker = next((line for line in text.splitlines() if line.startswith(prefix + " ")), "")
    # The original case must actually complete all source records. Successful
    # empty filters, source-only reports and partial decoder loops cannot pass.
    if (result.returncode != 0 or complete is None or not marker
            or f"sprites={expected['record_count']} " not in marker
            or f"native_bound={expected['native_bound']} " not in marker
            or not marker.endswith(("pixel_hashes=exact", "full_pixel_bytes=exact"))):
        raise ValueError("actual native sprite loader failed or was partial; see " + str(terminal))
    verify_current_build(request["build"], configuration, request["declaration"], root)
    # Publication remains immutable; the current process used exactly this
    # isolated runtime container and explicitly supplied non-save source input.
    if identity(root / "media/baseset/openttd.grf", root) != before or identity(root / runtime["path"], root) != runtime:
        raise ValueError("actual native sprite loader changed canonical/runtime payload bytes")
    process = json.loads((root / result.native_report["path"]).read_text(encoding="utf-8"))
    if process.get("input_integrity_passed") is not True:
        raise ValueError("actual native sprite process lacks closed source input provenance")
    binding_document = json.loads(candidate_binding.read_text(encoding="utf-8"))
    observed_path = contained(root / binding_document["native_observation"], context.reports)
    observed = json.loads(observed_path.read_text(encoding="utf-8"))
    # Actual native opened-file bytes must independently equal the selected
    # candidate. A copied file and an expected-pixels environment are preparation
    # only and cannot substitute for this real SpriteFile observation.
    if (observed.get("kind") != "native_candidate_grf_observation" or observed.get("native_origin_bytes_verified") is not True
            or observed.get("run_id") != context.run_id or observed.get("configuration") != configuration
            or observed.get("process") != context.process or observed.get("binding") != identity(candidate_binding, root)
            or observed.get("runtime_grf") != runtime or observed.get("opened_bytes_sha256") != request["inputs"]["candidate-grf"]["sha256"]
            or observed.get("opened_bytes") != (root / runtime["path"]).stat().st_size):
        raise ValueError("actual native opened GRF identity is missing or disagrees")
    document = dict(_documentation=__doc__, schema_version=1, kind="native_sprite_loader_result", complete=True,
        family=family, recipe=request["recipe"], configuration=configuration, run_id=context.run_id,
        application=build["targets"][request["declaration"]["target"]], build_result=request["build"]["result"],
        source_epoch=build["source_epoch"], tool_profile=build["tool_profile"],
        expected_manifest=request["inputs"]["expected-manifest"], expected_payload=request["inputs"]["expected-payload"],
        native_process=result.native_report, terminal=identity(terminal, root), runtime_payload=runtime,
        actual_candidate=request["inputs"]["candidate-grf"], candidate_checksum=request["inputs"]["candidate-checksum"],
        candidate_binding=identity(candidate_binding, root), native_candidate_observation=identity(observed_path, root),
        runtime_config=identity(config, root), tag=request["declaration"]["test_tags"][0], marker=marker,
        native_sprites_verified=expected["record_count"], native_sprite_bound=expected["native_bound"],
        assertions=int(complete.group(1).replace(",", "")), elapsed_seconds=round(time.monotonic() - started, 3),
        native_loading_verified=True, full_pixel_bytes_exact=family in ("launch-fleet", "ground"),
        paid_world_loaded=False, ordinary_game_reviewed=False, publishes_source=False,
        source_generation=request.get("prepared_source"), expectation_bundle=request.get("prepared_expectations"), prepared_attempt=request.get("prepared_attempt"))
    with output.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(document, indent=2) + "\n")
    return dict(manifest=identity(output, root), native_process=result.native_report, native_loading_verified=True,
                checks=["actual original native application/GRF/cache loader completed its one selected case",
                        "all independent source geometry/anchors/pixel expectations matched",
                        "current image/configuration/non-save input/process/transcript bound exactly",
                        "canonical/runtime payload preserved; no paid world or source publication"])


def check_candidate_result(document, root):
    """Recheck a leaf's real candidate/origin/process chain before aggregate acceptance.

    @param document Actual positive native_sprite_loader_result, never a preparation manifest.
    @param root Current owning checkout; historical nested image identities are not substituted.
    @return Exact actual candidate reference shared by the successfully checked leaf.
    """
    from sprite_native_inputs import observe_candidate_baseset, CANDIDATE_PREFIX
    required = ("actual_candidate", "candidate_checksum", "candidate_binding", "native_candidate_observation", "native_process")
    # Aggregate acceptance cannot silently admit an older leaf protocol that
    # checked expected pixels but never observed its actual opened container.
    if any(name not in document for name in required):
        raise ValueError("native sprite leaf lacks actual candidate/origin evidence")
    for name in required:
        reference = document[name]
        path = contained(root / reference["path"], root / "mlm/reviews")
        if identity(path, root) != reference:
            raise ValueError("native candidate aggregate witness changed")
    binding = json.loads((root / document["candidate_binding"]["path"]).read_text(encoding="utf-8"))
    relative = Path(binding["runtime"]).parts
    # Only the original process home can close this source-copy-native chain;
    # another process/configuration's equal file hash is insufficient.
    if len(relative) != 5 or relative[:2] != ("mlm", "runtime") or relative[3:] != (document["run_id"], binding["process"]):
        raise ValueError("native candidate runtime owner disagrees")
    recipe = selected_recipe(document["family"])
    context = RunContext(root, recipe["feature"], recipe["scenario"], document["run_id"], relative[2], binding["process"])
    environment = {CANDIDATE_PREFIX: str(root / document["candidate_binding"]["path"]),
                   CANDIDATE_PREFIX + "_SHA256": document["candidate_binding"]["sha256"],
                   "OTTD_TEST_CONFIGURATION": document["configuration"]}
    candidate = observe_candidate_baseset(context, environment, document["configuration"])
    observed = json.loads((root / document["native_candidate_observation"]["path"]).read_text(encoding="utf-8"))
    process = json.loads((root / document["native_process"]["path"]).read_text(encoding="utf-8"))
    expected = next((row for row in process.get("bound_inputs", []) if row.get("role") == "expected_sprites"), None)
    # Real process status, opening input roles and independent native stream
    # identity must agree. Rehashed metadata cannot strip the required candidate
    # or substitute a source-only preparation for the actual loaded container.
    if (candidate not in process.get("bound_inputs", []) or expected is None
            or document["actual_candidate"] != {key: candidate[key] for key in ("path", "sha256")}
            or document["candidate_checksum"] != candidate["checksum_source"]
            or expected.get("source_manifest") != document["expected_manifest"]
            or {key: expected[key] for key in ("path", "sha256")} != document["expected_payload"]
            or process.get("run_id") != document["run_id"] or process.get("configuration") != document["configuration"]
            or process.get("executable") != document["application"] or process.get("exit_status") != 0
            or process.get("root_reaped") is not True or process.get("timed_out") is not False
            or process.get("input_integrity_passed") is not True
            or document["native_candidate_observation"] not in process.get("artifacts", [])
            or document["terminal"]["sha256"] not in {row["sha256"] for row in process.get("transcripts", [])}
            or observed.get("kind") != "native_candidate_grf_observation" or observed.get("native_origin_bytes_verified") is not True
            or observed.get("schema_version") != 1
            or observed.get("binding") != document["candidate_binding"] or observed.get("runtime_grf") != candidate["runtime_grf"]
            or observed.get("configuration") != document["configuration"] or observed.get("run_id") != document["run_id"]
            or observed.get("process") != binding["process"] or observed.get("opened_bytes_sha256") != candidate["sha256"]
            or observed.get("opened_bytes") != (root / candidate["runtime_grf"]["path"]).stat().st_size):
        raise ValueError("native candidate/opened-origin/process/expected roles disagree")
    return document["actual_candidate"]


def collect_stage(request, root):
    """Join five actual current native leaves without running or inventing another decoder.

    @param request Registered read-only aggregate with explicit native result identities.
    @param root Current checkout owning all actual images and process witnesses.
    @return Measured five-family/configuration closure and its current aggregate identity.
    """
    configuration = request["configuration"]
    results, candidate = {}, None
    # Every actual family must have its own current native outcome and complete
    # source/process/transcript provenance. No source-only or historical success
    # may stand in for one missing family or another build configuration.
    for reference in request["inputs"].values():
        path = contained(root / reference["path"], root / "mlm/reviews")
        if identity(path, root) != reference:
            raise ValueError("native sprite aggregate input identity changed")
        document = json.loads(path.read_text(encoding="utf-8"))
        family = document.get("family")
        if (document.get("kind") != "native_sprite_loader_result" or document.get("complete") is not True
                or document.get("native_loading_verified") is not True or document.get("configuration") != configuration
                or document.get("run_id") != request["run_id"] or family in results):
            raise ValueError("native sprite aggregate family/configuration/status disagrees")
        recipe = selected_recipe(family)
        facts = load_facts()[family]
        # Every leaf must cover its complete independently retained original
        # protocol, not merely set a successful flag after a partial pixel loop.
        if document.get("native_sprites_verified") != facts["record_count"] or document.get("native_sprite_bound") != facts["native_bound"]:
            raise ValueError("native sprite aggregate has partial original family coverage")
        verify_current_build(dict(result=document["build_result"]), configuration, recipe, root)
        actual_candidate = check_candidate_result(document, root)
        # One published bundle must come from one actual container; individually
        # successful families from different candidate GRFs cannot be joined.
        if candidate is not None and candidate != actual_candidate:
            raise ValueError("native sprite aggregate mixed different actual candidate GRFs")
        candidate = actual_candidate
        for witness in (document["native_process"], document["terminal"], document["expected_manifest"], document["expected_payload"]):
            if identity(root / witness["path"], root) != witness:
                raise ValueError("native sprite aggregate process/source witness changed")
        results[family] = reference
    if set(results) != set(load_facts()):
        raise ValueError("native sprite aggregate requires all five actual families")
    if request["stage"] == "prepare":
        return dict(checks=["all five actual native image/configuration/source/process witnesses verified"])
    context = RunContext(root, request["feature"], request["scenario"], request["run_id"])
    path = contained(root / request["outputs"]["manifest"], context.reports)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8") as stream:
        json.dump(dict(_documentation=__doc__, schema_version=1, kind="native_sprite_loader_closure",
            complete=True, configuration=configuration, run_id=context.run_id, families=results, actual_candidate=candidate,
            native_loading_verified=True, paid_world_loaded=False, publishes_source=False), stream, indent=2)
        stream.write("\n")
    return dict(manifest=identity(path, root), checks=["all five separately executed actual native loader leaves qualified"])


def exercise_decoder_refusal(request, root):
    """Prove a real decoder rejects one deliberately incorrect expected pixel fingerprint.

    @param request Exact current native/source/build selection; never a previous native pass.
    @param root Current checkout, containing the actual independently built native application.
    @return Explicit actual decoder-refusal report, never successful native loading evidence.
    """
    family, configuration, application, build = checked_request(request, root)
    suffix = "-refusal-" + uuid.uuid4().hex[:10]
    context, _config, runtime, candidate_binding = prepare_native_context(request, root, suffix)
    canonical_before = identity(root / "media/baseset/openttd.grf", root)
    original = checked_payload(family, request["inputs"]["expected-manifest"], context)
    data = bytearray(original.read_bytes())
    header = 12 if family == "m1" else 14 if family in ("space", "orbital") else 16
    # Keep protocol, geometry and real source pixels intact. Alter only the
    # first expected FNV value so the real application must reach sprite decoding
    # and reject its actual buffer, rather than fail an input-path/header guard.
    data[header + 10] ^= 1
    payload = context.data / "decoder-fault.bin"
    payload.write_bytes(data)
    source_document = json.loads((root / request["inputs"]["expected-manifest"]["path"]).read_text(encoding="utf-8"))
    source_document.update(_documentation="Owner: r2fc-file-cleanup3.md18C; deliberate native decoder refusal fixture; never accepted source/native proof.",
                           expected_payload=identity(payload, root))
    source_path = context.reports / "decoder-fault-source.json"
    source_path.write_text(json.dumps(source_document, indent=2) + "\n", encoding="utf-8")
    environment = bind_expected_payload(context, family, configuration, request["inputs"]["expected-manifest"])
    environment.update(OTTD_TEST_EXPECTED_SPRITES=str(payload),
        OTTD_TEST_EXPECTED_SPRITES_SHA256=identity(payload, root)["sha256"],
        OTTD_TEST_EXPECTED_SPRITES_MANIFEST=str(source_path),
        OTTD_TEST_EXPECTED_SPRITES_MANIFEST_SHA256=identity(source_path, root)["sha256"])
    environment = bind_candidate_baseset(context, candidate_binding, configuration, environment)
    terminal = context.evidence / "decoder-mismatch.txt"
    command = [str(application), request["declaration"]["test_tags"][0], "--reporter", "console"]
    started = time.monotonic()
    # Deliberately bypass the normal Python golden-byte preflight for this
    # negative native-only fixture. The compiled owner still validates complete
    # coherent input identities; the real decoder must expose the wrong pixels.
    with terminal.open("x", encoding="utf-8") as sink:
        completed = _run_owned_process(command, dict(env=environment, cwd=context.runtime_dir, stdout=sink,
            stderr=subprocess.STDOUT, timeout=request["declaration"]["native_timeout_seconds"],
            creationflags=subprocess.CREATE_NO_WINDOW))
    text = terminal.read_text(encoding="utf-8")
    function = "HashGroundPixels" if family == "ground" else "HashArtPixels"
    if completed.returncode == 0 or completed.timed_out or function not in text or "failed" not in text.lower():
        raise ValueError("native refusal did not reach the actual decoded-pixel comparison; see " + str(terminal))
    verify_current_build(request["build"], configuration, request["declaration"], root)
    checked_payload(family, request["inputs"]["expected-manifest"], context)
    # A deliberate negative expectation never authorizes publication changes.
    # Preserve both the actual runtime container and its canonical shipping owner.
    if (identity(root / runtime["path"], root) != runtime
            or identity(root / "media/baseset/openttd.grf", root) != canonical_before):
        raise ValueError("native decoder refusal changed runtime/canonical payload bytes")
    document = dict(_documentation=__doc__, schema_version=1, kind="native_sprite_decoder_refusal",
        family=family, recipe=request["recipe"], configuration=configuration, run_id=context.run_id,
        application=build["targets"][request["declaration"]["target"]], build_result=request["build"]["result"],
        source_epoch=build["source_epoch"], argv=command, exit_status=completed.returncode,
        original_expected=request["inputs"]["expected-manifest"], fault_payload=identity(payload, root),
        fault_source_report=identity(source_path, root), terminal=identity(terminal, root), runtime_payload=runtime,
        candidate_binding=identity(candidate_binding, root), root_reaped=completed.root_reaped,
        elapsed_seconds=round(time.monotonic() - started, 3), decoder_mismatch_observed=True,
        native_loading_verified=False, paid_world_loaded=False, publishes_source=False)
    output = context.reports / "decoder-refusal.json"
    output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return dict(report=identity(output, root), decoder_mismatch_observed=True, native_loading_verified=False)


def prepared_outputs(context, recipe, configuration, attempt=None):
    """Select explicit native output roles without replacing an earlier attempt's proof.

    @param context Consuming family view sharing the independently generated source run.
    @param recipe Exact registered native leaf and its original output role declarations.
    @param configuration Actual independent native configuration.
    @param attempt Optional explicit attempt identifier within each original output role.
    @return Repository-relative output paths requested by this prepared stage.
    """
    if attempt is not None:
        identifier(attempt)
    outputs = {}
    # Repeated native checks keep their original reports/evidence owners.
    # Explicit names cannot overwrite completed results or borrow old processes.
    for row in recipe["outputs"]:
        relative = Path(row["path"].format(configuration=configuration))
        if attempt is not None:
            relative = Path(relative.parts[0]) / ("prepared-" + attempt) / Path(*relative.parts[1:])
        outputs[row["role"]] = contained(context.directory / relative, context.directory).relative_to(context.root).as_posix()
    return outputs


def execute_prepared(family, configuration, source_run, source_recipe, build_reference, decoder_refusal=False, attempt=None, expectations=None, selected_stage=None):
    """Run a registered native leaf against explicit already-produced source-byte prerequisites.

    @param family Original family selected by its registered native case.
    @param configuration Actual current independently built Debug or RelWithDebInfo image.
    @param source_run Exact complete supplying source invocation, never latest/cached discovery.
    @param source_recipe Exact complete root containing this family's pure expectation leaf.
    @param build_reference Current actual native_build_result identity for this configuration.
    @param decoder_refusal Run only an explicit actual native pixel-mismatch refusal fixture.
    @param attempt Explicit fresh native attempt preserving prior results and process homes.
    @param expectations Optional exact53bundle continuing a separate completed52packing ledger; never fabricated nodes.
    @param selected_stage Optional prepare/verify split for bounded invocations; verify repeats all actual input/build guards.
    @return Actual dispatcher verification result; native proof is always newly executed.
    """
    from artifact_recipe_stage import execute_request
    identifier(source_run)
    identifier(source_recipe)
    if selected_stage not in (None, "prepare", "verify"):
        raise ValueError("prepared native art requires a bounded prepare or verify stage")
    if attempt is not None:
        identifier(attempt)
    # Fault runs already own unique refusal contexts. A failed decoder must
    # never acquire the name of a successful prepared native qualification.
    if decoder_refusal and attempt is not None:
        raise ValueError("named prepared attempts are only for positive native qualification")
    recipe = selected_recipe(family)
    source_path = contained(ROOT / "mlm/reviews/rebuild" / source_recipe / source_run / "reports/rebuild.json", ROOT / "mlm/reviews")
    state = json.loads(source_path.read_text(encoding="utf-8"))
    name = recipe_family(family) + "-expected-payload"
    # Reuse only explicitly selected independently fixed source bytes. Neither
    # native outcomes nor another configuration's executable/proof are inherited.
    if (state.get("kind") != "artifact_rebuild_result" or state.get("schema_version") != 1
            or state["signature"].get("recipe") != source_recipe
            or state["complete"] is not True or state["status"] != "verified" or state["run_id"] != source_run):
        raise ValueError("prepared native loader requires explicit completed pure source-byte prerequisites")
    context = RunContext(ROOT, recipe["feature"], recipe["scenario"], source_run)
    if expectations is not None:
        bundle = checked_expectation_bundle(expectations, context)
        if bundle["source_result"] != identity(source_path, ROOT):
            raise ValueError("prepared expectation bundle has a different source supplier")
        expected_inputs = {"expected-manifest": bundle["families"][family]["manifest"],
                           "expected-payload": bundle["families"][family]["expected_payload"],
                           "candidate-grf": bundle["actual_candidate"], "candidate-checksum": bundle["actual_checksum"]}
    else:
        declared = state["signature"]["declarations"][name]
        # Old combined source graphs remain usable only with the same explicit
        # original pure expectation protocol; native outcomes cannot supply it.
        if (state["nodes"][name]["status"] != "verified" or declared["execution"] != "pure_conversion"
                or declared["configuration_required"] is not False
                or declared["configuration_reuse"] != "source_bytes_only" or declared["comparison"] != "independent_source_bytes"):
            raise ValueError("prepared native loader requires explicit completed pure source-byte prerequisites")
        verify_passed_stage(state["nodes"][name]["stages"]["verify"], ROOT)
        if "merged-art-candidates" not in state["nodes"]:
            raise ValueError("prepared native loader requires a separate actual candidate supplier; use the checked expectation bundle")
        candidate = state["nodes"]["merged-art-candidates"]
        verify_passed_stage(candidate["stages"]["verify"], ROOT)
        expected_inputs = {"expected-manifest": state["nodes"][name]["outputs"]["manifest"],
                           "expected-payload": state["nodes"][name]["outputs"]["expected-payload"],
                           "candidate-grf": candidate["outputs"]["grf"], "candidate-checksum": candidate["outputs"]["checksum"]}
    selection = load_recipe(recipe["id"], configuration, ROOT)
    if not selection["executable"]:
        raise ValueError("prepared native loader recipe is pending or has missing retained inputs")
    sources = snapshot(selection, ROOT)["sources"]
    context.reports.mkdir(parents=True, exist_ok=True)
    inputs = verify_retained_inputs(recipe, ROOT)
    inputs.update(expected_inputs)
    if decoder_refusal:
        return exercise_decoder_refusal(dict(recipe=recipe["id"], stage="verify", declaration=recipe,
            configuration=configuration, run_id=source_run, feature=recipe["feature"], scenario=recipe["scenario"],
            inputs=inputs, build=dict(status="passed", result=build_reference),
            prepared_source=identity(source_path, ROOT), prepared_expectations=expectations,
            publishes_source=False, paid_work_authorized=False), ROOT)
    last = None
    # Existing actual source results stay untouched. Each new prepare/verify
    # request uses the same shared dispatcher and current native/source policy,
    # with fresh evidence rather than resuming or rewriting any completed stage.
    for stage in (("prepare", "verify") if selected_stage is None else (selected_stage,)):
        token = "prepared-" + configuration.lower() + "-" + stage + "-" + uuid.uuid4().hex
        request = dict(_documentation=__doc__, schema_version=1, kind="artifact_recipe_stage_request",
            recipe=recipe["id"], stage=stage, declaration=recipe, configuration=configuration,
            run_id=source_run, feature=recipe["feature"], scenario=recipe["scenario"], inputs=inputs,
            outputs=prepared_outputs(context, recipe, configuration, attempt),
            sources=sources, build=dict(status="passed", result=build_reference),
            result_path=(context.reports / (token + "-result.json")).relative_to(ROOT).as_posix(),
            prepared_source=identity(source_path, ROOT), prepared_attempt=attempt,
            prepared_expectations=expectations,
            publishes_source=False, paid_work_authorized=False)
        path = context.reports / (token + ".json")
        path.write_text(json.dumps(request, indent=2) + "\n", encoding="utf-8")
        last = execute_request(path, ROOT)
    return last


def main():
    """Execute one registered bounded native leaf using explicit prepared source/build inputs.

    @return None; actual failures retain their current transcript and propagate nonzero status.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--family", choices=tuple(load_facts()), required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--source-run", required=True)
    parser.add_argument("--source-recipe", default="ground-expected-payload")
    parser.add_argument("--build-result", type=Path, required=True)
    parser.add_argument("--decoder-refusal", action="store_true", help="Observe a deliberate actual decoder mismatch; never publishes a native pass")
    parser.add_argument("--attempt", help="Explicit fresh native attempt with separate proof paths and process home")
    parser.add_argument("--expectations", type=Path, help="Exact current expectation bundle continuing the selected packing ledger")
    parser.add_argument("--stage", choices=("prepare", "verify"), help="Split source/build preparation from the actual verified child into bounded invocations")
    arguments = parser.parse_args()
    result = execute_prepared(arguments.family, arguments.configuration, arguments.source_run,
                              arguments.source_recipe, identity(arguments.build_result.resolve(), ROOT), arguments.decoder_refusal,
                              arguments.attempt, identity(arguments.expectations.resolve(), ROOT) if arguments.expectations is not None else None,
                              arguments.stage)
    print(json.dumps(result if arguments.decoder_refusal else
                     dict(recipe=result["recipe"], complete=result["complete"], outputs=result["outputs"]), indent=2))


if __name__ == "__main__":
    main()
