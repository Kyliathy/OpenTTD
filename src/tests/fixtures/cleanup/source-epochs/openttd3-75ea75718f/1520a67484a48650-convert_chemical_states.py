"""Convert genuine chemical works construction and original status material into native source poses.

@file convert_chemical_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group40C/task40.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Industry retains all physical stock, recipe/work and synchronized state. These
512 source parts grant no packed range or ordinary-game rendering authority.
"""

import argparse
import copy
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_chemical_base as plant
import convert_mine_construction as construction
from convert_mine_states import apply_lens


mine = plant.mine
ROOT, RECORD, PUBLIC, OUTPUT, ASSET = plant.ROOT, plant.RECORD, plant.PUBLIC, plant.OUTPUT, plant.ASSET
PREFIX = "chemical-works-source"
STATES = ("construction", "ready", "working", "blocked")
NEW_VARIANTS = plant.NEW_VARIANTS
COMPATIBLE = plant.DONORS
MANIFEST = OUTPUT / f"{PREFIX}-manifest.json"


def validate_preserved_identity(row):
    """Keep source artifacts exact while accepting the separately verified requested demo rebuild.

    @param row Immutable source-closure path/hash from the retained preservation baseline.
    @return None; only four named game/test binaries may use an exact documented transition.
    """
    actual = mine.base.digest(ROOT / row["path"])
    if actual == row["sha256"]:
        return
    allowed = {f"build/{configuration}/{name}.exe" for configuration in ("Debug", "RelWithDebInfo")
               for name in ("openttd", "openttd_test")}
    if row["path"] not in allowed:
        raise ValueError("preserved chemical source artifact changed: " + row["path"])
    receipt = json.loads((ROOT / "logs/underground-demo-verification.json").read_text(encoding="utf-8"))
    family = json.loads(RECORD.read_text(encoding="utf-8"))["families"][ASSET]
    transitions = receipt.get("binary_transitions", [])
    if (receipt.get("both_configurations_live_continuation_passed") is not True
            or receipt.get("chemical_full_source_sha256") != family["native_sha256"]
            or not any(item["path"] == row["path"] and item["before_sha256"] == row["sha256"]
                       and item["after_sha256"] == actual for item in transitions)):
        raise ValueError("chemical source lacks the exact later demo binary transition: " + row["path"])


def expected_construction_reuse(record):
    """Bind each compatible chemical cell to an actual same-stage chemical works original.

    @param record Existing shared processor originals and completed chemical works source.
    @return Exact individual apparatus, local alias, foundation and hut ancestry.
    """
    metal, graph = record["families"][plant.shared.ASSET], []
    for stage in (1, 2):
        for target, source_variant in COMPATIBLE:
            selection = next((row for row in metal["construction_cells"][str(stage)] if row["variant"] == source_variant), None)
            source_family, source_record = plant.shared.ASSET, record
            if selection is None:
                inherited = next(row for row in metal["construction_reuse"]["apparatus"]
                                 if row["stage"] == stage and row["target_variant"] == source_variant)
                selection = inherited["source_selection"]
                source_family, source_record = inherited["source_family"], plant.shared.donor_record()
            source, component = mine.component_record(source_record, selection)
            graph.append({"stage": stage, "target_variant": target, "target_world_cell": [target % 4, target // 4],
                          "source_family": source_family, "source_selection": selection,
                          "source_sha256": source["sha256"], "component_geometry_sha256": mine.signature(component),
                          "physical_role": plant.ROLES[target], "inventory": "Same-stage compatible empty individual apparatus; no cargo or work"})
    return {"apparatus": graph, "local_aliases": {"8": 5, "10": 3},
            "foundation_targets": list(range(16)), "hut_target": 11,
            "donor_m1_reuse": copy.deepcopy(metal["construction_reuse"]["donor_m1_reuse"])}


def stage_cells(record, public):
    """Assemble actual per-tile construction0..3 from genuine stage-specific originals.

    @param record Approved completed model, fourteen new unfinished cells and frozen individual reuse.
    @param public Actual compiled single-layout sixteen-cell processor contract.
    @return Four dictionaries of whole indexed native64x64 apparatus at unchanged contacts.
    """
    family, donor = record["families"][ASSET], plant.shared.donor_record()
    reuse = expected_construction_reuse(record)
    # Target role, original identity and native stage jointly authorize reuse;
    # neither a convenient whole campus nor a finishing-stage relabel can do so.
    if (family["construction_reuse"] != reuse
            or mine.signature(reuse) != family["approved_construction_reuse_sha256"]):
        raise ValueError("processor construction compatible stage ancestry changed")
    selections = family["construction_cells"]
    if set(selections) != {"1", "2"}:
        raise ValueError("processor construction stage matrix is incomplete")
    registrations = {stage: {str(row["variant"]): mine.component_record(record, row)[1] for row in rows}
                     for stage, rows in selections.items()}
    if mine.signature(registrations) != family["approved_construction_component_geometry_sha256"]:
        raise ValueError("processor construction original registration changed")
    # Refuse an incomplete or relabelled matrix before expensive conversion.
    # A recomputed registration hash cannot authorize a missing real stage.
    for stage, rows in selections.items():
        variants = [row["variant"] for row in rows]
        if len(variants) != 7 or set(variants) != set(NEW_VARIANTS):
            raise ValueError("processor construction new apparatus matrix is missing or ambiguous")
        for selection in rows:
            source, component = mine.component_record(record, selection)
            if (source.get("processor_asset_id") != ASSET or source.get("unfinished_empty") is not True
                    or source.get("construction_stage") != int(stage) or component.get("construction_stage") != int(stage)):
                raise ValueError("processor construction source lacks its genuine unfinished stage owner")
    completed = plant.completed_cells(record, public)
    foundation, unfinished_hut, finishing_hut = construction.reused_cells(donor)
    stages = {0: {variant: foundation.copy() for variant in range(16)}, 3: completed}
    # Each original tile owns its actual native stage. A completed neighbouring
    # cell remains stage3 while whole-industry construction is still unfinished.
    for stage in (1, 2):
        rows = selections[str(stage)]
        cells = {11: (unfinished_hut if stage == 1 else finishing_hut).copy()}
        # New chemical works-only apparatus must have genuine empty unfinished
        # originals, not erased completed bodies or another family's art.
        for selection in rows:
            source, _component = mine.component_record(record, selection)
            if source.get("processor_asset_id") != ASSET or source.get("unfinished_empty") is not True:
                raise ValueError("processor construction source lacks its genuine unfinished owner")
            cells[selection["variant"]] = construction.stage_original(record, selection, stage, plant.shared.processor_contract(ASSET))
        # Donor cells retain their original local geometry during conversion;
        # only the explicit compatible target mapping changes their world offset.
        for row in reuse["apparatus"]:
            if row["stage"] == stage:
                source_record = record if row["source_family"] == plant.shared.ASSET else donor
                contract = (plant.shared.processor_contract(plant.shared.ASSET)
                            if source_record is record else mine.mine_contract("COAL"))
                cells[row["target_variant"]] = construction.stage_original(source_record, row["source_selection"], stage, contract)
        cells[8], cells[10] = cells[5].copy(), cells[3].copy()
        for variant, sprite in cells.items():
            mine.validate_cell(sprite, variant)
        stages[stage] = cells
    return stages


def operating_lenses(record, public):
    """Select exact original M1 glass phases at the identical existing native control fitting.

    @param record Frozen processor lamp registration and compatible original-control ancestry.
    @param public Actual construction/static/synchronized-working phase policy.
    @return Ready/working/blocked four-phase glass lists, with static states aliasing original phase0.
    """
    family = record["families"][ASSET]
    donor = json.loads(mine.facilities.RECORD.read_text(encoding="utf-8"))
    component = next(row for row in donor["components"] if row["key"] == "facility-status")
    expected = {"hut_variant": 11, "world_cell": [3, 2], "cue_target": [30, 43, 3, 3],
                "source_record": mine.facilities.RECORD.relative_to(ROOT).as_posix(), "component": "facility-status",
                "component_path": component["path"], "component_sha256": component["sha256"],
                "component_geometry_sha256": mine.signature(component), "states": ["ready", "working", "blocked"],
                "frame_aliases": public["native_state_policy"]["frame_aliases"],
                "quantity_owner": public["native_state_policy"]["quantity_owner"]}
    # The existing physical hut fitting and native phase aliases own these
    # pixels; adding a second clock, larger lamp or stock cue would change scope.
    if (family["operating_component_geometry"] != expected
            or mine.signature(expected) != family["approved_operating_component_geometry_sha256"]
            or component["sha256"] != family["construction_reuse"]["donor_m1_reuse"]["phase_component_sha256"]):
        raise ValueError("processor operating original glass or native ownership changed")
    phases = mine.facilities.indicator(component)
    return {"ready": [phases[1][0]] * 4, "working": phases[2], "blocked": [phases[3][0]] * 4}


def encoded_png(image, indexed=True):
    """Encode deterministic source or review bytes with explicit plan/architecture ownership.

    @param image Indexed native source or RGB diagnostic comparison.
    @param indexed Whether palette index0 owns transparency.
    @return PNG bytes; no original or source pixel is painted by encoding.
    """
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md Group40C/task40.3; archi-map-layers.md; source only")
    buffer = io.BytesIO()
    options = {"transparency": 0} if indexed else {}
    image.save(buffer, format="PNG", pnginfo=metadata, optimize=False, compress_level=9, **options)
    return buffer.getvalue()


def review_board(poses):
    """Expose all native source states/phases and their exact static aliases.

    @param poses State/phase to sixteen independent native original back/front pairs.
    @return RGB source comparison, never native world sorting or ordinary-game proof.
    """
    board = Image.new("RGB", (2608, 1664), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((16, 8), "SOURCE ONLY: fixed4x4 chemical works;512 parts; native per-tile construction; ready/blocked static0; working original4 phases", fill="black")
    draw.text((16, 24), "Industry retains actual stock/selected recipe/work. Only existing3x3 hut glass changes after completion. Packing/game display pending.", fill="black")
    # Native phases are compared side by side so aliased idle/full states and
    # the genuine synchronized-working glass remain visible without a new clock.
    for state_index, state in enumerate(STATES):
        for frame in range(4):
            x, y = 16 + frame * 648, 56 + state_index * 400
            draw.text((x, y), f"{state} / native phase{frame}", fill="black")
            scene = plant.shared.campus(poses[(state, frame)]).convert("RGBA").resize((640, 384), Image.Resampling.NEAREST)
            board.paste(scene, (x, y + 16), scene)
    return board


def construction_board(stages):
    """Show all sixty-four independently owned construction cells at native contacts.

    @param stages Genuine whole original sprites keyed by native stage0..3 and actual cell0..15.
    @return RGB source-stage/contact review; complete original floors remain visible.
    """
    board = Image.new("RGB", (2080, 704), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((16, 8), "SOURCE construction0..3 x16 actual cells: original paid foundation / unfinished / finishing / exact completed base", fill="black")
    # Independent cell rows reveal missing apparatus, native contact drift or
    # mistaken stage relabels that a composed campus could obscure.
    for stage in range(4):
        for variant in range(16):
            x, y = 16 + variant * 128, 40 + stage * 164
            draw.text((x, y), f"stage{stage} cell{variant}", fill="black")
            sprite = stages[stage][variant].convert("RGBA").resize((128, 128), Image.Resampling.NEAREST)
            board.paste(sprite, (x, y + 24), sprite)
    return board


def build(record, public):
    """Build all512 actual native source slots from retained original stages and equipment glass.

    @param record Frozen completed/construction source selections and original-component ancestry.
    @param public Compiled actual one-layout processor family contract.
    @return Full source report and deterministic atlas/review/phase-GIF/manifest payloads.
    """
    stages, lenses = stage_cells(record, public), operating_lenses(record, public)
    atlas = Image.new("P", (2048, 1024), 0)
    atlas.putpalette(mine.base.PALETTE_BYTES)
    poses, rows = {}, []
    # Native construction selects the actual tile stage; completed operating
    # apparatus/floors stay exact while only the existing control glass changes.
    for state_index, state in enumerate(STATES):
        for frame in range(4):
            poses[(state, frame)] = {}
            for variant in range(16):
                whole = stages[frame if state == "construction" else 3][variant]
                if state != "construction" and variant == 11:
                    whole = apply_lens(whole, lenses[state][frame], [30, 43, 3, 3])
                pair = mine.source_parts(whole)
                poses[(state, frame)][variant] = pair
                for index, (part, sprite) in enumerate(zip(mine.PARTS, pair)):
                    if sprite.getbbox() is None:
                        raise ValueError("processor source lacks an actual original native part")
                    x, y = (variant * 2 + index) * 64, (state_index * 4 + frame) * 64
                    atlas.paste(sprite, (x, y))
                    rows.append({"asset_id": ASSET, "state": state, "frame": frame, "direction": "none", "native_layout": 0,
                                 "tile_variant": variant, "world_cell": public["native_cells"][variant], "part": part,
                                 "box": [x, y, 64, 64], "anchor": [-31, -33], "structurally_absent": False,
                                 "nonempty_pixels": sum(value != 0 for value in sprite.get_flattened_data()),
                                 "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest()})
    path = OUTPUT / f"{PREFIX}-authored.png"
    files = {path: encoded_png(atlas), OUTPUT / f"{PREFIX}-review.png": encoded_png(review_board(poses), False),
             OUTPUT / f"{PREFIX}-construction-cells.png": encoded_png(construction_board(stages), False)}
    frames = [plant.shared.campus(poses[("working", frame)]).resize((960, 576), Image.Resampling.NEAREST) for frame in range(4)]
    buffer = io.BytesIO()
    frames[0].save(buffer, format="GIF", save_all=True, append_images=frames[1:], duration=500, loop=0,
                   transparency=0, disposal=2, optimize=False,
                   comment=b"r2mlvs14 Group40C; archi-map-layers.md; source-only four-phase comparison")
    files[OUTPUT / f"{PREFIX}-working.gif"] = buffer.getvalue()
    family = record["families"][ASSET]
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md Group40C/task40.3; archi-map-layers.md",
              "asset_id": ASSET, "source_coverage_complete": True, "runtime_activated": False,
              "source": path.relative_to(ROOT).as_posix(), "source_sha256": hashlib.sha256(files[path]).hexdigest(),
              "directions": ["none"], "states": list(STATES), "frames": 4, "native_layout": 0,
              "native_size": [64, 64], "anchor": [-31, -33], "footprint": [4, 4], "tile_variants": 16,
              "source_slots": 512, "nonempty_source_parts": 512, "structurally_absent_parts": 0, "source_parts": rows,
              "native_state_policy": public["native_state_policy"], "construction_cells": family["construction_cells"],
              "construction_reuse": family["construction_reuse"], "operating_component_geometry": family["operating_component_geometry"],
              "inventory_policy": family["inventory_policy"],
              "remaining": "Master10.4D selected packing/native binding/decoded pixels/ordinary game proof"}
    report["artifacts"] = [{"path": target.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for target, payload in files.items()]
    files[MANIFEST] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def validate_public_source(public, report):
    """Bind full public source coverage to the exact live512-part matrix and immutable original.

    @param public Executing shared chemical works source row, with packed/runtime activation still absent.
    @param report Fresh full source report or its exact retained live source manifest.
    @return None after live payload, original identity, complete matrix and every pixel-row check.
    """
    plant.shared.validate_processor_contract(public)
    authoring = public.get("authoring", {})
    # A plausible count, foreign atlas or early range cannot stand in for the
    # complete actual source matrix and the existing native geometry/state owner.
    if (report.get("asset_id") != ASSET or report.get("source_slots") != 512
            or report.get("nonempty_source_parts") != 512 or report.get("structurally_absent_parts") != 0
            or report.get("source_coverage_complete") is not True or report.get("runtime_activated") is not False
            or report.get("source") != (OUTPUT / f"{PREFIX}-authored.png").relative_to(ROOT).as_posix()
            or report.get("native_state_policy") != public["native_state_policy"]
            or public["source"]["status"] != "authored" or authoring.get("coverage_complete") is not True
            or authoring.get("native_atlas") != report["source"] or authoring.get("native_sha256") != report.get("source_sha256")
            or authoring.get("atlas_manifest") != MANIFEST.relative_to(ROOT).as_posix()
            or authoring.get("staged_pose_count") != 512 or authoring.get("nonempty_source_parts") != 512
            or authoring.get("structurally_absent_parts") != 0 or authoring.get("runtime_activated") is not False
            or public["packed"]["offset"] is not None or public["packed"]["sha256"] is not None):
        raise ValueError("processor complete public source or exact live atlas binding changed")
    if mine.base.digest(mine.base.repository_path(public["source"]["path"])) != public["source"].get("sha256"):
        raise ValueError("processor public immutable original changed")
    if mine.base.digest(mine.base.repository_path(report["source"])) != report["source_sha256"]:
        raise ValueError("processor public native atlas changed")
    if json.loads(MANIFEST.read_text(encoding="utf-8")) != report:
        raise ValueError("processor full source manifest differs from its exact live handoff")
    rows = report.get("source_parts", [])
    expected = {(state, frame, variant, part) for state in STATES for frame in range(4) for variant in range(16) for part in mine.PARTS}
    actual = {(row["state"], row["frame"], row["tile_variant"], row["part"]) for row in rows}
    if len(rows) != 512 or actual != expected:
        raise ValueError("processor full source matrix is missing or ambiguous")
    with Image.open(ROOT / report["source"]) as atlas:
        # Native source coverage requires the same palette, canvas and alpha
        # owner as the actual renderer; an opaque or resized picture cannot
        # replace independently registered native source parts.
        if atlas.mode != "P" or atlas.size != (2048, 1024) or atlas.info.get("transparency") != 0:
            raise ValueError("processor full source loses native palette/canvas/transparency")
        # Each complete source slot must own the exact ordered native cell,
        # anchor, nonempty original pixels and real atlas rectangle it declares.
        for row in rows:
            variant = row["tile_variant"]
            x = (variant * 2 + mine.PARTS.index(row["part"])) * 64
            y = (STATES.index(row["state"]) * 4 + row["frame"]) * 64
            sprite = atlas.crop((x, y, x + 64, y + 64))
            count = sum(value != 0 for value in sprite.get_flattened_data())
            if (row["asset_id"] != ASSET or row["direction"] != "none" or row["native_layout"] != 0
                    or row["world_cell"] != [variant % 4, variant // 4] or row["anchor"] != [-31, -33]
                    or row["box"] != [x, y, 64, 64] or row["structurally_absent"] is not False or count == 0
                    or row["nonempty_pixels"] != count or row["source_pixels_sha256"] != hashlib.sha256(sprite.tobytes()).hexdigest()):
                raise ValueError("processor full source slot changes native ownership or actual pixels")


def publish_source(record, document, report):
    """Publish complete source through the existing shared owner without assigning sprite ranges.

    @param record Current retained original selections; completed-base ancestry stays intact.
    @param document Shared actual asset manifest; only the executing family source row changes.
    @param report Complete validated512-slot source report and live artifacts.
    @return None; native gameplay quantities, packed data and activation remain unchanged.
    """
    updated, public_document = copy.deepcopy(record), copy.deepcopy(document)
    family = updated["families"][ASSET]
    row = next(row for row in public_document["assets"] if row["asset_id"] == ASSET)
    selected = {entry["source"] for entry in family["base_cells"]}
    # Publication exposes every selected new construction source and all
    # original compatible apparatus/control references; base-only filenames
    # cannot conceal missing native stages or substitute for phase originals.
    for selections in family["construction_cells"].values():
        selected.update(entry["source"] for entry in selections)
    originals = [source["path"] for source in updated["candidates"] if source["id"] in selected]
    for reuse in family["construction_reuse"]["apparatus"]:
        source_record = updated if reuse["source_family"] == plant.shared.ASSET else plant.shared.donor_record()
        source, _component = mine.component_record(source_record, reuse["source_selection"])
        originals.append(source["path"])
    for reuse in family["reuse_graph"]:
        if reuse["kind"] == "compatible_processor_original":
            source, _component = mine.component_record(updated, reuse["source_selection"])
            originals.append(source["path"])
    originals.extend([family["construction_reuse"]["donor_m1_reuse"]["source_path"],
                      family["operating_component_geometry"]["component_path"]])
    row["authoring"].update(date="2026-10-12", status="complete_native_processor_source_runtime_pending",
                            originals=sorted(set(originals)), source_record=RECORD.relative_to(ROOT).as_posix(),
                            atlas_manifest=MANIFEST.relative_to(ROOT).as_posix(), native_atlas=report["source"],
                            native_sha256=report["source_sha256"], staged_pose_count=512, nonempty_source_parts=512,
                            structurally_absent_parts=0, coverage_complete=True, runtime_activated=False, remaining=report["remaining"])
    family.update(status="complete_native_processor_source_runtime_pending", source_coverage_complete=True,
                  atlas_manifest=MANIFEST.relative_to(ROOT).as_posix(), native_atlas=report["source"],
                  native_sha256=report["source_sha256"], runtime_activated=False, remaining=report["remaining"])
    validate_public_source(row, report)
    RECORD.write_text(json.dumps(updated, indent=2) + "\n", encoding="utf-8")
    PUBLIC.write_text(json.dumps(public_document, indent=2) + "\n", encoding="utf-8")


def main():
    """Review or publish only this complete chemical works source through actual original/native ownership.

    @return None; deterministic source validation precedes every source metadata mutation.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    operation = parser.add_mutually_exclusive_group(required=True)
    operation.add_argument("--review", action="store_true")
    operation.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    record, document = json.loads(RECORD.read_text(encoding="utf-8")), json.loads(PUBLIC.read_text(encoding="utf-8"))
    public = next(row for row in document["assets"] if row["asset_id"] == ASSET)
    report, files = build(record, public)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for path, payload in files.items():
        path.write_bytes(payload)
    if args.publish:
        publish_source(record, document, report)
    print(json.dumps({key: report[key] for key in ("asset_id", "source_slots", "nonempty_source_parts", "source_coverage_complete", "runtime_activated")}))


if __name__ == "__main__":
    main()
