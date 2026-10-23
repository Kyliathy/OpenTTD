"""Read-only inventory of transfer inputs; importing this module creates no run.

@file artifact_inventory.py Typed migration boundary, never a disposal authority.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups2-6 and24.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group28 bounded ownership scanning.
@see docs/zz-reach2/architecture/savegame/archi-savegame.md.
"""

import argparse
from dataclasses import asdict, dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import stat

from artifact_paths import contained, owned_root, repository_root

SAVE_TAGS = {b"OTTD": "lzo", b"OTTN": "none", b"OTTZ": "zlib", b"OTTX": "lzma"}
SAVE_NAME = re.compile(r"(?:\.(?:sav|scn|sv[0-9]|ss[0-9]|sc[0-9]|bak|backup)(?:$|[.~_-])|autosave)", re.I)
DISPOSABLE = ("logs", "test-cache", "mlm/runtime", "mlm/reviews", "build")
ROLES = {"save", "unknown_save", "continuation_save", "configuration", "source_baseline",
         "historical_provenance", "original_media", "source", "generated", "unclassified"}
DISPOSITIONS = {"retain", "export", "regenerate", "review"}


@dataclass(frozen=True)
class MigrationRow:
    """One observed input and its explicit proposed transfer treatment.

    @param original_path Exact original repository-relative spelling, including aliases.
    @param destination Proposed durable path; None means classification remains open.
    @param sha256 Expected identity, or actual identity when first inventoried.
    @param role Semantic input category, independent of ignore/tracking status.
    @param consumers Named readers or preservation obligations requiring these bytes.
    @param disposition Retain, export, regenerate or review; never deletion permission.
    @param verification Present, missing, changed or alias-verified observation.
    @param actual_sha256 Actual observed identity, None when the input is missing.
    @param bytes Observed file length, None when missing.
    @param format Detected save container or unsupported/unknown save-like content.
    @param generator Explicit recipe for reproducible material; None for originals.
    """

    original_path: str
    destination: str | None
    sha256: str | None
    role: str
    consumers: tuple[str, ...]
    disposition: str
    verification: str
    actual_sha256: str | None
    bytes: int | None
    format: str | None = None
    generator: str | None = None

    def as_dict(self):
        """Return the JSON-compatible typed row without changing its identity.

        @param self Immutable observed migration row.
        @return Dictionary with explicit schema field names.
        """
        return asdict(self)


def checked_path(path, root):
    """Reuse shared containment and refuse every existing redirected component.

    @param path Relative or absolute input/output candidate beneath the checkout.
    @param root Canonical source root selected by the caller.
    @return Canonical contained path without junctions or symbolic links.
    @throws ValueError For traversal, reparse points or paths outside the checkout.
    """
    candidate = Path(path)
    if not candidate.is_absolute():
        candidate = root / candidate
    result = contained(candidate, root)
    # Even an internal junction can change ownership between inventory and export;
    # inspect the original spelling rather than only the resolved destination.
    for component in (candidate, *candidate.parents):
        if component.exists():
            attributes = getattr(component.lstat(), "st_file_attributes", 0)
            # Redirected data cannot be certified for later destructive cleanup.
            if component.is_symlink() or attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
                raise ValueError(f"redirected inventory path: {component}")
        if component == root:
            break
    return result


def file_identity(path):
    """Stream a full identity and reject files changing during observation.

    @param path Already-contained existing regular file.
    @return SHA256 and byte length of the unchanged observed file.
    @throws ValueError If metadata changes while the bytes are read.
    """
    before = path.stat()
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        # Large saved games and historical binaries must not require whole-file copies.
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    after = path.stat()
    if (before.st_size, before.st_mtime_ns, before.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
        raise ValueError(f"input changed during inventory: {path}")
    return digest.hexdigest(), after.st_size


def save_format(path):
    """Recognize native container signatures and protect undecodable save-like names.

    Tags match SaveLoadFormatTag in saveload.cpp and read_stream in the existing
    savegame_plane_digest.py. LZO and unknown formats require no Python decoding.
    @param path Existing regular file; only the first four bytes are read.
    @return Container label, unknown_save for suspect names/tags, or None.
    """
    with path.open("rb") as stream:
        tag = stream.read(4)
    if tag in SAVE_TAGS:
        return SAVE_TAGS[tag]
    # An old or future container, backup or autosave remains protected even if
    # Python cannot decode it; cleanup must never use decoding as a retention gate.
    if tag.startswith(b"OTT") or SAVE_NAME.search(path.name):
        return "unknown_save"
    return None


def in_disposable(path):
    """Identify known generated owners without inferring that their files are disposable.

    @param path Repository-relative POSIX spelling.
    @return Whether the input currently occupies an explicitly named cleanup owner.
    """
    return any(path == owner or path.startswith(owner + "/") for owner in DISPOSABLE)


def retention_destination(original, role, detected):
    """Choose an existing durable owner for an explicitly protected input.

    @param original Repository-relative original path, including its filename.
    @param role Typed retention category.
    @param detected Save container label, or None for non-save material.
    @return Proposed destination preserving the original relative name.
    """
    if not in_disposable(original):
        return original
    if detected:
        return "savegames/retained/" + original
    if role == "configuration":
        return "src/tests/fixtures/config/multilayer/retained/" + original
    return "src/tests/fixtures/cleanup/retained/" + original


def classify_reference(reference, root=None, role=None, consumers=(), destination=None, aliases=None, identity_reader=None):
    """Observe one typed reference without importing a legacy acceptance module.

    @param reference Path string or object containing path and optional expected sha256.
    @param root Optional explicit checkout root, including isolated test checkouts.
    @param role Explicit semantic role, or None to infer only known filesystem roles.
    @param consumers Explicit reader names; retention alone does not certify all consumers.
    @param destination Proposed durable path; None selects a retention destination.
    @param aliases Optional existing input-manifest entries, checked only if original is absent.
    @param identity_reader Optional invocation-local unchanged-file identity cache.
    @return MigrationRow with honest missing/changed/current/alias verification status.
    @throws ValueError For invalid roles, conflicting aliases or redirected paths.
    """
    root = repository_root(root)
    reference = {"path": str(reference)} if isinstance(reference, (str, Path)) else reference
    path = checked_path(reference["path"], root)
    original = path.relative_to(root).as_posix()
    expected = reference.get("sha256")
    selected_alias = None
    # Preserve the resolver's existing-path precedence while checking the alias's
    # own expected hash independently; alias metadata never blesses changed bytes.
    if not path.is_file() and aliases is not None:
        selected_alias = aliases.get(original)
        if selected_alias is not None:
            path = checked_path(selected_alias["path"], root)
            # Two authorities for one logical key must agree before bytes are used.
            if expected is not None and expected != selected_alias["sha256"]:
                raise ValueError(f"conflicting alias identity: {original}")
            expected = selected_alias["sha256"]
    actual, length, detected = None, None, None
    status = "missing"
    if path.is_file():
        actual, length = (identity_reader or file_identity)(path)
        detected = save_format(path)
        status = "alias_verified" if selected_alias else "present"
        # Changed content is reported rather than silently replacing an expected hash.
        if expected is not None and actual != expected:
            status = "changed"
    if role is None:
        role = "unclassified"
        if detected:
            role = "unknown_save" if detected == "unknown_save" else "save"
        elif path.suffix.lower() == ".cfg":
            role = "configuration"
        elif original.startswith("OpenTTD-baseline/"):
            role = "source_baseline"
        elif original.startswith("media/"):
            role = "original_media"
        elif original.startswith(("src/", "cmake/")):
            role = "source"
    if role not in ROLES:
        raise ValueError(f"unknown migration role: {role}")
    disposition = "review"
    # Save/config/original ownership is independent of tracking and ignore rules;
    # generated location only chooses export versus keeping its durable owner.
    if role in {"save", "unknown_save", "continuation_save", "configuration", "source_baseline", "original_media", "source", "historical_provenance"}:
        disposition = "export" if in_disposable(original) else "retain"
        if destination is None:
            destination = retention_destination(original, role, detected)
    if destination is not None:
        destination = checked_path(destination, root).relative_to(root).as_posix()
    return MigrationRow(original, destination, expected or actual, role, tuple(consumers),
        disposition, status, actual, length, detected)


def walk_files(root, roots):
    """Enumerate explicit owners without crossing Git or redirected-directory boundaries.

    @param root Canonical checkout root.
    @param roots Selected relative files/directories; dot selects the checkout.
    @return Iterator of unique regular files, with no filesystem mutations.
    @throws ValueError If any selected subtree contains a nested repository or link.
    """
    pending = []
    for item in roots:
        path = root if str(item) == "." else checked_path(item, root)
        if not path.exists():
            raise ValueError(f"missing inventory owner: {path}")
        pending.append((path, path.is_file()))
    seen = set()
    # Overlapping roots must not create duplicate aliases or silently traverse a
    # second repository. Reject uncertain ownership instead of skipping valuable data.
    while pending:
        path, regular_file = pending.pop()
        if path in seen:
            continue
        seen.add(path)
        if regular_file:
            yield path
            continue
        # Recheck the current directory edge before entering it. Its parents were
        # already admitted in this pass; repeating their full canonical/lstat
        # chain for every directory caused the measured traversal CPU blowup.
        status = path.lstat()
        attributes = getattr(status, "st_file_attributes", 0)
        if stat.S_ISLNK(status.st_mode) or attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
            raise ValueError(f"redirected inventory path: {path}")
        # Root Git metadata is preserved as Git metadata, never classified as saves.
        if path != root and (path / ".git").exists():
            raise ValueError(f"nested repository in inventory: {path}")
        # Windows directory enumeration already supplies edge metadata. Reuse
        # that one observation instead of reopening every generated object three
        # times for exists/type/attributes. Each entered directory edge is rechecked;
        # redirected/special edges still refuse, and protected bytes are hashed.
        with os.scandir(path) as entries:
            for entry in sorted(entries, key=lambda value: value.name, reverse=True):
                child = Path(entry.path)
                if child == root / ".git":
                    continue
                status = entry.stat(follow_symlinks=False)
                attributes = getattr(status, "st_file_attributes", 0)
                if stat.S_ISLNK(status.st_mode) or attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
                    raise ValueError(f"redirected inventory path: {child}")
                if not (stat.S_ISREG(status.st_mode) or stat.S_ISDIR(status.st_mode)):
                    raise ValueError(f"special inventory path: {child}")
                pending.append((child, stat.S_ISREG(status.st_mode)))


def collect_candidates(root=None, roots=(".",)):
    """Collect all saves, meaningful configs and the existing local source baseline.

    @param root Optional checkout root; discovery reads only this checkout.
    @param roots Explicit inventory scopes; default scans everything except Git metadata.
    @return Sorted typed retention candidates; no exports, contexts or reports are written.
    """
    root = repository_root(root)
    rows = []
    consumers = {}
    manifest = root / "src/tests/fixtures/multilayer-inputs.json"
    if manifest.is_file():
        # The existing catalogue supplies reader names, never an exhaustive
        # retention list. Uncatalogued saves receive the same preservation policy.
        for entry in json.loads(manifest.read_text(encoding="utf-8"))["inputs"].values():
            consumers[entry["path"]] = ("transfer-preservation", "artifact_paths.resolve_input_path")
    # Inspect headers even for unfamiliar extensions: extension-only discovery
    # would lose renamed old saves. Keep source-baseline bytes without regenerating them.
    for path in walk_files(root, roots):
        relative = path.relative_to(root).as_posix()
        detected = save_format(path)
        if detected or path.suffix.lower() == ".cfg" or relative.startswith("OpenTTD-baseline/"):
            # walk_files already validated every ownership edge. Rechecking all
            # ancestors for every copied save adds minutes without a stronger
            # snapshot; identity still checks for concurrent file changes.
            role = "source_baseline"
            if detected:
                role = "unknown_save" if detected == "unknown_save" else "save"
            elif path.suffix.lower() == ".cfg":
                role = "configuration"
            digest, length = file_identity(path)
            disposition = "export" if in_disposable(relative) else "retain"
            rows.append(MigrationRow(relative, retention_destination(relative, role, detected),
                digest, role, consumers.get(relative, ("transfer-preservation",)),
                disposition, "present", digest, length, detected))
    return sorted(rows, key=lambda row: row.original_path)


def verify_input_aliases(root=None):
    """Check catalogue hashes independently of the legacy resolver's path precedence.

    @param root Optional checkout root holding the existing input catalogue.
    @return Typed observations for every logical key, including missing/changed inputs.
    """
    root = repository_root(root)
    manifest = checked_path("src/tests/fixtures/multilayer-inputs.json", root)
    if not manifest.is_file():
        return []
    entries = json.loads(manifest.read_text(encoding="utf-8"))["inputs"]
    # One logical key has one authority. Following explicit aliases only when the
    # old file is absent matches existing callers without skipping identity checks.
    return [classify_reference(dict(path=key, sha256=value["sha256"]), root,
        consumers=("artifact_paths.resolve_input_path",), aliases=entries) for key, value in entries.items()]


def trace_references(reference, root=None, configuration=None):
    """Trace explicit Space receipt schemas without importing their executable readers.

    @param reference Exact bound receipt path or path/SHA256 identity.
    @param root Optional checkout root for read-only traversal and isolated checks.
    @param configuration Optional selected native configuration; final pair remains linked.
    @return Typed edges, validation failures, cycles and actual continuation roots.
    """
    from artifact_space_graph import trace_space_references
    return trace_space_references(reference, root, configuration)


def migration_manifest(rows):
    """Compact identical retained payloads while preserving every original alias.

    @param rows Typed inventory rows; every observed save remains represented.
    @return Versioned JSON contract; destinations are proposals until verified export.
    @throws ValueError If an alias or destination has conflicting content authorities.
    """
    grouped, originals, destinations = {}, {}, {}
    # Equal bytes may share payloads only with complete aliases. Durable original
    # locations take precedence over proposed copies; no original is overwritten.
    for row in sorted(rows, key=lambda item: (item.disposition != "retain", item.original_path)):
        if row.original_path in originals:
            if originals[row.original_path] != row.sha256:
                raise ValueError(f"conflicting original: {row.original_path}")
            continue
        originals[row.original_path] = row.sha256
        # Missing/changed inputs must be resolved before publication or export.
        if row.verification not in {"present", "alias_verified"} or row.sha256 is None:
            raise ValueError(f"unverified migration input: {row.original_path}: {row.verification}")
        if row.destination in destinations:
            if destinations[row.destination] != row.sha256:
                raise ValueError(f"conflicting destination: {row.destination}")
        destinations[row.destination] = row.sha256
        key = (row.sha256, row.role, row.disposition)
        if key not in grouped:
            grouped[key] = dict(destination=row.destination, sha256=row.sha256,
                role=row.role, consumers=list(row.consumers), disposition=row.disposition,
                verification="inventoried_not_exported" if row.disposition == "export" else "present",
                bytes=row.bytes, format=row.format, generator=row.generator, original_paths=[])
        grouped[key]["original_paths"].append(row.original_path)
        grouped[key]["consumers"] = sorted(set(grouped[key]["consumers"]) | set(row.consumers))
    return dict(_documentation="Owner: r2fc-file-cleanup3.md Group2; typed preservation proposals, not deletion permission. All original_paths are aliases of the exact payload SHA256.",
        schema_version=1, payloads=list(grouped.values()), original_path_count=len(originals))


def main():
    """Inspect retained-input candidates or explicitly publish an inventory contract.

    @return None; default execution prints a summary and writes no files.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--scope", action="append")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()
    root = repository_root(args.root)
    rows = collect_candidates(root, args.scope or (".",))
    manifest = migration_manifest(rows)
    if args.report:
        owner = owned_root(root, "test-cache")
        output = contained(checked_path(args.report, root), owner)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(dict(_documentation=__doc__, rows=[row.as_dict() for row in rows]), indent=2) + "\n", encoding="utf-8")
    if args.manifest:
        owner = owned_root(root, "src/tests/fixtures/cleanup")
        output = contained(checked_path(args.manifest, root), owner)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(candidates=len(rows), payloads=len(manifest["payloads"]),
        saves=sum(row.format is not None for row in rows), bytes=sum(row.bytes for row in rows))))


if __name__ == "__main__":
    main()
