"""Pack the selected M1 source set into independent native Action 5 segments.

@file pack_m1_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 79B/80B/79C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoints 8.6/10.4B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups79-81.
Existing M0 PNG/NFO and MLVI definition bytes are never rewritten. Source
coverage must be verified before publication; --describe is layout preparation.
"""

import argparse
import copy
import hashlib
import itertools
import json
import re
import sys

import PIL
from PIL import Image

import convert_m1_art as base
from convert_terrain_art import png_bytes
from sprite_quantize import crop_and_shift


SCOPE = base.ROOT / "media/multilayer/m1-pack-scope.json"
PUBLIC = base.PUBLIC
PART_REPORTS = {"mine." + name for name in ("COAL", "IORE", "CORE", "SALT", "LITH", "COBT", "RARE")}
PART_REPORTS.update({"facility." + name for name in ("rail-station", "road-terminal", "metal-refinery",
                                                   "chemical-works", "battery-works", "electronics-works")})
SOURCE_BINDINGS = {"facility.utility-riser", "geothermal.conduit", "control.tube", "control.geothermal"}


def loader_marker(report):
    """Describe complete actual native loading, including independently preserved sprites.

    @param report Current emitted packing report with selected count, external span and native bound.
    @return Exact successful native-loader sentinel shared by packing and runtime proof consumers.
    """
    external = report["preserved_external_sprite_count"]
    return (f"M1_PACKED_ART_LOADED sprites={66 + report['pose_count'] + external} "
            f"native_bound={report['native_sprite_bound']} protected=66 selected={report['pose_count']} "
            f"external={external} pixel_hashes=exact")


def source_binding(family):
    """Read the existing producer's explicit source handoff without rewriting its schema.

    @param family One selected public family with authored source or authoring metadata.
    @return Uniform read-only atlas/report/count binding for the existing packer.
    """
    authoring = family.get("authoring", {})
    if authoring.get("coverage_complete") is True:
        return authoring
    source = family["source"]
    # Four completed utility/control producers publish their verified atlas
    # through source, not authoring. Only these exact owners authorize this form.
    if (family["asset_id"] in SOURCE_BINDINGS and source.get("status") == "authored"
            and source.get("source_coverage_complete") is True):
        return {"coverage_complete": True, "atlas_manifest": source["manifest"], "native_atlas": source["path"],
                "native_sha256": source["sha256"], "staged_pose_count": len(required_keys(family))}
    raise ValueError(family["asset_id"] + ": source verification pending; finish its producer before packing")


def absent_part(family, pose):
    """Resolve absence only from the actual station/terminal physical parent contract.

    @param family Selected native family with explicit nullable part declarations.
    @param pose One source identity, including actual direction and tile variant.
    @return Whether the physical native parent is structurally absent.
    """
    # The installed riser is a solid industry, with original pixels entirely
    # in back and an explicitly empty front. Its producer declares that native
    # policy rather than a station-style nullable parent list.
    riser_front = (family["asset_id"] == "facility.utility-riser" and pose.get("part") == "front"
                   and pose.get("foreground_policy") == "solid_native_industry_back_owns_original_pixels_front_empty"
                   and family.get("geometry_notes", {}).get("foreground_policy")
                   == "solid native industry footprint; back owns original opaque pixels, front empty; no capsule/vehicle track through machinery")
    return (riser_front or pose.get("tile_variant", 0) in family.get("nullable_parts", {}).get(pose.get("part"), [])
            or pose["direction"] in family.get("nullable_parts_by_direction", {}).get(pose.get("part"), []))


def physical_anchor(family, pose):
    """Check the native parent/child anchor against its published physical geometry.

    @param family Selected station/terminal contract, or an ordinary source family.
    @param pose Original source cell with any explicitly compensated native parent anchor.
    @return Validated packed anchor; absent parents retain a transparent source placeholder.
    """
    anchors = family.get("native_packed_anchors")
    if anchors is None:
        return pose["anchor"]
    graphics = family["native_graphics"][pose["direction"]]
    key = str(graphics[pose["tile_variant"]]) if isinstance(graphics, list) else pose["direction"]
    expected = anchors[key][pose["part"]]
    if pose.get("native_packed_anchor") != expected:
        raise ValueError(family["asset_id"] + ": native parent/child anchor changed")
    return pose["anchor"] if expected is None else expected


def required_keys(family):
    """Expand exact source identities in the shared native packing order.

    @param family Published state/frame/direction/tile/part geometry contract.
    @return Ordered tuples (state, frame, direction, tile_variant, part).
    """
    return list(itertools.product(family["states"], range(family["frames"]), family["directions"],
                                  range(family["tile_variants"]), family["parts"]))


def validate_family(family):
    """Reject malformed selected geometry before allocating native references.

    @param family Public selected family with exact state/view/part/native fields.
    @return None; reject missing/duplicated coverage terms or unusable canvases.
    """
    required = {"asset_id", "states", "frames", "directions", "tile_variants", "parts", "native_size", "anchor", "packed"}
    if not required.issubset(family):
        raise ValueError("selected source family is missing required fields")
    # Duplicate names produce duplicate identities in an apparently larger
    # Cartesian set; empty domains cannot allocate meaningful native ranges.
    for field in ("states", "directions", "parts"):
        values = family[field]
        if not values or len(values) != len(set(values)):
            raise ValueError(family["asset_id"] + ": missing/duplicate " + field)
    if family["frames"] < 1 or family["tile_variants"] < 1:
        # Every source family needs at least one genuine phase and physical/UI
        # variant; a zero-sized range cannot count as complete delivered art.
        raise ValueError(family["asset_id"] + ": empty native coverage")
    if len(family["native_size"]) != 2 or len(family["anchor"]) != 2 or any(value < 1 for value in family["native_size"]):
        # A pixel canvas and its world/UI anchor must share explicit x/y axes.
        raise ValueError(family["asset_id"] + ": invalid native canvas/anchor")


def selected_ranges(public, scope):
    """Preserve published native IDs/enum order and append new source families.

    @param public Current public sprite contract; source and packed fields persist.
    @param scope Selected IDs, frozen published range order and native art route.
    @return Ordered native family/range rows and the exclusive sprite bound.
    @throws ValueError If IDs duplicate, overlap or enter protected/unactivated slots.
    """
    selected = set(scope["asset_ids"])
    if len(selected) != len(scope["asset_ids"]):
        raise ValueError("duplicate selected M1 family")
    start, core_end = scope["planned_core_range"]
    if scope["appended_start"] != core_end or public["core_extension"]["planned_end_exclusive"] != core_end:
        # Later art starts after all published core slots, including unselected
        # future families. Selection cannot reclaim those existing public offsets.
        raise ValueError("M1 append start disagrees with published core range")
    families = {}
    # A public duplicate must not silently replace the geometry used by an old
    # native range or allow two source owners to claim the same selected name.
    for family in public["assets"]:
        if family["asset_id"] not in selected:
            continue
        if family["asset_id"] in families:
            raise ValueError("duplicate public M1 family: " + family["asset_id"])
        validate_family(family)
        families[family["asset_id"]] = family
    if set(families) != selected:
        raise ValueError("selected M1 family is absent from public contract")

    preserved = scope["preserved_ranges"]
    preserved_ids = [row["asset_id"] for row in preserved]
    if len(preserved_ids) != len(set(preserved_ids)) or not set(preserved_ids).issubset(selected):
        # A selection cannot drop/redeclare a previously shipped family and
        # reclaim its native slots or shift its generated enum identity.
        raise ValueError("missing/duplicate preserved M1 family")
    frozen = {row["asset_id"]: row for row in preserved}
    result, occupied = [], set()
    # The already shipped Space preview has independent NFO/source ownership.
    # New M1 art appends beyond it rather than moving or overwriting those IDs.
    for row in scope.get("preserved_external_ranges", []):
        numbers = set(range(row["native_offset"], row["native_offset"] + row["sprite_count"]))
        if row["sprite_count"] < 1 or occupied & numbers:
            raise ValueError("invalid/overlapping preserved external range")
        occupied.update(numbers)
    cursor = max([scope["appended_start"]] + [row["native_offset"] + row["sprite_count"]
                 for row in preserved + scope.get("preserved_external_ranges", [])])
    order = preserved_ids + [asset_id for asset_id in scope["asset_ids"] if asset_id not in frozen]
    # Shipped declaration order is part of the native selector contract. New
    # families follow the explicit selection order, even when their public
    # manifest rows occur before an existing appended family.
    for asset_id in order:
        family = families[asset_id]
        packed = family["packed"]
        count = len(required_keys(family))
        if asset_id in frozen:
            old = frozen[asset_id]
            offset = old["native_offset"]
            if count != old["sprite_count"] or packed.get("native_offset") != offset or packed.get("sprite_count") != count:
                # Existing world/UI sprite IDs retain both their extent and
                # public binding; a changed source matrix needs a new family.
                raise ValueError("published native range changed: " + asset_id)
        elif packed["namespace"] == "native_map_layer_extension":
            offset = packed["offset"]
        else:
            offset = cursor
            cursor += count
        if packed["namespace"] == "native_map_layer_extension":
            if offset is None or offset < start or offset + count > core_end:
                # Published core ranges remain exact even when only some
                # families are delivered; no art can spill into later slots.
                raise ValueError("invalid selected core range: " + asset_id)
            if packed["offset"] != offset:
                raise ValueError("published core offset changed: " + asset_id)
        else:
            if offset < scope["appended_start"]:
                raise ValueError("appended family enters protected/core slots: " + asset_id)
        if packed.get("native_offset", offset) != offset or packed.get("sprite_count", count) != count:
            # A repeated conversion must reproduce already published new
            # ranges too; inserting another selection cannot relocate them.
            raise ValueError("selected native binding changed: " + asset_id)
        if "route" in packed and packed["route"] != scope["native_route"]:
            # A definition-local ID cannot be treated as an existing native
            # artwork binding belonging to a different payload route.
            raise ValueError("selected native route changed: " + asset_id)
        numbers = set(range(offset, offset + count))
        if occupied & numbers:
            raise ValueError("overlapping selected native range: " + asset_id)
        occupied.update(numbers)
        result.append({"asset_id": asset_id, "native_offset": offset, "sprite_count": count,
                       "states": family["states"], "frames": family["frames"], "directions": family["directions"],
                       "tile_variants": family["tile_variants"], "parts": family["parts"],
                       "native_size": family["native_size"], "anchor": family["anchor"]})
    # Published native families outside the selection still own their slots.
    # Refuse silently dropping them instead of recycling their old numbers.
    for family in public["assets"]:
        if family["asset_id"] not in selected and family["packed"].get("route") == scope["native_route"]:
            raise ValueError("published native family omitted: " + family["asset_id"])
    if cursor > 65535:
        raise ValueError("M1 native block exceeds Action 5 count range")
    return result, cursor


def validate_source_image(image, asset_id, company_colors="none"):
    """Enforce native DOS colors and alpha on independently supplied source pixels.

    @param image Indexed source atlas or a bounded in-memory corruption fixture.
    @param asset_id Public family identity included in actionable error messages.
    @param company_colors Explicit published company-mask policy; ordinary material stays fixed.
    @return None; reject a different palette, alpha policy or reserved color.
    """
    if image.mode != "P" or image.info.get("transparency") != 0 or image.getpalette() != base.PALETTE_BYTES:
        # The whole native art route shares one DOS palette; changing that table
        # changes color meaning even when sprite geometry and indices are equal.
        raise ValueError(asset_id + ": source palette/transparency mismatch")
    forbidden = base.FORBIDDEN - {0}
    if company_colors == "explicit_mask":
        forbidden = forbidden - set(range(198, 206))
    if set(image.get_flattened_data()) & forbidden:
        raise ValueError(asset_id + ": reserved source palette entry")


def validate_company_mask(family, pose, cell):
    """Require each company-color pixel to belong to an explicit authored physical fitting.

    @param family Selected native family with its published remapping policy.
    @param pose Source pose declaring individual ground-fleet pixels or one capsule plaque.
    @param cell Original indexed native source cell before transparent-margin cropping.
    @return None after exact pixel-coordinate and shade correspondence.
    """
    rows = pose.get("components", {}).get("company_mask", [])
    plaque = pose.get("company_plaque")
    if plaque is not None:
        rows = [plaque]
    expected = {tuple(row["target"]): row["palette_index"] for row in rows}
    actual = {(index % cell.width, index // cell.width): value
              for index, value in enumerate(cell.get_flattened_data()) if 198 <= value < 206}
    # Ordinary materials and cargo cannot acquire livery merely because the
    # family supports remapping; only the exact original fitting may change.
    if actual != expected or (actual and family.get("company_colors") != "explicit_mask"):
        raise ValueError(family["asset_id"] + ": company palette leaves its explicit original mask")


def source_poses(family, documents, images):
    """Resolve a family's authoritative source poses and enforce producer coverage.

    @param family Published source/authoring geometry and authoritative atlas binding.
    @param documents Cache of decoded source-atlas reports by repository path.
    @param images Cache of verified DOS source images by repository path.
    @return Identity-keyed source-pose records and the immutable indexed source image.
    @throws ValueError If the source is unverified, stale or missing exact coverage.
    """
    asset_id = family["asset_id"]
    authoring = source_binding(family)
    report_path = authoring["atlas_manifest"]
    if report_path not in documents:
        documents[report_path] = json.loads(base.repository_path(report_path).read_text(encoding="utf-8"))
    document = documents[report_path]
    if "atlases" in document:
        candidates = [row for row in document["atlases"] if row["path"] == authoring["native_atlas"]]
    elif asset_id in PART_REPORTS:
        # These thirteen completed native producers retain source/source_parts
        # reports. Normalize their explicit handoff in memory, never amend the
        # original report or infer coverage from a picture's dimensions.
        if (document.get("asset_id") != asset_id or document.get("source") != authoring["native_atlas"]
                or document.get("source_sha256") != authoring["native_sha256"]
                or document.get("source_coverage_complete") is not True
                or document.get("source_slots") != len(required_keys(family))):
            raise ValueError(asset_id + ": invalid native source-parts handoff " + report_path)
        with Image.open(base.repository_path(document["source"])) as original:
            size = list(original.size)
        candidates = [{"asset_id": asset_id, "path": document["source"], "sha256": document["source_sha256"],
                       "size": size, "sprites": document.get("source_parts", document.get("sprites", []))}]
    else:
        if (document.get("schema_version") != 1 or document.get("asset_id") != asset_id
                or document.get("path") != authoring["native_atlas"] or document.get("source_coverage_complete") is not True):
            # Ground producers publish a single verified atlas directly. Only
            # that explicit schema/identity/completion contract is accepted;
            # an arbitrary report without an atlas list is not a fallback.
            raise ValueError(asset_id + ": invalid verified single-atlas report " + report_path)
        candidates = [document]
        # These four producer reports bind the pixel digest in public source,
        # not in the report itself. Their exact explicit completion/count/path
        # contract remains mandatory; no other family may borrow this form.
        if asset_id in SOURCE_BINDINGS:
            if document.get("pose_count") != len(required_keys(family)):
                raise ValueError(asset_id + ": source-owned utility/control coverage changed")
            candidates = [{**document, "sha256": authoring["native_sha256"]}]
    if len(candidates) != 1:
        raise ValueError(asset_id + ": missing/ambiguous authoritative atlas " + report_path)
    atlas = candidates[0]
    path = base.repository_path(atlas["path"])
    if atlas["sha256"] != authoring["native_sha256"] or base.digest(path) != atlas["sha256"]:
        # Source binding, report and actual file must identify the same selected
        # pixels; a later manual edit cannot bypass deterministic source authoring.
        raise ValueError(asset_id + ": stale source atlas " + atlas["path"])
    if atlas["path"] not in images:
        image = Image.open(path)
        validate_source_image(image, asset_id, family.get("company_colors", "none"))
        images[atlas["path"]] = image
    image = images[atlas["path"]]
    if list(image.size) != atlas["size"]:
        raise ValueError(asset_id + ": source atlas dimensions changed")
    poses = {}
    # Supplemental survey sheets contain several independent public families.
    # Filter by explicit per-pose identity; an atlas neighbour is not coverage.
    for pose in atlas["sprites"]:
        if pose.get("asset_id", atlas.get("asset_id")) != asset_id:
            continue
        # Purchase sources declare a shared UI anchor on their atlas. Inherit
        # that explicit geometry only when a pose has no individual anchor;
        # a missing declaration in both places still fails normal validation.
        pose = {**pose, "anchor": pose.get("anchor", atlas.get("anchor"))}
        key = (pose["state"], pose["frame"], pose["direction"], pose.get("tile_variant", 0), pose.get("part", "body"))
        if key in poses:
            raise ValueError(asset_id + ": duplicate source pose " + str(key))
        x, y, width, height = pose["box"]
        if [width, height] != family["native_size"] or pose["anchor"] != family["anchor"]:
            # Native cell size/anchor comes from the physical/UI consumer,
            # regardless of larger contact-sheet placement or crop margins.
            raise ValueError(asset_id + ": source native geometry changed")
        if not (0 <= x < x + width <= image.width and 0 <= y < y + height <= image.height):
            raise ValueError(asset_id + ": source cell leaves atlas")
        cell = image.crop((x, y, x + width, y + height))
        validate_company_mask(family, pose, cell)
        absent = absent_part(family, pose)
        # Empty roof/glass or through-stop parents are legitimate only where
        # actual native geometry says they do not exist. Missing physical art
        # cannot be hidden by relabelling an empty source rectangle.
        if bool(pose.get("structurally_absent", absent)) != absent:
            raise ValueError(asset_id + ": structural absence differs from native geometry")
        # Hidden natural vents intentionally contain no pixels, preserving
        # survey/geology disclosure. That declared visibility state is distinct
        # from an absent physical station/riser parent.
        empty_expected = absent or pose["state"] in family.get("empty_allowed_states", [])
        if (cell.getbbox() is None) != empty_expected:
            raise ValueError(asset_id + ": empty source physical part")
        expected_hash = pose.get("source_pixels_sha256")
        if expected_hash is not None and expected_hash != hashlib.sha256(cell.tobytes()).hexdigest():
            raise ValueError(asset_id + ": source cell pixel identity changed")
        pose["packed_anchor"] = physical_anchor(family, pose)
        poses[key] = pose
    if set(poses) != set(required_keys(family)) or authoring["staged_pose_count"] != len(poses):
        # Complete Cartesian identity coverage, not merely a matching count,
        # prevents repeated directions or another tile from hiding absent art.
        missing = set(required_keys(family)) - set(poses)
        raise ValueError(asset_id + ": incomplete source coverage " + str(sorted(missing)))
    return poses, image


def cropped_pose(image, pose):
    """Crop only transparent margins while preserving the original world anchor.

    @param image Verified indexed native source atlas.
    @param pose Explicit source cell box and physical/UI anchor.
    @return Cropped palette indices, width, height and adjusted relative x/y.
    """
    x, y, width, height = pose["box"]
    cell = image.crop((x, y, x + width, y + height))
    return crop_and_shift(list(cell.get_flattened_data()), width, height, *pose.get("packed_anchor", pose["anchor"]))


def shelf_sheet(sprites, width, gap):
    """Pack native poses on bounded rows without changing their canonical order.

    @param sprites Cropped palette indices/width/height/relative-anchor tuples.
    @param width Maximum packed sheet width, within native NFO coordinate bounds.
    @param gap Transparent separation between neighbouring packed rectangles.
    @return Indexed DOS sheet and each sprite's sheet x/y origin.
    """
    positions, x, y, row_height = [], 0, 0, 0
    # A single horizontal strip would exceed 16-bit sprite-sheet coordinates for
    # the complete M1 set. Stable shelves preserve order within a bounded PNG.
    for _pixels, sprite_width, sprite_height, _rx, _ry in sprites:
        if sprite_width > width:
            raise ValueError("sprite exceeds packed sheet width")
        if x + sprite_width > width:
            y += row_height + gap
            x, row_height = 0, 0
        positions.append((x, y))
        x += sprite_width + gap
        row_height = max(row_height, sprite_height)
    height = y + row_height
    if height > 65535:
        raise ValueError("packed M1 sheet exceeds native coordinate range")
    sheet = Image.new("P", (width, height), 0)
    sheet.putpalette(base.PALETTE_BYTES)
    for (indices, sw, sh, _rx, _ry), origin in zip(sprites, positions):
        cell = Image.new("P", (sw, sh), 0)
        cell.putdata(indices)
        sheet.paste(cell, origin)
    return sheet, positions


def extended_byte(value):
    """Encode the Action 5 unsigned byte/word count convention explicitly.

    @param value Sprite count/offset in the uint16 loader range.
    @return NFO hex bytes, using the FF sentinel before a little-endian word.
    """
    if not 0 <= value <= 65535:
        raise ValueError("Action 5 value leaves uint16 range")
    return f"{value:02X}" if value < 255 else f"FF {value & 255:02X} {value >> 8:02X}"


def format_nfo(entries, sprites, positions):
    """Emit only delivered contiguous native ranges; leave every gap unactivated.

    @param entries Sprite identities with their published native offsets.
    @param sprites Cropped sprite pixel/bounds/anchor tuples in native offset order.
    @param positions Matching source-sheet origins in stable packed order.
    @return UTF-8 NFO bytes including each true Action 5 type 1C offset block.
    """
    lines = ["// @file map_layers_m1.nfo Generated selected M1 source sprites.",
             "// @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/79C",
             "// @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md", ""]
    index = 0
    # Separate blocks skip both reserved 66..73 and undelivered future core
    # families. No transparent placeholder is counted as a delivered sprite.
    while index < len(entries):
        end = index + 1
        while end < len(entries) and entries[end]["native_offset"] == entries[end - 1]["native_offset"] + 1:
            # Only consecutive published IDs share one loader replacement block.
            end += 1
        count, offset = end - index, entries[index]["native_offset"]
        lines.extend(["//@@WARNING DISABLE 44",
                      f"   -1 * 0 05 9C {extended_byte(count)} {extended_byte(offset)} // {count} sprites at native offset {offset}",
                      "//@@WARNING ENABLE 44"])
        for position in range(index, end):
            sx, sy = positions[position]
            _pixels, sw, sh, rx, ry = sprites[position]
            lines.append(f"   -1 sprites/map_layers_m1.png 8bpp {sx} {sy} {sw} {sh} {rx} {ry} normal")
        lines.append("")
        index = end
    return ("\n".join(lines) + "\n").encode("utf-8")


def cpp_name(asset_id):
    """Translate a stable published family name to its generated C++ identifier.

    @param asset_id Public family identity containing dots, hyphens and digits.
    @return Unambiguous PascalCase generated enum member.
    """
    return "".join(word[:1].upper() + word[1:] for word in re.split(r"[.-]", asset_id))


def format_header(ranges, bound, external):
    """Generate shared native range declarations from the one packing authority.

    @param ranges Ordered family ranges matching the public sprite contract.
    @param bound Exclusive native sprite bound after all selected appended art.
    @param external Existing independently published native ranges that new art must preserve.
    @return UTF-8 C++ declarations with plan, type and field documentation.
    """
    lines = ["/**", " * @file multilayer_sprite_assets.h Generated M1 native graphics ranges; do not edit.",
             " * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/81B/79C",
             " * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md", " */",
             "#ifndef MULTILAYER_SPRITE_ASSETS_H", "#define MULTILAYER_SPRITE_ASSETS_H", "", "#include <array>", "#include <cstdint>", "",
             "/** Selected M1 graphics identities; saved engine/cargo/industry IDs stay with their existing owners. */",
             "enum class MultiLayerSpriteAsset : uint8_t {"]
    for row in ranges:
        lines.append(f"\t{cpp_name(row['asset_id'])}, ///< {row['asset_id']}.")
    lines.extend(["\tEnd, ///< Number of selected family declarations.", "};", "",
                  "/** One complete native family range. Pose order is state/frame/direction/tile/part. */",
                  "struct MultiLayerSpriteRange {", "\tuint16_t offset; ///< Relative native Action5 type1C offset.",
                  "\tuint16_t count; ///< Exact physical source pose count.", "\tuint8_t states; ///< Public state count.",
                  "\tuint8_t frames; ///< Authored visual phase/texture count.", "\tuint8_t directions; ///< Engine view count.",
                  "\tuint16_t tiles; ///< Physical tile variants or independent portal endpoints.",
                  "\tuint8_t parts; ///< Physical back/front or vehicle part count.", "};", "",
                  f"static constexpr uint16_t MULTILAYER_M1_SPRITE_COUNT = {bound}; ///< Exclusive selected native block bound, including preserved gaps.",
                  f"static constexpr uint16_t MULTILAYER_PRESERVED_EXTERNAL_SPRITE_COUNT = {sum(row['sprite_count'] for row in external)}; ///< Independently published sprites inside the selected block.",
                  f"static constexpr uint16_t MULTILAYER_SPACE_PREVIEW_OFFSET = {next(row['native_offset'] for row in external if row['id'] == 'space-preview')}; ///< Stable original Space preview start.",
                  f"static constexpr std::array<MultiLayerSpriteRange, {len(ranges)}> _multilayer_m1_sprite_ranges = {{{{"])
    for row in ranges:
        values = [row["native_offset"], row["sprite_count"], len(row["states"]), row["frames"], len(row["directions"]), row["tile_variants"], len(row["parts"])]
        lines.append("\t{" + ", ".join(map(str, values)) + "}, // " + row["asset_id"])
    lines.extend(["}};", "", "#endif /* MULTILAYER_SPRITE_ASSETS_H */", ""])
    return "\n".join(lines).encode("utf-8")


def derive():
    """Validate selected source handoffs and prepare exact native payload inputs.

    @return Packed source metadata and output-path/byte mapping; no files written.
    @throws ValueError If source coverage, tool versions or native references fail.
    """
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    if PIL.__version__ != scope["toolchain"]["pillow"] or sys.version.split()[0] != scope["toolchain"]["python"]:
        # Encoders/serializers are pinned so clean conversions can be compared
        # byte-for-byte without a silent toolchain-dependent source revision.
        raise ValueError("M1 conversion toolchain differs from pinned versions")
    legacy = public["legacy"]
    if base.digest(base.repository_path(legacy["reference_sheet"])) != legacy["sha256"]:
        raise ValueError("protected M0 source sheet changed")
    # Independently shipped preview art retains exact source bytes and IDs;
    # changing its payload is not authorized by this selected M1 expansion.
    for row in scope.get("preserved_external_ranges", []):
        for name, digest in row["source_sha256"].items():
            if base.digest(base.repository_path(name)) != digest:
                raise ValueError("preserved external source changed: " + name)
    ranges, bound = selected_ranges(public, scope)
    families = {row["asset_id"]: row for row in public["assets"]}
    documents, images, combined = {}, {}, []
    # Every selected family contributes its exact authoritative source poses.
    # Pure conversion neither advances a work owner nor activates native assets.
    for row in ranges:
        family = families[row["asset_id"]]
        poses, image = source_poses(family, documents, images)
        for index, key in enumerate(required_keys(family)):
            pose = poses[key]
            entry = {"asset_id": row["asset_id"], "native_offset": row["native_offset"] + index,
                     "state": key[0], "frame": key[1], "direction": key[2], "tile_variant": key[3], "part": key[4],
                     "source_atlas": source_binding(family)["native_atlas"], "source_box": pose["box"], "source_anchor": pose["anchor"],
                     "structurally_absent": absent_part(family, pose)}
            combined.append((entry, cropped_pose(image, pose)))
    combined.sort(key=lambda pair: pair[0]["native_offset"])
    entries, sprites = map(list, zip(*combined))
    sheet, positions = shelf_sheet(sprites, scope["sheet_width"], scope["sprite_gap"])
    external = scope.get("preserved_external_ranges", [])
    png, nfo, header = png_bytes(sheet), format_nfo(entries, sprites, positions), format_header(ranges, bound, external)
    for entry, sprite, origin in zip(entries, sprites, positions):
        _indices, width, height, rx, ry = sprite
        entry.update(box=[*origin, width, height], anchor=[rx, ry])
    report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/79C; selected native base-set art, runtime pending",
              "schema_version": 1, "route": scope["native_route"], "scope": scope["asset_ids"], "toolchain": scope["toolchain"],
              "preserved_external_ranges": external, "preserved_external_sprite_count": sum(row["sprite_count"] for row in external),
              "legacy": legacy, "native_sprite_bound": bound, "pose_count": len(entries), "families": ranges, "sprites": entries,
              "png": {"path": scope["png"], "sha256": hashlib.sha256(png).hexdigest(), "size": list(sheet.size)},
              "nfo": {"path": scope["nfo"], "sha256": hashlib.sha256(nfo).hexdigest()},
              "header": {"path": scope["header"], "sha256": hashlib.sha256(header).hexdigest()},
              "packed_grf": None, "runtime_activated": False}
    files = {base.repository_path(scope["png"]): png, base.repository_path(scope["nfo"]): nfo,
             base.repository_path(scope["header"]): header}
    return report, files


def publish(report, files):
    """Publish complete validated packing inputs without claiming GRF loading.

    @param report Exact selected native ranges/crops and source-derived hashes.
    @param files Encoded separate PNG/NFO/generated-header repository outputs.
    @return None; write only selected packing artifacts, not the cargo package.
    """
    for path, encoded in files.items():
        path.write_bytes(encoded)
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    base.repository_path(scope["report"]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    rows = {row["asset_id"]: row for row in report["families"]}
    # Definition namespace/local offsets persist. The explicit art route binds
    # native base-set pixels without changing saved MLVI package identities.
    for family in public["assets"]:
        if family["asset_id"] not in rows:
            continue
        row = rows[family["asset_id"]]
        family["packed"].update(route=report["route"], native_offset=row["native_offset"], sprite_count=row["sprite_count"],
                                source_payload=scope["report"], sha256=None)
    PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def finalise_base_terrain():
    """Reconcile historical partial flags only after exact base/detail validation.

    @return None; publish verified terrain source flags without altering pixels.
    @throws ValueError If preserved originals, base cells or edge/survey coverage fail.
    """
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in public["assets"]}
    documents, images = {}, {}
    targets = []
    # Original base sheets already contain twelve real state/texture cells per
    # depth. Their earlier partial flags also covered then-missing edge/survey
    # art; verify both delivered contracts before completing those old flags.
    for depth, resources in [("underground", ["COAL", "IORE", "CORE", "BAUX", "SALT"]),
                             ("deep", ["LITH", "NORE", "COBT", "RARE", "URAN"])]:
        family = families["terrain." + depth]
        if base.digest(base.repository_path(family["source"]["path"])) != family["source"]["sha256"]:
            raise ValueError("immutable base terrain original changed")
        candidate = copy.deepcopy(family)
        candidate["authoring"]["coverage_complete"] = True
        # A temporary read-only validation copy permits checking the producer's
        # old staged cells. Public completion is published only after every
        # actual base/detail pose passes the normal source validator below.
        source_poses(candidate, documents, images)
        source_poses(families["terrain.edges." + depth], documents, images)
        for resource in resources:
            source_poses(families["survey." + resource], documents, images)
        targets.append(family)
    for family in targets:
        family["source"]["status"] = "authored"
        family["authoring"].update(coverage_complete=True, status="source_coverage_verified_runtime_pending")
    PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    print("Base terrain source handoff: 24 preserved state/texture poses plus 62 edge/survey poses verified; historical partial flags reconciled, runtime pending")


def bind_packed_payload(publish_source=True):
    """Bind selected native ranges only to a reproducibly built, actually loaded GRF.

    @param publish_source Publish the public source binding only for an explicit authoring operation.
    @return None; publish actual payload identity while leaving runtime integration pending.
    @throws ValueError If source inputs, build proof, container checksum or actual loader evidence is stale.
    """
    report, files = derive()
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    path = base.ROOT / "logs/proofs/mlm/native-art/packed-build-proof.json"
    proof = json.loads(path.read_text(encoding="utf-8"))
    grf = base.ROOT / "media/baseset/openttd.grf"
    sha = base.digest(grf)
    reproducible = proof["reproducible"]
    cached = proof["generator_free"]
    # One successful build cannot bind stale source inputs or an untested cached
    # distribution. Both clean payloads and the real generator-free load must
    # fingerprint the same bytes before any public packed identity is published.
    if reproducible["sha256"] != [sha, sha] or cached["sha256"] != sha or not cached["generator_discovery_disabled"]:
        raise ValueError("selected packed payload build/load evidence is stale")
    for output, encoded in files.items():
        digest = hashlib.sha256(encoded).hexdigest()
        relative = str(output.relative_to(base.ROOT)).replace("\\", "/")
        if output.read_bytes() != encoded or reproducible["source_outputs"].get(relative) != digest:
            # Exact source byte comparisons refuse source-only edits after the
            # last actual GRF build, including anchor or native range changes.
            raise ValueError("selected packed source differs from actual build: " + relative)
    checksum = (base.ROOT / "media/baseset/openttd.grf.hash").read_text().strip()
    if reproducible["grfid_md5"] != [checksum, checksum] or not re.fullmatch("[0-9a-f]{32}", checksum):
        # GRF container-v2 metadata uses the GRFID checksum, not whole-file MD5.
        raise ValueError("selected GRFID checksum differs from verified payload")
    loaded = base.repository_path(cached["loader_log"]).read_text(encoding="utf-8", errors="replace")
    external = report["preserved_external_sprite_count"]
    marker = loader_marker(report)
    if marker not in loaded or "All tests passed" not in loaded:
        # A marker printed before a failed assertion cannot close actual loading.
        raise ValueError("selected packed artwork has no successful native loader evidence")
    report["packed_grf"] = {"path": "media/baseset/openttd.grf", "sha256": sha, "grfid_md5": checksum,
                            "selected_sprites": report["pose_count"], "protected_sprites": 66, "preserved_external_sprites": external,
                            "native_bound": report["native_sprite_bound"], "proof": str(path.relative_to(base.ROOT)).replace("\\", "/"),
                            "loader_log": cached["loader_log"]}
    base.repository_path(scope["report"]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    rows = {row["asset_id"]: row for row in report["families"]}
    # Only selected native graphics receive the GRF identity. Definition-local
    # namespace/offset and the immutable cargo package retain their existing owners.
    for family in public["assets"]:
        if family["asset_id"] not in rows:
            continue
        row = rows[family["asset_id"]]
        family["packed"].update(route=report["route"], native_offset=row["native_offset"], sprite_count=row["sprite_count"],
                                source_payload=scope["report"], payload="media/baseset/openttd.grf", sha256=sha)
    if publish_source:
        PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    print(f"Selected M1 packed handoff: {len(report['families'])} families / {report['pose_count']} actual sprites, source-exact cached loading and immutable M0 verified; runtime consumers pending")


def main():
    """Describe the layout or bake complete verified native source packing inputs.

    @return None; print actual staged counts or reject unverified source producers.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--describe", action="store_true", help="read-only layout preparation; no source validation or publication")
    parser.add_argument("--finalise-base-terrain", action="store_true", help="validate preserved base/detail cells and reconcile old source flags")
    parser.add_argument("--bind-packed", action="store_true", help="bind only reproducibly built and actually loaded native artwork")
    args = parser.parse_args()
    if args.bind_packed:
        bind_packed_payload()
        return
    if args.finalise_base_terrain:
        finalise_base_terrain()
        return
    if args.describe:
        scope = json.loads(SCOPE.read_text(encoding="utf-8"))
        public = json.loads(PUBLIC.read_text(encoding="utf-8"))
        ranges, bound = selected_ranges(public, scope)
        pending = []
        # This read-only description respects both published producer handoff
        # forms; utility/control source flags are not missing authoring work.
        for family in public["assets"]:
            if family["asset_id"] not in scope["asset_ids"]:
                continue
            try:
                source_binding(family)
            except ValueError:
                pending.append(family["asset_id"])
        print(f"M1 planned native layout: {len(ranges)} families / {sum(row['sprite_count'] for row in ranges)} source sprites / exclusive bound {bound}; no source/payload checks executed")
        print("Source verification pending: " + ", ".join(pending))
        return
    report, files = derive()
    publish(report, files)
    print(f"M1 packing inputs staged: {len(report['families'])} families / {report['pose_count']} native sprites; actual GRF packing/loading remains pending")


if __name__ == "__main__":
    main()
