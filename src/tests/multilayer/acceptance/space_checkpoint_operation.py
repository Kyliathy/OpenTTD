"""Publish genuine explicitly selected Plan11 operations after qualified recovery.

@file space_checkpoint_operation.py No paid execution or inherited authorization.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Groups22/36, durable save and ordinary reload roles.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group12B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md.
The Plan11producer performs its own authorized action; this module validates
existing actual process/save/build evidence and invokes its pure business validator.
"""

import inspect
import json
from pathlib import Path
import re

from space_checkpoint import (CONFIGURATIONS, POLICY, SCOPE, FoundationKind, ORIGINAL_OBSERVER,
                              SpaceFoundation, SpaceObserver, _publish_bytes)
import space_checkpoint_foundation as foundation_owner
from space_checkpoint_load import checked, identity, compare_clocks, compare_revision
from artifact_inventory import file_identity
from artifact_paths import RunContext, identifier
from space_preview_checks import chunks
from savegame_tables import saved_owner_table as table

KIND = "space_operation_receipt"
RESULT_FIELDS = {"_documentation", "schema_version", "kind", "complete", "configuration", "task", "phase", "tag",
                 "source", "before", "output", "application", "terminal", "native_process", "build_result",
                 "source_files", "validator_source", "observer_contract", "initial", "final", "financial_policy", "scenario", "run_id"}
FIELDS = {"_documentation", "schema_version", "kind", "complete", "foundation_type", "predecessor", "seal",
          "task", "next_task", "observer_contract", "configurations", "closed_scope", "financial_policy", "authorization_granted", "path_policy"}
WITNESSES = {"operation_result", "native_process", "native_terminal", "before", "output", "build_result", "source_epoch", "tool_profile", "validator_source"}
ORDINARY_WITNESSES = {"ordinary_result", "ordinary_process", "ordinary_terminal", "ordinary_output"}
CLOSED_TASKS = {"Plan11/15CSGZVEEH", "Plan11/15CSGZVEEF", "Plan11/15CSGZVEE", "Plan11/15CSGZQ", "Plan11/15CSGZRB"}
CLOSED_PHASES = {"construction-cash-query", "construction-cash-work", "working-cash-query", "working-cash-work", "reserve-bridge", "arrival-settlement"}


def _observer(contract, selected):
    """Select an explicit current pure observer without importing historical callers.

    @param contract Saved semantic observer identifier.
    @param selected Explicit SpaceObserver, or None for original retirement semantics.
    @return Matching callable adapter; unknown paid-body contracts never get guessed.
    """
    actual = ORIGINAL_OBSERVER if selected is None else selected
    # A later body mode changes ownership semantics and must be chosen by its
    # actual Plan11 caller, rather than inferred from a completed receipt.
    if not isinstance(actual, SpaceObserver) or actual.contract_id != contract or not callable(actual.read):
        raise ValueError("select the explicit matching Plan11 observer contract")
    return actual


def _parent(reference, root, observer, seen=None):
    """Verify one explicit predecessor using its actual recorded observation role.

    @param reference Exact predecessor foundation/operation report identity.
    @param root Current checkout.
    @param observer Explicit current later observer, if needed by the predecessor.
    @param seen Internal ancestry paths for cycle refusal.
    @return Typed predecessor plus checked selected source/observation facts.
    """
    path = checked(reference, root)
    predecessor = foundation_owner.load_foundation(path, root=root)
    document = json.loads(path.read_text(encoding="utf-8"))
    # Original qualification/operations retain their original observer. A later
    # paid-body reader cannot reinterpret the already completed original census.
    selected = None if document.get("observer_contract", ORIGINAL_OBSERVER.contract_id) == ORIGINAL_OBSERVER.contract_id else observer
    return predecessor, foundation_owner.verify_foundation(predecessor, selected, seen)


def _scope(terminal, operation):
    """Read one task-owned native result marker rather than relabeling an old cash run.

    @param terminal Actual retained current-run terminal text.
    @param operation Typed current operation result.
    @return Actual numeric clock/revision metadata for strict saved-state comparison.
    """
    lines = [line for line in terminal.splitlines() if line.startswith("SPACE_OPERATION_RESULT ")]
    if len(lines) != 1:
        raise ValueError("operation needs one actual task-owned native result marker")
    if not re.search(r"All tests passed .*1 test case", terminal):
        raise ValueError("operation needs actual successful single-case native completion")
    tokens = [value.split("=", 1) for value in lines[0].split()[1:]]
    fields = dict(tokens)
    required = {"task", "phase", "ticks_before", "ticks_after", "revision_text", "revision_modified", "revision_newgrf", "save_version", "inherited_paid_scope"}
    # A successful old paid stage cannot become a new operation by changing a
    # JSON label. The selected native callback must emit this exact task/phase
    # and explicitly refuse inherited financial permissions.
    if len(fields) != len(tokens) or set(fields) != required or fields["task"] != operation["task"] or fields["phase"] != operation["phase"] or fields["inherited_paid_scope"] != "false":
        raise ValueError("native operation task/phase/inherited scope disagrees")
    return {key: value if key in ("task", "phase", "revision_text", "inherited_paid_scope") else int(value)
            for key, value in fields.items()}


def _result_shape(operation, task, configuration):
    """Admit only a real new typed operation with closed original financial policies.

    @param operation Parsed space_operation_result, not a legacy operation dict.
    @param task Exact task selected by the predecessor's next dispatch.
    @param configuration Independent configuration being validated.
    @return None; no historical work/query receipt is auto-converted.
    """
    if not isinstance(operation, dict):
        raise ValueError("new operation requires its exact typed result fields")
    fields = RESULT_FIELDS | ({"ordinary_result"} if operation.get("schema_version") == 2 else set())
    if set(operation) != fields:
        raise ValueError("new operation requires its exact typed result fields")
    # Scope remains owned by the current Plan11task. Closed stages and a load-only
    # qualifier cannot masquerade as a genuine new capacity/operation producer.
    if type(operation["schema_version"]) is not int or operation["schema_version"] not in (1, 2) or operation["kind"] != "space_operation_result" or operation["complete"] is not True or operation["configuration"] != configuration or operation["task"] != task or task in CLOSED_TASKS or operation["phase"] in CLOSED_PHASES or operation["tag"] == "[.space-checkpoint-load]" or operation["financial_policy"] != POLICY:
        raise ValueError("operation task/configuration/closed financial scope disagrees")
    identifier(operation["phase"])


def _ordinary_witnesses(operation, build, root, copies=None):
    """Verify version-two ordinary reloads while preserving version-one history.

    @param operation Exact typed operation whose schema selects its witness roles.
    @param build Selected verified current or completed native build document.
    @param root Checkout containing explicit physical inputs.
    @param copies Copied witness paths after publication/export, or None for live proof.
    @return Additional explicit ordinary identities; old completed operations return none.
    """
    if operation["schema_version"] == 1:
        return {}
    from space_operation_restore import verify_restore, witness_references
    path = checked(operation["ordinary_result"], root) if copies is None else copies["ordinary_result"]
    if file_identity(path)[0] != operation["ordinary_result"]["sha256"]:
        raise ValueError("ordinary operation result changed during relocation")
    document = json.loads(path.read_text(encoding="utf-8"))
    verify_restore(document, operation, root, build, current=copies is None, copies=copies)
    return witness_references(operation, document)


def _saved_facts(operation, source, before, output, terminal, parent_row, observer):
    """Check actual pre-action state, native counters and complete saved owner sets.

    @param operation Typed actual current or copied completed operation result.
    @param source Exact selected predecessor save.
    @param before Native saved state before any authorized new action.
    @param output Actual saved result of that action.
    @param terminal Current or copied actual process transcript.
    @param parent_row Independently verified predecessor source/observation.
    @param observer Explicit current semantic observation adapter.
    @return Actual initial/final observations and elapsed native ticks.
    """
    compare_clocks(source, before, before)
    native = _scope(terminal.read_text(encoding="utf-8", errors="replace"), operation)
    versions, families = zip(*(chunks(path) for path in (source, before, output)))
    original, initial, final = families
    # No action may precede the declared before snapshot. Check every physical
    # family plus explicit clock/pause/revision semantics before the business
    # validator considers any genuine output changes.
    if versions != (440, 440, 440) or len(original) != 121 or original.keys() != initial.keys() or original.keys() != final.keys():
        raise ValueError("operation lost the complete121-family save closure")
    for family, raw in original.items():
        if family not in (b"DATE", b"GLOG") and initial[family] != raw:
            raise ValueError("operation changed physical owners before its before snapshot")
    compare_revision(source, before, native)
    first_tick, last_tick = table(before, b"DATE")[0]["tick_counter"], table(output, b"DATE")[0]["tick_counter"]
    # Recorded work cannot run backwards or hide ticks outside its actual native
    # before/output snapshots; the business validator owns the permitted budget.
    if native["ticks_before"] != first_tick or native["ticks_after"] != last_tick or last_tick < first_tick or native["save_version"] != 440:
        raise ValueError("operation native clocks disagree with actual saved ticks")
    observed_before, observed_after = [json.loads(json.dumps(observer.read(path))) for path in (before, output)]
    # Preserve the independently verified predecessor census before admitting
    # any output changes under the current task's explicit observer contract.
    if observed_before != parent_row["observed"] or operation["initial"] != observed_before or operation["final"] != observed_after:
        raise ValueError("operation actual owner/material observations disagree")
    return dict(initial=observed_before, final=observed_after, elapsed_ticks=last_tick - first_tick, chunk_families=121)


def _validation_facts(validation, operation, facts):
    """Check the explicit task validator's measured facts without granting future scope.

    @param validation Result returned by the actual Plan11-owned pure validator.
    @param operation Selected task/phase result.
    @param facts Independently measured saved counters/observations.
    @return None; no missing validator or inherited approval can become permission.
    """
    # Only the complete measured business contract may certify this task;
    # a convenient truthy summary cannot supply missing clocks or permissions.
    if not isinstance(validation, dict) or set(validation) != {"kind", "task", "phase", "elapsed_ticks", "authorizes_future_operations", "facts"}:
        raise ValueError("Plan11 validator must return its explicit measured business facts")
    # Completed business facts must agree with the measured native task and
    # clocks. They can never grant permissions to a later task.
    if validation["kind"] != "plan11_operation_validation" or validation["task"] != operation["task"] or validation["phase"] != operation["phase"] or validation["elapsed_ticks"] != facts["elapsed_ticks"] or validation["authorizes_future_operations"] is not False or not isinstance(validation["facts"], dict):
        raise ValueError("Plan11 validation task/ticks/future permission disagrees")


def validate_receipt_shape(document):
    """Admit the actual successor discriminator without accepting old legacy receipts.

    @param document Candidate or published typed operation receipt.
    @return None; original closed financial scopes remain immutable history.
    """
    # Keep completed receipts within their exact role/authority boundary;
    # missing or extra fields cannot silently introduce a new paid allowance.
    if not isinstance(document, dict) or set(document) != FIELDS:
        raise ValueError("operation receipt requires exact typed fields")
    # Publication records completed work, never permission for another action.
    # Both configurations and the original consumed financial decisions persist.
    if type(document["schema_version"]) is not int or document["schema_version"] != 1 or document["kind"] != KIND or document["complete"] is not True or document["foundation_type"] != FoundationKind.OPERATION_RECEIPT.value or document["closed_scope"] != SCOPE or document["financial_policy"] != POLICY or document["authorization_granted"] is not False or set(document["configurations"]) != set(CONFIGURATIONS):
        raise ValueError("invalid operation receipt discriminator/scope/policy")
    # Dispatch must remain explicit and cannot reopen a completed cash decision.
    if not isinstance(document["next_task"], str) or not document["next_task"].startswith("Plan11/") or document["next_task"] in CLOSED_TASKS:
        raise ValueError("successor cannot reopen a closed Plan11 task")


def publish_successor(predecessor, operations, next_task, destination, operation_validator, observer=None):
    """Publish paired actual operations only after current provenance and business checks.

    @param predecessor Typed selected qualified original or genuine operation receipt.
    @param operations Both configurations' exact current typed result references.
    @param next_task Explicit subsequent/current Plan11dispatch, never a closed task.
    @param destination Explicit current Space review report destination.
    @param operation_validator Pure Plan11(source,before,output,result,root) business validator.
    @param observer Explicit SpaceObserver, or the unchanged original default.
    @return Typed operation foundation after atomic immutable publication.
    """
    # Only the explicit current Plan11 owner may validate completed business
    # permissions; an untyped legacy dict or absent callback cannot bypass it.
    if not isinstance(predecessor, SpaceFoundation) or not callable(operation_validator):
        raise TypeError("successor requires a typed predecessor and explicit Plan11 validator")
    root = predecessor.root
    predecessor_reference = identity(predecessor.manifest_path, root)
    if predecessor_reference["sha256"] != predecessor.sha256:
        raise ValueError("predecessor changed since typed selection")
    _selected, parent = _parent(predecessor_reference, root, observer)
    target, context = foundation_owner._destination(destination, root)
    if set(operations) != set(CONFIGURATIONS):
        raise ValueError("successor needs both independent configurations")
    actual_observer = ORIGINAL_OBSERVER if observer is None else observer
    if not isinstance(actual_observer, SpaceObserver):
        raise TypeError("select an explicit SpaceObserver adapter")
    candidate_rows = {}
    # Check every real result before publishing any consumer-selectable receipt.
    # The business callback only validates already executed work; it must not
    # launch a producer or derive another paid allowance from parent metadata.
    for configuration in CONFIGURATIONS:
        operation = foundation_owner.read(operations[configuration], root)
        _result_shape(operation, parent["next_task"], configuration)
        _observer(operation["observer_contract"], actual_observer)
        if operation["source"] != parent["configurations"][configuration]["source"]:
            raise ValueError("operation selected a different predecessor save")
        source, before, output, application, terminal, process_path = [checked(operation[field], root)
            for field in ("source", "before", "output", "application", "terminal", "native_process")]
        actual_context = RunContext(root, "space", identifier(operation["scenario"]), identifier(operation["run_id"]))
        # Before/output saves and the actual transcript/process report must all
        # belong to the declared current invocation, rather than an older run.
        if before.parent != actual_context.saves or output.parent != actual_context.saves or terminal.parent != actual_context.evidence or process_path.parent != actual_context.reports:
            raise ValueError("operation process outputs belong to another invocation/role")
        process = foundation_owner.read(operation["native_process"], root)
        # Require the actual selected native case, successful child status and
        # every claimed output, rather than accepting a report's complete flag.
        if process["exit_status"] != 0 or process["inputs"] != [operation["source"]] or process["executable"] != operation["application"] or process["configuration"] != configuration or process["argv"] != [str(application), operation["tag"], "--durations", "yes"] or any(operation[field] not in process["artifacts"] for field in ("before", "output", "terminal")) or operation["terminal"] not in process["transcripts"]:
            raise ValueError("operation actual process membership/image/tag disagrees")
        build = foundation_owner.read(operation["build_result"], root)
        epoch, profile = foundation_owner.read(build["source_epoch"], root), foundation_owner.read(build["tool_profile"], root)
        foundation_owner.validate_build_document(build, epoch, profile, configuration, root, True, ("openttd_surface_capital_test",))
        if build["targets"]["openttd_surface_capital_test"] != operation["application"]:
            raise ValueError("operation did not run its actual current capital build")
        epoch_identities = {(reference["path"], reference["sha256"]) for reference in epoch["files"]}
        native_sources = []
        # Bind the selected native tag and pure business validator to the same
        # actual build epoch so an unrelated current build cannot certify them.
        for reference in operation["source_files"]:
            path = checked(reference, root)
            if (reference["path"], reference["sha256"]) not in epoch_identities:
                raise ValueError("operation source is absent from its actual build epoch")
            if path.suffix in (".cpp", ".h"):
                native_sources.append(path.read_text(encoding="utf-8"))
        if not any(operation["tag"] in text for text in native_sources):
            raise ValueError("operation selected tag has no actual native source owner")
        checked(operation["validator_source"], root)
        validator_file = inspect.getsourcefile(getattr(operation_validator, "func", operation_validator))
        # Bind the actual current Python validator's bytes to the declared
        # task source, including tests whose identical copy lives in a fake root.
        if validator_file is None or file_identity(Path(validator_file))[0] != operation["validator_source"]["sha256"] or operation["validator_source"] not in operation["source_files"]:
            raise ValueError("operation validator source does not match the actual callable")
        facts = _saved_facts(operation, source, before, output, terminal, parent["configurations"][configuration], actual_observer)
        ordinary_references = _ordinary_witnesses(operation, build, root)
        validation = operation_validator(source, before, output, operation, root)
        _validation_facts(validation, operation, facts)
        candidate_rows[configuration] = (operation, build, facts, validation, ordinary_references)
    first, second = [candidate_rows[name][2]["final"] for name in CONFIGURATIONS]
    if first != second:
        raise ValueError("operation cross-configuration owner/material results disagree")
    rows = {}
    # Each configuration exports its own measured saves and bounded witnesses;
    # cross-configuration agreement does not permit sharing a world identity.
    for configuration, (operation, build, facts, validation, ordinary_references) in candidate_rows.items():
        references = dict(operation_result=operations[configuration], native_process=operation["native_process"], native_terminal=operation["terminal"],
            before=operation["before"], output=operation["output"], build_result=operation["build_result"],
            source_epoch=build["source_epoch"], tool_profile=build["tool_profile"], validator_source=operation["validator_source"])
        references.update(ordinary_references)
        witnesses = {}
        # Copy only bounded independent witnesses/current saves. Old image paths
        # remain completed facts and never become an executable retention rule.
        for role, reference in references.items():
            path = checked(reference, root)
            owner = context.data if role in ("before", "output", "ordinary_output") else context.evidence
            witnesses[role] = _publish_bytes(owner / f"{reference['sha256'][:16]}-{path.name}", path.read_bytes(), root)
        rows[configuration] = dict(source=operation["source"], output=witnesses["output"],
                                   facts=facts, validation=validation, witnesses=witnesses)
    document = dict(_documentation=__doc__, schema_version=1, kind=KIND, complete=True,
        foundation_type=FoundationKind.OPERATION_RECEIPT.value, predecessor=predecessor_reference, seal=parent["seal"],
        task=parent["next_task"], next_task=next_task, observer_contract=actual_observer.contract_id,
        configurations=rows, closed_scope=SCOPE, financial_policy=POLICY, authorization_granted=False,
        path_policy="Only predecessor/seal, source/output and copied witnesses are physical dependencies; nested old EXE/source identities remain completed facts.")
    verify_operation_document(document, root, context, actual_observer, {target})
    # Close on actual current source/image epochs after all validation/copying;
    # this publication can never certify a different build that raced the checks.
    for configuration, (operation, build, _facts, _validation, ordinary_references) in candidate_rows.items():
        foundation_owner.validate_build_document(build, foundation_owner.read(build["source_epoch"], root), foundation_owner.read(build["tool_profile"], root), configuration, root, True, ("openttd_surface_capital_test",))
        checked(operations[configuration], root)
        # Close on every actual ordinary witness too; a replaced reload report
        # cannot become part of an otherwise current native operation receipt.
        for reference in ordinary_references.values():
            checked(reference, root)
    _publish_bytes(target, (json.dumps(document, indent=2) + "\n").encode("utf-8"), root)
    return foundation_owner.load_foundation(target, root=root)


def verify_operation_document(document, root, context, observer, seen):
    """Verify completed successor witnesses without rerunning the old business code/image.

    @param document Exact typed operation receipt.
    @param root Checkout containing the current explicit lineage.
    @param context Review run owning these copied witnesses.
    @param observer Explicit semantic adapter for this receipt's contract.
    @param seen Already visited explicit foundation paths, for cycle refusal.
    @return Actual selected output saves/observations with no future permission.
    """
    validate_receipt_shape(document)
    actual_observer = _observer(document["observer_contract"], observer)
    _predecessor, parent = _parent(document["predecessor"], root, observer, seen)
    # A copied receipt continues the exact selected task and original seal,
    # even after its image/source paths have become historical observations.
    if document["task"] != parent["next_task"] or document["seal"] != parent["seal"]:
        raise ValueError("successor task/seal differs from its explicit predecessor")
    selected = {}
    # Recompute each independent saved world from its compact physical closure;
    # never trust a summary row as a replacement for actual owner observations.
    for configuration, row in document["configurations"].items():
        if set(row) != {"source", "output", "facts", "validation", "witnesses"} or set(row["witnesses"]) not in (WITNESSES, WITNESSES | ORDINARY_WITNESSES) or row["source"] != parent["configurations"][configuration]["source"] or row["output"] != row["witnesses"]["output"]:
            raise ValueError("successor source/output/witness closure disagrees")
        paths = {role: checked(reference, root) for role, reference in row["witnesses"].items()}
        for role, path in paths.items():
            owner = context.data if role in ("before", "output", "ordinary_output") else context.evidence
            if path.parent != owner:
                raise ValueError("successor witness belongs to another invocation/role")
        operation, process, build, epoch, profile = [json.loads(paths[role].read_text(encoding="utf-8"))
            for role in ("operation_result", "native_process", "build_result", "source_epoch", "tool_profile")]
        _result_shape(operation, parent["next_task"], configuration)
        expected_roles = WITNESSES | (ORDINARY_WITNESSES if operation["schema_version"] == 2 else set())
        if set(paths) != expected_roles:
            raise ValueError("operation version disagrees with its ordinary witness closure")
        # Completed copies still bind the original task source and observation
        # semantics. Historical classification cannot excuse a changed lineage.
        # Export relocates the selected predecessor save. Preserve its exact
        # original path/hash inside process facts and require identical bytes at
        # the parent's newly verified physical owner.
        if operation["source"]["sha256"] != row["source"]["sha256"] or operation["observer_contract"] != document["observer_contract"]:
            raise ValueError("successor completed source/observer facts disagree")
        references = dict(native_process=operation["native_process"], native_terminal=operation["terminal"], before=operation["before"], output=operation["output"],
                          build_result=operation["build_result"], source_epoch=build["source_epoch"], tool_profile=build["tool_profile"], validator_source=operation["validator_source"])
        references.update(_ordinary_witnesses(operation, build, root, paths))
        # Relocated copies must preserve the original witness bytes, while old
        # machine paths inside those bytes remain historical facts.
        for role, reference in references.items():
            if row["witnesses"][role]["sha256"] != reference["sha256"]:
                raise ValueError("successor completed witness identity disagrees")
        foundation_owner.validate_build_document(build, epoch, profile, configuration, root, False, ("openttd_surface_capital_test",))
        # Require the actually selected case and all emitted roles even after
        # their old image has been removed by a legitimate clean or rebuild.
        if process["exit_status"] != 0 or process["executable"] != operation["application"] or process["inputs"] != [operation["source"]] or operation["application"] != build["targets"]["openttd_surface_capital_test"] or len(process["argv"]) != 4 or not process["argv"][0].replace("\\", "/").endswith("/" + operation["application"]["path"]) or process["argv"][1:] != [operation["tag"], "--durations", "yes"] or any(operation[field] not in process["artifacts"] for field in ("before", "output", "terminal")) or operation["terminal"] not in process["transcripts"]:
            raise ValueError("successor completed process/build facts disagree")
        facts = _saved_facts(operation, checked(row["source"], root), paths["before"], paths["output"], paths["native_terminal"], parent["configurations"][configuration], actual_observer)
        _validation_facts(row["validation"], operation, facts)
        if row["facts"] != facts:
            raise ValueError("successor saved-state facts changed")
        selected[configuration] = dict(source=row["output"], observed=facts["final"])
    # Complete physical witnesses remain mandatory even though their nested
    # historical binary/source paths are deliberately not dereferenced.
    for row in document["configurations"].values():
        for reference in [row["source"], row["output"], *row["witnesses"].values()]:
            checked(reference, root)
    return dict(complete=True, foundation_type=FoundationKind.OPERATION_RECEIPT.value, configurations=selected,
        next_task=document["next_task"], seal=document["seal"], financial_policy=POLICY, closed_scope=SCOPE,
        authorization_granted=False, historical_images_required=False)
