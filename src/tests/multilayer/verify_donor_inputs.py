"""Verify retained donor originals without accessing the donor or running a game.

@file verify_donor_inputs.py Exact imported bytes and namespaced fixture aliases.
@see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group 2.
@see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
Historical source and receipt copies retain their original bytes. Their import
provenance belongs to donor-import.json, not to edits of their old file headers.
"""

import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent / "support"))
from artifact_paths import contained, repository_root, resolve_input_path

CONTRACT = "src/tests/fixtures/cleanup/donor-import.json"
SCOPES = {
    "saves": {"save", "configuration"},
    "sources": {"source_epoch"},
    "assets": {"asset_original", "fixture_original"},
    "history": {"historical_receipt"},
}
OWNERS = {
    "save": ("src/tests/fixtures/", "savegames/retained/", "regression/"),
    "configuration": ("src/tests/fixtures/config/",),
    "source_epoch": ("src/tests/fixtures/cleanup/source-epochs/",),
    "asset_original": ("media/",),
    "fixture_original": ("src/tests/fixtures/",),
    "historical_receipt": ("src/tests/fixtures/checkpoints/underground/",),
}


def verify_retained_inputs(root=None, scope="all", document=None):
    """Check one retained-input scope through A's existing ownership boundaries.

    @param root Explicit checkout, or the repository containing this tool.
    @param scope all, saves, sources, assets or history; permits bounded bursts.
    @param document Optional in-memory preservation contract for focused tests.
    @return Verified payload, alias and byte counts; no current gameplay claim.
    @throws ValueError For unknown roles, escaping paths, changed bytes or aliases.
    """
    root = repository_root(root)
    if document is None:
        document = json.loads((root / CONTRACT).read_text(encoding="utf-8"))
    # This reader accepts a preservation manifest, not a gameplay checkpoint or
    # an old native receipt whose hashes could imply unearned current acceptance.
    if document.get("schema_version") != 1 or document.get("kind") != "donor_input_preservation":
        raise ValueError("unsupported donor preservation contract")
    if scope not in {"all", *SCOPES}:
        raise ValueError("unknown donor verification scope")
    namespace = document["namespace"]
    if namespace != "donor/openttd3-75ea75718f/":
        raise ValueError("unexpected donor alias namespace")
    catalogue = json.loads((root / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))["inputs"]
    counts = dict(scope=scope, payloads=0, aliases=0, bytes=0)
    destinations = {}
    # Every unique retained payload is hashed once per selected scope. Validate
    # every alias mapping, then exercise one alias through the public resolver;
    # duplicate historical paths must not multiply expensive physical reads.
    for row in document["payloads"]:
        role = row["role"]
        if role not in OWNERS:
            raise ValueError("unknown retained role: " + role)
        # Scope splitting limits each check to its actual ownership area while
        # preserving the same validation rules as the aggregate invocation.
        if scope != "all" and role not in SCOPES[scope]:
            continue
        relative = row["destination"]
        path = contained(root / relative, root)
        if not relative.startswith(OWNERS[role]):
            raise ValueError("retained input has the wrong owner: " + relative)
        if not path.is_file():
            raise ValueError("retained input is missing: " + relative)
        before = path.stat()
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        after = path.stat()
        # An unchanged filename is insufficient: readers must reject concurrent
        # mutation and partial copies before accepting their original identity.
        if digest != row["sha256"] or after.st_size != row["bytes"] or (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
            raise ValueError("retained input changed: " + relative)
        if relative in destinations:
            if destinations[relative] != digest:
                raise ValueError("conflicting retained destination: " + relative)
        destinations[relative] = digest
        originals = row["original_paths"]
        if not originals:
            raise ValueError("retained input lacks original aliases")
        if role != "asset_original":
            reference = dict(path=relative, sha256=digest)
            for original in originals:
                if catalogue.get(namespace + original) != reference:
                    raise ValueError("retained donor alias changed: " + original)
            if resolve_input_path(namespace + originals[0], root) != path:
                raise ValueError("public resolver selected a different retained input")
        counts["payloads"] += 1
        counts["aliases"] += len(originals)
        counts["bytes"] += row["bytes"]
    return counts


def execute_stage(request, root):
    """Verify retained donor inputs through the existing selected recipe dispatcher.

    @param request Exact existing donor-underground-inputs prepare/verify request and output role.
    @param root Owning Acheckout; no Bdirectory, old executable or old cache is accessed.
    @return Actual preservation counts/checks, never current gameplay or art acceptance.
    """
    root = repository_root(root)
    reference = request["inputs"]["donor-input-contract"]
    source = contained(root / reference["path"], root / "src/tests/fixtures/cleanup")
    payload = source.read_bytes()
    if hashlib.sha256(payload).hexdigest() != reference["sha256"]:
        raise ValueError("donor input recipe preservation contract identity changed")
    document = json.loads(payload)
    # A preserved original contract grants read-only verification only. A
    # changed phase/namespace cannot turn this recipe into a donor importer,
    # source publisher or paid-world bootstrap.
    if (request["paid_work_authorized"] is not False or request["publishes_source"] is not False
            or document.get("kind") != "donor_input_preservation" or document.get("schema_version") != 1
            or document.get("donor_commit") != "75ea75718fa7d14632bf795105721df1e390f753"):
        raise ValueError("donor input recipe requires its exact preserved read-only contract")
    if request["stage"] == "prepare":
        return dict(checks=["exact retained donor preservation contract supplied; no donor process or cache invoked"])
    if request["stage"] != "verify":
        raise ValueError("donor input recipe requires an explicit prepare/verify stage")
    scope = request["declaration"]["original_parameters"]["scope"]
    observed = verify_retained_inputs(root, scope=scope, document=document)
    from artifact_paths import RunContext
    context = RunContext(root, request["feature"], request["scenario"], request["run_id"])
    output = contained(root / request["outputs"]["manifest"], context.reports)
    result = dict(_documentation=__doc__, schema_version=1, kind="retained_donor_input_verification", run_id=context.run_id,
                  source_contract=reference, observed=observed, historical_receipts_unchanged=True,
                  current_native_acceptance=False, publishes_source=False)
    with output.open("x", encoding="utf-8") as sink:
        sink.write(json.dumps(result, indent=2) + "\n")
    return dict(checks=["all selected donor payload/alias/hash/owner identities verified without Bdirectories or old caches"], observed=observed)


def main():
    """Run one bounded read-only verification and optionally record its result.

    @return None; optional output is confined to test-cache or a review report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scope", choices=["all", *SCOPES], default="all")
    parser.add_argument("--output", help="Optional repository-relative verification report.")
    args = parser.parse_args()
    result = verify_retained_inputs(scope=args.scope)
    if args.output:
        root = repository_root()
        output = contained(root / args.output, root)
        # Verification can report into disposable owners only. It cannot rewrite
        # the durable manifest or promote an observed file to a new expectation.
        if not output.is_relative_to(root / "test-cache") and not output.is_relative_to(root / "mlm/reviews"):
            raise ValueError("verification report needs a generated owner")
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(dict(_documentation="m2o23-1-come-space.md Group 2; exact input verification only.", **result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
