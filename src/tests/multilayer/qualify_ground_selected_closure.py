"""Close the selected Ground domain from explicitly selected current witnesses.

@file qualify_ground_selected_closure.py Historical image hashes remain facts only.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group20BC.
@see docs/zz-reach2/architecture/archi-historical-debug.md.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHD.
"""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import struct

from ground_viewport_review import ROOT, SOURCE_ROLES, checked_result
from ground_review_sources import GroundSourceViews, WORLD_CONTRACT, handoff
from ground_source_facts import load_facts, verify_facts
from artifact_paths import contained, create_run_context
from artifact_rebuild import identity
from space_source_review import checked_result as checked_selectors

MODES = ("collector", "industry-states", "purchases", "rail", "road", "stations")


def native_domain(path):
    """Read the complete independently sourced domain exercised by the current decoder.

    @param path Hash-verified current expected native byte fixture.
    @return All 22,018 unique native offsets, retaining the original prefix gaps.
    """
    data = path.read_bytes()
    # The prefix and selected range jointly protect every original native family;
    # accepting only the new Ground range would lose pre-existing sprite owners.
    if (len(data) < 16 or data[:8] != b"GRDART1\0"
            or struct.unpack_from("<HHHH", data, 8) != (22018, 22045, 7791, 14254)):
        raise ValueError("selected current native fixture header changed")
    cursor, offsets = 16, []
    # Traverse declared palette records to preserve gaps and aliases instead of
    # guessing a contiguous range that could conceal missing original sprites.
    for _ in range(22018):
        if cursor + 14 > len(data):
            raise ValueError("selected current native fixture is truncated")
        offset, width, height, _x, _y, _digest = struct.unpack_from("<HHHhhI", data, cursor)
        cursor += 14 + width * height
        offsets.append(offset)
    # Complete coverage requires one record per unique offset, with neither
    # trailing data nor truncation allowed to conceal a missing native owner.
    if cursor != len(data) or offsets != sorted(set(offsets)):
        raise ValueError("selected current native fixture has duplicate, truncated or trailing records")
    return set(offsets)


def immutable_views(witness, mode, configuration, selector):
    """Validate an explicit current capture through its existing provenance contract.

    @param witness Exact current capture identity, never a historical image receipt.
    @param mode Original bounded capture mechanism required for this selected family.
    @param configuration Actual current native configuration of the selected captures.
    @param selector Exact current source/decoder/selector prerequisite identity.
    @return Checked current native view, without historical retained-EXE dependencies.
    """
    proof = checked_result(witness, mode, configuration, ROOT)
    if proof["source_selector"] != selector:
        raise ValueError("selected current views name another source/decoder/selector epoch")
    return proof


def coverage_rows(dispatch, families, selected, domain, views):
    """Join exact physical domains to current views, preserving deferred owner gates.

    @param dispatch Original owner dispatch; old qualification fields remain historical.
    @param families Detached current source families and their original native geometry.
    @param selected Exact fifty selected original Ground and M1 family IDs.
    @param domain Complete independently decoded native offset set.
    @param views Explicit mode-to-current-capture identities checked by their owners.
    @return Fifty disjoint family rows with twenty-four current drawing bindings.
    """
    counts = Counter(row["dispatch"] for row in dispatch["families"])
    assets = [row["asset_id"] for row in dispatch["families"]]
    # Exact identities and owner counts prevent art qualification from dropping
    # a deferred feature or promoting it into the current operating scope.
    if (counts != {"current_bound": 21, "current_existing": 3, "later_owner": 26}
            or len(assets) != 50 or set(assets) != selected):
        raise ValueError("selected current/later dispatch changed its exact fifty-family scope")
    result, allocated = [], set()
    # Each family keeps its complete native Cartesian range. Only existing
    # current owners receive view evidence; deferred owners retain their tasks.
    for row in dispatch["families"]:
        asset = row["asset_id"]
        family = families[asset]
        begin, count = family["packed"]["native_offset"], family["packed"]["sprite_count"]
        required = math.prod((len(family["states"]), family["frames"], len(family["directions"]),
                              family["tile_variants"], len(family["parts"])))
        offsets = set(range(begin, begin + count))
        # Complete source coverage may neither overlap another owner nor escape
        # the decoder domain, and the original dispatch must name that same range.
        if (count != required or not offsets <= domain or offsets & allocated
                or row["native_offset"] != begin or row["sprite_count"] != count):
            raise ValueError("selected native source domain/range changed: " + asset)
        allocated.update(offsets)
        evidence = None
        if row["dispatch"] == "later_owner":
            # Native art does not complete deferred gameplay; retain every
            # explicit source-plan task and its reason for remaining open.
            if not row.get("group") or not row.get("master") or not row.get("reason"):
                raise ValueError("selected later owner lost its unfinished gameplay dispatch")
        else:
            if asset in ("facility.rail-station", "facility.road-terminal"):
                mode = "stations"
            elif asset == "facility.vent-collector":
                mode = "collector"
            elif asset.startswith("vehicle.model-"):
                mode = "rail" if int(asset.rsplit("-", 1)[1]) <= 6 else "road"
            elif asset.startswith("purchase.model-"):
                mode = "purchases"
            # Mines and the four existing processors share the original industry
            # observer; no other facility may inherit its current drawing claim.
            elif asset.startswith("mine.") or asset in (
                    "facility.metal-refinery", "facility.chemical-works",
                    "facility.battery-works", "facility.electronics-works"):
                mode = "industry-states"
            else:
                raise ValueError("selected current family lacks an actual native viewport: " + asset)
            evidence = views[mode]
        result.append(dict(asset_id=asset, native_offset=begin, sprite_count=count,
            dispatch=row["dispatch"], owner=row["owner"], group=row.get("group"), master=row.get("master"),
            actual_view_evidence=evidence, native_decoded_and_loaded=True, operating_claimed=False))
    if len(allocated) != 15488:
        raise ValueError("selected current/later coverage omitted a required native pose")
    return result


def qualify(configuration, selector_reference, views):
    """Close one explicit source/decoder/selector/capture selection without publication.

    @param configuration Independently executed native configuration selected by the caller.
    @param selector_reference Exact completed current Ground source-selector identity.
    @param views All six explicit original current capture identities, keyed by mode.
    @return Technical closure; useful-route, operating and human gates remain open.
    """
    if set(views) != set(MODES):
        raise ValueError("selected closure requires all six explicit original native capture modes")
    selector = checked_selectors(selector_reference, "ground", configuration, ROOT)
    native_reference = selector["decoder_prerequisite"]
    native = json.loads((ROOT / native_reference["path"]).read_text(encoding="utf-8"))
    domain = native_domain(ROOT / native["expected_payload"]["path"])
    source_inputs = None
    # Each capture retains its existing frame/process contract. Join only one
    # source/build/container epoch rather than independently plausible mixed runs.
    for mode in MODES:
        proof = immutable_views(views[mode], mode, configuration, selector_reference)
        inputs = {role: proof["source_inputs"][role] for role in SOURCE_ROLES}
        if source_inputs is None:
            source_inputs = inputs
        # Shared input and payload identities bind all fifty families to one
        # selected closure, even when the cameras were captured in separate runs.
        if (inputs != source_inputs or proof["build_result"] != selector["build_result"]
                or proof["runtime_payload"]["sha256"] != native["runtime_payload"]["sha256"]):
            raise ValueError("selected current views mix source/build/native-container epochs")
    facts = load_facts()
    preservation = verify_facts()
    dispatch_ref = selector["selected_inputs"]["dispatch"]
    dispatch = json.loads((ROOT / dispatch_ref["path"]).read_text(encoding="utf-8"))
    with GroundSourceViews(ROOT, selector["run_id"], source_inputs) as sources:
        rows = coverage_rows(dispatch, sources.families, sources.allowed, domain, views)
        sources.verify_current()
    worlds = json.loads((ROOT / WORLD_CONTRACT).read_text(encoding="utf-8"))
    return dict(_documentation=__doc__, schema_version=2, kind="selected_ground_current_closure", passed=True,
        configuration=configuration, selected_families=50, selected_native_poses=15488,
        current_viewport_families=24, later_owner_families=26, coverage=rows,
        source_inputs=source_inputs, source_selector=selector_reference, native_decoder=native_reference,
        native_current_views=views, dispatch=dispatch_ref, preservation=preservation,
        public_manifest=identity(handoff.PUBLIC, ROOT),
        historical_source_audit=facts["historical_source_audit"],
        historical_view_epochs={mode: worlds["modes"][mode]["historical_origin"] for mode in MODES},
        historical_view_dependencies_required=False, master_10_4_selected_scope_complete=True,
        whole_underground_plan_complete=False, later_tube_plant_riser_lifecycle_and_human_gates="open",
        paid_progress_claimed=False, human_acceptance=False, transported_bootstrap_claimed=False,
        source=identity(Path(__file__), ROOT))


def main():
    """Publish an explicit current selected closure under a fresh cleanable review run.

    @return None; missing, mixed or stale current witnesses propagate a refusal.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--selector-result", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    for mode in MODES:
        parser.add_argument("--" + mode + "-result", type=Path, required=True)
    args = parser.parse_args()
    views = {mode: identity(getattr(args, mode.replace("-", "_") + "_result").resolve(), ROOT) for mode in MODES}
    result = qualify(args.configuration, identity(args.selector_result.resolve(), ROOT), views)
    context = create_run_context("selected-ground", "selected-closure", run_id=args.run_id, root=ROOT)
    target = contained(context.reports / ("current-" + args.configuration + ".json"), context.reports)
    with target.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(result, indent=2) + "\n")
    print(json.dumps(dict(result=identity(target, ROOT), passed=True, selected_families=50,
                          selected_native_poses=15488, current_viewport_families=24, later_owner_families=26)))


if __name__ == "__main__":
    main()
