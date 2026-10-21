"""Convert genuine completed battery apparatus and individually compatible original service cells.

@file convert_battery_base.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups41C/41D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This32-part completed base preserves Industry recipe/stock/work ownership.
Genuine full512-state source, native packing and game display remain separate.
"""

import argparse
import copy
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_processor_base as shared


mine = shared.mine
ROOT, RECORD, PUBLIC, OUTPUT = shared.ROOT, shared.RECORD, shared.PUBLIC, shared.OUTPUT
ASSET = "facility.battery-works"
PREFIX = "battery-works-base"
NEW_VARIANTS = (7, 8, 9, 10)
DONORS = ((0, "facility.metal-refinery", 0), (1, "facility.metal-refinery", 1),
          (4, "facility.chemical-works", 2), (5, "facility.chemical-works", 3),
          (6, "facility.chemical-works", 5), (12, "facility.metal-refinery", 10),
          (13, "facility.metal-refinery", 13), (14, "facility.metal-refinery", 14),
          (15, "facility.metal-refinery", 15))
ROLES = ('empty-lithium-ore-hopper', 'empty-world-X-lithium-feed-belt', 'empty-cobalt-ore-hopper', 'empty-world-X-cobalt-feed-belt', 'low-closed-chemical-day-tank', 'low-chemical-transfer-pump', 'low-closed-electrode-slurry-mixer', 'low-electrode-coating-calender', 'low-cell-winding-machine', 'low-cell-sealing-press', 'low-closed-formation-cabinet', 'm1-control-hut', 'low-controlled-cooling-cabinet', 'low-forced-air-blower', 'empty-dispatch-ledge', 'low-tool-store')
MANIFEST = OUTPUT / f"{PREFIX}-manifest.json"


def expected_reuse(record):
    """Resolve only compatible individual completed apparatus and exact M1 control ancestry.

    @param record Shared original document with selected completed refinery and chemical components.
    @return Exact physical-role reuse graph; independent paid cells retain their own native owners.
    """
    graph = []
    contract = shared.processor_contract(ASSET)
    # Only individually compatible empty/closed physical apparatus can move
    # between source cells. Industry retains every paid cell and stock owner.
    for target, source_family, source_variant in DONORS:
        donor = record["families"][source_family]
        selection = next(row for row in donor["base_cells"] if row["variant"] == source_variant)
        source, component = mine.component_record(record, selection)
        graph.append({"variant": target, "kind": "compatible_processor_original", "source_family": source_family,
                      "source_selection": selection, "source_sha256": source["sha256"],
                      "component_geometry_sha256": mine.signature(component), "physical_role": ROLES[target],
                      "source_physical_role": donor["roles"][source_variant], "target_world_cell": contract["native_cells"][target],
                      "inventory": "Empty or closed individual original apparatus only; no original stock, work, recipe, family identity or whole campus is copied"})
    graph.extend([{"variant": 2, "kind": "local_original", "source_variant": 0,
                   "physical_role": "Same empty hopper in its distinct paid cobalt cell; no visible ore relabel"},
                  {"variant": 3, "kind": "local_original", "source_variant": 1,
                   "physical_role": "Same empty world-X feed belt in its distinct paid cobalt cell"},
                  {"variant": 11, "kind": "m1_control", "donor_control_reuse": copy.deepcopy(shared.donor_record()["families"]["mine.COAL"]["control_reuse"]),
                   "physical_role": "Exact original one-tile M1control hut/floor/status fitting; no new quantity or energy authority"}])
    return sorted(graph, key=lambda row: row["variant"])


def selected_originals(record, selections):
    """Bind selected original identities separately from their component placements.

    @param record Immutable processor candidates with actual executed prompts and references.
    @param selections Explicit four-cell completed Battery selections.
    @return Canonical candidate identities that must match the reviewed approval.
    """
    originals = {}
    # A newly rehashed foreign image must not inherit approval from a valid
    # source window; identity, prompt and reference lineage remain inseparable.
    for selection in selections:
        source, _component = mine.component_record(record, selection)
        originals[source["id"]] = {key: source[key] for key in
                                   ("id", "path", "sha256", "prompt", "references", "image_size")}
    return originals


def validate_layout(source):
    """Check every declared source window against the whole retained original.

    @param source Selected candidate with non-overlapping complete rectangular crops.
    @return None after measured clearance, separator and whole-canvas checks.
    @throws ValueError If an original model crosses a crop or another panel.
    """
    mine.facilities.verify_original(source)
    with Image.open(mine.base.repository_path(source["path"])) as image:
        rgba = image.convert("RGBA")
    windows = [row["source_window"] for row in source["components"]]
    # Distinct source panels cannot share pixels. A useful neighboring cell
    # does not authorize clipping equipment from another model into this one.
    for index, left in enumerate(windows):
        for right in windows[index + 1:]:
            if (max(left[0], right[0]) < min(left[2], right[2])
                    and max(left[1], right[1]) < min(left[3], right[3])):
                raise ValueError("battery source windows overlap")
    # Only declared magenta separators may lie outside the complete windows;
    # an apparatus, plinth or significant shadow cannot silently be discarded.
    for index, (red, green, blue, alpha) in enumerate(rgba.get_flattened_data()):
        x, y = index % rgba.width, index // rgba.width
        separator = red > 220 and green < 60 and blue > 220
        if (alpha >= 16 and not separator
                and not any(box[0] <= x < box[2] and box[1] <= y < box[3] for box in windows)):
            raise ValueError("battery model pixels leave complete source windows")


def completed_cells(record, public):
    """Select sixteen actual native completed apparatus cells with immutable original ancestry.

    @param record Battery brief, four selected completed originals and twelve compatible reuse mappings.
    @param public Current compiled fixed-layout battery processor family, never a draft source rotation.
    @return Variant-indexed whole indexed64x64 apparatus at unchanged native anchors and contacts.
    """
    shared.validate_processor_contract(public)
    if public["asset_id"] != ASSET:
        raise ValueError("battery completed source is outside executing family")
    family = record["families"][ASSET]
    reuse = expected_reuse(record)
    # Exact model, physical role and original ancestry jointly authorize the
    # completed base. A convenient foreign component or new stock caption cannot.
    if (family["asset_id"] != ASSET or family["roles"] != list(ROLES) or family["view"] != "none"
            or family["reuse_graph"] != reuse or mine.signature(reuse) != family["approved_reuse_sha256"]):
        raise ValueError("battery completed model or compatible physical reuse changed")
    model = {key: family[key] for key in ("roles", "materials", "model", "inventory_policy")}
    if mine.signature(model) != family["approved_model_sha256"]:
        raise ValueError("battery approved completed model changed")
    if mine.signature(family["selected_model_inventory"]) != family["approved_model_inventory_sha256"]:
        raise ValueError("battery selected apparatus inventory changed")
    # Every native field must match the compiled owner; source art cannot
    # alter physical geometry, construction dispatch or synchronized phases.
    for key in ("footprint", "native_cells", "native_size", "anchor", "source_height_limit", "directions", "native_layout", "native_state_policy"):
        if family[key] != public[key]:
            raise ValueError("battery completed source changes native " + key)
    selections = family["base_cells"]
    variants = [row["variant"] for row in selections]
    # All four distinct battery machines are required; duplicates cannot
    # disguise a missing calender, winding station, sealing press or cabinet.
    if len(variants) != len(NEW_VARIANTS) or set(variants) != set(NEW_VARIANTS):
        raise ValueError("battery completed original matrix is missing or ambiguous")
    registrations = {str(row["variant"]): mine.component_record(record, row)[1] for row in selections}
    if mine.signature(registrations) != family["approved_base_component_geometry_sha256"]:
        raise ValueError("battery completed original registration changed")
    if mine.signature(selected_originals(record, selections)) != family["approved_base_originals_sha256"]:
        raise ValueError("battery selected immutable original approval changed")
    contract, cells, checked_sources = shared.processor_contract(ASSET), {}, set()
    # New process equipment must be genuinely complete and closed/empty; an
    # unfinished body, altered raw or another processor's stock cannot fill a gap.
    for selection in selections:
        source, _component = mine.component_record(record, selection)
        if (source.get("processor_asset_id") != ASSET or source.get("construction_stage") != 3
                or source.get("completed_empty") is not True):
            raise ValueError("battery cannot relabel foreign or unfinished completed apparatus")
        if selection["variant"] not in source["selection"].get("approved_variants", []):
            raise ValueError("battery component lacks explicit native/model approval")
        # Shared panels are inspected once per build; all components still
        # undergo their own complete source, contact and sorting validation.
        if source["id"] not in checked_sources:
            validate_layout(source)
            checked_sources.add(source["id"])
        cells[selection["variant"]] = mine.original_cell(record, selection, contract)
    # Original donor coordinates stay authoritative while converting each
    # compatible physical component; its explicit new world offset is separate.
    for row in reuse:
        if row["kind"] == "compatible_processor_original":
            cells[row["variant"]] = mine.original_cell(record, row["source_selection"], shared.processor_contract(row["source_family"]))
    cells[2], cells[3] = cells[0].copy(), cells[1].copy()
    cells[11] = mine.control_cell(shared.donor_record())
    # Independent native validation catches an over-tall body or shifted
    # floor that a combined campus could conceal behind its neighbors.
    for variant, sprite in cells.items():
        mine.validate_cell(sprite, variant)
    return cells


def encoded_png(image, indexed=True):
    """Encode deterministic battery source/review bytes with their exact owning plans.

    @param image Indexed native source/campus or RGB diagnostic review.
    @param indexed Whether palette index0 owns native transparency.
    @return Stable PNG bytes without any original-pixel correction.
    """
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md Group41C; archi-map-layers.md; completed base source only")
    buffer = io.BytesIO()
    options = {"transparency": 0} if indexed else {}
    image.save(buffer, format="PNG", pnginfo=metadata, optimize=False, compress_level=9, **options)
    return buffer.getvalue()


def review_board(cells, parts, scene):
    """Expose battery completed model, whole native cells and intentional original foreground overlap.

    @param cells Actual sixteen completed machinery originals at canonical native contacts.
    @param parts Original underlay and independently parented upper subset for each cell.
    @param scene Source-only fixed4x4 comparison; Industry retains all physical quantities/work.
    @return RGB base review, never full construction/state coverage or ordinary-game proof.
    """
    board = Image.new("RGB", (1696, 1200), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((12, 8), "SOURCE COMPLETED BASE ONLY: battery works fixed4x4/16 cells/32 parts; full512 construction/states and native display pending", fill="black")
    rgba = scene.convert("RGBA")
    board.paste(rgba, (16, 56), rgba)
    large = rgba.resize((960, 576), Image.Resampling.NEAREST)
    board.paste(large, (352, 48), large)
    draw.text((16, 280), "Empty ore feeds; closed chemical/formation fittings.", fill="black")
    draw.text((16, 296), "Industry owns actual recipe/stock/work.", fill="black")
    # Independent cells expose missing hardware and source contact drift that
    # a whole campus could obscure; repeated upper pixels are intentional.
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


def build(record, public):
    """Build only the selected completed32-part battery base from genuine original material.

    @param record Reviewed battery model/registrations and exact compatible individual ancestry.
    @param public Actual compiled fixed-layout battery source contract.
    @return Completed-base report and deterministic atlas/campus/review/manifest payloads.
    """
    cells = completed_cells(record, public)
    parts = {variant: mine.source_parts(whole) for variant, whole in cells.items()}
    atlas = Image.new("P", (2048, 64), 0)
    atlas.putpalette(mine.base.PALETTE_BYTES)
    rows = []
    # Sixteen real native cells each retain their whole original underlay and
    # original upper subset at the same64x64 source anchor, without new ranges.
    for variant in range(16):
        for index, (part, sprite) in enumerate(zip(mine.PARTS, parts[variant])):
            if sprite.getbbox() is None:
                raise ValueError("battery completed base lacks a genuine source part")
            x = (variant * 2 + index) * 64
            atlas.paste(sprite, (x, 0))
            rows.append({"asset_id": ASSET, "state": "construction", "frame": 3, "direction": "none", "native_layout": 0,
                         "tile_variant": variant, "world_cell": public["native_cells"][variant], "part": part,
                         "box": [x, 0, 64, 64], "anchor": [-31, -33], "structurally_absent": False,
                         "nonempty_pixels": sum(value != 0 for value in sprite.get_flattened_data()),
                         "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest()})
    path = OUTPUT / f"{PREFIX}-atlas.png"
    scene = shared.campus(parts)
    files = {path: encoded_png(atlas), OUTPUT / f"{PREFIX}-campus.png": encoded_png(scene),
             OUTPUT / f"{PREFIX}-review.png": encoded_png(review_board(cells, parts, scene), False)}
    family = record["families"][ASSET]
    report = {"_documentation": {"owner": "r2mlvs14-remaining-assets-and-runtime-integration.md Group41C", "architecture": "archi-map-layers.md"},
              "schema_version": 1, "asset_id": ASSET, "base_coverage_complete": True, "source_coverage_complete": False,
              "runtime_activated": False, "source_slots_required": 512, "base_source_parts": 32, "nonempty_base_parts": 32,
              "base_native_atlas": path.relative_to(ROOT).as_posix(), "base_native_sha256": hashlib.sha256(files[path]).hexdigest(),
              "native_cells": public["native_cells"], "native_size": [64, 64], "anchor": [-31, -33], "source_height_limit": 20,
              "source_parts": rows, "base_cells": family["base_cells"], "reuse_graph": family["reuse_graph"],
              "approved_base_originals_sha256": family["approved_base_originals_sha256"],
              "selected_model_inventory": family["selected_model_inventory"], "inventory_policy": family["inventory_policy"],
              "remaining": "41D genuine full512 construction/operating source; master10.4D packing/native binding/decoded pixels/ordinary game"}
    report["artifacts"] = [{"path": target.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for target, payload in files.items()]
    files[MANIFEST] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def validate_public_base(public, report):
    """Bind only completed battery base coverage to its exact live payload and original.

    @param public Current shared authored battery row, with full source and runtime still pending.
    @param report Fresh32-part source report, never a substitute512-pose proof.
    @return None after exact base/original/source-owner binding checks.
    """
    shared.validate_processor_contract(public)
    authoring = public.get("authoring", {})
    # A whole-source flag or native-looking count cannot authorize missing
    # genuine construction states, a foreign atlas or premature sprite ranges.
    if (public["asset_id"] != ASSET or report.get("asset_id") != ASSET or report.get("base_source_parts") != 32
            or report.get("nonempty_base_parts") != 32 or report.get("source_slots_required") != 512
            or report.get("base_coverage_complete") is not True or report.get("source_coverage_complete") is not False
            or report.get("runtime_activated") is not False or public["source"]["status"] != "authored"
            or authoring.get("base_coverage_complete") is not True or authoring.get("selected_base_nonempty_parts") != 32
            or authoring.get("base_native_atlas") != report.get("base_native_atlas")
            or authoring.get("base_native_sha256") != report.get("base_native_sha256")
            or authoring.get("base_atlas_manifest") != MANIFEST.relative_to(ROOT).as_posix()
            or authoring.get("runtime_activated") is not False
            or public["packed"]["offset"] is not None or public["packed"]["sha256"] is not None):
        raise ValueError("battery completed base/public source binding or activation changed")
    if mine.base.digest(mine.base.repository_path(public["source"]["path"])) != public["source"].get("sha256"):
        raise ValueError("battery public immutable original changed")
    if mine.base.digest(mine.base.repository_path(report["base_native_atlas"])) != report["base_native_sha256"]:
        raise ValueError("battery public live completed base atlas changed")
    if authoring.get("coverage_complete") is True:
        try:
            import convert_battery_states as states
            full_report = json.loads(states.MANIFEST.read_text(encoding="utf-8"))
        except (ImportError, OSError, ValueError) as error:
            raise ValueError("battery later full source lacks its live512-slot handoff") from error
        states.validate_public_source(public, full_report)
    elif authoring.get("coverage_complete") is not False:
        raise ValueError("battery public source coverage is not an explicit owner claim")


def publish_base(record, document, report):
    """Publish only the completed-base source through the existing shared battery family.

    @param record Current original document; every earlier processor/candidate remains exact.
    @param document Existing shared asset manifest; only executing source metadata changes.
    @param report Complete32-part live base report, without512-source/runtime authority.
    @return None; no recipe, cargo, work, packed range or native rendering mutation.
    """
    updated, public_document = copy.deepcopy(record), copy.deepcopy(document)
    family = updated["families"][ASSET]
    row = next(row for row in public_document["assets"] if row["asset_id"] == ASSET)
    if row.get("authoring", {}).get("coverage_complete") is True:
        raise ValueError("battery full source cannot be downgraded to completed base")
    source, _component = mine.component_record(updated, family["base_cells"][0])
    row["source"] = {"status": "authored", "path": source["path"], "sha256": source["sha256"],
                     "provenance": "first_party_original_model_and_compatible_components", "rights": "first_party_generated_original"}
    row["authoring"].update(date="2026-10-21", status="completed_base_selected_full_source_pending", source_record=RECORD.relative_to(ROOT).as_posix(),
                            base_coverage_complete=True, selected_base_nonempty_parts=32, base_native_atlas=report["base_native_atlas"],
                            base_native_sha256=report["base_native_sha256"], base_atlas_manifest=MANIFEST.relative_to(ROOT).as_posix(),
                            coverage_complete=False, runtime_activated=False, remaining=report["remaining"])
    family.update(status="completed_base_selected_full_source_pending", base_coverage_complete=True,
                  base_native_atlas=report["base_native_atlas"], base_native_sha256=report["base_native_sha256"],
                  base_atlas_manifest=MANIFEST.relative_to(ROOT).as_posix(), remaining=report["remaining"])
    validate_public_base(row, report)
    RECORD.write_text(json.dumps(updated, indent=2) + "\n", encoding="utf-8")
    PUBLIC.write_text(json.dumps(public_document, indent=2) + "\n", encoding="utf-8")


def main():
    """Review or publish only the executing completed battery base from retained originals.

    @return None; all original/model/native validation precedes base publication.
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
