"""Incrementally preserve saves before a generated owner may be removed.

@file save_retention.py Read-only planning and explicit, verified save export.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group24.
@see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
The Group9 migration remains historical authority. This supplementary journal
records subsequent save versions without replacing its frozen logical inputs.
Payload publication precedes the atomic journal commit; interruption authorizes
no deletion and can leave only an exact, reusable retained copy.
"""

import argparse
import copy
import json
import os
from pathlib import Path
import shutil
import tempfile

from artifact_inventory import DISPOSABLE, checked_path, file_identity, in_disposable, save_format, walk_files
from artifact_paths import repository_root
from artifact_preservation import publish_document

MIGRATION = "src/tests/fixtures/cleanup/migration.json"
JOURNAL = "src/tests/fixtures/cleanup/save-exports.json"


def _document(path, root):
    """Read an optional preservation contract and its exact current identity.

    @param path Repository-relative migration/journal location.
    @param root Explicit contained checkout or disposable test fixture.
    @return Pair of decoded document and SHA256, or an empty document and None.
    """
    target = checked_path(path, root)
    if not target.exists():
        return {}, None
    before = file_identity(target)[0]
    document = json.loads(target.read_text(encoding="utf-8"))
    if file_identity(target)[0] != before:
        raise ValueError(f"preservation record changed while reading: {path}")
    return document, before


def _observe(root, scopes, configurations=False):
    """Identify every save-like file, including renamed and undecodable saves.

    @param root Explicit checkout whose ownership edges must remain ordinary.
    @param scopes Exact requested files/directories; absent scopes remain watched.
    @param configurations Also protect native .cfg and process startup .scr files.
    @return Sorted original path, hash, length and container observations.
    """
    existing = [scope for scope in scopes if checked_path(scope, root).exists()]
    observations = []
    # Inspect every header rather than just extensions. A native title or a
    # renamed old save receives the same retention guarantee as current.sav.
    for path in walk_files(root, existing):
        detected = save_format(path)
        # Whole-owner cleanup must retain meaningful native settings as well as
        # saves. A save header wins even when its filename happens to end in cfg.
        if detected is None and not (configurations and path.suffix.lower() in {".cfg", ".scr"}):
            continue
        digest, size = file_identity(path)
        observations.append(dict(path=path.relative_to(root).as_posix(),
                                 sha256=digest, bytes=size, format=detected))
    return sorted(observations, key=lambda row: row["path"])


def _destination(original, digest, root, configuration=False):
    """Select a stable Group9 save owner without replacing different bytes.

    @param original Exact original spelling retained in the journal aliases.
    @param digest Full source SHA256; shortened directory collisions still refuse.
    @param root Checkout determining ordinary Windows filename length limits.
    @param configuration Select Group9's durable native-configuration owner.
    @return Relative retained destination, checked for existing content conflicts.
    """
    owner = "src/tests/fixtures/config/multilayer/retained" if configuration else "savegames/retained"
    relative = owner + "/" + original
    # Deep paths and mutable same-name saves need a distinct bounded identity;
    # full original names remain in aliases even when their payload name is short.
    if len(str(root / relative)) > 235:
        relative = f"{owner}/by-hash/{digest[:16]}/{Path(original).name[:80]}"
    target = checked_path(relative, root)
    # Existing bytes are immutable. A changed original gets another hash owner,
    # while a conflict inside that explicit hash owner is never overwritten.
    if target.exists() and file_identity(target)[0] != digest:
        relative = f"{owner}/by-hash/{digest[:16]}/{Path(original).name[:80]}"
        target = checked_path(relative, root)
        if target.exists() and file_identity(target)[0] != digest:
            raise ValueError(f"conflicting retained save destination: {relative}")
    return relative


def _check_retained_owner(destination, owners, root):
    """Keep the retained copy outside every generated or selected deletion owner.

    @param destination Explicit proposed durable payload path.
    @param owners Canonical watched paths freshly validated once at this boundary.
    @param root Checkout whose redirect/containment checks still apply.
    @return None; a copy within the deletion set cannot preserve its source.
    """
    target = checked_path(destination, root)
    relative = target.relative_to(root).as_posix()
    # Both the global generated boundary and this invocation's exact scopes
    # matter: an edited record cannot certify a copy that cleanup will remove.
    if in_disposable(relative) or relative.startswith(".git/"):
        raise ValueError("retained save destination is not a durable owner")
    # Scope ancestry is shared by every payload in this boundary. Re-resolving
    # the same scopes for thousands of saves caused the real planning slowdown;
    # callers revalidate all owners at each new plan/apply/verification boundary.
    for owner in owners:
        if target == owner or target.is_relative_to(owner):
            raise ValueError("retained save destination is inside the deletion scope")


def plan_save_exports(root=None, scopes=DISPOSABLE, configurations=False):
    """Plan exact save copies and historical aliases without filesystem writes.

    @param root Optional checkout root; explicit roots also support isolated tests.
    @param scopes Exact owners to inspect and watch, including currently absent ones.
    @param configurations Also export native configs when retiring complete owners.
    @return JSON-compatible plan bound to current saves and preservation records.
    """
    root = Path(root).resolve() if root is not None else repository_root()
    scopes = sorted(set(checked_path(scope, root).relative_to(root).as_posix() for scope in scopes))
    migration, migration_hash = _document(MIGRATION, root)
    journal, journal_hash = _document(JOURNAL, root)
    if journal and journal.get("kind") != "incremental_save_exports":
        raise ValueError("unexpected save export journal kind")
    known = {}
    # Group9's verified durable payloads can supply subsequent equal-byte aliases.
    # The supplemental journal keeps all later versions without rebinding those
    # original frozen catalogue keys to the newest mutable save.
    for row in [*migration.get("payloads", []), *journal.get("payloads", [])]:
        known.setdefault(row["sha256"], row["destination"])
    observations = _observe(root, scopes, configurations)
    owners = tuple(checked_path(scope, root) for scope in scopes)
    exports = {}
    for observed in observations:
        digest = observed["sha256"]
        if digest not in exports:
            destination = known.get(digest)
            if destination is None:
                destination = _destination(observed["path"], digest, root, observed["format"] is None)
            else:
                if file_identity(checked_path(destination, root))[0] != digest:
                    raise ValueError(f"known retained save changed: {destination}")
            _check_retained_owner(destination, owners, root)
            exports[digest] = dict(sha256=digest, bytes=observed["bytes"],
                format=observed["format"], destination=destination, original_paths=[])
        exports[digest]["original_paths"].append(observed["path"])
    return dict(_documentation="Owner: r2fc2-file-cleanup3.md24; save export proposal, never deletion permission.",
        schema_version=1, kind="save_export_plan", scopes=scopes, configurations=configurations, observations=observations,
        metadata={MIGRATION: migration_hash, JOURNAL: journal_hash}, payloads=list(exports.values()))


def _check_sources(plan, root):
    """Refuse changed, missing or newly created save candidates in watched scopes.

    @param plan Original read-only proposal, including absent watched owners.
    @param root Checkout to rescan immediately before publication or deletion.
    @return None; exact observation equality is required.
    """
    if plan.get("kind") != "save_export_plan":
        raise ValueError("expected an explicit save export plan")
    # A prior planning pass cannot certify a newly redirected deletion owner.
    # Validate the scopes again here, once for this complete boundary check.
    owners = tuple(checked_path(scope, root) for scope in plan["scopes"])
    for row in plan["payloads"]:
        _check_retained_owner(row["destination"], owners, root)
    if _observe(root, plan["scopes"], plan.get("configurations", False)) != plan["observations"]:
        raise ValueError("save candidates changed; replan and export before deletion")


def _copy_payload(row, root):
    """Copy one exact payload and atomically admit it without overwriting a peer.

    @param row Planned payload with source aliases, retained destination and SHA256.
    @param root Contained checkout; source saves are never edited or removed.
    @return None; partial temporary copies are removed and never become authority.
    """
    source = checked_path(row["original_paths"][0], root)
    target = checked_path(row["destination"], root)
    expected = row["sha256"]
    if file_identity(source)[0] != expected:
        raise ValueError("source save changed before export")
    if target.exists():
        if file_identity(target)[0] != expected:
            raise ValueError("retained save destination changed")
        return
    target.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=".export-", suffix=".pending", dir=target.parent)
    os.close(descriptor)
    pending = checked_path(temporary, root)
    try:
        shutil.copyfile(source, pending)
        with pending.open("r+b") as stream:
            os.fsync(stream.fileno())
        if file_identity(pending)[0] != expected:
            raise ValueError("truncated or changed save export copy")
        if file_identity(source)[0] != expected:
            raise ValueError("source save changed while exporting")
        try:
            # Linking the finished temporary file is atomic and refuses an
            # existing destination on both Windows and POSIX. Removing the
            # temporary name leaves one ordinary retained file, no shared home.
            os.link(pending, target)
        except FileExistsError:
            if file_identity(target)[0] != expected:
                raise ValueError("concurrent retained save destination conflict")
        if file_identity(target)[0] != expected:
            raise ValueError("published retained save changed")
    finally:
        pending.unlink(missing_ok=True)


def apply_save_exports(plan, root=None):
    """Publish verified copies and then atomically commit their migration record.

    @param plan Exact plan_save_exports result; stale plans fail without authority.
    @param root Optional checkout, also permitting disposable test fixtures.
    @return Verified current save/payload totals and the journal's exact identity.
    """
    root = Path(root).resolve() if root is not None else repository_root()
    _check_sources(plan, root)
    # Reject concurrent preservation edits rather than overwriting another
    # invocation's aliases. A crash before commit leaves reusable payloads only.
    for path, expected in plan["metadata"].items():
        if _document(path, root)[1] != expected:
            raise ValueError(f"preservation record changed; replan: {path}")
    journal, _ = _document(JOURNAL, root)
    journal = copy.deepcopy(journal) if journal else dict(
        _documentation="Owner: r2fc2-file-cleanup3.md24; all save versions and historical original aliases; no decoder or gameplay authority.",
        schema_version=1, kind="incremental_save_exports", payloads=[], aliases={})
    by_hash = {row["sha256"]: row for row in journal["payloads"]}
    # Every copied payload verifies independently. No partial set is committed;
    # all observed aliases and newly appeared candidates are checked again below.
    for row in plan["payloads"]:
        _copy_payload(row, root)
        if row["sha256"] not in by_hash:
            by_hash[row["sha256"]] = copy.deepcopy(row)
        else:
            old = by_hash[row["sha256"]]
            if old["destination"] != row["destination"]:
                raise ValueError("save journal destination changed")
            old["original_paths"] = sorted(set(old["original_paths"] + row["original_paths"]))
        reference = dict(path=row["destination"], sha256=row["sha256"])
        for original in row["original_paths"]:
            journal["aliases"][original] = reference
    journal["payloads"] = list(by_hash.values())
    _check_sources(plan, root)
    for path, expected in plan["metadata"].items():
        if _document(path, root)[1] != expected:
            raise ValueError(f"preservation record changed before commit: {path}")
    # Recheck the exact retained bytes just before the commit marker. Historical
    # journal aliases may refer to earlier versions of a mutable original; only
    # the plan's current observations authorize this invocation's deletion scope.
    for row in plan["payloads"]:
        if file_identity(checked_path(row["destination"], root))[0] != row["sha256"]:
            raise ValueError("retained save changed before journal commit")
    publish_document(JOURNAL, journal, root)
    return verify_save_exports(plan, root)


def verify_save_exports(plan, root=None):
    """Recheck exact candidates and committed retained bytes before owner deletion.

    @param plan Exact invocation proposal, never a historical latest-save guess.
    @param root Optional checkout whose writer guards are owned by the cleaner.
    @return Journal identity and verified current alias/payload/byte totals.
    """
    root = Path(root).resolve() if root is not None else repository_root()
    _check_sources(plan, root)
    journal, digest = _document(JOURNAL, root)
    aliases = journal.get("aliases", {})
    for row in plan["payloads"]:
        reference = dict(path=row["destination"], sha256=row["sha256"])
        if file_identity(checked_path(row["destination"], root))[0] != row["sha256"]:
            raise ValueError("retained save changed before deletion")
        for original in row["original_paths"]:
            if aliases.get(original) != reference:
                raise ValueError(f"save export has no committed alias: {original}")
    if digest is None:
        raise ValueError("save export journal is not committed")
    return dict(verified=True, journal=dict(path=JOURNAL, sha256=digest),
                save_aliases=sum(row["format"] is not None for row in plan["observations"]),
                configuration_aliases=sum(row["format"] is None for row in plan["observations"]), payloads=len(plan["payloads"]),
                retained_bytes=sum(row["bytes"] for row in plan["payloads"]))


def main():
    """Print a read-only JSON proposal or explicitly apply verified save export.

    @return None; this command never deletes sources or creates native runtimes.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--scope", action="append")
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    plan = plan_save_exports(args.root, args.scope if args.scope is not None else DISPOSABLE)
    result = apply_save_exports(plan, args.root) if args.apply else plan
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
