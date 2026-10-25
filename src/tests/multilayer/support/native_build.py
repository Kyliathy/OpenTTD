"""Build selected existing native targets from an explicit portable profile.

@file native_build.py Read-only tool discovery; generated build/run evidence is disposable.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups13/15/30.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group28 complete source inventory.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group30 retained runtime distribution.
No CMakeCache is transferred as authority. Builds use one worker by default and at most two, preserve
read-only Git version discovery and record actual current source/image identities.
"""

import argparse
import ctypes
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

from artifact_paths import contained, create_run_context, repository_root
from artifact_inventory import file_identity
from native_build_dependencies import (dependency_command, prepare_provider, source_archives, verify_installed, write_exact)
from runner import stop_owned_process_tree
from owned_process_tree import close_owned_job, launch_owned_process

ROOT = Path(__file__).resolve().parents[4]
GIB = 1024 ** 3
# The user directs continued one-worker builds with the available desktop RAM.
# Keep4GiB physically free rather than blocking cached configuration at6GiB;
# the separate commit reserve and owned-process watchdog remain in force.
PHYSICAL_RESERVE = 4 * GIB
COMMIT_RESERVE = 8 * GIB
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
    runtime_asset_sources(document, root)
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


def runtime_asset_sources(profile, root):
    """Verify exact retained runtime data independently of an old build directory.

    @param profile Portable profile declaring optional pinned distribution inputs.
    @param root Checkout owning immutable user-provided base graphics and audio.
    @return Ordered pairs of declarations and verified fixture source paths.
    """
    result = []
    destinations = set()
    # Native observers select original_dos explicitly. Its proprietary input
    # bytes must travel as retained fixtures, rather than survive only in build/.
    for reference in profile["dependencies"].get("runtime_assets", []):
        if set(reference) != {"path", "sha256", "destination"}:
            raise ValueError("runtime asset needs exact source, hash and destination")
        source = contained(root / reference["path"], root / "src/tests/fixtures")
        parts = Path(reference["destination"]).parts
        # Only base-set files may populate this distribution seam. Duplicate or
        # redirected entries could replace another runtime input before a test.
        if len(parts) != 2 or parts[0] != "baseset" or reference["destination"].casefold() in destinations:
            raise ValueError("runtime asset requires a unique baseset destination")
        destinations.add(reference["destination"].casefold())
        if file_identity(source)[0] != reference["sha256"]:
            raise ValueError("retained runtime asset changed")
        result.append((reference, source))
    return result


def prepare_runtime_assets(profile, build, root):
    """Materialize pinned immutable distribution inputs into the current build.

    @param profile Portable profile with exact retained fixture identities.
    @param build Current contained disposable distribution owner.
    @param root Checkout owning the fixture sources and generated build.
    @return None; conflicting existing bytes refuse instead of being overwritten.
    """
    # Complete validation precedes all copies; a missing later asset must not
    # produce a partial distribution that appears ready for native observation.
    for reference, source in runtime_asset_sources(profile, root):
        destination = contained(build / reference["destination"], build)
        payload = source.read_bytes()
        if file_identity(source)[0] != reference["sha256"]:
            raise ValueError("runtime asset changed while preparing distribution")
        write_exact(destination, payload)
        if file_identity(destination)[0] != reference["sha256"]:
            raise ValueError("prepared runtime asset identity differs")


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
    paths.update(source for _reference, source in runtime_asset_sources(profile, root))
    # Immutable historical source fixtures are data, not translation units.
    # Qualification scripts, their sole catalogue and durable art contracts
    # influence what a current executable may certify just as native sources
    # do. Walk live owners only; generated reviews/runtime never join an epoch.
    for name in ("src", "cmake", "os", "media", "bin", "mlm/tools"):
        for path in (root / name).rglob("*"):
            # Interpreter caches are generated diagnostics, never authored art
            # or qualification source. Importing a helper must not stale a build.
            if not path.is_file() or "__pycache__" in path.parts or path.suffix in (".pyc", ".pyo"):
                continue
            relative = path.relative_to(root).as_posix()
            if relative.startswith("src/tests/fixtures/"):
                continue
            # Native/build inputs, every live Python controller and original
            # art bytes must all stay unchanged while evidence uses this build.
            # Language text is executable resource input; unrelated prose is not.
            if (path.suffix in (".cpp", ".cc", ".cxx", ".c", ".h", ".hh", ".hxx", ".hpp", ".inc", ".cmake", ".in", ".rc", ".manifest", ".py")
                    or path.name == "CMakeLists.txt"
                    or relative.startswith("media/")
                    or relative.startswith("src/lang/") and path.suffix == ".txt"):
                paths.add(path)
    catalogue = root / "mlm/review-producers.json"
    if catalogue.is_file():
        paths.add(catalogue)
    # These versioned contracts select wire schemas, independent sprite facts
    # and portable configuration. Historical source/saved-world fixture trees
    # remain separately hash-bound inputs, not live source inventory.
    for name in ("config", "graphics", "saveload"):
        paths.update(path for path in (root / "src/tests/fixtures" / name).rglob("*") if path.is_file())
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


def verify_source_epoch(expected, root, profile_path=None):
    """Require exact source membership and bytes for a still-current native build.

    @param expected Original complete native_source_epoch captured before compilation.
    @param root Explicit selected checkout, never a workstation cache.
    @param profile_path Exact portable profile used by that build.
    @return None; added, removed or changed translation/qualification files refuse reuse.
    """
    # Checking only old filenames misses newly added build inputs. Reuse the
    # original inventory policy for both membership and bytes in a single pass.
    if source_epoch(root, profile_path) != expected:
        raise ValueError("current recipe build source changed or source inventory was added/removed")


def write_document(path, document, root):
    """Write generated current-run metadata and return its actual byte identity.

    @param path Explicit generated destination whose role parent already exists.
    @param document Actual observed state, never a historical expected baseline.
    @param root Authorized checkout.
    @return Exact written metadata identity.
    """
    contained(path, root).write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return identity(path, root)


class MemoryStatus(ctypes.Structure):
    """Actual Windows MEMORYSTATUSEX fields; process working-set sums cannot measure available commit."""

    _fields_ = [("length", ctypes.c_ulong), ("load", ctypes.c_ulong),
                ("physical_total", ctypes.c_ulonglong), ("physical_available", ctypes.c_ulonglong),
                ("commit_total", ctypes.c_ulonglong), ("commit_available", ctypes.c_ulonglong),
                ("virtual_total", ctypes.c_ulonglong), ("virtual_available", ctypes.c_ulonglong),
                ("extended_available", ctypes.c_ulonglong)]


def memory_status():
    """Read real Windows physical/commit availability before admitting or continuing a selected build.

    @return Actual total/available byte counts; observation failure refuses the heavy operation.
    """
    if os.name != "nt":
        raise ValueError("selected native memory guard requires the declared Windows build platform")
    status = MemoryStatus()
    status.length = ctypes.sizeof(status)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
        raise ctypes.WinError()
    return {key: int(getattr(status, key)) for key in ("physical_total", "physical_available", "commit_total", "commit_available")}


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
    arguments = [str(value) for value in command]
    child, opening, closing, minimum = None, None, None, None
    samples, stopped, error = 0, False, None
    try:
        with terminal.open("xb") as stream:
            opening = memory_status()
            minimum = {key: opening[key] for key in ("physical_available", "commit_available")}
            # Preserve the user's desktop and commit headroom before starting
            # any compiler/provider process; existing EXEs cannot bypass refusal.
            if opening["physical_available"] < PHYSICAL_RESERVE or opening["commit_available"] < COMMIT_RESERVE:
                raise ValueError("selected native build blocked: requires 6GiB physical and 8GiB commit reserve")
            options = dict(cwd=context.root, env=environment, stdout=stream, stderr=subprocess.STDOUT)
            if os.name == "nt":
                options["creationflags"] = subprocess.CREATE_NO_WINDOW
            else:
                options["start_new_session"] = True
            child = launch_owned_process(arguments, **options)
            # Only this selected project's owned root is watched. Repeated real
            # reserve samples stop our compiler tree, never unrelated user apps.
            while child.poll() is None:
                closing = memory_status()
                samples += 1
                for key in minimum:
                    minimum[key] = min(minimum[key], closing[key])
                if closing["physical_available"] < PHYSICAL_RESERVE or closing["commit_available"] < COMMIT_RESERVE:
                    stopped = True
                    stop_owned_process_tree(child)
                    raise ValueError("selected native build stopped after crossing the physical/commit reserve")
                if time.monotonic() - started >= timeout:
                    raise subprocess.TimeoutExpired(arguments, timeout)
                try:
                    child.wait(timeout=min(2, max(0.01, timeout - (time.monotonic() - started))))
                except subprocess.TimeoutExpired:
                    pass
            child.wait(timeout=15)
    except BaseException as failure:
        error = str(failure)
        raise
    finally:
        if child is not None:
            close_owned_job(child)
            if child.poll() is None:
                stop_owned_process_tree(child)
            child.wait(timeout=15)
        record = dict(_documentation="Owner: r2fc-file-cleanup3.md; actual memory-guarded selected build stage, never gameplay proof.",
                      stage=stage, argv=arguments, exit_status=child.returncode if child is not None else None,
                      elapsed_seconds=round(time.monotonic() - started, 3), terminal=identity(terminal, context.root),
                      memory=dict(opening=opening, last_observed=closing, minimum=minimum, samples=samples,
                                  physical_reserve=PHYSICAL_RESERVE, commit_reserve=COMMIT_RESERVE, stopped_for_memory=stopped),
                      root_reaped=child.poll() is not None if child is not None else None, error=error)
        write_document(context.reports / (stage + ".json"), record, context.root)
    if child.returncode:
        raise ValueError(f"native build stage failed ({child.returncode}): {stage}; see {terminal}")
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
    cache = build / "CMakeCache.txt"
    if cache.is_file():
        fields = {}
        # Reconfigure compatible generated trees in place with every controlling
        # option supplied explicitly. --fresh recursively removes CMakeFiles and
        # is forbidden by the user's preservation rule, even inside a build tree.
        for line in cache.read_text(encoding="utf-8").splitlines():
            if re.match(r"^[A-Za-z_][A-Za-z0-9_]*:[^=]+=", line):
                key, value = line.split("=", 1)
                fields[key.split(":", 1)[0]] = value.replace("\\", "/").casefold()
        required = dict(CMAKE_HOME_DIRECTORY=str(root), CMAKE_GENERATOR=profile["generator"],
                        CMAKE_GENERATOR_PLATFORM=profile["architecture"],
                        CMAKE_GENERATOR_TOOLSET="v143,version=" + profile["tools"]["toolset"])
        if any(fields.get(key) != value.replace("\\", "/").casefold() for key, value in required.items()):
            raise ValueError("incompatible existing CMake tree; select a new contained build directory without deleting the old one")
    return [tools["cmake"]["path"], "-S", str(root), "-B", str(build), "-G", profile["generator"],
            "-A", profile["architecture"], "-T", "v143,version=" + profile["tools"]["toolset"],
            "-DCMAKE_GENERATOR_INSTANCE=" + tools["visual_studio"]["path"],
            "-DCMAKE_SYSTEM_VERSION=" + profile["tools"]["windows_sdk"],
            "-DCMAKE_TOOLCHAIN_FILE=" + str(provider / "scripts/buildsystems/vcpkg.cmake"),
            "-DZ_VCPKG_ROOT_DIR=" + str(provider),
            "-DVCPKG_INSTALLED_DIR=" + str(build / "vcpkg_installed"), "-DVCPKG_MANIFEST_INSTALL=OFF",
            "-DVCPKG_TARGET_TRIPLET=" + profile["dependencies"]["target_triplet"],
            *["-D" + key + "=" + value for key, value in profile["options"].items()],
            *["-D" + key.upper() + "_EXECUTABLE=" + tools[key]["path"] for key in ("grfcodec", "grfid", "nforenum")]]


def wait_for_compile_headroom(context, stage, workers, timeout=300, working_budget=None):
    """Wait briefly for compiler working memory in addition to unspendable reserves.

    @param context Current generated build evidence owner.
    @param stage Exact compiler/linker stage that has not started yet.
    @param workers Validated one/two-worker limit.
    @param timeout Maximum resource-admission wait in seconds; no child exists during it.
    @param working_budget Optional explicit incremental working allowance in bytes; live reserves remain unchanged.
    @return Exact admitted observation identity; timeout retains a refusal and raises.
    """
    started = time.monotonic()
    budget = 2 * GIB * workers if working_budget is None else working_budget
    # A cached no-change build can use a smaller startup allowance. Keep a
    # meaningful minimum and the original maximum; neither changes live reserves.
    if type(budget) is not int or not GIB // 2 <= budget <= 2 * GIB * workers:
        raise ValueError("compiler working allowance must be between512MiB and2GiB per selected worker")
    report = context.reports / (stage + "-admission.json")
    document = dict(_documentation="Owner: archi-historical-debug.md; m2o23-4 Group59 compiler working-memory admission, not a reserve reduction.",
                    kind="native_compile_admission", stage=stage, workers=workers,
                    physical_reserve=PHYSICAL_RESERVE, commit_reserve=COMMIT_RESERVE,
                    working_budget=budget, admitted=False, samples=[])
    # A compiler needs working memory above the desktop reserve. Repeatedly
    # launching at the bare floor only kills useful work; wait with no child
    # while keeping bounded, observable admission and the original runtime guard.
    while True:
        current = memory_status()
        elapsed = time.monotonic() - started
        document["samples"].append(dict(elapsed_seconds=round(elapsed, 3), **current))
        # Both physical and commit working budgets must coexist with the
        # protected reserves before any owned compiler/linker process is created.
        if current["physical_available"] >= PHYSICAL_RESERVE + budget and current["commit_available"] >= COMMIT_RESERVE + budget:
            document["admitted"] = True
            return write_document(report, document, context.root)
        write_document(report, document, context.root)
        if elapsed >= timeout:
            raise ValueError("compiler working headroom did not recover within its admission budget; see " + str(report))
        time.sleep(min(2, max(0.01, timeout - elapsed)))


def run_compile_step(context, stage, command, environment, workers, timeout=7200, admission_timeout=300, working_budget=None):
    """Admit working memory, then retain the ordinary owned/reserve-guarded compiler result.

    @param context Current build evidence owner.
    @param stage Exact unique compiler/linker stage.
    @param command Structured original CMake/MSBuild argv.
    @param environment Existing isolated tool environment.
    @param workers Existing one/two-worker limit.
    @param timeout Actual compilation budget after admission.
    @param admission_timeout Bounded pre-child resource wait.
    @param working_budget Optional explicit cached-build admission allowance; actual process memory stays guarded.
    @return Actual stage outcome with its separate admission observation.
    """
    admission = wait_for_compile_headroom(context, stage, workers, admission_timeout, working_budget)
    try:
        outcome = run_step(context, stage, command, environment, timeout)
    except BaseException:
        report = context.reports / (stage + ".json")
        if report.is_file():
            outcome = json.loads(report.read_text(encoding="utf-8"))
            outcome["compiler_admission"] = admission
            write_document(report, outcome, context.root)
        raise
    outcome["compiler_admission"] = admission
    write_document(context.reports / (stage + ".json"), outcome, context.root)
    return outcome


def generated_project_order(build, targets, root):
    """Read the configured CMake dependency order without inventing compiler flags.

    @param build Contained generated Visual Studio project directory.
    @param targets Explicit selected executable target names.
    @param root Checkout containing sources and generated projects.
    @return Dependency-first project rows with their exact generated source items.
    """
    ordered, active, visited = [], set(), set()

    def visit(project):
        """Keep generated tools ahead of consumers and reject foreign/cyclic projects.

        @param project One actual CMake-generated project reference.
        @return None; append each checked project once after its prerequisites.
        """
        project = contained(project, build)
        if project in active:
            raise ValueError("cyclic generated native project reference")
        if project in visited:
            return
        # Only the configured CMake graph may select compilation. A missing or
        # foreign project cannot turn an explicit target into an all/default build.
        if project.suffix != ".vcxproj" or not project.is_file():
            raise ValueError("missing/unsupported generated native project")
        active.add(project)
        tree = ET.parse(project)
        sources = []
        # Preserve CMake's actual project/source order. MSBuild still owns all
        # configuration-specific options, exclusions and generated prerequisites.
        for node in tree.iter():
            raw = node.attrib.get("Include")
            if not raw:
                continue
            if node.tag.endswith("}ProjectReference"):
                visit(Path(raw) if Path(raw).is_absolute() else project.parent / raw)
            elif node.tag.endswith("}ClCompile"):
                # SelectedFiles is an MSBuild item list; these metacharacters
                # cannot safely represent one literal source item in that list.
                if any(value in raw for value in ('"', ';', '\n', '\r')) or "$(" in raw:
                    raise ValueError("unresolved/unsafe generated native source item")
                contained(Path(raw) if Path(raw).is_absolute() else project.parent / raw, root)
                sources.append(raw)
        active.remove(project)
        visited.add(project)
        ordered.append(dict(project=project, target=project.stem, sources=list(dict.fromkeys(sources))))

    # Shared engine/code generators are visited once, before any selected leaf.
    for target in targets:
        visit(build / (target + ".vcxproj"))
    return ordered


def write_compile_response(path, project, configuration, sources, workers, root):
    """Bind one small selected-file compile to the original generated project.

    @param path Fresh current-run response-file destination.
    @param project Exact configured CMake project.
    @param configuration Explicit native configuration.
    @param sources Exact generated source item strings in this bounded burst.
    @param workers Validated one/two-worker limit for both compiler schedulers.
    @param root Checkout owning generated evidence.
    @return Exact response-file identity; no linker or alternative flags are used.
    """
    lines = ["# @see archi-historical-debug.md; m2o23-4 Group59 bounded source compilation.",
             '"' + str(project) + '"', "/t:ClCompile", "/p:Configuration=" + configuration,
             "/p:Platform=x64", "/p:BuildProjectReferences=false",
             '/p:SelectedFiles="' + ";".join(sources) + '"', "/p:UseMultiToolTask=true",
             f"/p:MultiProcMaxCount={workers}", f"/p:CL_MPCount={workers}",
             "/nodeReuse:false", "/verbosity:minimal", "/nologo"]
    contained(path, root).write_text("\n".join(lines) + "\n", encoding="utf-8")
    return identity(path, root)


def precompile_selected_projects(context, tools, build, configuration, targets, environment, root, batch_size, workers):
    """Release compiler/project state between explicit bounded source bursts.

    @param context Current build evidence owner; never a gameplay run.
    @param tools Actual portable-profile tool installations.
    @param build Configured CMake project directory.
    @param configuration Exact selected configuration.
    @param targets Explicit executable selection from build_selected.
    @param environment Existing isolated compiler environment.
    @param root Checkout whose current source epoch is already frozen.
    @param batch_size One through32source items per owned compiler invocation.
    @param workers Existing one/two-worker bound, independently of burst size.
    @return Actual successful preparation/compile/link steps; final target builds still run.
    """
    steps = []
    # Follow the actual CMake dependency graph so cold code generators complete
    # before selective compilation. Small projects keep their ordinary build;
    # large projects are compiled in bursts, then linked by CMake/MSBuild itself.
    for index, row in enumerate(generated_project_order(build, targets, root)):
        target, sources = row["target"], row["sources"]
        if len(sources) > batch_size:
            # CMake custom rules remain the sole generated-input authority;
            # selective ClCompile deliberately does not run another project's rules.
            stage = f"source-rules-{index:03}-{target}"
            steps.append(run_step(context, stage, [tools["msbuild"]["path"], str(row["project"]),
                "/t:CustomBuild", "/p:Configuration=" + configuration, "/p:Platform=x64",
                "/p:BuildProjectReferences=false", "/nodeReuse:false", "/verbosity:minimal", "/nologo"], environment))
            # A complete burst exits/reaps its owned tree before the next opens.
            # No memory reserve is relaxed and no partially compiled image qualifies.
            for offset in range(0, len(sources), batch_size):
                selected = sources[offset:offset + batch_size]
                stage = f"source-burst-{index:03}-{target}-{offset // batch_size:03}"
                response = context.data / (stage + ".rsp")
                reference = write_compile_response(response, row["project"], configuration, selected, workers, root)
                inputs = [identity(contained(Path(raw) if Path(raw).is_absolute() else row["project"].parent / raw, root), root)
                          for raw in selected]
                outcome = run_compile_step(context, stage, [tools["msbuild"]["path"], "@" + str(response)], environment, workers)
                # Bind the exact executed response and source bytes, including
                # generated revision code, before allowing the next burst/link.
                if identity(response, root) != reference or any(identity(root / item["path"], root) != item for item in inputs):
                    raise ValueError("selected compiler response/source changed during its burst")
                outcome["compile_selection"] = dict(response=reference, sources=inputs, links_image=False)
                write_document(context.reports / (stage + ".json"), outcome, root)
                steps.append(outcome)
        command = selected_target_command(tools, build, configuration, target, workers)
        stage = f"prepared-project-{index:03}-{target}"
        steps.append(run_compile_step(context, stage, command, environment, workers) if sources
                     else run_step(context, stage, command, environment))
    return steps


def selected_target_command(tools, build, configuration, target, workers):
    """Compile one project with bounded independent translation-unit processes.

    @param tools Actual discovered CMake installation from the portable profile.
    @param build Explicit contained generated project directory.
    @param configuration Exact selected native configuration.
    @param target One validated executable target, never the default/all project.
    @param workers Already validated one/two-worker scheduling bound.
    @return Structured CMake/MSBuild argv preserving the compiler and reserve policy.
    """
    # A long CL batch retained enough compiler state to cross desktop reserves.
    # MSBuild's installed MultiToolTask schedules separate source compilations;
    # cap that scheduler as well as CL, while completing only one project at once.
    return [tools["cmake"]["path"], "--build", str(build), "--config", configuration,
            "--target", target, "--parallel", "1", "--", "/nodeReuse:false",
            "/p:UseMultiToolTask=true", f"/p:MultiProcMaxCount={workers}", f"/p:CL_MPCount={workers}"]


def build_selected(profile_path, configuration, targets, overrides=None, root=None, build_override=None, workers=1, compile_batch_size=0, compile_admission_budget=None):
    """Build only explicit existing targets sequentially and publish actual current proof.

    @param profile_path Explicit portable profile, or the versioned default.
    @param configuration One independent selected configuration.
    @param targets Ordered existing target names, never a wildcard/all target.
    @param overrides Optional explicit external tool installation paths.
    @param root Optional selected checkout, including a cold transfer rehearsal.
    @param build_override Optional contained disposable build directory.
    @param workers One compiler worker by default; two is the explicit maximum, with one selected project at a time.
    @param compile_batch_size Optional1-32source-item bursts after observed project pressure; zero keeps ordinary incremental compilation.
    @param compile_admission_budget Optional512MiB-to2GiB-per-worker startup allowance for an existing dependency cache; runtime reserves are unchanged.
    @return Exact native_build_result identity after all actual stages succeed.
    """
    root = repository_root(root)
    profile = load_profile(profile_path, root)
    if configuration not in CONFIGURATIONS:
        raise ValueError("select an explicit supported native configuration")
    # Boolean flags are not worker counts; accepting them would produce an
    # invalid compiler option instead of the explicit bounded pressure policy.
    if type(workers) is not int or workers not in (1, 2):
        raise ValueError("selected native build requires one or two compiler workers")
    # Burst size bounds retained project/compiler state, not parallelism. Reject
    # unbounded/boolean selections before tool discovery or generated-tree writes.
    if type(compile_batch_size) is not int or compile_batch_size not in range(33):
        raise ValueError("selected native source burst requires zero or one through32items")
    # An explicit small allowance is for a known cached incremental build.
    # Reject invalid byte counts before tool discovery or any build preparation.
    if compile_admission_budget is not None and (type(compile_admission_budget) is not int
            or not GIB // 2 <= compile_admission_budget <= 2 * GIB * workers):
        raise ValueError("invalid cached compiler admission allowance")
    cmake_source = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    # Only named executable producers may compile; selecting all/default would
    # unnecessarily rebuild unrelated scenarios and obscure their provenance.
    if not targets or len(set(targets)) != len(targets) or any(not re.fullmatch(r"openttd(?:_[a-z0-9_]+)?", name) or "add_executable(" + name not in cmake_source for name in targets):
        raise ValueError("select explicit existing native executable targets")
    tools = discover_tools(profile, overrides)
    build = build_directory(profile, root, build_override)
    prepare_runtime_assets(profile, build, root)
    existing = verify_installed(profile, build / "vcpkg_installed", root, allow_partial=True)
    # A cold dependency build retains the conservative default. The smaller
    # allowance cannot silently apply to unprepared third-party compilation.
    if compile_admission_budget is not None and existing is None:
        raise ValueError("explicit compiler allowance requires an existing dependency cache")
    epoch = source_epoch(root, profile_path)
    context = create_run_context("build", "selected-native", root=root)
    build.mkdir(parents=True, exist_ok=True)
    temporary = contained(build / ".native-work/temp", root)
    temporary.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ, TEMP=str(temporary), TMP=str(temporary), GIT_OPTIONAL_LOCKS="0",
        VCPKG_DISABLE_METRICS="1", VCPKG_MAX_CONCURRENCY=str(workers), CMAKE_BUILD_PARALLEL_LEVEL="1",
        MSBUILDDISABLENODEREUSE="1", VCPKG_VISUAL_STUDIO_PATH=tools["visual_studio"]["path"])
    environment["PATH"] = str(Path(tools["cmake"]["path"]).parent) + os.pathsep + environment.get("PATH", "")
    provider = prepare_provider(profile, tools, build, root)
    environment["VCPKG_ROOT"] = str(provider)
    steps = []
    if existing is None:
        steps.append(run_step(context, "dependencies", dependency_command(profile, provider, build), environment))
    tools["dependencies"] = verify_installed(profile, build / "vcpkg_installed", root)
    steps.append(run_step(context, "configure", configure_command(profile, tools, provider, build, root), environment))
    if compile_batch_size:
        steps.extend(precompile_selected_projects(context, tools, build, configuration, targets,
                                                  environment, root, compile_batch_size, workers))
    # Complete one selected target before the next, bounding both MSBuild and
    # compiler fan-out. No owned idle worker may survive to block cleanup.
    for target in targets:
        steps.append(run_compile_step(context, "build-" + target,
                                      selected_target_command(tools, build, configuration, target, workers), environment, workers,
                                      working_budget=compile_admission_budget))
    # Refuse a build whose source changed during compilation. Publication is
    # about the measured epoch, never merely an EXE left in a known directory.
    verify_source_epoch(epoch, root, profile_path)
    epoch_reference = write_document(context.data / "source-epoch.json", epoch, root)
    selected_profile = Path(profile_path or PROFILE)
    profile_reference = identity(selected_profile if selected_profile.is_absolute() else root / selected_profile, root)
    result = dict(_documentation="Owner: r2fc-file-cleanup3.md13; actual selected native build, never gameplay qualification.",
        schema_version=1, kind="native_build_result", complete=True, configuration=configuration,
        build_directory=build.relative_to(root).as_posix(),
        targets={target: identity(build / configuration / (target + ".exe"), root) for target in targets},
        source_epoch=epoch_reference, tool_profile=profile_reference, tools=tools, options=profile["options"], workers=workers, steps=steps)
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
    parser.add_argument("--workers", type=int, choices=(1, 2), default=1, help="One selected project; one compiler worker by default, two maximum.")
    parser.add_argument("--compile-batch-size", type=int, choices=range(33), default=0,
                        help="Optional small source bursts after actual project-memory pressure; zero uses ordinary incremental compilation.")
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
        result = build_selected(args.profile, args.configuration, args.target, overrides,
                                build_override=args.build_directory, workers=args.workers, compile_batch_size=args.compile_batch_size)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
