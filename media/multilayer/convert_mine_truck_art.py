"""Convert actual model07 bulk interiors inside its unchanged native road body.

@file convert_mine_truck_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group21B
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
This source-only handoff owns no road traffic, cargo inventory or packed range.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_ground_fleet_art as fleet
from convert_terrain_art import png_bytes
from tools.ground_fleet_contract import mine_truck_cargo_geometry, native_cargo_forms, variant_contract


ASSET = "vehicle.model-07"
REPORT = fleet.OUTPUT / "vehicle-model-07-bulk-source-manifest.json"


def review_bytes(board):
    """Encode an opaque RGB review without an indexed sprite transparency flag.

    @param board RGB geometry or contact board, separate from native atlas alpha.
    @return Reproducible PNG bytes suitable only for source inspection.
    """
    output = io.BytesIO()
    board.save(output, format="PNG", optimize=False, compress_level=9)
    return output.getvalue()


def geometry_signature(family):
    """Freeze actual road hold geometry with native cargo and body registrations.

    @param family Retained own07 family with its eight approved body headings.
    @return SHA256 of cargo masks, genuine body ancestry and native label policy.
    """
    fields = {"geometry": family["cargo_geometry"],
              "registrations": family["approved_base_registration"], "variants": variant_contract(7)}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode()).hexdigest()


def checked_family(document, public):
    """Require the complete actual cargo registry and approved own-road masks.

    @param document Retained source selections or isolated negative fixture.
    @param public Shared native sprite contract or isolated negative fixture.
    @return Own07 family and its authoritative shared specification.
    """
    spec = fleet.public_family(ASSET, public)
    family = document["families"][ASSET]
    # A matching canvas cannot authorize a rail hold, changed truck body or
    # missing actual refit. Check all owners before sampling any source pixels.
    if (family["cargo_geometry"] != mine_truck_cargo_geometry()
            or family["approved_cargo_geometry_sha256"] != geometry_signature(family)
            or set(family["approved_bulk_materials"]) != set(spec["native_cargo_labels"])):
        raise ValueError("mine truck cargo geometry or material registry changed")
    expected = list(enumerate(spec["native_cargo_labels"]))
    actual = [(row["variant"], row["cargo_label"]) for row in family["cargo_source_matrix"]]
    if actual != expected:
        raise ValueError("mine truck source refits differ from native cargo order")
    return family, spec


def material_source(label, document):
    """Resolve exact bulk originals, including retained native Earth/Rock grains.

    @param label Actual model07 dry-bulk label, never a local cargo ID.
    @param document Retained source record with explicit same-material reuse.
    @return Opaque original crop and mechanically verified extraction evidence.
    """
    if native_cargo_forms(7).get(label) != "dry_bulk":
        raise ValueError("mine truck material is not an actual dry-bulk cargo")
    family = document["families"][ASSET]
    selection = family["approved_bulk_materials"][label]
    matches = [row for row in document["candidates"] if row["key"] == selection["source"]]
    if len(matches) != 1:
        raise ValueError("mine truck material has no unique retained original")
    candidate = matches[0]
    if label in ("ERTH", "ROCK"):
        # These textures reuse actual M1 cargo originals rather than a newly
        # colored mineral. The opaque interior excludes the authored heap's
        # directional outline; the original catalogue still owns its meaning.
        origin = candidate.get("original_record", {})
        if origin.get("path") != "media/multilayer/source/m1/generation-record.json":
            raise ValueError("mine truck Earth/Rock original owner changed")
        retained = json.loads((fleet.base.ROOT / origin["path"]).read_text(encoding="utf-8"))
        originals = [row for row in retained["sources"] if row["key"] == origin.get("key")]
        if len(originals) != 1:
            raise ValueError("mine truck Earth/Rock original is unavailable")
        original = originals[0]
        # Source bytes, cargo identity and original prompt must agree. A valid
        # grain crop from another cargo cannot impersonate spoil provenance.
        if (original["asset_id"] != "cargo." + label or candidate["asset_id"] != original["asset_id"]
                or candidate["path"] != original["path"] or candidate["sha256"] != original["sha256"]
                or origin["sha256"] != original["sha256"] or candidate["prompt"] != original["prompt"]):
            raise ValueError("mine truck Earth/Rock original ancestry changed")
    else:
        # Shared approved minerals contribute only their genuine grain texture,
        # never the ordinary wagon's shell, aperture, alpha or wheel geometry.
        ordinary = document["families"]["vehicle.model-05"]["approved_bulk_materials"].get(label)
        if selection != ordinary:
            raise ValueError("mine truck shared mineral differs from approved original")
    material, evidence = fleet.bulk_material(label, document, 7)
    return material, {**evidence, "reuse_policy": "original opaque material interior only; own road hold, no recoloring"}


def cargo_mask(direction, load, geometry=None):
    """Select only the own-road hold floor behind its fixed foreground steel.

    @param direction Actual selected truck heading.
    @param load Native packet-backed partial or full visual category.
    @param geometry Optional approved geometry for isolated refusal checks.
    @return Native64x48 alpha mask, with partial contained in full.
    """
    if geometry is None:
        geometry = mine_truck_cargo_geometry()
    if geometry != mine_truck_cargo_geometry():
        raise ValueError("mine truck cannot borrow another cargo aperture")
    return fleet.cargo_mask(direction, load, geometry)


def compose_bulk(body, material, direction, load):
    """Place unchanged original material behind the own truck's retained rim.

    @param body Indexed actual own07 empty body in this genuine heading.
    @param material Opaque original grain interior with verified ancestry.
    @param direction Actual native truck heading, never synthesized by rotation.
    @param load Partial or full packet-backed visual fill category.
    @return Indexed body retaining every pixel and alpha outside the hold mask.
    """
    full, mask = cargo_mask(direction, "full"), cargo_mask(direction, load)
    # Bulk cannot make a new silhouette, cover a tyre/cab or fill transparent
    # space. Fail on invalid physical geometry rather than repair the source.
    if any(allowed and pixel == 0 for allowed, pixel in zip(full.get_flattened_data(), body.get_flattened_data())):
        raise ValueError("mine truck bulk aperture leaves its original body")
    if material.getchannel("A").getextrema()[0] < 128:
        raise ValueError("mine truck original material contains transparent gaps")
    left, top, right, bottom = full.getbbox()
    donor = fleet.base.quantize(material.resize((right - left, bottom - top), Image.Resampling.BOX), {})
    image = body.copy()
    image.paste(donor, (left, top), mask.crop((left, top, right, bottom)))
    if image.tobytes() == body.tobytes():
        raise ValueError("mine truck material disappeared at native scale")
    return image


def geometry_guide(document, public):
    """Expose actual truck hold/rim registrations against original body pixels.

    @param document Retained own07 body and cargo registrations.
    @param public Shared sprite geometry and actual native refit specification.
    @return RGB native1x/4x board; green full, amber partial and red steel rim.
    """
    family, _spec = checked_family(document, public)
    board = Image.new("RGB", (1048, 560), (42, 46, 50))
    labels = ImageDraw.Draw(board)
    labels.text((12, 12), "MODEL07 ROAD HOLD GEOMETRY: green full / amber rear-B partial / red preserved rim; native1x/4x", fill="white")
    # Neighboring genuine views must retain the same cab/bed ownership while
    # their actual foreshortened floor apertures follow the fixed native camera.
    for index, direction in enumerate(fleet.DIRECTIONS):
        body, _evidence = fleet.native_view(family["selections"]["empty/" + direction], direction, document, public)
        overlay = Image.new("RGBA", (64, 48))
        draw = ImageDraw.Draw(overlay)
        draw.bitmap((0, 0), cargo_mask(direction, "full"), fill=(0, 220, 100, 150))
        draw.bitmap((0, 0), cargo_mask(direction, "partial"), fill=(255, 190, 0, 180))
        for line in family["cargo_geometry"]["foreground_rim_lines"][direction]:
            draw.line([tuple(point) for point in line], fill=(255, 80, 80, 255), width=1)
        pose = Image.alpha_composite(body.convert("RGBA"), overlay)
        left, top = 12 + index % 4 * 256, 40 + index // 4 * 250
        labels.text((left, top), direction, fill="white")
        board.paste(pose, (left + 80, top + 16), pose)
        large = pose.resize((256, 192), Image.Resampling.NEAREST)
        board.paste(large, (left, top + 58), large)
    return board


def build(document=None, public=None):
    """Build all actual frame-zero bulk refits while exposing later missing poses.

    @param document Optional isolated record; defaults to curated retained inputs.
    @param public Optional isolated public manifest for boundary refusal checks.
    @return Staged report and exact base/load/review bytes; no public writes.
    """
    if document is None:
        document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    if public is None:
        public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
    family, spec = checked_family(document, public)
    base_report, files = fleet.build_base(ASSET, document, public)
    if not base_report["base_geometry_complete"]:
        raise ValueError("mine truck cargo needs all eight genuine body views")
    base_atlas = Image.open(io.BytesIO(files[fleet.base.ROOT / base_report["path"]]))
    bodies = {}
    # Every cargo reuses its own whole genuine heading. Reading the regenerated
    # base avoids substituting stale proof pixels or another body's geometry.
    for row in base_report["sprites"]:
        x, y, width, height = row["box"]
        bodies[row["direction"]] = base_atlas.crop((x, y, x + width, y + height))
    atlas = Image.new("P", (512, 48 * 2 * len(spec["native_cargo_labels"])), 0)
    atlas.putpalette(fleet.base.PALETTE_BYTES)
    atlas.info["transparency"] = 0
    sprites, materials = [], []
    # Every native cargo gets its own explicit original material and both load
    # categories. No recolored fallback or frame-one padding can claim coverage.
    for variant, label in enumerate(spec["native_cargo_labels"]):
        material, evidence = material_source(label, document)
        materials.append(evidence)
        for level, load in enumerate(("partial", "full")):
            for index, direction in enumerate(fleet.DIRECTIONS):
                image = compose_bulk(bodies[direction], material, direction, load)
                x, y = 64 * index, 48 * (2 * variant + level)
                atlas.paste(image, (x, y))
                sprites.append({"direction": direction, "state": load, "frame": 0, "part": "body",
                                "tile_variant": variant, "cargo_label": label, "anchor": [-32, -32],
                                "box": [x, y, 64, 48], "material_source": evidence,
                                "body_registration_sha256": family["approved_base_registration"][direction]})
    # Empty bodies are genuinely cargo-independent, but each actual refit keeps
    # a separate native slot. These aliases cannot fill motion/status entries.
    aliases = [{"tile_variant": variant, "cargo_label": label, "state": "empty", "frame": 0,
                "direction": row["direction"], "part": "body", "source_atlas": base_report["path"],
                "source_sha256": base_report["sha256"], "source_box": row["box"]}
               for variant, label in enumerate(spec["native_cargo_labels"]) for row in base_report["sprites"]]
    present = {(row["state"], row["frame"], row["direction"], row["tile_variant"]) for row in sprites + aliases}
    required = {(state, frame, direction, variant) for state in spec["states"] for frame in range(spec["frames"])
                for direction in fleet.DIRECTIONS for variant in range(spec["tile_variants"])}
    path = fleet.OUTPUT / "vehicle-model-07-bulk-authored.png"
    encoded = png_bytes(atlas)
    report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md Group21B; road bulk frame0 only",
              "schema_version": 1, "asset_id": ASSET, "source_record": fleet.RECORD.relative_to(fleet.base.ROOT).as_posix(),
              "path": path.relative_to(fleet.base.ROOT).as_posix(), "sha256": hashlib.sha256(encoded).hexdigest(),
              "cargo_geometry_sha256": geometry_signature(family), "materials": materials,
              "bulk_frame0_pose_count": len(sprites), "empty_frame0_alias_count": len(aliases),
              "staged_body_matrix_pose_count": len(present), "required_body_matrix_pose_count": len(required),
              "missing_pose_count": len(required - present), "missing": [list(identity) for identity in sorted(required - present)],
              "source_coverage_complete": False, "packed": False, "runtime_activated": False,
              "sprites": sprites, "empty_aliases": aliases}
    files[path] = encoded
    files[REPORT] = (json.dumps(report, indent=2) + "\n").encode()
    files[fleet.OUTPUT / "vehicle-model-07-cargo-geometry.png"] = review_bytes(geometry_guide(document, public))
    # All heading/fill combinations stay visible at native1x/4x per material,
    # making foreground steel cuts and rear-B partial selection reviewable.
    for label in spec["native_cargo_labels"]:
        board = Image.new("RGB", (1048, 1120), (42, 46, 50))
        for level, load in enumerate(("partial", "full")):
            rows = [row for row in sprites if row["cargo_label"] == label and row["state"] == load]
            caption = "MODEL07 SOURCE LOAD: " + label + " " + load + "; frame0/native1x/4x; states/purchase/runtime pending"
            board.paste(fleet.base_contact(atlas, rows, ASSET, caption), (0, 560 * level))
        files[fleet.OUTPUT / ("vehicle-model-07-" + label.lower() + "-contact.png")] = review_bytes(board)
    comparison = Image.new("RGB", (1048, 640), (42, 46, 50))
    labels = ImageDraw.Draw(comparison)
    labels.text((12, 12), "MODEL07 ORIGINAL MATERIALS: full N / E native4x; frame0 sources, runtime pending", fill="white")
    # End-on and side-on native sample sizes expose material collapse that a
    # large source patch can hide, including the actual Earth/Rock originals.
    for index, label in enumerate(spec["native_cargo_labels"]):
        left, top = 12 + index % 4 * 256, 42 + index // 4 * 196
        labels.text((left, top), label + " N / E", fill="white")
        for direction, dx, dy in (("N", 12, 28), ("E", 100, 76)):
            row = next(row for row in sprites if row["cargo_label"] == label and row["state"] == "full" and row["direction"] == direction)
            x, y, width, height = row["box"]
            native = atlas.crop((x, y, x + width, y + height))
            subject = native.crop(native.getbbox()).convert("RGBA")
            large = subject.resize((4 * subject.width, 4 * subject.height), Image.Resampling.NEAREST)
            comparison.paste(large, (left + dx, top + dy), large)
    files[fleet.OUTPUT / "vehicle-model-07-bulk-materials-contact.png"] = review_bytes(comparison)
    return report, files


def main():
    """Check or publish only the bounded reproducible truck bulk-source artifacts.

    @return None; fail on stale outputs without changing public/native coverage.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--publish", action="store_true", help="write exact source review artifacts only")
    args = parser.parse_args()
    report, files = build()
    # Publication writes only explicit repository proof artifacts. The normal
    # read-only path refuses stale files rather than trusting previous reports.
    for path, encoded in files.items():
        if args.publish:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(encoded)
        elif not path.exists() or path.read_bytes() != encoded:
            # Missing or changed outputs cannot serve as source handoff evidence.
            raise ValueError("stale mine truck bulk artifact: " + str(path))
    print(f"model07: {report['bulk_frame0_pose_count']} loads + {report['empty_frame0_alias_count']} empty aliases; {report['missing_pose_count']} missing; source/runtime pending")


if __name__ == "__main__":
    main()
