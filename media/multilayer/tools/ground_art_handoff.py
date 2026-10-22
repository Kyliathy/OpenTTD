"""Normalize only the qualified selected ground sources for an independent native append.

@file ground_art_handoff.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DE, selected current source inputs.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.4D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Existing source matrices, native prefixes and simulation owners remain intact.
Transparent parts require the exact producer's structural/visibility policy.
"""

import copy
import hashlib
import json
from pathlib import Path
import sys

import PIL
from PIL import Image

import pack_m1_art as packing
import verify_selected_ground_sources as selected


ROOT = selected.ROOT
PUBLIC = selected.PUBLIC
SCOPE = ROOT / "media/multilayer/ground-pack-scope.json"
GEOMETRY = ("asset_id", "consumer", "states", "frames", "directions", "tile_variants", "parts",
            "native_size", "anchor", "footprint", "geometry", "company_colors", "animation_colors")
COMPANY_INDICES = set(range(198, 206))


def bound(relative):
    """Resolve a real repository-owned input or intended output path.

    @param relative Nonempty repository-relative path, never an external cache alias.
    @return Resolved absolute repository path.
    @throws ValueError On missing path declarations or repository escape.
    """
    # A missing declaration must not become a guessed path or an external
    # artifact that circumvents this checkout's immutable source identities.
    if not isinstance(relative, str) or not relative:
        raise ValueError("ground source path declaration is absent")
    path = (ROOT / relative).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError("ground source path escapes the repository")
    return path


def digest(path):
    """Hash the actual bytes of a source or retained prefix artifact.

    @param path Existing absolute repository path.
    @return Exact SHA256 identity.
    """
    if not path.is_file():
        raise ValueError("ground required input was removed or is not a file: " + str(path))
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(relative):
    """Read an explicit repository JSON owner without fallback or schema guessing.

    @param relative Repository-relative JSON path.
    @return Decoded original document.
    """
    return json.loads(bound(relative).read_text(encoding="utf-8"))


def read_scope(public=None, current_sources=False):
    """Validate the current selected source/range contract and retained native prefix.

    @param public Optional detached public document for focused refusal checks.
    @param current_sources Use independently retained physical/prefix facts without old publication products.
    @return Scope, before snapshot and current public document; no files are changed.
    @throws ValueError On source selection, pinned tools, published range or append drift.
    """
    scope = read_json(SCOPE.relative_to(ROOT).as_posix())
    if current_sources:
        from ground_source_facts import load_facts, verify_facts
        facts = load_facts()
        verify_facts()
        frozen = {row["asset_id"]: dict(row["geometry"], packed=row["packed"]) for row in facts["native_prefix"]}
        # The old append snapshot bound three already-shipped selected families.
        # Reconstruct only that exact before-view, not current Ground ranges or
        # the obsolete2145-file source/application publication snapshot.
        for asset, physical in facts["selected_public_facts"].items():
            packed = frozen.get(asset, {}).get("packed", {"native_offset": None})
            frozen[asset] = dict(physical, packed=packed)
        before = dict(native_end_exclusive=scope["native_start"], native_prefix=facts["native_prefix"],
                      public={"assets": list(frozen.values())})
    else:
        before = read_json(scope["baseline"])
    public = read_json(PUBLIC.relative_to(ROOT).as_posix()) if public is None else public
    # This append begins after every current M1/Space/orbital/launch sprite;
    # an old M1-only bound cannot reclaim later published slots.
    if (scope["selected_asset_ids"] != list(selected.SELECTED) or scope["native_route"] != "native_baseset_ground"
            or scope["native_start"] != before["native_end_exclusive"] or scope["native_start"] != 7791
            or scope["protected_native_end_exclusive"] != 7791 or scope["native_end_exclusive"] != 22045
            or scope["sprite_count"] != 14254 or len(before["native_prefix"]) != 62
            or scope["toolchain"]["python"] != sys.version.split()[0] or scope["toolchain"]["pillow"] != PIL.__version__):
        raise ValueError("ground source selection, native append or pinned toolchain changed")
    audit = scope["source_audit"]
    if not current_sources:
        if digest(bound(audit["path"])) != audit["sha256"]:
            raise ValueError("ground selected source audit changed")
    current = {row["asset_id"]: row for row in public["assets"]}
    if len(current) != len(public["assets"]):
        raise ValueError("ground public family identities are ambiguous")
    frozen = {row["asset_id"]: row for row in before["public"]["assets"]}
    # Current published range/enum identities remain untouched. Global GRF
    # checksum joins are separately qualified after actual payload decoding.
    for old in before["native_prefix"]:
        row = current.get(old["asset_id"], {})
        packed = row.get("packed", {})
        if any(packed.get(key) != old["packed"].get(key) for key in ("route", "native_offset", "sprite_count")):
            raise ValueError("ground append changes a published native prefix range")
        if any(row.get(key) != frozen[old["asset_id"]].get(key) for key in GEOMETRY):
            raise ValueError("ground append changes published native prefix geometry")
    expected_bound = [name for name in selected.SELECTED if frozen[name]["packed"].get("native_offset") is not None]
    expected_new = [name for name in selected.SELECTED if name not in expected_bound]
    if scope["already_bound_asset_ids"] != expected_bound or scope["asset_ids"] != expected_new:
        raise ValueError("ground append drops or duplicates an already bound selected family")
    references = scope["source_rows"]
    if [row["asset_id"] for row in references] != expected_new:
        raise ValueError("ground explicit source adapter matrix is incomplete")
    cursor = scope["native_start"]
    # Every new range follows exact canonical matrix cardinality. Public source
    # ordering cannot insert a family ahead of the frozen append selection.
    for reference in references:
        family = current.get(reference["asset_id"])
        if family is None:
            raise ValueError("ground selected source family is absent")
        packing.validate_family(family)
        count = len(packing.required_keys(family))
        if reference["native_offset"] != cursor or reference["sprite_count"] != count:
            raise ValueError("ground append range or source matrix changed")
        cursor += count
    if cursor != scope["native_end_exclusive"]:
        raise ValueError("ground final native append bound changed")
    return scope, before, public


def validate_preserved_files(scope, before):
    """Require byte-exact prior files during the input-staging phase.

    @param scope Exact executing append scope, including its retained predecessor payload.
    @param before Full2145-file prefix/source/application snapshot from79D.1.
    @return None; later payload publication needs its own explicit transition proof.
    """
    # Input staging owns only new Ground artifacts. An old source, executable,
    # header or GRF must never be rewritten as a side effect of normalization.
    for row in before["files"]:
        if digest(bound(row["path"])) != row["sha256"]:
            raise ValueError("ground input staging changed preserved file: " + row["path"])
    predecessor = before["predecessor_payload"]
    if digest(bound(predecessor["path"])) != predecessor["sha256"]:
        raise ValueError("ground predecessor payload copy changed")


def allowed_empty(asset_id, pose, scope):
    """Recognize only declared absent parents, empty riser fronts or hidden natural vents.

    @param asset_id Executing selected physical/UI family.
    @param pose Exact original source row, retaining its producer's structural semantics.
    @param scope Explicit per-family transparent-part count and authority.
    @return Whether this exact source role permits a fully transparent native cell.
    """
    policy = scope["empty_source_policy"].get(asset_id)
    if policy is None:
        return False
    # Native station layouts omit particular parents; their exact source-row
    # flag distinguishes intentional absence from a missing rendered roof.
    if asset_id in ("facility.rail-station", "facility.road-terminal"):
        return pose.get("structurally_absent") is True
    if asset_id == "facility.utility-riser":
        return (pose["part"] == "front"
                and pose.get("foreground_policy") == "solid_native_industry_back_owns_original_pixels_front_empty")
    if asset_id == "geothermal.vent":
        return pose["state"] == "hidden"
    return False


def validate_native_palette(image, family):
    """Preserve approved vehicle remap masks while refusing animation and foreign color tables.

    @param image Exact immutable indexed native source, before any margin cropping.
    @param family Frozen public family with explicit company-color and animation policies.
    @return None; no palette index or mask pixel is changed.
    """
    # Qualified vehicles deliberately use the eight original engine company
    # shades in their authored masks. Other families keep the common strict
    # policy; neither path admits animated or arbitrary reserved material.
    if family["company_colors"] == "none":
        packing.validate_source_image(image, family["asset_id"])
        return
    if family["company_colors"] != "explicit_mask":
        raise ValueError("unsupported ground company-color policy")
    if (image.mode != "P" or image.info.get("transparency") != 0
            or image.getpalette() != packing.base.PALETTE_BYTES
            or family["animation_colors"] != "forbidden"):
        raise ValueError("ground native palette/transparency/animation policy changed")
    forbidden = packing.base.FORBIDDEN - {0} - COMPANY_INDICES
    if set(image.get_flattened_data()) & forbidden:
        raise ValueError("ground source contains forbidden animation/reserved colors")


def source_poses(family, scope, before, documents, images):
    """Normalize one of four exact producer schemas into complete canonical native poses.

    @param family Current public selected source family; gameplay state remains with its native owner.
    @param scope Qualified range/source adapter map and exact transparent-part policies.
    @param before Frozen public/source/native prefix snapshot.
    @param documents Per-conversion cache of actual decoded immutable source reports.
    @param images Per-conversion cache of verified indexed native source atlases.
    @return Canonical identity-to-pose mapping and its unchanged indexed source image.
    @throws ValueError On unknown schemas, changed identities, missing poses, geometry or illegal transparency.
    """
    asset_id = family["asset_id"]
    matches = [row for row in scope["source_rows"] if row["asset_id"] == asset_id]
    if len(matches) != 1:
        raise ValueError("ground source adapter is missing or ambiguous: " + asset_id)
    reference = matches[0]
    original = next(row for row in before["public"]["assets"] if row["asset_id"] == asset_id)
    if any(family.get(key) != original.get(key) for key in GEOMETRY):
        raise ValueError("ground source changes qualified native geometry: " + asset_id)
    source, authoring = family["source"], family.get("authoring", {})
    complete = authoring.get("coverage_complete") is True or source.get("source_coverage_complete") is True
    # Public source, original bytes and approved native atlas remain one
    # provenance chain; new packing metadata cannot authorize foreign pixels.
    if (source != original["source"] or source.get("status") != "authored" or not complete
            or authoring.get("native_atlas", source["path"]) != reference["source_atlas"]
            or authoring.get("native_sha256", source["sha256"]) != reference["source_sha256"]
            or digest(bound(source["path"])) != source["sha256"]):
        raise ValueError("ground immutable source binding changed: " + asset_id)
    report_path = reference["manifest"]
    if digest(bound(report_path)) != reference["manifest_sha256"]:
        raise ValueError("ground qualified source report changed: " + asset_id)
    if report_path not in documents:
        documents[report_path] = read_json(report_path)
    document = documents[report_path]
    kind = reference["adapter_kind"]
    # Each producer schema is explicit. A missing key never falls back to
    # another layout or to a draft/template/placeholder source image.
    if kind == "atlas_list_v1":
        if document.get("schema_version") != 1:
            raise ValueError("ground atlas-list source schema changed")
        candidates = [row for row in document["atlases"] if row["path"] == reference["source_atlas"]]
        if len(candidates) != 1:
            raise ValueError("ground authoritative atlas is missing or ambiguous")
        atlas = candidates[0]
        if atlas["sha256"] != reference["source_sha256"]:
            raise ValueError("ground atlas-list source hash changed")
        rows = atlas["sprites"]
    elif kind in ("physical_source_parts", "physical_sprite_rows"):
        if (document.get("asset_id") != asset_id or document.get("source_coverage_complete") is not True
                or document.get("source") != reference["source_atlas"]
                or document.get("source_sha256") != reference["source_sha256"]):
            raise ValueError("ground physical source report changed")
        atlas = document
        rows = document["source_parts" if kind == "physical_source_parts" else "sprites"]
    elif kind == "single_sprite_rows_v1":
        if (document.get("schema_version") != 1 or document.get("asset_id") != asset_id
                or document.get("source_coverage_complete") is not True
                or document.get("path") != reference["source_atlas"]
                or document.get("sha256", reference["source_sha256"]) != reference["source_sha256"]):
            raise ValueError("ground single-sprite source report changed")
        atlas, rows = document, document["sprites"]
    else:
        raise ValueError("unsupported ground source adapter: " + str(kind))
    image_path = reference["source_atlas"]
    if digest(bound(image_path)) != reference["source_sha256"]:
        raise ValueError("ground actual native source pixels changed")
    if image_path not in images:
        images[image_path] = Image.open(bound(image_path))
        validate_native_palette(images[image_path], family)
    image = images[image_path]
    if "size" in atlas and list(image.size) != atlas["size"]:
        raise ValueError("ground source atlas dimensions changed")
    poses, empty_count = {}, 0
    # Every source parent/cell/phase retains its complete native canvas before
    # margin cropping. Exact matrix identity and original pixel hashes prevent
    # duplicated views or a blank neighboring parent from concealing a gap.
    for raw in rows:
        if raw.get("asset_id", atlas.get("asset_id", asset_id)) != asset_id:
            continue
        pose = copy.deepcopy(raw)
        pose["anchor"] = pose.get("anchor", atlas.get("anchor"))
        key = (pose["state"], pose["frame"], pose["direction"], pose.get("tile_variant", 0), pose.get("part", "body"))
        if key in poses:
            raise ValueError("ground duplicate source pose: " + asset_id)
        x, y, width, height = pose["box"]
        if ([width, height] != family["native_size"] or pose["anchor"] != family["anchor"]
                or not (0 <= x < x + width <= image.width and 0 <= y < y + height <= image.height)):
            raise ValueError("ground source canvas/bounds/anchor changed: " + asset_id)
        cell = image.crop((x, y, x + width, y + height))
        empty = cell.getbbox() is None
        permits_empty = allowed_empty(asset_id, pose, scope)
        if empty != permits_empty:
            raise ValueError("ground source is missing art or contradicts structural absence: " + asset_id)
        empty_count += int(empty)
        pixel_sha = hashlib.sha256(cell.tobytes()).hexdigest()
        declared_sha = pose.get("source_pixels_sha256")
        if declared_sha is not None and declared_sha != pixel_sha:
            raise ValueError("ground original source-part pixel identity changed")
        pose.update(structurally_absent=empty, source_pixels_sha256=pixel_sha)
        # Source masks are read from the exact frozen native pixels. Cropping
        # and later decoding must preserve both their positions and indices;
        # conversion never recolors a company plaque or invents a new mask.
        mask = bytes(int(value in COMPANY_INDICES) for value in cell.get_flattened_data())
        pose["company_color_pixels"] = sum(mask)
        pose["company_color_mask_sha256"] = hashlib.sha256(mask).hexdigest()
        poses[key] = pose
    expected_empty = scope["empty_source_policy"].get(asset_id, {}).get("count", 0)
    if set(poses) != set(packing.required_keys(family)) or empty_count != expected_empty:
        raise ValueError("ground complete source matrix or intentional absence count changed: " + asset_id)
    return poses, image
