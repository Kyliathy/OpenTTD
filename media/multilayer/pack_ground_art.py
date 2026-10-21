"""Prepare deterministic selected ground sprites after the complete published native prefix.

@file pack_ground_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCCB native station offsets.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.4D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only new Ground PNG/NFO/header inputs are staged here. Actual GRF decoding,
checksum publication, native consumers and game delivery remain separate gates.
"""

import argparse
import hashlib
import io
import json

from PIL import PngImagePlugin

import pack_m1_art as packing
from tools import ground_art_handoff as handoff


ROOT, PUBLIC, SCOPE = handoff.ROOT, handoff.PUBLIC, handoff.SCOPE


def format_header(ranges, start, bound):
    """Generate independent typed range declarations without editing any older header.

    @param ranges Exact canonical47-family append order and matrix cardinalities.
    @param start Current complete native prefix end, including Space and launch art.
    @param bound Exclusive complete Ground append end.
    @return Deterministic self-contained C++ header bytes with owning-plan documentation.
    """
    lines = ["/** @file ground_sprite_assets.h Generated selected ground artwork ranges.",
             " * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.",
             " * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.", " */", "",
             "#ifndef GROUND_SPRITE_ASSETS_H", "#define GROUND_SPRITE_ASSETS_H", "", "#include <array>",
             "#include <cstdint>", '#include "launch_fleet_sprite_assets.h"', "",
             "/** Stable append-local family identities; all prior native enum values stay unchanged. */",
             "enum class GroundSpriteFamily : uint8_t {"]
    # This explicit order is immutable once published. Sorting public source
    # rows later cannot relocate a physical cell's native sprite identity.
    for index, row in enumerate(ranges):
        lines.append(f"\t{packing.cpp_name(row['asset_id'])} = {index}, ///< {row['asset_id']}.")
    lines.extend([f"\tEnd = {len(ranges)}, ///< Exclusive Ground family bound.", "};", "",
                  "/** Canonical state/frame/view/cell/parent dimensions for one native source family. */",
                  "struct GroundSpriteRange {", "\tuint16_t begin; ///< Absolute native sprite offset.",
                  "\tuint16_t count; ///< Complete Cartesian source pose count.", "\tuint8_t states; ///< Native state count.",
                  "\tuint8_t frames; ///< Source phase count; no simulation clock or work authority.",
                  "\tuint8_t directions; ///< Genuine declared source views.", "\tuint16_t tiles; ///< Actual native cell/variant count.",
                  "\tuint8_t parts; ///< Independently parented source component count.", "};", "",
                  f"static constexpr uint16_t GROUND_SPRITE_START = {start}; ///< Complete preserved prefix precedes this append.",
                  f"static constexpr uint16_t GROUND_SPRITE_END = {bound}; ///< Exclusive Ground sprite bound.",
                  'static_assert(GROUND_SPRITE_START == LAUNCH_FLEET_SPRITE_END, "Ground sprites must follow the complete native prefix");',
                  f"static constexpr std::array<GroundSpriteRange, {len(ranges)}> _ground_sprite_ranges = {{{{"])
    for row in ranges:
        values = [row["native_offset"], row["sprite_count"], len(row["states"]), row["frames"],
                  len(row["directions"]), row["tile_variants"], len(row["parts"])]
        lines.append("\t{" + ", ".join(map(str, values)) + "}, // " + row["asset_id"])
    lines.extend(["}};", "", "#endif /* GROUND_SPRITE_ASSETS_H */", ""])
    return "\n".join(lines).encode("utf-8")


def encoded_png(image):
    """Encode the indexed shelf sheet without recoloring, resampling or changing transparency.

    @param image Exact native palette indices and source-derived sprite placements.
    @return Deterministic PNG bytes with source-owner/architecture metadata.
    """
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md Group79D; archi-map-layers.md; packing inputs only")
    output = io.BytesIO()
    image.save(output, format="PNG", pnginfo=metadata, transparency=0, optimize=False, compress_level=9)
    return output.getvalue()


def native_pose(family, key, poses, image):
    """Pack source pixels at their actual native parent or relative-child reference.

    @param family Accepted public family, without changing its shared source canvas anchor.
    @param key Exact state/frame/view/cell/part source identity.
    @param poses Complete verified family source rows, including declared structural absence.
    @param image Unchanged indexed accepted source atlas.
    @return Native pixels, dimensions and image offset; relative glass retains its full roof reference.
    @throws ValueError If an installed facility part lacks an exact signed native offset or roof/glass contract.
    """
    pose = poses[key]
    # Most native consumers draw directly from the source canvas origin. The
    # two conventional facilities instead retain existing independent parent
    # origins and a relative roof child, requiring their producer's compensation.
    if family["asset_id"] not in ("facility.rail-station", "facility.road-terminal"):
        return packing.cropped_pose(image, pose)
    if pose["structurally_absent"]:
        return packing.cropped_pose(image, pose)
    anchor = pose.get("native_packed_anchor")
    # Native image offsets are signed16-bit coordinates. A missing, guessed
    # or malformed compensation must stop publication rather than move a part.
    if not isinstance(anchor, list) or len(anchor) != 2 or any(type(value) is not int or not -32768 <= value <= 32767 for value in anchor):
        raise ValueError("installed facility source lacks an exact signed native anchor")
    # Native relative children use the parent's actual sprite left/top after
    # cropping. Keep a live glass roof's full source canvas so its zero child
    # anchor still addresses that same reference; sorting bounds stay native.
    if family["asset_id"] == "facility.rail-station" and key[4] == "roof":
        glass = poses.get((*key[:4], "glass"))
        if glass is None:
            raise ValueError("native roof source lacks its relative glass contract")
        if not glass["structurally_absent"]:
            left, top, width, height = pose["box"]
            pixels = list(image.crop((left, top, left + width, top + height)).get_flattened_data())
            return pixels, width, height, *anchor
    return packing.cropped_pose(image, {**pose, "anchor": anchor})


def derive(public=None):
    """Convert only exact qualified native sources into new append inputs.

    @param public Optional detached public document for focused source/range refusal checks.
    @return Complete source-derived packing report and exact new PNG/NFO/header bytes.
    @throws ValueError On changed prefix, source geometry, palette, coverage or unauthorized absence.
    """
    scope, before, public = handoff.read_scope(public)
    handoff.validate_preserved_files(scope, before)
    families = {row["asset_id"]: row for row in public["assets"]}
    documents, images, ranges, entries, sprites = {}, {}, [], [], []
    # Every canonical source identity gets its own native slot, including
    # required static aliases and explicitly absent parents. Pixel cropping
    # preserves the original world anchor and approved company-mask indices.
    for reference in scope["source_rows"]:
        asset_id = reference["asset_id"]
        family = families[asset_id]
        poses, image = handoff.source_poses(family, scope, before, documents, images)
        ranges.append({"asset_id": asset_id, "native_offset": reference["native_offset"], "sprite_count": reference["sprite_count"],
                       **{key: family[key] for key in ("states", "frames", "directions", "tile_variants", "parts", "native_size", "anchor")}})
        for index, key in enumerate(packing.required_keys(family)):
            pose = poses[key]
            sprites.append(native_pose(family, key, poses, image))
            entries.append({"asset_id": asset_id, "native_offset": reference["native_offset"] + index,
                            "state": key[0], "frame": key[1], "direction": key[2], "tile_variant": key[3], "part": key[4],
                            "source_atlas": reference["source_atlas"], "source_box": pose["box"], "source_anchor": pose["anchor"],
                            "source_pixels_sha256": pose["source_pixels_sha256"], "structurally_absent": pose["structurally_absent"],
                            "company_color_pixels": pose["company_color_pixels"], "company_color_mask_sha256": pose["company_color_mask_sha256"],
                            "native_graphics": pose.get("native_graphics"), "native_track_bits": pose.get("native_track_bits"),
                            "world_cell": pose.get("world_cell"), "source_alias_of": pose.get("alias_of")})
    sheet, positions = packing.shelf_sheet(sprites, scope["sheet_width"], scope["sprite_gap"])
    png = encoded_png(sheet)
    nfo = packing.format_nfo(entries, sprites, positions).replace(b"map_layers_m1.png", b"ground_final.png")
    nfo = (b"// @see r2mlvs14-remaining-assets-and-runtime-integration.md Group79D; archi-map-layers.md.\n" + nfo)
    header = format_header(ranges, scope["native_start"], scope["native_end_exclusive"])
    # The retained report binds cropped bytes and anchors to the full original
    # native source. A real loader can compare every index, including holes,
    # company plaques and intentionally transparent physical parents.
    for entry, sprite, position in zip(entries, sprites, positions):
        indices, width, height, x, y = sprite
        entry.update(box=[*position, width, height], anchor=[x, y], pixel_sha256=hashlib.sha256(bytes(indices)).hexdigest())
    files = {handoff.bound(scope["png"]): png, handoff.bound(scope["nfo"]): nfo, handoff.bound(scope["header"]): header}
    report = {"_documentation": __doc__, "schema_version": 1, "route": scope["native_route"],
              "native_start": scope["native_start"], "native_sprite_bound": scope["native_end_exclusive"],
              "pose_count": len(entries), "families": ranges, "sprites": entries, "toolchain": scope["toolchain"],
              "scope": {"path": SCOPE.relative_to(ROOT).as_posix(), "sha256": handoff.digest(SCOPE)},
              "source_audit": scope["source_audit"], "preserved_family_count": 62,
              "structurally_absent_parts": sum(row["structurally_absent"] for row in entries),
              "company_color_pixels": sum(row["company_color_pixels"] for row in entries),
              "outputs": [{"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(data).hexdigest()}
                          for path, data in files.items()], "sheet_size": list(sheet.size),
              "packed_grf": None, "runtime_activated": False}
    return report, files


def main():
    """Prepare reviewable conversion bytes only under the repository test cache.

    @return None; shipped payload/public bindings and all existing files remain unchanged.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--derive", required=True, type=str, help="Repository test-cache directory for a clean conversion")
    args = parser.parse_args()
    output = handoff.bound(args.derive)
    if not output.is_relative_to(ROOT / "test-cache"):
        raise ValueError("ground dry conversion must stay in repository test-cache")
    report, files = derive()
    output.mkdir(parents=True, exist_ok=True)
    for path, data in files.items():
        (output / path.name).write_bytes(data)
    (output / "packed-ground-manifest.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("pose_count", "native_start", "native_sprite_bound", "sheet_size", "structurally_absent_parts")}))


if __name__ == "__main__":
    main()
