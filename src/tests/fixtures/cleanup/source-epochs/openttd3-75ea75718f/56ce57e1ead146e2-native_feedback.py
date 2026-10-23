"""Run bounded native TBM feedback reproduction, preserving the demo and startup script.

@file native_feedback.py
@see r2tbmf-tbm-fixes.md Group1
@see ../../architecture/gui/archi-gui.md
"""

import hashlib
import argparse
import json
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
from artifact_paths import resolve_input_path, RunContext, identifier
from runner import save_native_capture, current_run_context, run_native, start_native

CONTEXT = current_run_context("tbm", "native-feedback")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()



def digest(path):
    """Hash an existing evidence/input file without changing it.

    @param path Repository-owned file to identify.
    @return SHA256 hexadecimal digest of the exact bytes.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """Run one isolated native process with a bounded timeout and restore startup bytes.

    @return None; raise on failed checks or any unexpected retained-input change.
    """
    build = ROOT / "build"
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    journey = parser.add_mutually_exclusive_group()
    journey.add_argument("--long-haul", action="store_true")
    journey.add_argument("--extend-work", action="store_true")
    journey.add_argument("--straight-tunnel", action="store_true")
    parser.add_argument("--reload-demo", action="store_true")
    parser.add_argument("--fresh-layer", choices=["underground", "deep"], help="Load the prepared uncommissioned native scenario for this rock layer")
    parser.add_argument("--reload-accepted", action="store_true", help="Resume the last accepted area/tunnel snapshot and compare with its uninterrupted native result")
    parser.add_argument("--prior-run", help="exact earlier feedback invocation supplying the accepted save and comparison log")
    args = parser.parse_args()
    if args.fresh_layer:
        if args.long_haul:
            parser.error("Fresh planning and retained long-haul measurement are separate journeys")
        args.extend_work = True
    if args.straight_tunnel:
        args.extend_work = True  # The same real machine continues, using the explicit straight-intent control.
    if args.extend_work:
        args.reload_demo = True  # Extend a preserved loaded copy, never regenerate the published demo during this check.
    if args.reload_accepted and not args.extend_work:
        parser.error("--reload-accepted requires --extend-work or --straight-tunnel")
    # A continuation compares one actual previous run, never an implicit log
    # or an older immutable file with a coincidentally matching basename.
    if args.reload_accepted and not args.prior_run:
        parser.error("--reload-accepted requires --prior-run")
    prior = RunContext(ROOT, CONTEXT.feature, CONTEXT.scenario, identifier(args.prior_run)) if args.prior_run else None
    source = ROOT / "src/tests/fixtures/haulage/planning/underground-demo-390.sav"
    executable = build / "RelWithDebInfo/openttd_test.exe"
    script = RUNTIME / "scripts/game_start.scr"
    original = script.read_bytes() if script.exists() else None
    protected = [source, build / "play.cfg", build / "native-art.cfg"]
    if args.reload_demo:
        protected.append(ROOT / "src/tests/fixtures/haulage/planning/underground-demo-tbm-ui-393.sav")
    intent_name = "tunnel" if args.straight_tunnel else "area"
    evidence_prefix = f"tbm-fresh-{args.fresh_layer}-{intent_name}-" if args.fresh_layer else "tbm-tunnel-" if args.straight_tunnel else "tbm-extend-" if args.extend_work else "tbm-ui-"
    accepted_name = evidence_prefix + "393.sav" if args.fresh_layer else "tbm-extended-tunnel-393.sav" if args.straight_tunnel else "tbm-extended-area-393.sav"
    if args.fresh_layer:
        protected.append(resolve_input_path(ROOT / "src/tests/fixtures/haulage" / f"tbm-planning-{args.fresh_layer}-393.sav"))
    if args.reload_accepted:
        protected.append(resolve_input_path(prior.saves / accepted_name))
    before = {str(path.relative_to(ROOT)): digest(path) for path in protected}
    suffix = "-haul50" if args.long_haul else "-reload" if args.reload_demo else ""
    log = CONTEXT.logs / f"r2tbmf-c-native-ui{suffix}.log"
    if args.extend_work:
        log = CONTEXT.logs / "r2tbmf-e-native-extension.log"
    if args.straight_tunnel:
        log = CONTEXT.logs / "r2tbmf-f-native-tunnel.log"
    if args.fresh_layer:
        log = CONTEXT.logs / f"r2tbmf-g-native-{args.fresh_layer}-{intent_name}.log"
    reference = None
    if args.reload_accepted:
        reference = [line for line in (prior.logs / log.name).read_text(encoding="utf-8").splitlines() if line.startswith("FEEDBACK_RESULT ")]
        assert len(reference) == 1, "Run the corresponding uninterrupted journey first"
        kind = "tunnel" if args.straight_tunnel else "area"
        log = log.with_name(log.stem + "-reload.log") if args.fresh_layer else CONTEXT.logs / f"r2tbmf-f-native-{kind}-reload.log"
    environment = dict(os.environ)
    environment.pop("OTTD_TBM_FEEDBACK_LONG_HAUL", None)
    environment.pop("OTTD_TBM_FEEDBACK_RELOAD", None)
    environment.pop("OTTD_TBM_FEEDBACK_EXTEND", None)
    environment.pop("OTTD_TBM_FEEDBACK_TUNNEL", None)
    environment.pop("OTTD_TBM_FEEDBACK_FRESH", None)
    if args.fresh_layer:
        environment["OTTD_TBM_FEEDBACK_FRESH"] = args.fresh_layer
    environment.pop("OTTD_TBM_FEEDBACK_ACCEPTED", None)
    environment.pop("OTTD_TBM_FEEDBACK_ACCEPTED_SOURCE", None)
    if args.reload_accepted:
        environment["OTTD_TBM_FEEDBACK_ACCEPTED"] = "1"
        environment["OTTD_TBM_FEEDBACK_ACCEPTED_SOURCE"] = str(resolve_input_path(prior.saves / accepted_name))
    if args.straight_tunnel:
        environment["OTTD_TBM_FEEDBACK_TUNNEL"] = "1"
    if args.extend_work:
        environment["OTTD_TBM_FEEDBACK_EXTEND"] = "1"
    if args.reload_demo:
        environment["OTTD_TBM_FEEDBACK_RELOAD"] = "1"
    if args.long_haul:
        environment["OTTD_TBM_FEEDBACK_LONG_HAUL"] = "1"
    start = time.monotonic()
    try:
        script.write_text("# @see r2tbmf-tbm-fixes.md Group1 native input reproduction.\ntbm_feedback\n", encoding="utf-8")
        with log.open("wb") as output:
            result = run_native(CONTEXT, [str(executable), "[.tbm-feedback]"], cwd=build,
                                    stdout=output, stderr=subprocess.STDOUT, timeout=110, env=environment)
    finally:
        # Restore exact startup bytes even after timeout/assertion; the user's next
        # interactive launch must never invoke the isolated test-only observer.
        if original is None:
            script.unlink(missing_ok=True)
        else:
            script.write_bytes(original)
        for path in protected:
            assert digest(path) == before[str(path.relative_to(ROOT))], path
    reviews = CONTEXT.images
    # Each file is the native renderer's actual RGB output. Conversion changes
    # only its container; pixels remain comparable to later corrected UI runs.
    captures = []
    for ppm in sorted(CONTEXT.capture_paths(evidence_prefix + "*.ppm")):
        png = ppm.with_suffix(".png")
        with Image.open(ppm) as image:
            assert image.getbbox() is not None, f"Native UI capture is entirely black: {ppm}"
            save_native_capture(CONTEXT, image, ppm, png)
        captures.append(str(png.relative_to(ROOT)))
        ppm.unlink()
    CONTEXT.write_report("feedback-provenance.json", Path(__file__).relative_to(ROOT).as_posix(),
                         [ROOT / path for path in captures], protected, "RelWithDebInfo", "[.tbm-feedback]")
    print(json.dumps({"exit": result.returncode, "seconds": round(time.monotonic() - start, 2),
                      "executable_sha256": digest(executable), "inputs": before,
                      "captures": captures, "log": str(log.relative_to(ROOT))}, indent=2))
    result.check_returncode()
    if reference is not None:
        reloaded = [line for line in log.read_text(encoding="utf-8").splitlines() if line.startswith("FEEDBACK_RESULT ")]
        assert reloaded == reference, "Reload result differs from uninterrupted movement/owners; inspect both native logs"
        print("Accepted snapshot reload matches uninterrupted owners, cash, RNG and every actual train part.")


if __name__ == "__main__":
    main()
