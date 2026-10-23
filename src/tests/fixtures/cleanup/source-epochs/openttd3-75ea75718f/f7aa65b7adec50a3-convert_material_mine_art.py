"""Convert selected material-mine originals through the existing native source contract.

@file convert_material_mine_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups28/29/31/32/34/35.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Each mineral owns genuine completed original apparatus/cargo. Compatible empty
construction apparatus and equipment glass retain explicit per-cell ancestry;
no copied whole industry, reserve/output ledger or native sprite range is added.
"""

import argparse
import copy
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_mine_states as coal_states


mine = coal_states.mine
construction = coal_states.construction
OUTPUT = mine.OUTPUT
NAMES = {"IORE": "iron", "CORE": "copper", "SALT": "salt", "LITH": "lithium", "COBT": "cobalt", "RARE": "rare-earth"}
STATES = coal_states.STATES


def prefix(public):
    """Resolve an existing selected resource's artifact basename without inventing native identity.

    @param public Existing shared selected material-mine family row.
    @return Stable resource-specific source artifact prefix.
    """
    resource = public["asset_id"].removeprefix("mine.")
    if resource not in NAMES:
        raise ValueError("material mine source is outside the dispatched resource scope")
    return NAMES[resource] + "-mine"


def validate_reuse(record, family, public):
    """Bind compatible individual components to exact native geometry and retained original ancestry.

    @param record Shared retained mine original document; existing donor fields remain unchanged.
    @param family Actual selected material-mine brief and explicit component graph.
    @param public Current installed resource's shared geometry/state contract.
    @return None after physical role, geometry and complete original-graph validation.
    """
    mine.validate_mine_contract(public)
    donor = record["families"]["mine.COAL"]
    expected = {"source_record": mine.RECORD.relative_to(mine.ROOT).as_posix(), "source_family": "mine.COAL",
                "physical_roles": donor["roles"], "native_cells": donor["native_cells"], "native_size": [64, 64],
                "anchor": [-31, -33], "source_height_limit": 20,
                "empty_construction_cells": donor["construction_cells"],
                "construction_component_geometry_sha256": donor["approved_construction_component_geometry_sha256"],
                "construction_reuse_sha256": donor["approved_construction_reuse_sha256"],
                "control_reuse_sha256": donor["approved_control_reuse_sha256"],
                "operating_component_geometry_sha256": donor["approved_operating_component_geometry_sha256"],
                "noncargo_base_cell": next(row for row in donor["base_cells"] if row["variant"] == 8)}
    # Reuse applies only to these identical physical roles/native cells. A
    # copied campus, foreign machine, Coal-bearing base or shifted loading
    # approach cannot supply a different mineral's missing source originals.
    if (family["asset_id"] != public["asset_id"] or family["compatible_components"] != expected
            or mine.signature(expected) != family["approved_compatible_components_sha256"]
            or family["roles"] != donor["roles"] or family["control_reuse"] != donor["control_reuse"]
            or family["approved_control_reuse_sha256"] != donor["approved_control_reuse_sha256"]):
        raise ValueError("material mine compatible physical component ancestry changed")
    for key in ("native_cells", "loading_cells", "native_size", "anchor", "source_height_limit", "footprint"):
        if family[key] != public[key]:
            raise ValueError("material mine source brief changes actual native " + key)
    registrations = {stage: {str(row["variant"]): mine.component_record(record, row)[1] for row in selections}
                     for stage, selections in expected["empty_construction_cells"].items()}
    if mine.signature(registrations) != expected["construction_component_geometry_sha256"]:
        raise ValueError("material mine original construction registration changed")


def base_cells(record, family, public):
    """Register all nine genuine completed cells against the actual installed mineral layout.

    @param record Retained selected completed originals and compatible physical components.
    @param family Executing mineral's frozen completed source selections.
    @param public Exact native geometry/state row for this mineral, never another mine's identity.
    @return Variant-indexed64x64 whole original cells including one exact compatible M1 hut.
    """
    validate_reuse(record, family, public)
    selections = family["base_cells"]
    variants = [selection["variant"] for selection in selections]
    if len(variants) != 8 or set(variants) != set(range(9)) - {7}:
        raise ValueError("material mine completed cell matrix is missing or ambiguous")
    registrations = {str(row["variant"]): mine.component_record(record, row)[1] for row in selections}
    if mine.signature(registrations) != family["approved_base_component_geometry_sha256"]:
        raise ValueError("material mine completed native registration changed")
    resource = public["asset_id"].removeprefix("mine.")
    contract = mine.mine_contract(resource)
    cells = {7: mine.control_cell(record)}
    # Every cargo-bearing apparatus needs genuine originals for this mineral.
    # Only the exact no-cargo filter is compatible completed machinery reuse.
    for selection in selections:
        source, _component = mine.component_record(record, selection)
        if selection["variant"] == 8:
            if selection != family["compatible_components"]["noncargo_base_cell"]:
                raise ValueError("material mine dust filter changes compatible original identity")
        elif source.get("mineral_resource") != resource:
            raise ValueError("material mine cannot relabel another mineral's cargo-bearing original")
        whole = mine.original_cell(record, selection, contract)
        mine.validate_cell(whole, selection["variant"])
        cells[selection["variant"]] = whole
    return cells


def stage_cells(record, family, public, completed):
    """Assemble actual per-cell stages from compatible empty apparatus and this mineral's base.

    @param record Retained mine originals with explicit original-stage provenance.
    @param family Selected mineral's approved individual component graph.
    @param public Installed native layout and per-cell stage ownership.
    @param completed Nine genuine completed cells for this mineral, unchanged at stage3.
    @return Four native stage-indexed mappings of complete individual cells.
    """
    foundation, unfinished_hut, finishing_hut = construction.reused_cells(record)
    contract = mine.mine_contract(public["asset_id"].removeprefix("mine."))
    stages = {0: {variant: foundation.copy() for variant in range(9)}, 1: {7: unfinished_hut},
              2: {7: finishing_hut}, 3: completed}
    # Reuse resolves each actual role separately from its retained empty
    # original. No whole donor industry or Coal stock enters this construction.
    for stage in (1, 2):
        selections = family["compatible_components"]["empty_construction_cells"][str(stage)]
        variants = [row["variant"] for row in selections]
        if len(variants) != 8 or set(variants) != set(range(9)) - {7}:
            raise ValueError("material mine compatible construction roles are missing or duplicated")
        for selection in selections:
            stages[stage][selection["variant"]] = construction.stage_original(record, selection, stage, contract)
    return stages


def review_board(poses, completed, title):
    """Expose all native states/phases and independent completed cells in one model-specific review.

    @param poses State/phase mappings of nine independently owned back/front source pairs.
    @param completed Actual completed mineral cells for independent native role review.
    @param title Native mineral resource identity for source-only captions.
    @return RGB state/cell comparison, never an ordinary-game rendering claim.
    """
    board = Image.new("RGB", (2080, 1768), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((12, 8), f"SOURCE ONLY: {title} fixed3x3 machinery;16 states/phases; separate loading floor untouched; packed/native game proof pending", fill="black")
    # Complete campuses disclose progression/static aliases, while the final
    # individual roles expose missing equipment and source-part registration.
    for state_index, state in enumerate(STATES):
        for frame in range(4):
            x, y = frame * 520 + 4, state_index * 356 + 40
            draw.text((x + 8, y), f"{state} frame{frame}" + (" static0 alias" if state in ("blocked", "depleted") else ""), fill="black")
            scene = mine.campus(poses[(state, frame)]).convert("RGBA").resize((512, 320), Image.Resampling.NEAREST)
            board.paste(scene, (x, y + 24), scene)
    draw.text((12, 1472), "Actual completed native machinery cells and independent original parts", fill="black")
    for variant in range(9):
        x = 12 + variant * 224
        draw.text((x, 1500), f"cell{variant} ({variant % 3},{variant // 3})", fill="black")
        whole = completed[variant].convert("RGBA").resize((128, 128), Image.Resampling.NEAREST)
        board.paste(whole, (x, 1520), whole)
        for index, sprite in enumerate(mine.source_parts(completed[variant])):
            draw.text((x + index * 96, 1652), mine.PARTS[index], fill="black")
            rgba = sprite.convert("RGBA")
            board.paste(rgba, (x + index * 96, 1672), rgba)
    return board


def encoded_png(image, indexed=True):
    """Encode deterministic source artifacts with owning-plan/architecture metadata.

    @param image Native indexed source or RGB review image.
    @param indexed Whether to reserve native palette index0 as transparency.
    @return PNG bytes without a filesystem mutation.
    """
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md selected material mines; archi-map-layers.md; source only")
    output = io.BytesIO()
    options = {"transparency": 0} if indexed else {}
    image.save(output, format="PNG", pnginfo=metadata, optimize=False, compress_level=9, **options)
    return output.getvalue()


def build(record, public):
    """Validate and convert the complete selected mineral source through existing native owners.

    @param record Retained model/component originals and approved native placement mappings.
    @param public Existing shared selected material-mine contract, with runtime still pending.
    @return Full288-slot source report and deterministic base/full/review/GIF artifact bytes.
    """
    name = prefix(public)
    family = record["families"][public["asset_id"]]
    completed = base_cells(record, family, public)
    stages = stage_cells(record, family, public, completed)
    lenses = coal_states.original_lenses(record)
    atlas = Image.new("P", (1152, 1024), 0)
    atlas.putpalette(mine.base.PALETTE_BYTES)
    base_atlas = Image.new("P", (1152, 64), 0)
    base_atlas.putpalette(mine.base.PALETTE_BYTES)
    poses, rows = {}, []
    # Native construction is per owned tile. Completed states retain genuine
    # mineral stock/apparatus and vary only the compatible control-hut glass.
    for state_index, state in enumerate(STATES):
        for frame in range(4):
            poses[(state, frame)] = {}
            for variant in range(9):
                whole = stages[frame if state == "construction" else 3][variant]
                # Only completed operating states use the control fitting;
                # unfinished stages and all other machinery remain exact.
                if state != "construction" and variant == 7:
                    whole = coal_states.apply_lens(whole, lenses[state][frame], [30, 43, 3, 3])
                mine.validate_cell(whole, variant)
                pair = mine.source_parts(whole)
                poses[(state, frame)][variant] = pair
                for index, (part, sprite) in enumerate(zip(mine.PARTS, pair)):
                    if sprite.getbbox() is None:
                        raise ValueError("material mine source lacks a real native source part")
                    x, y = (variant * 2 + index) * 64, (state_index * 4 + frame) * 64
                    atlas.paste(sprite, (x, y))
                    # The exact completed construction phase supplies the
                    # separate base reference; no extra ready-state authority.
                    if state == "construction" and frame == 3:
                        base_atlas.paste(sprite, (x, 0))
                    rows.append({"asset_id": public["asset_id"], "state": state, "frame": frame, "direction": "none",
                                 "native_layout": 0, "tile_variant": variant, "world_cell": [variant % 3, variant // 3],
                                 "part": part, "box": [x, y, 64, 64], "anchor": [-31, -33], "structurally_absent": False,
                                 "nonempty_pixels": sum(value != 0 for value in sprite.get_flattened_data()),
                                 "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest()})
    atlas_path = OUTPUT / f"{name}-source-authored.png"
    files = {atlas_path: encoded_png(atlas), OUTPUT / f"{name}-base-atlas.png": encoded_png(base_atlas),
             OUTPUT / f"{name}-base-campus.png": encoded_png(mine.campus(poses[("construction", 3)])),
             OUTPUT / f"{name}-source-review.png": encoded_png(review_board(poses, completed, public["asset_id"]), indexed=False)}
    frames = [mine.campus(poses[("working", frame)]).resize((768, 480), Image.Resampling.NEAREST) for frame in range(4)]
    output = io.BytesIO()
    frames[0].save(output, format="GIF", save_all=True, append_images=frames[1:], duration=500, loop=0, transparency=0,
                   disposal=2, optimize=False, comment=b"r2mlvs14 selected material mine; archi-map-layers.md; source comparison only")
    files[OUTPUT / f"{name}-source-working.gif"] = output.getvalue()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md selected material mines; archi-map-layers.md",
              "asset_id": public["asset_id"], "source": atlas_path.relative_to(mine.ROOT).as_posix(),
              "source_sha256": hashlib.sha256(files[atlas_path]).hexdigest(), "source_coverage_complete": True, "runtime_activated": False,
              "directions": ["none"], "native_layout": 0, "native_cells": public["native_cells"], "loading_cells": public["loading_cells"],
              "states": list(STATES), "frames": 4, "native_size": [64, 64], "anchor": [-31, -33], "source_height_limit": 20,
              "source_slots": 288, "nonempty_source_parts": 288, "structurally_absent_parts": 0, "source_parts": rows,
              "base_nonempty_parts": 18, "base_cells": family["base_cells"], "compatible_components": family["compatible_components"],
              "native_state_policy": public["native_state_policy"],
              "stock_policy": "Completed apparatus/floor/mineral cargo stay exact outside the actual hut fitting; depletion preserves final stored output",
              "remaining": "Master10.4D selected packing/native binding/decoded pixels/ordinary game proof"}
    report["artifacts"] = [{"path": path.relative_to(mine.ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items()]
    files[OUTPUT / f"{name}-source-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def validate_public_source(public, report):
    """Bind full authored source claims to exact live native atlas and retained original identity.

    @param public Existing shared selected mine row after source publication.
    @param report Fresh complete source report for this same actual mineral family.
    @return None; false counts, foreign identities or native activation refuse.
    """
    mine.validate_mine_contract(public)
    name, authoring = prefix(public), public.get("authoring", {})
    # Source completeness names exact actual owners and payloads; metadata
    # agreement alone cannot authorize a foreign atlas, missing cells or range.
    if (report["asset_id"] != public["asset_id"] or report["source_slots"] != 288
            or report["nonempty_source_parts"] != 288 or report["structurally_absent_parts"] != 0
            or report["source"] != (OUTPUT / f"{name}-source-authored.png").relative_to(mine.ROOT).as_posix()
            or public["source"]["status"] != "authored" or not authoring.get("coverage_complete")
            or authoring.get("native_atlas") != report["source"] or authoring.get("native_sha256") != report["source_sha256"]
            or authoring.get("atlas_manifest") != (OUTPUT / f"{name}-source-manifest.json").relative_to(mine.ROOT).as_posix()
            or authoring.get("staged_pose_count") != 288 or authoring.get("nonempty_source_parts") != 288
            or authoring.get("structurally_absent_parts") != 0 or authoring.get("runtime_activated") is not False
            or public["packed"]["offset"] is not None or public["packed"]["sha256"] is not None):
        raise ValueError("material mine public source coverage or exact native atlas binding changed")
    if mine.base.digest(mine.base.repository_path(public["source"]["path"])) != public["source"].get("sha256"):
        raise ValueError("material mine public original source changed")
    if mine.base.digest(mine.base.repository_path(report["source"])) != report["source_sha256"]:
        raise ValueError("material mine public live native atlas changed")


def publish_source(record, document, report):
    """Publish only the executing mineral's full source through the existing shared family owner.

    @param record Retained selected model/component document; donor families remain untouched.
    @param document Existing public source contract; all other family rows stay exact.
    @param report Complete validated original source report with native/game work still pending.
    @return None; no packed sprite allocation or native rendering activation occurs.
    """
    updated, public_document = copy.deepcopy(record), copy.deepcopy(document)
    family = updated["families"][report["asset_id"]]
    row = next(row for row in public_document["assets"] if row["asset_id"] == report["asset_id"])
    base_source, _component = mine.component_record(updated, family["base_cells"][0])
    selected = {entry["source"] for entry in family["base_cells"]}
    # Every selected per-cell construction ancestor belongs in the handoff;
    # a base filename alone cannot communicate complete288-slot provenance.
    for selections in family["compatible_components"]["empty_construction_cells"].values():
        selected.update(entry["source"] for entry in selections)
    selected.add(updated["families"]["mine.COAL"]["operating_component_geometry"]["depleted_selection"]["source"])
    originals = [source["path"] for source in updated["candidates"] if source["id"] in selected]
    originals.extend([family["control_reuse"]["source_path"], "media/multilayer/source/m1/raw/facility-status-phases-v1.png"])
    row["source"] = {"status": "authored", "path": base_source["path"], "sha256": base_source["sha256"],
                     "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
    row.setdefault("authoring", {}).update(date="2026-10-12", status="complete_native_mine_source_runtime_pending", originals=sorted(set(originals)),
                                           source_record=mine.RECORD.relative_to(mine.ROOT).as_posix(),
                                           atlas_manifest=(OUTPUT / f"{prefix(row)}-source-manifest.json").relative_to(mine.ROOT).as_posix(),
                                           native_atlas=report["source"], native_sha256=report["source_sha256"], staged_pose_count=288,
                                           nonempty_source_parts=288, structurally_absent_parts=0, selected_base_nonempty_parts=18,
                                           coverage_complete=True, runtime_activated=False, remaining=report["remaining"])
    family.update(status="complete_native_mine_source_runtime_pending", source_coverage_complete=True, runtime_activated=False,
                  atlas_manifest=row["authoring"]["atlas_manifest"], native_atlas=report["source"], native_sha256=report["source_sha256"], remaining=report["remaining"])
    validate_public_source(row, report)
    mine.RECORD.write_text(json.dumps(updated, indent=2) + "\n", encoding="utf-8")
    mine.PUBLIC.write_text(json.dumps(public_document, indent=2) + "\n", encoding="utf-8")


def validate_following_source(public):
    """Recognize later completed material sources in older focused preservation fixtures.

    @param public Existing shared family row whose original metadata may predate its own dispatch.
    @return True only for a selected material mine with a valid complete live source handoff.
    @throws ValueError When a claimed complete selected source lacks exact native/payload evidence.
    """
    resource = public["asset_id"].removeprefix("mine.")
    # Older owner fixtures preserve unrelated rows. Only a subsequent actual
    # selected source delivery may supersede its old planned metadata; pending
    # or foreign families still require their original exact baseline identity.
    if resource not in NAMES or public.get("authoring", {}).get("coverage_complete") is not True:
        return False
    path = OUTPUT / f"{prefix(public)}-source-manifest.json"
    report = json.loads(path.read_text(encoding="utf-8"))
    validate_public_source(public, report)
    return True


def main():
    """Convert/publish one explicitly selected material mine without changing native packed routes.

    @return None; all original/cell/ancestry/native validation precedes artifact publication.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--resource", choices=tuple(NAMES), required=True)
    operation = parser.add_mutually_exclusive_group(required=True)
    operation.add_argument("--review", action="store_true")
    operation.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    record = json.loads(mine.RECORD.read_text(encoding="utf-8"))
    document = json.loads(mine.PUBLIC.read_text(encoding="utf-8"))
    public = next(row for row in document["assets"] if row["asset_id"] == "mine." + args.resource)
    report, files = build(record, public)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for path, payload in files.items():
        path.write_bytes(payload)
    if args.publish:
        publish_source(record, document, report)
    print(json.dumps({key: report[key] for key in ("asset_id", "source_slots", "nonempty_source_parts", "source_coverage_complete", "runtime_activated")}))


if __name__ == "__main__":
    main()
