"""Pack the selected M1 source set into independent native Action 5 segments.

@file pack_m1_art.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 79B/80B/79C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoints 8.6/10.4B
Existing M0 PNG/NFO and MLVI definition bytes are never rewritten. Source
coverage must be verified before publication; --describe is layout preparation.
"""

import argparse
import copy
import hashlib
import itertools
import json
from pathlib import Path
import re
import sys

import PIL
from PIL import Image

import convert_m1_art as base
from convert_terrain_art import png_bytes
from sprite_quantize import crop_and_shift


SCOPE = base.ROOT / "media/multilayer/m1-pack-scope.json"
PUBLIC = base.PUBLIC


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
    cursor = max([scope["appended_start"]] + [row["native_offset"] + row["sprite_count"] for row in preserved])
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


def validate_source_image(image, asset_id):
    """Enforce native DOS colors and alpha on independently supplied source pixels.

    @param image Indexed source atlas or a bounded in-memory corruption fixture.
    @param asset_id Public family identity included in actionable error messages.
    @return None; reject a different palette, alpha policy or reserved color.
    """
    if image.mode != "P" or image.info.get("transparency") != 0 or image.getpalette() != base.PALETTE_BYTES:
        # The whole native art route shares one DOS palette; changing that table
        # changes color meaning even when sprite geometry and indices are equal.
        raise ValueError(asset_id + ": source palette/transparency mismatch")
    if set(image.get_flattened_data()) & (base.FORBIDDEN - {0}):
        raise ValueError(asset_id + ": reserved source palette entry")


def source_poses(family, documents, images):
    """Resolve a family's authoritative source poses and enforce producer coverage.

    @param family Published source/authoring geometry and authoritative atlas binding.
    @param documents Cache of decoded source-atlas reports by repository path.
    @param images Cache of verified DOS source images by repository path.
    @return Identity-keyed source-pose records and the immutable indexed source image.
    @throws ValueError If the source is unverified, stale or missing exact coverage.
    """
    asset_id = family["asset_id"]
    authoring = family.get("authoring", {})
    if not authoring.get("coverage_complete", False):
        raise ValueError(asset_id + ": source verification pending; finish its producer before packing")
    report_path = authoring["atlas_manifest"]
    if report_path not in documents:
        documents[report_path] = json.loads(base.repository_path(report_path).read_text(encoding="utf-8"))
    document = documents[report_path]
    if "atlases" in document:
        candidates = [row for row in document["atlases"] if row["path"] == authoring["native_atlas"]]
    else:
        if (document.get("schema_version") != 1 or document.get("asset_id") != asset_id
                or document.get("path") != authoring["native_atlas"] or document.get("source_coverage_complete") is not True):
            # Ground producers publish a single verified atlas directly. Only
            # that explicit schema/identity/completion contract is accepted;
            # an arbitrary report without an atlas list is not a fallback.
            raise ValueError(asset_id + ": invalid verified single-atlas report " + report_path)
        candidates = [document]
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
        validate_source_image(image, asset_id)
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
        if image.crop((x, y, x + width, y + height)).getbbox() is None:
            raise ValueError(asset_id + ": empty source physical part")
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
    return crop_and_shift(list(cell.get_flattened_data()), width, height, *pose["anchor"])


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
    # Place each verified native sprite at its recorded shelf position, retaining
    # exact palette indices and transparent holes rather than resampling cells.
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
        # Consecutive replacement IDs retain their own geometry and sheet crops.
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


def format_header(ranges, bound):
    """Generate shared native range declarations from the one packing authority.

    @param ranges Ordered family ranges matching the public sprite contract.
    @param bound Exclusive native sprite bound after all selected appended art.
    @return UTF-8 C++ declarations with plan, type and field documentation.
    """
    lines = ["/**", " * @file multilayer_sprite_assets.h Generated M1 native graphics ranges; do not edit.",
             " * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/81B/79C",
             " * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md", " */",
             "#ifndef MULTILAYER_SPRITE_ASSETS_H", "#define MULTILAYER_SPRITE_ASSETS_H", "", "#include <array>", "#include <cstdint>", "",
             "/** Selected M1 graphics identities; saved engine/cargo/industry IDs stay with their existing owners. */",
             "enum class MultiLayerSpriteAsset : uint8_t {"]
    # Generated enum identities follow existing public range order so native
    # consumers keep their selected family identities across regeneration.
    for row in ranges:
        lines.append(f"\t{cpp_name(row['asset_id'])}, ///< {row['asset_id']}.")
    lines.extend(["\tEnd, ///< Number of selected family declarations.", "};", "",
                  "/** One complete native family range. Pose order is state/frame/direction/tile/part. */",
                  "struct MultiLayerSpriteRange {", "\tuint16_t offset; ///< Relative native Action5 type1C offset.",
                  "\tuint16_t count; ///< Exact physical source pose count.", "\tuint8_t states; ///< Public state count.",
                  "\tuint8_t frames; ///< Authored visual phase/texture count.", "\tuint8_t directions; ///< Engine view count.",
                  "\tuint16_t tiles; ///< Physical tile variants or independent portal endpoints.",
                  "\tuint8_t parts; ///< Physical back/front or vehicle part count.", "};", "",
                  f"static constexpr uint16_t MULTILAYER_M1_SPRITE_COUNT = {bound}; ///< Exclusive selected native block bound.",
                  f"static constexpr std::array<MultiLayerSpriteRange, {len(ranges)}> _multilayer_m1_sprite_ranges = {{{{"])
    # Range geometry is emitted from the same existing public contract as the
    # enum; source conversion cannot allocate another native identity namespace.
    for row in ranges:
        values = [row["native_offset"], row["sprite_count"], len(row["states"]), row["frames"], len(row["directions"]), row["tile_variants"], len(row["parts"])]
        lines.append("\t{" + ", ".join(map(str, values)) + "}, // " + row["asset_id"])
    lines.extend(["}};", "", "#endif /* MULTILAYER_SPRITE_ASSETS_H */", ""])
    return "\n".join(lines).encode("utf-8")


def selected_sources(public, scope, context, manifests):
    """Relocate selected source reads while preserving the original approved identities.

    @param public In-memory canonical public manifest; disk bytes remain unchanged.
    @param scope Existing selected native family/range contract.
    @param context Exact recipe invocation supplying every generated source.
    @param manifests Original manifest aliases mapped to current path/SHA256 records.
    @return Parsed current source reports keyed by their selected repository paths.
    """
    from evidence import identity
    from artifact_paths import contained, identifier
    if context.root.resolve() != base.ROOT:
        raise ValueError("selected packing belongs to another source checkout")
    identifier(context.run_id)
    families = {row["asset_id"]: row for row in public["assets"]}
    required = {families[name]["authoring"]["atlas_manifest"] for name in scope["asset_ids"]}
    if set(manifests) != required:
        raise ValueError("selected M1 source manifests are incomplete or ambiguous")
    documents, aliases = {}, {}
    # Resolve every declared source once by its exact run/role/hash. Old alias
    # strings identify canonical bindings; they are never read from old caches.
    for alias, reference in manifests.items():
        path = contained(base.ROOT / reference["path"], base.ROOT / "mlm/reviews")
        parts = path.relative_to(base.ROOT / "mlm/reviews").parts
        # A correct hash at an earlier run is insufficient: the selected source
        # producer must supply a report in this exact recipe invocation.
        if len(parts) != 5 or parts[2] != context.run_id or parts[3] != "reports":
            raise ValueError("source manifest has a foreign invocation or storage role")
        if identity(path) != reference:
            raise ValueError("selected source manifest changed")
        documents[reference["path"]] = json.loads(path.read_text(encoding="utf-8"))
        aliases[alias] = reference["path"]
    # Only source report/atlas locations may change. Canonical coverage, native
    # sizes, anchors, approved pixel hashes and published ranges stay authoritative.
    for name in scope["asset_ids"]:
        authoring = families[name]["authoring"]
        selected = aliases[authoring["atlas_manifest"]]
        document = documents[selected]
        candidates = document.get("atlases", [document])
        matches = [row for row in candidates if Path(row["path"]).name == Path(authoring["native_atlas"]).name]
        if len(matches) != 1:
            raise ValueError("selected source atlas is missing/ambiguous: " + name)
        atlas = matches[0]
        image = contained(base.ROOT / atlas["path"], base.ROOT / "mlm/reviews")
        parts = image.relative_to(base.ROOT / "mlm/reviews").parts
        # Source candidates remain images from the same run, independently of
        # packing data and any similarly named historical authoring output.
        if len(parts) != 5 or parts[2] != context.run_id or parts[3] != "images":
            raise ValueError("selected atlas has a foreign invocation or storage role")
        if atlas["sha256"] != authoring["native_sha256"]:
            raise ValueError("selected source differs from approved original pixels: " + name)
        authoring.update(atlas_manifest=selected, native_atlas=atlas["path"])
    return documents


def derive(source_manifests=None, context=None):
    """Validate selected source handoffs and prepare exact native payload inputs.

    @param source_manifests Exact original aliases to current source report identities.
    @param context Explicit run for selected sources and disposable packing outputs.
    @return Packed source metadata and output-path/byte mapping; no files written.
    @throws ValueError If source coverage, tool versions or native references fail.
    """
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    # Selected regeneration needs the whole explicit source closure. Neither a
    # context alone nor a partial source map may fall back to an old proof cache.
    if (source_manifests is None) != (context is None):
        raise ValueError("selected packing requires both a context and all source manifests")
    if PIL.__version__ != scope["toolchain"]["pillow"] or sys.version.split()[0] != scope["toolchain"]["python"]:
        # Encoders/serializers are pinned so clean conversions can be compared
        # byte-for-byte without a silent toolchain-dependent source revision.
        raise ValueError("M1 conversion toolchain differs from pinned versions")
    legacy = public["legacy"]
    if base.digest(base.repository_path(legacy["reference_sheet"])) != legacy["sha256"]:
        raise ValueError("protected M0 source sheet changed")
    ranges, bound = selected_ranges(public, scope)
    families = {row["asset_id"]: row for row in public["assets"]}
    documents = selected_sources(public, scope, context, source_manifests) if context is not None else {}
    images, combined = {}, []
    # Every selected family contributes its exact authoritative source poses.
    # Pure conversion neither advances a work owner nor activates native assets.
    for row in ranges:
        family = families[row["asset_id"]]
        poses, image = source_poses(family, documents, images)
        # Every declared state/frame/view/part retains its existing native offset.
        for index, key in enumerate(required_keys(family)):
            pose = poses[key]
            entry = {"asset_id": row["asset_id"], "native_offset": row["native_offset"] + index,
                     "state": key[0], "frame": key[1], "direction": key[2], "tile_variant": key[3], "part": key[4],
                     "source_atlas": family["authoring"]["native_atlas"], "source_box": pose["box"], "source_anchor": pose["anchor"]}
            combined.append((entry, cropped_pose(image, pose)))
    combined.sort(key=lambda pair: pair[0]["native_offset"])
    entries, sprites = map(list, zip(*combined))
    sheet, positions = shelf_sheet(sprites, scope["sheet_width"], scope["sprite_gap"])
    png, nfo, header = png_bytes(sheet), format_nfo(entries, sprites, positions), format_header(ranges, bound)
    # Report the actual packed crop/anchor for every emitted native identity.
    for entry, sprite, origin in zip(entries, sprites, positions):
        _indices, width, height, rx, ry = sprite
        entry.update(box=[*origin, width, height], anchor=[rx, ry])
    report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/79C; selected native base-set art, runtime pending",
              "schema_version": 1, "route": scope["native_route"], "scope": scope["asset_ids"], "toolchain": scope["toolchain"],
              "legacy": legacy, "native_sprite_bound": bound, "pose_count": len(entries), "families": ranges, "sprites": entries,
              "png": {"path": scope["png"], "sha256": hashlib.sha256(png).hexdigest(), "size": list(sheet.size)},
              "nfo": {"path": scope["nfo"], "sha256": hashlib.sha256(nfo).hexdigest()},
              "header": {"path": scope["header"], "sha256": hashlib.sha256(header).hexdigest()},
              "packed_grf": None, "runtime_activated": False}
    files = {base.repository_path(scope["png"]): png, base.repository_path(scope["nfo"]): nfo,
             base.repository_path(scope["header"]): header}
    if context is not None:
        files = {}
        # Candidate payloads retain their shipping basenames and exact bytes,
        # while writing them can affect only the selected generated data owner.
        for role, encoded in (("png", png), ("nfo", nfo), ("header", header)):
            path = context.output("data", Path(scope[role]).name)
            report[role]["path"] = path.relative_to(base.ROOT).as_posix()
            files[path] = encoded
    return report, files


def publish(report, files):
    """Publish complete validated packing inputs without claiming GRF loading.

    @param report Exact selected native ranges/crops and source-derived hashes.
    @param files Encoded separate PNG/NFO/generated-header repository outputs.
    @return None; write only selected packing artifacts, not the cargo package.
    """
    # This existing explicit publisher writes the validated canonical inputs;
    # selected regression recipes call derive and never enter this function.
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
    # Only independently checked terrain/survey originals gain source completion;
    # this authoring action still grants no native runtime or human acceptance.
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
    marker = f"M1_PACKED_ART_LOADED sprites={66 + report['pose_count']} native_bound={report['native_sprite_bound']} protected=66 selected={report['pose_count']} pixel_hashes=exact"
    if marker not in loaded or "All tests passed" not in loaded:
        # A marker printed before a failed assertion cannot close actual loading.
        raise ValueError("selected packed artwork has no successful native loader evidence")
    report["packed_grf"] = {"path": "media/baseset/openttd.grf", "sha256": sha, "grfid_md5": checksum,
                            "selected_sprites": report["pose_count"], "protected_sprites": 66,
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
        pending = [row["asset_id"] for row in public["assets"] if row["asset_id"] in scope["asset_ids"] and not row.get("authoring", {}).get("coverage_complete", False)]
        print(f"M1 planned native layout: {len(ranges)} families / {sum(row['sprite_count'] for row in ranges)} source sprites / exclusive bound {bound}; no source/payload checks executed")
        print("Source verification pending: " + ", ".join(pending))
        return
    report, files = derive()
    publish(report, files)
    print(f"M1 packing inputs staged: {len(report['families'])} families / {report['pose_count']} native sprites; actual GRF packing/loading remains pending")


if __name__ == "__main__":
    main()
