"""Explicit invocation transport for multi-layer native acceptance runners.

@file runner.py Reusable runner support; no subprocess monkey-patching.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Groups 3-5.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups10/18B/19B; exact inputs and delegated native producers.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group25 atomic image reuse.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
"""

import os
import hashlib
from pathlib import Path
import subprocess
import signal
import json
import sys
import uuid

from artifact_paths import contained, create_run_context, resolve_input_path
from owned_process_tree import close_owned_job, launch_owned_process

_invocation = None


def save_native_capture(context, raster, source, target):
    """Convert a native capture while retaining its exact producer chain.

    @param context Explicit invocation containing the source native process report.
    @param raster Loaded native image; pixels must still equal the source container.
    @param source Actual PPM produced by the current invocation.
    @param target PNG in the same feature image directory.
    @return None; append hash-bound conversion provenance before PPM disposal.
    """
    from PIL import Image
    source = contained(source, context.root / "mlm/reviews")
    target = contained(target, source.parent)
    parts = source.relative_to(context.root / "mlm/reviews").parts
    # Container conversion cannot move a frame into another feature/invocation
    # or replace earlier evidence. Only the container and extension may change.
    if len(parts) != 5 or parts[2] != context.run_id or parts[3] != "images" or target.suffix != ".png" or target.exists():
        raise ValueError("capture conversion requires a fresh PNG beside its current-run source")
    original = {"path": source.relative_to(context.root).as_posix(),
                "sha256": hashlib.sha256(source.read_bytes()).hexdigest()}
    with Image.open(source) as native:
        assert raster.size == native.size and raster.tobytes() == native.tobytes(), "conversion changed native pixels"
    parents = []
    # Exact path plus hash links the conversion to the native process that
    # actually wrote these bytes. No filename-prefix or latest-run lookup.
    for report_path in context.reports.glob("native-*.json"):
        report = json.loads(report_path.read_text(encoding="utf-8"))
        # Native scene observations may share this prefix, but only a measured
        # process record can own the actual source container being converted.
        if not isinstance(report.get("artifacts"), list):
            continue
        if original in report["artifacts"]:
            parents.append({"path": report_path.relative_to(context.root).as_posix(),
                            "sha256": hashlib.sha256(report_path.read_bytes()).hexdigest()})
    if not parents:
        raise ValueError("native capture has no matching process provenance")
    raster.save(target)
    with Image.open(target) as encoded:
        assert encoded.size == raster.size and encoded.tobytes() == raster.tobytes(), "PNG changed native pixels"
    record = {"_documentation": "Owner: r2fc-file-cleanup.md; native-container conversion, no new rendering claim",
              "feature": parts[0], "scenario": parts[1], "run_id": context.run_id,
              "source_container": original, "native_reports": parents,
              "path": target.relative_to(context.root).as_posix(),
              "sha256": hashlib.sha256(target.read_bytes()).hexdigest(), "pixels_unchanged": True}
    owner = context.for_feature(parts[0], parts[1])
    with owner.output("reports", "capture-conversions.jsonl").open("a", encoding="utf-8") as sink:
        sink.write(json.dumps(record) + "\n")


def collect_native_screenshots(context):
    """Collect engine screenshot-command output into its caller's feature directory.

    @param context Explicit process data home and feature/scenario owner.
    @return Actual moved PNG paths; source/output escapes and collisions refuse.
    """
    directory = contained(context.runtime_dir / "screenshot", context.runtime_dir)
    captures = []
    # The engine keeps its standard data-home folder. Only screenshots from
    # this exact process are collected; other runs and user data are excluded.
    for candidate in directory.glob("*.png"):
        source = contained(candidate, context.runtime_dir)
        target = context.output("images", source.name)
        if target.exists():
            raise ValueError(f"engine capture would replace an existing frame: {target}")
        source.rename(target)
        captures.append(target)
    return captures


def _runtime_image(target, encoded):
    """Reuse identical image bytes or atomically replace this process's image.

    @param target Already-contained runtime EXE/DLL destination.
    @param encoded Exact current source bytes, including any console projection.
    @return None; replacing a directory entry preserves any historical peer links.
    """
    # Repeated observers in one process home must not rewrite a matching image.
    # A changed build receives a fresh entry so old hard-linked peers stay exact.
    if target.is_file() and target.read_bytes() == encoded:
        return
    temporary = target.with_name(target.name + "." + uuid.uuid4().hex + ".tmp")
    try:
        with temporary.open("xb") as sink:
            sink.write(encoded)
        temporary.replace(target)
    finally:
        temporary.unlink(missing_ok=True)


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
    _runtime_image(target, encoded)
    # Windows searches beside the launched image for its runtime DLLs. Copy
    # the matching build dependencies rather than using installed user content.
    for dependency in source.parent.glob("*.dll"):
        _runtime_image(context.runtime_dir / dependency.name, dependency.read_bytes())
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
        # Only a named native config is a configuration input. Python helper
        # -c code shares this spelling and must retain its structured argument.
        if result[index] == "-c" and value.suffix.lower() == ".cfg":
            # A caller-created process config already has the correct owner;
            # otherwise copy the named repository template into this home.
            if value.is_absolute() and value.resolve().is_relative_to(context.runtime_dir):
                continue
            result[index + 1] = str(context.prepare_runtime(value.name))
        elif result[index] == "-g" and value.suffix.lower() in (".sav", ".scn"):
            result[index + 1] = str(resolve_input_path(value, root=context.root))
    return result


def _transcript_starts(context, options):
    """Record exact byte starts of explicitly supplied seekable diagnostic streams.

    @param context Current feature/run owner; outside-checkout logs are refused.
    @param options Structured subprocess options before launching the child.
    @return Stream name to contained source path/start offset for regular files.
    """
    result = {}
    # Only actual named file streams provide byte boundaries. Inherited console
    # handles, DEVNULL and PIPE retain their normal subprocess behavior.
    for name in ("stdout", "stderr"):
        stream = options.get(name)
        filename = getattr(stream, "name", None)
        if not isinstance(filename, (str, os.PathLike)):
            continue
        source = contained(filename, context.root)
        if not source.is_file():
            continue
        stream.flush()
        start = os.lseek(stream.fileno(), 0, os.SEEK_CUR)
        result[name] = (source, start)
    return result


def _retain_transcripts(context, result, starts, process_id):
    """Copy only this child's observed output into its required evidence owner.

    @param context Explicit feature/scenario/run destination.
    @param result CompletedProcess containing any captured PIPE output.
    @param starts Explicit named file/start offsets recorded before launch.
    @param process_id Unique child identity shared with its native report.
    @return Evidence paths; optional diagnostic logs remain disposable copies.
    """
    retained = []
    # Each child owns distinct evidence, even when a caller appends diagnostics
    # to an older log. Prior bytes cannot masquerade as this process's terminal.
    for name in ("stdout", "stderr"):
        if name in starts:
            source, start = starts[name]
            payload = source.read_bytes()[start:]
        else:
            payload = getattr(result, name, None)
            if payload is None:
                continue
            if isinstance(payload, str):
                payload = payload.encode("utf-8")
        destination = context.output("evidence", f"native-{process_id}-{name}.txt")
        with destination.open("xb") as sink:
            sink.write(payload)
        retained.append(destination)
    return retained


def stop_owned_process_tree(child):
    """Stop only the supplied live process tree and reap its exact root handle.

    @param child Popen root created by this invocation, never a discovered process name.
    @return Whether the root has been reaped; already completed roots remain untouched.
    """
    close_owned_job(child)
    if child.poll() is not None:
        child.wait(timeout=15)
        return True
    if os.name == "nt":
        try:
            subprocess.run(["taskkill.exe", "/PID", str(child.pid), "/T", "/F"], check=False,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           creationflags=subprocess.CREATE_NO_WINDOW, timeout=15)
        except (OSError, subprocess.TimeoutExpired):
            # The exact root handle remains ours if the platform tree utility
            # is unavailable; its fallback kill and wait must still complete.
            pass
    elif getattr(child, "artifact_owned_session", False):
        # Only a session created for this exact root may receive a group signal;
        # caller/system process groups can never become a cleanup target.
        try:
            os.killpg(child.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    if child.poll() is None:
        child.kill()
    child.wait(timeout=15)
    return True


def _run_owned_process(arguments, options):
    """Communicate with one owned child while retaining timeout output and descendants.

    @param arguments Structured already-contained native executable/argument sequence.
    @param options Original subprocess.run options; caller failure policy is applied after evidence.
    @return CompletedProcess with timeout/root-reaping observations, never a native success claim.
    """
    options = dict(options)
    timeout = options.pop("timeout", None)
    incoming = options.pop("input", None)
    options.pop("check", None)
    if options.pop("capture_output", False):
        # Captured and externally supplied streams are different evidence owners;
        # conflicting requests must refuse before a child acquires either one.
        if options.get("stdout") is not None or options.get("stderr") is not None:
            raise ValueError("stdout/stderr cannot be combined with capture_output")
        options.update(stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if incoming is not None:
        if options.get("stdin") is not None:
            raise ValueError("stdin cannot be combined with input")
        options["stdin"] = subprocess.PIPE
    if os.name != "nt":
        options.setdefault("start_new_session", True)
    child = launch_owned_process(arguments, **options)
    timed_out = False
    try:
        try:
            stdout, stderr = child.communicate(incoming, timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            stop_owned_process_tree(child)
            stdout, stderr = child.communicate(timeout=15)
    finally:
        # Interruptions and communication failures cannot leave this invocation's
        # native descendants alive or terminate an unrelated application instead.
        close_owned_job(child)
        if child.poll() is None:
            stop_owned_process_tree(child)
        child.wait(timeout=15)
    result = subprocess.CompletedProcess(arguments, child.returncode, stdout, stderr)
    result.timed_out = timed_out
    result.root_reaped = child.poll() is not None
    return result


def run_native(context, command, *, native_producer=None, **kwargs):
    """Run one bounded native child with explicit artifact and runtime ownership.

    @param context Invocation selected by the caller, never inferred from filenames.
    @param command Structured executable/argument sequence.
    @param native_producer Optional actual repository Python producer, distinct from a dispatcher's entry point.
    @param kwargs Existing subprocess options, including timeout and log streams.
    @return subprocess.CompletedProcess with the original caller's failure policy.
    """
    entrypoint = Path(sys.argv[0]).resolve()
    entrypoint_name = entrypoint.relative_to(context.root).as_posix() if entrypoint.is_relative_to(context.root) else "external-caller"
    producer_name = entrypoint_name
    if native_producer is not None:
        owner = contained(context.root / native_producer, context.root)
        # Delegation names the actual executable Python wrapper, never an
        # outside-workspace file or a document masquerading as native producer.
        if owner.suffix != ".py" or not owner.is_file():
            raise ValueError("explicit native producer must be an existing repository Python source")
        producer_name = owner.relative_to(context.root).as_posix()
    options = dict(kwargs)
    options["env"] = context.environment(options.get("env"))
    # Native helpers must not display an extra terminal on Windows; actual GUI
    # backends remain selectable by the test's explicit video arguments.
    if os.name == "nt" and "creationflags" not in options:
        options["creationflags"] = subprocess.CREATE_NO_WINDOW
    arguments = native_command(context, command)
    executable = Path(arguments[0]).resolve()
    initial_image = None
    if executable.is_file() and executable.is_relative_to(context.root):
        initial_image = {"path": executable.relative_to(context.root).as_posix(),
                         "sha256": hashlib.sha256(executable.read_bytes()).hexdigest()}
    writer = context.root / producer_name
    initial_writer = hashlib.sha256(writer.read_bytes()).hexdigest() if writer.is_file() else None
    configurations = [name for name in ("Debug", "RelWithDebInfo") if name in Path(arguments[0]).parts]
    configuration = configurations[0] if len(configurations) == 1 else options["env"].get("OTTD_TEST_CONFIGURATION", "unknown")
    from sprite_native_inputs import observe_expected_payload, observe_candidate_baseset
    bound_input = observe_expected_payload(context, options["env"], configuration)
    bound_candidate = observe_candidate_baseset(context, options["env"], configuration)
    bound_inputs = [row for row in (bound_input, bound_candidate) if row is not None]
    # One invocation may contain several serial observers. Snapshot current
    # output identities so this process does not claim an earlier child's frames
    # or saves merely because they share the invocation identifier.
    existing = [*context.capture_paths(), *context.images.glob("*"), *context.saves.glob("*"), *context.data.glob("*"),
                *context.evidence.glob("*"), *context.reports.glob("*")]
    before = {path: (path.stat().st_size, path.stat().st_mtime_ns) for path in existing if path.is_file()}
    process_id = uuid.uuid4().hex
    transcript_starts = _transcript_starts(context, options)
    result = _run_owned_process(arguments, options)
    transcripts = _retain_transcripts(context, result, transcript_starts, process_id)
    engine_captures = collect_native_screenshots(context)
    inputs = []
    # Explicit source hooks and -g identify the saved worlds actually requested
    # by this process. Output prefixes never participate in immutable identity.
    for key, value in options["env"].items():
        if key.startswith("OTTD_") and key.endswith(("_SOURCE", "_INPUT")) and Path(value).suffix.lower() in (".sav", ".scn"):
            inputs.append(resolve_input_path(value, root=context.root))
    for index, value in enumerate(arguments[:-1]):
        if value == "-g" and Path(arguments[index + 1]).suffix.lower() in (".sav", ".scn"):
            inputs.append(resolve_input_path(arguments[index + 1], root=context.root))
    if bound_input is not None:
        inputs.append(context.root / bound_input["path"])
    if bound_candidate is not None:
        inputs.extend(context.root / row["path"] for row in (bound_candidate, bound_candidate["checksum_source"]))
    # Native observers also write machine-readable scene/owner observations.
    # Snapshot them before publishing our own process record, avoiding recursion
    # while binding only files created/changed during this actual child.
    candidates = sorted(set([*context.capture_paths(), *context.images.glob("*"), *context.saves.glob("*"), *context.data.glob("*"),
                             *context.evidence.glob("*"), *context.reports.glob("*"), *engine_captures]))
    produced = [path for path in candidates if path.is_file() and
                before.get(path) != (path.stat().st_size, path.stat().st_mtime_ns)]
    report = context.write_report("native-" + process_id + ".json", producer_name,
                                  produced, sorted(path for path in set(inputs) if path.is_file()), configuration)
    record = json.loads(report.read_text(encoding="utf-8"))
    if native_producer is not None:
        record["entrypoint"] = entrypoint_name
    record.update(argv=arguments, exit_status=result.returncode,
                  timed_out=result.timed_out, root_reaped=result.root_reaped,
                  runtime=context.runtime_dir.relative_to(context.root).as_posix(),
                  transcripts=[dict(path=path.relative_to(context.root).as_posix(),
                                    sha256=hashlib.sha256(path.read_bytes()).hexdigest()) for path in transcripts])
    # Native executable identity belongs to this build/run. Shell launchers
    # emit their own child report, so no installed tool is mistaken for the game.
    integrity_error = None
    if initial_image is not None:
        record["executable"] = initial_image
        closing_image = hashlib.sha256(executable.read_bytes()).hexdigest() if executable.is_file() else None
        if closing_image != initial_image["sha256"]:
            integrity_error = ValueError("native executable changed during execution")
            record["closing_executable_sha256"] = closing_image
    if initial_writer is not None:
        record["producer_source"] = {"path": producer_name, "sha256": initial_writer}
        closing_writer = hashlib.sha256(writer.read_bytes()).hexdigest() if writer.is_file() else None
        if closing_writer != initial_writer:
            integrity_error = ValueError("native producer source changed during execution")
    if bound_inputs:
        record["bound_inputs"] = bound_inputs
        # Bound source identities describe bytes observed before launch even
        # if a faulty child rewrites/removes one. Preserve that failed process
        # and its transcripts instead of losing evidence while hashing a missing file.
        opening = [{key: row[key] for key in ("path", "sha256")} for row in bound_inputs]
        if bound_candidate is not None:
            opening.append(bound_candidate["checksum_source"])
        expected_paths = {row["path"] for row in opening}
        record["inputs"] = [row for row in record["inputs"] if row["path"] not in expected_paths] + opening
        try:
            if observe_expected_payload(context, options["env"], configuration) != bound_input:
                raise ValueError("sprite native input changed during execution")
            if observe_candidate_baseset(context, options["env"], configuration) != bound_candidate:
                raise ValueError("candidate GRF input changed during execution")
        except (ValueError, OSError) as error:
            integrity_error = error
        # Preserve the actual child/process transcript even when a source input
        # changed. Such a process remains failed evidence and cannot qualify a loader.
        record["input_integrity_passed"] = integrity_error is None
        if integrity_error is not None:
            record["input_integrity_error"] = str(integrity_error)
    elif initial_image is not None or initial_writer is not None:
        # Non-sprite observers also bind their actual launch image and writer.
        # Missing expected pixels never permits source/executable drift to pass.
        record["input_integrity_passed"] = integrity_error is None
        if integrity_error is not None:
            record["input_integrity_error"] = str(integrity_error)
    report.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    result.native_report = dict(path=report.relative_to(context.root).as_posix(),
                                sha256=hashlib.sha256(report.read_bytes()).hexdigest())
    if integrity_error is not None:
        raise ValueError("native input or executable changed during execution; see " + str(report)) from integrity_error
    # Timeout and check=True failures retain the same required transcript/input
    # record as successful children before preserving the caller's exception policy.
    if result.timed_out:
        error = subprocess.TimeoutExpired(arguments, options.get("timeout"), output=result.stdout, stderr=result.stderr)
        error.native_report = result.native_report
        raise error
    if options.get("check", False) and result.returncode:
        error = subprocess.CalledProcessError(result.returncode, arguments, output=result.stdout, stderr=result.stderr)
        error.native_report = result.native_report
        raise error
    return result


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
    if os.name != "nt":
        options.setdefault("start_new_session", True)
    child = subprocess.Popen(native_command(context, command), **options)
    child.artifact_owned_session = os.name != "nt" and options.get("start_new_session", False)
    return child
