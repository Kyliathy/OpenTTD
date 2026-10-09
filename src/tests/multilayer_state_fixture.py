#!/usr/bin/env python3
"""Focused actual metadata round trips and unchanged legacy save loading.

@file multilayer_state_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md
@see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md
"""

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
AREA = ROOT / "docs/zz-reach2/upgrades/2026-10"
FIXTURES = ROOT / "src/tests/fixtures"
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path, RunContext, identifier, contained
from runner import current_run_context, run_native, console_executable
CONTEXT = current_run_context("state", "metadata")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime("grftest.cfg")
_OUTPUTS = {}


def prior_artifact(context, run_id, kind, name):
    """Select exact earlier-stage evidence without guessing the latest run.

    @param context Current feature/scenario whose earlier run is being checked.
    @param run_id Explicit prior invocation supplied by the operator.
    @param kind Either saves or logs; this function never creates evidence.
    @param name Exact expected basename, including refusal outputs allowed to be absent.
    @return Canonical prior path; the caller checks existence or semantic refusal.
    """
    # Only same-case evidence may support a partial rerun's aggregate claim.
    # A malformed kind or basename must not select unrelated retained files.
    if kind not in ("saves", "logs") or Path(name).name != name:
        raise ValueError("prior evidence requires an exact save/log basename")
    previous = RunContext(context.root, context.feature, context.scenario, identifier(run_id))
    directory = previous.saves if kind == "saves" else previous.logs
    return contained(directory / name, directory)


def log_path(name, root=ROOT, context=CONTEXT):
    """Route captured diagnostics into the caller's explicit feature invocation.

    @param name Log basename preserving its evidence identity.
    @param root Expected repository root; a foreign checkout is refused.
    @param context Calling feature's shared runtime/review identity.
    @return New run-owned log path, separate from immutable historical evidence.
    """
    # Logs cannot redirect the current invocation or change file roles through
    # directory syntax, a foreign checkout or an unexpected artifact suffix.
    if root.resolve() != context.root or not name.endswith(".log"):
        raise ValueError("fixture log must be a repository-owned .log basename")
    return context.output("logs", name)


def output_save_path(name, context=CONTEXT):
    """Reserve a generated save without granting write access to old fixtures.

    @param name New save basename used by this fixture stage.
    @param context Explicit owning feature/scenario supplied by the caller.
    @return New output path, remembered for same-process continuation reads.
    """
    if not name.endswith(".sav"):
        raise ValueError("fixture output must be a .sav basename")
    path = context.output("saves", name)
    _OUTPUTS[(context.feature, context.scenario, name)] = path
    return path


def save_path(name, context=CONTEXT):
    """Resolve an immutable save or an explicitly reserved same-process output.

    @param name Basename using the owning plan's established evidence prefix.
    @param context Explicit feature whose reserved outputs may be continued.
    @return Exact input path; a reserved but absent output never falls back to history.
    """
    # Only owned save basenames may select a feature directory; arbitrary paths
    # must not overwrite retained inputs or place saves in the documentation tree.
    if Path(name).name != name or not name.endswith(".sav"):
        raise ValueError("fixture save must be a .sav basename")
    key = (context.feature, context.scenario, name)
    if key in _OUTPUTS:
        return _OUTPUTS[key]
    families = (
        ("r2mlvs1-", "content"),
        ("r2mlvs2-", "geology"),
        ("r2mlvs3-", "portals"),
        ("r2mlvs14-", "state"),
        ("r2mlvs6-ds-", "processors"),
        ("r2mlvs6-dz-", "processors"),
        ("r2mlvs6-ea-", "processors"),
        ("r2mlvs6-", "mines"),
    )
    # Specific processor prefixes precede the wider mine prefix so each save
    # stays with the feature that owns its persistence contract.
    for prefix, family in families:
        if name.startswith(prefix):
            return resolve_input_path(FIXTURES / family / name)
    raise ValueError(f"unknown fixture save owner: {name}")


def run_case(name, arguments, expected, accepted=True, prefix="r2mlvs14-b", single_player=False, context=CONTEXT):
    """Run one repository-local dedicated boundary and retain its evidence.

    @param name Stable evidence suffix for this owned fixture.
    @param arguments Structured dedicated load/new-world arguments.
    @param expected Metadata acceptance/refusal text required in the actual log.
    @param accepted Whether the candidate must reach startup/save; refusal must produce no save.
    @param prefix Owning plan's evidence prefix; saves live in feature fixture directories.
    @param single_player Use null-video nonnetworked mode and the actual start_ai command to create an owned company for portal fixtures; default preserves dedicated behavior.
    @param context Explicit feature invocation used for runtime and evidence ownership.
    @return Saved output path; failure preserves supplied inputs through the caller's finally block.
    """
    output = output_save_path(f"{prefix}-{name}.sav", context)
    script = output.with_suffix(".scr")
    log = log_path(f"{prefix}-{name}.log", context=context)
    output.unlink(missing_ok=True)
    company_setup = "start_ai\n" if single_player and "-g" not in arguments else ""  # New null-video worlds need an actual owner; loaded worlds retain their saved company without creating another.
    script.write_text("# @see r2mlvs14-savegame-determinism-and-lifecycle.md, metadata fixture.\n"
                      f'dump_info cargotypes\n{company_setup}save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    (context.runtime_dir / "scripts/game_start.scr").write_text(f'exec "{script.as_posix()}"\n', encoding="utf-8")
    env = dict(os.environ, OTTD_TIMEOUT="60", OTTD_DEBUG="misc=5,net=2,sl=3,grf=2,desync=5,script=2")
    env.pop("OTTD_MULTILAYER_LOADER_PROBE", None)
    command = ["C:/Program Files/Git/bin/bash.exe", (ROOT / "mlm/tools/run/run_dedicated.sh").as_posix(),
               (ROOT / "build").as_posix(), "Debug", log.as_posix(), "-X", "-c", "grftest.cfg", *arguments]
    try:
        if single_player:
            # Null media suppresses automatic company creation. The nonnetworked start_ai command executes synchronously and supplies the actual owner.
            console = console_executable(context, ROOT / "build/Debug/openttd.exe")
            game_arguments = arguments if "-g" in arguments else ["-g", *arguments]
            with log.open("wb") as sink:
                run_native(context, [str(console), "-v", "null", "-s", "null", "-m", "null", "-x", "-X", "-d", env["OTTD_DEBUG"],
                                "-c", "grftest.cfg", *game_arguments], cwd=ROOT / "build", env=env, stdout=sink,
                               stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW, check=True, timeout=70)
        else:
            run_native(context, command, check=True, env=env, timeout=70)
        evidence = log.read_text(encoding="utf-8", errors="replace")
        assert expected in evidence, (name, "required metadata evidence missing")
        assert output.is_file() == accepted, (name, "acceptance/save boundary differs")
        print(f"{name}: metadata boundary passed", flush=True)
        return output
    finally:
        # Disposable console source is owned by this repository fixture; supplied saves are never deleted.
        script.unlink(missing_ok=True)


def main():
    """Check metadata/legacy families, or two selected normal-build loads without full regressions.

    @return None; failures raise after restoring pre-existing configuration/startup bytes.
    """
    startup = RUNTIME / "scripts/game_start.scr"
    config = CONTEXT.prepare_runtime("grftest.cfg")
    original_script = startup.read_bytes()
    original_config = config.read_bytes()
    legacy = [("surface", ROOT / "regression/gs_compat/test.sav"),
              ("layers369", ROOT / "build/save/epic_1234_8x8.sav"),
              ("layers370", ROOT / "build/save/camera-deep-repaired.sav")]
    hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for _, path in legacy}
    parser = argparse.ArgumentParser()
    parser.add_argument("--smoke", action="store_true", help="load only an existing native and a supplied Classic save")
    args = parser.parse_args()
    try:
        config.write_text("# @see r2mlvs14-savegame-determinism-and-lifecycle.md.\n"
                          "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                          "gameplay_profile = multilayer_industry\n"
                          "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n", encoding="utf-8")
        if args.smoke:
            run_case("normal-native", ["-g", save_path("r2mlvs14-b-native-new.sav").as_posix()],
                     "MLVS_METADATA schema=1 profile=1 version=1 legacy=false editor=false", prefix="r2mlvs1-i")
            run_case("normal-classic", ["-g", legacy[0][1].as_posix()],
                     "MLVS_METADATA absent profile=0 version=0 legacy=true", prefix="r2mlvs1-i")
        else:
            created = run_case("native-new", ["-G", "1234"], "MLVS_PROFILE id=1 version=1")
            reloaded = run_case("native-reload", ["-g", created.as_posix()],
                                "MLVS_METADATA schema=1 profile=1 version=1 legacy=false editor=false")
            run_case("native-reload-again", ["-g", reloaded.as_posix()],
                     "MLVS_METADATA schema=1 profile=1 version=1 legacy=false editor=false")
            # Each real older family lacks MLMD; the pending Multilayer selector must not convert its accepted Classic policy.
            for name, path in legacy:
                run_case(name, ["-g", path.as_posix()], "MLVS_METADATA absent profile=0 version=0 legacy=true")
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        for path, digest in hashes.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, "supplied legacy save changed"
    result_name = "r2mlvs1-i-final-metadata-results.txt" if args.smoke else "r2mlvs14-b-metadata-results.txt"
    result_scope = "R2MLVS1 normal build: selected native and supplied Classic loads passed.\n" if args.smoke else "R2MLVS14 M0 metadata: explicit native new/reload/reload, Surface/369/370 legacy loads passed.\n"
    (CONTEXT.reports / result_name).write_text(
        result_scope +
        "Supplied legacy file SHA256 preserved. Feature payloads and multiplayer runtime remain pending.\n", encoding="utf-8")


if __name__ == "__main__":
    main()
