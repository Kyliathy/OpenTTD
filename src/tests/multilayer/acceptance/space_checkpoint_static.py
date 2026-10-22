"""Prepare only the ordinary paused console/load/save seam for checkpoint qualification.

@file space_checkpoint_static.py Dedicated ordinary restore, never the old paid replay.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups12/15.
Source is staged here; actual application compilation/execution belongs to15.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess

from space_checkpoint import ROOT, CONFIGURATIONS, SpaceCheckpoint, load_checkpoint, verify_checkpoint
from space_checkpoint_foundation import verify_static_result, verify_console_projection, STATIC_SOURCE_FILES as SOURCE_FILES
from space_checkpoint_load import checked, identity, verify_load_result
from artifact_paths import create_run_context
from runner import console_executable, run_native

def execute_static_check(checkpoint, configuration, application, native_reference):
    """Run the ordinary paused save seam against the explicitly selected original.

    @param checkpoint Typed immutable original seal.
    @param configuration Independently linked Debug/RelWithDebInfo selection.
    @param application Actual current ordinary build image; no historical fallback.
    @param native_reference Same build/configuration's successful dedicated native-load receipt.
    @return Published ordinary static receipt identity after full current validation.
    """
    if not isinstance(checkpoint, SpaceCheckpoint):
        raise TypeError("ordinary restore requires a typed original checkpoint")
    # The typed selection and explicit image must agree before any runtime is
    # reserved; the other configuration's successful load is not interchangeable.
    if configuration not in CONFIGURATIONS or checkpoint.configuration not in (None, configuration):
        raise ValueError("ordinary restore disagrees with typed configuration")
    verify_checkpoint(checkpoint)
    root = checkpoint.root
    native = json.loads(checked(native_reference, root).read_text(encoding="utf-8"))
    verify_load_result(native, root)
    if native["configuration"] != configuration:
        raise ValueError("ordinary restore needs the same native configuration")
    app_reference = identity(application, root)
    application = checked(app_reference, root)
    if application.parent.name != configuration or application.name not in ("openttd.exe", "openttd"):
        raise ValueError("ordinary restore requires the selected current game image")
    seal = json.loads(checkpoint.manifest_path.read_text(encoding="utf-8"))
    source_reference = seal["configurations"][configuration]["save"]
    source = checked(source_reference, root)
    sources = [identity(root / path, root) for path in SOURCE_FILES]
    context = create_run_context("space", "checkpoint-static", root=root)
    # Resume may inspect an existing result, but may never overwrite its save
    # by accidentally rerunning the ordinary application in the same invocation.
    if any(path.is_file() for role in ("images", "saves", "data", "reports", "evidence", "scratch") for path in (context.directory / role).rglob("*")):
        raise ValueError("ordinary checkpoint invocation already contains output")
    context.prepare_runtime()
    executable = console_executable(context, application)
    executable_reference = identity(executable, root)
    projection = verify_console_projection(application, executable, context.runtime_dir, root)
    output = context.saves / "ordinary.sav"
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text(f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment["OTTD_TEST_CONFIGURATION"] = configuration
    arguments = [str(executable), "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-d", "sl=3",
                 "-c", str(context.config_path), "-g", str(source)]
    log = context.output("logs", "ordinary.log")
    with log.open("wb") as sink:
        result = run_native(context, arguments, cwd=context.runtime_dir, stdout=sink, stderr=subprocess.STDOUT,
                            timeout=60, env=environment)
    if result.returncode != 0:
        raise ValueError(f"ordinary checkpoint restore failed; inspect {log}")
    reports = list(context.reports.glob("native-*.json"))
    if len(reports) != 1:
        raise ValueError("ordinary restore needs one actual process")
    process = json.loads(reports[0].read_text(encoding="utf-8"))
    if len(process["transcripts"]) != 1:
        raise ValueError("ordinary restore lacks its retained current terminal")
    document = dict(_documentation=__doc__, schema_version=1, kind="ordinary_space_checkpoint_restore", complete=True,
        configuration=configuration, phase="ordinary-static", seal=dict(path=checkpoint.manifest_path.relative_to(root).as_posix(), sha256=checkpoint.sha256),
        source=source_reference, application=app_reference, console_application=executable_reference,
        output=identity(output, root), terminal=process["transcripts"][0], native_process=identity(reports[0], root),
        native_load=native_reference, run_id=context.run_id, source_files=sources, gameplay_ticks_executed=0, projection=projection)
    verify_static_result(document, checkpoint, native_reference)
    target = context.output("reports", "space-checkpoint-static.json")
    target.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return identity(target, root)


def main():
    """Require explicit source/build/native-load selections for ordinary recovery.

    @return None; this command never invokes a paid replay or accepts a work switch.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoint", type=Path)
    parser.add_argument("--configuration", choices=CONFIGURATIONS, required=True)
    parser.add_argument("--application", type=Path, required=True)
    parser.add_argument("--native-result", type=Path, required=True)
    args = parser.parse_args()
    checkpoint = load_checkpoint(args.checkpoint, args.configuration)
    print(json.dumps(execute_static_check(checkpoint, args.configuration, args.application,
                                        identity(args.native_result, ROOT)), indent=2))


if __name__ == "__main__":
    main()
