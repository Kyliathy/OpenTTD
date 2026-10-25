"""Package the complete preserved working tree, including local Git history.

@file package_transfer.py Explicit source transfer and cold staging boundary.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group29.
@see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
Ignore rules never select transfer contents. The actual cleaner supplies exact
generated exclusions; save export and required-input checks precede staging.
"""

import argparse
import hashlib
import json
import logging
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import subprocess
import zipfile

from artifact_inventory import checked_path, file_identity
from artifact_paths import repository_root
from artifact_preservation import verify_cleanup_inputs
from save_retention import JOURNAL, apply_save_exports, plan_save_exports, verify_save_exports

MANIFEST = ".transfer-manifest.json"
LOGGER = logging.getLogger(__name__)


def _git(root, *arguments):
    """Read Git metadata without staging, changing refs or invoking checkout.

    @param root Repository whose local metadata must be self-contained.
    @param arguments Explicit read-only Git argument sequence.
    @return UTF-8 output; an incomplete object store refuses transfer.
    """
    environment = dict(os.environ, GIT_OPTIONAL_LOCKS="0")
    # A developer's shell override must not make a local-looking checkout depend
    # on another object store during either enumeration or extracted validation.
    for name in ("GIT_DIR", "GIT_WORK_TREE", "GIT_COMMON_DIR", "GIT_OBJECT_DIRECTORY", "GIT_ALTERNATE_OBJECT_DIRECTORIES"):
        environment.pop(name, None)
    result = subprocess.run(["git", "-C", str(root), *arguments], capture_output=True,
                            encoding="utf-8", errors="strict", env=environment, check=False)
    if result.returncode:
        raise ValueError(f"Git transfer dependency check failed: {root}: {result.stderr.strip()}")
    return result.stdout


def _cleanup_scopes(root):
    """Ask the delivered cleaner for its exact disposable target selection.

    @param root Explicit checkout containing the tested clean.ps1 owner.
    @return Relative scopes; no exports, process checks or deletion run here.
    """
    command = ("$ErrorActionPreference='Stop'; $repo=[Console]::In.ReadToEnd(); "
               ". (Join-Path $repo 'clean.ps1'); "
               "$paths=@(Get-DevelopmentCleanupPlan $repo $false | "
               "ForEach-Object { $_.Path.Substring($repo.Length+1).Replace('\\','/') }); "
               "ConvertTo-Json -InputObject $paths -Compress")
    result = subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", command],
                            input=str(root), text=True, capture_output=True, check=False)
    if result.returncode:
        raise ValueError("cleaner exclusion planning failed: " + result.stderr.strip())
    return json.loads(result.stdout)


def _inside(relative, scopes):
    """Test exact audited owner membership using portable path components.

    @param relative Repository-relative file or directory spelling.
    @param scopes Exact exclusions returned by the cleaner.
    @return True only for an exclusion itself or one of its descendants.
    """
    return any(relative == scope or relative.startswith(scope + "/") for scope in scopes)


def _ordinary(path):
    """Refuse redirected or special filesystem edges before reading/copying.

    @param path Existing edge beneath an explicitly selected source/destination.
    @return Stat observation for an ordinary file or directory.
    """
    status = path.lstat()
    # Redirects can silently import an external checkout or export outside the
    # authorized destination. Neither ordinary links nor Windows junctions pass.
    if stat.S_ISLNK(status.st_mode) or getattr(status, "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT:
        raise ValueError(f"redirected transfer path: {path}")
    if not (stat.S_ISREG(status.st_mode) or stat.S_ISDIR(status.st_mode)):
        raise ValueError(f"special transfer path: {path}")
    return status


def _git_dependency(root):
    """Require a complete local Git store for a root or nested source repository.

    @param root Explicit repository discovered while enumerating preserved files.
    @return None; external stores/worktree metadata refuse instead of being lost.
    """
    marker = root / ".git"
    _ordinary(marker)
    if not marker.is_dir():
        raise ValueError(f"linked-worktree or external Git metadata requires explicit recovery: {marker}")
    local = marker.resolve()
    if Path(_git(root, "rev-parse", "--show-toplevel").strip()).resolve() != root.resolve():
        raise ValueError(f"external Git working-tree dependency: {root}")
    # Common stores and alternates may live outside the working tree even when
    # its files appear complete. This helper never follows or rewrites them.
    for argument in ("--git-dir", "--git-common-dir"):
        spelling = Path(_git(root, "rev-parse", argument).strip())
        selected = spelling if spelling.is_absolute() else root / spelling
        if selected.resolve() != local:
            raise ValueError(f"external Git object/common store: {root}")
    for name in ("objects/info/alternates", "objects/info/http-alternates", "commondir", "gitdir"):
        dependency = marker / name
        if dependency.exists():
            if dependency.read_bytes().strip():
                raise ValueError(f"external Git dependency: {dependency}")
    worktrees = marker / "worktrees"
    if worktrees.is_dir():
        if any(worktrees.iterdir()):
            raise ValueError(f"linked-worktree metadata requires separate transfer selection: {worktrees}")
    configuration = _git(root, "config", "--local", "--list").lower()
    promised = False
    # Git accepts several true spellings, and a promisor pack can suppress
    # ordinary missing-object failures. Refuse all declared remote promises,
    # rather than mistake fsck's promised omissions for a complete local store.
    for line in configuration.splitlines():
        key, separator, value = line.partition("=")
        if key.endswith(".promisor") and value not in ("false", "no", "off", "0"):
            promised = True
        if key == "extensions.partialclone":
            promised = True
    if promised or any((marker / "objects/pack").glob("*.promisor")):
        raise ValueError(f"partial Git object store requires completion before transfer: {root}")
    _git(root, "fsck", "--full", "--no-reflogs", "--no-dangling")


def _tracked_exclusions(repository, root, scopes):
    """Reject selected generated exclusions that would omit tracked working files.

    @param repository Root or nested local source repository being inspected.
    @param root Transfer root determining portable relative member paths.
    @param scopes Exact current cleaner exclusions, including routing exceptions.
    @return None; the cheap root check also runs before expensive save inspection.
    """
    for spelling in filter(None, _git(repository, "ls-files", "-z").split("\0")):
        relative = (repository / spelling).relative_to(root).as_posix()
        if _inside(relative, scopes):
            raise ValueError(f"tracked working file overlaps generated exclusion: {relative}")


def enumerate_preserved_files(root=None):
    """Hash every preserved working file and Git file regardless of ignore state.

    @param root Optional selected repository; defaults to this checkout.
    @return Transfer manifest with exact cleaner exclusions and complete file rows.
    """
    root = repository_root(root)
    scopes = _cleanup_scopes(root)
    files, repositories, pending = [], [], [root]
    names = set()
    # Enumerate the filesystem, not git ls-files: approved pixels, ignored saves
    # and untracked source are as valuable as tracked files. Nested local source
    # repositories travel intact; external/redirected dependencies refuse.
    while pending:
        parent = pending.pop()
        _ordinary(parent)
        if (parent / ".git").exists():
            _git_dependency(parent)
            relative_repo = parent.relative_to(root).as_posix()
            repositories.append(relative_repo)
            _tracked_exclusions(parent, root, scopes)
        for child in sorted(parent.iterdir(), reverse=True):
            relative = child.relative_to(root).as_posix()
            if _inside(relative, scopes):
                continue
            status = _ordinary(child)
            if stat.S_ISDIR(status.st_mode):
                pending.append(child)
                continue
            if relative == MANIFEST:
                raise ValueError("source collides with reserved transfer manifest")
            folded = relative.casefold()
            if folded in names:
                raise ValueError(f"case-colliding transfer path: {relative}")
            names.add(folded)
            digest, length = file_identity(child)
            files.append(dict(path=relative, sha256=digest, bytes=length))
    if "." not in repositories:
        raise ValueError("transfer requires the selected repository's local Git metadata")
    return dict(_documentation="Owner: r2fc2-file-cleanup3.md29; complete preserved working bytes, no generated proof authority.",
                schema_version=1, kind="source_transfer_manifest", exclusions=scopes,
                git_repositories=sorted(repositories), files=sorted(files, key=lambda row: row["path"]),
                self_exclusion=MANIFEST)


def _destination(path, source):
    """Validate an explicitly authorized scratch/temporary write destination.

    @param path Requested new stage, archive or extraction root.
    @param source Payload root, which must never contain its own output.
    @return Absolute ordinary path within project scratch or user-approved Temp.
    """
    raw = Path(path).absolute()
    # Check original edges before resolving; a junction must not make a forbidden
    # path appear authorized after its spelling has already escaped ownership.
    for component in (raw, *raw.parents):
        if component.exists():
            _ordinary(component)
    target = raw.resolve()
    workspace = repository_root()
    permitted = target.is_relative_to(workspace / "test-cache")
    local = os.environ.get("LOCALAPPDATA")
    if local:
        local = Path(local).resolve()
        relative = target.relative_to(local) if target.is_relative_to(local) else None
        if relative is not None and relative.parts:
            permitted |= relative.parts[0] == "Temp" or relative.parts[0].startswith("cxc-")
    if not permitted:
        raise ValueError(f"transfer destination is outside authorized scratch/Temp: {target}")
    if target.is_relative_to(source.resolve()):
        raise ValueError("transfer output must be outside its payload (self-exclusion)")
    return target


def _member(path):
    """Validate an archive path before creating any extracted filesystem entry.

    @param path Portable relative member spelling from the verified manifest.
    @return Pure relative POSIX path without traversal or Windows stream syntax.
    """
    result = PurePosixPath(path)
    # Windows separators/drive streams can escape a POSIX-looking archive path.
    # Empty/dot components also admit duplicate spellings of an extracted file.
    if not path or "\\" in path or ":" in path or result.is_absolute() or any(part in ("", ".", "..") for part in path.split("/")):
        raise ValueError(f"unsafe transfer member: {path}")
    # Device names and trailing-dot/space aliases are unsafe even when their ZIP
    # spelling is relative; Windows could redirect or merge extracted members.
    reserved = {"con", "prn", "aux", "nul", *(f"com{i}" for i in range(1, 10)), *(f"lpt{i}" for i in range(1, 10))}
    for part in result.parts:
        if part.endswith((" ", ".")) or part.split(".", 1)[0].casefold() in reserved:
            raise ValueError(f"unsafe Windows transfer member: {path}")
    return result


def _check_output_lengths(target, names):
    """Require a destination usable by the supported Windows file/tool APIs.

    @param target Explicit new stage or extraction root.
    @param names Complete portable payload member names, without renaming aliases.
    @return None; a longer root refuses before creating any partial output tree.
    """
    if os.name != "nt":
        return
    # Retained names are immutable inputs. A short authorized cxc-* root keeps
    # their complete spelling usable without changing machine registry settings
    # or introducing extended-path spellings that native build tools cannot use.
    longest = max((target.joinpath(*_member(name).parts) for name in names), key=lambda path: len(str(path)), default=target)
    if len(str(longest)) > 259:
        raise ValueError(f"Windows transfer path exceeds 259 characters; choose a shorter destination root: {longest}")


def stage_transfer(destination, root=None):
    """Export generated saves and copy a complete source-only transfer tree.

    @param destination New empty root in authorized scratch/Temp, outside source.
    @param root Optional selected live checkout; working files are never deleted.
    @return Staged root, manifest counts and exact preservation result.
    """
    root = repository_root(root)
    target = _destination(destination, root)
    if target.exists():
        raise ValueError(f"stage destination already exists: {target}")
    LOGGER.info("Planning exact cleaner exclusions for %s", root)
    scopes = _cleanup_scopes(root)
    _tracked_exclusions(root, root, scopes)
    LOGGER.info("Inspecting saves and configurations in %s excluded owners", len(scopes))
    plan = plan_save_exports(root, scopes, configurations=True)
    LOGGER.info("Checking required retained inputs; observed %s save/config aliases", len(plan["observations"]))
    required = verify_cleanup_inputs(root, scopes)
    journal_path = checked_path(JOURNAL, root)
    existing = json.loads(journal_path.read_text(encoding="utf-8")) if journal_path.exists() else {}
    aliases = existing.get("aliases", {})
    committed = bool(existing) and all(
        aliases.get(original) == dict(path=row["destination"], sha256=row["sha256"])
        for row in plan["payloads"] for original in row["original_paths"])
    # Reuse the established journal only after fresh source observations and
    # retained-byte verification. Changed/new saves take the ordinary exporter;
    # no previous successful report substitutes for current physical checks.
    if committed:
        LOGGER.info("Verifying %s already committed payloads without rewriting exports", len(plan["payloads"]))
        exports = verify_save_exports(plan, root)
    else:
        LOGGER.info("Exporting %s distinct save/config payloads", len(plan["payloads"]))
        exports = apply_save_exports(plan, root)
    LOGGER.info("Enumerating complete preserved working files and local Git stores")
    manifest = enumerate_preserved_files(root)
    _check_output_lengths(target, (row["path"] for row in manifest["files"]))
    ancestor = target.parent
    while not ancestor.exists():
        ancestor = ancestor.parent
    required_bytes = sum(row["bytes"] for row in manifest["files"])
    if shutil.disk_usage(ancestor).free < required_bytes:
        raise ValueError("insufficient space for independent staged source copies")
    LOGGER.info("Copying %s preserved files (%s bytes) to %s", len(manifest["files"]), required_bytes, target)
    target.mkdir(parents=True)
    # Copy files separately and verify both observations. Hard links would let
    # staged builds or future edits modify the live source through shared bytes.
    for row in manifest["files"]:
        source = checked_path(row["path"], root)
        output = target.joinpath(*_member(row["path"]).parts)
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, output)
        if file_identity(source) != (row["sha256"], row["bytes"]) or file_identity(output) != (row["sha256"], row["bytes"]):
            raise ValueError(f"transfer bytes changed during staging: {row['path']}")
    LOGGER.info("Rechecking source save observations after independent staging copies")
    verify_save_exports(plan, root)
    # Git identity is checked again inside the staged tree, never borrowed from
    # the original workstation. A partial stage remains incomplete on refusal.
    for relative in manifest["git_repositories"]:
        _git_dependency(target / relative)
    LOGGER.info("Staged transfer and local Git stores verified")
    return dict(root=str(target), files=len(manifest["files"]),
                bytes=sum(row["bytes"] for row in manifest["files"]),
                required_inputs=required, save_exports=exports)


def stage_selected_recovery(destination, references, current_build, root=None):
    """Stage declared durable inputs and one current build for bounded cold recovery.

    @param destination New project-owned test-cache directory; never reused or cleaned.
    @param references Exact source/input identities required by the selected recipes.
    @param current_build Fresh selected native-build report identity, not a historical image.
    @param root Source checkout containing the declared inputs and current build.
    @return Exact staged payload manifest; no Git metadata or earlier review tree is copied.
    """
    from artifact_rebuild import identity
    from native_build import verify_source_epoch
    root = repository_root(root)
    target = checked_path(destination, root)
    # The rehearsal is an explicitly isolated disposable owner. Reusing any
    # existing tree could silently admit an older generated prerequisite.
    if not target.is_relative_to(root / 'test-cache') or target.exists():
        raise ValueError('selected recovery requires a fresh project test-cache destination')
    for parent in target.parents:
        if parent.exists():
            _ordinary(parent)
    build_path = checked_path(current_build['path'], root)
    if identity(build_path, root) != current_build:
        raise ValueError('selected current build report changed')
    build = json.loads(build_path.read_text(encoding='utf-8'))
    # Only a successful current build supplies executable bytes. Historical
    # image identities nested in retained facts never join this allowed set.
    if build.get('kind') != 'native_build_result' or build.get('complete') is not True:
        raise ValueError('selected recovery requires a completed current native build')
    epoch_path = checked_path(build['source_epoch']['path'], root)
    epoch = json.loads(epoch_path.read_text(encoding='utf-8'))
    verify_source_epoch(epoch, root, build['tool_profile']['path'])
    generated = [current_build, build['source_epoch'], build['tool_profile'], *build['targets'].values()]
    allowed_generated = {row['path']: row for row in generated}
    payload = {}
    # Exact source epochs and durable input declarations form the transferred
    # closure. An arbitrary logs/review/cache file cannot enter through a caller.
    for reference in [*references, *generated]:
        relative = reference['path']
        path = checked_path(relative, root)
        _ordinary(path)
        disposable = relative.split('/')[0] in ('build', 'logs', 'test-cache') or relative.startswith(('mlm/reviews/', 'mlm/runtime/'))
        if disposable and allowed_generated.get(relative) != reference:
            raise ValueError('undeclared earlier generated recovery input: ' + relative)
        if '.git' in Path(relative).parts:
            raise ValueError('selected recovery never writes Git metadata')
        if identity(path, root) != reference:
            raise ValueError('selected recovery input changed: ' + relative)
        payload[relative] = reference
    target.mkdir(parents=True)
    # Copy each exact admitted file separately. No links, recursive deletion,
    # directory overlay or inherited working-tree cache is used.
    for relative, reference in sorted(payload.items()):
        source = checked_path(relative, root)
        output = checked_path(target / relative, root)
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, output)
        if file_identity(output)[0] != reference['sha256']:
            raise ValueError('selected staged recovery bytes differ: ' + relative)
    manifest = dict(_documentation=__doc__, kind='selected_cold_recovery_transfer',
        files=list(payload.values()), current_build=current_build, old_workstation_reviews_available=False,
        historical_executables_copied=False, git_metadata_copied=False)
    (target / '.selected-recovery-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    return dict(root=target.relative_to(root).as_posix(), files=len(payload),
        manifest=identity(target / '.selected-recovery-manifest.json', root))


def create_transfer_archive(archive, root=None):
    """Create a verified ZIP64 package with a manifest and external checksum.

    @param archive New archive path outside the payload in authorized scratch/Temp.
    @param root Optional selected clean/staged checkout.
    @return Archive identity, sidecar path and verified payload totals.
    """
    root = repository_root(root)
    target = _destination(archive, root)
    sidecar = target.with_name(target.name + ".sha256")
    # Neither an older package nor its checksum may be silently replaced.
    if target.exists() or sidecar.exists():
        raise ValueError("transfer archive or external checksum already exists")
    manifest = enumerate_preserved_files(root)
    required = verify_cleanup_inputs(root, manifest["exclusions"])
    plan = plan_save_exports(root, manifest["exclusions"], configurations=True)
    # Packaging never drops an unexported newly created save/configuration. A
    # caller stages/exports it first; read-only archive creation cannot repair it.
    if plan["observations"]:
        try:
            verify_save_exports(plan, root)
        except (OSError, ValueError) as error:
            raise ValueError(f"unexported save/configuration; stage/export before packaging: {error}") from error
    target.parent.mkdir(parents=True, exist_ok=True)
    pending = target.with_name(target.name + ".pending")
    with pending.open("xb") as stream:
        with zipfile.ZipFile(stream, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=1, allowZip64=True) as package:
            package.writestr(MANIFEST, json.dumps(manifest, indent=2) + "\n")
            # Read/write each member once, checking the complete streamed bytes
            # against enumeration before publishing any successful package name.
            for row in manifest["files"]:
                source = checked_path(row["path"], root)
                digest, length = hashlib.sha256(), 0
                with source.open("rb") as incoming, package.open(row["path"], "w", force_zip64=True) as outgoing:
                    for block in iter(lambda: incoming.read(1024 * 1024), b""):
                        outgoing.write(block)
                        digest.update(block)
                        length += len(block)
                if (digest.hexdigest(), length) != (row["sha256"], row["bytes"]):
                    raise ValueError(f"source changed during archive creation: {row['path']}")
    os.replace(pending, target)
    digest, length = file_identity(target)
    sidecar.write_text(digest + "  " + target.name + "\n", encoding="utf-8")
    result = verify_transfer_archive(target)
    return dict(path=str(target), sha256=digest, bytes=length, checksum=str(sidecar), required_inputs=required, **result)


def verify_transfer_archive(archive, extraction=None):
    """Check checksum, exact safe member set and hashes before optional extraction.

    @param archive Explicit existing ZIP64 package with its external SHA256 sidecar.
    @param extraction Optional new empty root in authorized scratch/Temp.
    @return Verified file/byte totals and optional extracted root; no baseline edits.
    """
    path = Path(archive).resolve()
    sidecar = path.with_name(path.name + ".sha256")
    digest = sidecar.read_text(encoding="utf-8").split()[0]
    if file_identity(path)[0] != digest:
        raise ValueError("transfer archive external checksum differs")
    target = _destination(extraction, path) if extraction is not None else None
    if target is not None:
        if target.exists():
            raise ValueError("transfer extraction destination already exists")
    with zipfile.ZipFile(path) as package:
        infos = package.infolist()
        names = [row.filename for row in infos]
        if len({name.casefold() for name in names}) != len(names):
            raise ValueError("duplicate or case-colliding archive member")
        for info in infos:
            _member(info.filename)
            if stat.S_ISLNK(info.external_attr >> 16):
                raise ValueError("archive contains a redirected member")
        manifest = json.loads(package.read(MANIFEST))
        # Only the transfer owner's manifest can define this payload; another
        # receipt cannot grant extraction or hide the manifest's own exclusion.
        if manifest.get("kind") != "source_transfer_manifest" or manifest.get("self_exclusion") != MANIFEST:
            raise ValueError("invalid transfer manifest owner/self-exclusion")
        # Git verification is also path-bound: metadata must never redirect its
        # read-only checks into a checkout outside the extracted payload.
        for relative in manifest["git_repositories"]:
            if relative != ".":
                _member(relative)
        rows = {row["path"]: row for row in manifest["files"]}
        for name in rows:
            _member(name)
        # One explicit manifest describes all other members; it intentionally
        # cannot contain its own SHA256 or accidentally include another archive.
        if len(rows) != len(manifest["files"]) or set(names) != {*rows, MANIFEST}:
            raise ValueError("archive member set differs from complete transfer manifest")
        for name, row in rows.items():
            observed, length = hashlib.sha256(), 0
            with package.open(name) as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b""):
                    observed.update(block)
                    length += len(block)
            if (observed.hexdigest(), length) != (row["sha256"], row["bytes"]):
                raise ValueError(f"transfer member identity differs: {name}")
        if target is not None:
            _check_output_lengths(target, rows)
            target.mkdir(parents=True)
            # Extraction happens only after every member passes. Preserve the
            # payload files, excluding the synthetic manifest from the checkout.
            for name in rows:
                output = target.joinpath(*_member(name).parts)
                output.parent.mkdir(parents=True, exist_ok=True)
                with package.open(name) as incoming, output.open("xb") as outgoing:
                    shutil.copyfileobj(incoming, outgoing)
            for relative in manifest["git_repositories"]:
                _git_dependency(target / relative)
    return dict(verified=True, files=len(rows), payload_bytes=sum(row["bytes"] for row in rows.values()),
                extraction=str(target) if target is not None else None)


def main():
    """Expose explicit inspection, staging, packaging and archive verification.

    @return None; inspection is read-only and each write needs an explicit output.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path)
    choice = parser.add_mutually_exclusive_group()
    choice.add_argument("--stage", type=Path)
    choice.add_argument("--archive", type=Path)
    choice.add_argument("--verify", type=Path)
    parser.add_argument("--extract", type=Path)
    arguments = parser.parse_args()
    if arguments.extract is not None and arguments.verify is None:
        parser.error("--extract requires --verify")
    if arguments.stage is not None:
        result = stage_transfer(arguments.stage, arguments.root)
    elif arguments.archive is not None:
        result = create_transfer_archive(arguments.archive, arguments.root)
    elif arguments.verify is not None:
        result = verify_transfer_archive(arguments.verify, arguments.extract)
    else:
        result = enumerate_preserved_files(arguments.root)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
