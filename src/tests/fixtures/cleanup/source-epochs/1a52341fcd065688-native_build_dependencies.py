"""Recreate pinned native dependency ports without modifying an external checkout.

@file native_build_dependencies.py Disposable local vcpkg provider and exact package checks.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group13.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group30 cold dependency recovery.
Original port trees were retained through read-only git archive. Rebuilds need
only their compact immutable source bundle, without an external Git history.
No Git mutation, baseline refresh or external installation is performed.
"""

import hashlib
import io
import json
from pathlib import Path
import re
import tarfile
import zipfile

from artifact_paths import contained
from artifact_inventory import file_identity


def installed_packages(installed):
    """Read effective installed package versions, separate from registry intentions.

    @param installed Explicit repository-local vcpkg installed directory.
    @return Package-name mapping, empty when a clean checkout has no installed tree.
    """
    status = Path(installed) / "vcpkg/status"
    if not status.is_file():
        return {}
    result = {}
    # Only successfully installed paragraphs describe usable dependencies.
    # Cached/downloaded package metadata cannot substitute for installation.
    for block in status.read_text(encoding="utf-8").split("\n\n"):
        fields = dict(line.split(": ", 1) for line in block.splitlines()
                      if ": " in line and not line.startswith(" "))
        if fields.get("Status") != "install ok installed":
            continue
        name = fields["Package"]
        if name in result:
            raise ValueError(f"ambiguous installed package architecture: {name}")
        result[name] = dict(version=fields["Version"], port_version=int(fields.get("Port-Version", "0")),
                            triplet=fields["Architecture"])
    return result


def verify_installed(profile, installed, root, allow_partial=False):
    """Require exact pinned versions and every declared installed payload before compiling.

    @param profile Validated portable native build profile.
    @param installed Explicit current local installed directory.
    @param root Checkout containing all dependency payloads.
    @param allow_partial Permit an exact subset from an interrupted dependency install.
    @return Compact actual package/status identities, or None when entirely absent.
    """
    actual = installed_packages(installed)
    if not actual:
        return None
    expected = {row["name"]: {key: row[key] for key in ("version", "port_version", "triplet")}
                for row in profile["dependencies"]["packages"]}
    # A failed dependency stage can resume only its exact already-installed
    # subset. Changed versions or unrelated packages still fail before compiling.
    if allow_partial and actual.keys() < expected.keys() and all(actual[name] == expected[name] for name in actual):
        return None
    if actual != expected:
        raise ValueError("installed dependency versions differ from the explicit portable profile")
    count = 0
    # The installed database alone is insufficient after partial deletion.
    # Each package's exact inventory must still resolve inside the local owner.
    for row in profile["dependencies"]["packages"]:
        listing = Path(installed) / "vcpkg/info" / f"{row['name']}_{row['version']}_{row['triplet']}.list"
        for relative in listing.read_text(encoding="utf-8").splitlines():
            candidate = contained(Path(installed) / relative, installed)
            if not candidate.exists():
                raise ValueError(f"installed dependency payload missing: {relative}")
            count += candidate.is_file()
    status = Path(installed) / "vcpkg/status"
    return dict(packages=actual, payload_files=count,
                status=dict(path=status.relative_to(root).as_posix(), sha256=file_identity(status)[0]))


def source_archives(profile, root):
    """Read exact retained third-party recipe sources without an old external Git history.

    @param profile Explicit portable dependency pins and source-bundle identity.
    @param root Checkout retaining the compact immutable source fixture.
    @return Provider tar bytes and package/tar pairs, with all provenance verified.
    """
    dependencies = profile["dependencies"]
    reference = dependencies["source_bundle"]
    path = contained(root / reference["path"], root)
    if file_identity(path)[0] != reference["sha256"]:
        raise ValueError("pinned dependency source bundle changed")
    with zipfile.ZipFile(path) as bundle:
        manifest = json.loads(bundle.read("manifest.json"))
        expected_ports = {row["name"]: row["git_tree"] for row in dependencies["packages"]}
        # Compact source retention replaces dependence on another workstation's
        # Git object database. Every original port tree must remain explicit.
        if manifest["kind"] != "pinned_native_dependency_sources" or manifest["schema_version"] != 1 or manifest["provider_revision"] != dependencies["provider_revision"] or manifest["ports"] != expected_ports:
            raise ValueError("pinned dependency recipe provenance disagrees")
        archives = {}
        for row in manifest["archives"]:
            payload = bundle.read(row["path"])
            if hashlib.sha256(payload).hexdigest() != row["sha256"]:
                raise ValueError("pinned dependency source archive changed")
            archives[row["path"]] = payload
        return archives["provider.tar"], [(row, archives["ports/" + row["name"] + ".tar"])
                                          for row in dependencies["packages"]]


def archived_provider_sources(profile, root):
    """Identify exact reconstructible dependency source bytes from retained archives.

    @param profile Explicit portable profile with original bundle/provider/port pins.
    @param root Checkout retaining that hash-verified original source bundle.
    @return Provider-relative path/SHA256 map; no extraction or writes.
    """
    provider, ports = source_archives(profile, root)
    files = {}
    # A newly materialized provider is generated, but an edited local source file
    # is valuable. Only exact archived bytes can be classified reproducible here;
    # modified/transformed source still needs its own explicit disposition.
    archives = [("", provider), *(("ports/" + package["name"] + "/", payload) for package, payload in ports)]
    for prefix, payload in archives:
        with tarfile.open(fileobj=io.BytesIO(payload)) as archive:
            for member in archive.getmembers():
                if member.isfile():
                    files[prefix + member.name] = hashlib.sha256(archive.extractfile(member).read()).hexdigest()
    return files


def write_exact(path, payload):
    """Create a local generated provider file, refusing changed interrupted contents.

    @param path Already contained provider destination.
    @param payload Exact pinned source or selected tool bytes.
    @return None; an identical partial export is safely reusable.
    """
    if path.exists():
        if path.read_bytes() != payload:
            raise ValueError(f"generated dependency provider changed: {path}")
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as stream:
        stream.write(payload)


def unpack_archive(payload, destination, root, replacements=None):
    """Extract only regular pinned source files beneath an authorized local owner.

    @param payload Exact git archive tar bytes.
    @param destination Generated provider/port directory inside the checkout.
    @param root Authorized checkout; all writes must remain contained.
    @param replacements Explicit generated helper substitutions, keyed by archive path.
    @return None; links, special entries and traversal are rejected before writes.
    """
    destination = contained(destination, root)
    with tarfile.open(fileobj=io.BytesIO(payload)) as archive:
        members = archive.getmembers()
        # Validate the entire source archive first. A linked port definition
        # cannot gain permission to create build state outside the local owner.
        for member in members:
            contained(destination / member.name, destination)
            if not member.isdir() and not member.isfile():
                raise ValueError("pinned provider archive contains a link/special entry")
        # Reconstruct source bytes only after every archive member is contained;
        # exact writes support interruption without overwriting changed inputs.
        for member in members:
            target = contained(destination / member.name, destination)
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                write_exact(target, (replacements or {}).get(member.name, archive.extractfile(member).read()))


def provider_files(provider):
    """Hash the small generated provider source/tool closure, excluding its own receipt.

    @param provider Explicit repository-local provider root.
    @return Stable relative-file/hash rows for detecting changed reusable providers.
    """
    paths = {provider / name for name in (".vcpkg-root", "LICENSE.txt", "vcpkg.exe", "vcpkg.disable-metrics")}
    # Lock files created by vcpkg are generated state, not source identities.
    # Restrict reuse verification to the actual pinned tool/scripts/ports.
    for name in ("scripts", "triplets", "ports"):
        paths.update(path for path in (provider / name).rglob("*") if path.is_file())
    return [dict(path=path.relative_to(provider).as_posix(), sha256=file_identity(path)[0]) for path in sorted(paths)]


def prepare_provider(profile, tools, build, root):
    """Reconstruct a pinned local tool/scripts/port provider from retained recipe sources.

    @param profile Explicit portable profile including actual historical package trees.
    @param tools Discovered external vcpkg and GNU patch executable paths.
    @param build Explicit disposable local build directory.
    @param root Authorized checkout for every generated write.
    @return Local provider root; external installations remain read-only.
    """
    patch_template = Path(__file__).with_name("native_build_apply_patches.cmake.in").read_text(encoding="utf-8")
    selection = dict(profile=profile, patch_template=patch_template,
                     tool_paths={name: tools[name]["path"] for name in ("patch", "vcpkg")},
                     tool_hashes={name: file_identity(Path(tools[name]["path"]))[0] for name in ("patch", "vcpkg")})
    key = hashlib.sha256(json.dumps(selection, sort_keys=True).encode("utf-8")).hexdigest()[:16]
    provider = contained(build / ".native-tools" / ("vcpkg-" + key), root)
    receipt = provider / "provider.json"
    if receipt.is_file():
        recorded = json.loads(receipt.read_text(encoding="utf-8"))
        # Reusing a provider requires the same pinned profile and every original
        # source/tool byte. Its build/download directories live elsewhere.
        if recorded["profile_key"] != key or recorded["files"] != provider_files(provider):
            raise ValueError("generated dependency provider identity changed")
        return provider
    scripts, ports = source_archives(profile, root)
    # Audited port definitions cannot directly launch Git-writing commands.
    # The unused non-Windows breakpad Git-fetch branch is blocked separately by
    # replacing that generated helper with an unconditional refusal below.
    for row, payload in ports:
        with tarfile.open(fileobj=io.BytesIO(payload)) as archive:
            for member in archive.getmembers():
                if member.isfile() and member.name.endswith(".cmake"):
                    text = archive.extractfile(member).read().decode("utf-8")
                    if re.search(r"\bCOMMAND\s+[^\n]*\bgit\b", text, re.IGNORECASE):
                        raise ValueError(f"port needs a forbidden Git-writing source recipe: {row['name']}")
    # vcpkg's standard patch helper executes git apply, which the user forbids.
    # Only this local generated provider replaces it; all original source patches
    # stay byte exact, and the actual GNU patch tool is part of discovered proof.
    patch_script = patch_template.replace("@PATCH_EXECUTABLE@", Path(tools["patch"]["path"]).as_posix())
    refusal = (
        "# @see r2fc-file-cleanup3.md13; generated provider refuses all Git mutation.\n"
        "#[[/** Refuse Git-fetch source recipes. @param ARGV Unsupported port arguments. */]]\n"
        "function(vcpkg_from_git)\n"
        '  message(FATAL_ERROR "Git-fetch source recipe is forbidden; select a pinned archive port")\n'
        "endfunction()\n")
    replacements = {"scripts/cmake/z_vcpkg_apply_patches.cmake": patch_script.encode("utf-8"),
                    "scripts/cmake/vcpkg_from_git.cmake": refusal.encode("utf-8")}
    unpack_archive(scripts, provider, root, replacements)
    for row, payload in ports:
        unpack_archive(payload, provider / "ports" / row["name"], root)
    write_exact(provider / "vcpkg.exe", Path(tools["vcpkg"]["path"]).read_bytes())
    write_exact(provider / "vcpkg.disable-metrics", b"")
    document = dict(_documentation="Owner: r2fc-file-cleanup3.md13; disposable pinned provider, no Git writes.",
                    profile_key=key, files=provider_files(provider))
    write_exact(receipt, (json.dumps(document, indent=2) + "\n").encode("utf-8"))
    return provider


def dependency_command(profile, provider, build):
    """Construct a classic pinned-port install with entirely local writable state.

    @param profile Validated actual package pins, not an inferred current registry.
    @param provider Generated source/tool provider beneath the build owner.
    @param build Explicit repository-local disposable build directory.
    @return Structured argv; callers retain actual output and exit status.
    """
    dependencies = profile["dependencies"]
    return [str(provider / "vcpkg.exe"), "install", "--classic", "--disable-metrics", "--no-print-usage",
            "--triplet=" + dependencies["target_triplet"], "--host-triplet=" + dependencies["host_triplet"],
            "--vcpkg-root=" + str(provider), "--overlay-ports=" + str(provider / "ports"),
            "--x-install-root=" + str(build / "vcpkg_installed"),
            "--x-buildtrees-root=" + str(build / ".native-work/buildtrees"),
            "--x-packages-root=" + str(build / ".native-work/packages"),
            "--downloads-root=" + str(build / ".native-work/downloads"),
            "--binarysource=clear", "--binarysource=files," + str(build / ".native-work/binary-cache") + ",readwrite",
            *dependencies["roots"]]
