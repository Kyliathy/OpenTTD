"""Regenerate one bounded native-art proof phase into ignored evidence storage.

@file regenerate_art_proofs.py Run initialize, focused, each capture, then finalize.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Groups 6, 9 and 10.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md Group 10R (task 10R.4): the vent collector's checks and capture.
Source conversion and the two packed-build modes are separate focused commands.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import uuid

from evidence import CACHE, DEFAULT_MANIFEST, EvidenceSet, ROOT, identity, contained
from runner import current_run_context
from artifact_paths import create_run_context

# Each capture: producer script, feature, scenario, report basename and producer options. A report basename may
# name the build configuration, for a producer that writes one report per configuration.
CAPTURES = {
    "terrain": ("src/tests/multilayer_terrain_capture.py", "geology", "terrain-hardness", "m1-terrain-runtime.json", []),
    "control": ("src/tests/multilayer_control_capture.py", "construction", "layer-controls", "m1-control-runtime.json", ["--job-inspection", "--survey-gallery"]),
    "loop-deep": ("src/tests/multilayer_loop_capture.py", "tbm", "spoil-loop-deep", "m1-loop-deep-1-runtime.json", ["--scenario", "deep", "--gallery-proof"]),
    "loop-underground": ("src/tests/multilayer_loop_capture.py", "tbm", "spoil-loop-underground", "m1-loop-underground-1-runtime.json", ["--scenario", "underground", "--gallery-proof", "--sound-proof"]),
    "facility": ("src/tests/multilayer_facility_capture.py", "facilities", "service-depots", "m1-facility-runtime.json", []),
    "geology": ("src/tests/multilayer/geology/native_checks.py", "geology", "terrain-hardness", "geology-runtime.json",
                ["--case", "native", "--source", "src/tests/fixtures/haulage/tbm-geology-journey-1-regional-midcut.sav"]),
    # The vent collector (Plan14 79C) is a packed family too, so a cold proof must capture its native stages; before
    # 10R.4 only `activate_m1_art.py --collector-native` bound it, and the generic finalize lacked its evidence.
    "collector": ("src/tests/multilayer/collector_runtime_checks.py", "geothermal", "collector-art", "native-{configuration}.json", ["native"]),
}
FOCUSED = ("[m1-terrain-portal],[m1-control-selection],[m1-train-art],[m1-facility-selection],"
           "[m1-sprite-selection],[m1-sound-selection],[geology-regional],[geology-state],"
           "[collector-art-selection],[geothermal-collector],[geothermal-heat-source]")


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
    @return Actual successful diagnostic log; failures retain it and raise.
    """
    log = context.output("logs", name + ".log")
    serial = 1
    # A corrected phase keeps its previous failure diagnostics. The selection
    # records the returned concrete log, so no latest-log inference is needed.
    while log.exists():
        log = context.output("logs", f"{name}-{serial}.log")
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
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="RelWithDebInfo")
    args = parser.parse_args()
    path = contained(args.manifest if args.manifest.is_absolute() else ROOT / args.manifest, CACHE)
    if args.phase == "initialize":
        if path.exists():
            raise ValueError("selection already exists; choose a new manifest or preserve the old cache before cold regeneration")
        context = current_run_context("multilayer-art", "native-proof")
        document = {"_documentation": "Owner: r2fc-file-cleanup.md; actual regenerated native evidence, no user acceptance implied",
                    "schema_version": 1, "provenance": "native_run", "run_id": context.run_id,
                    "configuration": args.configuration, "reports": {}, "logs": {},
                    "outputs": {"packed": (CACHE / "derived/packed-manifest.json").relative_to(ROOT).as_posix(),
                                "runtime": (path.parent / "runtime-manifest.json").relative_to(ROOT).as_posix(),
                                "sound": (path.parent / "sound-runtime.json").relative_to(ROOT).as_posix(),
                                "gallery": (path.parent / "gallery/m1-runtime-gallery.json").relative_to(ROOT).as_posix()}}
        save_selection(path, document)
        print(f"Initialized native proof invocation {context.run_id}: {path.relative_to(ROOT)}", flush=True)
        return
    selection = EvidenceSet(path)
    document = selection.document
    os.environ["OTTD_TEST_RUN_ID"] = document["run_id"]
    context = current_run_context("multilayer-art", "native-proof")
    configuration = document["configuration"]
    if args.phase == "focused":
        if "terrain-checks" in document["logs"]:
            raise ValueError("focused phase already recorded for this invocation")
        log = execute(context, [ROOT / "build" / configuration / "openttd_test.exe", FOCUSED], "focused", ROOT / "build")
        assert "All tests passed" in log.read_text(encoding="utf-8", errors="replace")
        # One actual bounded owner suite can satisfy several named obligations;
        # every alias still binds the exact same observed log bytes.
        for role in ("terrain-checks", "control-checks", "vehicle-checks", "facility-checks", "geology-checks", "sound-selection",
                     "collector-checks"):
            document["logs"][role] = identity(log)
        save_selection(path, document)
    elif args.phase in CAPTURES:
        if args.phase in document["reports"]:
            raise ValueError("capture phase already recorded; choose a new invocation for another run")
        script, feature, scenario, report_name, options = CAPTURES[args.phase]
        # Each process attempt owns a fresh reservation, including a retry after
        # partial rendering. Its mixed-feature frames still share that one ID;
        # the gallery explicitly combines the selected successful reports.
        capture_context = create_run_context(feature, scenario,
                                             run_id=context.run_id + "-" + args.phase + "-" + uuid.uuid4().hex[:8])
        arguments = [sys.executable, "-B", ROOT / script, *options]
        if args.phase != "geology":
            arguments += ["--configuration", configuration]
        execute(capture_context, arguments, "capture-" + args.phase)
        report_path = capture_context.reports / report_name.format(configuration=configuration)
        report = json.loads(report_path.read_text(encoding="utf-8"))
        document["reports"][args.phase] = identity(report_path)
        document["logs"][args.phase + "-capture"] = identity(ROOT / report["log"])
        save_selection(path, document)
        EvidenceSet(path).report(args.phase)
    else:
        # All required real scenarios must exist before packed/runtime/gallery
        # publication; a successful subset cannot substitute for a cold proof.
        for role in CAPTURES:
            selection.report(role)
        import pack_m1_art as pack
        pack.bind_packed_payload(publish_source=False)
        packed = json.loads(selection.output("packed").read_text(encoding="utf-8"))
        document["logs"]["packed-loader"] = identity(ROOT / packed["packed_grf"]["loader_log"])
        save_selection(path, document)
        # The Underground journey includes actual backwards motion as well as
        # cutting, unloading and idle. Deep alone does not exercise all sounds.
        sound_role = "loop-underground"
        sound_report = selection.report(sound_role)
        execute(context, [sys.executable, "-B", ROOT / "media/multilayer/verify_m1_sound.py",
                          "--evidence", sound_report["log"], "--capture", document["reports"][sound_role]["path"],
                          "--selection-log", document["logs"]["sound-selection"]["path"],
                          "--output", selection.output("sound"), "--cache-only"], "sound")
        execute(context, [sys.executable, "-B", ROOT / "media/multilayer/activate_m1_art.py",
                          "--evidence-manifest", path, "--cache-only"], "activation")
        execute(context, [sys.executable, "-B", ROOT / "media/multilayer/review_m1_art.py",
                          "--evidence-manifest", path, "--cache-only"], "gallery")
    print(f"Native proof phase {args.phase} completed: {path.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
