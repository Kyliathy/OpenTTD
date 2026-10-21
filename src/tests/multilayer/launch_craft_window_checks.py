"""Qualify actual native picked-hull inspector dispatch without changing paid owners.

@file launch_craft_window_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDB.
The preceding62JDcorpus stays historical and immutable. New UI evidence uses its
own native cases, source compatibility adapter, logs and actual window captures.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import time
from PIL import Image
from support.artifact_paths import create_run_context
import launch_facility_selector_checks as selectors

ROOT=selectors.ROOT
OUTPUT=ROOT/"logs/proofs/mlm/native-art/space/launch-craft-window-v1"
SPEC=importlib.util.spec_from_file_location("paid_launch_inputs",Path(__file__).with_name("launch_fleet_paid_review_checks-1.py"))
paid_inputs=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(paid_inputs)
CASES={"surface":("preparing","before"),"orbit":("return","before"),"queue":("preparing","before")}


def original_queue_scope():
    """Prove shared scenario extraction changed only its test linkage/include.

    @return Exact original closed queue source identity and new shared public test interface.
    @throws ValueError On any change to the actual paid scenario/controller/work body.
    """
    closed=selectors.art.read_json(ROOT/"logs/proofs/mlm/native-art/space/launch-fleet-paid-review-v1/Debug-queue.json")
    reference=next(row for row in closed["sources"] if row["path"]=="src/tests/launch_fleet_paid_review.cpp")
    data=(ROOT/reference["path"]).read_bytes()
    newline=b"\r\n" if b"\r\n" in data else b"\n"
    include=b'#include "launch_fleet_paid_review_fixture.h"'+newline
    signature=b"nlohmann::json EarnPaidLaunchQueue("
    # Reconstruct exact preceding source bytes rather than rebasing its SHA.
    # The original native paid transaction and normal work body remain untouched.
    if data.count(include)!=1 or data.count(signature)!=1:
        raise ValueError("shared paid scenario linkage differs beyond its declared interface")
    original=data.replace(include,b"").replace(signature,b"static "+signature)
    if hashlib.sha256(original).hexdigest()!=reference["sha256"]:
        raise ValueError("original paid queue scenario body changed")
    return dict(original=reference,shared_header=selectors.art.source.identity(ROOT/"src/tests/launch_fleet_paid_review_fixture.h"))


def run(configuration,case):
    """Execute real generic map/list window dispatch in a current loaded paid world.

    @param configuration Debug or RelWithDebInfo isolated actual-engine application.
    @param case Real Surface flight, Space flight/deployed probe, or actual paid queued rocket.
    @return Native selected-window/purity receipt with actual portable UI images.
    @throws ValueError On changed source owners, wrong selections, stale refusal or failed/overdue native execution.
    """
    phase,boundary=CASES[case];source,qualification=paid_inputs.qualified_source(phase,boundary)
    _,before=selectors.boundary();scenario=original_queue_scope()
    app=ROOT/"build"/configuration/"openttd_space_art_test.exe";application=selectors.art.source.identity(app)
    context=create_run_context("space","launch-craft-window-"+case,run_id="craft-window-"+str(time.time_ns()),root=ROOT,profile="launch-fleet-art",process=configuration.lower())
    context.prepare_runtime();(context.runtime_dir/"scripts/game_start.scr").write_text("launch_craft_window\n",encoding="utf-8")
    env=context.environment();env["OTTD_CRAFT_WINDOW_SOURCE"]=str(ROOT/source["path"]);env["OTTD_CRAFT_WINDOW_MODE"]=case
    terminal=ROOT/("logs/launch-craft-window-"+configuration+"-"+case+".log")
    with terminal.open("w",encoding="utf-8") as output:
        result=subprocess.run([str(app),"[.launch-craft-window]","--reporter","console","--durations","yes"],cwd=ROOT/"build",env=env,stdout=output,stderr=subprocess.STDOUT,timeout=60,check=False,creationflags=subprocess.CREATE_NO_WINDOW)
    text=terminal.read_text(encoding="utf-8",errors="replace");passed=re.search(r"All tests passed \(([\d,]+) assertions in 1 test case\)",text)
    # Native success must include the actual public dispatcher and resolved
    # window selections, rather than merely rechecking lower-level map picking.
    if result.returncode or passed is None:
        raise ValueError("native exact-window dispatch failed; inspect "+str(terminal))
    observation=context.reports/"native-craft-window.json";document=selectors.art.read_json(observation)
    if not document["actual_generic_dispatch"] or document["component_pose_inputs"] or document["capture_ticks"] or document["owner_digests_before"]!=document["owner_digests_after"]:
        raise ValueError("window dispatch changed paid native owners")
    for row in document["selections"]:
        if (row["vehicle"],row["generation"])!=(row["selected_vehicle"],row["selected_generation"]):
            raise ValueError("native inspector lost the actually selected incarnation")
        image=context.images/row["image"];stale=image.with_name(image.stem+"-stale-refused.ppm")
        if image.read_bytes()!=stale.read_bytes():
            raise ValueError("stale reference changed the actual current window pixels")
    for image in context.images.glob("*.ppm"):
        with Image.open(image) as pixels:
            pixels.save(image.with_suffix(".png"))
    # Both immutable source/native authorities and the exact executing binary
    # must remain current throughout the ordinary window observation.
    if selectors.boundary()[1]!=before or selectors.art.source.identity(app)!=application or original_queue_scope()!=scenario:
        raise ValueError("window qualification inputs changed during native execution")
    names=("src/launch_assembly_gui.h","src/launch_assembly_gui.cpp","src/space_survey_gui.h","src/asteroid_gui.cpp","src/vehicle_gui.cpp",
           "src/tests/launch_craft_window.cpp","src/tests/launch_fleet_paid_review_fixture.h","src/tests/multilayer/launch_craft_window_checks.py","src/CMakeLists.txt","src/tests/CMakeLists.txt","CMakeLists.txt")
    proof=dict(_documentation=__doc__,complete=True,configuration=configuration,case=case,source=source,qualification=qualification,
               application=application,assertions=int(passed.group(1).replace(",","")),cases=1,terminal=selectors.art.source.identity(terminal),
               observation=selectors.art.source.identity(observation),actual_generic_dispatch=True,shared_original_queue=scenario,
               sources=[selectors.art.source.identity(ROOT/name) for name in names],images=[selectors.art.source.identity(path) for path in sorted(context.images.iterdir())],visual_review_pending=True)
    OUTPUT.mkdir(parents=True,exist_ok=True);(OUTPUT/(configuration+"-"+case+".json")).write_text(json.dumps(proof,indent=2)+"\n",encoding="utf-8")
    print(configuration,case,"actual inspector assertions",proof["assertions"],"images",context.images.relative_to(ROOT),flush=True)
    return proof


def main():
    """Execute a single bounded native window case in the chosen configuration."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("Debug","RelWithDebInfo"),required=True)
    parser.add_argument("--case",choices=tuple(CASES),required=True)
    args=parser.parse_args();run(args.configuration,args.case)


if __name__ == "__main__":
    main()
