"""Capture original paid passive descent and one actual atomic recovery opportunity.

@file launch_pod_review_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDC.2.
Original qualified custody reports bind each exact saved shell/packet state.
No fixture grants or source poses; recovery mode earns one normal native tick.
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

ROOT=selectors.ROOT
OUTPUT=ROOT/"logs/proofs/mlm/native-art/space/launch-pod-review-v1"
INDEX=ROOT/"logs/descent-custody-verification.json"
QUALIFICATIONS={
 "release":"mlm/reviews/space/descent-custody-verify-release/20261014-090201-b09c8c33be/reports/descent-custody.json",
 "before-first":"mlm/reviews/space/descent-custody-verify-before-first/20261014-090205-87509b36a7/reports/descent-custody.json",
 "first":"mlm/reviews/space/descent-custody-verify-first/20261014-090209-6db1145d70/reports/descent-custody.json",
 "before-second":"mlm/reviews/space/descent-custody-verify-before-second/20261014-090214-aaa7b82775/reports/descent-custody.json",
 "second":"mlm/reviews/space/descent-custody-verify-second/20261014-090219-b4e0cfd1b2/reports/descent-custody.json",
}


def run(configuration,case):
    """Load and inspect exact original paid shell/cargo state in the actual game.

    @param configuration Debug or RelWithDebInfo existing isolated actual-engine target.
    @param case Qualified original phase or recover from its actual ready-before-first snapshot.
    @return Native ownership/art/window observations with explicit earned-tick provenance.
    @throws ValueError On stale input, failed/overdue native review or artificial progress.
    """
    phase="before-first" if case=="recover" else case
    index=selectors.art.read_json(INDEX);qualification=ROOT/QUALIFICATIONS[phase]
    reference=selectors.art.source.identity(qualification)
    if reference not in index["reports"]:
        raise ValueError("passive source qualification differs from original accepted custody index")
    report=selectors.art.read_json(qualification);source=report["complete"]
    # Both accepted membership and exact saved bytes are required: an unrelated
    # or replaced save cannot establish previously paid shell/packet custody.
    if source not in index["artifacts"] or selectors.art.source.identity(ROOT/source["path"])!=source:
        raise ValueError("passive source saved bytes differ from original paid custody qualification")
    _,protected=selectors.boundary();application=ROOT/"build"/configuration/"openttd_space_art_test.exe";binary=selectors.art.source.identity(application)
    context=create_run_context("space","launch-pod-review-"+case,run_id="pod-review-"+str(time.time_ns()),root=ROOT,profile="launch-fleet-art",process=configuration.lower())
    context.prepare_runtime();(context.runtime_dir/"scripts/game_start.scr").write_text("launch_pod_review\n",encoding="utf-8")
    env=context.environment();env["OTTD_POD_REVIEW_SOURCE"]=str(ROOT/source["path"]);env["OTTD_POD_REVIEW_MODE"]=case
    terminal=ROOT/("logs/launch-pod-review-"+configuration+"-"+case+".log")
    with terminal.open("w",encoding="utf-8") as output:
        result=subprocess.run([str(application),"[.launch-pod-review]","--reporter","console","--durations","yes"],cwd=ROOT/"build",env=env,stdout=output,stderr=subprocess.STDOUT,timeout=60,check=False,creationflags=subprocess.CREATE_NO_WINDOW)
    text=terminal.read_text(encoding="utf-8",errors="replace");passed=re.search(r"All tests passed \(([\d,]+) assertions in 1 test case\)",text)
    # A complete real native case is required. The final ready state is live;
    # successful atomic recovery must consume that exact shell instead of art
    # leaving a permanent recovered marker or substituting another identity.
    if result.returncode or passed is None:
        raise ValueError("actual passive pod review failed; inspect "+str(terminal))
    observation=context.reports/"native-pod-review.json";native=selectors.art.read_json(observation)
    if native["source_pose_grants"] or native["capture_only_ticks"]:
        raise ValueError("passive presentation assigned original source state or work")
    if case=="recover":
        recovery=native["recovery"]
        # Only original native one-tick settlement can retire this exact selected
        # shell; invented progress or a retained stale GUI identity fails review.
        if not recovery or recovery["earned_ticks"]!=1 or not recovery["deleted_exact_shell"] or not recovery["stale_selection_empty"] or recovery["assigned_work_or_cargo"]:
            raise ValueError("passive recovery was not the actual original one-tick settlement")
    for image in context.images.glob("*.ppm"):
        with Image.open(image) as pixels:
            pixels.save(image.with_suffix(".png"))
    # Original saved bytes/native authorities and executing application remain
    # exact; native observation is independent of any prior component cache.
    if selectors.boundary()[1]!=protected or selectors.art.source.identity(application)!=binary:
        raise ValueError("passive review sources/application changed during execution")
    names=("src/tests/launch_pod_review.cpp","src/tests/multilayer/launch_pod_review_checks-1.py","src/launch_assembly_gui.h","src/launch_assembly_gui.cpp","src/asteroid_gui.cpp","src/space_sprite.cpp","CMakeLists.txt","src/tests/CMakeLists.txt")
    proof=dict(_documentation=__doc__,complete=True,configuration=configuration,case=case,source=source,qualification=reference,index=selectors.art.source.identity(INDEX),
               application=binary,terminal=selectors.art.source.identity(terminal),observation=selectors.art.source.identity(observation),assertions=int(passed.group(1).replace(",","")),cases=1,
               sources=[selectors.art.source.identity(ROOT/name) for name in names],images=[selectors.art.source.identity(path) for path in sorted(context.images.iterdir())],
               source_pose_grants=False,visual_review_pending=True)
    OUTPUT.mkdir(parents=True,exist_ok=True);(OUTPUT/(configuration+"-"+case+".json")).write_text(json.dumps(proof,indent=2)+"\n",encoding="utf-8")
    print(configuration,case,"actual passive assertions",proof["assertions"],"images",context.images.relative_to(ROOT),flush=True)
    return proof


def main():
    """Run one bounded actual passive shell snapshot/recovery case."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("Debug","RelWithDebInfo"),required=True)
    parser.add_argument("--case",choices=tuple(QUALIFICATIONS)+("recover",),required=True)
    args=parser.parse_args();run(args.configuration,args.case)


if __name__=="__main__":
    main()
