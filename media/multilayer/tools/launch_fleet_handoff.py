"""Freeze qualified launch/fleet source art for append-only native packing.

@file launch_fleet_handoff.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups62H-62J.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Source-era native receipts remain historical authoring evidence after native
owners change. This adapter preserves accepted original pixels/physical cuts;
it neither grants native compatibility nor publishes a packed payload.
"""

import argparse
import hashlib
import json

from PIL import Image

import convert_space_art as source
import pack_m1_art as packing
from tools import cargo_rocket_art as art

ROOT = source.ROOT
ASSETS = ("facility.launch-center", "vehicle.model-14")
HANDOFF = ROOT/"media/multilayer/source/space/launch-fleet-source-handoff-v1.json"
OUTPUT = ROOT/"logs/proofs/mlm/native-art/space/launch-fleet-handoff-v1"
FIELDS = ("state", "frame", "direction", "tile_variant", "part")


def key(row):
    """Address a physical source slot in the established native packing order.

    @param row Explicit logical or canonical source row.
    @return State/frame/direction/tile/part tuple without new saved identities.
    """
    return tuple(row[name] for name in FIELDS)


def address(row):
    """Serialize one exact source identity for a compact immutable pixel map.

    @param row Qualified canonical source row with strict integer frame/tile.
    @return Unambiguous JSON tuple spelling.
    """
    return json.dumps(key(row), separators=(",", ":"))


def bound(reference):
    """Verify an explicit repository input without consulting recency or external files.

    @param reference Repository-relative path and exact immutable SHA256.
    @return Resolved existing repository file.
    @throws ValueError On missing, changed or outside-repository input.
    """
    path = (ROOT/reference["path"]).resolve()
    # A hash does not authorize arbitrary workstation inputs as selected art.
    if not path.is_relative_to(ROOT) or not path.is_file():
        raise ValueError("launch/fleet bound source is absent or outside repository")
    if source.identity(path) != reference:
        raise ValueError("launch/fleet immutable input changed: "+reference["path"])
    return path


def source_contract(family):
    """Separate immutable authored source/geometry from later packing/runtime bindings.

    @param family Actual public launch-center or rocket descriptor.
    @return Exact physical/source contract owned before payload publication.
    """
    return {name: value for name, value in family.items() if name not in ("packed", "authoring")}


def freeze():
    """Capture genuinely qualified source boundaries once, before native/payload changes.

    @return Immutable launch/fleet handoff after actual original producer verification.
    @throws ValueError If an accepted snapshot exists or any preceding gate is stale.
    """
    if HANDOFF.exists():
        raise ValueError("launch/fleet handoff already exists; never regenerate accepted expectations")
    import convert_cargo_rocket_source as rocket
    import convert_launch_center_source as facility
    import pack_space_art as space
    import pack_orbital_art as orbital

    public, record = art.read_json(art.PUBLIC), source.read_record()
    art.checked_predecessor(record, public)
    space_scope, orbital_scope = space.read_scope(), orbital.read_scope()
    # Existing native owners end exactly at6143. A new family must append
    # after both complete Space and orbital ranges without reclaiming gaps.
    if space_scope["native_end_exclusive"] != 5167 or orbital_scope["native_end_exclusive"] != 6143:
        raise ValueError("launch/fleet predecessor native extents changed")
    families = {row["asset_id"]: row for row in public["assets"]}
    sources = {}
    # Actual producer decoders qualify every accepted part and direct alias
    # while their original native proof inputs/executables still match.
    for asset, converter, count in ((ASSETS[0], facility, 6), (ASSETS[1], rocket, 7)):
        family = families[asset]
        report_path = ROOT/family["authoring"]["atlas_manifest"]
        proof_path = report_path.parent/"verification.json"
        report, proof = art.read_json(report_path), art.read_json(proof_path)
        reference = source.identity(report_path)
        # Preceding executable source qualification, public ownership and
        # actual saved bytes must agree; flags alone cannot freeze missing art.
        if (proof["status"] != "passed" or proof["cases"] != count or proof["source_manifest"] != reference
                or family["authoring"]["atlas_manifest_sha256"] != reference["sha256"]
                or family["source"]["status"] != "authored" or family["authoring"]["coverage_complete"] is not True):
            raise ValueError(asset+": current accepted source qualification is absent")
        for name in (("converter", "test", "helper", "occupied_helper") if asset == ASSETS[0] else ("converter", "tests")):
            bound(proof[name])
        with Image.open(bound(report["atlas"])) as image:
            decoded = converter.decode(report, image)
        canonical = {address(row): dict(row_sha256=art.semantic_digest(row), native_sha256=row["native_sha256"])
                     for row in report["canonical_parts"]}
        sources[asset] = dict(report=reference, qualification=source.identity(proof_path), atlas=report["atlas"],
                              source_contract=source_contract(family), definition_binding={name: family["packed"][name] for name in ("namespace", "offset", "sha256")},
                              report_sha256=art.semantic_digest(report), canonical=canonical, logical_poses=len(decoded),
                              canonical_images=len(canonical), absent_logical_parts=sum(row["structurally_absent"] for row, _ in decoded))
    protected = {entry["path"]: entry for entry in orbital_scope["protected_outputs"]}
    # Preserve every previous packing sheet/declaration and its source scope;
    # the new append will not edit the existing Space/orbital producer files.
    for path in (orbital_scope["png"], orbital_scope["nfo"], orbital_scope["header"], "media/multilayer/orbital-pack-scope.json"):
        protected[path] = source.identity(ROOT/path)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    previous_grf = OUTPUT/"predecessor-openttd.grf"
    previous_grf.write_bytes((ROOT/"media/baseset/openttd.grf").read_bytes())
    # Retain actual predecessor payload bytes before rebuilding the shared GRF;
    # its later checksum must change while every old decoded sprite stays exact.
    handoff = dict(_documentation=__doc__, schema_version=1, asset_order=list(ASSETS), native_start=6143,
                   native_end_exclusive=7791, native_route="native_baseset_launch", sources=sources,
                   protected_outputs=list(protected.values()), predecessor_payload=source.identity(previous_grf),
                   predecessor_payload_original=source.identity(ROOT/"media/baseset/openttd.grf"),
                   prior_public_ranges={name: row["packed"] for name, row in families.items() if name not in ASSETS},
                   prior_public_rows={name: art.semantic_digest(row) for name, row in families.items() if name not in ASSETS},
                   public_envelope_sha256=art.semantic_digest({name: value for name, value in public.items() if name != "assets"}),
                   candidate_count=len(record["candidates"]), candidate_prefix_sha256=art.semantic_digest(record["candidates"]),
                   exclusion_count=len(record["excluded_sources"]), exclusion_prefix_sha256=art.semantic_digest(record["excluded_sources"]),
                   authoring_native_evidence="historical_qualified_before_runtime_changes; current_native_compatibility_required_separately",
                   packed=False, runtime=False)
    HANDOFF.write_text(json.dumps(handoff, indent=2)+"\n", encoding="utf-8")
    return handoff


def checked_handoff(handoff=None):
    """Require retained accepted source/proof bytes and immutable original ancestry.

    @param handoff Optional detached frozen handoff for focused refusal checks.
    @return Current accepted source handoff; no historical native owner is reactivated.
    @throws ValueError On wrong scope, changed proofs or original provenance drift.
    """
    handoff = art.read_json(HANDOFF) if handoff is None else handoff
    # The snapshot grants exactly two source families and an append extent;
    # a completion flag cannot add another native or gameplay authority.
    if (handoff["asset_order"] != list(ASSETS) or handoff["native_start"] != 6143
            or handoff["native_end_exclusive"] != 7791 or handoff["native_route"] != "native_baseset_launch"
            or handoff["packed"] is not False or handoff["runtime"] is not False):
        raise ValueError("launch/fleet qualified source scope changed")
    record = source.read_record()
    # Future original authoring may append, but cannot rewrite any of the
    # genuine sources/rejections used to qualify this physical sprite boundary.
    if (art.semantic_digest(record["candidates"][:handoff["candidate_count"]]) != handoff["candidate_prefix_sha256"]
            or art.semantic_digest(record["excluded_sources"][:handoff["exclusion_count"]]) != handoff["exclusion_prefix_sha256"]):
        raise ValueError("launch/fleet qualified original provenance changed")
    # Retained original bytes, full prompt receipts and actual generation
    # inputs remain immutable, including rejected drafts. Metadata equality
    # alone cannot hide a missing or replaced historical original PNG.
    originals = {}
    for candidate in record["candidates"][:handoff["candidate_count"]]:
        if candidate["asset_id"] not in ASSETS:
            continue
        # Launch-center receipts declare reference_inputs; rocket receipts
        # also expose references. The shared actual-input records carry roles
        # in addition to the exact path/hash checked at this source boundary.
        references = [{name: value[name] for name in ("path", "sha256")} for value in candidate["reference_inputs"]]
        for reference in ({name: candidate[name] for name in ("path", "sha256")}, candidate["generation_receipt"], *references):
            originals[reference["path"], reference["sha256"]] = reference
    for reference in originals.values():
        bound(reference)
    for asset in ASSETS:
        accepted = handoff["sources"][asset]
        report, proof = art.read_json(bound(accepted["report"])), art.read_json(bound(accepted["qualification"]))
        # Actual accepted proof receipts and source reports remain immutable
        # historical evidence, independent of later legitimate native builds.
        if (proof["status"] != "passed" or proof["cases"] != (6 if asset == ASSETS[0] else 7)
                or proof["source_manifest"] != accepted["report"] or report["asset_id"] != asset
                or report["coverage_complete"] is not True or report["packed"] is not False or report["runtime_activated"] is not False
                or art.semantic_digest(report) != accepted["report_sha256"]):
            raise ValueError(asset+": qualified historical source evidence changed")
        # The source freeze must preserve the accepted report's independent
        # canonical expectations; callers cannot supply a coherently forged
        # replacement pixel table alongside a changed PNG buffer.
        if (accepted["logical_poses"] != (1536 if asset == ASSETS[0] else 112)
                or accepted["canonical_images"] != len(report["canonical_parts"])
                or accepted["canonical"] != {address(row): dict(row_sha256=art.semantic_digest(row), native_sha256=row["native_sha256"])
                                              for row in report["canonical_parts"]}):
            raise ValueError(asset+": frozen accepted source expectation changed")
        bound(accepted["atlas"])
    return handoff


def source_poses(family, report=None, image=None, handoff=None):
    """Adapt exact qualified launch/rocket physical parts to the existing native packer.

    @param family Public descriptor with immutable source/geometry and staged packed fields.
    @param report Optional detached complete source report for metadata refusal checks.
    @param image Optional actual indexed atlas or detached PNG corruption buffer.
    @param handoff Optional detached frozen handoff, rechecked against its accepted proofs.
    @return Identity-keyed physical source poses and their verified indexed atlas.
    @throws ValueError On missing, duplicate, cross-view, false-absence or rehashed source.
    """
    handoff = checked_handoff(handoff)
    asset = family["asset_id"]
    if asset not in ASSETS:
        raise ValueError("launch/fleet adapter refuses an unqualified family")
    accepted = handoff["sources"][asset]
    authoring = family["authoring"]
    # Current public source ownership must still name the accepted actual
    # source atlas/report; later packed fields cannot change physical geometry.
    if (art.semantic_digest(source_contract(family)) != art.semantic_digest(accepted["source_contract"])
            or authoring["coverage_complete"] is not True
            or authoring["atlas_manifest"] != accepted["report"]["path"]
            or authoring["atlas_manifest_sha256"] != accepted["report"]["sha256"]
            or authoring["native_atlas"] != accepted["atlas"]["path"] or authoring["native_sha256"] != accepted["atlas"]["sha256"]):
        raise ValueError(asset+": public qualified source binding changed")
    report = art.read_json(bound(accepted["report"])) if report is None else report
    if image is None:
        with Image.open(bound(accepted["atlas"])) as opened:
            image = opened.copy()
    packing.validate_source_image(image, asset)
    required = set(packing.required_keys(family))
    observed = [key(row) for row in report["sprites"]]
    # An exact source product protects actual rotated cells and both rocket
    # frames; matching aggregate counts alone cannot supply missing positions.
    if (len(observed) != accepted["logical_poses"] or set(observed) != required
            or len(observed) != len(required) or report["coverage_complete"] is not True):
        raise ValueError(asset+": incomplete or duplicate physical source coverage")
    canonical = {key(row): row for row in report["canonical_parts"]}
    # Canonical native metadata and frozen pixel hashes were accepted before
    # this packing stage; a rehashed PNG/report cannot redefine an original cut.
    if (len(canonical) != accepted["canonical_images"] or len(canonical) != len(report["canonical_parts"])
            or {address(row): dict(row_sha256=art.semantic_digest(row), native_sha256=row["native_sha256"])
                for row in canonical.values()} != accepted["canonical"]):
        raise ValueError(asset+": accepted canonical physical source changed")
    poses = {}
    # Every logical alias keeps its original heading/cell/part/frame policy;
    # approved zero-pixel back/front cuts remain real slots, not missing artwork.
    for row in report["sprites"]:
        slot = key(row)
        alias = row["alias_of"]
        target = canonical.get(key(alias) if alias is not None else slot)
        if target is None:
            raise ValueError(asset+": missing direct canonical alias target")
        # Direct aliases may reuse pixels but never change physical view or
        # depth. The immutable report below additionally binds the exact policy.
        if (type(row["frame"]) is not int or type(row["tile_variant"]) is not int
                or target["alias_of"] is not None or row["direction"] != target["direction"]
                or row["tile_variant"] != target["tile_variant"] or row["part"] != target["part"]
                or row["box"] != target["box"] or row["anchor"] != family["anchor"]
                or row["native_sha256"] != target["native_sha256"]):
            raise ValueError(asset+": changed direct physical alias or geometry")
        x, y, width, height = row["box"]
        # Cropping can remove only transparent margins from the exact native
        # cell; neither oversized boxes nor Boolean coordinates create coverage.
        if (any(type(value) is not int for value in row["box"]) or [width, height] != family["native_size"]
                or not (0 <= x < x+width <= image.width and 0 <= y < y+height <= image.height)):
            raise ValueError(asset+": physical source rectangle leaves its native atlas")
        pixels = image.crop((x, y, x+width, y+height))
        empty = pixels.getbbox() is None
        # Original ownership can leave either physical facility part empty:
        # some cells contain only foreground shell, others only background.
        # Their frozen canonical rows alone authorize absence; a missing body
        # or newly painted cut cannot pass as accepted complete source art.
        if (hashlib.sha256(pixels.tobytes()).hexdigest() != target["native_sha256"]
                or row["structurally_absent"] is not empty
                or row["opaque_pixels"] != sum(value != 0 for value in pixels.get_flattened_data())
                or (empty and asset != ASSETS[0])):
            raise ValueError(asset+": changed original pixels or false physical absence")
        poses[slot] = dict(row)
    # Bind all accepted phase, occupancy, contact, owner-map and alias metadata,
    # including fields the shared low-level cropper does not itself interpret.
    if art.semantic_digest(report) != accepted["report_sha256"]:
        raise ValueError(asset+": qualified source report metadata changed")
    return poses, image


def validate_prepacking_prefix(handoff=None):
    """Check current predecessor payload/public rows before any new packed publication.

    @param handoff Optional already verified source freeze for this focused batch.
    @return None; later legitimate GRF publication needs decoded prefix comparison instead.
    @throws ValueError On changed published ranges, old source outputs or payload bytes.
    """
    handoff = checked_handoff() if handoff is None else handoff
    public = art.read_json(art.PUBLIC)
    families = {row["asset_id"]: row for row in public["assets"] if row["asset_id"] not in ASSETS}
    # No prior family/range may disappear or move to make room for this append.
    if ({name: art.semantic_digest(row) for name, row in families.items()} != handoff["prior_public_rows"]
            or art.semantic_digest({name: value for name, value in public.items() if name != "assets"}) != handoff["public_envelope_sha256"]):
        raise ValueError("launch/fleet predecessor public family/range changed")
    for reference in handoff["protected_outputs"]:
        bound(reference)
    bound(handoff["predecessor_payload"])
    bound(handoff["predecessor_payload_original"])


def layout(public=None, handoff=None):
    """Describe the exact append through existing family domains without staging native files.

    @param public Optional detached public contract for range/geometry refusal checks.
    @param handoff Optional qualified frozen source handoff, checked before allocation.
    @return Ordered two-family ranges and exclusive7791bound; no sprite is published.
    @throws ValueError On prior range drift, changed source domains or relocated new bindings.
    """
    handoff = checked_handoff(handoff)
    public = art.read_json(art.PUBLIC) if public is None else public
    rows = {row["asset_id"]: row for row in public["assets"]}
    if len(rows) != len(public["assets"]):
        raise ValueError("launch/fleet public family is duplicated")
    # All existing native families retain their original route/offset/count.
    # Shared GRF checksum refresh is a separate decoded-payload publication gate.
    for asset, old in handoff["prior_public_ranges"].items():
        if "native_offset" not in old:
            continue
        current = rows.get(asset, {}).get("packed", {})
        if any(current.get(name) != old.get(name) for name in ("namespace", "offset", "route", "native_offset", "sprite_count")):
            raise ValueError("launch/fleet protected native range changed: "+asset)
    cursor, ranges = handoff["native_start"], []
    for asset in ASSETS:
        family = rows[asset]
        accepted = handoff["sources"][asset]
        packing.validate_family(family)
        count = len(packing.required_keys(family))
        packed = family["packed"]
        # Existing source geometry and any already staged append are immutable;
        # a reordered selection or new source field cannot relocate native IDs.
        if (art.semantic_digest(source_contract(family)) != art.semantic_digest(accepted["source_contract"])
                or count != accepted["logical_poses"] or packed.get("native_offset", cursor) != cursor
                or packed.get("sprite_count", count) != count or packed.get("route", handoff["native_route"]) != handoff["native_route"]
                or packed["namespace"] != accepted["definition_binding"]["namespace"]
                or packed["offset"] != accepted["definition_binding"]["offset"]):
            raise ValueError(asset+": qualified native append binding changed")
        ranges.append(dict(asset_id=asset, native_offset=cursor, sprite_count=count,
                           **{name: family[name] for name in ("states", "frames", "directions", "tile_variants", "parts", "native_size", "anchor")}))
        cursor += count
    if cursor != handoff["native_end_exclusive"]:
        raise ValueError("launch/fleet native append extent changed")
    return ranges, cursor


def main():
    """Freeze or inspect only this qualified source boundary, without native publication.

    @return Zero after the requested pure source qualification succeeds.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--freeze", action="store_true")
    args = parser.parse_args()
    handoff = freeze() if args.freeze else checked_handoff()
    validate_prepacking_prefix(handoff)
    print(json.dumps(dict(assets=handoff["asset_order"], native_start=handoff["native_start"],
                          native_end_exclusive=handoff["native_end_exclusive"], packed=False, runtime=False)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
