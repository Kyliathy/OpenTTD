"""Verify real final-release input, cancellation reload and reuse of the same native machine.

@see r2tbmf-tbm-fixes.md Group7B
@see ../../architecture/savegame/archi-savegame.md
"""

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import time

from PIL import Image

# Repository markers identify the source checkout after tools move; neither
# documentation depth nor the caller's working directory owns runtime output.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import save_native_capture, capture_path, current_run_context, run_native, start_native

CONTEXT = current_run_context("tbm", "release")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()



def digest(path):
    """@param path Retained input. @return SHA256 of its actual bytes."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_case(name, tag, env, script_text):
    """Run one native process with fresh outputs and restore the startup script.

    @param name Focused evidence name.
    @param tag Existing native test entry point.
    @param env Explicit process-local scenario controls.
    @param script_text Startup command for the actual application loader, or None for a command fixture.
    @return Captured native log text after successful execution.
    """
    build = ROOT / "build"
    fixtures = ROOT / "src/tests/fixtures/haulage"
    serial = 1
    while (CONTEXT.logs / f"r2tbmf-s-{name}-{serial}.log").exists():
        serial += 1
    log = CONTEXT.logs / f"r2tbmf-s-{name}-{serial}.log"
    output = CONTEXT.saves / f"tbm-release-{name}-396-{serial}"
    env["OTTD_TBM_LOGISTICS_OUTPUT"] = str(output)
    protected = [build / "play.cfg", build / "native-art.cfg", resolve_input_path(fixtures / "underground-demo-390.sav"),
                 resolve_input_path(fixtures / "tbm-feedback-haul50-393.sav"), resolve_input_path(fixtures / "tbm-parking-parked-395.sav")]
    if "OTTD_TBM_LOGISTICS_SOURCE" in env:
        protected.append(Path(env["OTTD_TBM_LOGISTICS_SOURCE"]))
    before = {path: digest(path) for path in protected}
    startup = RUNTIME / "scripts/game_start.scr"
    original = startup.read_bytes() if startup.exists() else None
    started = time.monotonic()
    try:
        if script_text is not None:
            startup.write_text("# @see r2tbmf-tbm-fixes.md Group7B native release.\n" + script_text + "\n", encoding="utf-8")
        with log.open("wb") as target:
            result = run_native(CONTEXT, [str(build / "RelWithDebInfo/openttd_test.exe"), tag], cwd=build, env=env,
                                    stdout=target, stderr=subprocess.STDOUT, timeout=110)
    finally:
        if original is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(original)
        for path, expected in before.items():
            assert digest(path) == expected, path
    print(f"Release {name}: exit={result.returncode}, seconds={time.monotonic() - started:.2f}, log={log.relative_to(ROOT)}", flush=True)
    result.check_returncode()
    return log.read_text(encoding="utf-8")


def main():
    """Run the requested focused slice; failed consumers can be retried without regenerating earlier native inputs.

    @return None; raise on a failed native behavior or changed retained file.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--case", choices=("all", "gui", "record", "cancel", "legacy", "released", "connected"), default="all")
    args = parser.parse_args()
    fixtures = ROOT / "src/tests/fixtures/haulage"
    base = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_TBM_")}
    if args.case in ("all", "gui"):
        actual = run_case("gui", "[.tbm-motion-reload]", dict(base, OTTD_TBM_FINAL_RELEASE="1"), "tbm_motion")
        assert "RELEASE_NATIVE " in actual
        reviews = CONTEXT.images
        for name in ("request", "ready"):
            ppm = capture_path(CONTEXT, f"tbm-release-{name}.ppm")
            with Image.open(ppm) as picture:
                assert picture.getbbox() is not None
                save_native_capture(CONTEXT, picture, ppm, ppm.with_suffix(".png"))
    cancel_source = resolve_input_path(fixtures / "tbm-final-release-396-cancel-return.sav")
    if args.case in ("all", "record") and not cancel_source.exists():
        run_case("record", "[tbm-release-settlement]", dict(base, OTTD_TBM_LOGISTICS_RECORD="tbm-final-release-396"), None)
    for name in ("cancel", "legacy", "released", "connected"):
        if args.case not in ("all", name):
            continue
        source = cancel_source if name == "cancel" else resolve_input_path(fixtures / (
            "tbm-parking-parked-395.sav" if name == "legacy" else f"tbm-release-{name}-396.sav"))
        assert source.is_file(), source
        env = dict(base, OTTD_TBM_LOGISTICS_SOURCE=str(source), OTTD_TBM_RELEASE_RUN="1")
        if name == "cancel":
            env["OTTD_TBM_LOGISTICS_COMPLETE_LOOP"] = "1"
        elif name == "legacy":
            env["OTTD_TBM_RESUME_PARKED"] = "1"
        else:
            env["OTTD_TBM_RELEASE_REUSE"] = "1"
        actual = run_case(name, "[.tbm-logistics-runtime]", env, "logistics_probe")
        assert "LOGISTICS_COMPLETE_LOOP " in actual if name == "cancel" else "LOGISTICS_NEXT_EXCHANGE " in actual
        if name in ("released", "connected"):
            assert "RELEASE_LOADED " in actual and "RELEASE_REUSED " in actual
        for line in actual.splitlines():
            if line.startswith(("RELEASE_", "LOGISTICS_COMPLETE_LOOP ", "LOGISTICS_NEXT_EXCHANGE ")):
                print(line, flush=True)


if __name__ == "__main__":
    main()
