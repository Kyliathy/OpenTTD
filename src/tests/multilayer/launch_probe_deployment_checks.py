"""Qualify existing real paid carried-to-orbital probe presentation and exact identity.

@file launch_probe_deployment_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDC.1.
Reuse the actual already-executed paid phase pairs and current typed orbital
inspector. No source fixture, legacy receipt, gameplay world or native field is
rewritten; archived applications own the preceding physical capture epochs.
"""
import json
from pathlib import Path
import launch_facility_selector_checks as selectors

ROOT=selectors.ROOT
PHYSICAL=ROOT/"logs/proofs/mlm/native-art/space/launch-fleet-paid-review-v1"
WINDOWS=ROOT/"logs/proofs/mlm/native-art/space/launch-craft-window-v1"
OUTPUT=ROOT/"logs/proofs/mlm/native-art/space/launch-probe-deployment-v1"


def checked(reference):
    """Require original actual evidence bytes without reclassifying an old app as current.

    @param reference Repository-relative retained source/application/report/image identity.
    @return Same immutable reference after exact byte verification.
    @throws ValueError On a missing or changed accepted artifact.
    """
    if selectors.art.source.identity(ROOT/reference["path"])!=reference:
        raise ValueError("paid deployment evidence changed: "+reference["path"])
    return reference


def qualify():
    """Bind actual paid native deployment, physical layer picking and exact orbital context.

    @return Complete focused two-configuration qualification with no earned/synthetic progress.
    @throws ValueError On wrong custody, changed identity, duplicate deployment or stale evidence.
    """
    selectors.boundary();ledger=selectors.art.read_json(ROOT/"logs/phase-ledger-verification.json")
    current=selectors.art.read_json(WINDOWS/"publication-verification.json");checked(current["runtime_manifest"])
    physical_path=PHYSICAL/"viewport-queue-verification.json"
    physical=selectors.art.read_json(physical_path)
    if selectors.art.source.identity(physical_path) not in current["evidence"]:
        raise ValueError("current UI qualification does not bind the original physical corpus")
    results=[]
    # The original before/after pair earns the actual transition once. Retained
    # native archives preserve its real renderer epoch despite later UI changes.
    for configuration in ("Debug","RelWithDebInfo"):
        review_path=PHYSICAL/("review-"+configuration+".json")
        if selectors.art.source.identity(review_path) not in physical["reviews"]:
            raise ValueError("physical review differs from its accepted aggregate")
        physical_review=selectors.art.read_json(review_path);checked(physical_review["retained_application"])
        observations=[];references=[]
        for boundary in ("before","after"):
            path=PHYSICAL/(configuration+"-delivery-"+boundary+".json")
            if selectors.art.source.identity(path) not in physical_review["proofs"]:
                raise ValueError("deployment pair differs from its accepted physical review")
            proof=selectors.art.read_json(path)
            if proof["application"]["sha256"]!=physical_review["retained_application"]["sha256"]:
                raise ValueError("physical deployment observation lacks its exact qualified native archive")
            for reference in [proof["source"],proof["qualification"],proof["terminal"],proof["observation"],*proof["images"]]:
                checked(reference)
            # Accepted saved bytes and their original native paid qualification
            # are both required; a presentational pose cannot earn deployment.
            if proof["source"] not in ledger["artifacts"] or proof["qualification"] not in ledger["reports"]:
                raise ValueError("deployment input lacks original paid qualification")
            observations.append(selectors.art.read_json(ROOT/proof["observation"]["path"]));references.append(selectors.art.source.identity(path))
        parent=[];probe=[]
        for observation in observations:
            parent.append(next(row for row in observation["crafts"] if row["model"]==14))
            probe.append(next(row for row in observation["crafts"] if row["model"]==15))
        before,after=probe
        expected_before=(4,2,True,False);expected_after=(0,2,False,True)
        fields=("placement","layer","native_hidden","correct_layer_picked")
        # The same paid child changes actual Carried custody to actual Orbital,
        # with the native Hidden/layer/picker contracts and no replacement hull.
        if (before["vehicle"],before["generation"])!=(after["vehicle"],after["generation"]) or tuple(before[key] for key in fields)!=expected_before or tuple(after[key] for key in fields)!=expected_after:
            raise ValueError("paid probe deployment lost original custody/identity/layer")
        if len(observations[0]["crafts"])!=len(observations[1]["crafts"]):
            raise ValueError("probe deployment created a duplicate native hull")
        if parent[0]["flight"]["deployed"] or not parent[1]["flight"]["deployed"] or parent[0]["flight"]["missions_started"]!=parent[1]["flight"]["missions_started"]:
            raise ValueError("deployment changed original once-only flight receipt")
        window_path=WINDOWS/(configuration+"-orbit.json")
        if selectors.art.source.identity(window_path) not in current["evidence"]:
            raise ValueError("probe window differs from current accepted exact-selection qualification")
        window=selectors.art.read_json(window_path)
        checked(window["observation"]);checked(window["terminal"])
        for image in window["images"]:
            checked(image)
        native=selectors.art.read_json(ROOT/window["observation"]["path"])
        selection=next(row for row in native["selections"] if row["model"]==15)
        if not selection["orbital_inspector"] or (selection["selected_vehicle"],selection["selected_generation"])!=(after["vehicle"],after["generation"]):
            raise ValueError("deployed probe inspector lost exact native incarnation")
        # Its current actual binary is retained by the reviewed window stage;
        # the physical epoch remains a different explicitly archived executable.
        application=next(row for row in current["applications"] if row["observed"]["path"]==window["application"]["path"])
        checked(application["retained"])
        if application["retained"]["sha256"]!=window["application"]["sha256"]:
            raise ValueError("probe window application differs from its actual retained binary")
        results.append(dict(configuration=configuration,probe=before["vehicle"],generation=before["generation"],
                            before=dict(zip(fields,expected_before)),after=dict(zip(fields,expected_after)),
                            original_paid_pairs=references,current_native_window=selectors.art.source.identity(window_path),
                            physical_application=physical_review["retained_application"],inspector_application=application["retained"],
                            actual_typed_orbital_selection=selection))
    receipt=dict(_documentation=__doc__,complete=True,group="62JDC.1",actual_paid_deployment=True,duplicate_hulls=False,
                 source_pose_grants=False,new_simulation_ticks=0,configurations=results,
                 preceding_physical=selectors.art.source.identity(PHYSICAL/"viewport-queue-verification.json"),
                 preceding_windows=selectors.art.source.identity(WINDOWS/"publication-verification.json"),
                 producer=selectors.art.source.identity(Path(__file__)),remaining_scope="62JDC.2 actual passive descent/recovery;62JDDconditional16.2closure")
    OUTPUT.mkdir(parents=True,exist_ok=True);path=OUTPUT/"verification.json";path.write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
    print("actual paid probe16/generation2 deployment and typed orbital selection qualify in both configurations",flush=True)
    return receipt


if __name__=="__main__":
    qualify()
