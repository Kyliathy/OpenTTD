"""Publish compact run-selected native semantics independently of disposable diagnostics.

@file prior_stage_evidence.py Exact earlier invocation, configuration, source and save roles.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group21.
@see docs/zz-reach2/architecture/archi-historical-debug.md.
"""
import json
from pathlib import Path

from artifact_paths import RunContext, contained, identifier, resolve_input_path
from artifact_rebuild import identity, verify_current_build


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
