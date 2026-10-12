"""Convert genuine Coal construction originals using the actual native stage owner.

@file convert_mine_construction.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group27C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
One paid foundation and compatible hut reuse retained M1 originals; eight actual
machinery roles have separate genuine unfinished/finishing originals. This source
comparison neither creates work/ground nor binds native sprite ranges.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_mine_art as mine


OUTPUT = mine.OUTPUT
PREFIX = "coal-mine-construction"


def reused_cells(record):
    """Reproduce compatible foundation/huts from their exact retained M1 ancestry.

    @param record Mine document with frozen construction reuse and base ancestry.
    @return Native foundation, unfinished hut and original finishing-glass hut.
    @throws ValueError Before publication on changed ancestry or foreign geometry.
    """
    family = record["families"]["mine.COAL"]
    reuse = family["construction_reuse"]
    # Only a generic one-cell paid yard and the same physical control hut are
    # compatible. Other Earth machinery would introduce foreign Coal equipment.
    if (mine.signature(reuse) != family["approved_construction_reuse_sha256"]
            or reuse["source_record"] != mine.facilities.RECORD.relative_to(mine.ROOT).as_posix()
            or reuse["source_family"] != "facility.earth-receiver"
            or reuse["source_state"] != "construction" or reuse["source_frame"] != 0
            or reuse["foundation_source_variant"] != 4 or reuse["hut_source_variant"] != 5
            or reuse["foundation_targets"] != list(range(9)) or reuse["hut_target"] != 7
            or reuse["native_canvas"] != [64, 64] or reuse["anchor"] != [-31, -33]
            or reuse["phase_component"] != "facility-status"
            or reuse["cue_target"] != [30, 43, 3, 3]
            or reuse["finishing_hut_body_state"] != "ready"
            or reuse["finishing_hut_glass_state"] != "construction"
            or reuse["finishing_hut_glass_frame"] != 2):
        raise ValueError("mine construction reuse changes compatible geometry or stage ancestry")
    donor = json.loads(mine.facilities.RECORD.read_text(encoding="utf-8"))
    source = next(row for row in donor["families"] if row["asset_id"] == reuse["source_family"])
    component = next(row for row in donor["components"] if row["key"] == reuse["phase_component"])
    # Raw identity and actual native pixels must agree. A trusted caption or
    # new checksum on a different donor cannot authorize construction reuse.
    if (source["path"] != reuse["source_path"] or source["sha256"] != reuse["source_sha256"]
            or component["sha256"] != reuse["phase_component_sha256"]
            or source["cue_targets"][5] != reuse["cue_target"]
            or mine.base.digest(mine.base.repository_path(reuse["source_native_atlas"])) != reuse["source_native_sha256"]):
        raise ValueError("mine construction original/native ancestry changed")
    mine.facilities.verify_original(source)
    cells, _boxes = mine.facilities.select_cells(source)
    phases = mine.facilities.indicator(component)
    natives = []
    # These two exact source cells are the compatible components, never a
    # copied Earth industry or its soil-handling apparatus/loading footprint.
    for variant in (4, 5):
        pose = mine.native_housing(cells[0][variant])
        pose = mine.facilities.insert_indicator(pose, phases[0][0], source["cue_targets"][variant])
        native = mine.base.quantize(pose, {})
        with Image.open(mine.base.repository_path(reuse["source_native_atlas"])) as atlas:
            x = variant * 128
            expected = atlas.crop((x, 0, x + 64, 64))
        if native.tobytes() != expected.tobytes():
            raise ValueError("mine construction reuse does not reproduce exact M1 native pixels")
        mine.validate_cell(native, variant)
        natives.append(native)
    pose = mine.native_housing(cells[1][5])
    finishing = mine.base.quantize(mine.facilities.insert_indicator(pose, phases[0][2], reuse["cue_target"]), {})
    completed = mine.control_cell(record)
    # Finishing/commissioning changes only genuine original glass at the
    # existing opaque fitting. The completed physical hut remains identical.
    for y in range(64):
        for x in range(64):
            if not (30 <= x < 33 and 43 <= y < 46):
                if finishing.getpixel((x, y)) != completed.getpixel((x, y)):
                    raise ValueError("mine finishing hut changes pixels outside original glass")
    mine.validate_cell(finishing, 7)
    return *natives, finishing


def stage_original(record, selection, stage, contract):
    """Resolve one genuine unfinished apparatus with an explicit native stage identity.

    @param record Retained raw originals and immutable registered component mappings.
    @param selection Actual native machinery cell/source/component identity.
    @param stage Actual construction stage1 or2, never a renderer-owned work counter.
    @param contract Compiled fixed mine layout, source canvas and native placement.
    @return Whole original native cell, validated before splitting its source parts.
    """
    source, component = mine.component_record(record, selection)
    # Relabeling a completed/other-stage source would hide missing construction
    # artwork even when its pixels and source window happen to be valid.
    if source.get("construction_stage") != stage or component.get("construction_stage") != stage:
        raise ValueError("mine construction original is assigned to the wrong native stage")
    whole = mine.original_cell(record, selection, contract)
    mine.validate_cell(whole, selection["variant"])
    return whole


def stage_cells(record, public):
    """Assemble every actual per-cell stage while preserving the approved completed model.

    @param record Mine originals with exact compatible reuse and both eight-role matrices.
    @param public Existing shared mine.COAL source contract and native state policy.
    @return Stage-indexed native whole-cell mappings for all nine machinery owners.
    """
    _report, base_files = mine.build(record, public)
    family = record["families"]["mine.COAL"]
    selections = family["construction_cells"]
    if set(selections) != {"1", "2"}:
        raise ValueError("mine construction lacks the genuine unfinished/finishing stage matrix")
    registrations = {}
    # Every new machinery role needs its own genuine original in both phases;
    # duplicate cells or a copied foundation cannot conceal missing apparatus.
    for stage, rows in selections.items():
        variants = [row["variant"] for row in rows]
        if len(variants) != 8 or set(variants) != set(range(9)) - {7}:
            raise ValueError("mine construction native machinery matrix is missing or ambiguous")
        registrations[stage] = {str(row["variant"]): mine.component_record(record, row)[1] for row in rows}
    if mine.signature(registrations) != family["approved_construction_component_geometry_sha256"]:
        raise ValueError("mine construction native original registration changed")
    foundation, unfinished_hut, finishing_hut = reused_cells(record)
    contract = mine.mine_contract("COAL")
    stages = {0: {variant: foundation.copy() for variant in range(9)}, 1: {7: unfinished_hut}, 2: {7: finishing_hut}, 3: {}}
    # Construction is per actual machinery tile. A completed tile can display
    # stage3 while a different owned tile still blocks industry commissioning.
    for stage in (1, 2):
        for selection in selections[str(stage)]:
            stages[stage][selection["variant"]] = stage_original(record, selection, stage, contract)
    with Image.open(io.BytesIO(base_files[OUTPUT / "coal-mine-base-atlas.png"])) as atlas:
        for variant in range(9):
            x = variant * 128
            stages[3][variant] = atlas.crop((x, 0, x + 64, 64))
    return stages


def review_board(stages, parts):
    """Expose four physical stages and every independent cell/back/front at source scale.

    @param stages Four mappings of nine complete genuine native machinery cells.
    @param parts Stage/cell-indexed independently stored original back/front pairs.
    @return RGB review board; it is a source comparison, never native game evidence.
    """
    board = Image.new("RGB", (1728, 1480), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((12, 8), "SOURCE CONSTRUCTION ONLY: four native stages, nine actual cells; separate loading floor untouched; packing/game binding pending", fill="black")
    labels = ("paid foundation (exact M1 yard)", "unfinished machinery (exact M1 hut)", "finishing apparatus/original commissioning glass", "exact approved completed base")
    # Full campuses show physical progression; independent parts expose source
    # gaps and registration errors that a flattened attractive scene can hide.
    for stage in range(4):
        top = 40 + stage * 360
        draw.text((12, top), f"native construction stage{stage}: {labels[stage]}", fill="black")
        campus = mine.campus(parts[stage]).convert("RGBA").resize((512, 320), Image.Resampling.NEAREST)
        board.paste(campus, (8, top + 24), campus)
        for variant in range(9):
            x = 544 + variant * 128
            draw.text((x, top + 26), f"cell{variant} ({variant % 3},{variant // 3})", fill="black")
            whole = stages[stage][variant].convert("RGBA").resize((128, 128), Image.Resampling.NEAREST)
            board.paste(whole, (x, top + 48), whole)
            for index, sprite in enumerate(parts[stage][variant]):
                draw.text((x + index * 64, top + 192), mine.PARTS[index], fill="black")
                rgba = sprite.convert("RGBA")
                board.paste(rgba, (x + index * 64, top + 212), rgba)
    return board


def build(record, public):
    """Validate all construction sources before producing the complete72-slot comparison.

    @param record Retained mine source/provenance document and frozen registrations.
    @param public Existing shared native mine.COAL family, still source-only.
    @return Construction report and deterministic artifact bytes, without writing files.
    """
    stages = stage_cells(record, public)
    atlas = Image.new("P", (1152, 256), 0)
    atlas.putpalette(mine.base.PALETTE_BYTES)
    parts, rows = {}, []
    # Native stages enumerate actual tile-owned construction states. No source
    # alias, hidden rotation or industry-local animation substitutes for them.
    for stage in range(4):
        parts[stage] = {}
        for variant in range(9):
            mine.validate_cell(stages[stage][variant], variant)
            pair = mine.source_parts(stages[stage][variant])
            parts[stage][variant] = pair
            for index, (part, sprite) in enumerate(zip(mine.PARTS, pair)):
                if sprite.getbbox() is None:
                    raise ValueError("mine construction lacks a nonempty actual native source part")
                x, y = (variant * 2 + index) * 64, stage * 64
                atlas.paste(sprite, (x, y))
                rows.append({"asset_id": "mine.COAL", "state": "construction", "frame": stage, "direction": "none", "native_layout": 0,
                             "tile_variant": variant, "world_cell": [variant % 3, variant // 3], "part": part,
                             "box": [x, y, 64, 64], "anchor": [-31, -33], "nonempty_pixels": sum(value != 0 for value in sprite.get_flattened_data()),
                             "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest()})
    files = {OUTPUT / f"{PREFIX}-atlas.png": mine.png_bytes(atlas)}
    buffer = io.BytesIO()
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md Group27C; archi-map-layers.md; source only")
    review_board(stages, parts).save(buffer, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files[OUTPUT / f"{PREFIX}-review.png"] = buffer.getvalue()
    family = record["families"]["mine.COAL"]
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md Group27C; archi-map-layers.md",
              "asset_id": "mine.COAL", "directions": ["none"], "native_layout": 0, "footprint": [3, 3], "native_size": [64, 64],
              "anchor": [-31, -33], "source_height_limit": 20, "loading_cells": family["loading_cells"],
              "construction_stages": [0, 1, 2, 3], "stage_owner": "GetIndustryConstructionStage(actual owned tile)",
              "construction_source_slots": 72, "construction_coverage_complete": True, "source_coverage_complete": False,
              "runtime_activated": False, "source_parts": rows, "construction_reuse": family["construction_reuse"],
              "construction_cells": family["construction_cells"], "part_policy": "Existing native full original underlay and independently sorted upper original subset",
              "remaining": "Operating/paused/depleted source27D, aggregate27.3 and native packing/decoded/game proof10.4D"}
    report["artifacts"] = [{"path": path.relative_to(mine.ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items()]
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def main():
    """Publish the fully validated construction comparison with full-family coverage pending.

    @return None; every original, ancestry and native placement refusal precedes writes.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true", required=True)
    parser.parse_args()
    record = json.loads(mine.RECORD.read_text(encoding="utf-8"))
    document = json.loads(mine.PUBLIC.read_text(encoding="utf-8"))
    public = next(row for row in document["assets"] if row["asset_id"] == "mine.COAL")
    report, files = build(record, public)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for path, payload in files.items():
        path.write_bytes(payload)
    print(json.dumps({key: report[key] for key in ("construction_source_slots", "construction_coverage_complete", "source_coverage_complete", "runtime_activated")}))


if __name__ == "__main__":
    main()
