"""Final affected integration and one actual GUI-to-public-tunnel save chain.

@see r2tbmf-tbm-fixes.md Group11; architecture/vehicles/archi-vehicles.md.
Reuse completed binaries, run cases serially and retain existing evidence bytes.
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

CONTEXT = current_run_context("tbm", "final-integration")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

BUILD = ROOT / "build"
FIXTURES = ROOT / "src/tests/fixtures/haulage"
REVIEWS = CONTEXT.images
FILTERS = {
    "vehicle-work": "[tbm-consist],[tbm-model],[tbm-pacing],[tbm-support],[tbm-backing],[tbm-route]",
    "vehicle-parking": "[tbm-advice],[tbm-parking],[tbm-parking-depot],[tbm-release-settlement],[tbm-release-traffic]",
    "vehicle-orders": "[tbm-hangar-acceptance],[tbm-hangar-orders],[tbm-hangar-reuse],[tbm-hangar-service],[tbm-native-orders],[tbm-selected-hangar]",
    "corrections": "[tbm-area],[tbm-replanning-append],[tbm-planning-refusal],[tbm-presentation],[excavation-cut],[excavation-pass]",
    "planning": "[underground-placement],[underground-toolbar],[underground-viewport-command],[underground-window],[tbm-presentation],[tbm-replanning-append],[tbm-replanning-preview],[tbm-planning-command],[tbm-planning-intent],[tbm-planning-refusal],[tbm-area]",
    "vehicles": "[tbm-consist],[tbm-model],[tbm-pacing],[tbm-advice],[tbm-parking],[tbm-parking-depot],[tbm-release-settlement],[tbm-release-traffic],[tbm-hangar-acceptance],[tbm-hangar-orders],[tbm-hangar-reuse],[tbm-hangar-service],[tbm-native-orders],[tbm-selected-hangar],[tbm-support],[tbm-backing],[tbm-route]",
    "geology": "[m1-terrain-portal],[geology-generation],[geology-generation-boundary],[geology-regional],[geology-state],[geology-state-legacy],[geology-state-refusal],[excavation-cut],[excavation-cut-refusal],[excavation-cut-reserve],[excavation-demolition],[excavation-resume-conservation],[excavation-construction-matrix],[excavation-date-boundary],[excavation-pass],[excavation-starter]",
    "repeat-exit": "[tbm-exit-journey]",
    "seed": "[.tbm-final-seed]",
    "ui": "[.tbm-feedback]",
    "exit": "[.tbm-logistics-runtime]",
}


def digest(path):
    """@param path Protected repository artifact. @return Exact SHA256 without writing its input."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(case, config, source=None):
    """Execute one bounded slice of the combined final verification matrix.

    @param case Existing focused scope or next actual GUI/save-chain stage.
    @param config Completed Debug or optimized native binary; this runner never builds.
    @param source Explicit prior native checkpoint for UI/exit continuation.
    @return Fresh native output prefix, for the next genuinely loaded stage.
    """
    serial = 1
    while (CONTEXT.logs / f"r2tbmf-zb-{config}-{case}-{serial}.log").exists(): serial += 1
    log = CONTEXT.logs / f"r2tbmf-zb-{config}-{case}-{serial}.log"
    output = CONTEXT.saves / f"tbm-final-{config.lower()}-{case}-{serial}"
    startup = RUNTIME / "scripts/game_start.scr"
    original = startup.read_bytes()
    protected = [BUILD / "native-art.cfg", BUILD / "play.cfg", resolve_input_path(FIXTURES / "underground-demo-390.sav"), resolve_input_path(FIXTURES / "underground-demo-tbm-ui-393.sav")]
    env = {key: value for key, value in os.environ.items() if not key.startswith(("OTTD_TBM_", "OTTD_GEOLOGY_", "OTTD_M1_"))}
    command = None
    if case in ("seed", "repeat-exit"):
        env["OTTD_TBM_LOGISTICS_RECORD"] = output.name
    elif case == "ui":
        source = resolve_input_path(source)
        assert resolve_input_path(source).is_file(), source
        protected.append(source)
        env.update(OTTD_TBM_FINAL_SOURCE=str(source), OTTD_TBM_FINAL_OUTPUT=output.name)
        command = "tbm_feedback\n"
    elif case == "exit":
        source = resolve_input_path(source)
        assert resolve_input_path(source).is_file(), source
        protected.append(source)
        env.update(OTTD_TBM_LOGISTICS_SOURCE=str(source), OTTD_TBM_LOGISTICS_OUTPUT=str(output),
                   OTTD_TBM_EXIT_RELOAD="1", OTTD_TBM_RELEASE_RUN="1", OTTD_TBM_EXIT_INPUT="underground", OTTD_TBM_FINAL_TRAFFIC="1")
        command = "logistics_probe\n"
    before = {path: digest(path) for path in protected}
    started = time.monotonic()
    try:
        if command: startup.write_text(command, encoding="utf-8")
        with log.open("w", encoding="utf-8") as stream:
            result = run_native(CONTEXT, [str(BUILD / config / "openttd_test.exe"), FILTERS[case]], cwd=BUILD, env=env,
                                    stdout=stream, stderr=subprocess.STDOUT, timeout=120)
        if case in ("ui", "exit"):
            from PIL import Image
            images = list(CONTEXT.capture_paths(output.name + "*.ppm")) if case == "ui" else [path for pattern in ("tbm-hangar-*.ppm", "tbm-exit-underground-*.ppm") for path in CONTEXT.capture_paths(pattern)]
            captures = []
            # Fresh run ownership makes every selected frame current evidence;
            # feature directories retain native routing through PNG conversion.
            for path in images:
                target = path.with_suffix(".png") if case == "ui" else path.parent / (output.name + "-" + path.stem + ".png")
                with Image.open(path) as picture: save_native_capture(CONTEXT, picture, path, target)
                captures.append(target)
            CONTEXT.write_report(f"{case}-provenance.json", Path(__file__).relative_to(ROOT).as_posix(),
                                 captures, [source], config, FILTERS[case])
    finally:
        startup.write_bytes(original)
        for path, expected in before.items(): assert digest(path) == expected, path
    evidence = log.read_text(encoding="utf-8")
    print(f"Final {config}/{case}: result={result.returncode}, seconds={time.monotonic()-started:.2f}, log={log.relative_to(ROOT)}", flush=True)
    for line in evidence.splitlines():
        if line.startswith(("FINAL_", "EXIT_", "FEEDBACK_EXTENSION", "ADVICE_RESULT", "All tests passed", "test cases:", "assertions:")):
            print(line, flush=True)
    result.check_returncode()
    assert "All tests passed" in evidence, log
    return output


def main():
    """@return None; run only an explicitly selected not-yet-verified or failed final slice."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--case", choices=tuple(FILTERS), required=True)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), default="RelWithDebInfo")
    parser.add_argument("--source")
    args = parser.parse_args()
    run(args.case, args.config, args.source)


if __name__ == "__main__":
    main()
