"""Convert the last selected Surface processor originals without inventing stock or work.

@file convert_selected_processors.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups41-42.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Four-angle originals support model review; the native consumer owns one fixed
view, sixteen independently constructed cells, and its existing status phases.
"""

import argparse
import copy
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_chemical_states as chemical


mine = chemical.mine
ROOT, RECORD, PUBLIC, OUTPUT = chemical.ROOT, chemical.RECORD, chemical.PUBLIC, chemical.OUTPUT
STATES = chemical.STATES
NEW = (5, 12)
# These are individual compatible empty fittings, not a copied foreign campus.
# Two electrode passes and two assembly/test cells retain separate paid tiles.
DONORS = {
    "facility.battery-works": {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 8: 5, 9: 9, 10: 10, 11: 11, 13: 13, 14: 14, 15: 15},
    # Electronics uses dry feed, cabinets, ventilation and empty handling
    # ledges. It has no slurry reactor, fluid tank or chemical-transfer pump.
    "facility.electronics-works": {0: 0, 1: 1, 2: 15, 3: 13, 4: 0, 8: 14, 9: 9, 10: 13, 11: 11, 13: 13, 14: 14, 15: 15},
}
ALIASES = {6: 5, 7: 12}
SELECTED = ("facility.battery-works", "facility.electronics-works")


def prefix(asset):
    """Resolve the selected family's existing source artifact namespace.

    @param asset Exact selected public asset identifier.
    @return Family basename, without allocating any packed sprite identity.
    """
    if asset not in SELECTED:
        raise ValueError("processor is outside the dispatched selected scope")
    return asset.removeprefix("facility.") + "-source"


def paths(asset):
    """Locate reproducible source products beneath the existing art proof cache.

    @param asset Exact selected processor identifier.
    @return Authored atlas and source manifest paths.
    """
    name = prefix(asset)
    return OUTPUT / f"{name}-authored.png", OUTPUT / f"{name}-manifest.json"


def reuse_graph(donor_stages, asset):
    """Bind each compatible target to its actual donor cell at the same construction stage.

    @param donor_stages Validated chemical original cells, indexed by native stage and variant.
    @param asset Selected physical processor model whose compatible roles authorize reuse.
    @return Explicit per-cell ancestry with exact native original pixel identities.
    """
    rows = []
    # Construction ancestry follows the individual physical fitting throughout
    # installation; a completed donor cannot stand in for unfinished hardware.
    for stage in range(4):
        for target, source in DONORS[asset].items():
            rows.append({"stage": stage, "target_variant": target,
                         "source_family": chemical.ASSET, "source_variant": source,
                         "source_pixels_sha256": hashlib.sha256(donor_stages[stage][source].tobytes()).hexdigest()})
    return {"apparatus": rows, "local_aliases": {str(k): v for k, v in ALIASES.items()},
            "quantity_owner": "Existing Industry stock/recipe/work only; all visible fittings empty"}


def source_stages(record, public, donor_public):
    """Assemble independently placeable native cells from reviewed originals and compatible fittings.

    @param record Immutable processor original provenance and frozen family brief.
    @param public Executing family's compiled native geometry/state contract.
    @param donor_public Existing chemical family contract, used only for individual physical reuse.
    @return Four native construction-stage dictionaries and existing original status lenses.
    """
    chemical.plant.shared.validate_processor_contract(public)
    asset = public["asset_id"]
    prefix(asset)
    family = record["families"][asset]
    model = {key: family[key] for key in ("roles", "model", "inventory_policy", "recipe_ownership")}
    if mine.signature(model) != family["approved_model_sha256"]:
        raise ValueError("selected processor model or inventory ownership changed")
    donor = chemical.stage_cells(record, donor_public)
    reuse = reuse_graph(donor, asset)
    if family["reuse_graph"] != reuse:
        raise ValueError("selected processor compatible individual ancestry changed")
    registration = {stage: {str(row["variant"]): mine.component_record(record, row)[1]
                            for row in rows} for stage, rows in family["original_cells"].items()}
    if mine.signature(registration) != family["approved_component_geometry_sha256"]:
        raise ValueError("selected processor original registration changed")
    if set(family["original_cells"]) != {"1", "2", "3"}:
        raise ValueError("selected processor is missing a genuine installation stage")
    stages = {stage: {target: donor[stage][source].copy() for target, source in DONORS[asset].items()}
              for stage in range(4)}
    stages[0].update({variant: donor[0][0].copy() for variant in NEW})
    # Newly designed process machinery needs its own genuine unfinished and
    # completed originals. Reuse is limited to the explicit compatible graph.
    for stage in (1, 2, 3):
        rows = family["original_cells"][str(stage)]
        variants = [row["variant"] for row in rows]
        if len(rows) != 2 or set(variants) != set(NEW):
            raise ValueError("selected processor new machinery matrix is missing or ambiguous")
        for selection in rows:
            source, component = mine.component_record(record, selection)
            # Actual family, genuine installation stage and empty physical
            # fittings authorize this source; relabelled foreign stock does not.
            if (source.get("processor_asset_id") != asset or source.get("construction_stage") != stage
                    or component.get("construction_stage") != stage or source.get("empty_equipment") is not True):
                raise ValueError("selected processor original has a foreign model or installation stage")
            stages[stage][selection["variant"]] = mine.original_cell(record, selection, public)
    # Identical installed machinery may occur on distinct independently paid
    # cells. The explicit aliases carry pixels only, never native work or stock.
    for cells in stages.values():
        for target, source in ALIASES.items():
            cells[target] = cells[source].copy()
        for variant, sprite in cells.items():
            mine.validate_cell(sprite, variant)
    return stages, chemical.operating_lenses(record, donor_public)


def png(image, indexed=True):
    """Encode a deterministic source or diagnostic PNG with explicit ownership.

    @param image Indexed native source or RGB review image.
    @param indexed Whether native palette index zero supplies transparency.
    @return PNG bytes without altering original artwork.
    """
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md Groups41-42; archi-map-layers.md; source only")
    buffer = io.BytesIO()
    image.save(buffer, format="PNG", pnginfo=metadata, optimize=False, compress_level=9,
               **({"transparency": 0} if indexed else {}))
    return buffer.getvalue()


def review(poses, asset):
    """Show every construction/state phase at native registration for visual source QA.

    @param poses Native state/phase to sixteen original back/front cell pairs.
    @param asset Executing family identifier printed only on the diagnostic board.
    @return RGB source contact sheet; this is not ordinary-game evidence.
    """
    board = Image.new("RGB", (2608, 1664), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((16, 8), f"SOURCE ONLY {asset}: fixed4x4 /512 parts; Industry owns stock, recipe and work; packing/game pending", fill="black")
    # Side-by-side native phases expose unintended motion in static states and
    # loss of machinery or floor contact between independently built tiles.
    for index, state in enumerate(STATES):
        for frame in range(4):
            x, y = 16 + frame * 648, 56 + index * 400
            draw.text((x, y), f"{state} / native phase{frame}", fill="black")
            scene = chemical.plant.shared.campus(poses[state, frame]).convert("RGBA").resize((640, 384), Image.Resampling.NEAREST)
            board.paste(scene, (x, y + 16), scene)
    return board


def build(record, public, donor_public):
    """Derive the complete native source matrix from actual original stages and glass.

    @param record Frozen executing family and immutable original source registrations.
    @param public Actual selected processor's geometry and native status contract.
    @param donor_public Proven compatible chemical apparatus/control source contract.
    @return Complete source report and deterministic output-path to byte mappings.
    """
    asset = public["asset_id"]
    stages, lenses = source_stages(record, public, donor_public)
    atlas = Image.new("P", (2048, 1024), 0)
    atlas.putpalette(mine.base.PALETTE_BYTES)
    poses, rows = {}, []
    # Construction uses each actual tile's stage. After completion only the
    # existing hut glass changes; source animation cannot produce stock/work.
    for index, state in enumerate(STATES):
        for frame in range(4):
            poses[state, frame] = {}
            for variant in range(16):
                whole = stages[frame if state == "construction" else 3][variant]
                if state != "construction" and variant == 11:
                    whole = chemical.apply_lens(whole, lenses[state][frame], [30, 43, 3, 3])
                pair = mine.source_parts(whole)
                poses[state, frame][variant] = pair
                for part_index, (part, sprite) in enumerate(zip(mine.PARTS, pair)):
                    if sprite.getbbox() is None:
                        raise ValueError("selected processor source contains an empty native part")
                    x, y = (variant * 2 + part_index) * 64, (index * 4 + frame) * 64
                    atlas.paste(sprite, (x, y))
                    rows.append({"asset_id": asset, "state": state, "frame": frame, "direction": "none",
                                 "native_layout": 0, "tile_variant": variant, "world_cell": public["native_cells"][variant],
                                 "part": part, "box": [x, y, 64, 64], "anchor": [-31, -33], "structurally_absent": False,
                                 "nonempty_pixels": sum(v != 0 for v in sprite.get_flattened_data()),
                                 "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest()})
    atlas_path, manifest = paths(asset)
    name = prefix(asset)
    files = {atlas_path: png(atlas), OUTPUT / f"{name}-review.png": png(review(poses, asset), False),
             OUTPUT / f"{name}-campus.png": png(chemical.plant.shared.campus(poses["ready", 0])),
             OUTPUT / f"{name}-construction-cells.png": png(chemical.construction_board(stages), False)}
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md Groups41-42; archi-map-layers.md",
              "asset_id": asset, "source_coverage_complete": True, "runtime_activated": False,
              "source": atlas_path.relative_to(ROOT).as_posix(), "source_sha256": hashlib.sha256(files[atlas_path]).hexdigest(),
              "directions": ["none"], "review_angles": ["NE", "SE", "SW", "NW"], "states": list(STATES), "frames": 4,
              "native_layout": 0, "native_size": [64, 64], "anchor": [-31, -33], "footprint": [4, 4], "tile_variants": 16,
              "source_slots": 512, "nonempty_source_parts": 512, "structurally_absent_parts": 0, "source_parts": rows,
              "native_state_policy": public["native_state_policy"], "reuse_graph": record["families"][asset]["reuse_graph"],
              "inventory_policy": record["families"][asset]["inventory_policy"],
              "remaining": "Master10.4D selected packing/decoded pixels; native display requires its own producer evidence"}
    report["artifacts"] = [{"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()}
                           for path, payload in files.items()]
    files[manifest] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def validate_public_source(public, report):
    """Reject incomplete matrices, stale pixels and premature runtime claims before publication.

    @param public Executing shared manifest row with complete source metadata.
    @param report Exact live source manifest to bind to its immutable original and atlas.
    @return None after validating every original native slot and its published identity.
    """
    chemical.plant.shared.validate_processor_contract(public)
    asset, authoring = public["asset_id"], public.get("authoring", {})
    atlas_path, manifest = paths(asset)
    # Complete source coverage is authorized only by the real full matrix and
    # matching payload; a count or flag alone cannot publish art or activation.
    if (report.get("asset_id") != asset or report.get("source_slots") != 512
            or report.get("nonempty_source_parts") != 512 or report.get("structurally_absent_parts") != 0
            or report.get("source_coverage_complete") is not True or report.get("runtime_activated") is not False
            or report.get("source") != atlas_path.relative_to(ROOT).as_posix()
            or report.get("native_state_policy") != public["native_state_policy"]
            or authoring.get("coverage_complete") is not True or authoring.get("native_atlas") != report["source"]
            or authoring.get("native_sha256") != report.get("source_sha256")
            or public["source"]["status"] != "authored" or authoring.get("runtime_activated") is not False
            or public["packed"]["offset"] is not None or public["packed"]["sha256"] is not None):
        raise ValueError("selected processor complete source binding or activation changed")
    if mine.base.digest(ROOT / public["source"]["path"]) != public["source"].get("sha256"):
        raise ValueError("selected processor immutable original changed")
    if mine.base.digest(atlas_path) != report["source_sha256"]:
        raise ValueError("selected processor authored atlas changed")
    if json.loads(manifest.read_text(encoding="utf-8")) != report:
        raise ValueError("selected processor report differs from live source manifest")
    expected = {(state, frame, variant, part) for state in STATES for frame in range(4)
                for variant in range(16) for part in mine.PARTS}
    rows = report.get("source_parts", [])
    actual = {(row["state"], row["frame"], row["tile_variant"], row["part"]) for row in rows}
    if len(rows) != 512 or actual != expected:
        raise ValueError("selected processor full source matrix is missing or ambiguous")
    with Image.open(atlas_path) as atlas:
        # The native source needs indexed alpha, legal dimensions and exact
        # ordered original pixels, not an opaque contact-sheet substitute.
        if atlas.mode != "P" or atlas.size != (2048, 1024) or atlas.info.get("transparency") != 0:
            raise ValueError("selected processor native source palette/canvas changed")
        for row in rows:
            variant = row["tile_variant"]
            x = (variant * 2 + mine.PARTS.index(row["part"])) * 64
            y = (STATES.index(row["state"]) * 4 + row["frame"]) * 64
            sprite = atlas.crop((x, y, x + 64, y + 64))
            pixels = list(sprite.get_flattened_data())
            # Each slot retains its actual cell, world anchor and nonempty
            # original pixels; shifted or stale parts cannot inherit coverage.
            if (row["asset_id"] != asset or row["direction"] != "none" or row["native_layout"] != 0
                    or row["world_cell"] != [variant % 4, variant // 4] or row["anchor"] != [-31, -33]
                    or row["box"] != [x, y, 64, 64] or row["structurally_absent"] is not False
                    or sprite.getbbox() is None or row["nonempty_pixels"] != sum(value != 0 for value in pixels)
                    or set(pixels) & (mine.base.FORBIDDEN - {0})
                    or row["source_pixels_sha256"] != hashlib.sha256(sprite.tobytes()).hexdigest()):
                raise ValueError("selected processor slot changes native ownership or original pixels")


def publish(record, document, report):
    """Publish only complete source coverage through the existing public family owner.

    @param record Current immutable original provenance; earlier family documents remain exact.
    @param document Shared public manifest; only the executing processor row is updated.
    @param report Complete validated source matrix whose files have already been written.
    @return None; no packed ranges, gameplay state or native activation are changed.
    """
    updated, public_document = copy.deepcopy(record), copy.deepcopy(document)
    asset = report["asset_id"]
    family = updated["families"][asset]
    row = next(row for row in public_document["assets"] if row["asset_id"] == asset)
    selected = [selection for rows in family["original_cells"].values() for selection in rows]
    originals = {mine.component_record(record, selection)[0]["path"] for selection in selected}
    donor = next(row for row in document["assets"] if row["asset_id"] == chemical.ASSET)
    originals.update(donor["authoring"]["originals"])
    source, _component = mine.component_record(record, family["original_cells"]["3"][0])
    row["source"] = {"status": "authored", "path": source["path"], "sha256": source["sha256"],
                     "provenance": "first_party_original_model_and_compatible_components", "rights": "first_party_generated_original"}
    row["authoring"].update(date="2026-10-16", status="complete_native_processor_source_runtime_pending",
                            originals=sorted(originals), source_record=RECORD.relative_to(ROOT).as_posix(),
                            atlas_manifest=paths(asset)[1].relative_to(ROOT).as_posix(), native_atlas=report["source"],
                            native_sha256=report["source_sha256"], staged_pose_count=512, nonempty_source_parts=512,
                            structurally_absent_parts=0, coverage_complete=True, runtime_activated=False, remaining=report["remaining"])
    family.update(status="complete_native_processor_source_runtime_pending", source_coverage_complete=True,
                  native_atlas=report["source"], native_sha256=report["source_sha256"],
                  atlas_manifest=paths(asset)[1].relative_to(ROOT).as_posix(), runtime_activated=False)
    validate_public_source(row, report)
    RECORD.write_text(json.dumps(updated, indent=2) + "\n", encoding="utf-8")
    PUBLIC.write_text(json.dumps(public_document, indent=2) + "\n", encoding="utf-8")


def main():
    """Regenerate or publish one selected processor after immutable original selection.

    @return None; all source/model/native checks precede publication.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset", choices=SELECTED, required=True)
    operation = parser.add_mutually_exclusive_group(required=True)
    operation.add_argument("--review", action="store_true")
    operation.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    record, document = json.loads(RECORD.read_text(encoding="utf-8")), json.loads(PUBLIC.read_text(encoding="utf-8"))
    public = next(row for row in document["assets"] if row["asset_id"] == args.asset)
    donor = next(row for row in document["assets"] if row["asset_id"] == chemical.ASSET)
    report, files = build(record, public, donor)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for path, payload in files.items():
        path.write_bytes(payload)
    if args.publish:
        publish(record, document, report)
    print(json.dumps({key: report[key] for key in ("asset_id", "source_slots", "source_coverage_complete", "runtime_activated")}))


if __name__ == "__main__":
    main()
