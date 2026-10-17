"""Append completed Space source families through the existing native pack owners.

@file pack_space_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups79-80
Original source pixels and prior native ranges remain immutable. Staging packing
inputs does not claim decoded payload loading or native gameplay presentation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import PIL
from PIL import Image

import convert_space_art as source
import pack_m1_art as packing
from convert_terrain_art import png_bytes

ROOT = source.ROOT
SCOPE = ROOT / "media/multilayer/space-pack-scope.json"
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"


def read_scope():
    """Validate pinned conversion tools and every protected source output.

    @return Explicit Space append contract after original-prefix verification.
    @throws ValueError On changed legacy art, toolchain or native append limits.
    """
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    # Stable native IDs and byte-identical encoding require the same pinned tools; Space cannot reclaim the existing preview or relocate any M1 sprite.
    if (scope["toolchain"]["python"] != sys.version.split()[0]
            or scope["toolchain"]["pillow"] != PIL.__version__
            or scope["native_start"] != 3599 or scope["protected_native_end_exclusive"] != 3599
            or scope["native_end_exclusive"] != 5167):
        raise ValueError("Space packing toolchain or protected native limits changed")
    # Hash all old source sheets/NFO and the generated M1 range declaration without reopening their historical original-generation graphs.
    for identity in scope["protected_outputs"]:
        if source.identity(ROOT / identity["path"]) != identity:
            raise ValueError("protected native source changed: " + identity["path"])
    return scope


def asteroid_poses(family, report, images):
    """Adapt the explicit three-size asteroid report to the shared packer's cells.

    @param family Public natural-body state/frame/view/size/anchor contract.
    @param report Exact completed source report pinned by its owning handoff.
    @param images Conversion-local cache of checked indexed source atlases.
    @return Complete native identity map and the actual original-derived atlas.
    @throws ValueError On missing or changed source pixels, geometry or coverage.
    """
    asset = family["asset_id"]
    # A static natural body must keep all three real saved sizes; the craft report adapter cannot infer these sizes from artwork or treat them as unrelated tiles.
    if (report.get("asset_id") != asset or not report.get("complete_source_coverage")
            or report.get("logical_pose_count") != 384
            or family["size_variants"] != ["small", "medium", "large"]):
        raise ValueError(asset + ": invalid completed asteroid source handoff")
    native = next(row for row in report["artifacts"] if Path(row["path"]).name == "native.png")
    # The public source and original review must identify the same actual native atlas before any range can be staged.
    if (native["path"] != family["authoring"]["native_atlas"]
            or native["sha256"] != family["authoring"]["native_sha256"]
            or source.identity(ROOT / native["path"]) != native):
        raise ValueError(asset + ": changed original-derived asteroid atlas")
    if native["path"] not in images:
        images[native["path"]] = Image.open(ROOT / native["path"])
        packing.validate_source_image(images[native["path"]], asset)
    image = images[native["path"]]
    poses = {}
    # Every source row carries its saved-size spelling and variant independently; checking both prevents a repeated Large image from hiding a missing Small or Medium identity.
    for entry in report["entries"]:
        variant = family["size_variants"].index(entry["size"])
        left, top, right, bottom = entry["box"]
        pose = dict(state=entry["state"], frame=entry["frame"], direction=entry["direction"],
                    tile_variant=variant, part="body", box=[left, top, right - left, bottom - top], anchor=entry["anchor"])
        key = pose["state"], pose["frame"], pose["direction"], variant, "body"
        # Native bounds and decoded bytes are checked before transparent-margin cropping; neither metadata counts nor a plausible neighbouring view can supply a missing original pose.
        if (key in poses or variant != entry["variant"] or pose["box"][2:] != family["native_size"]
                or pose["anchor"] != family["anchor"]
                or not (0 <= left < right <= image.width and 0 <= top < bottom <= image.height)
                or hashlib.sha256(image.crop(entry["box"]).tobytes()).hexdigest() != entry["pixel_sha256"]):
            raise ValueError(asset + ": changed/duplicate asteroid source cell")
        poses[key] = pose
    if set(poses) != set(packing.required_keys(family)):
        raise ValueError(asset + ": incomplete asteroid source identity matrix")
    return poses, image


def format_header(ranges, bound):
    """Declare a separate append-only native Space range table.

    @param ranges Actual seven family ranges in explicit master-plan source order.
    @param bound Exclusive native Action5 sprite bound after all Space poses.
    @return UTF-8 C++ declaration bytes with owning plan and field documentation.
    """
    lines = ["/** @file space_sprite_assets.h Generated final Space native ranges.",
             " * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG", " */",
             "#ifndef SPACE_SPRITE_ASSETS_H", "#define SPACE_SPRITE_ASSETS_H", "",
             "#include <array>", "#include <cstdint>", "",
             "/** Existing source families in fixed append order; no saved gameplay identity. */",
             "enum class SpaceSpriteAsset : uint8_t {"]
    lines.extend("\t" + packing.cpp_name(row["asset_id"]) + ", ///< " + row["asset_id"] for row in ranges)
    lines.extend(["\tEnd, ///< Number of final Space art families.", "};", "",
                  "/** Pure native range; pose order is state/frame/direction/saved-size. */",
                  "struct SpaceSpriteRange {",
                  "\tuint16_t offset; ///< Relative native Map Layer block offset.",
                  "\tuint16_t count; ///< Complete logical source pose count.",
                  "\tuint8_t states; ///< Published visual states.",
                  "\tuint8_t frames; ///< Authored phases and explicit static aliases.",
                  "\tuint8_t directions; ///< Genuine original engine views.",
                  "\tuint8_t sizes; ///< Saved asteroid size variants, or one for craft.",
                  "};", "",
                  "static constexpr uint16_t SPACE_SPRITE_START = 3599; ///< All original M0/M1/preview slots precede this append.",
                  f"static constexpr uint16_t SPACE_SPRITE_END = {bound}; ///< Exclusive final native Space sprite bound.",
                  f"static constexpr std::array<SpaceSpriteRange, {len(ranges)}> _space_sprite_ranges = {{{{"])
    # The declaration is generated from validated public range domains, avoiding an independent hand-maintained source of state counts or offsets.
    for row in ranges:
        values = (row["native_offset"], row["sprite_count"], len(row["states"]), row["frames"], len(row["directions"]), row["tile_variants"])
        lines.append("\t{" + ", ".join(map(str, values)) + "}, // " + row["asset_id"])
    lines.extend(["}};", "", "#endif /* SPACE_SPRITE_ASSETS_H */", ""])
    return "\n".join(lines).encode("utf-8")


def derive(public=None):
    """Prepare exact Space packing inputs without mutating prior source owners.

    @param public Optional detached public manifest for focused corruption checks.
    @return Complete packing report and repository output byte mapping.
    @throws ValueError On stale source handoff, missing coverage or changed native bindings.
    """
    scope = read_scope()
    public = json.loads(PUBLIC.read_text(encoding="utf-8")) if public is None else public
    families = {row["asset_id"]: row for row in public["assets"]}
    documents, images, sprites, entries, ranges = {}, {}, [], [], []
    cursor = scope["native_start"]
    # Complete the seven already reviewed source families in master-plan order; each appends its own contiguous range after the frozen prior prefix.
    for asset in scope["asset_ids"]:
        family = families[asset]
        packing.validate_family(family)
        identity = scope["source_reviews"][asset]
        authoring = family["authoring"]
        # Source-only publication is the entry gate. Another atlas, changed report or false completion flag cannot become a packed sprite through an existing native ID.
        if (not authoring["coverage_complete"] or authoring["atlas_manifest"] != identity["path"]
                or authoring["atlas_manifest_sha256"] != identity["sha256"]
                or source.identity(ROOT / identity["path"]) != identity):
            raise ValueError(asset + ": changed or unapproved source handoff")
        report = json.loads((ROOT / identity["path"]).read_text(encoding="utf-8"))
        # Validate retained original/receipt/input bytes from the accepted snapshot without rerunning a historical full source-generation graph.
        for candidate in report["source_candidates"]:
            for original in [{key: candidate[key] for key in ("path", "sha256")}, candidate["generation_receipt"], *candidate["references"]]:
                if source.identity(ROOT / original["path"]) != original:
                    raise ValueError(asset + ": immutable source ancestry changed")
        poses, image = (asteroid_poses(family, report, images) if asset in source.ASTEROID_ASSETS
                        else packing.source_poses(family, documents, images))
        count = len(packing.required_keys(family))
        packed = family["packed"]
        # Definition-local MLVI namespaces remain unchanged; a previously staged native append must reproduce its own range instead of shifting after a later selection.
        if (packed.get("native_offset", cursor) != cursor or packed.get("sprite_count", count) != count
                or packed.get("route", scope["native_route"]) != scope["native_route"]):
            raise ValueError(asset + ": existing native Space binding changed")
        ranges.append(dict(asset_id=asset, native_offset=cursor, sprite_count=count, states=family["states"],
                           frames=family["frames"], directions=family["directions"], tile_variants=family["tile_variants"],
                           parts=family["parts"], native_size=family["native_size"], anchor=family["anchor"]))
        # Shared crop_and_shift preserves world anchors and transparent internal holes; every logical alias still receives its declared native identity.
        for index, key in enumerate(packing.required_keys(family)):
            pose = poses[key]
            sprites.append(packing.cropped_pose(image, pose))
            entries.append(dict(asset_id=asset, native_offset=cursor + index, state=key[0], frame=key[1],
                                direction=key[2], tile_variant=key[3], part=key[4], source_atlas=authoring["native_atlas"],
                                source_box=pose["box"], source_anchor=pose["anchor"]))
        cursor += count
    if cursor != scope["native_end_exclusive"]:
        raise ValueError("Space native append count differs from frozen seven-family scope")
    sheet, positions = packing.shelf_sheet(sprites, 2048, 2)
    png = png_bytes(sheet)
    nfo = packing.format_nfo(entries, sprites, positions).replace(b"map_layers_m1", b"space_final")
    nfo = nfo.replace(b"Generated selected M1 source sprites", b"Generated final Space source sprites")
    nfo = nfo.replace(b"r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/79C", b"r2mlvs10-asteroids-and-mining-probes.md Group14BG")
    header = format_header(ranges, cursor)
    # Each report row retains both original cell geometry and packed crop geometry so an independent native loader can compare actual decoded bytes and anchors.
    for entry, sprite, position in zip(entries, sprites, positions):
        pixels, width, height, x, y = sprite
        entry.update(box=[*position, width, height], anchor=[x, y], pixel_sha256=hashlib.sha256(bytes(pixels)).hexdigest())
    files = {ROOT / scope["png"]: png, ROOT / scope["nfo"]: nfo, ROOT / scope["header"]: header}
    report = dict(_documentation="Owner: r2mlvs10 Group14BG; final Space packing inputs, native decoding/runtime remain separate.",
                  schema_version=1, route=scope["native_route"], source_reviews=scope["source_reviews"],
                  protected_outputs=scope["protected_outputs"], toolchain=scope["toolchain"],
                  native_start=scope["native_start"], native_sprite_bound=cursor, pose_count=len(entries), families=ranges,
                  sprites=entries, outputs=[dict(path=path.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(data).hexdigest())
                                             for path, data in files.items()], packed_grf=None, runtime_activated=False)
    return report, files


def publish(report, files):
    """Stage only the exact validated Space pack outputs and their own bindings.

    @param report Deterministically derived Space append/source evidence.
    @param files Exact PNG/NFO/generated range-header outputs.
    @return None; source staging leaves actual GRF/runtime acceptance pending.
    """
    scope = read_scope()
    # Avoid touching an identical C++ range header: later packing-only reproducibility runs must not trigger unnecessary native rebuilds.
    for path, data in files.items():
        if not path.exists() or path.read_bytes() != data:
            path.write_bytes(data)
    destination = ROOT / scope["report"]
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    ranges = {row["asset_id"]: row for row in report["families"]}
    # Only these seven existing public families gain native staging references; no unrelated source hashes or saved MLVI offsets are refreshed.
    for family in public["assets"]:
        if family["asset_id"] in ranges:
            row = ranges[family["asset_id"]]
            family["packed"].update(route=report["route"], native_offset=row["native_offset"],
                                    sprite_count=row["sprite_count"], source_payload=scope["report"])
    PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Validate or stage the complete final Space packing inputs.

    @return None; print source counts without claiming actual GRF loading.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--stage", action="store_true", help="write exact own PNG/NFO/header and staging references")
    args = parser.parse_args()
    report, files = derive()
    if args.stage:
        publish(report, files)
    print(f"Space packing inputs: {report['pose_count']} poses, native [{report['native_start']},{report['native_sprite_bound']}), decoded/runtime pending")


if __name__ == "__main__":
    main()
