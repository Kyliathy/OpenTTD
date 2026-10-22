"""Regenerate one bounded native-art proof phase into ignored evidence storage.

@file regenerate_art_proofs.py Run initialize, focused, each capture, then finalize.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups16/19/20.
Source conversion and the two packed-build modes are separate focused commands.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

from evidence import EvidenceSet, ROOT, identity, contained, selection_document, bind_capture
from runner import current_run_context
from artifact_paths import create_run_context
from artifact_rebuild import load_recipe

CAPTURES = {
    "terrain": ("src/tests/multilayer_terrain_capture.py", "geology", "terrain-hardness", "m1-terrain-runtime.json", []),
    "control": ("src/tests/multilayer_control_capture.py", "construction", "layer-controls", "m1-control-runtime.json", ["--job-inspection", "--survey-gallery"]),
    "loop-deep": ("src/tests/multilayer_loop_capture.py", "tbm", "spoil-loop-deep", "m1-loop-deep-1-runtime.json", ["--scenario", "deep", "--gallery-proof"]),
    "loop-underground": ("src/tests/multilayer_loop_capture.py", "tbm", "spoil-loop-underground", "m1-loop-underground-1-runtime.json", ["--scenario", "underground", "--gallery-proof", "--sound-proof"]),
    "facility": ("src/tests/multilayer_facility_capture.py", "facilities", "service-depots", "m1-facility-runtime.json", []),
    "geology": ("src/tests/multilayer/geology/native_checks.py", "geology", "terrain-hardness", "geology-runtime.json",
                ["--case", "native", "--source", "src/tests/fixtures/haulage/tbm-geology-journey-1-regional-midcut.sav"]),
}
FOCUSED = ("[m1-terrain-portal],[m1-control-selection],[m1-train-art],[m1-facility-selection],"
           "[m1-sprite-selection],[m1-sound-selection],[geology-regional],[geology-state]")


def save_selection(path, document):
    """Write the current explicit selection after its phase has succeeded.

    @param path Selected manifest in generated storage.
    @param document Actual hash-bound observations accumulated from completed phases.
    @return None; incomplete manifests cannot pass validators asking for missing roles.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


def execute(context, arguments, name, cwd=ROOT):
    """Run one bounded proof producer and capture its actual exit status/output.

    @param context Explicit native invocation inherited by the child process.
    @param arguments Structured Python or native argument sequence.
    @param name Unique phase log basename in this invocation.
    @param cwd Required working directory of the native or source tool.
    @return Actual required transcript in evidence/; failures retain it and raise.
    """
    log = context.output("evidence", name + ".log")
    serial = 1
    # A corrected phase keeps its previous failure diagnostics. The selection
    # records the returned concrete log, so no latest-log inference is needed.
    while log.exists():
        log = context.output("evidence", f"{name}-{serial}.log")
        serial += 1
    with log.open("wb") as sink:
        subprocess.run([str(value) for value in arguments], cwd=cwd, env=context.environment(),
                       stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=120,
                       creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
    return log


def main():
    """Execute one selected phase, preserving source art and compatibility inputs.

    @return None; completed records carry real output identities, never placeholder logs.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("phase", choices=("initialize", "focused", *CAPTURES, "finalize"))
    parser.add_argument("--manifest", type=Path, help="exact schema2 selection in the chosen review run")
    parser.add_argument("--run-id", help="fresh explicit invocation for initialize; recipe children inherit their invocation")
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="RelWithDebInfo")
    args = parser.parse_args()
    if args.phase == "initialize":
        # Initialization must belong to a user-selected or inherited recipe
        # invocation, rather than silently choosing an old or latest proof set.
        if args.run_id is None and "OTTD_TEST_RUN_ID" not in os.environ:
            raise ValueError("initialize requires --run-id or an explicitly inherited recipe invocation")
        context = (create_run_context("multilayer-art", "native-proof", run_id=args.run_id)
                   if args.run_id is not None else current_run_context("multilayer-art", "native-proof"))
        path = contained(ROOT / args.manifest, context.reports) if args.manifest else context.reports / "evidence.json"
        if path.exists():
            raise ValueError("selection already exists; select a fresh invocation")
        save_selection(path, selection_document(context, args.configuration))
        print(f"Initialized native proof invocation {context.run_id}: {path.relative_to(ROOT)}", flush=True)
        return
    if args.manifest is None:
        raise ValueError("select --manifest from the explicit recipe run; old caches are historical only")
    selection = EvidenceSet(args.manifest)
    path = selection.path
    document = selection.document
    os.environ["OTTD_TEST_RUN_ID"] = document["run_id"]
    context = selection.context.for_feature(selection.context.feature, selection.context.scenario)
    configuration = document["configuration"]
    prerequisites = load_recipe("m1-native-loader", configuration)
    # Missing retained approval roots or pending native adapters cannot be
    # replaced by yesterday's gallery. Diagnose sources until their owners pass.
    if not prerequisites["executable"]:
        raise ValueError(f"selected graphics prerequisites are missing/pending: {prerequisites['missing_inputs']} {prerequisites['pending_groups']}")
    if args.phase == "focused":
        if "terrain-checks" in document["evidence"]:
            raise ValueError("focused phase already recorded for this invocation")
        log = execute(context, [ROOT / "build" / configuration / "openttd_test.exe", FOCUSED], "focused", ROOT / "build")
        assert "All tests passed" in log.read_text(encoding="utf-8", errors="replace")
        # One actual bounded owner suite can satisfy several named obligations;
        # every alias still binds the exact same observed log bytes.
        for role in ("terrain-checks", "control-checks", "vehicle-checks", "facility-checks", "geology-checks", "sound-selection"):
            document["evidence"][role] = identity(log)
        save_selection(path, document)
    elif args.phase in CAPTURES:
        if args.phase in document["reports"]:
            raise ValueError("capture phase already recorded; choose a new invocation for another run")
        script, feature, scenario, report_name, options = CAPTURES[args.phase]
        # All recipe leaves share the explicit invocation. Partial failures
        # require a fresh run rather than borrowing frames from an older child.
        capture_context = context.for_feature(feature, scenario)
        arguments = [sys.executable, "-B", ROOT / script, *options]
        if args.phase != "geology":
            arguments += ["--configuration", configuration]
        execute(capture_context, arguments, "capture-" + args.phase)
        report_path = capture_context.reports / report_name
        report = bind_capture(capture_context, report_path, configuration)
        selected_path = capture_context.output("reports", "selected-" + args.phase + ".json")
        save_selection(selected_path, report)
        document["reports"][args.phase] = identity(selected_path)
        document["evidence"][args.phase + "-capture"] = report["terminal"]
        save_selection(path, document)
        EvidenceSet(path).report(args.phase)
    else:
        # All required real scenarios must exist before packed/runtime/gallery
        # publication; a successful subset cannot substitute for a cold proof.
        for role in CAPTURES:
            selection.report(role)
        packed = json.loads(selection.output("packed").read_text(encoding="utf-8"))
        assert packed["packed_grf"]["loader_log"] == selection.terminal("packed-loader"), "selected packed loader evidence differs"
        # The Underground journey includes actual backwards motion as well as
        # cutting, unloading and idle. Deep alone does not exercise all sounds.
        sound_role = "loop-underground"
        sound_report = selection.report(sound_role)
        execute(context, [sys.executable, "-B", ROOT / "media/multilayer/verify_m1_sound.py",
                          "--evidence-manifest", path,
                          "--evidence", selection.capture_terminal(sound_report), "--capture", document["reports"][sound_role]["path"],
                          "--selection-log", document["evidence"]["sound-selection"]["path"],
                          "--output", selection.output("sound"), "--cache-only"], "sound")
        execute(context, [sys.executable, "-B", ROOT / "media/multilayer/activate_m1_art.py",
                          "--evidence-manifest", path, "--cache-only"], "activation")
        execute(context, [sys.executable, "-B", ROOT / "media/multilayer/review_m1_art.py",
                          "--evidence-manifest", path, "--cache-only"], "gallery")
    print(f"Native proof phase {args.phase} completed: {path.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
