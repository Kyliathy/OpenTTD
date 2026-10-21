"""Verify and review the bounded ordinary paid launch snapshot corpus.

@file launch_fleet_paid_review_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JD.1.
Run capture bursts through checks-1.py with at most three phases per invocation.
This separate read-only verifier binds original earned provenance, actual native
raster/GUI artifacts and independent frozen source phase policy. Review flags
record completed inspection of those images; they never grant game progress.
"""
import argparse
import json
from pathlib import Path
import shutil
from PIL import Image, ImageDraw
from support.artifact_paths import create_run_context
import launch_facility_selector_checks as selectors
from tools import cargo_rocket_states as states

ROOT = selectors.ROOT
OUTPUT = ROOT/"logs/proofs/mlm/native-art/space/launch-fleet-paid-review-v1"
PHASES=("preparing","ascent","delivery","return","turnaround","complete")


def checked(reference):
    """Require the exact original artifact identity behind one observation.

    @param reference Explicit repository-relative native/source/proof path and SHA256.
    @return Same verified reference, without deriving any expected result.
    @throws ValueError On missing, changed or substituted evidence.
    """
    if selectors.art.source.identity(ROOT/reference["path"]) != reference:
        raise ValueError("stale ordinary paid review evidence: "+reference["path"])
    return reference


def verify(configuration):
    """Verify all original phase pairs and the actual paid occupied-pad queue.

    @param configuration Debug or RelWithDebInfo captured by the current isolated native app.
    @return Ordered native proofs/observations and independent source comparisons.
    @throws ValueError On incomplete scenes, stale identities, mutated owners or wrong source phases.
    """
    policy,_=selectors.boundary()
    ledger=selectors.art.read_json(ROOT/"logs/phase-ledger-verification.json")
    cases=[phase+"-"+boundary for phase in PHASES for boundary in ("before","after")]+["queue"]
    results=[];comparisons=0
    # Every before/after snapshot remains a distinct qualified paid world.
    # Exact native applications, source files and images bind the whole corpus.
    for case in cases:
        path=OUTPUT/(configuration+"-"+case+".json");proof=selectors.art.read_json(path)
        for reference in [proof["application"],proof["source"],proof["qualification"],proof["ledger"],proof["terminal"],proof["observation"],*proof["sources"],*proof["images"]]:
            checked(reference)
        if proof["source"] not in ledger["artifacts"] or proof["qualification"] not in ledger["reports"] or not proof["complete"]:
            raise ValueError("snapshot lacks original earned paid qualification")
        observation=selectors.art.read_json(ROOT/proof["observation"]["path"])
        # Native snapshot observation is pure even when the separate queue
        # preparation paid for and earned its own earlier148work opportunities.
        if observation["component_pose_inputs"] or observation["simulation_ticks"] or observation["gameplay_commands"] or observation["owner_digests_before"] != observation["owner_digests_after"]:
            raise ValueError("ordinary paid presentation changed saved owners")
        for craft in observation["crafts"]:
            if craft["model"] != 14:
                continue
            flight=craft.get("flight")
            selected=states.source_pose(policy["phases"][flight["phase"]] if flight else None,
                policy["blocks"][flight["blocked"]] if flight else "None",flight["outbound_burned"] if flight else False,
                flight["recovery_burned"] if flight else False,flight["remaining"] if flight else 0,policy)
            native=craft["source"]
            # Earned native phases must still select their original authored
            # state/frame; the current painter cannot redefine that source policy.
            if selectors.art.STATES[native["state"]] != selected["state"] or native["frame"] != selected["frame"]:
                raise ValueError("actual paid rocket differs from original frozen source policy")
            comparisons+=1
        results.append((case,path,proof,observation))
    return results,comparisons


def contacts(configuration, results):
    """Make review sheets entirely from actual native world/window captures.

    @param configuration Native build owning these exact observed images.
    @param results Complete verified native paid corpus, without latest-file discovery.
    @return Review invocation and its actual native contact images.
    """
    context=create_run_context("space","launch-fleet-paid-contact",root=ROOT,profile="launch-fleet-art",process=configuration.lower())
    sheet=Image.new("RGB",(1280,744),"#29323c");draw=ImageDraw.Draw(sheet)
    # Compare genuine phases and layer transitions while keeping source pixels
    # unchanged; only a fixed camera crop and descriptive labels form the sheet.
    for index,(case,path,proof,observation) in enumerate(results[:12]):
        craft=next(row for row in observation["crafts"] if row["model"]==14)
        image_path=Path(proof["observation"]["path"]).parent.parent/"images"/Path(craft["image"]).with_suffix(".png")
        with Image.open(ROOT/image_path) as image:
            sheet.paste(image.crop((160,68,480,292)),(index%4*320,index//4*248+24))
        draw.text((index%4*320+5,index//4*248+5),case+" / layer "+str(craft["layer"]),fill="white")
    world=context.images/(configuration+"-paid-phases.png");sheet.save(world)
    ui=Image.new("RGB",(2013,600),"#29323c");draw=ImageDraw.Draw(ui)
    selected={case:(proof,observation) for case,path,proof,observation in results}
    # Fuel/phase/carried-payload and occupied-pad reasons must be visible in
    # the actual native workshop widget, independently of machine observations.
    for index,case in enumerate(("preparing-before","complete-after","queue")):
        proof,observation=selected[case];center=observation["centers"][0]
        name="center-"+str(center["station"])+"-selected-stock.png"
        image=Path(proof["observation"]["path"]).parent.parent/"images"/name
        with Image.open(ROOT/image) as pixels:
            ui.paste(pixels,(index*671,24))
        draw.text((index*671+5,5),case+" / actual native inventory and status",fill="white")
    windows=context.images/(configuration+"-paid-disclosures.png");ui.save(windows)
    return context,[world,windows]


def review(configuration, reviewed=False):
    """Bind complete native evidence and optionally record completed visual inspection.

    @param configuration Explicit Debug or RelWithDebInfo original capture set.
    @param reviewed Record the caller's inspection of actual native contact/window images.
    @return Native/capture review receipt, retaining all source and paid owners.
    """
    results,comparisons=verify(configuration);context,images=contacts(configuration,results)
    application=results[0][2]["application"]
    retained=OUTPUT/("qualified-paid-review-"+configuration+".exe")
    # Preserve the exact actual executable before a later focused review target
    # adds tests; never replace a different previously accepted native binary.
    if retained.exists():
        if selectors.art.source.identity(retained)["sha256"] != application["sha256"]:
            raise ValueError("refusing to replace earlier qualified paid-review application")
    else:
        shutil.copyfile(ROOT/application["path"],retained)
    receipt=dict(_documentation=__doc__,complete=True,configuration=configuration,visual_review_complete=reviewed,
                 ordinary_paid_world_loaded=True,component_pose_inputs=False,cases=len(results),
                 assertions=sum(proof["assertions"] for case,path,proof,observation in results),source_policy_comparisons=comparisons,
                 proofs=[selectors.art.source.identity(path) for case,path,proof,observation in results],
                 review_images=[selectors.art.source.identity(image) for image in images],
                 retained_application=selectors.art.source.identity(retained),reviewer=selectors.art.source.identity(Path(__file__)),
                 capture_only_ticks=0,paid_queue_ticks=148,paid_queue_capital=400000,paid_queue_granted_resources=False)
    destination=OUTPUT/("review-"+configuration+".json");destination.write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
    print(configuration,"verified paid cases",len(results),"assertions",receipt["assertions"],"source-policy comparisons",comparisons,flush=True)
    for image in images:
        print(image.relative_to(ROOT),flush=True)
    return receipt


def finalize():
    """Bind both inspected snapshot/queue corpora and exact native counterpart pixels.

    @return Complete62JD physical viewport/queue receipt, retaining later UI/pod gates.
    @throws ValueError On missing review, stale native artifacts or different counterpart pixels.
    """
    reviews=[];corpora=[]
    # Both real compilers must preserve the same physical owners and every
    # native viewport/window byte; one reviewed build cannot stand for another.
    for configuration in ("Debug","RelWithDebInfo"):
        path=OUTPUT/("review-"+configuration+".json");review=selectors.art.read_json(path)
        if not review["visual_review_complete"] or review["cases"] != 13 or review["assertions"] != 1222:
            raise ValueError("both current physical paid corpora require completed visual review")
        checked(review["retained_application"]);checked(review["reviewer"])
        for image in review["review_images"]:
            checked(image)
        records=[]
        for reference in review["proofs"]:
            checked(reference);proof=selectors.art.read_json(ROOT/reference["path"])
            for image in proof["images"]:
                checked(image)
            records.append(proof)
        reviews.append(selectors.art.source.identity(path));corpora.append(records)
    pairs=0;indexed=0
    for left,right in zip(corpora[0],corpora[1],strict=True):
        keys=("phase","boundary","scenario","source")
        if any(left[key] != right[key] for key in keys):
            raise ValueError("counterpart cases differ in their original paid input")
        images=[]
        for proof in (left,right):
            images.append({Path(row["path"]).name:row["sha256"] for row in proof["images"] if Path(row["path"]).suffix in (".ppm",".indexed")})
        if images[0] != images[1]:
            raise ValueError("native counterpart viewport/window pixels differ")
        pairs+=sum(name.endswith(".ppm") for name in images[0]);indexed+=sum(name.endswith(".indexed") for name in images[0])
    selectors.boundary()
    receipt=dict(_documentation=__doc__,complete=True,group="62JD",cases_per_configuration=13,assertions_per_configuration=1222,
                 source_policy_comparisons_per_configuration=14,actual_counterpart_native_ppm=pairs,actual_counterpart_indexed=indexed,
                 component_pose_inputs=False,original_snapshot_pairs=6,real_paid_queue_work_ticks=148,real_paid_queue_capital=400000,
                 source_grants=False,visual_review_complete=True,reviews=reviews,
                 remaining_scope=["62JDB actual picked-hull window dispatch/disclosures","62JDC paid probe and passive-pod presentation","62JDD conditional Plan9task16.2 closure"])
    destination=OUTPUT/"viewport-queue-verification.json";destination.write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
    print("qualified62JD",pairs,"exact native image counterparts /",indexed,"indexed counterparts; actual inspector and pod gates remain open",flush=True)
    return receipt


def main():
    """Verify explicit native evidence and build repeatable review contact sheets."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("Debug","RelWithDebInfo"))
    parser.add_argument("--reviewed",action="store_true")
    parser.add_argument("--finalize",action="store_true")
    args=parser.parse_args()
    if args.finalize:
        finalize()
    elif args.configuration:
        review(args.configuration,args.reviewed)
    else:
        parser.error("select --configuration or --finalize")


if __name__ == "__main__":
    main()
