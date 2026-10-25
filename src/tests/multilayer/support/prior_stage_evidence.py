"""Publish compact run-selected native semantics independently of disposable diagnostics.

@file prior_stage_evidence.py Exact earlier invocation, configuration, source and save roles.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group21.
@see docs/zz-reach2/architecture/archi-historical-debug.md.
"""
import json
from pathlib import Path

from artifact_paths import RunContext, contained, identifier, resolve_input_path
from artifact_rebuild import identity, verify_current_build
from artifact_recipes import check_reference_roles, load_catalogue, validate_selection

IMPORTED_FAMILIES = ("normal-start", "lifecycle", "useful-route", "performance")


def retained_reference(reference, context):
    """Observe original bytes through the existing retained-first alias owner.

    @param reference Original sealed path/hash, optionally including its byte count.
    @param context Consumer's explicit checkout; no donor directory is searched.
    @return Physical path and a separate observation preserving the original reference.
    """
    path = resolve_input_path(reference["path"], context.root)
    observed = identity(path, context.root)
    # Relocation may change only the pathname. Original content and any sealed
    # byte count must survive; current code/images never replace dated facts.
    if observed["sha256"] != reference["sha256"] or ("bytes" in reference and path.stat().st_size != reference["bytes"]):
        raise ValueError("retained evidence alias/hash/byte count disagrees")
    return path, dict(original=dict(reference), observed=observed, role="historical_fact")


def generated_evidence_inputs(context, suppliers, configuration):
    """Verify exact current completed suppliers using the sole recipe catalogue.

    @param context Explicit selected run; earlier invocations are never inferred.
    @param suppliers Records with recipe, configuration and exact verify-stage result identity.
    @param configuration Actual independent native consumer configuration.
    @return Exact output references authorized as generated prerequisites.
    """
    from artifact_rebuild import verify_passed_stage
    catalogue = load_catalogue(context.root)
    result, seen = [], set()
    # Every supplier must independently prove its registered output role and
    # completed current run. A copied path/hash plus a recipe label is not proof.
    for supplier in suppliers:
        recipe = validate_selection(catalogue, supplier["recipe"], context.root)[supplier["recipe"]]
        path = contained(context.root / supplier["result"]["path"], context.root / "mlm/reviews")
        if identity(path, context.root) != supplier["result"]:
            raise ValueError("generated supplier result identity changed")
        document = json.loads(path.read_text(encoding="utf-8"))
        selected_configuration = configuration if recipe["configuration_required"] else None
        # Configuration-free pure sources remain distinct from a native run;
        # their explicit None binding cannot authorize an opposite native image.
        if (supplier.get("configuration") != selected_configuration or document.get("configuration") != selected_configuration
                or document.get("kind") != "artifact_recipe_stage_result" or document.get("complete") is not True
                or document.get("recipe") != recipe["id"] or document.get("stage") != "verify"
                or document.get("run_id") != context.run_id or context.run_id not in path.parts
                or recipe["id"] in seen or recipe["enabled"] is not True):
            raise ValueError("generated prerequisite recipe/run/configuration is unbound")
        seen.add(recipe["id"])
        verify_passed_stage(dict(result=supplier["result"]), context.root)
        expected = {row["role"]: (RunContext(context.root, recipe["feature"], recipe["scenario"], context.run_id).directory /
                    row["path"].format(configuration=selected_configuration)).relative_to(context.root).as_posix()
                    for row in recipe["outputs"]}
        if {role: row["path"] for role, row in document["outputs"].items()} != expected:
            raise ValueError("generated prerequisite output roles disagree with catalogue")
        request_path = contained(context.root / document["request"]["path"], context.root / "mlm/reviews")
        if identity(request_path, context.root) != document["request"]:
            raise ValueError("generated supplier request identity changed")
        request = json.loads(request_path.read_text(encoding="utf-8"))
        # A fresh wrapper must not launder stale conversion/controller bytes.
        # Recheck the original exact declaration, request and supplying sources;
        # no adapter is rerun and no old receipt is edited by this observation.
        if (request.get("kind") != "artifact_recipe_stage_request" or request.get("schema_version") != 1
                or request.get("stage") != "verify" or request.get("recipe") != recipe["id"]
                or request.get("declaration") != recipe or request.get("run_id") != context.run_id
                or request.get("configuration") != selected_configuration or request.get("outputs") != expected
                or request.get("result_path") != supplier["result"]["path"]
                or request.get("publishes_source") is not False or request.get("paid_work_authorized") is not False
                or request.get("inputs") != document.get("inputs") or request.get("sources") != document.get("sources")
                or not document.get("facts", {}).get("checks") or not request.get("sources")):
            raise ValueError("generated supplier request/declaration/source binding disagrees")
        for source in [*request["sources"], *request["inputs"].values()]:
            if identity(contained(context.root / source["path"], context.root), context.root) != source:
                raise ValueError("generated supplier current source/input changed")
        result.extend(document["outputs"].values())
    return result


def evidence_references(value, pointer=""):
    """Enumerate structured references without guessing roles from directory names.

    @param value Actual imported report metadata, not text scraped from diagnostics.
    @param pointer JSON-pointer prefix used to bind exact fields in the caller's adapter.
    @return Iterator of exact pointer/reference pairs; quoted history still needs an explicit role.
    """
    if isinstance(value, dict):
        if "path" in value and "sha256" in value:
            yield pointer, value
            return
        # A field's exact position is the adaptation contract. Recursive walking
        # discovers undeclared dependencies; it never supplies default live roles.
        for name, child in value.items():
            token = str(name).replace("~", "~0").replace("/", "~1")
            yield from evidence_references(child, pointer + "/" + token)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            yield from evidence_references(child, pointer + "/" + str(index))


def read_imported_evidence(family, reference, context, *, historical=False, bindings=(), suppliers=(),
                           configuration=None, build=None, recipe=None, native_processes=()):
    """Translate donor report references without promoting history to current acceptance.

    @param family One of normal-start, lifecycle, useful-route or performance reader scopes.
    @param reference Exact original/current report identity selected by the caller.
    @param context Existing RunContext; no alternate catalogue or output owner is created.
    @param historical Explicit read-only historical inspection; never a native acceptance claim.
    @param bindings Exact JSON-pointer-to-role declarations for every reference in a current report.
    @param suppliers Explicit completed recipe/run/configuration witnesses for generated inputs.
    @param configuration Current independent Debug/RelWithDebInfo execution, absent for history.
    @param build Current build stage containing its exact native_build_result reference.
    @param recipe Actual registered native consumer declaration, required for current evidence.
    @param native_processes Required actual command reports produced in this invocation.
    @return Original metadata and separate checked role observations; domain acceptance stays with its public reader.
    """
    if family not in IMPORTED_FAMILIES:
        raise ValueError("unsupported imported evidence reader scope")
    if historical:
        # Historical inspection has no current build or generated suppliers.
        # Keeping this branch disjoint prevents an old passed receipt becoming
        # current by supplying a new executable beside its dated observations.
        if bindings or suppliers or build is not None or recipe is not None or native_processes or configuration is not None:
            raise ValueError("historical evidence cannot carry current acceptance bindings")
        path, relocation = retained_reference(reference, context)
        return dict(family=family, classification="historical_relocation", report=json.loads(path.read_text(encoding="utf-8")),
                    relocation=relocation, transport_verified=False, gameplay_acceptance=False)
    if configuration not in ("Debug", "RelWithDebInfo") or build is None or recipe is None or not native_processes:
        raise ValueError("current evidence requires explicit build/configuration and native command reports")
    registered = validate_selection(load_catalogue(context.root), recipe["id"], context.root)[recipe["id"]]
    if recipe != registered:
        raise ValueError("current reader recipe differs from the sole catalogue")
    verify_current_build(build, configuration, registered, context.root)
    generated = generated_evidence_inputs(context, suppliers, configuration)
    check_reference_roles(dict(role="generated_prerequisite", **reference), context.root, generated, context.run_id)
    path = contained(context.root / reference["path"], context.root / "mlm/reviews")
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("run_id") != context.run_id or document.get("configuration") != configuration:
        raise ValueError("current report run/configuration disagrees")
    declared = {row["pointer"]: row["role"] for row in bindings}
    actual = dict(evidence_references(document))
    if len(declared) != len(bindings) or declared.keys() != actual.keys():
        raise ValueError("current report has undeclared or duplicate reference bindings")
    observations = []
    # Every live dependency receives its own role. Only explicitly historical
    # fields may quote vanished code/images; required transcripts are generated
    # prerequisites, never optional diagnostics merely because they end in .log.
    for pointer, row in actual.items():
        role = declared[pointer]
        if role not in {"historical_fact", "retained_input", "generated_prerequisite", "current_source", "current_executable", "current_output", "diagnostic"}:
            raise ValueError("unknown imported reference role")
        check_reference_roles(dict(row, role=role), context.root, generated, context.run_id)
        observations.append(dict(pointer=pointer, role=role, original=row))
    images = json.loads((context.root / build["result"]["path"]).read_text(encoding="utf-8"))["targets"].values()
    commands = []
    # A real successful command in this exact run/configuration/image is the
    # process boundary; transported metadata alone never proves native work.
    for process in native_processes:
        check_reference_roles(dict(process, role="current_output"), context.root, generated, context.run_id)
        native = json.loads((context.root / process["path"]).read_text(encoding="utf-8"))
        if (native.get("run_id") != context.run_id or native.get("configuration") != configuration
                or native.get("exit_status") != 0 or native.get("timed_out") is not False
                or native.get("root_reaped") is not True or native.get("input_integrity_passed") is not True
                or native.get("executable") not in images or not native.get("transcripts")):
            raise ValueError("current command image/run/configuration/status is unbound")
        for transcript in native["transcripts"]:
            check_reference_roles(dict(transcript, role="current_output"), context.root, run_id=context.run_id)
        commands.append(process)
    return dict(family=family, classification="current_role_observation", report=document, observations=observations,
                native_processes=commands, build=build, transport_verified=True, gameplay_acceptance=False)


def publish_relocation(context, name, family, reference):
    """Write a separate historical relocation observation while preserving the sealed receipt.

    @param context Explicit generated report owner.
    @param name Fresh output basename inside this RunContext's reports.
    @param family Explicit normal-start/lifecycle/useful-route/performance reader scope.
    @param reference Original sealed report identity, possibly using a retained alias.
    @return New observation identity; no original receipt is rewritten or certified as current.
    """
    observation = read_imported_evidence(family, reference, context, historical=True)
    document = dict(_documentation=__doc__, schema_version=1, kind="historical_relocation_observation",
                    run_id=context.run_id, **{key: value for key, value in observation.items() if key != "report"})
    target = context.output("reports", name)
    with target.open("x", encoding="utf-8") as sink:
        sink.write(json.dumps(document, indent=2) + "\n")
    return identity(target, context.root)


def stage_path(context, name):
    """Select one semantic report without creating output directories or probing logs.

    @param context Explicit owning feature, scenario and invocation.
    @param name Original exact diagnostic basename identifying the producer case.
    @return Run-owned compact semantic report path.
    """
    # Basenames preserve the producer's actual case identity; path syntax or a
    # foreign suffix could otherwise cross another case's continuation boundary.
    if Path(name).name != name or not name.endswith(".log"):
        raise ValueError("prior-stage evidence requires an exact .log case basename")
    return contained(context.reports / ("prior-" + name[:-4] + "-semantics.json"), context.reports)


def publish_stage(context, name, lines, configuration, application, sources, inputs, outputs, accepted=True):
    """Seal actual required observations after a successful bounded native producer.

    @param context Actual producer's explicit feature/scenario/run context.
    @param name Original diagnostic basename identifying this exact completed case.
    @param lines Required observations parsed from the actual just-completed process.
    @param configuration Exact independently executed native configuration.
    @param application Actual native executable identity; retained only as a historical fact.
    @param sources Relevant producer/source paths whose current bytes were executed.
    @param inputs Exact immutable source save paths used by this process.
    @param outputs Basename-to-save-path mapping; None explicitly records a refused output.
    @param accepted Whether this case accepted its input; refusal can still be a completed test.
    @return Published compact report identity, with no executable or diagnostic copy.
    """
    # Empty observations or an unknown configuration cannot establish an earlier
    # accepted boundary for a later partial run, even if a namesake save exists.
    if not lines or configuration not in ("Debug", "RelWithDebInfo"):
        raise ValueError("prior-stage publication needs actual observations and configuration")
    result = dict(_documentation=__doc__, schema_version=1, kind="native_prior_stage_semantics", complete=True,
        feature=context.feature, scenario=context.scenario, run_id=context.run_id, name=name,
        configuration=configuration, accepted=accepted, lines=list(lines),
        historical_application=identity(application, context.root),
        source_epoch=[identity(path, context.root) for path in sources],
        inputs=[identity(path, context.root) for path in inputs], outputs={})
    # Refused saves have an explicit absent role. Existing files cannot be
    # silently treated as successful output of an accepted or refused case.
    for basename, path in outputs.items():
        if Path(basename).name != basename:
            raise ValueError("prior-stage output role must be an exact save basename")
        result["outputs"][basename] = identity(path, context.root) if path is not None else None
    target = stage_path(context, name)
    with target.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(result, indent=2) + "\n")
    return identity(target, context.root)


def load_stage(context, run_id, name, configuration):
    """Read one explicitly selected completed stage without falling back to diagnostic logs.

    @param context Consumer's exact feature/scenario and checkout.
    @param run_id Explicit prior invocation, never the latest run or a filename-derived guess.
    @param name Original exact producer diagnostic basename.
    @param configuration Required independent native configuration.
    @return Checked semantic report; prior image/source hashes remain historical facts.
    """
    previous = RunContext(context.root, context.feature, context.scenario, identifier(run_id))
    target = stage_path(previous, name)
    if not target.is_file():
        raise ValueError("selected prior-stage semantics missing; prepare its explicit registered prerequisite: " + str(target))
    document = json.loads(target.read_text(encoding="utf-8"))
    # Exact ownership and configuration prevent one accepted historical case
    # from being reused under a different invocation, feature or native image.
    if (document.get("kind") != "native_prior_stage_semantics" or document.get("complete") is not True
            or document.get("schema_version") != 1 or document.get("feature") != context.feature
            or document.get("scenario") != context.scenario or document.get("run_id") != run_id
            or document.get("name") != name or document.get("configuration") != configuration
            or not document.get("lines") or not document.get("source_epoch")):
        raise ValueError("selected prior-stage run/configuration/source/semantic scope disagrees")
    # Save owners can move to durable aliases after export. Their original
    # identities stay exact, while the old executable is never dereferenced.
    for reference in [*document["inputs"], *(row for row in document["outputs"].values() if row is not None)]:
        path = resolve_input_path(reference["path"], context.root)
        if identity(path, context.root)["sha256"] != reference["sha256"]:
            raise ValueError("selected prior-stage saved input/output identity changed")
    # An explicit refusal remains a no-output boundary even if a later writer
    # creates a same-named save after the semantic report was published.
    for basename, reference in document["outputs"].items():
        if reference is None and contained(previous.saves / basename, previous.saves).exists():
            raise ValueError("selected prior refused case unexpectedly has a saved output")
    return document


def semantic_text(context, run_id, name, configuration):
    """Return required prior native observations independently of diagnostic-file retention.

    @param context Exact consumer feature/scenario/check-out selection.
    @param run_id Explicit earlier invocation.
    @param name Original diagnostic basename selecting its compact semantic report.
    @param configuration Exact required native configuration.
    @return Only actual retained observation lines, with no metadata substring matches.
    """
    return "\n".join(load_stage(context, run_id, name, configuration)["lines"])


def prior_save(context, run_id, name, basename, configuration):
    """Resolve an exact earlier output role, including explicitly refused save absence.

    @param context Consumer's exact feature/scenario and checkout.
    @param run_id Explicit prior invocation.
    @param name Original producer diagnostic basename selecting its semantic report.
    @param basename Required saved-output basename, never inferred by a directory scan.
    @param configuration Exact independently executed native configuration.
    @return Hash-verified retained-first save path, or None for an explicit refused output.
    """
    document = load_stage(context, run_id, name, configuration)
    if basename not in document["outputs"]:
        raise ValueError("selected prior-stage lacks the requested saved-output role")
    reference = document["outputs"][basename]
    return resolve_input_path(reference["path"], context.root) if reference is not None else None


def execute_stage(request, root):
    """Prepare one real metadata reload from an older-format save through the public recipe graph.

    @param request Exact state-prior-native declaration, configuration and retained input identities.
    @param root Selected checkout; no old-workstation application or log is consulted.
    @return Actual bounded reload checks and compact semantic output identity.
    """
    verify_current_build(request["build"], request["configuration"], request["declaration"], root)
    source = resolve_input_path(request["inputs"]["original-metadata-save"]["path"], root)
    if identity(source, root) != request["inputs"]["original-metadata-save"]:
        raise ValueError("selected metadata prerequisite input changed")
    if request["stage"] == "prepare":
        return dict(checks=["retained older-format metadata input verified; no replacement save generated"])
    import sys
    sys.path.insert(0, str(root / "src/tests"))
    from multilayer_state_fixture import run_case
    context = RunContext(root, request["feature"], request["scenario"], request["run_id"])
    context.prepare_runtime("grftest.cfg")
    output = run_case("native-reload", ["-g", source.as_posix()],
        "MLVS_METADATA schema=1 profile=1 version=1 legacy=false editor=false",
        context=context, producer=Path(__file__), configuration=request["configuration"])
    semantic = stage_path(context, "r2mlvs14-b-native-reload.log")
    document = dict(_documentation=__doc__, kind="prepared_native_prior_stage", complete=True,
        run_id=context.run_id, configuration=request["configuration"], source=request["inputs"]["original-metadata-save"],
        saved=identity(output, root), semantics=identity(semantic, root), original_format_preserved=True)
    target = contained(root / request["outputs"]["manifest"], context.reports)
    with target.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(document, indent=2) + "\n")
    return dict(checks=["actual selected native metadata reload passed", "compact semantics and exact input/output roles published"],
                manifest=identity(target, root))
