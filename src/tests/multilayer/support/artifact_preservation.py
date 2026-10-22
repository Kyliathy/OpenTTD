"""Export inventoried immutable inputs without deleting or rewriting originals.

@file artifact_preservation.py Save/config migration and source-epoch preservation.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group9.
Uses the existing migration and multilayer input catalogues; neither ignore
rules nor successful decoding establish permission to discard an original.
"""

import argparse
import copy
import json
import os
from pathlib import Path
import shutil

from artifact_inventory import checked_path, file_identity
from artifact_paths import repository_root, resolve_input_path


def publish_document(path, document, root):
    """Atomically publish a documented migration result after its inputs verify.

    @param path Explicit final manifest, contained in the selected checkout.
    @param document JSON-compatible preservation result.
    @param root Explicit source root.
    @return Published manifest path; an interrupted pending file is not authority.
    """
    target = checked_path(path, root)
    target.parent.mkdir(parents=True, exist_ok=True)
    pending = checked_path(target.with_name(target.name + ".pending"), root)
    with pending.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(document, indent=2) + "\n")
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(pending, target)
    return target


def preserve_file(original, destination, expected, root):
    """Copy exact immutable bytes, refusing missing, changed or conflicting inputs.

    @param original Explicit inventoried source, never a latest-file search.
    @param destination Retained owner selected by the migration contract.
    @param expected Original SHA256 identity captured before cleanup edits.
    @param root Explicit source checkout.
    @return Exact retained path/SHA256 identity after verifying both sides.
    """
    source = checked_path(original, root)
    target = checked_path(destination, root)
    if file_identity(source)[0] != expected:
        raise ValueError(f"original preservation input changed: {original}")
    if target.exists():
        if file_identity(target)[0] != expected:
            raise ValueError(f"conflicting retained input: {destination}")
    else:
        target.parent.mkdir(parents=True, exist_ok=True)
        pending = checked_path(target.with_name(target.name + ".pending"), root)
        # Reusing an unrelated interrupted export would conceal a writer or
        # source change; only the same original identity may resume this copy.
        if pending.exists() and file_identity(pending)[0] != expected:
            raise ValueError(f"conflicting interrupted export: {pending}")
        shutil.copyfile(source, pending)
        if file_identity(pending)[0] != expected:
            raise ValueError(f"source changed while exporting: {original}")
        os.replace(pending, target)
    if file_identity(source)[0] != expected or file_identity(target)[0] != expected:
        raise ValueError(f"preservation identities changed at close: {original}")
    return dict(path=target.relative_to(root).as_posix(), sha256=expected)


def preserve_payloads(manifest, root, checkpoint=None):
    """Export every inventoried save/configuration, including corrupt legacy saves.

    @param manifest Group2 typed payload/complete-original-alias manifest.
    @param root Explicit checkout containing original files and retained owners.
    @param checkpoint Optional already verified Space seal selecting its exact saves.
    @return New verified migration document; caller publishes only on full success.
    """
    result = copy.deepcopy(manifest)
    selected = {}
    if checkpoint is not None:
        selected = {row["save"]["sha256"]: row["save"]["path"] for row in checkpoint["configurations"].values()}
    destinations = {}
    # Payload hashes deduplicate physical storage, while every original spelling
    # remains independently verified and recorded as an alias. Unsupported save
    # formats are copied exactly; no decoder is used as a preservation gate.
    for row in result["payloads"]:
        expected = row["sha256"]
        destination = selected.get(expected, row["destination"])
        # Prefixing a deep original review path can exceed Windows' ordinary
        # filename limit. Keep its basename in a bounded hash directory; the
        # complete original relative spellings remain in the migration aliases.
        if len(str(root / destination)) + len(".pending") > 245:
            owner = "savegames/retained" if row["role"] in ("save", "unknown_save") else "src/tests/fixtures/config/multilayer/retained"
            destination = f"{owner}/by-hash/{expected[:16]}/{Path(destination).name}"
        target = checked_path(destination, root)
        old_digest = destinations.setdefault(str(target).casefold(), expected)
        if old_digest != expected:
            raise ValueError(f"same destination has different inventoried content: {destination}")
        for original in row["original_paths"]:
            if file_identity(checked_path(original, root))[0] != expected:
                raise ValueError(f"inventoried alias changed: {original}")
        preserve_file(row["original_paths"][0], destination, expected, root)
        row["destination"] = destination
        row["verification"] = "exported_verified" if row["disposition"] == "export" else "retained_verified"
    result["_documentation"] = "Owner: r2fc-file-cleanup3.md9; all original aliases and retained payloads verified; no deletion permission."
    return result


def migrated_catalogue(catalogue, manifest, root):
    """Extend the existing input catalogue with exact retained-first save aliases.

    @param catalogue Existing multilayer-inputs.json document.
    @param manifest Fully verified migration document.
    @param root Checkout whose retained bytes must match every published alias.
    @return Schema2 catalogue, retaining all existing logical input identities.
    """
    result = copy.deepcopy(catalogue)
    entries = result["inputs"]
    # Every exported original becomes a stable key. Canonical retained paths
    # also receive identities so direct fixture selection validates the bytes.
    for row in manifest["payloads"]:
        if row["disposition"] != "export":
            continue
        reference = dict(path=row["destination"], sha256=row["sha256"])
        for original in [*row["original_paths"], row["destination"]]:
            previous = entries.get(original)
            if previous is not None and previous["sha256"] != row["sha256"]:
                raise ValueError(f"existing input identity disagrees with migration: {original}")
            entries[original] = reference
    for key, reference in entries.items():
        if file_identity(checked_path(reference["path"], root))[0] != reference["sha256"]:
            raise ValueError(f"retained input catalogue hash changed: {key}")
    result.update(schema_version=2, alias_precedence="retained_before_existing",
                  _documentation="Owner: r2fc-file-cleanup3.md9; hash-bound stable keys prefer retained copies; unindexed explicit prior runs remain direct inputs.")
    return result


def verify_preservation(manifest, root, originals=True):
    """Verify a published migration without creating exports or runtime contexts.

    @param manifest Explicit typed migration document.
    @param root Checkout containing its retained payloads.
    @param originals Also verify all old aliases; False supports a cleaned transfer.
    @return Compact payload/alias/byte totals after every required identity matches.
    """
    aliases = 0
    length = 0
    # Full-byte hashing, rather than decoder success, protects legacy/corrupt
    # fixtures and distinct same-name saves just as strictly as current saves.
    for row in manifest["payloads"]:
        if row["verification"] not in ("exported_verified", "retained_verified"):
            raise ValueError("unverified migration payload")
        actual, size = file_identity(checked_path(row["destination"], root))
        if actual != row["sha256"]:
            raise ValueError(f"retained migration payload changed: {row['destination']}")
        if originals:
            for original in row["original_paths"]:
                if file_identity(checked_path(original, root))[0] != actual:
                    raise ValueError(f"original migration alias changed: {original}")
        aliases += len(row["original_paths"])
        length += size
    return dict(payloads=len(manifest["payloads"]), original_aliases=aliases,
                retained_bytes=length, original_aliases_verified=originals)


def verify_migration_catalogue(manifest, catalogue, root):
    """Resolve every retained catalogue alias through the actual shared reader.

    @param manifest Published verified payload/original alias migration.
    @param catalogue Existing canonical multilayer input catalogue after migration.
    @param root Checkout in which old originals may still coexist with retained inputs.
    @return Complete input-key/payload/alias totals after exact shared-reader checks.
    """
    entries = catalogue["inputs"]
    if catalogue.get("alias_precedence") != "retained_before_existing":
        raise ValueError("migration catalogue must explicitly select retained-first aliases")
    aliases = 0
    # Check complete migration coverage before resolving the catalogue. A valid
    # subset of aliases cannot conceal an unexported user save or configuration.
    for row in manifest["payloads"]:
        aliases += len(row["original_paths"])
        if row["disposition"] != "export":
            continue
        reference = dict(path=row["destination"], sha256=row["sha256"])
        for original in row["original_paths"]:
            if entries.get(original) != reference:
                raise ValueError(f"migration alias missing or changed: {original}")
    if aliases != manifest["original_path_count"]:
        raise ValueError("migration original alias coverage disagrees")
    # Exercise retained-first selection and full hashing for every key, rather
    # than accepting the JSON's declared destination without using its reader.
    for key, reference in entries.items():
        selected = resolve_input_path(key, root)
        if selected != checked_path(reference["path"], root):
            raise ValueError(f"shared reader selected a different input: {key}")
    return dict(catalogue_keys=len(entries), payloads=len(manifest["payloads"]),
                original_aliases=aliases, retained_alias_precedence_verified=True)


def main():
    """Verify by default; explicitly export before publishing updated catalogues.

    @return None; --export is the only path that writes retained files/manifests.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", action="store_true")
    parser.add_argument("--retained-only", action="store_true")
    parser.add_argument("--verify-catalogue", action="store_true")
    args = parser.parse_args()
    root = repository_root()
    migration = root / "src/tests/fixtures/cleanup/migration.json"
    manifest = json.loads(migration.read_text(encoding="utf-8"))
    if args.export:
        import sys
        sys.path.insert(0, str(root / "src/tests/multilayer/acceptance"))
        from space_checkpoint import load_checkpoint, verify_checkpoint
        selected = load_checkpoint()
        verify_checkpoint(selected)
        seal = json.loads(selected.manifest_path.read_text(encoding="utf-8"))
        manifest = preserve_payloads(manifest, root, seal)
        catalogue_path = root / "src/tests/fixtures/multilayer-inputs.json"
        catalogue = migrated_catalogue(json.loads(catalogue_path.read_text(encoding="utf-8")), manifest, root)
        # Both complete documents are prepared before publication. Originals
        # remain intact even if publication is interrupted between these files.
        publish_document(migration, manifest, root)
        publish_document(catalogue_path, catalogue, root)
    if args.verify_catalogue:
        catalogue = json.loads((root / "src/tests/fixtures/multilayer-inputs.json").read_text(encoding="utf-8"))
        print(json.dumps(verify_migration_catalogue(manifest, catalogue, root), indent=2))
    else:
        print(json.dumps(verify_preservation(manifest, root, not args.retained_only), indent=2))


if __name__ == "__main__":
    main()
