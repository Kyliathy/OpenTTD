"""Retain compact verified Space history before sealing its checkpoint contract.

@file publish_history_audit.py Execution aid; copies immutable witnesses without editing them.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups7-8.
"""

import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_inventory import checked_path, file_identity
from artifact_paths import contained, owned_root


def retain(reference, destination, records, role):
    """Copy one qualified original witness into its exact history owner without overwrite.

    @param reference Original repository-relative path/SHA256 identity.
    @param destination Explicit history-relative destination preserving immutable bytes.
    @param records Receipt list receiving original and retained identities plus semantic role.
    @param role Historical witness purpose; nested old paths are facts, not live inputs.
    @return Retained repository-relative path/SHA256 identity.
    """
    source = checked_path(reference["path"], ROOT)
    if file_identity(source)[0] != reference["sha256"]:
        raise ValueError(f"original witness changed: {reference['path']}")
    owner = owned_root(ROOT, "src/tests/fixtures/cleanup/space-history")
    target = contained(checked_path(owner / destination, ROOT), owner)
    target.parent.mkdir(parents=True, exist_ok=True)
    if not target.exists():
        with target.open("xb") as stream:
            stream.write(source.read_bytes())
    if file_identity(target)[0] != reference["sha256"]:
        raise ValueError(f"retained witness conflicts: {target}")
    retained = dict(path=target.relative_to(ROOT).as_posix(), sha256=reference["sha256"])
    records.append(dict(role=role, original=reference, retained=retained))
    return retained


def read(reference):
    """Read only the exact JSON authority named by an already verified audit.

    @param reference Contained repository path/SHA256 identity.
    @return Parsed immutable metadata; mismatches fail before publication.
    """
    path = checked_path(reference["path"], ROOT)
    if file_identity(path)[0] != reference["sha256"]:
        raise ValueError(f"audit input changed: {reference['path']}")
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    """Publish original receipt witnesses, source epochs and compact actual process facts.

    @return None; no EXE/screenshot is retained and no checkpoint seal is claimed yet.
    """
    audit_path = ROOT / "test-cache/cleanup/space-history-audit.json"
    audit = json.loads(audit_path.read_text(encoding="utf-8"))
    if not audit["complete"] or audit["fresh_application_ran"] or audit["child_process_attempts"] != 0:
        raise ValueError("historical audit is incomplete or executed a child")
    root = read(audit["original_root"])
    records, windows, processes, source_witnesses = [], [], {}, {}
    audit_reference = dict(path=audit_path.relative_to(ROOT).as_posix(), sha256=file_identity(audit_path)[0])
    retained_audit = retain(audit_reference, "audit.json", records, "validated_historical_facts_not_fresh_native_proof")
    retain(audit["original_root"], "original-bound.json", records, "original_bound_receipt_bytes")
    retain(audit["original_cli_failure"], "original-cli-failure.log", records, "original_verifier_failure_before_storage_selection")
    retain(audit["storage_bound_result"], "storage-selected-bind.json", records, "unchanged_original_verifier_with_explicit_storage_aliases")
    # Thirteen source epochs explain original observer and native-image changes;
    # retain their small manifests/source bytes, never their executable payloads.
    for index, reference in enumerate(audit["source_epochs"]):
        epoch = read(reference)
        retain(reference, f"epochs/source-epoch-{index+1:02d}.json", records, "historical_source_epoch_manifest")
        for product in epoch["products"]:
            source = product["source"]
            if Path(source).suffix.lower() in {".cpp", ".h", ".py"}:
                source_witnesses[(source, product["sha256"])] = dict(path=product["path"], sha256=product["sha256"])
    graph = json.loads((ROOT / "test-cache/cleanup/space-graph-debug.json").read_text(encoding="utf-8"))
    # Other features advanced some imported live sources before this cleanup.
    # Their exact declared old source bodies remain useful independent witnesses.
    for edge in graph["edges"]:
        logical = edge["input"]["path"]
        if not edge["required_for_current_validation"] or edge["verification"] != "archived_verified":
            continue
        if Path(logical).suffix.lower() in {".cpp", ".h", ".py"} or Path(logical).name == "CMakeLists.txt":
            source_witnesses[(logical, edge["input"]["sha256"])] = edge["retained_aliases"][0]
    unique_sources = {}
    for (logical, digest), reference in sorted(source_witnesses.items()):
        if digest not in unique_sources:
            unique_sources[digest] = retain(reference, f"sources/{digest}-{Path(logical).name}", records, "immutable_original_source_epoch_not_compiled_live")
    # Compact every actual native operation and process outcome already validated
    # by the original binder. No prior save/screenshot collection is duplicated.
    names = ["construction-cash-query", *[f"construction-cash-work-{index:03d}" for index in range(1, 33)]]
    for row in root["configurations"]:
        configuration = row["configuration"]
        final = read(row["receipt"])
        retain(row["receipt"], f"original-final-{configuration}.json", records, "original_final_receipt_bytes")
        ordinary = read(row["ordinary"])
        retain(row["ordinary"], f"original-ordinary-{configuration}.json", records, "original_static_restore_receipt_bytes")
        retain(final["operation"]["terminal"], f"native-final-{configuration}.log", records, "original_actual_native_terminal_witness")
        for name in names:
            path = ROOT / f"logs/surface-entry-{name}-{configuration}.json"
            receipt = json.loads(path.read_text(encoding="utf-8"))
            operation = receipt["operation"]
            windows.append(dict(configuration=configuration, stage=name,
                receipt=dict(path=path.relative_to(ROOT).as_posix(), sha256=file_identity(path)[0]),
                source=operation["source"], output=operation["output"], application=operation["application"],
                measured_cash_flow=operation["measured_cash_flow"], capital_scope=operation["capital_scope"],
                initial_cash=operation["initial"]["company"]["cash"], final_cash=operation["final"]["company"]["cash"]))
            for reference in operation["native_processes"]:
                process = read(reference)
                processes[reference["path"]] = dict(original_report=reference, exit_status=process["exit_status"],
                    inputs=process["inputs"], artifacts=process["artifacts"], executable=process["executable"],
                    configuration=process["configuration"], argv=process["argv"])
        for reference in ordinary["operation"]["native_processes"]:
            process = read(reference)
            processes[reference["path"]] = dict(original_report=reference, exit_status=process["exit_status"],
                inputs=process["inputs"], artifacts=process["artifacts"], executable=process["executable"],
                configuration=process["configuration"], argv=process["argv"])
    if len(processes) != 68:
        raise ValueError("compact history lost an actual original process outcome")
    approvals = []
    known_references = {(edge["input"]["path"], edge["input"]["sha256"]) for edge in graph["edges"]}
    # Both exceptional lower-floor decisions are already consumed. Preserve
    # independent user-decision bytes and the recorded closed scope, without
    # treating an old authorized flag as permission for any future window.
    for name, scope_key in (("reserve-bridge", "bridge_scope"), ("arrival-settlement", "settlement_scope")):
        proof_path = ROOT / f"logs/surface-entry-{name}-verification.json"
        proof_reference = dict(path=proof_path.relative_to(ROOT).as_posix(), sha256=file_identity(proof_path)[0])
        if (proof_reference["path"], proof_reference["sha256"]) not in known_references:
            raise ValueError(f"closed scope is outside the imported original lineage: {name}")
        proof = read(proof_reference)
        if not proof[scope_key]["ended"]:
            raise ValueError(f"lower-floor scope remains open: {name}")
        authorization_path = ROOT / f"logs/surface-entry-{name}-authorization.json"
        authorization = dict(path=authorization_path.relative_to(ROOT).as_posix(), sha256=file_identity(authorization_path)[0])
        decision = read(authorization)
        retain(authorization, f"approvals/{name}-authorization.json", records, "consumed_historical_user_decision_not_new_authorization")
        retain(proof_reference, f"approvals/{name}-closed-result.json", records, "recorded_historical_closed_scope_result")
        retain(decision["proposal"], f"approvals/{name}-original-proposal.json", records, "original_bounded_proposal_for_consumed_decision")
        approvals.append(dict(name=name, authorization=authorization, original_result=proof_reference,
            closed_scope=proof[scope_key], consumed=True, further_scope_authorized=False))
    facts = dict(_documentation=__doc__, schema_version=1, kind="space_historical_witnesses",
        validated_audit=retained_audit, live_retained_inputs=records,
        historical_path_policy="Paths inside original receipts, epochs, process outcomes and windows are non-dereferenced historical facts. Only live_retained_inputs.retained are required files.",
        windows=windows, process_outcomes=list(processes.values()), source_witness_aliases=[dict(original_path=logical, sha256=digest, retained=unique_sources[digest])
            for logical, digest in sorted(source_witnesses)],
        closed_scope=root["capital_scope"], actual_cash=root["actual_cash"], measured_cash_flow=root["measured_cash_flow"],
        operating_floor=40000, capital_floor=100000, lower_floor_exceptions_spent=True, consumed_lower_floor_approvals=approvals,
        further_paid_scope_authorized=False, next_task="Plan11/15CSGZVEEI", fresh_build_qualified=False)
    destination = ROOT / "src/tests/fixtures/cleanup/space-history-facts.json"
    destination.write_text(json.dumps(facts, indent=2) + "\n", encoding="utf-8")
    print("Retained historical witnesses", len(records), "unique source epochs", len(unique_sources),
        "actual process facts", len(processes), "facts bytes", destination.stat().st_size)


if __name__ == "__main__":
    main()
