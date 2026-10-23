"""Bind actual paid windows and publish only two own ordinary-game runtime records.

@file launch_craft_window_publication.py
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group28 retired cache publication.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDB.
The earlier physical62JD/native62JC checkpoints stay immutable. Actual current
public window selections, source compatibility and native counterpart pixels
qualify only the facility/model14 records; probe/pod acceptance remains separate.
"""
# Refuse this completed historical entry before importing any old cache readers.
# Current recipes preserve its original facts without rerunning source publication.
if __name__ == "__main__":
    raise SystemExit("Historical craft-window publication is retired; current art recovery grants no paid replay or old EXE-cache dependency. Use artifact_rebuild.py launch-occupied-viewports --configuration RelWithDebInfo for current evidence.")

import argparse
import copy
import json
from pathlib import Path
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
    """Refuse the completed historical publication and its obsolete cache graph.

    @return None; this retired entry always raises before reading or writing artifacts.
    @throws ValueError Current evidence requires the registered recipe and live source plan.
    """
    raise ValueError("Historical craft-window publication is retired; current art recovery grants no paid replay or old EXE-cache dependency.")


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
