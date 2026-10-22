"""Retain real moving TBM saves and compare their frame-by-frame native continuations.

@file native_motion.py
@see r2tbmf-tbm-fixes.md Group4
@see ../../architecture/savegame/archi-savegame.md
"""

import argparse
import hashlib
import os
from pathlib import Path
import re
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

CONTEXT = current_run_context("tbm", "motion")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

KINDS = ("approach", "bend", "wait")


def digest(path):
    """Identify retained bytes without mutation.

    @param path Existing repository-owned input.
    @return SHA256 digest.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """Run one native baseline or exact real-file continuation and restore startup bytes.

    @return None; raise on native failure, changed protected inputs or any mismatched frame.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--replay", choices=KINDS)
    mode.add_argument("--pace-seed-layer", choices=["underground", "deep"], help="Retain a genuinely driven old-rate partial face before installing new tuning")
    mode.add_argument("--pace-check-layer", choices=["underground", "deep"], help="Verify the migrated old fraction and actual half-speed continuation")
    parser.add_argument("--pace-resaved", action="store_true", help="Load the migrated current-version copy and compare its next cut exactly")
    parser.add_argument("--pace-fraction", action="store_true", help="Reload the naturally earned new-rate half/quarter and compare the same actual cut")
    parser.add_argument("--prior-run", help="exact motion baseline invocation for replay")
    args = parser.parse_args()
    # Replay must identify the baseline whose frames and native saves it compares.
    if args.replay and not args.prior_run:
        parser.error("--replay requires --prior-run")
    if args.pace_resaved and not args.pace_check_layer:
        parser.error("--pace-resaved requires --pace-check-layer")
    if args.pace_fraction and (not args.pace_check_layer or args.pace_resaved):
        parser.error("--pace-fraction requires --pace-check-layer, independently of --pace-resaved")
    build = ROOT / "build"
    fixtures = ROOT / "src/tests/fixtures/haulage"
    reference_path = CONTEXT.logs / "r2tbmf-h-motion-native.log"
    log = reference_path if not args.replay else CONTEXT.logs / f"r2tbmf-h-motion-{args.replay}-reload.log"
    env = dict(os.environ)
    env.pop("OTTD_TBM_MOTION_REPLAY", None)
    env.pop("OTTD_TBM_MOTION_TICK", None)
    env.pop("OTTD_TBM_PACE_SEED", None)
    env.pop("OTTD_TBM_PACE_CHECK", None)
    env.pop("OTTD_TBM_PACE_RESAVED", None)
    env.pop("OTTD_TBM_PACE_FRACTION", None)
    protected = [resolve_input_path(fixtures / "underground-demo-390.sav"), resolve_input_path(fixtures / "underground-demo-tbm-ui-393.sav"),
                 build / "play.cfg", build / "native-art.cfg"]
    reference = None
    tick = 0
    if args.pace_seed_layer:
        env["OTTD_TBM_PACE_SEED"] = args.pace_seed_layer
        log = CONTEXT.logs / f"r2tbmf-j-pace-seed-{args.pace_seed_layer}.log"
        protected.append(resolve_input_path(fixtures / f"tbm-fresh-{args.pace_seed_layer}-area-393.sav"))
    if args.pace_check_layer:
        env["OTTD_TBM_PACE_CHECK"] = args.pace_check_layer
        suffix = "-fraction-reload" if args.pace_fraction else "-reload" if args.pace_resaved else ""
        log = CONTEXT.logs / f"r2tbmf-j-pace-new-{args.pace_check_layer}{suffix}.log"
        protected.append(resolve_input_path(fixtures / f"tbm-pace-{args.pace_check_layer}-393.sav"))
        if args.pace_fraction:
            env["OTTD_TBM_PACE_FRACTION"] = "1"
            protected.append(resolve_input_path(fixtures / f"tbm-pace-{args.pace_check_layer}-fraction-394.sav"))
        if args.pace_resaved:
            env["OTTD_TBM_PACE_RESAVED"] = "1"
            protected.append(resolve_input_path(fixtures / f"tbm-pace-{args.pace_check_layer}-394.sav"))
    if args.replay:
        from artifact_paths import RunContext, identifier
        prior = RunContext(ROOT, CONTEXT.feature, CONTEXT.scenario, identifier(args.prior_run))
        reference_path = prior.logs / "r2tbmf-h-motion-native.log"
        env["OTTD_TBM_REPLAY_SOURCE"] = str(resolve_input_path(prior.saves / f"tbm-motion-{args.replay}-393.sav"))
        reference = reference_path.read_text(encoding="utf-8").splitlines()
        matches = [line for line in reference if line.startswith(f"MOTION_SNAPSHOT kind={args.replay} ")]
        assert len(matches) == 1, "Run the uninterrupted native baseline first"
        tick = int(re.search(r"tick=(\d+)", matches[0]).group(1))
        env["OTTD_TBM_MOTION_REPLAY"] = args.replay
        env["OTTD_TBM_MOTION_TICK"] = str(tick)
        protected.append(Path(env["OTTD_TBM_REPLAY_SOURCE"]))
    before = {path: digest(path) for path in protected}
    script = RUNTIME / "scripts/game_start.scr"
    original = script.read_bytes() if script.exists() else None
    started = time.monotonic()
    try:
        script.write_text("# @see r2tbmf-tbm-fixes.md Group4 native motion replay.\ntbm_motion\n", encoding="utf-8")
        with log.open("wb") as output:
            result = run_native(CONTEXT, [str(build / "RelWithDebInfo/openttd_test.exe"), "[.tbm-motion-reload]"],
                                    cwd=build, env=env, stdout=output, stderr=subprocess.STDOUT, timeout=110)
    finally:
        # A timeout/assertion must never leave the user's next normal launch
        # invoking this isolated probe, nor change any supplied input bytes.
        if original is None:
            script.unlink(missing_ok=True)
        else:
            script.write_bytes(original)
        for path in protected:
            assert digest(path) == before[path], path
    print(f"Native {args.replay or 'baseline'}: exit={result.returncode}, seconds={time.monotonic() - started:.2f}, log={log.relative_to(ROOT)}")
    result.check_returncode()
    actual = log.read_text(encoding="utf-8").splitlines()
    if not args.replay and not args.pace_seed_layer and not args.pace_check_layer:
        # Convert only the native renderer's exact pixels, retaining useful
        # review files while refusing a silent all-black null-renderer result.
        reviews = CONTEXT.images
        for kind in KINDS:
            ppm = capture_path(CONTEXT, f"tbm-motion-{kind}.ppm")
            with Image.open(ppm) as image:
                assert image.getbbox() is not None, ppm
                save_native_capture(CONTEXT, image, ppm, ppm.with_suffix(".png"))
            ppm.unlink()
    if reference is not None:
        expected = [line for line in reference if line.startswith("MOTION_TRACE ") and int(re.search(r"tick=(\d+)", line).group(1)) >= tick]
        observed = [line for line in actual if line.startswith("MOTION_TRACE ")]
        # Compare every complete native frame, beginning with the state rebuilt
        # by the loader. Final receipts alone could conceal a wrong route.
        assert len(observed) == len(expected), (len(observed), len(expected))
        for wanted, got in zip(expected, observed):
            assert wanted == got, f"Native replay diverged:\nexpected {wanted}\nactual   {got}"
        assert [line for line in actual if line.startswith("MOTION_RESULT ")] == [line for line in reference if line.startswith("MOTION_RESULT ")]
        first_cursor = int(re.search(r"cursor=(\d+)", observed[0]).group(1))
        last_cursor = int(re.search(r"cursor=(\d+)", observed[-1]).group(1))
        assert last_cursor > first_cursor, "The continuation must make the next actual cut"
        print(f"Exact match: {len(observed)} frames, cursor {first_cursor}->{last_cursor}, every native part/cargo/support reservation and owner digest.")
    elif args.pace_check_layer:
        old_log = f"r2tbmf-j-pace-new-{args.pace_check_layer}.log" if args.pace_fraction else f"r2tbmf-j-pace-seed-{args.pace_check_layer}.log"
        old_lines = (CONTEXT.logs / old_log).read_text(encoding="utf-8").splitlines()
        old = next(line for line in old_lines if line.startswith("PACE_FRACTION " if args.pace_fraction else "PACE_SEED ")).split(" clock=", 1)[1]
        new = next(line for line in actual if line.startswith("PACE_LOADED ")).split(" clock=", 1)[1]
        # The only loaded owner change is the numerically equivalent new work
        # fraction, which changes the project digest. All other saved state is
        # byte-for-byte equal in the canonical native observation.
        assert re.sub(r"project=\d+", "project=<migrated>", old) == re.sub(r"project=\d+", "project=<migrated>", new)
        if args.pace_fraction:
            assert old == new, "New fractional save must reload without any migration change"
            expected_cut = next(line for line in old_lines if line.startswith("PACE_CUT ")).split(" clock=", 1)[1]
            actual_cut = next(line for line in actual if line.startswith("PACE_CUT ")).split(" clock=", 1)[1]
            assert actual_cut == expected_cut, "Splitting execution at the actual fractional save changed the next cut"
        if args.pace_resaved:
            first = (CONTEXT.logs / f"r2tbmf-j-pace-new-{args.pace_check_layer}.log").read_text(encoding="utf-8").splitlines()
            assert [line for line in actual if line.startswith(("PACE_LOADED ", "PACE_CUT "))] == [line for line in first if line.startswith(("PACE_LOADED ", "PACE_CUT "))]
        print("Pacing migration preserves terrain, cargo, partial work, cash, RNG and native positions; next cut matches the half-speed budget.")
    elif not args.pace_seed_layer:
        for kind in KINDS:
            assert any(line.startswith(f"MOTION_SNAPSHOT kind={kind} ") for line in actual), kind
    else:
        assert any(line.startswith(f"PACE_SEED layer={args.pace_seed_layer} ") for line in actual)
    print(f"Executable SHA256: {digest(build / 'RelWithDebInfo/openttd_test.exe')}")


if __name__ == "__main__":
    main()
