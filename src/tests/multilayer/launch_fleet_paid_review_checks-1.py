"""Capture ordinary paid launch snapshots without granting poses, resources or progress.

@file launch_fleet_paid_review_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JD.1.
Each original phase report and save must match the accepted phase ledger. A
focused burst selects one or more phases; every native process has60seconds.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import time
from PIL import Image
from support.artifact_paths import create_run_context
import launch_facility_selector_checks as selectors

ROOT = selectors.ROOT
OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/launch-fleet-paid-review-v1"
LEDGER = ROOT / "logs/phase-ledger-verification.json"
QUALIFICATIONS = {
    "preparing": "mlm/reviews/space/paid-probe-phase-verify-preparing/20261014-122357-441519346a/reports/paid-probe-phase.json",
    "ascent": "mlm/reviews/space/paid-probe-phase-verify-ascent/20261014-122454-cc6a261537/reports/paid-probe-phase.json",
    "delivery": "mlm/reviews/space/paid-probe-phase-verify-delivery/20261014-122503-32e7f2ab72/reports/paid-probe-phase.json",
    "return": "mlm/reviews/space/paid-probe-phase-verify-return/20261014-122511-e179f1e40e/reports/paid-probe-phase.json",
    "turnaround": "mlm/reviews/space/paid-probe-phase-verify-turnaround/20261014-122519-ae2c6c2c19/reports/paid-probe-phase.json",
    "complete": "mlm/reviews/space/paid-probe-phase-verify-complete/20261014-122529-d67e85300f/reports/paid-probe-phase.json",
}


def qualified_source(phase, boundary):
    """Resolve one exact previously earned paid snapshot through its qualified report.

    @param phase Original preparation/ascent/delivery/return/turnaround/completion label.
    @param boundary Before or after that original native interval.
    @return Immutable source-save and original report identities.
    @throws ValueError On missing, changed or unqualified phase provenance.
    """
    art = selectors.art
    ledger = art.read_json(LEDGER)
    report_path = ROOT / QUALIFICATIONS[phase]
    report_ref = art.source.identity(report_path)
    if report_ref not in ledger["reports"]:
        raise ValueError("paid phase report differs from original accepted ledger")
    report = art.read_json(report_path)
    source = report[boundary]
    # The original ledger must bind the actual saved bytes as well as its
    # qualification report; a filename or current pose cannot supply paid work.
    if source not in ledger["artifacts"] or art.source.identity(ROOT/source["path"]) != source:
        raise ValueError("paid snapshot bytes differ from original accepted phase")
    return source, report_ref


def run(configuration, phase, boundary, scenario="snapshot"):
    """Load/capture an actual original paid world using current native game code.

    @param configuration Debug or RelWithDebInfo from the existing isolated art target.
    @param phase Original earned paid-flight phase to inspect.
    @param boundary Original before/after snapshot, never a synthetic pose.
    @param scenario Read-only snapshot or actual paid native assembly/work/pad queue.
    @return Pure native owner/capture observations awaiting explicit visual review.
    @throws ValueError On stale provenance, failed/overdue capture or changed paid/source owners.
    """
    source, qualification = qualified_source(phase, boundary)
    _, protected = selectors.boundary()
    application = ROOT/"build"/configuration/"openttd_space_art_test.exe"
    executable = selectors.art.source.identity(application)
    label = phase+"-"+boundary if scenario == "snapshot" else "queue"
    context = create_run_context("space", "launch-fleet-paid-"+label, run_id="launch-paid-"+str(time.time_ns()), root=ROOT, profile="launch-fleet-art", process=configuration.lower())
    context.prepare_runtime()
    (context.runtime_dir/"scripts/game_start.scr").write_text("launch_fleet_paid_review\n",encoding="utf-8")
    environment=context.environment();environment["OTTD_LAUNCH_PAID_REVIEW_SOURCE"]=str(ROOT/source["path"])
    environment["OTTD_LAUNCH_PAID_REVIEW_MODE"] = scenario
    terminal=ROOT/("logs/launch-fleet-paid-"+configuration+"-"+label+".log")
    started=time.monotonic()
    with terminal.open("w",encoding="utf-8") as output:
        result=subprocess.run([str(application),"[.launch-fleet-paid-review]","--reporter","console","--durations","yes"],cwd=ROOT/"build",env=environment,stdout=output,stderr=subprocess.STDOUT,timeout=60,check=False,creationflags=subprocess.CREATE_NO_WINDOW)
    text=terminal.read_text(encoding="utf-8",errors="replace")
    passed=re.search(r"All tests passed \(([\d,]+) assertions in 1 test case\)",text)
    # A normal process exit and actual native case are both required; no fake
    # source pose or observation-only substitute can stand for loaded gameplay.
    if result.returncode or passed is None:
        raise ValueError("paid native presentation failed; inspect "+str(terminal))
    observation=context.reports/"paid-native-observation.json"
    document=selectors.art.read_json(observation)
    if document["component_pose_inputs"] or document["simulation_ticks"] or document["gameplay_commands"] or document["owner_digests_before"] != document["owner_digests_after"]:
        raise ValueError("paid presentation changed original physical owners")
    if scenario == "queue":
        paid = document["paid_scenario"]
        # Only the actual original paid transaction and completed native work
        # may supply this occupied-pad FIFO case; fixture opportunities refuse.
        if not paid or paid["gameplay_commands"] != 3 or paid["earned_simulation_ticks"] != 148 or paid["capital_paid"] != 400000 or paid["granted_resources"] or paid["assigned_work"] or not paid["queued_after_paid_completion"]:
            raise ValueError("queue scenario did not earn original paid assembly")
        images = context.images
        working = (images/"paid-workshop-working.indexed").read_bytes()
        paused = (images/"paid-workshop-paused.indexed").read_bytes()
        if sum(left != right for left, right in zip(working, paused, strict=True)) != 3:
            raise ValueError("actual paid pause changed more than the original three cue pixels")
    # Preserve genuine rendered pixels in a portable review container. Every
    # original PPM/indexed artifact stays intact and traceable to its native run.
    for image in context.images.glob("*.ppm"):
        with Image.open(image) as pixels:
            pixels.save(image.with_suffix(".png"))
    # Presentation may not mutate the paid save, compiled application or frozen
    # native/source authorities behind any of the observed world images.
    if selectors.boundary()[1] != protected or selectors.art.source.identity(application) != executable or selectors.art.source.identity(ROOT/source["path"]) != source:
        raise ValueError("paid native input/application/source changed during capture")
    sources=("src/tests/launch_fleet_paid_review.cpp","src/tests/multilayer/launch_fleet_paid_review_checks-1.py","src/tests/native_art_render_fixture.h","src/launch_assembly_gui.cpp","CMakeLists.txt","src/tests/CMakeLists.txt")
    proof=dict(_documentation=__doc__,complete=True,configuration=configuration,phase=phase,boundary=boundary,scenario=scenario,source=source,
               qualification=qualification,ledger=selectors.art.source.identity(LEDGER),application=executable,cases=1,
               assertions=int(passed.group(1).replace(",","")),elapsed_seconds=round(time.monotonic()-started,3),
               terminal=selectors.art.source.identity(terminal),observation=selectors.art.source.identity(observation),
               images=[selectors.art.source.identity(path) for path in sorted(context.images.iterdir())],
               sources=[selectors.art.source.identity(ROOT/name) for name in sources],ordinary_paid_world_loaded=True,
               component_pose_inputs=False,simulation_ticks=0,visual_review_pending=True)
    OUTPUT.mkdir(parents=True,exist_ok=True)
    (OUTPUT/(configuration+"-"+label+".json")).write_text(json.dumps(proof,indent=2)+"\n",encoding="utf-8")
    print(configuration,phase,boundary,"assertions",proof["assertions"],"images",context.images.relative_to(ROOT),flush=True)
    return proof


def main():
    """Run a focused paid snapshot burst from explicit original qualifications."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("Debug","RelWithDebInfo"),required=True)
    parser.add_argument("--phase",choices=tuple(QUALIFICATIONS),nargs="+",required=True)
    parser.add_argument("--boundary",choices=("before","after"),required=True)
    parser.add_argument("--scenario",choices=("snapshot","queue"),default="snapshot")
    args=parser.parse_args()
    # Isolate each real paid snapshot in a fresh native process, preventing
    # another phase's caches or live owners from supplying this observation.
    for phase in args.phase:
        run(args.configuration,phase,args.boundary,args.scenario)


if __name__ == "__main__":
    main()
