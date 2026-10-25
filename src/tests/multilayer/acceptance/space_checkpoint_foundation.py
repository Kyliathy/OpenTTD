"""Qualify original checkpoints and verify compact completed execution witnesses.

@file space_checkpoint_foundation.py Current checks and historical qualification have explicit roles.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group22, durable compact continuation export.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups12/12B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md.
Publication checks actual current EXEs/source. Later verification rechecks copied
receipts, terminals and small native saves; old image/source identities nested
inside those witnesses are historical facts, never prerequisites for another build.
"""

import json
from pathlib import Path
import re
from types import SimpleNamespace

from space_checkpoint import (ROOT, CONFIGURATIONS, POLICY, SCOPE, FoundationKind, SpaceCheckpoint,
                              SpaceFoundation, _publish_bytes, load_checkpoint, verify_checkpoint)
from space_checkpoint_load import (SOURCE_FILES, TAG, PHASE, compare_loaded_saves, compare_revision,
                                   terminal_fields, identity, checked, verify_load_result, selected_checkpoint,
                                   checkpoint_observation)
from artifact_inventory import checked_path, file_identity
from artifact_paths import RunContext, identifier
from savegame_tables import saved_owner_table as table
from space_preview_checks import chunks
from surface_constructor_retirement_ledger import observe

KIND = "qualified_space_foundation"
QUALIFICATION_SOURCES = (*SOURCE_FILES, "src/tests/multilayer/acceptance/space_checkpoint_foundation.py",
                         "src/tests/multilayer/acceptance/space_checkpoint_static.py")
STATIC_SOURCE_FILES = ("src/tests/multilayer/acceptance/space_checkpoint_static.py",
                      "src/tests/multilayer/acceptance/space_checkpoint_foundation.py",
                      "src/tests/multilayer/support/runner.py", "src/tests/multilayer/support/artifact_paths.py")
FIELDS = {"_documentation", "schema_version", "kind", "complete", "foundation_type", "seal",
          "configurations", "closed_scope", "financial_policy", "next_task", "authorization_granted", "path_policy"}
WITNESSES = {"native_result", "native_process", "native_terminal", "native_before", "native_output",
             "ordinary_result", "ordinary_process", "ordinary_terminal", "ordinary_output", "build_result", "source_epoch", "tool_profile"}
STATIC_FIELDS = {"_documentation", "schema_version", "kind", "complete", "configuration", "phase", "seal", "source",
                 "application", "console_application", "output", "terminal", "native_process", "native_load", "run_id",
                 "source_files", "gameplay_ticks_executed", "projection"}
BUILD_FIELDS = {"_documentation", "schema_version", "kind", "complete", "configuration", "build_directory",
                "targets", "source_epoch", "tool_profile", "tools", "options", "steps"}


def read(reference, root):
    """Read one explicit identity-bound physical JSON witness.

    @param reference Exact path/SHA256 object in the selected checkout.
    @param root Explicit source checkout.
    @return Decoded document after full-byte and containment verification.
    """
    return json.loads(checked(reference, root).read_text(encoding="utf-8"))


def validate_build_document(document, epoch, profile, configuration, root, current,
                            required_targets=("openttd", "openttd_surface_capital_test")):
    """Validate the Group13build contract, optionally checking its current physical inputs.

    @param document Exact native_build_result document.
    @param epoch Identity-verified native_source_epoch document.
    @param profile Identity-verified portable_native_build_profile document.
    @param configuration Required independent native configuration.
    @param root Checkout owning current files when current=True.
    @param current True during publication; False for copied completed-history facts.
    @param required_targets Actual targets needed by this qualification or new operation.
    @return None; failed steps, absent owners or mismatched images cannot qualify.
    """
    # A build must have executed successfully under an explicit configuration;
    # neither a portable profile alone nor yesterday's cached EXE is a build result.
    if set(document) not in (BUILD_FIELDS, BUILD_FIELDS | {"workers"}) or document["kind"] != "native_build_result" or type(document["schema_version"]) is not int or document["schema_version"] != 1 or document["complete"] is not True or document["configuration"] != configuration:
        raise ValueError("invalid current native build result")
    # Historical sealed builds predate the explicit worker observation. Fresh
    # merged publication must carry the one/two-worker pressure contract; old
    # receipts keep their exact original fields and never become current proof.
    if (current and "workers" not in document) or ("workers" in document and (type(document["workers"]) is not int or document["workers"] not in (1, 2))):
        raise ValueError("current native build needs its bounded actual compiler worker count")
    if not set(required_targets) <= document["targets"].keys():
        raise ValueError("checkpoint build lacks a required selected target")
    if not document["steps"] or any(step["exit_status"] != 0 for step in document["steps"]):
        raise ValueError("checkpoint build contains an incomplete/failed step")
    # Portable build intent and the actual source epoch have distinct roles;
    # neither a machine CMakeCache nor an unsupported metadata version substitutes.
    if epoch["kind"] != "native_source_epoch" or type(epoch["schema_version"]) is not int or epoch["schema_version"] != 1 or profile["kind"] != "portable_native_build_profile" or type(profile["schema_version"]) is not int or profile["schema_version"] != 1:
        raise ValueError("invalid native source epoch/tool profile")
    if not set(QUALIFICATION_SOURCES) <= {reference["path"] for reference in epoch["files"]}:
        raise ValueError("build epoch lacks actual qualification source owners")
    if current:
        # Every actual build/source identity must still match at publication.
        # Completed-history verification later keeps these as quoted facts and
        # must not demand an old EXE after another legitimate compilation.
        for reference in [*document["targets"].values(), *epoch["files"]]:
            checked(reference, root)


def compare_static_saves(source, output, expected, native):
    """Verify the ordinary paused loader with explicit clock/pause/revision rules.

    @param source Immutable original selected by the seal.
    @param output Actual ordinary console resave, without game-loop advancement.
    @param expected Independent retained original owner/material observation.
    @param native Actual compiled metadata from the same build's dedicated native load.
    @return Checked ordinary owner/clock/family facts.
    """
    first, last = table(source, b"DATE"), table(output, b"DATE")
    # Each complete world has one saved clock owner. Missing/extra fields could
    # otherwise conceal time or RNG changes behind a partial observation.
    if set(first) != {0} or set(last) != {0} or first[0].keys() != last[0].keys():
        raise ValueError("ordinary checkpoint DATE schema changed")
    # All actual date/tick/RNG/scheduler fields stay exact. Only the existing
    # loader's due LinkGraph worker pause may augment Normal with bit6.
    for key, value in first[0].items():
        if key != "pause_mode" and last[0][key] != value:
            raise ValueError(f"ordinary checkpoint clock/state changed: {key}")
    if first[0]["pause_mode"] != 1 or last[0]["pause_mode"] not in (1, 65):
        raise ValueError("ordinary checkpoint changed unsupported pause flags")
    if last[0]["pause_mode"] == 65:
        jobs, schedule = table(output, b"LGRJ"), table(output, b"LGRS")[0]
        if not any(jobs[reference - 1]["join_date"] <= last[0]["economy_date"] for reference in schedule["running"]):
            raise ValueError("ordinary worker pause lacks an actual due job")
    version, original = chunks(source)
    restored_version, restored = chunks(output)
    # Static recovery preserves this exact save-format/owner closure. A partial
    # or migrated world needs another contract rather than a successful cash check.
    # Current merged saves keep all128declared families. Historical sealed440
    # observations retain their original121-family contract unchanged.
    if version not in (440, 441) or restored_version != version or len(original) != (128 if version == 441 else 121) or original.keys() != restored.keys():
        raise ValueError("ordinary checkpoint changed version/family closure")
    if version == 441:
        from savegame_tables import merged_world_chunks
        merged_world_chunks(source)
        merged_world_chunks(output)
    # Metadata is checked separately above/below; every other physical family
    # remains byte-exact, including native stock, body, packet and terrain owners.
    for name, raw in original.items():
        if name not in (b"DATE", b"GLOG") and restored[name] != raw:
            raise ValueError(f"ordinary checkpoint changed family: {name!r}")
    appended = compare_revision(source, output, native)
    actual = checkpoint_observation(output)
    if actual != expected:
        raise ValueError("ordinary loaded owners disagree with independent checkpoint")
    return dict(chunk_families=len(original), clocks=dict(source=first[0], output=last[0]),
                appended_load_revision=appended, observed=actual, gameplay_ticks_executed=0)


def verify_console_projection(application, executable, runtime, root):
    """Check the actual ordinary image/projection and its matching runtime DLLs.

    @param application Current ordinary GUI build image.
    @param executable Actual launched console projection, or the same POSIX image.
    @param runtime Explicit current process data home.
    @param root Checkout containing both actual current images and dependencies.
    @return Completed projection/DLL identity facts; only the existing allowance applies.
    """
    if executable == application:
        return dict(method="same_image", application=identity(application, root), executable=identity(executable, root), dlls={})
    if executable.parent != runtime:
        raise ValueError("ordinary console image belongs to another runtime")
    gui, console = bytearray(application.read_bytes()), executable.read_bytes()
    if len(gui) < 64:
        raise ValueError("ordinary application has no complete PE header")
    offset = int.from_bytes(gui[60:64], "little")
    # Projection changes exactly the two-byte PE subsystem, with all remaining
    # image bytes unchanged. An arbitrary console app cannot qualify the GUI build.
    if offset + 94 > len(gui) or gui[offset:offset + 4] != b"PE\0\0" or int.from_bytes(gui[offset + 92:offset + 94], "little") != 2:
        raise ValueError("ordinary application has no GUI PE subsystem")
    gui[offset + 92:offset + 94] = (3).to_bytes(2, "little")
    if bytes(gui) != console:
        raise ValueError("ordinary console differs beyond the permitted PE projection")
    # Required local DLLs must match the selected ordinary build. System DLLs
    # are not copied from the workstation into a transferable proof archive.
    expected = {path.name: identity(path, root)["sha256"] for path in application.parent.glob("*.dll")}
    actual = {path.name: identity(path, root)["sha256"] for path in runtime.glob("*.dll")}
    if actual != expected:
        raise ValueError("ordinary console DLLs differ from selected build")
    return dict(method="pe_subsystem_2_to_3", application=identity(application, root), executable=identity(executable, root), dlls=actual)


def verify_static_result(document, checkpoint, native_reference):
    """Validate actual current ordinary process evidence against its independent original.

    @param document Versioned ordinary_space_checkpoint_restore receipt.
    @param checkpoint Typed original seal.
    @param native_reference Actual same-configuration native load receipt identity.
    @return Checked ordinary semantic result, without advancing or importing old chains.
    """
    root = checkpoint.root
    # Static loading is a distinct no-work protocol; its schema cannot accept a
    # legacy advance/replay receipt merely because its process exited successfully.
    if set(document) != STATIC_FIELDS or type(document["schema_version"]) is not int or document["schema_version"] != 1 or document["kind"] != "ordinary_space_checkpoint_restore" or document["complete"] is not True or document["phase"] != "ordinary-static" or type(document["gameplay_ticks_executed"]) is not int or document["gameplay_ticks_executed"] != 0:
        raise ValueError("invalid ordinary static checkpoint receipt")
    configuration = document["configuration"]
    selected = selected_checkpoint(checkpoint, configuration)
    native_result = read(native_reference, root)
    source_reference = native_result["output"] if isinstance(checkpoint, SpaceFoundation) else selected["save"]
    # All three identities select the same original/configuration and already
    # checked native build; another successful restore cannot substitute for it.
    if document["seal"] != dict(path=checkpoint.manifest_path.relative_to(root).as_posix(), sha256=checkpoint.sha256) or document["source"] != source_reference or document["native_load"] != native_reference:
        raise ValueError("ordinary restore selected a different seal/source/native build")
    verify_load_result(native_result, root)
    if native_result["seal"] != document["seal"]:
        raise ValueError("ordinary/native selected foundations differ")
    if native_result["configuration"] != configuration:
        raise ValueError("ordinary/native configuration mismatch")
    native = terminal_fields(checked(native_result["terminal"], root).read_text(encoding="utf-8", errors="replace"))
    context = RunContext(root, "space", "checkpoint-static", identifier(document["run_id"]))
    source, application, executable, output, terminal, process_path = [checked(document[field], root)
        for field in ("source", "application", "console_application", "output", "terminal", "native_process")]
    # Ordinary loading must use this configuration's complete game, rather than
    # a focused harness or an unrelated successful executable.
    if application.parent.name != configuration or application.name not in ("openttd.exe", "openttd"):
        raise ValueError("ordinary image/configuration mismatch")
    projection = verify_console_projection(application, executable, context.runtime_dir, root)
    if document["projection"] != projection:
        raise ValueError("ordinary projection facts disagree with current images")
    # File presence in another invocation is not this child's output evidence.
    if output != context.saves / "ordinary.sav" or terminal.parent != context.evidence or process_path.parent != context.reports:
        raise ValueError("ordinary outputs belong to another invocation")
    process = read(document["native_process"], root)
    expected_argv = [str(executable), "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-d", "sl=3",
                     "-c", str(context.config_path), "-g", str(source)]
    # Exact argv excludes old3,000tick replay and unpause modes. Bind it to the
    # actual selected input/image and this process's required saved/transcript outputs.
    if process["exit_status"] != 0 or process["argv"] != expected_argv or process["inputs"] != [document["source"]] or process["executable"] != document["console_application"] or process["configuration"] != configuration or document["output"] not in process["artifacts"] or document["terminal"] not in process["transcripts"]:
        raise ValueError("ordinary static process inputs/image/output disagree")
    text = terminal.read_text(encoding="utf-8", errors="replace")
    if re.search(r"(?:save failed|load failed|error loading|assertion failed)", text, re.I):
        raise ValueError("ordinary static transcript reports failure")
    if {reference["path"] for reference in document["source_files"]} != set(STATIC_SOURCE_FILES):
        raise ValueError("ordinary restore lacks its explicit source owner identities")
    for reference in document["source_files"]:
        checked(reference, root)
    expected = selected["expected"]
    facts = compare_static_saves(source, output, expected, native)
    facts["projection"] = projection
    # Closing hashes reject a build/source/output writer changing any authority
    # after the static saved-state checks but before qualification publication.
    for reference in [document[field] for field in ("source", "application", "console_application", "output", "terminal", "native_process")] + document["source_files"]:
        checked(reference, root)
    return facts


def qualify_selected_foundation(foundation, native_results, ordinary_results, build_results, destination):
    """Bind the selected successor's paused migration to both current merged builds.

    @param foundation Typed exact continuation, never an inferred latest save.
    @param native_results Configuration-to-actual-native-load receipt identities.
    @param ordinary_results Configuration-to-actual-ordinary441restore identities.
    @param build_results Configuration-to-current selected build identities.
    @param destination New run-owned qualification JSON, outside original fixtures.
    @return Exact published qualification identity, granting no gameplay authority.
    """
    if not isinstance(foundation, SpaceFoundation):
        raise TypeError("selected qualification requires a typed continuation")
    original = verify_foundation(foundation)
    root = foundation.root
    target, context = _destination(destination, root)
    if any(set(rows) != set(CONFIGURATIONS) for rows in (native_results, ordinary_results, build_results)):
        raise ValueError("selected qualification requires both current configurations")
    rows = {}
    # Each configuration must use its actual newly built callback and ordinary
    # image. One successful world cannot stand in for the independently linked pair.
    for configuration in CONFIGURATIONS:
        native = read(native_results[configuration], root)
        native_facts = verify_load_result(native, root)
        ordinary = read(ordinary_results[configuration], root)
        ordinary_facts = verify_static_result(ordinary, foundation, native_results[configuration])
        build = read(build_results[configuration], root)
        validate_build_document(build, read(build["source_epoch"], root), read(build["tool_profile"], root), configuration, root, True)
        if native["application"] != build["targets"]["openttd_surface_capital_test"] or ordinary["application"] != build["targets"]["openttd"]:
            raise ValueError("selected qualification used different current images")
        rows[configuration] = dict(native_result=native_results[configuration], ordinary_result=ordinary_results[configuration],
            build_result=build_results[configuration], source=native["source"], output=ordinary["output"],
            native_facts=native_facts, ordinary_facts=ordinary_facts)
    document = dict(_documentation=__doc__, schema_version=1, kind="merged_space_foundation_qualification",
        complete=True, foundation=identity(foundation.manifest_path, root), configurations=rows,
        next_task=original["next_task"], closed_scope=original["closed_scope"], financial_policy=original["financial_policy"],
        gameplay_ticks_executed=0, authorization_granted=False, completes_next_task=False)
    # Rechecking the selected original protects its seal/dispatch from incidental
    # changes during observation. Publication creates only this separate report.
    verify_foundation(foundation)
    return _publish_bytes(target, (json.dumps(document, indent=2) + "\n").encode(), root)


def _shape(document):
    """Validate the exact completed-qualification role and closed original policies.

    @param document Candidate or published foundation.
    @return None; operation/legacy receipt shapes cannot masquerade as this role.
    """
    # Explicit role/schema admission prevents complete=true legacy receipts from
    # entering the active continuation foundation protocol.
    if not isinstance(document, dict) or set(document) != FIELDS:
        raise ValueError("qualified foundation requires its exact typed fields")
    # A completed qualification grants no future operation rights and cannot
    # reopen the closed cash scope or its already consumed lower-floor decisions.
    if type(document["schema_version"]) is not int or document["schema_version"] != 1 or document["kind"] != KIND or document["complete"] is not True or document["foundation_type"] != FoundationKind.SEALED_CHECKPOINT.value or document["closed_scope"] != SCOPE or document["financial_policy"] != POLICY or document["authorization_granted"] is not False:
        raise ValueError("invalid qualified foundation role/scope/policy")
    if set(document["configurations"]) != set(CONFIGURATIONS):
        raise ValueError("qualified foundation requires both independent configurations")
    # Preserve boolean/int policy types across JSON consumers. Numeric flags
    # must not impersonate closed decisions when another language reads the seal.
    for field, expected in (("closed_scope", SCOPE), ("financial_policy", POLICY)):
        if any(type(document[field][key]) is not type(value) for key, value in expected.items()):
            raise ValueError("qualified foundation policy field types changed")


def _destination(path, root, allow_retained=False):
    """Resolve an explicit current Space review report and its run-local witness owner.

    @param path Caller-selected foundation report destination.
    @param root Source checkout.
    @param allow_retained Admit the exact durable exported owner for read-only loading/verification.
    @return Contained report path and pure RunContext; no reservation or write occurs.
    """
    selected = checked_path(path, root)
    retained = checked_path("src/tests/fixtures/checkpoints/space/continuations", root)
    # Durable continuation reads admit only content-addressed complete manifests;
    # ordinary native publication still belongs to a fresh generated review run.
    if allow_retained and selected.is_relative_to(retained):
        relative = selected.relative_to(retained).parts
        if (len(relative) != 3 or re.fullmatch("[0-9a-f]{64}", relative[0]) is None
                or relative[1:] != ("reports", "foundation.json")):
            raise ValueError("retained foundation requires its exact content-addressed owner")
        home = selected.parent.parent
        return selected, SimpleNamespace(directory=home, reports=home / "reports", data=home / "data", evidence=home / "evidence")
    relative = selected.relative_to(checked_path("mlm/reviews", root)).parts
    # A published result belongs to one current review run. It must never turn
    # a fixture, log file or partially written pending file into a foundation.
    if len(relative) != 5 or relative[0] != "space" or relative[3] != "reports" or selected.suffix != ".json":
        raise ValueError("foundation belongs in one explicit Space review reports owner")
    return selected, RunContext(root, "space", identifier(relative[1]), identifier(relative[2]))


def qualify_checkpoint(checkpoint, native_results, ordinary_results, build_results, destination):
    """Check actual current results, then seal compact completed qualification witnesses.

    @param checkpoint Typed immutable original source contract.
    @param native_results Both configurations' current dedicated native-load references.
    @param ordinary_results Both configurations' current ordinary-static references.
    @param build_results Both configurations' current actual build result references.
    @param destination Explicit current Space review report path.
    @return Typed completed SpaceFoundation; historical images are never copied.
    """
    if not isinstance(checkpoint, SpaceCheckpoint):
        raise TypeError("qualification requires the typed immutable checkpoint")
    verify_checkpoint(checkpoint)
    root = checkpoint.root
    target, context = _destination(destination, root)
    if any(set(references) != set(CONFIGURATIONS) for references in (native_results, ordinary_results, build_results)):
        raise ValueError("qualification needs all three actual result sets in both configurations")
    seal = json.loads(checkpoint.manifest_path.read_text(encoding="utf-8"))
    candidates = {}
    # Validate all current inputs before creating any qualification witness. One
    # failed configuration cannot leave a half-qualified consumer-selectable result.
    for configuration in CONFIGURATIONS:
        native = read(native_results[configuration], root)
        native_facts = verify_load_result(native, root)
        ordinary = read(ordinary_results[configuration], root)
        ordinary_facts = verify_static_result(ordinary, checkpoint, native_results[configuration])
        build = read(build_results[configuration], root)
        epoch, profile = read(build["source_epoch"], root), read(build["tool_profile"], root)
        validate_build_document(build, epoch, profile, configuration, root, True)
        # Only the actual selected build's images can supply this qualification;
        # successful semantic output from another image has a different meaning.
        if native["application"] != build["targets"]["openttd_surface_capital_test"] or ordinary["application"] != build["targets"]["openttd"]:
            raise ValueError("qualification process did not run the actual selected build images")
        candidates[configuration] = (native, ordinary, build, native_facts, ordinary_facts)
    rows = {}
    # Only small required proof witnesses are copied. Nested original paths in
    # these immutable receipts/epochs remain facts; no EXE/source archive is created.
    for configuration, (native, ordinary, build, native_facts, ordinary_facts) in candidates.items():
        references = dict(native_result=native_results[configuration], native_process=native["native_process"], native_terminal=native["terminal"],
            native_before=native["before"], native_output=native["output"], ordinary_result=ordinary_results[configuration],
            ordinary_process=ordinary["native_process"], ordinary_terminal=ordinary["terminal"], ordinary_output=ordinary["output"],
            build_result=build_results[configuration], source_epoch=build["source_epoch"], tool_profile=build["tool_profile"])
        witnesses = {}
        for role, reference in references.items():
            original = checked(reference, root)
            owner = context.data if role.endswith(("before", "output")) else context.evidence
            payload = original.read_bytes()
            witnesses[role] = _publish_bytes(owner / f"{reference['sha256'][:16]}-{original.name}", payload, root)
            if witnesses[role]["sha256"] != reference["sha256"]:
                raise ValueError("qualification witness changed while copying")
        rows[configuration] = dict(source=seal["configurations"][configuration]["save"],
            observed=native_facts["observed"], native_facts=native_facts, ordinary_facts=ordinary_facts, witnesses=witnesses)
    document = dict(_documentation=__doc__, schema_version=1, kind=KIND, complete=True,
        foundation_type=FoundationKind.SEALED_CHECKPOINT.value,
        seal=dict(path=checkpoint.manifest_path.relative_to(root).as_posix(), sha256=checkpoint.sha256),
        configurations=rows, closed_scope=SCOPE, financial_policy=POLICY, next_task=seal["next_task"], authorization_granted=False,
        path_policy="Only seal, configurations.source and configurations.witnesses are physical dependencies; identities nested inside completed witnesses are historical, never current EXE/source requirements.")
    _verify_document(document, root, context)
    # All current build inputs must still match after witness copying. A source
    # or linker writer cannot turn mixed epochs into a completed qualification.
    for configuration, (_native, _ordinary, build, _native_facts, _ordinary_facts) in candidates.items():
        validate_build_document(build, read(build["source_epoch"], root), read(build["tool_profile"], root), configuration, root, True)
    _publish_bytes(target, (json.dumps(document, indent=2) + "\n").encode("utf-8"), root)
    foundation = load_foundation(target, root=root)
    verify_foundation(foundation)
    return foundation


def _verify_document(document, root, context):
    """Recheck completed physical witnesses without opening their historical EXE paths.

    @param document Exact typed foundation data.
    @param root Checkout containing copied witnesses and the original seal.
    @param context Explicit review run owning every copied qualification witness.
    @return Selected source/observation facts for future explicitly bound operations.
    """
    _shape(document)
    seal_path = checked(document["seal"], root)
    checkpoint = load_checkpoint(seal_path, root=root)
    verify_checkpoint(checkpoint)
    seal = json.loads(seal_path.read_text(encoding="utf-8"))
    if document["next_task"] != seal["next_task"]:
        raise ValueError("original qualification changed the next source task")
    selected = {}
    for configuration, row in document["configurations"].items():
        # Every independent configuration needs its original source and complete
        # bounded witness set; an abbreviated history must fail explicitly.
        if set(row) != {"source", "observed", "native_facts", "ordinary_facts", "witnesses"} or set(row["witnesses"]) != WITNESSES or row["source"] != seal["configurations"][configuration]["save"]:
            raise ValueError("qualified source/witness closure disagrees")
        witnesses = {role: checked(reference, root) for role, reference in row["witnesses"].items()}
        # Only this qualification run's copied data/evidence are live witnesses.
        # A raw old logs/build/review reference cannot evade the export boundary
        # merely because it still has the same bytes on this workstation.
        for role, path in witnesses.items():
            owner = context.data if role.endswith(("before", "output")) else context.evidence
            if path.parent != owner:
                raise ValueError("qualification witness belongs to another run/role")
        native, ordinary, build, epoch, profile = [json.loads(witnesses[role].read_text(encoding="utf-8"))
            for role in ("native_result", "ordinary_result", "build_result", "source_epoch", "tool_profile")]
        validate_build_document(build, epoch, profile, configuration, root, False)
        # Preserve exact original witness relationships while selecting only
        # their copied physical bytes. Old paths nested in receipts are not read.
        original_roles = dict(native_process=native["native_process"], native_terminal=native["terminal"], native_before=native["before"], native_output=native["output"],
            ordinary_process=ordinary["native_process"], ordinary_terminal=ordinary["terminal"], ordinary_output=ordinary["output"], source_epoch=build["source_epoch"], tool_profile=build["tool_profile"])
        for role, original in original_roles.items():
            if row["witnesses"][role]["sha256"] != original["sha256"]:
                raise ValueError("completed qualification witness identity disagrees")
        native_process = json.loads(witnesses["native_process"].read_text(encoding="utf-8"))
        ordinary_process = json.loads(witnesses["ordinary_process"].read_text(encoding="utf-8"))
        # Copied records retain their original image/input/phase linkage. Their
        # historical classification never excuses disagreement between witnesses.
        if native["configuration"] != configuration or ordinary["configuration"] != configuration or native["source"] != row["source"] or ordinary["source"] != row["source"] or native["tag"] != TAG or native["phase"] != PHASE or ordinary["phase"] != "ordinary-static" or native["application"] != build["targets"]["openttd_surface_capital_test"] or ordinary["application"] != build["targets"]["openttd"]:
            raise ValueError("completed qualification image/input/phase facts disagree")
        # Each actual completed process must identify the same selected source
        # and image as its receipt, without reopening the original EXE path.
        if native_process["exit_status"] != 0 or ordinary_process["exit_status"] != 0 or native_process["executable"] != native["application"] or ordinary_process["executable"] != ordinary["console_application"] or native_process["inputs"] != [row["source"]] or ordinary_process["inputs"] != [row["source"]]:
            raise ValueError("completed qualification process facts disagree")
        native_fields = terminal_fields(witnesses["native_terminal"].read_text(encoding="utf-8", errors="replace"))
        expected = read(seal["configurations"][configuration]["expected_observation"], root)["observation"]
        source = checked(row["source"], root)
        native_facts = compare_loaded_saves(source, witnesses["native_before"], witnesses["native_output"], expected, native_fields)
        ordinary_facts = compare_static_saves(source, witnesses["ordinary_output"], expected, native_fields)
        projection = ordinary["projection"]
        # Full image-byte/DLL comparison happened before publication. These are
        # completed identity facts now, never a promise about another executable.
        if set(projection) != {"method", "application", "executable", "dlls"} or projection["method"] not in ("same_image", "pe_subsystem_2_to_3") or projection["application"] != ordinary["application"] or projection["executable"] != ordinary["console_application"]:
            raise ValueError("completed ordinary projection identities disagree")
        ordinary_facts["projection"] = projection
        # Recomputed physical saves and the independent original expectation take
        # precedence over serialized summaries inside the completed foundation.
        if row["native_facts"] != native_facts or row["ordinary_facts"] != ordinary_facts or row["observed"] != expected:
            raise ValueError("completed qualification semantic facts changed")
        selected[configuration] = dict(source=row["source"], observed=expected)
    # Rehash the actual retained authorities at close; nested historical image
    # and source-epoch paths deliberately do not participate in this live set.
    references = [document["seal"]]
    for row in document["configurations"].values():
        references.extend([row["source"], *row["witnesses"].values()])
    for reference in references:
        checked(reference, root)
    return dict(complete=True, foundation_type=FoundationKind.SEALED_CHECKPOINT.value,
                configurations=selected, next_task=document["next_task"], authorization_granted=False,
                historical_images_required=False, seal=document["seal"], financial_policy=POLICY, closed_scope=SCOPE)


def load_foundation(path, configuration=None, root=None):
    """Load only an explicitly published typed foundation without runtime creation.

    @param path Explicit qualified current review report.
    @param configuration Debug/RelWithDebInfo selection, or None for both.
    @param root Optional checkout owning all physical witnesses.
    @return SpaceFoundation bound to the exact document and discriminator.
    """
    source = Path(root or ROOT).resolve()
    selected, _context = _destination(path, source, allow_retained=True)
    if configuration not in (*CONFIGURATIONS, None):
        raise ValueError("unknown foundation configuration")
    digest = file_identity(selected)[0]
    document = json.loads(selected.read_text(encoding="utf-8"))
    # New operation receipts have their own real schema. They are never adapted
    # into qualified-original records or routed through legacy verify/bind code.
    if document.get("kind") == "space_operation_receipt":
        from space_checkpoint_operation import validate_receipt_shape
        validate_receipt_shape(document)
    else:
        _shape(document)
    if file_identity(selected)[0] != digest:
        raise ValueError("foundation changed during typed loading")
    return SpaceFoundation(source, selected, digest, FoundationKind(document["foundation_type"]), configuration)


def verify_foundation(foundation, observer=None, _seen=None):
    """Verify an exact completed selection; never qualify a different current build.

    @param foundation Typed result of load_foundation, not a raw legacy receipt.
    @param observer Explicit later Plan11 observer; original qualification uses its own owner.
    @param _seen Internal explicit ancestry paths for cycle refusal, never latest-run discovery.
    @return Bound continuation sources and observations after witness verification.
    """
    if not isinstance(foundation, SpaceFoundation):
        raise TypeError("verify_foundation requires a typed SpaceFoundation")
    seen = set() if _seen is None else set(_seen)
    if foundation.manifest_path in seen:
        raise ValueError("continuation foundation cycle")
    seen.add(foundation.manifest_path)
    if file_identity(foundation.manifest_path)[0] != foundation.sha256:
        raise ValueError("foundation changed since typed selection")
    _target, context = _destination(foundation.manifest_path, foundation.root, allow_retained=True)
    document = json.loads(foundation.manifest_path.read_text(encoding="utf-8"))
    if foundation.foundation_type is FoundationKind.OPERATION_RECEIPT:
        from space_checkpoint_operation import verify_operation_document
        result = verify_operation_document(document, foundation.root, context, observer, seen)
    else:
        result = _verify_document(document, foundation.root, context)
    if file_identity(foundation.manifest_path)[0] != foundation.sha256:
        raise ValueError("foundation changed during verification")
    return result


def export_foundation(foundation, observer=None):
    """Durably export the declared compact continuation closure without copying old EXEs.

    @param foundation Explicit verified generated or already retained typed foundation.
    @param observer Optional exact Plan11 observer required by a later operation receipt.
    @return Verified durable foundation; repeated export reuses identical bytes.
    """
    facts = verify_foundation(foundation, observer)
    root = foundation.root
    retained = checked_path("src/tests/fixtures/checkpoints/space/continuations", root)
    if foundation.manifest_path.is_relative_to(retained):
        return foundation
    document = json.loads(foundation.manifest_path.read_text(encoding="utf-8"))
    home = retained / foundation.sha256
    existing = home / "reports/foundation.json"
    if existing.is_file():
        exported = load_foundation(existing, foundation.configuration, root)
        verify_foundation(exported, observer)
        previous = json.loads(existing.read_text(encoding="utf-8"))
        expected_witnesses = {name: {role: ref["sha256"] for role, ref in row["witnesses"].items()}
                              for name, row in document["configurations"].items()}
        actual_witnesses = {name: {role: ref["sha256"] for role, ref in row["witnesses"].items()}
                            for name, row in previous["configurations"].items()}
        # Already published exports retain their existing safe spelling. Exact
        # original witness bytes and dispatch must agree before idempotent reuse.
        if (actual_witnesses != expected_witnesses or previous["next_task"] != document["next_task"]
                or previous["foundation_type"] != document["foundation_type"]):
            raise ValueError("existing durable continuation differs from the selected original witnesses")
        return exported
    parent = None
    if foundation.foundation_type is FoundationKind.OPERATION_RECEIPT:
        predecessor = load_foundation(checked(document["predecessor"], root), root=root)
        parent = export_foundation(predecessor, observer)
        parent_facts = verify_foundation(parent, observer)
        document["predecessor"] = dict(path=parent.manifest_path.relative_to(root).as_posix(), sha256=parent.sha256)
    # Every live witness is copied by its declared role. Nested image/source
    # paths inside those immutable receipts remain historical facts, never files
    # discovered by a recursive archive walk.
    for configuration, row in document["configurations"].items():
        original_output = row.get("output")
        for role, reference in row["witnesses"].items():
            source = checked(reference, root)
            is_save = role in ("before", "output", "native_before", "native_output", "ordinary_output")
            owner = home / ("data" if is_save else "evidence")
            target = owner / (reference["sha256"][:16] + "-" + role + source.suffix)
            row["witnesses"][role] = _publish_bytes(target, source.read_bytes(), root)
            if row["witnesses"][role]["sha256"] != reference["sha256"]:
                raise ValueError("continuation witness changed while exporting")
        if parent is not None:
            row["source"] = parent_facts["configurations"][configuration]["source"]
            row["output"] = row["witnesses"]["output"]
            if row["output"]["sha256"] != original_output["sha256"]:
                raise ValueError("successor export changed saved output identity")
    target = home / "reports/foundation.json"
    context = SimpleNamespace(directory=home, reports=home / "reports", data=home / "data", evidence=home / "evidence")
    if parent is None:
        _verify_document(document, root, context)
    else:
        from space_checkpoint_operation import verify_operation_document
        verify_operation_document(document, root, context, observer, set())
    if file_identity(foundation.manifest_path)[0] != foundation.sha256:
        raise ValueError("source foundation changed while exporting")
    _publish_bytes(target, (json.dumps(document, indent=2) + "\n").encode("utf-8"), root)
    exported = load_foundation(target, foundation.configuration, root)
    retained_facts = verify_foundation(exported, observer)
    # Export changes physical owners, never saved bytes, task dispatch, closed
    # policies or observations. A partial copied graph cannot become resumable.
    if (retained_facts["next_task"] != facts["next_task"]
            or {name: row["source"]["sha256"] for name, row in retained_facts["configurations"].items()}
            != {name: row["source"]["sha256"] for name, row in facts["configurations"].items()}):
        raise ValueError("durable continuation export changed selected saved state/dispatch")
    return exported
