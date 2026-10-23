"""Bind an actual ordinary zero-tick reload to a new typed Space operation.

@file space_operation_restore.py Ordinary witnesses extend the existing successor owner.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group36.
@see docs/zz-reach2/architecture/archi-historical-debug.md Historical facts and current proof.
Current publication verifies images/processes; durable copies retain the measured
save, terminal and process facts without requiring an archived application.
"""

import json
import os
import subprocess

from artifact_paths import RunContext, create_run_context
from runner import console_executable, run_native
from space_checkpoint_load import checked, identity
from space_checkpoint_foundation import compare_static_saves, verify_console_projection

SCENARIO = "goods-capacity-restore"
ROLES = {"ordinary_result", "ordinary_process", "ordinary_terminal", "ordinary_output"}
SOURCE_FILES = ("src/tests/multilayer/acceptance/space_operation_restore.py",
                "src/tests/multilayer/acceptance/space_checkpoint_foundation.py",
                "src/tests/multilayer/support/artifact_paths.py",
                "src/tests/multilayer/support/runner.py")
FIELDS = {"_documentation", "schema_version", "kind", "complete", "configuration", "task", "phase",
          "source", "application", "console_application", "output", "terminal", "native_process",
          "run_id", "source_files", "projection", "gameplay_ticks_executed"}
FLAGS = ["-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-d", "sl=3", "-c"]


def witness_references(operation, document):
    """Select the four explicit ordinary witnesses of a version-two operation.

    @param operation Actual typed operation declaring its ordinary result identity.
    @param document That identity's decoded ordinary reload receipt.
    @return Exact role-to-identity map; never a search for the latest output.
    """
    return dict(ordinary_result=operation["ordinary_result"], ordinary_process=document["native_process"],
                ordinary_terminal=document["terminal"], ordinary_output=document["output"])


def verify_restore(document, operation, root, build, current=True, copies=None):
    """Check current execution or relocated completed ordinary-reload witnesses.

    @param document Exact ordinary_space_operation_restore receipt.
    @param operation Native operation whose output was actually loaded.
    @param root Explicit checkout owning live or copied witnesses.
    @param build Already selected native build containing both actual images.
    @param current True checks live source/images; False uses completed copied facts.
    @param copies Role-to-Path copied closure, required only after publication/export.
    @return Actual121-family zero-tick restore facts; no future permission.
    """
    # An ordinary process must reload this operation's actual output, with the
    # same task/configuration, rather than borrowing a successful old save load.
    if (set(document) != FIELDS or type(document["schema_version"]) is not int or document["schema_version"] != 1
            or document["kind"] != "ordinary_space_operation_restore" or document["complete"] is not True
            or document["configuration"] != operation["configuration"] or document["task"] != operation["task"]
            or document["phase"] != "ordinary-static" or document["source"] != operation["output"]
            or type(document["gameplay_ticks_executed"]) is not int or document["gameplay_ticks_executed"] != 0):
        raise ValueError("ordinary operation restore selected a different task/save/configuration")
    roles = {"ordinary_process": document["native_process"], "ordinary_terminal": document["terminal"], "ordinary_output": document["output"]}
    paths = {}
    # Relocation changes physical owners only. Every copied report, transcript
    # and save must preserve the exact bytes originally measured by the process.
    for role, reference in roles.items():
        path = checked(reference, root) if current else copies[role]
        if identity(path, root)["sha256"] != reference["sha256"]:
            raise ValueError("ordinary copied witness identity changed")
        paths[role] = path
    source = checked(document["source"], root) if current else copies["output"]
    native_terminal = checked(operation["terminal"], root) if current else copies["native_terminal"]
    if identity(source, root)["sha256"] != document["source"]["sha256"]:
        raise ValueError("ordinary restore source changed")
    process = json.loads(paths["ordinary_process"].read_text(encoding="utf-8"))
    context = RunContext(root, "space", SCENARIO, document["run_id"])
    argv = process["argv"]
    # Fixed paused-console arguments, exact input/output membership and the
    # process-local home must survive transfer. Absolute old root prefixes are
    # historical; their canonical role suffixes still identify the invocation.
    if (process["exit_status"] != 0 or process["configuration"] != operation["configuration"]
            or process["inputs"] != [document["source"]] or process["executable"] != document["console_application"]
            or process["feature"] != "space" or process["scenario"] != SCENARIO or process["run_id"] != document["run_id"]
            or process["runtime"] != context.runtime_dir.relative_to(root).as_posix()
            or len(argv) != 17 or argv[1:14] != FLAGS or argv[15] != "-g"
            or not argv[0].replace("\\", "/").endswith("/" + document["console_application"]["path"])
            or not argv[14].replace("\\", "/").endswith("/" + context.config_path.relative_to(root).as_posix())
            or not argv[16].replace("\\", "/").endswith("/" + document["source"]["path"])
            or document["output"] not in process["artifacts"] or document["terminal"] not in process["artifacts"]
            or process["transcripts"] != [document["terminal"]]):
        raise ValueError("ordinary operation process/input/image membership disagrees")
    if document["application"] != build["targets"].get("openttd"):
        raise ValueError("ordinary restore did not use the selected ordinary game")
    projection = document["projection"]
    # Completed PE/DLL comparisons retain their exact identities after images
    # are cleaned. Live publication must still compare the actual image bytes.
    if (set(projection) != {"method", "application", "executable", "dlls"}
            or projection["method"] not in ("same_image", "pe_subsystem_2_to_3")
            or projection["application"] != document["application"] or projection["executable"] != document["console_application"]):
        raise ValueError("ordinary operation console projection facts disagree")
    if {reference["path"] for reference in document["source_files"]} != set(SOURCE_FILES):
        raise ValueError("ordinary operation restore lacks exact source owners")
    if current:
        application = checked(document["application"], root)
        executable = checked(document["console_application"], root)
        if verify_console_projection(application, executable, context.runtime_dir, root) != projection:
            raise ValueError("ordinary operation projection changed")
        if paths["ordinary_output"] != context.saves / "ordinary.sav":
            raise ValueError("ordinary operation output belongs to another invocation")
        # Both witnesses must belong to this same actual ordinary invocation;
        # unrelated successful process evidence cannot qualify its output.
        if paths["ordinary_terminal"].parent != context.evidence or paths["ordinary_process"].parent != context.reports:
            raise ValueError("ordinary operation evidence belongs to another invocation")
        # Close on the exact live transport/observer sources that generated the
        # restore facts, preventing mixed-source publication during an edit.
        for reference in document["source_files"]:
            checked(reference, root)
    text = paths["ordinary_terminal"].read_text(encoding="utf-8", errors="replace")
    # A successful process exit alone cannot hide an unsuccessful save load or
    # replace the actual ordinary loader's diagnostic transcript.
    if "dbg: [sl:" not in text or "Loading game failed" in text:
        raise ValueError("ordinary operation restore lacks a successful load transcript")
    from space_checkpoint_operation import _scope
    native = _scope(native_terminal.read_text(encoding="utf-8", errors="replace"), operation)
    return compare_static_saves(source, paths["ordinary_output"], operation["final"], native)


def execute_restore(operation, root, build):
    """Reload the native query output once through the actual ordinary application.

    @param operation Successful current native query fields, before ordinary binding.
    @param root Explicit selected development checkout.
    @param build Current build with its exact ordinary and isolated image identities.
    @return Published ordinary receipt identity after zero-tick owner verification.
    """
    source = checked(operation["output"], root)
    application = checked(build["targets"]["openttd"], root)
    # Runtime identifiers are reserved project-wide across scenarios. The
    # ordinary child owns a distinct ID while binding the exact native output.
    context = create_run_context("space", SCENARIO, root=root, run_id=operation["run_id"] + "-ordinary")
    # A result is one actual invocation. Refuse to overwrite an existing run,
    # including its partial evidence, when a caller repeats the same identifier.
    if any(path.is_file() for role in ("images", "saves", "data", "reports", "evidence", "scratch") for path in (context.directory / role).rglob("*")):
        raise ValueError("ordinary operation restore invocation already contains output")
    context.prepare_runtime()
    executable = console_executable(context, application)
    projection = verify_console_projection(application, executable, context.runtime_dir, root)
    output = context.saves / "ordinary.sav"
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text(f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment["OTTD_TEST_CONFIGURATION"] = operation["configuration"]
    arguments = [str(executable), *FLAGS, str(context.config_path), "-g", str(source)]
    log = context.output("logs", "ordinary.log")
    with log.open("wb") as sink:
        result = run_native(context, arguments, cwd=context.runtime_dir, stdout=sink, stderr=subprocess.STDOUT,
                            timeout=60, env=environment, native_producer="src/tests/multilayer/acceptance/space_operation_restore.py")
    if result.returncode:
        raise ValueError(f"ordinary operation restore failed: {log}")
    reports = list(context.reports.glob("native-*.json"))
    if len(reports) != 1:
        raise ValueError("ordinary operation restore needs one actual process")
    process = json.loads(reports[0].read_text(encoding="utf-8"))
    if len(process["transcripts"]) != 1:
        raise ValueError("ordinary operation restore needs one actual terminal")
    document = dict(_documentation=__doc__, schema_version=1, kind="ordinary_space_operation_restore", complete=True,
        configuration=operation["configuration"], task=operation["task"], phase="ordinary-static", source=operation["output"],
        application=identity(application, root), console_application=identity(executable, root), output=identity(output, root),
        terminal=process["transcripts"][0], native_process=identity(reports[0], root), run_id=context.run_id,
        source_files=[identity(root / name, root) for name in SOURCE_FILES], projection=projection, gameplay_ticks_executed=0)
    verify_restore(document, operation, root, build)
    target = context.output("reports", "ordinary-restore.json")
    target.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return identity(target, root)
