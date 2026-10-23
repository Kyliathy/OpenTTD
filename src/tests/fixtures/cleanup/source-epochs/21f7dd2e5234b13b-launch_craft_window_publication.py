"""Bind actual paid windows and publish only two own ordinary-game runtime records.

@file launch_craft_window_publication.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDB.
The earlier physical62JD/native62JC checkpoints stay immutable. Actual current
public window selections, source compatibility and native counterpart pixels
qualify only the facility/model14 records; probe/pod acceptance remains separate.
"""
import argparse
import copy
import json
from pathlib import Path
import shutil
import launch_facility_selector_checks as selectors

ROOT=selectors.ROOT
OUTPUT=ROOT/"logs/proofs/mlm/native-art/space/launch-craft-window-v1"
PUBLIC=ROOT/"media/multilayer/contracts/multilayer_sprite_manifest.json"
ASSETS=("facility.launch-center","vehicle.model-14")


def checked(reference):
    """Require the actual executed source/artifact identity.

    @param reference Explicit repository path and original SHA256.
    @return Same verified evidence reference.
    @throws ValueError On a changed or missing native/source/proof artifact.
    """
    if selectors.art.source.identity(ROOT/reference["path"])!=reference:
        raise ValueError("stale actual-window evidence: "+reference["path"])
    return reference


def verify():
    """Verify current typed selections, immutable physical evidence and exact counterparts.

    @return Complete native evidence, applications and preserved preceding publication.
    @throws ValueError On partial execution, wrong selection, source drift or changed native pixels.
    """
    selectors.boundary()
    physical_path=ROOT/"logs/proofs/mlm/native-art/space/launch-fleet-paid-review-v1/viewport-queue-verification.json"
    physical=selectors.art.read_json(physical_path)
    for reference in physical["reviews"]:
        checked(reference);review=selectors.art.read_json(ROOT/reference["path"]);checked(review["retained_application"])
        for image in review["review_images"]:
            checked(image)
        for original in review["proofs"]:
            checked(original)
    evidence=[selectors.art.source.identity(physical_path)];applications=[];corpora=[]
    # Each compiler must run all three actual native contexts. Archived earlier
    # physics/viewport qualification never substitutes for current window dispatch.
    for configuration in ("Debug","RelWithDebInfo"):
        records=[];assertions=0
        for case in ("surface","orbit","queue"):
            path=OUTPUT/(configuration+"-"+case+".json");proof=selectors.art.read_json(path)
            if not proof["complete"] or proof["cases"]!=1 or not proof["actual_generic_dispatch"]:
                raise ValueError("incomplete current public window dispatch")
            for reference in [proof["application"],proof["source"],proof["qualification"],proof["terminal"],proof["observation"],*proof["sources"],*proof["images"]]:
                checked(reference)
            observation=selectors.art.read_json(ROOT/proof["observation"]["path"])
            if observation["capture_ticks"] or observation["component_pose_inputs"] or observation["owner_digests_before"]!=observation["owner_digests_after"]:
                raise ValueError("actual window changed paid gameplay owners")
            for selection in observation["selections"]:
                if (selection["vehicle"],selection["generation"])!=(selection["selected_vehicle"],selection["selected_generation"]):
                    raise ValueError("actual native inspector selected a different hull")
            evidence.append(selectors.art.source.identity(path));assertions+=proof["assertions"];records.append(proof)
        if assertions!=612:
            raise ValueError("current native window cases are incomplete")
        application=records[0]["application"];retained=OUTPUT/("qualified-window-review-"+configuration+".exe")
        if retained.exists():
            if selectors.art.source.identity(retained)["sha256"]!=application["sha256"]:
                raise ValueError("refusing to replace an earlier qualified actual-window application")
        else:
            shutil.copyfile(ROOT/application["path"],retained)
        applications.append(dict(observed=application,retained=selectors.art.source.identity(retained)));corpora.append(records)
    # Actual native window pixels, including the unchanged stale-refused views,
    # transfer physical inspection to the other compiler only after exact equality.
    pairs=0
    for left,right in zip(corpora[0],corpora[1],strict=True):
        frames=[]
        for proof in (left,right):
            frames.append({Path(row["path"]).name:row["sha256"] for row in proof["images"] if Path(row["path"]).suffix==".ppm"})
        if frames[0]!=frames[1]:
            raise ValueError("actual native window counterparts differ")
        pairs+=len(frames[0])
    preceding_path=ROOT/"logs/proofs/mlm/native-art/space/launch-fleet-runtime-v1/publication-verification.json"
    preceding=selectors.art.read_json(preceding_path);checked(preceding["runtime_manifest"])
    baseline=OUTPUT/"public-before-window-review.json"
    # Only the exact prior qualified public bytes can supply an expectation;
    # never seed it from already changed ordinary-game metadata.
    if not baseline.exists():
        if selectors.art.source.identity(PUBLIC)!=preceding["public_manifest"]:
            raise ValueError("preceding native-only public baseline is unavailable")
        baseline.write_bytes(PUBLIC.read_bytes())
    import hashlib
    if hashlib.sha256(baseline.read_bytes()).hexdigest()!=preceding["public_manifest"]["sha256"]:
        raise ValueError("preceding native public baseline changed")
    return evidence,applications,pairs,selectors.art.read_json(baseline),preceding


def publish(write=False,reviewed=False):
    """Prepare or publish qualified actual ordinary-game facility/rocket metadata.

    @param write Publish the exact two-family update after actual verification.
    @param reviewed Record completed inspection of the native selected-hull windows.
    @return Scoped current qualification/publication receipt, leaving probe/pod and16.2 open.
    @throws ValueError On unreviewed publication, wrong metadata scope or stale evidence.
    """
    if write and not reviewed:
        raise ValueError("inspect actual native windows before ordinary-game publication")
    evidence,applications,pairs,original,preceding=verify()
    runtime=copy.deepcopy(selectors.art.read_json(ROOT/preceding["runtime_manifest"]["path"]))
    runtime.update(_documentation=__doc__,ordinary_game_reviewed=reviewed,evidence=evidence,applications=applications,
                   actual_generic_window_dispatch=True,source_pose_grants=False)
    for binding in runtime["bindings"]:
        binding["gallery_status"]="ordinary_paid_native_display_and_exact_inspector_verified" if reviewed else "actual_windows_await_visual_inspection"
    destination=OUTPUT/"runtime-manifest.json";destination.write_text(json.dumps(runtime,indent=2)+"\n",encoding="utf-8")
    expected=copy.deepcopy(original)
    # Native pixels, original source/packed identities and all other families
    # stay exact. Only these two own ordinary-game runtime records advance.
    for row in expected["assets"]:
        if row["asset_id"] not in ASSETS:
            continue
        row["authoring"]["status"]="source_packed_runtime_verified"
        row["runtime"].update(status="bound_verified",ordinary_game_reviewed=True,binding_manifest=destination.relative_to(ROOT).as_posix())
    current=selectors.art.read_json(PUBLIC)
    if current!=original and current!=expected:
        raise ValueError("public metadata differs beyond the two qualified ordinary runtime records")
    if write:
        PUBLIC.write_text(json.dumps(expected,indent=2)+"\n",encoding="utf-8")
        if selectors.art.read_json(PUBLIC)!=expected:
            raise ValueError("ordinary native metadata readback differs")
    receipt=dict(_documentation=__doc__,complete=True,published=write,visual_review_complete=reviewed,native_cases_per_configuration=3,
                 native_assertions_per_configuration=612,actual_ppm_counterparts=pairs,actual_generic_dispatch=True,
                 paid_source_grants=False,remaining_scope=["62JDC paid probe/pod presentation","62JDD conditional Plan9task16.2 closure"],
                 public_manifest=selectors.art.source.identity(PUBLIC),public_before=selectors.art.source.identity(OUTPUT/"public-before-window-review.json"),
                 runtime_manifest=selectors.art.source.identity(destination),publisher=selectors.art.source.identity(Path(__file__)),
                 protected_public_rows=len(original["assets"])-2,evidence=evidence,applications=applications)
    target=OUTPUT/("publication-verification.json" if write else "publication-preview.json");target.write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
    print("qualified actual native craft windows:",612,"assertions/configuration;",pairs,"exact counterpart frames; published",write,flush=True)
    return receipt


def main():
    """Verify current exact native window evidence and publish the explicit reviewed scope."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--publish",action="store_true");parser.add_argument("--reviewed",action="store_true")
    args=parser.parse_args();publish(args.publish,args.reviewed)


if __name__=="__main__":
    main()
