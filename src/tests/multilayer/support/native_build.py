"""Build selected existing native targets from an explicit portable profile.

@file native_build.py Read-only tool discovery; generated build/run evidence is disposable.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups13/15/30.
No CMakeCache is transferred as authority. Builds use two workers, preserve
read-only Git version discovery and record actual current source/image identities.
"""

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

from artifact_paths import contained, create_run_context, repository_root
from artifact_inventory import file_identity
from native_build_dependencies import (dependency_command, prepare_provider, source_archives, verify_installed)

ROOT = Path(__file__).resolve().parents[4]
PROFILE = "src/tests/fixtures/config/multilayer/native-build-profile.json"
CONFIGURATIONS = ("RelWithDebInfo", "Debug")
PROFILE_FIELDS = {"_documentation", "schema_version", "kind", "platform", "generator", "architecture",
                  "configurations", "cxx_standard", "maximum_workers", "msbuild_node_reuse", "tools",
                  "options", "dependencies", "build_directory"}


def load_profile(path=None, root=None):
    """Validate portable intent before discovering tools, reserving runs or compiling.

    @param path Explicit profile path, or the versioned default beneath source.
    @param root Optional selected checkout.
    @return Exact parsed supported Windows/MSVC profile with pinned dependencies.
    """
    root = repository_root(root)
    selected = contained(Path(path) if path and Path(path).is_absolute() else root / (path or PROFILE), root)
    document = json.loads(selected.read_text(encoding="utf-8"))
    # Preserve actual project settings and the agreed bounded build policy.
    # A partial/foreign profile cannot silently choose another platform or CRT.
    if set(document) != PROFILE_FIELDS or type(document["schema_version"]) is not int or document["schema_version"] != 1 or document["kind"] != "portable_native_build_profile" or document["platform"] != "windows-x64" or document["generator"] != "Visual Studio 17 2022" or document["architecture"] != "x64" or document["configurations"] != list(CONFIGURATIONS) or document["cxx_standard"] != 20 or document["maximum_workers"] != 2 or document["msbuild_node_reuse"] is not False:
        raise ValueError("unsupported/mismatched portable native build profile")
    dependencies = document["dependencies"]
    manifest = json.loads((root / dependencies["source_manifest"]).read_text(encoding="utf-8"))
    if dependencies["manifest_baseline"] != manifest["builtin-baseline"]:
        raise ValueError("source manifest baseline changed; review the explicit dependency pins")
    # Exact historical port trees preserve the actual installed requirements;
    # current registry contents and the older manifest baseline are distinct.
    if dependencies["mode"] != "classic_pinned_ports" or dependencies["target_triplet"] != "x64-windows-static" or dependencies["host_triplet"] != "x64-windows" or not re.fullmatch("[0-9a-f]{40}", dependencies["provider_revision"]):
        raise ValueError("unsupported pinned dependency provider")
    names = set()
    for row in dependencies["packages"]:
        # Duplicate/unpinned package definitions could change a transitive
        # dependency without changing the selected top-level command.
        if row["name"] in names or not re.fullmatch("[0-9a-f]{40}", row["git_tree"]):
            raise ValueError("dependency pins are duplicated or incomplete")
        names.add(row["name"])
    if not set(dependencies["roots"]) <= names:
        raise ValueError("top-level dependency lacks an exact port pin")
    source_archives(document, root)
    return document


def probe(command, accepted=(0,)):
    """Read a discovered executable's actual version without creating build state.

    @param command Structured executable/arguments, never shell-interpolated text.
    @param accepted Exit codes appropriate to the tool's version-only invocation.
    @return Decoded combined output; missing/nonworking prerequisites fail early.
    """
    result = subprocess.run([str(value) for value in command], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            check=False, timeout=60)
    text = result.stdout.decode("utf-8", errors="replace")
    if result.returncode not in accepted:
        raise ValueError(f"required tool version probe failed: {command[0]}: {text}")
    return text.strip()


def executable(path, name):
    """Require an explicit/discovered existing tool without installing it externally.

    @param path Candidate external or repository-local executable.
    @param name Human-readable missing prerequisite identifier.
    @return Resolved actual tool path.
    """
    candidate = Path(path).resolve() if path else None
    # Discovering a missing tool must stop before an external installer or a
    # partially configured build can make the failure harder to diagnose.
    if candidate is None or not candidate.is_file():
        raise ValueError(f"required external prerequisite unavailable: {name}; supply its explicit override")
    return candidate


def discover_tools(profile, overrides=None):
    """Find explicit/PATH/installed tools and compare their actual pinned versions.

    @param profile Validated portable requirements, containing no machine paths.
    @param overrides Optional paths for cmake, visual_studio, vcpkg, grfcodec, git, patch, windows_sdk.
    @return Actual path/version metadata; discovery reserves no run and writes nothing.
    """
    overrides = dict(overrides or {})
    expected = profile["tools"]
    vs = overrides.get("visual_studio")
    if not vs:
        locator = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
        if locator.is_file():
            vs = probe([locator, "-latest", "-products", "*", "-version", "[17.0,18.0)",
                        "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"])
    if not vs:
        raise ValueError("Visual Studio 2022 C++ BuildTools unavailable; supply visual_studio override")
    vs = Path(vs).resolve()
    toolset = vs / "VC/Tools/MSVC" / expected["toolset"]
    compiler = executable(toolset / "bin/Hostx64/x64/cl.exe", "pinned MSVC x64 compiler")
    msbuild = executable(vs / "MSBuild/Current/Bin/MSBuild.exe", "MSBuild")
    cmake = executable(overrides.get("cmake") or shutil.which("cmake") or
                       vs / "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe", "CMake")
    git = executable(overrides.get("git") or shutil.which("git"), "read-only Git source reader")
    patch_tool = executable(overrides.get("patch") or shutil.which("patch") or git.parent.parent / "usr/bin/patch.exe",
                            "GNU patch for dependency patches without Git mutation")
    vcpkg_root = overrides.get("vcpkg") or os.environ.get("VCPKG_ROOT")
    if not vcpkg_root:
        located = shutil.which("vcpkg")
        vcpkg_root = str(Path(located).parent) if located else None
    vcpkg = executable(Path(vcpkg_root) / "vcpkg.exe" if vcpkg_root else None, "pinned vcpkg executable/root")
    sdk = Path(overrides.get("windows_sdk") or Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Windows Kits/10")
    if overrides.get("windows_sdk"):
        sdk = Path(overrides["windows_sdk"])
    executable(sdk / "bin" / expected["windows_sdk"] / "x64/rc.exe", "pinned Windows SDK")
    if not (toolset / "atlmfc/include/atlbase.h").is_file():
        raise ValueError("required Visual Studio ATL component unavailable")
    grf_directory = overrides.get("grfcodec")
    result = {}
    candidates = dict(cmake=(cmake, ["--version"], r"cmake version ([^\s]+)", (0,)),
                      compiler=(compiler, [], r"Compiler Version ([^\s]+)", (0, 2)),
                      msbuild=(msbuild, ["-version", "-nologo"], r"([0-9]+\.[0-9.]+)", (0,)),
                      vcpkg=(vcpkg, ["version", "--disable-metrics"], r"version ([^\s]+)", (0,)))
    # Version probes describe the selected binaries themselves, never stale
    # CMake cache strings. An unavailable/mismatched prerequisite stops early.
    for name, (path, arguments, pattern, accepted) in candidates.items():
        output = probe([path, *arguments], accepted)
        match = re.search(pattern, output)
        if match is None or match.group(1) != expected[name]:
            raise ValueError(f"required {name} version {expected[name]} unavailable: {output}")
        result[name] = dict(path=str(path), version=match.group(1))
    for name in ("grfcodec", "nforenum", "grfid"):
        candidate = Path(grf_directory) / (name + ".exe") if grf_directory else shutil.which(name)
        path = executable(candidate, name)
        if name != "grfid":
            output = probe([path, "-v" if name == "grfcodec" else "--version"])
            if expected[name] not in output:
                raise ValueError(f"required {name} version {expected[name]} unavailable: {output}")
        result[name] = dict(path=str(path), version=expected.get(name, "paired GRFCodec distribution"))
    result.update(git=dict(path=str(git), version=probe([git, "--version"])),
                  patch=dict(path=str(patch_tool), version=probe([patch_tool, "--version"]).splitlines()[0]),
                  visual_studio=dict(path=str(vs), toolset=expected["toolset"]),
                  windows_sdk=dict(path=str(sdk.resolve()), version=expected["windows_sdk"]))
    result["vcpkg"]["root"] = str(vcpkg.parent)
    if result["patch"]["version"] != expected["patch"]:
        raise ValueError("required GNU patch version unavailable")
    return result


def build_directory(profile, root, override=None):
    """Select only an explicitly disposable build owner within the current checkout.

    @param profile Validated portable default directory.
    @param root Authorized checkout.
    @param override Optional explicit build directory for a rehearsal/isolated invocation.
    @return Contained local build path, whether or not it exists yet.
    """
    path = Path(override or profile["build_directory"])
    candidate = contained(path if path.is_absolute() else root / path, root)
    if candidate.relative_to(root).parts[0] not in ("build", "test-cache"):
        raise ValueError("native build requires the build or test-cache disposable owner")
    return candidate


def source_epoch(root, profile_path=None):
    """Capture actual native/build and qualifier sources without archived fixture trees.

    @param root Selected source checkout before a build begins.
    @param profile_path Explicit portable profile selection, or the versioned default.
    @return Native source epoch with complete current source-file identities.
    """
    selected = Path(profile_path or PROFILE)
    selected = contained(selected if selected.is_absolute() else root / selected, root)
    paths = {root / "CMakeLists.txt", root / "vcpkg.json", selected}
    profile = json.loads(selected.read_text(encoding="utf-8"))
    paths.add(root / profile["dependencies"]["source_bundle"]["path"])
    # Immutable historical source fixtures are data, not translation units.
    # Capture live native/build/language/resource owners and explicit Python
    # qualification/continuation adapters without walking old output archives.
    for name in ("src", "cmake", "os", "media", "bin"):
        for path in (root / name).rglob("*"):
            if not path.is_file():
                continue
            relative = path.relative_to(root).as_posix()
            if relative.startswith("src/tests/fixtures/"):
                continue
            if path.suffix in (".cpp", ".c", ".h", ".hpp", ".cmake", ".in", ".rc", ".manifest") or path.name == "CMakeLists.txt" or relative.startswith("src/lang/") and path.suffix == ".txt":
                paths.add(path)
    paths.update(path for directory in ("src/tests/multilayer/support", "src/tests/multilayer/acceptance")
                 for path in (root / directory).glob("*.py"))
    return dict(_documentation="Owner: r2fc-file-cleanup3.md13; actual current live build/qualification source epoch.",
                schema_version=1, kind="native_source_epoch",
                files=[identity(path, root) for path in sorted(paths)])


def identity(path, root):
    """Hash one actual current source/output below the selected checkout.

    @param path Existing contained source, tool receipt or target image.
    @param root Explicit checkout.
    @return Exact repository-relative identity; rebuilt images have new actual hashes.
    """
    path = contained(path, root)
    return dict(path=path.relative_to(root).as_posix(), sha256=file_identity(path)[0])


def write_document(path, document, root):
    """Write generated current-run metadata and return its actual byte identity.

    @param path Explicit generated destination whose role parent already exists.
    @param document Actual observed state, never a historical expected baseline.
    @param root Authorized checkout.
    @return Exact written metadata identity.
    """
    contained(path, root).write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return identity(path, root)


def run_step(context, stage, command, environment, timeout=7200):
    """Run one selected build stage and retain actual merged output/exit status.

    @param context Explicit current generated build evidence owner.
    @param stage Unique serial stage label within this invocation.
    @param command Structured tool argv, never a shell command string.
    @param environment Local caches/temp and disabled metrics/node reuse.
    @param timeout Separate build budget in seconds, independent of native test caps.
    @return Actual stage outcome; failure is recorded before being propagated.
    """
    terminal = context.output("evidence", stage + ".txt")
    started = time.monotonic()
    with terminal.open("xb") as stream:
        result = subprocess.run([str(value) for value in command], cwd=context.root, env=environment,
                                stdout=stream, stderr=subprocess.STDOUT, timeout=timeout, check=False)
    record = dict(stage=stage, argv=[str(value) for value in command], exit_status=result.returncode,
                  elapsed_seconds=round(time.monotonic() - started, 3), terminal=identity(terminal, context.root))
    write_document(context.reports / (stage + ".json"), record, context.root)
    if result.returncode:
        raise ValueError(f"native build stage failed ({result.returncode}): {stage}; see {terminal}")
    return record


def matching_configuration(profile, tools, provider, build, root):
    """Check observed cache output against explicit portable/current machine intent.

    @param profile Portable build requirements; the cache never supplies these values.
    @param tools Actually discovered compatible tool installation paths.
    @param provider Pinned current local dependency provider.
    @param build Selected disposable build directory.
    @param root Selected source checkout.
    @return True only when generated configuration belongs to this exact selection.
    """
    cache = build / "CMakeCache.txt"
    if not cache.is_file():
        return False
    fields = {}
    # Read generated configuration solely to decide whether it must be replaced.
    # Every controlling value still comes from the explicit profile/tool discovery.
    for line in cache.read_text(encoding="utf-8").splitlines():
        if re.match(r"^[A-Za-z_][A-Za-z0-9_]*:[^=]+=", line):
            key, value = line.split("=", 1)
            fields[key.split(":", 1)[0]] = value.replace("\\", "/").casefold()
    expected = dict(CMAKE_HOME_DIRECTORY=str(root), CMAKE_GENERATOR=profile["generator"],
        CMAKE_GENERATOR_PLATFORM=profile["architecture"],
        CMAKE_GENERATOR_TOOLSET="v143,version=" + profile["tools"]["toolset"],
        CMAKE_GENERATOR_INSTANCE=tools["visual_studio"]["path"],
        CMAKE_SYSTEM_VERSION=profile["tools"]["windows_sdk"],
        CMAKE_TOOLCHAIN_FILE=str(provider / "scripts/buildsystems/vcpkg.cmake"),
        Z_VCPKG_ROOT_DIR=str(provider), VCPKG_INSTALLED_DIR=str(build / "vcpkg_installed"),
        VCPKG_MANIFEST_INSTALL="OFF", VCPKG_TARGET_TRIPLET=profile["dependencies"]["target_triplet"],
        **profile["options"])
    expected.update({key.upper() + "_EXECUTABLE": tools[key]["path"] for key in ("grfcodec", "grfid", "nforenum")})
    return all(fields.get(key) == value.replace("\\", "/").casefold() for key, value in expected.items())


def configure_command(profile, tools, provider, build, root):
    """Construct a cache-independent configure using the selected actual toolchain.

    @param profile Portable generator/options/target-triplet requirements.
    @param tools Actual discovered compatible external prerequisites.
    @param provider Pinned local dependency provider.
    @param build Explicit local build directory, including a previously missing one.
    @param root Source checkout.
    @return Structured CMake configure argv with no source publication or Git write.
    """
    # Refresh missing/mismatched machine configuration, preserving generated PCH
    # headers for an exact current match. Unconditional --fresh would force a
    # complete engine recompile on every otherwise incremental recipe build.
    fresh = [] if matching_configuration(profile, tools, provider, build, root) else ["--fresh"]
    return [tools["cmake"]["path"], *fresh, "-S", str(root), "-B", str(build), "-G", profile["generator"],
            "-A", profile["architecture"], "-T", "v143,version=" + profile["tools"]["toolset"],
            "-DCMAKE_GENERATOR_INSTANCE=" + tools["visual_studio"]["path"],
            "-DCMAKE_SYSTEM_VERSION=" + profile["tools"]["windows_sdk"],
            "-DCMAKE_TOOLCHAIN_FILE=" + str(provider / "scripts/buildsystems/vcpkg.cmake"),
            "-DZ_VCPKG_ROOT_DIR=" + str(provider),
            "-DVCPKG_INSTALLED_DIR=" + str(build / "vcpkg_installed"), "-DVCPKG_MANIFEST_INSTALL=OFF",
            "-DVCPKG_TARGET_TRIPLET=" + profile["dependencies"]["target_triplet"],
            *["-D" + key + "=" + value for key, value in profile["options"].items()],
            *["-D" + key.upper() + "_EXECUTABLE=" + tools[key]["path"] for key in ("grfcodec", "grfid", "nforenum")]]


def build_selected(profile_path, configuration, targets, overrides=None, root=None, build_override=None):
    """Build only explicit existing targets sequentially and publish actual current proof.

    @param profile_path Explicit portable profile, or the versioned default.
    @param configuration One independent selected configuration.
    @param targets Ordered existing target names, never a wildcard/all target.
    @param overrides Optional explicit external tool installation paths.
    @param root Optional selected checkout, including a cold transfer rehearsal.
    @param build_override Optional contained disposable build directory.
    @return Exact native_build_result identity after all actual stages succeed.
    """
    root = repository_root(root)
    profile = load_profile(profile_path, root)
    if configuration not in CONFIGURATIONS:
        raise ValueError("select an explicit supported native configuration")
    cmake_source = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    # Only named executable producers may compile; selecting all/default would
    # unnecessarily rebuild unrelated scenarios and obscure their provenance.
    if not targets or len(set(targets)) != len(targets) or any(not re.fullmatch(r"openttd(?:_[a-z0-9_]+)?", name) or "add_executable(" + name not in cmake_source for name in targets):
        raise ValueError("select explicit existing native executable targets")
    tools = discover_tools(profile, overrides)
    build = build_directory(profile, root, build_override)
    existing = verify_installed(profile, build / "vcpkg_installed", root, allow_partial=True)
    epoch = source_epoch(root, profile_path)
    context = create_run_context("build", "selected-native", root=root)
    build.mkdir(parents=True, exist_ok=True)
    temporary = contained(build / ".native-work/temp", root)
    temporary.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ, TEMP=str(temporary), TMP=str(temporary), GIT_OPTIONAL_LOCKS="0",
        VCPKG_DISABLE_METRICS="1", VCPKG_MAX_CONCURRENCY="2", CMAKE_BUILD_PARALLEL_LEVEL="2",
        MSBUILDDISABLENODEREUSE="1", VCPKG_VISUAL_STUDIO_PATH=tools["visual_studio"]["path"])
    environment["PATH"] = str(Path(tools["cmake"]["path"]).parent) + os.pathsep + environment.get("PATH", "")
    provider = prepare_provider(profile, tools, build, root)
    environment["VCPKG_ROOT"] = str(provider)
    steps = []
    if existing is None:
        steps.append(run_step(context, "dependencies", dependency_command(profile, provider, build), environment))
    tools["dependencies"] = verify_installed(profile, build / "vcpkg_installed", root)
    steps.append(run_step(context, "configure", configure_command(profile, tools, provider, build, root), environment))
    # Complete one selected target before the next, bounding both MSBuild and
    # compiler fan-out. No owned idle worker may survive to block cleanup.
    for target in targets:
        steps.append(run_step(context, "build-" + target, [tools["cmake"]["path"], "--build", str(build),
            "--config", configuration, "--target", target, "--parallel", "2", "--", "/nodeReuse:false", "/p:CL_MPCount=2"], environment))
    # Refuse a build whose source changed during compilation. Publication is
    # about the measured epoch, never merely an EXE left in a known directory.
    for reference in epoch["files"]:
        if identity(root / reference["path"], root) != reference:
            raise ValueError("native source changed during the selected build")
    epoch_reference = write_document(context.data / "source-epoch.json", epoch, root)
    selected_profile = Path(profile_path or PROFILE)
    profile_reference = identity(selected_profile if selected_profile.is_absolute() else root / selected_profile, root)
    result = dict(_documentation="Owner: r2fc-file-cleanup3.md13; actual selected native build, never gameplay qualification.",
        schema_version=1, kind="native_build_result", complete=True, configuration=configuration,
        build_directory=build.relative_to(root).as_posix(),
        targets={target: identity(build / configuration / (target + ".exe"), root) for target in targets},
        source_epoch=epoch_reference, tool_profile=profile_reference, tools=tools, options=profile["options"], steps=steps)
    return write_document(context.reports / "native-build.json", result, root)


def main():
    """Inspect prerequisites read-only or execute one explicitly selected native build.

    @return None; actual build failures propagate with their retained stage log path.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", type=Path)
    parser.add_argument("--configuration", choices=CONFIGURATIONS)
    parser.add_argument("--target", action="append", default=[])
    parser.add_argument("--build-directory", type=Path)
    parser.add_argument("--inspect", action="store_true")
    for key in ("cmake", "visual-studio", "vcpkg", "grfcodec", "git", "patch", "windows-sdk"):
        parser.add_argument("--" + key, type=Path)
    args = parser.parse_args()
    overrides = {key: value for key, value in vars(args).items()
                 if key in ("cmake", "visual_studio", "vcpkg", "grfcodec", "git", "patch", "windows_sdk") and value is not None}
    if args.inspect:
        profile = load_profile(args.profile)
        result = dict(profile=profile, tools=discover_tools(profile, overrides))
    else:
        if args.configuration is None:
            parser.error("execution requires --configuration and at least one --target")
        result = build_selected(args.profile, args.configuration, args.target, overrides, build_override=args.build_directory)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
