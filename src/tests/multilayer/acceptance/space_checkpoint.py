"""Seal and read exact Space continuation inputs without native execution.

@file space_checkpoint.py Durable checkpoint boundary, separate from operation receipts.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups8/11/12.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md.
Only explicit retained references are live dependencies. Paths nested inside
historical receipts identify past facts and never request an executable archive.
"""

import argparse
from dataclasses import dataclass
from enum import Enum
import hashlib
import json
import os
from pathlib import Path
import sys
from typing import Callable

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_inventory import checked_path, file_identity
from space_preview_checks import chunks
from surface_constructor_retirement_ledger import observe

OWNER = "src/tests/fixtures/checkpoints/space"
CONFIGURATIONS = ("RelWithDebInfo", "Debug")
FIELDS = {"_documentation", "schema_version", "kind", "historical_validation", "history",
          "historical_roots", "configurations", "closed_scope", "financial_policy",
          "next_task", "fresh_build_qualified", "migration_map"}
SCOPE = dict(window=32, total_earned_ticks=64000, maximum_windows=32, tick_cap=64000,
             ended=True, end_reason="tick_cap")
POLICY = dict(operating_floor=40000, capital_floor=100000, lower_floor_exceptions_spent=True,
              further_paid_scope_authorized=False)


@dataclass(frozen=True)
class SpaceCheckpoint:
    """An explicitly selected immutable manifest identity, never a legacy receipt.

    @param root Source checkout containing all durable references.
    @param manifest_path Exact published checkpoint.json, beneath the Space owner.
    @param sha256 Identity loaded before any consumer selects its continuation save.
    @param configuration Explicit build selection, or None to verify both originals.
    """

    root: Path
    manifest_path: Path
    sha256: str
    configuration: str | None = None


class FoundationKind(str, Enum):
    """Explicit continuation roles, distinct from untyped legacy operation dicts.

    SEALED_CHECKPOINT describes completed qualification of an immutable original;
    OPERATION_RECEIPT describes a later genuine Plan11 operation, supplied by12B.
    """

    SEALED_CHECKPOINT = "sealed_checkpoint"
    OPERATION_RECEIPT = "operation_receipt"


@dataclass(frozen=True)
class SpaceFoundation:
    """An exact published continuation foundation; it grants no paid scope.

    @param root Checkout containing explicit physical witnesses and selected saves.
    @param manifest_path Exact published qualified foundation or operation receipt.
    @param sha256 Immutable identity captured during typed loading.
    @param foundation_type Explicit sealed-checkpoint/operation-receipt discriminator.
    @param configuration Optional explicitly selected independent configuration.
    """

    root: Path
    manifest_path: Path
    sha256: str
    foundation_type: FoundationKind
    configuration: str | None = None


@dataclass(frozen=True)
class SpaceObserver:
    """Explicit current Plan11 observation contract; never loaded from an old receipt.

    @param contract_id Stable caller-selected semantic version identifier.
    @param read Actual current pure owner/material observer for a selected save.
    """

    contract_id: str
    read: Callable[[Path], dict]


ORIGINAL_OBSERVER = SpaceObserver("original-retirement-v1", observe)


def qualify_checkpoint(checkpoint, native_results, ordinary_results, build_results, destination):
    """Validate actual current results, then publish a completed original qualification.

    @param checkpoint Typed immutable original seal.
    @param native_results Configuration-to-current-native-load receipt identities.
    @param ordinary_results Configuration-to-current-ordinary-static receipt identities.
    @param build_results Configuration-to-current-build result identities.
    @param destination Explicit current review reports destination; never a fixture.
    @return Typed qualified SpaceFoundation after all witnesses verify.
    """
    # Import only when explicitly qualifying, preserving read-only seal imports
    # and avoiding a cycle with the native driver that consumes this seal API.
    from space_checkpoint_foundation import qualify_checkpoint as qualify
    return qualify(checkpoint, native_results, ordinary_results, build_results, destination)


def load_foundation(path, configuration=None, root=None):
    """Load an explicitly selected typed foundation without reserving a run.

    @param path Published current foundation, never an inferred latest run.
    @param configuration Explicit Debug/RelWithDebInfo selection, or None for both.
    @param root Optional explicit checkout for isolated verification.
    @return SpaceFoundation; legacy receipts are rejected rather than adapted.
    """
    from space_checkpoint_foundation import load_foundation as load
    return load(path, configuration, root)


def verify_foundation(foundation, observer=None):
    """Verify completed qualification witnesses without requiring its old EXE.

    @param foundation Typed exact selection returned by load_foundation.
    @param observer Optional explicit SpaceObserver for a later Plan11 operation contract.
    @return Checked selected saves/observations; no assumption about a later image.
    """
    from space_checkpoint_foundation import verify_foundation as verify
    return verify(foundation, observer)


def publish_successor(predecessor, operations, next_task, destination, operation_validator, observer=None):
    """Publish actual paired Plan11 operations with their explicit business validator.

    @param predecessor Typed qualified original or completed operation foundation.
    @param operations Configuration-to-current space_operation_result identities.
    @param next_task Explicit Plan11 dispatch after these completed operations.
    @param destination Explicit current review report destination.
    @param operation_validator Actual Plan11-owned source/before/output business validator.
    @param observer Optional explicit SpaceObserver; original semantics are the default.
    @return Typed operation-receipt foundation; publication grants no future paid scope.
    """
    from space_checkpoint_operation import publish_successor as publish
    return publish(predecessor, operations, next_task, destination, operation_validator, observer)


def _json(path):
    """Read one JSON document without creating a run or modifying its input.

    @param path Existing contained document.
    @return Decoded JSON value; parse errors remain visible to the caller.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def _reference(value, root):
    """Resolve only a declared durable identity, rejecting redirected input paths.

    @param value Exact path/SHA256 object, never an arbitrary historical string.
    @param root Explicit checkout root.
    @return Contained durable fixture path with its original spelling validated.
    """
    if not isinstance(value, dict) or set(value) != {"path", "sha256"}:
        raise ValueError("checkpoint reference requires exact path/sha256 fields")
    path, digest = value["path"], value["sha256"]
    # A checkpoint may depend only on transferred fixtures. A logs/build/review
    # reference would silently turn yesterday's workstation into a prerequisite.
    if not isinstance(path, str) or not path.startswith("src/tests/fixtures/") or "\\" in path:
        raise ValueError("checkpoint reference is outside durable fixtures")
    # Enforce full SHA256 identities before touching the selected file.
    if not isinstance(digest, str) or len(digest) != 64 or any(c not in "0123456789abcdef" for c in digest):
        raise ValueError("checkpoint reference has invalid SHA256")
    selected = checked_path(path, root)
    if selected.relative_to(root).as_posix() != path:
        raise ValueError("checkpoint reference requires a canonical fixture path")
    return selected


def _verified(value, root):
    """Verify an explicit live retained input before any semantic interpretation.

    @param value Declared durable identity.
    @param root Explicit checkout root.
    @return Existing file whose full bytes match the declaration.
    """
    path = _reference(value, root)
    if file_identity(path)[0] != value["sha256"]:
        raise ValueError(f"checkpoint input hash changed: {value['path']}")
    return path


def _schema(document, root):
    """Reject legacy receipts and incomplete or extended seal schemas before use.

    @param document Decoded manifest to validate.
    @param root Checkout used to validate all live reference spellings.
    @return None; malformed contracts fail before any observer is called.
    """
    if not isinstance(document, dict) or set(document) != FIELDS:
        raise ValueError("checkpoint requires its exact versioned fields")
    # Boolean True must not masquerade as schema version one; operation receipts
    # also cannot become checkpoints by merely adding a configuration field.
    if type(document["schema_version"]) is not int or document["schema_version"] != 1 or document["kind"] != "sealed_space_checkpoint":
        raise ValueError("unsupported checkpoint schema/kind")
    configurations = document["configurations"]
    if not isinstance(configurations, dict) or set(configurations) != set(CONFIGURATIONS):
        raise ValueError("checkpoint requires both independent configurations")
    # Configuration names and owner paths bind different original saves. A Debug
    # consumer cannot silently receive the release save through a shared alias.
    for configuration, row in configurations.items():
        if not isinstance(row, dict) or set(row) != {"save", "original", "expected_observation", "expected_chunks", "save_version"}:
            raise ValueError("invalid checkpoint configuration fields")
        for field in ("save", "expected_observation", "expected_chunks"):
            _reference(row[field], root)
        if row["save"]["path"] != f"{OWNER}/{configuration}/current.sav":
            raise ValueError("checkpoint save/configuration owner mismatch")
        if type(row["save_version"]) is not int or row["save_version"] != 440:
            raise ValueError("unexpected checkpoint save version")
    for field in ("historical_validation", "history", "migration_map"):
        _reference(document[field], root)
    # These original scopes are closed. Loading metadata never grants a new paid
    # window, relaxes either financial floor or qualifies a fresh application.
    if document["closed_scope"] != SCOPE or document["financial_policy"] != POLICY:
        raise ValueError("checkpoint closed scope/financial policy changed")
    # JSON booleans and integers are different policy fields even though Python
    # treats False == 0. Numeric substitutes cannot impersonate an ended scope.
    for field, expected in (("closed_scope", SCOPE), ("financial_policy", POLICY)):
        if any(type(document[field][key]) is not type(value) for key, value in expected.items()):
            raise ValueError("checkpoint policy field types changed")
    if document["next_task"] != "Plan11/15CSGZVEEI" or document["fresh_build_qualified"] is not False:
        raise ValueError("checkpoint cannot authorize or qualify new work")


def load_checkpoint(path=None, configuration=None, root=None):
    """Load the typed seal with no writes, runtime reservation or native imports.

    @param path Explicit published checkpoint.json; None selects the durable owner.
    @param configuration RelWithDebInfo/Debug, or None for both configurations.
    @param root Explicit source root for isolated tests; defaults to this checkout.
    @return SpaceCheckpoint bound to the exact manifest bytes just validated.
    """
    source = Path(root or ROOT).resolve()
    selected = checked_path(path or f"{OWNER}/checkpoint.json", source)
    owner = checked_path(OWNER, source)
    # Pending publications and legacy receipts cannot cross this typed boundary;
    # only the owner's final manifest name is a consumer-selectable checkpoint.
    if selected != owner / "checkpoint.json" or configuration not in (*CONFIGURATIONS, None):
        raise ValueError("select a published Space checkpoint and known configuration")
    digest = file_identity(selected)[0]
    _schema(_json(selected), source)
    if file_identity(selected)[0] != digest:
        raise ValueError("checkpoint changed during load")
    return SpaceCheckpoint(source, selected, digest, configuration)


def _verify_document(document, root, configuration=None):
    """Verify the complete original dependency closure and actual owner semantics.

    @param document Candidate or already published seal.
    @param root Source checkout containing its immutable inputs.
    @param configuration Optional explicit save selection; history remains complete.
    @return Compact successful verification facts; no native work or authorization.
    """
    _schema(document, root)
    history = _json(_verified(document["history"], root))
    audit = _json(_verified(document["historical_validation"], root))
    if history["validated_audit"] != document["historical_validation"]:
        raise ValueError("checkpoint history/audit roots disagree")
    # Historical verification must have observed the originals without replaying
    # financial scopes. Its success never stands in for fresh executable proof.
    if audit["kind"] != "validated_historical_space_facts" or audit["complete"] is not True or audit["fresh_application_ran"] is not False or audit["gameplay_ticks_executed"] != 0 or audit["child_process_attempts"] != 0:
        raise ValueError("invalid original historical validation")
    expected_roots = dict(original_bound=audit["original_root"], immediate_foundation=audit["immediate_foundation"], source_epochs=audit["source_epochs"])
    if document["historical_roots"] != expected_roots or len(audit["source_epochs"]) != 13:
        raise ValueError("checkpoint graph roots/source epochs disagree")
    live = history["live_retained_inputs"]
    originals = {}
    # This explicit list is the historical retained closure. Do not recursively
    # dereference old EXE/log strings inside its immutable witness documents.
    for witness in live:
        _verified(witness["retained"], root)
        original = witness["original"]
        if original["sha256"] != witness["retained"]["sha256"]:
            raise ValueError("historical witness does not preserve original bytes")
        key = (original["path"], original["sha256"])
        originals[key] = witness["retained"]
    # Each graph root needs an actual retained witness, including the thirteen
    # epochs. A recorded identity alone cannot conceal a missing required file.
    for original in [audit["original_root"], *audit["source_epochs"]]:
        if (original["path"], original["sha256"]) not in originals:
            raise ValueError("checkpoint graph root lacks retained witness")
    original_bound = _json(_verified(originals[(audit["original_root"]["path"], audit["original_root"]["sha256"])], root))
    # The prior foundation is an already validated historical identity, rather
    # than another executable/product tree to reconstruct during continuation.
    if original_bound["foundation"] != audit["immediate_foundation"] or original_bound["historical_versions"] != audit["source_epochs"]:
        raise ValueError("original bound graph/foundation linkage disagrees")
    for alias in history["source_witness_aliases"]:
        # Source epochs retain exact source bytes. They are explanatory fixtures,
        # never compiled inputs or permission to replace the current live source.
        if alias["retained"] not in originals.values() or alias["sha256"] != alias["retained"]["sha256"]:
            raise ValueError("source witness is outside the retained closure")
    if history["closed_scope"] != SCOPE or audit["closed_policy"]["capital_scope"] != SCOPE:
        raise ValueError("historical scope is not the closed original scope")
    for key, value in POLICY.items():
        if history[key] != value:
            raise ValueError("historical financial policy disagrees")
    # Both lower-floor decisions are spent. Their independent retained receipts
    # explain that closure without creating another authorization from metadata.
    approvals = history["consumed_lower_floor_approvals"]
    if {row["name"] for row in approvals} != {"reserve-bridge", "arrival-settlement"}:
        raise ValueError("missing closed lower-floor decisions")
    for approval in approvals:
        if approval["consumed"] is not True or approval["further_scope_authorized"] is not False or approval["closed_scope"]["ended"] is not True:
            raise ValueError("lower-floor approval reopened")
    migration = _json(_verified(document["migration_map"], root))
    if migration["kind"] != "space_checkpoint_migration" or migration["schema_version"] != 1:
        raise ValueError("invalid checkpoint migration map")
    expected_aliases = []
    verified = []
    # Verify both physical originals even for a selected build, so a transfer
    # cannot silently omit the other configuration's independent continuation.
    for name in CONFIGURATIONS:
        row = document["configurations"][name]
        old, = [item for item in audit["configurations"] if item["configuration"] == name]
        bound, = [item for item in original_bound["configurations"] if item["configuration"] == name]
        if row["original"] != old["output"] or row["save"]["sha256"] != old["output"]["sha256"] or bound["output"] != old["output"]:
            raise ValueError("checkpoint save differs from original graph root")
        save = _verified(row["save"], root)
        expectation = _json(_verified(row["expected_observation"], root))
        expected_chunks = _json(_verified(row["expected_chunks"], root))
        # The golden observation comes from the independent original receipt;
        # the selected observer is compared to it, never used to write it.
        if expectation["configuration"] != name or expectation["source_receipt"] != old["receipt"] or expectation["observation"] != old["observation"] or bound["observation"] != old["observation"]:
            raise ValueError("independent checkpoint observation disagrees")
        if expected_chunks["configuration"] != name or expected_chunks["source_audit"] != document["historical_validation"] or expected_chunks["chunk_sha256"] != old["chunk_sha256"]:
            raise ValueError("independent checkpoint chunk expectation disagrees")
        version, families = chunks(save)
        actual_chunks = {key.decode("ascii"): hashlib.sha256(value).hexdigest() for key, value in families.items()}
        if version != row["save_version"] or len(families) != 121 or actual_chunks != expected_chunks["chunk_sha256"]:
            raise ValueError("checkpoint actual chunk families changed")
        actual = json.loads(json.dumps(observe(save)))
        if actual != expectation["observation"]:
            raise ValueError("checkpoint actual owner/material observation changed")
        receipt_key = (old["receipt"]["path"], old["receipt"]["sha256"])
        if receipt_key not in originals:
            raise ValueError("missing independent final operation receipt")
        receipt = _json(_verified(originals[receipt_key], root))
        if receipt["operation"]["final"] != actual:
            raise ValueError("checkpoint differs from original final operation")
        expected_aliases.append(dict(original=row["original"], retained=row["save"]))
        verified.append(dict(configuration=name, save=row["save"], save_version=version, chunk_families=len(families)))
    if migration["aliases"] != expected_aliases:
        raise ValueError("checkpoint migration aliases disagree")
    references = [document[field] for field in ("history", "historical_validation", "migration_map")]
    references.extend(witness["retained"] for witness in live)
    references.extend(row[field] for row in document["configurations"].values()
                      for field in ("save", "expected_observation", "expected_chunks"))
    # Rehash every physical authority at close. A writer changing inputs during
    # observation must not leave a successful seal based on mixed source epochs.
    for reference in references:
        _verified(reference, root)
    return dict(kind="verified_space_checkpoint", complete=True, configurations=verified,
                selected_configuration=configuration, live_history_witnesses=len(live),
                gameplay_ticks_executed=0, fresh_build_qualified=False, next_task=document["next_task"])


def verify_checkpoint(checkpoint):
    """Verify the exact loaded seal read-only against retained facts and save owners.

    @param checkpoint Typed selection returned by load_checkpoint, never a dict receipt.
    @return Compact verification result; fresh-build qualification remains pending.
    """
    if not isinstance(checkpoint, SpaceCheckpoint):
        raise TypeError("verify_checkpoint requires a typed SpaceCheckpoint")
    if file_identity(checkpoint.manifest_path)[0] != checkpoint.sha256:
        raise ValueError("checkpoint manifest changed since load")
    result = _verify_document(_json(checkpoint.manifest_path), checkpoint.root, checkpoint.configuration)
    if file_identity(checkpoint.manifest_path)[0] != checkpoint.sha256:
        raise ValueError("checkpoint changed during verification")
    return result


def _encoded(document):
    """Encode a deterministic documented contract for immutable publication.

    @param document JSON-compatible contract with explicit owner/provenance.
    @return UTF-8 bytes with a final newline.
    """
    return (json.dumps(document, indent=2) + "\n").encode("utf-8")


def _publish_bytes(path, payload, root):
    """Atomically publish an immutable payload, refusing conflicting prior bytes.

    @param path Contained final destination within the checkout.
    @param payload Exact bytes copied or encoded from independent retained facts.
    @param root Explicit checkout used for containment and reparse checks.
    @return Exact destination identity, after full-byte verification.
    """
    selected = checked_path(path, root)
    digest = hashlib.sha256(payload).hexdigest()
    if selected.exists():
        if file_identity(selected)[0] != digest:
            raise ValueError(f"refuse conflicting retained payload: {selected}")
    else:
        selected.parent.mkdir(parents=True, exist_ok=True)
        pending = checked_path(selected.with_name(selected.name + ".pending"), root)
        # A prior interruption cannot be mistaken for a published seal. Reuse
        # only the identical pending bytes; refuse an unrelated writer's payload.
        if pending.exists() and pending.read_bytes() != payload:
            raise ValueError(f"conflicting interrupted publication: {pending}")
        with pending.open("wb") as sink:
            sink.write(payload)
            sink.flush()
            os.fsync(sink.fileno())
        os.replace(pending, selected)
    if file_identity(selected)[0] != digest:
        raise ValueError("published checkpoint payload changed")
    return dict(path=selected.relative_to(root).as_posix(), sha256=digest)


def seal_original_checkpoint(root=None):
    """Publish the two original GZVEEH saves only after complete seal verification.

    @param root Explicit source checkout; defaults to the executing tool's checkout.
    @return Verified typed checkpoint; originals remain untouched in their old owners.
    """
    source = Path(root or ROOT).resolve()
    history_path = checked_path("src/tests/fixtures/cleanup/space-history-facts.json", source)
    history_reference = dict(path=history_path.relative_to(source).as_posix(), sha256=file_identity(history_path)[0])
    history = _json(history_path)
    audit_reference = history["validated_audit"]
    audit = _json(_verified(audit_reference, source))
    configurations = {}
    aliases = []
    # Original identities are verified before each immutable copy. Expectations
    # are copied from the earlier audit, never obtained from the current reader.
    for name in CONFIGURATIONS:
        old, = [item for item in audit["configurations"] if item["configuration"] == name]
        original = checked_path(old["output"]["path"], source)
        payload = original.read_bytes()
        if hashlib.sha256(payload).hexdigest() != old["output"]["sha256"]:
            raise ValueError("original Space save changed before sealing")
        home = source / OWNER / name
        save = _publish_bytes(home / "current.sav", payload, source)
        expected = _publish_bytes(home / "expected-observation.json", _encoded(dict(
            _documentation="Owner: r2fc-file-cleanup3.md8; independent original final receipt observation.",
            configuration=name, source_receipt=old["receipt"], observation=old["observation"])), source)
        expected_chunks = _publish_bytes(home / "expected-chunks.json", _encoded(dict(
            _documentation="Owner: r2fc-file-cleanup3.md8; independently measured Group7 original chunk identities.",
            configuration=name, source_audit=audit_reference, chunk_sha256=old["chunk_sha256"])), source)
        configurations[name] = dict(save=save, original=old["output"], expected_observation=expected,
                                    expected_chunks=expected_chunks, save_version=old["save_version"])
        aliases.append(dict(original=old["output"], retained=save))
    migration = dict(_documentation="Owner: r2fc-file-cleanup3.md8; exact original-save aliases; no deletion permission.",
                     schema_version=1, kind="space_checkpoint_migration", aliases=aliases)
    migration_bytes = _encoded(migration)
    migration_hash = hashlib.sha256(migration_bytes).hexdigest()
    migration_reference = _publish_bytes(source / OWNER / f"migration-{migration_hash}.json", migration_bytes, source)
    document = dict(_documentation="Owner: r2fc-file-cleanup3.md8; sealed original history, fresh application qualification pending.",
        schema_version=1, kind="sealed_space_checkpoint", historical_validation=audit_reference, history=history_reference,
        historical_roots=dict(original_bound=audit["original_root"], immediate_foundation=audit["immediate_foundation"], source_epochs=audit["source_epochs"]),
        configurations=configurations, closed_scope=SCOPE, financial_policy=POLICY,
        next_task=history["next_task"], fresh_build_qualified=False, migration_map=migration_reference)
    _verify_document(document, source)
    # The content-addressed map is durable first. One final atomic manifest
    # replacement commits the already verified save/map/history relationship.
    _publish_bytes(source / OWNER / "checkpoint.json", _encoded(document), source)
    checkpoint = load_checkpoint(root=source)
    verify_checkpoint(checkpoint)
    return checkpoint


def main():
    """Verify a published seal, or explicitly create it from retained original facts.

    @return None; the default invocation performs no writes or native execution.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoint", type=Path)
    parser.add_argument("--configuration", choices=CONFIGURATIONS)
    parser.add_argument("--seal-original", action="store_true")
    args = parser.parse_args()
    if args.seal_original:
        if args.checkpoint is not None:
            parser.error("--seal-original owns its exact original checkpoint destination")
        checkpoint = seal_original_checkpoint()
    else:
        checkpoint = load_checkpoint(args.checkpoint, args.configuration)
    print(json.dumps(verify_checkpoint(checkpoint), indent=2))


if __name__ == "__main__":
    main()
