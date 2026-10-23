"""Reload retained397 hangar return/parked files through the real game loop.

@see r2tbmf-tbm-fixes.md Group8
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
from runner import save_native_capture, capture_path, current_run_context, run_native, start_native

CONTEXT = current_run_context("tbm", "hangar-orders")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()



def digest(path):
    """@param path Retained input. @return SHA256 of its actual bytes."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_case(name, source_override=None):
    """Run a real selected-hangar continuation without overwriting earlier evidence.

    @param name Return/parked continuation, native input, ordinary orders, or accepted remote reassignment.
    @param source_override Optional explicit retained native-order file, inside the same fixture family.
    @return None; raise on a failed journey or a changed protected input.
    """
    build = ROOT / "build"
    fixtures = ROOT / "src/tests/fixtures/haulage"
    state = "return" if name == "project" else "parked" if name in ("vehicle", "orders") else name
    source = resolve_input_path(source_override) if source_override else resolve_input_path(fixtures / (
        "tbm-hangar-reload-orders-run-1-orders-travel.sav" if name == "orders-reload" else f"tbm-hangar-397-2-hangar-{state}.sav"))
    assert resolve_input_path(source).is_file(), source
    if name == "reassigned":
        assert source_override, "Reassignment requires its explicitly retained native source file"
    assert source.is_file(), source
    serial = 1
    while (CONTEXT.logs / f"r2tbmf-u-{name}-{serial}.log").exists():
        serial += 1
    log = CONTEXT.logs / f"r2tbmf-u-{name}-{serial}.log"
    output = CONTEXT.saves / f"tbm-hangar-reload-{name}-run-{serial}"  # The actual save header owns its version; later writers must never mislabel new output as397.
    startup = RUNTIME / "scripts/game_start.scr"
    protected = [source, build / "native-art.cfg", build / "play.cfg", resolve_input_path(fixtures / "underground-demo-390.sav")]
    before = {path: digest(path) for path in protected}
    original = startup.read_bytes()
    env = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_TBM_")}
    env.update(OTTD_TBM_LOGISTICS_SOURCE=str(source), OTTD_TBM_LOGISTICS_OUTPUT=str(output),
               OTTD_TBM_HANGAR_RELOAD="1", OTTD_TBM_RELEASE_RUN="1")
    if name in ("project", "vehicle"):
        env["OTTD_TBM_HANGAR_INPUT"] = name
    if name in ("orders", "orders-reload"):
        env["OTTD_TBM_NATIVE_ORDERS"] = "input" if name == "orders" else "reload"
    if name == "reassigned":
        env["OTTD_TBM_HANGAR_REASSIGNED"] = "1"
    started = time.monotonic()
    try:
        startup.write_text("logistics_probe\n", encoding="utf-8")
        with log.open("w", encoding="utf-8") as stream:
            result = run_native(CONTEXT, [str(build / "RelWithDebInfo/openttd_test.exe"), "[.tbm-logistics-runtime]"],
                                    cwd=build, env=env, stdout=stream, stderr=subprocess.STDOUT, timeout=120)
    finally:
        startup.write_bytes(original)
        for path, expected in before.items():
            assert digest(path) == expected, path
    print(f"Hangar {name}: exit={result.returncode}, seconds={time.monotonic() - started:.2f}, log={log.relative_to(ROOT)}", flush=True)
    result.check_returncode()
    actual = log.read_text(encoding="utf-8")
    assert ("REASSIGNED_RELOAD " if name == "reassigned" else "ORDERS_RELOAD " if name in ("orders", "orders-reload") else "HANGAR_RELOAD ") in actual
    if name in ("project", "vehicle"):
        assert f"HANGAR_INPUT menu={name}" in actual
        from PIL import Image
        reviews = CONTEXT.images
        for view in ("controls", "choices"):
            ppm = capture_path(CONTEXT, f"tbm-hangar-{name}-{view}.ppm")
            with Image.open(ppm) as picture:
                save_native_capture(CONTEXT, picture, ppm, ppm.with_suffix(".png"))
    if name in ("orders", "orders-reload"):
        from PIL import Image
        reviews = CONTEXT.images
        for view in (("native-orders", "orders-resume") if name == "orders" else ("orders-resume",)):
            ppm = capture_path(CONTEXT, f"tbm-hangar-{view}.ppm")
            with Image.open(ppm) as picture:
                save_native_capture(CONTEXT, picture, ppm, ppm.with_suffix(".png"))
    for line in actual.splitlines():
        if line.startswith(("HANGAR_RELOAD ", "HANGAR_INPUT ", "ORDERS_RELOAD ", "REASSIGNED_RELOAD ")):
            print(line, flush=True)


def main():
    """@return None; execute only the selected new file acceptance slice."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--case", choices=("all", "return", "parked", "project", "vehicle", "orders", "orders-reload", "reassigned"), default="all")
    parser.add_argument("--source", help="Explicit retained file for the chosen native-order continuation")
    args = parser.parse_args()
    if args.case == "reassigned":
        run_case(args.case, args.source)
        return
    for name in ("return", "parked", "project", "vehicle", "orders", "orders-reload"):
        if args.case in ("all", name):
            run_case(name, args.source)


if __name__ == "__main__":
    main()
