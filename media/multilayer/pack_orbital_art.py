"""Append reviewed orbital sources without changing the prior native sprite prefix.

@file pack_orbital_art.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17BC.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups79-80
Packing grants no gameplay capability. Native decoding, selectors and ordinary
presentation remain separate acceptance gates after deterministic source staging.
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
from tools.orbital_art_contract import validate_orbital_contract

ROOT = source.ROOT
SCOPE = ROOT / "media/multilayer/orbital-pack-scope.json"
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
ASSETS = ("vehicle.model-21", "vehicle.model-16", "vehicle.model-17", "module.nucleus",
          "module.dock", "module.storage", "module.power", "module.fuel-store",
          "module.workshop", "module.processor")


def read_scope():
    """Protect the existing native sheets/ranges and pinned deterministic tools.

    @return Explicit ten-family append contract with its immutable source snapshots.
    @throws ValueError On changed prefix, tools, family order or append extent.
    """
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    # Existing IDs end at5167. New selection cannot reclaim an old slot, reorder
    # accepted families or silently encode with a different Python/Pillow pair.
    if (tuple(scope["asset_ids"]) != ASSETS or scope["native_start"] != 5167
            or scope["protected_native_end_exclusive"] != 5167 or scope["native_end_exclusive"] != 6143
            or scope["toolchain"]["python"] != sys.version.split()[0]
            or scope["toolchain"]["pillow"] != PIL.__version__):
        raise ValueError("orbital packing order, toolchain or protected native limits changed")
    # Prior M0/M1/preview/Space sheet and header bytes remain reproducible without
    # revisiting unrelated historical generation graphs or updating their hashes.
    for identity in scope["protected_outputs"]:
        if source.identity(ROOT / identity["path"]) != identity:
            raise ValueError("protected native source changed: " + identity["path"])
    return scope


def module_poses(family, report, native, images):
    """Validate genuine fixed-site cells, including explicitly absent foregrounds.

    @param family Actual public native footprint, anchor, stage and part contract.
    @param report Exact accepted module source manifest, optionally corrupted in a test.
    @param native Immutable indexed atlas identity pinned by the source handoff.
    @param images Conversion-local cache of palette-verified original atlases.
    @return Complete identity-keyed source parts and their actual source atlas.
    @throws ValueError On lost cells, false absence, geometry, hash, depth or alias drift.
    """
    asset = family["asset_id"]
    # A fixed native site has real world cells, not rotated whole-building
    # copies. Full source approval and all required logical parts must precede packing.
    if (report.get("asset_id") != asset or not report.get("coverage_complete")
            or report.get("logical_pose_count") != len(packing.required_keys(family))
            or family["directions"] != ["none"] or family["native_size"] != [64, 64]):
        raise ValueError(asset + ": invalid completed fixed-site source handoff")
    # The accepted manifest and on-disk atlas must name the same original
    # pixels; source completion alone cannot authorize a replaced building sheet.
    if native not in report["artifacts"] or source.identity(ROOT / native["path"]) != native:
        raise ValueError(asset + ": changed original-derived module atlas")
    if native["path"] not in images:
        image = Image.open(ROOT / native["path"])
        packing.validate_source_image(image, asset)
        images[native["path"]] = image
    image = images[native["path"]]
    poses, cells = {}, {}
    width, height = family["footprint"]
    # Every source rectangle retains one actual state/frame/world-cell/part.
    # Check original bytes before cropping so transparent holes cannot conceal
    # a missing tile, moved anchor or invented opaque foreground.
    for entry in report["sprites"]:
        key = (entry["state"], entry["frame"], entry["direction"], entry["tile_variant"], entry["part"])
        tile = entry["tile_variant"]
        wx, wy = tile % width, tile // width
        origin = [32 * width + 1 + 32 * (wy - wx), 16 * width - 17 + 16 * (wx + wy)]
        x, y, sw, sh = entry["box"]
        # Source-local metadata cannot reinterpret the saved footprint or make
        # a plausible neighbouring atlas cell stand in for its declared identity.
        if (key in poses or entry.get("asset_id") != asset or not 0 <= tile < width * height
                or entry["world_cell"] != [wx, wy] or entry["native_origin"] != origin
                or entry["native_layout"] != 0 or entry["anchor"] != family["anchor"]
                or [sw, sh] != family["native_size"]
                or not (0 <= x < x + sw <= image.width and 0 <= y < y + sh <= image.height)):
            raise ValueError(asset + ": changed/duplicate module cell geometry")
        cell = image.crop((x, y, x + sw, y + sh))
        empty = cell.getbbox() is None
        # Honest absent fronts carry zero original pixels. Empty backs or a
        # false absence declaration cannot become apparently complete coverage.
        if (hashlib.sha256(cell.tobytes()).hexdigest() != entry["native_sha256"]
                or entry["structurally_absent"] != empty
                or entry["opaque_pixels"] != sum(value != 0 for value in cell.get_flattened_data())
                or (empty and entry["part"] != "front")):
            raise ValueError(asset + ": changed module source pixels or false physical absence")
        poses[key] = dict(entry)
        cells[key] = cell.tobytes()
    if set(poses) != set(packing.required_keys(family)):
        raise ValueError(asset + ": incomplete module source identity matrix")
    # Explicit still/state aliases preserve their referenced original pixels;
    # they cannot fill an omitted logical source row or change physical depth.
    for key, pose in poses.items():
        alias = pose.get("alias_of")
        if alias is None:
            continue
        other = (alias["state"], alias["frame"], alias["direction"], alias["tile_variant"], alias["part"])
        if other not in cells or cells[key] != cells[other]:
            raise ValueError(asset + ": false module source alias")
    return poses, image


def format_header(ranges, bound):
    """Declare only the appended orbital native families and exact pose domains.

    @param ranges Actual ten-family ranges in original source-plan order.
    @param bound Exclusive Map Layer sprite bound after the appended orbital parts.
    @return Documented UTF-8 C++ enum/range bytes, leaving earlier headers unchanged.
    """
    lines = ["/** @file orbital_sprite_assets.h Generated orbital native ranges.",
             " * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K", " */",
             "#ifndef ORBITAL_SPRITE_ASSETS_H", "#define ORBITAL_SPRITE_ASSETS_H", "",
             "#include <array>", "#include <cstdint>", "",
             "/** Appended source families only; no saved gameplay identity or capability. */",
             "enum class OrbitalSpriteAsset : uint8_t {"]
    lines.extend("\t" + packing.cpp_name(row["asset_id"]) + ", ///< " + row["asset_id"] for row in ranges)
    lines.extend(["\tEnd, ///< Number of appended orbital source families.", "};", "",
                  "/** Native range with state/frame/direction/world-tile/part ordering. */",
                  "struct OrbitalSpriteRange {",
                  "\tuint16_t offset; ///< Relative native Map Layer block offset.",
                  "\tuint16_t count; ///< Complete logical part count, including honest absence/aliases.",
                  "\tuint8_t states; ///< Published physical/functional states.",
                  "\tuint8_t frames; ///< Authored phases and explicit still aliases.",
                  "\tuint8_t directions; ///< Eight craft views or one fixed site view.",
                  "\tuint8_t tiles; ///< Real native world cells; one for craft.",
                  "\tuint8_t parts; ///< Source body or back/front parts.", "};", "",
                  "static constexpr uint16_t ORBITAL_SPRITE_START = 5167; ///< Every prior native sprite precedes this append.",
                  f"static constexpr uint16_t ORBITAL_SPRITE_END = {bound}; ///< Exclusive final orbital native sprite bound.",
                  f"static constexpr std::array<OrbitalSpriteRange, {len(ranges)}> _orbital_sprite_ranges = {{{{"])
    # Generate domains from validated public rows rather than maintaining a
    # second ordinal table that could diverge from source/selector ownership.
    for row in ranges:
        values = (row["native_offset"], row["sprite_count"], len(row["states"]), row["frames"],
                  len(row["directions"]), row["tile_variants"], len(row["parts"]))
        lines.append("\t{" + ", ".join(map(str, values)) + "}, // " + row["asset_id"])
    lines.extend(["}};", "", "#endif /* ORBITAL_SPRITE_ASSETS_H */", ""])
    return "\n".join(lines).encode("utf-8")


def derive(public=None, source_manifests=None, context=None):
    """Derive deterministic orbital packing bytes from exact accepted originals.

    @param public Optional detached manifest for focused corruption/refusal checks.
    @param source_manifests Complete current source report identities keyed by public asset.
    @param context Exact current invocation owning disposable selected sources/candidates.
    @return Packing report and exact own PNG/NFO/generated-header byte mapping.
    @throws ValueError On changed source, contract, native range, palette or coverage.
    """
    scope = read_scope()
    public = json.loads(PUBLIC.read_text(encoding="utf-8")) if public is None else public
    # Current output selection requires its complete fresh source closure;
    # historical source-review approval paths cannot supply missing current work.
    if (source_manifests is None) != (context is None):
        raise ValueError("selected orbital packing requires context and source manifests together")
    reviews, documents = scope["source_reviews"], {}
    if context is not None:
        from regenerate_space_sources import bind_sources
        public, reviews, documents = bind_sources(public, scope, context, source_manifests)
    matches = [row for row in public["assets"] if row["asset_id"] in ASSETS]
    families = {row["asset_id"]: row for row in matches}
    # Dictionary construction must not silently replace a duplicated paid role;
    # missing or ambiguous families stop the whole append before source staging.
    if len(matches) != len(ASSETS) or set(families) != set(ASSETS):
        raise ValueError("missing or duplicate orbital public family")
    images, sprites, entries, ranges = {}, [], [], []
    cursor = scope["native_start"]
    # Append complete accepted source families once in fixed plan order. No
    # native slot can borrow another model's role or repeat a whole site per tile.
    for asset in ASSETS:
        family = families[asset]
        packing.validate_family(family)
        validate_orbital_contract(family)
        # Source packing owns geometry and pixel coverage. Later runtime-binding
        # metadata is historical/native evidence, independently requalified by
        # native recipes; it cannot alter or authorize these source candidates.
        contract = {key: value for key, value in family.items() if key not in ("packed", "authoring", "runtime")}
        expected_contract = {key: value for key, value in scope["public_contracts"][asset].items() if key != "runtime"}
        if contract != expected_contract:
            raise ValueError(asset + ": accepted orbital public source contract changed")
        reference = reviews[asset]
        native = (documents[reference["path"]]["native_source"] if context is not None
                  else scope["source_atlases"][asset])
        authoring = family.get("authoring", {})
        # Exact source approval, manifest and palette atlas form one gate.
        # Native staging cannot replace missing producer work with a new file.
        if (not authoring.get("coverage_complete") or authoring.get("atlas_manifest") != reference["path"]
                or authoring.get("atlas_manifest_sha256") != reference["sha256"]
                or authoring.get("native_atlas") != native["path"] or authoring.get("native_sha256") != native["sha256"]
                or source.identity(ROOT / reference["path"]) != reference):
            raise ValueError(asset + ": changed or unapproved orbital source handoff")
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        snapshot = dict(candidates=scope["source_candidates"][asset])
        # Verify every retained selected original, prompt receipt and actual
        # input image from the source snapshot without rerunning generation.
        for candidate in snapshot["candidates"]:
            source.source_candidate(candidate["key"], snapshot)
        poses, image = (module_poses(family, report, native, images) if asset.startswith("module.")
                        else packing.source_poses(family, documents, images))
        keys = packing.required_keys(family)
        count = len(keys)
        packed = family["packed"]
        # Existing definition-local MLVI offsets remain owned by content. A
        # repeated stage must reproduce its native append instead of moving it.
        binding = {key: packed.get(key) for key in ("namespace", "offset", "sha256")}
        expected_binding = scope["definition_bindings"][asset]
        # Later launch/ground publication legitimately updates the shared GRF
        # checksum for an already native-bound family. Accept only that actual
        # shipping container, retaining the frozen original identity as history;
        # this pure conversion neither publishes nor qualifies native decoding.
        if binding["sha256"] != expected_binding["sha256"]:
            if packed.get("route") != scope["native_route"] or binding["sha256"] != source.identity(ROOT / "media/baseset/openttd.grf")["sha256"]:
                raise ValueError(asset + ": orbital shared shipping container binding changed")
        if ({key: binding[key] for key in ("namespace", "offset")} != {key: expected_binding[key] for key in ("namespace", "offset")}
                or packed.get("native_offset", cursor) != cursor
                or packed.get("sprite_count", count) != count
                or packed.get("route", scope["native_route"]) != scope["native_route"]
                or packed.get("source_payload", scope["report"]) != scope["report"]):
            raise ValueError(asset + ": orbital native binding moved or changed source extent")
        ranges.append(dict(asset_id=asset, native_offset=cursor, sprite_count=count,
                           states=family["states"], frames=family["frames"], directions=family["directions"],
                           tile_variants=family["tile_variants"], parts=family["parts"]))
        # Shared transparent-margin cropping preserves each real world anchor,
        # including unchanged empty64x64 parts and legitimate interior holes.
        for index, key in enumerate(keys):
            pose = poses[key]
            sprites.append(packing.cropped_pose(image, pose))
            entries.append(dict(asset_id=asset, native_offset=cursor + index, state=key[0], frame=key[1],
                                direction=key[2], tile_variant=key[3], part=key[4], source_atlas=native["path"],
                                source_box=pose["box"], source_anchor=pose["anchor"],
                                structurally_absent=pose.get("structurally_absent", False),
                                source_alias=pose.get("alias_of"), explicit_static_alias=pose.get("explicit_static_alias", False)))
        cursor += count
    if cursor != scope["native_end_exclusive"]:
        raise ValueError("orbital append count differs from frozen ten-family scope")
    sheet, positions = packing.shelf_sheet(sprites, 2048, 2)
    png = png_bytes(sheet)
    nfo = packing.format_nfo(entries, sprites, positions).replace(b"map_layers_m1", b"orbital_final")
    nfo = nfo.replace(b"Generated selected M1 source sprites", b"Generated orbital source sprites")
    nfo = nfo.replace(b"r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/79C", b"r2mlvs11-orbital-stations-and-manufacturing.md Group13K")
    files = {ROOT / scope["png"]: png, ROOT / scope["nfo"]: nfo,
             ROOT / scope["header"]: format_header(ranges, cursor)}
    if context is not None:
        files = {context.output("data", Path(path).name): encoded for path, encoded in files.items()}
    # Retain original and cropped geometry/hashes so independent native decoding
    # can later prove exact pixels and anchors for every old and new logical ID.
    for entry, sprite, position in zip(entries, sprites, positions, strict=True):
        pixels, sw, sh, rx, ry = sprite
        entry.update(box=[*position, sw, sh], anchor=[rx, ry], pixel_sha256=hashlib.sha256(bytes(pixels)).hexdigest())
    report = dict(_documentation=__doc__, schema_version=1, route=scope["native_route"],
                  source_reviews=scope["source_reviews"], protected_outputs=scope["protected_outputs"],
                  toolchain=scope["toolchain"], native_start=scope["native_start"], native_sprite_bound=cursor,
                  pose_count=len(entries), families=ranges, sprites=entries,
                  outputs=[dict(path=path.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(data).hexdigest())
                           for path, data in files.items()], packed_grf=None, runtime_activated=False)
    if context is not None:
        report.update(schema_version=2, kind="selected_orbital_packing", run_id=context.run_id,
                      historical_source_reviews=scope["source_reviews"], source_reviews=reviews)
    return report, files


def publish(report, files):
    """Stage only the verified append bytes and selected native public references.

    @param report Deterministic complete orbital append/source evidence.
    @param files Exact own PNG/NFO/generated-header outputs.
    @return None; actual GRF decoding and native gameplay review remain pending.
    """
    scope = read_scope()
    # Identical derived bytes do not touch C++ headers or cause unnecessary
    # rebuilds. Earlier native source outputs are never part of this write set.
    for path, data in files.items():
        if not path.exists() or path.read_bytes() != data:
            path.write_bytes(data)
    destination = ROOT / scope["report"]
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    ranges = {row["asset_id"]: row for row in report["families"]}
    # Only these ten delivered families gain native staging links. Source,
    # definition-local offsets and all other public rows retain their identities.
    for family in public["assets"]:
        if family["asset_id"] in ranges:
            row = ranges[family["asset_id"]]
            family["packed"].update(route=report["route"], native_offset=row["native_offset"],
                                    sprite_count=row["sprite_count"], source_payload=scope["report"])
    PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Validate or stage the ten-family source append without native build claims."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--stage", action="store_true", help="write only own validated append inputs")
    args = parser.parse_args()
    report, files = derive()
    if args.stage:
        publish(report, files)
    print(f"Orbital inputs: {report['pose_count']} parts, native [{report['native_start']},{report['native_sprite_bound']}), decoded/runtime pending")


if __name__ == "__main__":
    main()
