"""Explicit invocation transport for multi-layer native acceptance runners.

@file runner.py Reusable runner support; no subprocess monkey-patching.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Groups 3-5.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
"""

import os
import hashlib
from pathlib import Path
import subprocess
import shutil

from artifact_paths import create_run_context, resolve_input_path

_invocation = None


def console_executable(context, executable):
    """Prepare a disposable Windows console copy with its exact build dependencies.

    @param context Process-local runtime owner for generated executable bytes.
    @param executable Existing repository build's GUI executable.
    @return Runtime console executable, or the original executable on other hosts.
    """
    source = Path(executable).resolve()
    if not source.is_relative_to(context.root / "build"):
        raise ValueError("console source must be a repository build executable")
    if os.name != "nt":
        return source
    context.prepare_runtime()
    encoded = bytearray(source.read_bytes())
    header = int.from_bytes(encoded[60:64], "little")
    # Only a valid GUI/console PE image may be projected into this data home;
    # the subsystem byte changes output handling, never gameplay code.
    if encoded[header:header + 4] != b"PE\0\0" or int.from_bytes(encoded[header + 92:header + 94], "little") not in (2, 3):
        raise ValueError("native executable has no supported PE subsystem")
    encoded[header + 92:header + 94] = (3).to_bytes(2, "little")
    target = context.runtime_dir / "openttd_console.exe"
    target.write_bytes(encoded)
    # Windows searches beside the launched image for its runtime DLLs. Copy
    # the matching build dependencies rather than using installed user content.
    for dependency in source.parent.glob("*.dll"):
        shutil.copyfile(dependency, context.runtime_dir / dependency.name)
    return target


def capture_path(context, name):
    """Find one exact native capture within this invocation's feature views.

    @param context Explicit invocation that produced the requested frame.
    @param name Exact capture basename, including the native container suffix.
    @return Unique path; absent or ambiguous captures fail rather than reuse history.
    """
    matches = context.capture_paths(name)
    if len(matches) != 1:
        raise ValueError(f"expected one current-run capture {name}, found {len(matches)}")
    return matches[0]


def report_provenance(context, report, producer, configuration):
    """Add traceable routing identities to a native semantic capture report.

    @param context Invocation whose current outputs the report describes.
    @param report Existing semantic report, including frames and source identity.
    @param producer Executed Python entry point inside the checkout.
    @param configuration Actual native build configuration.
    @return Same report with validated routing fields; frame pixels are untouched.
    """
    report.update(producer=Path(producer).resolve().relative_to(context.root).as_posix(),
                  feature=context.feature, scenario=context.scenario,
                  run_id=context.run_id, configuration=configuration)
    # A combined gallery retains each frame's real feature owner. Only the
    # reserved invocation may supply images, even if another run has the name.
    for frame in report["frames"]:
        path = (context.root / frame["path"]).resolve()
        parts = path.relative_to(context.root / "mlm/reviews").parts
        if len(parts) != 5 or parts[2] != context.run_id or parts[3] != "images":
            raise ValueError("report frame is outside its invocation's image directories")
        if hashlib.sha256(path.read_bytes()).hexdigest() != frame["sha256"]:
            raise ValueError("report frame changed before provenance was recorded")
        frame.update(feature=parts[0], scenario=parts[1], run_id=parts[2])
    return report


def current_run_context(feature, scenario):
    """Share one explicit invocation across cooperating imported runner modules.

    @param feature Caller-declared player-facing owner.
    @param scenario Caller-declared acceptance case.
    @return Feature view sharing the first module's reserved invocation.
    """
    global _invocation
    if _invocation is None:
        _invocation = create_run_context(feature, scenario)
    return _invocation.for_feature(feature, scenario)


def native_command(context, command):
    """Resolve native config/input arguments without mutating source templates.

    @param context Explicit invocation and native process role.
    @param command Structured argument list, including executable or shell wrapper.
    @return New argument list using the materialized local config and exact input.
    """
    result = [str(value) for value in command]
    context.prepare_runtime()
    # Only native option/value pairs are rewritten. Argument boundaries remain
    # structured, so paths and shell metacharacters are never evaluated as code.
    for index in range(len(result) - 1):
        value = Path(result[index + 1])
        if result[index] == "-c":
            # A caller-created process config already has the correct owner;
            # otherwise copy the named repository template into this home.
            if value.is_absolute() and value.resolve().is_relative_to(context.runtime_dir):
                continue
            result[index + 1] = str(context.prepare_runtime(value.name))
        elif result[index] == "-g" and value.suffix.lower() in (".sav", ".scn"):
            result[index + 1] = str(resolve_input_path(value, root=context.root))
    return result


def run_native(context, command, **kwargs):
    """Run one bounded native child with explicit artifact and runtime ownership.

    @param context Invocation selected by the caller, never inferred from filenames.
    @param command Structured executable/argument sequence.
    @param kwargs Existing subprocess options, including timeout and log streams.
    @return subprocess.CompletedProcess with the original caller's failure policy.
    """
    options = dict(kwargs)
    options["env"] = context.environment(options.get("env"))
    # Native helpers must not display an extra terminal on Windows; actual GUI
    # backends remain selectable by the test's explicit video arguments.
    if os.name == "nt" and "creationflags" not in options:
        options["creationflags"] = subprocess.CREATE_NO_WINDOW
    return subprocess.run(native_command(context, command), **options)


def start_native(context, command, **kwargs):
    """Start a separately owned native peer for a bounded network scenario.

    @param context Host/client context with its own writable data home.
    @param command Structured executable/argument sequence.
    @param kwargs Popen options; the caller retains wait/termination responsibility.
    @return Live subprocess.Popen managed by the caller's existing finally block.
    """
    options = dict(kwargs)
    options["env"] = context.environment(options.get("env"))
    if os.name == "nt" and "creationflags" not in options:
        options["creationflags"] = subprocess.CREATE_NO_WINDOW
    return subprocess.Popen(native_command(context, command), **options)
