"""Batch Group9 focused scenarios and serial actual-file continuations against one build.

@see r2tbmf-tbm-fixes.md Group9
@see ../../architecture/savegame/archi-savegame.md
"""

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import time

# Repository markers identify the source checkout after tools move; neither
# documentation depth nor the caller's working directory owns runtime output.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import save_native_capture, current_run_context, run_native, start_native

CONTEXT = current_run_context("tbm", "far-exit")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

BUILD = ROOT / "build"
FIXTURES = ROOT / "src/tests/fixtures/haulage"


def digest(path):
    """@param path Protected actual input. @return SHA256 of its bytes."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fresh(case):
    """@param case Focused acceptance slice. @return Unused log and corresponding fresh native evidence prefix."""
    serial = 1
    while (CONTEXT.logs / f"r2tbmf-y-{case}-{serial}.log").exists():
        serial += 1
    return CONTEXT.logs / f"r2tbmf-y-{case}-{serial}.log", CONTEXT.saves / f"tbm-exit-{case}-{serial}"


def run(case, source=None):
    """Run one bounded batch without parallel native processes or replacing supplied saves.

    @param case Focused generation or actual loaded exit state/input variant.
    @param source Explicit retained file for runtime continuation.
    @return None; report exact failure log while restoring startup and retained inputs.
    """
    log, output = fresh(case)
    env = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_TBM_")}
    startup = RUNTIME / "scripts/game_start.scr"
    original = startup.read_bytes()
    protected = [BUILD / "native-art.cfg", BUILD / "play.cfg", resolve_input_path(FIXTURES / "underground-demo-390.sav")]
    if case in ("focused", "journey"):
        env["OTTD_TBM_LOGISTICS_RECORD"] = output.name
        filters = "[tbm-exit-journey],[tbm-support],[portal-bootstrap],[portal-date],[portal-script]" if case == "focused" else "[tbm-exit-journey]"
    else:
        assert source, "A native replay requires its explicitly retained source"
        source = resolve_input_path(source)
        assert resolve_input_path(source).is_file(), source
        protected.append(source)
        env.update(OTTD_TBM_LOGISTICS_SOURCE=str(source), OTTD_TBM_LOGISTICS_OUTPUT=str(output),
                   OTTD_TBM_EXIT_RELOAD="1", OTTD_TBM_RELEASE_RUN="1")
        if case == "traffic-load":
            env.pop("OTTD_TBM_EXIT_RELOAD")
            env["OTTD_TBM_PORTAL_LOAD_ONLY"] = "1"
        if case in ("underground", "surface"):
            env["OTTD_TBM_EXIT_INPUT"] = case
        filters = "[.tbm-logistics-runtime]"
    before = {path: digest(path) for path in protected}
    started = time.monotonic()
    try:
        if case not in ("focused", "journey"):
            startup.write_text("logistics_probe\n", encoding="utf-8")
        with log.open("w", encoding="utf-8") as stream:
            result = run_native(CONTEXT, [str(BUILD / "RelWithDebInfo/openttd_test.exe"), filters],
                                    cwd=BUILD, env=env, stdout=stream, stderr=subprocess.STDOUT, timeout=120)
    finally:
        startup.write_bytes(original)
        for path, expected in before.items():
            assert digest(path) == expected, path
    print(f"Exit {case}: result={result.returncode}, seconds={time.monotonic() - started:.2f}, log={log.relative_to(ROOT)}", flush=True)
    actual = log.read_text(encoding="utf-8")
    for line in actual.splitlines():
        if line.startswith(("EXIT_", "PORTAL_", "All tests passed", "test cases:", "assertions:")):
            print(line, flush=True)
    if case in ("underground", "surface"):
        from PIL import Image
        reviews = CONTEXT.images
        for ppm in CONTEXT.capture_paths(f"*exit-{case}-*.ppm"):
            with Image.open(ppm) as picture:
                save_native_capture(CONTEXT, picture, ppm, ppm.with_suffix(".png"))
    result.check_returncode()


def main():
    """@return None; run only requested new or failed acceptance slices against the existing completed build."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--case", choices=("focused", "journey", "underground", "surface", "built", "commissioned", "withdrawing", "moving", "parked", "traffic-load"), required=True)
    parser.add_argument("--source")
    args = parser.parse_args()
    run(args.case, args.source)


if __name__ == "__main__":
    main()
