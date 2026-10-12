"""Convert genuine completed refinery originals through the compiled native processor geometry.

@file convert_processor_base.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups39B/39C/39D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This32-part base selects physical apparatus only. Full512-source construction/
operating poses, packed sprite ranges and ordinary-game rendering remain pending.
"""

import argparse
import copy
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_mine_art as mine
from tools.ground_processor_contract import ROOT, processor_contract, validate_processor_contract


RECORD = ROOT / "media/multilayer/source/ground/processor-generation-record.json"
PUBLIC = mine.PUBLIC
OUTPUT = mine.OUTPUT
ASSET = "facility.metal-refinery"
NEW_VARIANTS = (0, 1, 2, 3, 6, 7, 8, 9, 10, 13, 14, 15)
ROLES = ("empty-ore-hopper-four-legs", "empty-world-X-ore-feed-belt", "empty-two-roll-crusher", "complete-empty-mesh-screen-four-springs",
         "empty-fuel-hopper-four-legs", "empty-world-X-fuel-feed-belt", "low-reduction-furnace", "empty-two-mould-casting-bed",
         "empty-world-Y-discharge-chute", "two-roll-finishing-press", "low-cooling-cabinet", "m1-control-hut",
         "two-canister-dust-filter", "low-forced-air-blower", "empty-dispatch-ledge", "low-tool-store")


def donor_record():
    """Read existing immutable compatible individual apparatus/control ancestry.

    @return Current mine original document, never a copied whole industry or physical inventory.
    """
    return json.loads(mine.RECORD.read_text(encoding="utf-8"))


def expected_reuse(donor):
    """Resolve exactly the four declared compatible components from their original physical roles.

    @param donor Existing genuine Coal/M1 original registrations, without any modification.
    @return Exact reuse graph including separate actual fuel cells and the M1 control/filter ancestry.
    """
    coal = donor["families"]["mine.COAL"]
    selection = next(row for row in coal["base_cells"] if row["variant"] == 8)
    source, component = mine.component_record(donor, selection)
    return [{"variant": 4, "kind": "local_original", "source_variant": 0,
             "physical_role": "Exact duplicate compatible empty four-leg hopper; separate actual paid fuel-feed cell, no cargo relabel"},
            {"variant": 5, "kind": "local_original", "source_variant": 1,
             "physical_role": "Exact duplicate compatible empty world-X belt/one drive; separate actual paid fuel-feed cell"},
            {"variant": 11, "kind": "m1_control", "donor_control_reuse": copy.deepcopy(coal["control_reuse"]),
             "physical_role": "Exact completed M1 original one-tile control hut,64x64/anchor(-31,-33)/20 volume; only existing3x3 status glass varies later"},
            {"variant": 12, "kind": "compatible_original", "source_record": mine.RECORD.relative_to(ROOT).as_posix(),
             "source_family": "mine.COAL", "source_selection": selection, "source_sha256": source["sha256"],
             "component_geometry_sha256": mine.signature(component),
             "physical_role": "Exact no-cargo two-short-canister filter/flush grille/one compact blower; unchanged individual original/native pixel ancestry"}]


def completed_cells(record, public):
    """Register all sixteen completed native cells with exact empty-original or compatible ancestry.

    @param record Executing processor's selected completed originals and approved physical reuse graph.
    @param public Existing compiled selected Surface processor row, never a guessed draft footprint.
    @return Actual variant-indexed64x64 whole original apparatus at unchanged native anchors.
    """
    validate_processor_contract(public)
    if public["asset_id"] != ASSET:
        raise ValueError("completed refinery source is outside executing family")
    family, donor = record["families"][ASSET], donor_record()
    reuse = expected_reuse(donor)
    # Exactly declared physical roles/ancestry authorize completed reuse. A
    # copied campus, stock-bearing filter or unfinished guard cannot fill a gap.
    if (family["asset_id"] != ASSET or family["reuse_graph"] != reuse or mine.signature(reuse) != family["approved_reuse_sha256"]
            or family["roles"] != list(ROLES) or family["view"] != "none"):
        raise ValueError("refinery compatible physical reuse or model role changed")
    model = {key: family[key] for key in ("roles", "materials", "model", "inventory_policy")}
    if mine.signature(model) != family["approved_model_sha256"]:
        raise ValueError("refinery approved completed model changed")
    for key in ("footprint", "native_cells", "native_size", "anchor", "source_height_limit", "directions", "native_layout", "native_state_policy"):
        if family[key] != public[key]:
            raise ValueError("refinery source changes actual native " + key)
    selections = family["base_cells"]
    variants = [selection["variant"] for selection in selections]
    if len(variants) != len(NEW_VARIANTS) or set(variants) != set(NEW_VARIANTS):
        raise ValueError("refinery completed original matrix is missing or ambiguous")
    registrations = {str(row["variant"]): mine.component_record(record, row)[1] for row in selections}
    if mine.signature(registrations) != family["approved_base_component_geometry_sha256"]:
        raise ValueError("refinery completed original registration changed")
    contract, cells = processor_contract(ASSET), {}
    # Every new completed apparatus is genuinely empty and complete. An
    # unfinished construction source or another family's cargo is not a base.
    for selection in selections:
        source, _component = mine.component_record(record, selection)
        if (source.get("processor_asset_id") != ASSET or source.get("construction_stage") != 3
                or source.get("completed_empty") is not True):
            raise ValueError("refinery cannot relabel foreign or unfinished completed apparatus")
        cells[selection["variant"]] = mine.original_cell(record, selection, contract)
    # These are independent compatible apparatus cells, not a copied plant:
    # fuel uses its own paid hopper/belt; control/filter retain original pixels.
    cells[4], cells[5] = cells[0].copy(), cells[1].copy()
    cells[11] = mine.control_cell(donor)
    cells[12] = mine.original_cell(donor, reuse[3]["source_selection"], mine.mine_contract("COAL"))
    for variant, sprite in cells.items():
        mine.validate_cell(sprite, variant)
    return cells


def campus(parts):
    """Compose actual4x4 source cells through the existing underlay/front convention.

    @param parts Native original back/front pair by actual row-major variant0..15.
    @return Indexed320x192 source comparison, never proof of native world sorting or game display.
    """
    result = Image.new("P", (320, 192), 0)
    result.putpalette(mine.base.PALETTE_BYTES)
    result.info["transparency"] = 0
    # All original underlays precede independent upper parents. The same
    # native world diagonals retain registration across the larger footprint.
    for part in range(2):
        for variant in sorted(parts, key=lambda value: (value % 4 + value // 4, value)):
            x, y = variant % 4, variant // 4
            sprite = parts[variant][part]
            mask = Image.new("L", (64, 64))
            mask.putdata([255 if value else 0 for value in sprite.get_flattened_data()])
            result.paste(sprite, (128 + 32 * (y - x), 8 + 16 * (x + y)), mask)
    return result


def review_board(cells, parts, scene):
    """Expose native campus, independent actual cells and original overlapping source parents.

    @param cells Sixteen completed whole originals at their actual native anchors.
    @param parts Each cell's original underlay and independently sorted upper subset.
    @param scene Source-only completed4x4 campus with quantity/work still owned by Industry.
    @return RGB model-specific completed base review; full construction/operating art remains pending.
    """
    board = Image.new("RGB", (1696, 1200), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((12, 8), "SOURCE COMPLETED BASE ONLY: metal refinery fixed4x4/16 actual cells/32 parts; full512 construction/state source and native game display pending", fill="black")
    rgba = scene.convert("RGBA")
    board.paste(rgba, (16, 56), rgba)
    large = rgba.resize((960, 576), Image.Resampling.NEAREST)
    board.paste(large, (352, 48), large)
    draw.text((16, 280), "Empty apparatus communicates process.", fill="black")
    draw.text((16, 296), "Industry owns selected recipe/stock/work.", fill="black")
    # Independent native cells reveal missing hardware, contact drift and the
    # intentional repeated original pixels in underlay/upper-front parents.
    for variant in range(16):
        x, y = 16 + (variant % 8) * 208, 672 + (variant // 8) * 256
        draw.text((x, y), f"cell{variant} ({variant % 4},{variant // 4})", fill="black")
        draw.text((x, y + 16), ROLES[variant], fill="black")
        whole = cells[variant].convert("RGBA").resize((128, 128), Image.Resampling.NEAREST)
        board.paste(whole, (x, y + 40), whole)
        for index, sprite in enumerate(parts[variant]):
            draw.text((x + index * 88, y + 176), mine.PARTS[index], fill="black")
            part = sprite.convert("RGBA")
            board.paste(part, (x + index * 88, y + 196), part)
    return board


def encoded_png(image, indexed=True):
    """Encode stable original-derived artifacts with owning-plan/architecture metadata.

    @param image Native indexed source/campus or RGB review.
    @param indexed Whether palette index0 supplies native transparency.
    @return Deterministic PNG bytes without any filesystem mutation.
    """
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md Group39C; archi-map-layers.md; completed base source only")
    output = io.BytesIO()
    options = {"transparency": 0} if indexed else {}
    image.save(output, format="PNG", pnginfo=metadata, optimize=False, compress_level=9, **options)
    return output.getvalue()


def build(record, public):
    """Convert only the selected completed32-part base after all physical/original checks.

    @param record Genuine completed original selections and exact per-cell compatible ancestry.
    @param public Existing actual compiled metal refinery source contract.
    @return Completed base report and deterministic atlas/campus/review/manifest bytes.
    """
    cells = completed_cells(record, public)
    parts = {variant: mine.source_parts(whole) for variant, whole in cells.items()}
    atlas = Image.new("P", (2048, 64), 0)
    atlas.putpalette(mine.base.PALETTE_BYTES)
    rows = []
    # Actual sixteen machinery variants each retain separate original back
    # and upper-front parents at one unchanged64x64 native anchor.
    for variant in range(16):
        for index, (part, sprite) in enumerate(zip(mine.PARTS, parts[variant])):
            if sprite.getbbox() is None:
                raise ValueError("refinery completed base lacks a real original source part")
            x = (variant * 2 + index) * 64
            atlas.paste(sprite, (x, 0))
            rows.append({"asset_id": ASSET, "state": "construction", "frame": 3, "direction": "none",
                         "native_layout": 0, "tile_variant": variant, "world_cell": public["native_cells"][variant],
                         "part": part, "box": [x, 0, 64, 64], "anchor": [-31, -33], "structurally_absent": False,
                         "nonempty_pixels": sum(value != 0 for value in sprite.get_flattened_data()),
                         "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest()})
    path = OUTPUT / "metal-refinery-base-atlas.png"
    scene = campus(parts)
    files = {path: encoded_png(atlas), OUTPUT / "metal-refinery-base-campus.png": encoded_png(scene),
             OUTPUT / "metal-refinery-base-review.png": encoded_png(review_board(cells, parts, scene), indexed=False)}
    family = record["families"][ASSET]
    report = {"_documentation": {"owner": "docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39C",
                                  "architecture": "docs/zz-reach2/architecture/map-layers/archi-map-layers.md"},
              "schema_version": 1, "asset_id": ASSET, "base_coverage_complete": True, "source_coverage_complete": False,
              "runtime_activated": False, "source_slots_required": 512, "base_source_parts": 32, "nonempty_base_parts": 32,
              "base_native_atlas": path.relative_to(ROOT).as_posix(), "base_native_sha256": hashlib.sha256(files[path]).hexdigest(),
              "native_cells": public["native_cells"], "native_size": [64, 64], "anchor": [-31, -33], "source_height_limit": 20,
              "source_parts": rows, "base_cells": family["base_cells"], "reuse_graph": family["reuse_graph"],
              "inventory_policy": family["inventory_policy"], "remaining": "Genuine full512 construction/operating source39D; master10.4D packing/native binding/decoded pixels/ordinary game"}
    report["artifacts"] = [{"path": target.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for target, payload in files.items()]
    files[OUTPUT / "metal-refinery-base-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def validate_public_base(public, report):
    """Bind a selected completed-base claim to its exact live artifact and immutable raw identity.

    @param public Existing actual metal refinery row after completed-base source publication.
    @param report Fresh32-part completed-base report for this same family.
    @return None; later full source needs its own exact live512-slot handoff; packed/runtime activation still refuses.
    """
    validate_processor_contract(public)
    authoring = public.get("authoring", {})
    # A native-looking count or unrelated atlas cannot complete this base;
    # each live artifact and owner field must bind the actual selected source.
    if (public["asset_id"] != ASSET or report["asset_id"] != ASSET or report["base_source_parts"] != 32
            or report["nonempty_base_parts"] != 32 or report["source_slots_required"] != 512
            or report.get("base_coverage_complete") is not True or report.get("source_coverage_complete") is not False
            or report.get("runtime_activated") is not False
            or public["source"]["status"] != "authored" or authoring.get("base_coverage_complete") is not True
            or authoring.get("selected_base_nonempty_parts") != 32 or authoring.get("base_native_atlas") != report["base_native_atlas"]
            or authoring.get("base_native_sha256") != report["base_native_sha256"]
            or authoring.get("base_atlas_manifest") != (OUTPUT / "metal-refinery-base-manifest.json").relative_to(ROOT).as_posix()
            or authoring.get("runtime_activated") is not False or public["packed"]["offset"] is not None or public["packed"]["sha256"] is not None):
        raise ValueError("refinery public completed base binding or native activation changed")
    if mine.base.digest(mine.base.repository_path(public["source"]["path"])) != public["source"].get("sha256"):
        raise ValueError("refinery public original source changed")
    if mine.base.digest(mine.base.repository_path(report["base_native_atlas"])) != report["base_native_sha256"]:
        raise ValueError("refinery public live completed base atlas changed")
    # A later complete39D handoff may coexist with this immutable32-part base.
    # Its coverage flag requires the exact live512-slot matrix and native pixels;
    # a caption, count or unrelated full atlas cannot upgrade the base proof.
    if authoring.get("coverage_complete") is True:
        import convert_processor_states as states
        try:
            full_report = json.loads(states.MANIFEST.read_text(encoding="utf-8"))
        except (OSError, ValueError) as error:
            raise ValueError("refinery later full source lacks its live512-slot handoff") from error
        states.validate_public_source(public, full_report)
    elif authoring.get("coverage_complete") is not False:
        raise ValueError("refinery public source coverage is not an explicit owner claim")


def publish_base(record, document, report):
    """Publish only selected completed-base metadata while preserving the full source/game queue.

    @param record Current immutable-original model/component document.
    @param document Shared actual asset contract; every other family row remains exact.
    @param report Validated selected32-part base report, not full512 source evidence.
    @return None; no source coverage, packed range or gameplay owner is activated.
    """
    updated, public_document = copy.deepcopy(record), copy.deepcopy(document)
    family = updated["families"][ASSET]
    row = next(row for row in public_document["assets"] if row["asset_id"] == ASSET)
    if row.get("authoring", {}).get("coverage_complete") is True:
        raise ValueError("refinery full source cannot be downgraded to completed base")
    original, _component = mine.component_record(updated, family["base_cells"][0])
    row["source"] = {"status": "authored", "path": original["path"], "sha256": original["sha256"],
                     "provenance": "first_party_original_model_and_compatible_components", "rights": "first_party_generated_original"}
    row.setdefault("authoring", {}).update(status="completed_base_selected_full_source_pending", source_record=RECORD.relative_to(ROOT).as_posix(),
                                           base_coverage_complete=True, selected_base_nonempty_parts=32, base_native_atlas=report["base_native_atlas"],
                                           base_native_sha256=report["base_native_sha256"], base_atlas_manifest=(OUTPUT / "metal-refinery-base-manifest.json").relative_to(ROOT).as_posix(),
                                           coverage_complete=False, runtime_activated=False, remaining=report["remaining"])
    family.update(status="completed_base_selected_full_source_pending", base_coverage_complete=True,
                  base_native_atlas=report["base_native_atlas"], base_native_sha256=report["base_native_sha256"],
                  base_atlas_manifest=row["authoring"]["base_atlas_manifest"], remaining=report["remaining"])
    validate_public_base(row, report)
    RECORD.write_text(json.dumps(updated, indent=2) + "\n", encoding="utf-8")
    PUBLIC.write_text(json.dumps(public_document, indent=2) + "\n", encoding="utf-8")


def main():
    """Review or publish the executing completed refinery base through retained original pixels.

    @return None; all physical/original validation precedes deterministic base publication.
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
        publish_base(record, document, report)
    print(json.dumps({key: report[key] for key in ("asset_id", "base_source_parts", "base_coverage_complete", "source_coverage_complete", "runtime_activated")}))


if __name__ == "__main__":
    main()
