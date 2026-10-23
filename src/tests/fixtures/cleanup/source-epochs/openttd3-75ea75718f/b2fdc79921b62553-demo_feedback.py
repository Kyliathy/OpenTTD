"""Run one bounded native reproduction of a 2026-10-15 demo report on the real demo world.

The native observer (src/tests/demo_feedback_runtime.cpp, Catch2 tag [.demo-feedback]) loads the selected save
through the ordinary loader, unpauses it and logs the facts behind each report. This runner owns the run identity,
startup script, input protection and capture conversion; it never edits the input save.

@file demo_feedback.py
@see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Groups 1-3, 5 and 6 (--replay: task 6.7) task 9.7 (the "approach" scenario) and task 12.6 (the "change" scenario)
@see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 8 (the "face-loop" scenario, --view, --face-transfers,
--equivalent-to and --bend)
@see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 9 (--saves, --lifecycle and --allow-bay: the face-loop
checkpoints that face_save.py reloads)
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from PIL import Image

# Repository markers identify the checkout independently of the caller's working directory.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path  # noqa: E402
from runner import save_native_capture, current_run_context, run_native  # noqa: E402

#: Reproductions the native observer implements; each is also the artifact scenario name.
SCENARIOS = ("trace", "hauler-depot", "forced-hauler", "unload-order", "held", "new-area", "obstruction", "approach", "change", "poses", "map", "windows", "vehicle-spacing", "framerate",
             "face-loop")

#: Face-loop rows that depend on the simulation only; runs that differ only in their view must log them identically.
FACE_EVIDENCE = {"DEMO_FACE_PLAN": "plan", "DEMO_FACE_BEND": "bend", "DEMO_FACE_CUT": "cuts", "DEMO_FACE_PHASE": "phases", "DEMO_FACE_TRANSFER": "transfers",
                 "DEMO_FACE_STATE": "states", "DEMO_FACE_RATE": "rate", "DEMO_FACE_SUMMARY": "summary"}

#: The demo checkpoint play.ps1 tbm-work (menu 11) starts, refreshed at version 425 by r2df2 task 10.2 from the 403
#: snapshot the user played; never rewritten by this runner.
DEFAULT_INPUT = "src/tests/fixtures/demos/underground/tbm-work-425.sav"


def digest(path):
    """Hash an input file without changing it.

    @param path Existing file.
    @return SHA256 hexadecimal digest of its exact bytes.
    """
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def framerate_environment(context):
    """Select the real video driver and a copy of the player's own play.cfg for the frame-rate reading.

    The null driver runs ticks without wall-clock pacing, so its rates mean nothing. With the Windows driver the
    observer's ticks are paced like the player's game, and the copy carries the player's refresh rate, vsync,
    hardware acceleration and blitter. The game window is visible for the bounded run.

    @param context Run context owning the runtime home.
    @return Environment additions for the native observer.
    """
    config = context.prepare_runtime("play.cfg")
    return {"OTTD_DEMO_FEEDBACK_VIDEO": "win32", "OTTD_DEMO_FEEDBACK_CONFIG": str(config)}


def report_framerate(context):
    """Read the Frame rate window's numbers that the observer printed through the 'fps' console command.

    @param context Run context owning the logs.
    @return The console text; raises when no measurement was printed.
    """
    console_log = context.logs / "framerate-console.log"
    text = console_log.read_text(encoding="utf-8", errors="replace") if console_log.is_file() else ""
    assert "Game loop" in text, f"no frame-rate measurement was printed to {console_log}"
    return text


def collect_face_evidence(log):
    """Gather the face loop's evidence rows from a native log.

    @param log Native observer log of a face-loop run.
    @return Dictionary of row lists by kind; the plan and summary rows drop their view field.
    """
    evidence = {kind: [] for kind in FACE_EVIDENCE.values()}
    for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
        tag, _, rest = line.partition(" ")
        if tag in FACE_EVIDENCE:
            # The view names the run, not the simulation; it is the one field allowed to differ.
            fields = [field for field in rest.split(" ") if not field.startswith("view=")]
            evidence[FACE_EVIDENCE[tag]].append(" ".join(fields))
    return evidence


def compare_face_evidence(evidence, other_path):
    """Compare this run's face-loop evidence with an earlier run's, row by row.

    @param evidence This run's evidence.
    @param other_path Evidence JSON written by an earlier face-loop run.
    @return Dictionary naming each kind of row and whether it matched.
    """
    other = json.loads(Path(other_path).read_text(encoding="utf-8"))["evidence"]
    return {kind: evidence[kind] == other.get(kind, []) for kind in evidence}


def main():
    """Run one isolated native observer with a bounded timeout, then restore the startup script.

    @return None; raises on a failed native check or any change to a protected input.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--scenario", choices=SCENARIOS, required=True, help="report reproduction to run")
    parser.add_argument("--input", default=DEFAULT_INPUT, help="declared input save (fixture, prior run or build/save)")
    parser.add_argument("--ticks", type=int, default=6000, help="normal simulation ticks to observe after unpausing")
    parser.add_argument("--config", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo", help="test executable build")
    parser.add_argument("--timeout", type=int, default=110, help="seconds before the native child is stopped")
    parser.add_argument("--hauler-stop", type=int, default=120, help="obstruction: ticks after forcing the former hauler out before it is stopped")
    parser.add_argument("--replay", type=int, default=0, help="log this many ticks after loading a checkpoint saved by an earlier run, then stop")
    parser.add_argument("--view", choices=("headless", "visible", "hidden"), default="headless", help="face-loop camera: none, following the cutter, or on the Surface above it")
    parser.add_argument("--face-transfers", type=int, default=3, help="face-loop: face transfers to observe before stopping")
    parser.add_argument("--equivalent-to", help="face-loop: evidence JSON of an earlier run that must match this one row for row")
    parser.add_argument("--bend", action="store_true", help="face-loop: resume the loaded project, start its trains and bend its route at 45 degrees first")
    parser.add_argument("--saves", default="", help="face-loop: comma-separated checkpoint states to save (travelling, held, dwell, leaving, bay-return)")
    parser.add_argument("--lifecycle", choices=("", "stop", "park", "release"), default="", help="face-loop: player action whose result is saved")
    parser.add_argument("--allow-bay", action="store_true", help="face-loop: bay exchanges are expected (a route starting at its portal)")
    args = parser.parse_args()

    context = current_run_context("demo-fixes", args.scenario)
    context.prepare_runtime()
    build = ROOT / "build"
    executable = build / args.config / "openttd_test.exe"
    source = resolve_input_path(args.input)
    # The demo snapshot, the player's own config and the startup script must leave this run byte-identical.
    protected = [source, build / "play.cfg", build / "hotkeys.cfg"]
    before = {str(path.relative_to(ROOT)): digest(path) for path in protected if path.is_file()}
    script = context.runtime_dir / "scripts/game_start.scr"
    original = script.read_bytes() if script.exists() else None
    log = context.logs / f"demo-{args.scenario}.log"
    environment = dict(os.environ)
    environment["OTTD_DEMO_FEEDBACK_SCENARIO"] = args.scenario
    environment["OTTD_DEMO_FEEDBACK_INPUT"] = str(source)
    environment["OTTD_DEMO_FEEDBACK_TICKS"] = str(args.ticks)
    environment["OTTD_DEMO_HAULER_STOP"] = str(args.hauler_stop)
    environment["OTTD_DEMO_REPLAY"] = str(args.replay)
    environment["OTTD_TEST_CONFIGURATION"] = args.config
    environment["OTTD_DEMO_VIEW"] = args.view
    environment["OTTD_DEMO_FACE_TRANSFERS"] = str(args.face_transfers)
    environment["OTTD_DEMO_FACE_BEND"] = "1" if args.bend else "0"
    environment["OTTD_DEMO_FACE_ALLOW_BAY"] = "1" if args.allow_bay or args.bend else "0"
    if args.saves:
        environment["OTTD_DEMO_FACE_SAVES"] = args.saves
    environment["OTTD_DEMO_FACE_LIFECYCLE"] = args.lifecycle
    if args.scenario == "framerate":
        environment.update(framerate_environment(context))
    start = time.monotonic()
    try:
        script.parent.mkdir(parents=True, exist_ok=True)
        script.write_text("# @see r2df-demo-fixes-interface-movement-and-planning.md Group 1 native reproduction.\ndemo_feedback\n", encoding="utf-8")
        with log.open("wb") as output:
            result = run_native(context, [str(executable), "[.demo-feedback]"], cwd=build,
                                stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout, env=environment)
    finally:
        # The next interactive launch must never run the test-only observer.
        if original is None:
            script.unlink(missing_ok=True)
        else:
            script.write_bytes(original)
        for path in protected:
            if path.is_file():
                assert digest(path) == before[str(path.relative_to(ROOT))], f"protected input changed: {path}"
    # Each PPM is the native renderer's actual RGB output; PNG conversion changes only the container.
    captures = []
    for ppm in sorted(context.capture_paths("*.ppm")):
        png = ppm.with_suffix(".png")
        with Image.open(ppm) as image:
            assert image.getbbox() is not None, f"Native capture is entirely black: {ppm}"
            save_native_capture(context, image, ppm, png)
        captures.append(str(png.relative_to(ROOT)))
        ppm.unlink()
    # Provenance lists review artifacts only; the frame-rate console log stays beside the run's other logs.
    context.write_report("demo-feedback-provenance.json", Path(__file__).relative_to(ROOT).as_posix(),
                         [ROOT / path for path in captures], [source], args.config, "[.demo-feedback]")
    print(json.dumps({"scenario": args.scenario, "exit": result.returncode, "seconds": round(time.monotonic() - start, 2),
                      "executable_sha256": digest(executable), "inputs": before, "captures": captures,
                      "log": str(log.relative_to(ROOT))}, indent=2))
    if args.scenario == "face-loop":
        # The evidence beside the run's reports; a later run with another view compares itself with it.
        evidence = collect_face_evidence(log)
        report = context.reports / "face-loop-evidence.json"
        report.parent.mkdir(parents=True, exist_ok=True)
        report.write_text(json.dumps({"view": args.view, "input": str(source), "evidence": evidence}, indent=2), encoding="utf-8")
        print(json.dumps({"face_evidence": str(report.relative_to(ROOT)), "transfers": len(evidence["transfers"]), "states": len(evidence["states"])}))
        if args.equivalent_to:
            matches = compare_face_evidence(evidence, args.equivalent_to)
            print(json.dumps({"equivalent_to": args.equivalent_to, "matches": matches}))
            assert all(matches.values()), f"face-loop evidence differs from {args.equivalent_to}: {matches}"
    result.check_returncode()
    if args.scenario == "framerate":
        print(report_framerate(context))


if __name__ == "__main__":
    main()
