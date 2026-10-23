"""Review and compose retained dated generator originals on the verified native floor.

@file convert_generator_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups55-56.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group55B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group56 MagmaLoop source.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original pixels are sampled and losslessly partitioned; runtime activation remains separate.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube
import convert_facility_art as facilities


RECORD = tube.ROOT / "media/multilayer/source/ground/geothermal-generation-record.json"
CANVAS = (320, 224)
CELLS = [[x, y] for y in range(4) for x in range(4)]
ORIGINS = [(129 + 32 * (y - x), 31 + 16 * (x + y)) for x, y in CELLS]
FLOOR = [[160, 64], [288, 128], [160, 192], [32, 128]]
MODELS = {"facility.vent-powerhouse": ("VentTap", 1980), "facility.magma-powerhouse": ("MagmaLoop", 2000)}
STATES = facilities.STATES + ["heat_starved"]
DIRECTIONS = ("NE", "SE", "SW", "NW")
FOREGROUND = "solid_native_industry_cell_full_original_sections_no_vehicle_aperture"


def indexed_canvas(size):
    """Create an empty native DOS canvas without inventing any source pixels.

    @param size Actual native canvas dimensions, independent of an original's scale.
    @return Indexed image with the shared palette and transparent index zero.
    """
    canvas = Image.new("P", size, 0)
    canvas.putpalette(tube.base.PALETTE_BYTES)
    canvas.info["transparency"] = 0
    return canvas


def registration_signature(source):
    """Identify the reviewed original pixels and integer native placement.

    @param source Retained whole model/view metadata from the shared original record.
    @return SHA256 of the exact source geometry/selection, excluding workflow status.
    """
    fields = ("key", "asset_id", "direction", "path", "sha256", "native_crop",
              "native_subject_size", "native_position", "native_floor_corners", "native_geometry")
    selected = {field: source[field] for field in fields}
    # An incomplete original's paid construction stage is part of its identity.
    # Ready registrations keep their existing signature and final hardware role.
    if "construction_stage" in source:
        selected["construction_stage"] = source["construction_stage"]
    return hashlib.sha256(json.dumps(selected, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def native_base(key, document=None, spec=None):
    """Sample one checked original into its complete native world-floor canvas.

    @param key Explicit retained original/view registration, never the newest cache file.
    @param document Optional decoded source record for isolated refusal checks.
    @param spec Optional corresponding public family row for isolated contract checks.
    @return Indexed whole model and checked original registration.
    @throws ValueError If provenance, geometry, crop or native registration disagrees.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    source = tube.retained_candidate(key, document)
    asset_id = source["asset_id"]
    if asset_id not in MODELS:
        raise ValueError("generator original belongs to another source family")
    if spec is None:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
        spec = next(row for row in public["assets"] if row["asset_id"] == asset_id)
    model, year = MODELS[asset_id]
    geometry = spec.get("geometry_notes", {}).get("native_generator_geometry", {})
    expected = {"footprint": [4, 4], "tile_cells": CELLS, "cell_size": [64, 64], "anchor": [-31, -33]}
    # A whole original represents one physical industry across sixteen native
    # cells. Neither raster registration nor future metadata grants construction.
    if (spec["asset_id"] != asset_id or spec["consumer"] != 18 or spec["footprint"] != [4, 4]
            or spec["native_size"] != [64, 64] or spec["anchor"] != [-31, -33]
            or spec["geometry"] != "per_tile" or spec["tile_variants"] != 16
            or source.get("native_geometry") != expected or source.get("native_floor_corners") != FLOOR
            or geometry.get("model") != model or geometry.get("introduction_year") != year
            or geometry.get("world_cells") != CELLS or geometry.get("construction_stages") != [0, 1, 2, 3]):
        raise ValueError("generator source differs from verified native geometry")
    # A view excluded during source review cannot re-enter through an otherwise
    # valid crop or hash; directions are genuine renders, never raster rotations.
    if source.get("direction") not in spec["directions"] or source["direction"] in source.get("excluded_views", []):
        raise ValueError("generator original view is missing or excluded")
    approved = spec.get("geometry_notes", {}).get("approved_generator_registrations", {}).get(key)
    # Once a pose is reviewed, a valid raw hash alone cannot authorize shifting
    # or rescaling its machinery for later state composition or native packing.
    if approved is not None and registration_signature(source) != approved:
        raise ValueError("generator native registration differs from reviewed original")
    width, height = source["native_subject_size"]
    x, y = source["native_position"]
    # The measured whole-source scale preserves the 256x128 floor and low
    # machinery height. Exact per-cell fit is independently checked below.
    if not (width == 256 and 128 <= height <= 160 and x in (32, 33) and 31 <= y <= 64 and y + height <= 192):
        raise ValueError("generator original leaves native floor envelope")
    with Image.open(tube.base.repository_path(source["path"])) as original:
        original = original.convert("RGBA")
        left, top, right, bottom = source["native_crop"]
        # Crops must select complete original models within real source bytes;
        # transparent padding cannot conceal a missing edge or invalid window.
        if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
            raise ValueError("generator crop leaves immutable original")
        selected = original.crop((left, top, right, bottom))
    canvas = Image.new("RGBA", CANVAS)
    canvas.paste(selected.resize((width, height), Image.Resampling.BOX), (x, y))
    return tube.base.quantize(canvas, {}), source


def partition_model(campus, source):
    """Give every original indexed pixel exactly one real native cell owner.

    @param campus Checked whole original on its native320x224 canvas.
    @param source Checked immutable source identity and measured registration.
    @return Sixteen64x64 indexed pieces and lossless partition provenance.
    @throws ValueError If any pixel escapes, duplicates or leaves a physical cell absent.
    """
    # Native partitioning accepts only the canonical indexed canvas produced
    # by source conversion, so another image mode cannot reinterpret pixel zero.
    if campus.mode != "P" or campus.size != CANVAS:
        raise ValueError("generator whole canvas changes actual native projection")
    pieces = [indexed_canvas((64, 64)) for unused in CELLS]
    # Nearest physical floor contact assigns visible source pixels once, across
    # the overlapping native canvases. It creates no independent tile machine.
    for py in range(campus.height):
        for px in range(campus.width):
            value = campus.getpixel((px, py))
            if value == 0:
                continue
            eligible = [index for index, (ox, oy) in enumerate(ORIGINS) if 0 <= px - ox < 64 and 0 <= py - oy < 64]
            if not eligible:
                raise ValueError(f"generator original pixel leaves native cells: {px},{py}")
            owner = min(eligible, key=lambda index: (abs(px - ORIGINS[index][0] - 31)
                                                    + 2 * abs(py - ORIGINS[index][1] - 48), index))
            ox, oy = ORIGINS[owner]
            pieces[owner].putpixel((px - ox, py - oy), value)
    rebuilt = indexed_canvas(CANVAS)
    # Reconstruct stored pieces independently: a simultaneous whole-image copy
    # would conceal wrong offsets, duplicate ownership or dropped original pixels.
    for piece, (ox, oy) in zip(pieces, ORIGINS):
        for ly in range(64):
            for lx in range(64):
                value = piece.getpixel((lx, ly))
                if value == 0:
                    continue
                point = (ox + lx, oy + ly)
                if rebuilt.getpixel(point) != 0:
                    raise ValueError("generator cells duplicate original pixels")
                rebuilt.putpixel(point, value)
    # All sixteen floor cells must remain visible and the complete original
    # must survive. A giant single-cell fallback cannot satisfy source coverage.
    if rebuilt.tobytes() != campus.tobytes() or any(piece.getbbox() is None for piece in pieces):
        raise ValueError("generator partition loses original pixels or native cells")
    provenance = dict(source)
    provenance.update({"cell_order": CELLS, "native_cell_origins": [list(point) for point in ORIGINS],
                       "partition_policy": "unique_nearest_native_floor_contact_within_actual_cell_canvas",
                       "original_native_sha256": hashlib.sha256(tube.indexed_png(campus)).hexdigest(),
                       "cell_opaque_pixels": [sum(value != 0 for value in piece.get_flattened_data()) for piece in pieces]})
    return pieces, provenance


def ready_models(asset_id, document=None, public=None):
    """Resolve and verify all four genuine completed-equipment source views.

    @param asset_id Exact dated generator family, independent of runtime industry IDs.
    @param document Optional decoded retained source record for isolated refusal checks.
    @param public Optional decoded public Sprite Contract from the same operation.
    @return Ordered direction mapping to native whole image, lossless pieces and provenance.
    @throws ValueError If any direction is absent, duplicated, foreign or misregistered.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    if public is None:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == asset_id)
    spec = next(row for row in public["assets"] if row["asset_id"] == asset_id)
    selections = family.get("whole_view_selections", {})
    # Every physical orientation needs its own selected original registration;
    # repeating a source key cannot masquerade as another actual model view.
    if set(selections) != {"NE", "SE", "SW", "NW"} or len(set(selections.values())) != 4:
        raise ValueError("generator needs four distinct selected view registrations")
    models = {}
    # All views share one physical contract and retain newly visible faces.
    # Pixel signatures independently reject a duplicate raster under a new key.
    for direction in ("NE", "SE", "SW", "NW"):
        campus, source = native_base(selections[direction], document, spec)
        if source["direction"] != direction:
            raise ValueError("generator selected source has the wrong actual direction")
        pieces, provenance = partition_model(campus, source)
        models[direction] = (campus, pieces, provenance)
    if len({row[2]["original_native_sha256"] for row in models.values()}) != 4:
        raise ValueError("generator views repeat the same native original")
    return models


def construction_models(asset_id, stages=(0, 1, 2), document=None, public=None):
    """Resolve only reviewed incomplete hardware for the requested native stages.

    @param asset_id Exact dated generator family with verified physical geometry.
    @param stages Explicit distinct subset of incomplete stages0..2; stage3 deliberately aliases ready hardware later.
    @param document Optional independent source-record snapshot for focused refusal checks.
    @param public Optional matching public Sprite Contract snapshot.
    @return Ordered stage/view mapping to whole original, sixteen lossless pieces and source provenance.
    @throws ValueError If an original/stage/view is missing, unreviewed, substituted or moved after approval.
    """
    # Partial authoring may review supplied stages, but absent stage2 cannot be
    # silently completed by copying another stage or the installed ready model.
    if not stages or len(set(stages)) != len(stages) or any(stage not in (0, 1, 2) for stage in stages):
        raise ValueError("generator needs distinct incomplete construction stages")
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    if public is None:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    spec = next(row for row in public["assets"] if row["asset_id"] == asset_id)
    family = next(row for row in document["families"] if row["asset_id"] == asset_id)
    approved = spec.get("geometry_notes", {}).get("approved_generator_registrations", {})
    models = {}
    # Every requested paid stage owns four independently registered physical
    # views. Source approval and native cell conservation precede composition.
    for stage in stages:
        selections = family.get("construction_frame_sources", {}).get(str(stage), {})
        if set(selections) != {"NE", "SE", "SW", "NW"} or len(set(selections.values())) != 4:
            raise ValueError("generator construction stage needs four distinct reviewed views")
        for direction in ("NE", "SE", "SW", "NW"):
            key = selections[direction]
            source = tube.retained_candidate(key, document)
            # A completed model or another stage/orientation cannot impersonate
            # earned incomplete work merely through a matching source filename.
            if source.get("construction_stage") != stage or source.get("direction") != direction:
                raise ValueError("generator construction source has wrong native stage or view")
            if approved.get(key) != registration_signature(source):
                raise ValueError("generator construction registration differs from reviewed original")
            campus, checked = native_base(key, document, spec)
            pieces, provenance = partition_model(campus, checked)
            models[(stage, direction)] = (campus, pieces, provenance)
    return models


def review_construction_views(asset_id, stages=(0, 1, 2)):
    """Publish explicit incomplete-stage contacts without claiming full state coverage.

    @param asset_id Exact dated generator whose construction sources were reviewed.
    @param stages Explicit incomplete stages to display; missing stages refuse.
    @return Native source/contact receipt path under the reproducible proof cache.
    """
    models = construction_models(asset_id, stages)
    tube.REVIEW.mkdir(parents=True, exist_ok=True)
    stem = MODELS[asset_id][0].lower() + "-construction-" + "-".join(map(str, stages))
    page = Image.new("RGB", (2640, 500 * len(stages)), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    # Adjacent true orientations expose axis/placement drift at the same native
    # scale. Each row remains one earned construction stage, never runtime art.
    for index, ((stage, direction), (campus, unused, source)) in enumerate(models.items()):
        ox, oy = (index % 4) * 660, (index // 4) * 500
        rgba = campus.convert("RGBA").resize((640, 448), Image.Resampling.NEAREST)
        page.paste(rgba, (ox + 10, oy + 30), rgba.getchannel("A"))
        draw.text((ox + 12, oy + 8), f"{direction} / native stage{stage} unfinished / sixteen exact cells", fill=(30, 40, 50))
    target = tube.REVIEW / (stem + ".png")
    page.save(target)
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md55B; scoped incomplete model source, no operating/packed activation",
              "asset_id": asset_id, "stages": list(stages), "views": {f"{stage}:{direction}": row[2] for (stage, direction), row in models.items()},
              "contact": {"path": target.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(target.read_bytes()).hexdigest()},
              "source_coverage_complete": False, "runtime_activated": False}
    receipt = tube.REVIEW / (stem + ".json")
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return receipt


def review_ready_views(asset_id):
    """Bake a combined native/enlarged continuity review of the four selected views.

    @param asset_id Exact dated generator family whose completed hardware is selected.
    @return Review receipt path; source states and loaded runtime remain pending.
    """
    models = ready_models(asset_id)
    tube.REVIEW.mkdir(parents=True, exist_ok=True)
    stem = MODELS[asset_id][0].lower() + "-ready-views"
    page = Image.new("RGB", (1960, 1040), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    # Adjacent panels compare the same fixed projection at native and2x size.
    # Any view-dependent footprint drift remains visible before state authoring.
    for index, (direction, (campus, pieces, source)) in enumerate(models.items()):
        ox, oy = (index % 2) * 980, (index // 2) * 520
        draw.text((ox + 12, oy + 12), asset_id + " / " + direction + " / " + source["key"], fill=(30, 40, 50))
        rgba = campus.convert("RGBA")
        large = rgba.resize((640, 448), Image.Resampling.NEAREST)
        page.paste(large, (ox + 8, oy + 30), large.getchannel("A"))
        page.paste(rgba, (ox + 650, oy + 50), rgba.getchannel("A"))
        draw.text((ox + 660, oy + 300), "Actual native320x224 whole view", fill=(30, 40, 50))
        draw.text((ox + 660, oy + 324), "Sixteen64x64 sections; anchor(-31,-33)", fill=(30, 40, 50))
    target = tube.REVIEW / (stem + ".png")
    page.save(target)
    receipt = tube.REVIEW / (stem + ".json")
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md task55.2/56.2; actual whole-model continuity review",
              "asset_id": asset_id, "native_size": [64, 64], "anchor": [-31, -33], "footprint": [4, 4],
              "views": {direction: source for direction, (unused, unused_parts, source) in models.items()},
              "contact": {"path": target.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(target.read_bytes()).hexdigest()},
              "source_coverage_complete": False, "runtime_activated": False}
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return receipt


def review_model(key):
    """Bake a native/full-cell visual review without claiming complete state source.

    @param key Explicit whole model/view candidate whose immutable bytes are retained.
    @return Model review receipt path with native/contact/partition artifact identities.
    """
    campus, source = native_base(key)
    pieces, partition = partition_model(campus, source)
    tube.REVIEW.mkdir(parents=True, exist_ok=True)
    native_path = tube.REVIEW / (key + "-native.png")
    contact_path = tube.REVIEW / (key + "-contact.png")
    cells_path = tube.REVIEW / (key + "-cells.png")
    native_path.write_bytes(tube.indexed_png(campus))
    rgba = campus.convert("RGBA")
    visible = Image.new("RGB", CANVAS, (235, 239, 243))
    visible.paste(rgba, mask=rgba.getchannel("A"))
    outline = visible.copy()
    pen = ImageDraw.Draw(outline)
    # World-cell outlines reveal the actual floor registration independently
    # of whole-model appeal; each real native cell remains a distinct section.
    for x, y in CELLS:
        cx, cy = 160 + 32 * (y - x), 80 + 16 * (x + y)
        pen.line([(cx, cy - 16), (cx + 32, cy), (cx, cy + 16), (cx - 32, cy), (cx, cy - 16)], fill=(30, 160, 185))
    page = Image.new("RGB", (1320, 780), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    draw.text((14, 12), source["asset_id"] + " / " + source["direction"] + " / BASE REVIEW: 4x4, sixteen actual native cells", fill=(30, 40, 50))
    page.paste(visible.resize((960, 672), Image.Resampling.NEAREST), (12, 42))
    page.paste(outline, (984, 48))
    page.paste(visible, (984, 310))
    draw.text((984, 562), "Native64x64 / anchor(-31,-33)", fill=(30, 40, 50))
    draw.text((984, 586), "Whole source and runtime remain pending", fill=(30, 40, 50))
    page.save(contact_path)
    cells_page = Image.new("RGB", (1080, 1080), (235, 239, 243))
    cells_draw = ImageDraw.Draw(cells_page)
    # Show every lossless cell at4x with its physical coordinate. These crops
    # are source sections, not sixteen independently generated plant designs.
    for index, (piece, cell) in enumerate(zip(pieces, CELLS)):
        px, py = (index % 4) * 270, (index // 4) * 270
        large = piece.convert("RGBA").resize((256, 256), Image.Resampling.NEAREST)
        cells_page.paste(large, (px, py + 14), large.getchannel("A"))
        cells_draw.text((px + 4, py), "WORLD " + str(cell), fill=(30, 40, 50))
    cells_page.save(cells_path)
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md Groups55-56; native original/model review only",
              "source": partition, "source_coverage_complete": False, "runtime_activated": False,
              "artifacts": [{"path": path.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                            for path in (native_path, contact_path, cells_path)]}
    receipt = tube.REVIEW / (key + "-review.json")
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return receipt


def generator_equipment_signature(asset_id, document=None):
    """Freeze authored glass ancestry, physical fittings and native frame meaning.

    @param asset_id Exact dated generator family whose hardware is already reviewed.
    @param document Optional independent retained-source snapshot for refusal checks.
    @return SHA256 of original component identities and the complete equipment policy.
    @throws ValueError If components, phase coverage, fittings or native work meaning disagree.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == asset_id)
    components = json.loads(tube.base.repository_path(family["indicator_record"]).read_text(encoding="utf-8"))
    component = next(row for row in components["components"] if row["key"] == family["indicator_key"])
    heat = next(row for row in document["components"] if row["key"] == family["heat_starved_indicator_key"])
    facilities.verify_original(component)
    facilities.verify_original(heat)
    keys = set(family["whole_view_selections"].values())
    keys.update(key for views in family["construction_frame_sources"].values() for key in views.values())
    # Construction frames represent paid quarters, while completed frames are
    # visual phases. Neither final hardware nor a lamp phase completes paid work.
    if (component["states"] != facilities.STATES or component["frames"] != 4 or component["grid"] != [4, 4]
            or heat["states"] != ["heat_starved"] or heat["frames"] != 4 or len(heat["glass_boxes"]) != 4
            or set(family["indicator_targets"]) != keys or family["construction_indicator_phase"] != 0
            or family["construction_frame_sources"].get("3") != family["whole_view_selections"]
            or family["foreground_policy"] != FOREGROUND):
        raise ValueError("generator equipment/native-frame coverage mismatch")
    if asset_id == "facility.vent-powerhouse":
        if family["construction_ticks"] != {"total": 592, "stage_length": 148, "stages": [0, 1, 2, 3], "completion_separate": True}:
            raise ValueError("generator construction changes native paid work quarters")
    # Reviewed glass interiors are at most three native pixels across. This
    # protects the existing mounting, metal casing and every floor contact.
    for target in family["indicator_targets"].values():
        if (len(target) != 4 or not all(type(value) is int for value in target)
                or not 1 <= target[2] <= 3 or not 1 <= target[3] <= 3
                or not 0 <= target[0] <= CANVAS[0] - target[2]
                or not 0 <= target[1] <= CANVAS[1] - target[3]):
            raise ValueError("generator glass leaves native fitting")
    selected = {name: family[name] for name in ("indicator_record", "indicator_key", "heat_starved_indicator_key",
                "indicator_targets", "construction_indicator_phase", "construction_ticks", "construction_stage3_policy",
                "foreground_policy", "frame_owner", "state_owner")}
    selected["component"] = {name: component[name] for name in ("path", "sha256", "references", "states", "frames", "grid")}
    selected["heat_component"] = {name: heat[name] for name in ("path", "sha256", "references", "states", "frames", "glass_boxes")}
    return hashlib.sha256(json.dumps(selected, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def generator_glass_phases(family, document):
    """Resolve real authored glass for the four common modes and Heat shortage.

    @param family Reviewed generator family naming the two immutable component owners.
    @param document Retained-source snapshot containing the additional Heat component.
    @return State mapping to four opaque3x3 RGBA phases and their original ancestry.
    @throws ValueError If Heat crops overlap, escape their original or omit opaque glass.
    """
    record = json.loads(tube.base.repository_path(family["indicator_record"]).read_text(encoding="utf-8"))
    shared = next(row for row in record["components"] if row["key"] == family["indicator_key"])
    phases = facilities.indicator(shared)
    result = {state: (phases[index], shared) for index, state in enumerate(facilities.STATES)}
    heat = next(row for row in document["components"] if row["key"] == family["heat_starved_indicator_key"])
    facilities.verify_original(heat)
    selected = []
    with Image.open(tube.base.repository_path(heat["path"])) as original:
        original = original.convert("RGBA")
        # Each phase owns its own measured rectangle. An adjacent highlight or
        # a transparent gutter cannot masquerade as a genuinely authored phase.
        for index, box in enumerate(heat["glass_boxes"]):
            left, top, right, bottom = box
            if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
                raise ValueError("generator Heat glass crop leaves original")
            for previous in heat["glass_boxes"][:index]:
                if max(left, previous[0]) < min(right, previous[2]) and max(top, previous[1]) < min(bottom, previous[3]):
                    raise ValueError("generator Heat glass crops overlap")
            glass = original.crop(box)
            if glass.getchannel("A").getextrema()[0] < 128:
                raise ValueError("generator Heat glass is transparent")
            selected.append(glass.resize((3, 3), Image.Resampling.BOX))
    result["heat_starved"] = (selected, heat)
    return result


def generator_source_inputs(asset_id, document=None, public=None):
    """Resolve the complete reviewed hardware and equipment without activating it.

    @param asset_id Exact dated generator source family, independent of IndustryIDs.
    @param document Optional source snapshot; mutations never fall back to disk state.
    @param public Optional matching public Sprite Contract snapshot.
    @return Family, public row, state glass mapping and sixteen unique hardware models.
    @throws ValueError If coverage, frozen approval or native frame ownership disagrees.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    if public is None:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == asset_id)
    spec = next(row for row in public["assets"] if row["asset_id"] == asset_id)
    # Full source needs all actual modes and sixteen native cells. Metadata
    # cannot reinterpret an incomplete matrix or introduce a vehicle aperture.
    if (family["states"] != STATES or spec["states"] != STATES or family["directions"] != list(DIRECTIONS)
            or spec["directions"] != list(DIRECTIONS) or family["frames"] != 4 or spec["frames"] != 4
            or family["parts"] != ["back", "front"] or spec["parts"] != family["parts"]
            or family["frame_owner"] != spec["frame_owner"] or family["state_owner"] != spec["state_owner"]
            or set(family["construction_frame_sources"]) != {"0", "1", "2", "3"}):
        raise ValueError("generator native state/frame ownership mismatch")
    if generator_equipment_signature(asset_id, document) != spec["geometry_notes"].get("approved_generator_equipment_sha256"):
        raise ValueError("generator equipment differs from approved original fittings")
    ready = ready_models(asset_id, document, public)
    incomplete = construction_models(asset_id, document=document, public=public)
    models = {row[2]["key"]: row for row in list(ready.values()) + list(incomplete.values())}
    return family, spec, generator_glass_phases(family, document), models


def compose_generator_glass(original, pieces, component, target):
    """Replace only the reviewed glass pixels in their existing native cell owner.

    @param original Approved indexed320x224 whole model with fixed native opacity.
    @param pieces Sixteen independently validated lossless64x64 original sections.
    @param component Genuine authored opaque3x3 RGBA glass phase.
    @param target Reviewed x,y,width,height interior of the original cabinet lamp.
    @return Whole composed pose and sixteen copied sections, exact outside the fitting.
    @throws ValueError If glass becomes transparent or its original cell ownership changes.
    """
    x, y, width, height = target
    # A component can replace opaque glass only. It cannot paint a missing
    # mounting, repair clipped machinery or spill across the native canvas.
    if (original.mode != "P" or original.size != CANVAS or len(pieces) != 16
            or not 1 <= width <= 3 or not 1 <= height <= 3
            or not 0 <= x <= CANVAS[0] - width or not 0 <= y <= CANVAS[1] - height
            or component.getchannel("A").getextrema()[0] < 128):
        raise ValueError("generator glass leaves native fitting or loses opacity")
    glass = tube.base.quantize(component.resize((width, height), Image.Resampling.BOX), {})
    pose, sections = original.copy(), [piece.copy() for piece in pieces]
    # Static hardware is partitioned once. Updating only its tiny owned glass
    # avoids rescanning the whole campus for each visual phase, while retaining
    # the exact same physical ownership rule and checking the stored original.
    for dy in range(height):
        for dx in range(width):
            px, py = x + dx, y + dy
            value, before = glass.getpixel((dx, dy)), original.getpixel((px, py))
            if value == 0 or before == 0:
                raise ValueError("generator glass fitting is not opaque")
            owners = [index for index, (ox, oy) in enumerate(ORIGINS)
                      if 0 <= px - ox < 64 and 0 <= py - oy < 64 and pieces[index].getpixel((px - ox, py - oy)) != 0]
            if len(owners) != 1:
                raise ValueError("generator glass has no unique original cell owner")
            ox, oy = ORIGINS[owners[0]]
            if pieces[owners[0]].getpixel((px - ox, py - oy)) != before:
                raise ValueError("generator glass cell differs from original hardware")
            pose.putpixel((px, py), value)
            sections[owners[0]].putpixel((px - ox, py - oy), value)
    pose.info["transparency"] = 0
    return pose, sections


def build_generator(asset_id, document=None, public=None):
    """Compose the exact five-state2560-pose source from retained original pixels.

    @param asset_id Dated generator with complete reviewed original hardware and glass.
    @param document Optional independent source snapshot for focused refusal checks.
    @param public Optional corresponding public contract snapshot.
    @return Complete source mapping and deterministic indexed atlas PNG bytes.
    """
    family, spec, phases, models = generator_source_inputs(asset_id, document, public)
    atlas = indexed_canvas((8192, 1280))
    sprites = []
    # Native construction selects four incomplete stages, including final
    # hardware under amber before completion. Actual dated work timing remains
    # feature-owned; the passive2000 source descriptor supplies no invoice.
    # Other states preserve ready hardware and
    # select only genuine visual glass phases; no quantity or work timer changes.
    for state_index, state in enumerate(STATES):
        for frame in range(4):
            for view, direction in enumerate(DIRECTIONS):
                stage = frame if state == "construction" else None
                key = family["construction_frame_sources"][str(frame)][direction] if stage is not None else family["whole_view_selections"][direction]
                original, pieces, source = models[key]
                phase = family["construction_indicator_phase"] if stage is not None else frame
                glass, component = phases[state]
                target = family["indicator_targets"][key]
                unused, sections = compose_generator_glass(original, pieces, glass[phase], target)
                for tile, piece in enumerate(sections):
                    # Native solid industry cells retain identical back/front
                    # sections, matching the delivered collector convention.
                    # There is no invented transport aperture or new floor cut.
                    for part_index, part in enumerate(("back", "front")):
                        x, y = (view * 32 + tile * 2 + part_index) * 64, (state_index * 4 + frame) * 64
                        atlas.paste(piece, (x, y))
                        sprites.append({"asset_id": asset_id, "state": state, "frame": frame, "direction": direction,
                                        "tile_variant": tile, "world_cell": CELLS[tile], "part": part, "anchor": [-31, -33],
                                        "box": [x, y, 64, 64], "source_key": key, "source": source["path"],
                                        "source_sha256": source["sha256"], "native_crop": source["native_crop"],
                                        "native_subject_size": source["native_subject_size"], "native_position": source["native_position"],
                                        "native_construction_stage": stage,
                                        "indicator": {"path": component["path"], "sha256": component["sha256"],
                                                      "state": state, "frame": phase, "target": target}})
    stem = MODELS[asset_id][0].lower()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md55B; archi-map-layers.md; source only, packing/operation pending",
              "schema_version": 1, "asset_id": asset_id, "source_record": RECORD.relative_to(tube.ROOT).as_posix(),
              "path": (tube.REVIEW / (stem + "-authored.png")).relative_to(tube.ROOT).as_posix(), "size": [8192, 1280],
              "native_cell": [64, 64], "anchor": [-31, -33], "pose_count": len(sprites), "sprites": sprites,
              "frame_owner": family["frame_owner"], "state_owner": family["state_owner"], "foreground_policy": FOREGROUND,
              "construction_ticks": family["construction_ticks"], "source_coverage_complete": True, "runtime_activated": False,
              "approved_equipment_sha256": spec["geometry_notes"]["approved_generator_equipment_sha256"]}
    return report, tube.indexed_png(atlas)


def assemble_generator_pose(report, atlas, state, frame, direction, part="back"):
    """Reconstruct actual stored cells through their verified native world origins.

    @param report Complete source mapping with canonical world-cell and atlas boxes.
    @param atlas Decoded indexed8192x1280 source atlas.
    @param state One of the five actual presentation modes.
    @param frame Native paid construction quarter or completed equipment phase.
    @param direction Genuine NE/SE/SW/NW physical model view.
    @param part Aligned back or sortable front sections to inspect.
    @return Complete indexed320x224 pose assembled from sixteen actual sections.
    """
    pose = indexed_canvas(CANVAS)
    # Each original pixel has one physical cell owner; world geometry controls
    # assembly, independent of source-sheet order or overlapping sprite canvases.
    for row in report["sprites"]:
        if (row["state"], row["frame"], row["direction"], row["part"]) != (state, frame, direction, part):
            continue
        x, y, width, height = row["box"]
        piece = atlas.crop((x, y, x + width, y + height))
        pose.paste(piece, ORIGINS[row["tile_variant"]], piece.convert("RGBA").getchannel("A"))
    return pose


def validate_generator(report, encoded, document=None, public=None):
    """Refuse false coverage, provenance, ownership and altered original source pixels.

    @param report Complete source mapping submitted for conversion or packing.
    @param encoded Actual indexed atlas PNG bytes, never a declared hash alone.
    @param document Optional independent source record for malformed-input checks.
    @param public Optional matching Sprite Contract snapshot.
    @return None; successful source validation grants no packed range or operation.
    @throws ValueError If any required pose, encoded pixel or source policy differs.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    families = [row for row in document["families"] if row["asset_id"] == report["asset_id"]]
    # Source identity and native encoding cannot be redirected to another asset
    # merely by editing the report's top-level fields or its completion flags.
    if (len(families) != 1 or report["asset_id"] not in MODELS or report["size"] != [8192, 1280]
            or report["native_cell"] != [64, 64] or report["anchor"] != [-31, -33]
            or report["source_coverage_complete"] is not True or report["runtime_activated"] is not False):
        raise ValueError("generator source family/native canvas identity mismatch")
    family = families[0]
    required = {(state, frame, direction, tile, part) for state in STATES for frame in range(4)
                for direction in DIRECTIONS for tile in range(16) for part in ("back", "front")}
    actual = [(row["state"], row["frame"], row["direction"], row["tile_variant"], row["part"]) for row in report["sprites"]]
    # Unique semantic identities establish full coverage. A padded count, missing
    # native cell or duplicate direction cannot stand in for an authored pose.
    if len(actual) != len(required) or set(actual) != required or report["pose_count"] != len(required):
        raise ValueError("incomplete generator state/stage/view/cell/part coverage")
    candidates = {row["key"]: row for row in document["candidates"]}
    # Cheap independent ancestry checks precede a full source rebuild. They
    # distinguish paid construction stages from visual phases and bind every
    # cell to its real original, equipment fitting and physical world location.
    for row in report["sprites"]:
        construction = row["state"] == "construction"
        key = family["construction_frame_sources"][str(row["frame"])][row["direction"]] if construction else family["whole_view_selections"][row["direction"]]
        source = candidates[key]
        phase = 0 if construction else row["frame"]
        stage = row["frame"] if construction else None
        if (row["asset_id"] != family["asset_id"] or row["source_key"] != key or row["source"] != source["path"]
                or row["source_sha256"] != source["sha256"] or row["native_construction_stage"] != stage
                or row["indicator"]["state"] != row["state"] or row["indicator"]["frame"] != phase
                or row["indicator"]["target"] != family["indicator_targets"][key]
                or row["world_cell"] != CELLS[row["tile_variant"]] or row["anchor"] != [-31, -33]):
            raise ValueError("generator false original/frame/equipment provenance")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Native source must retain transparent zero and the canonical palette;
        # animated/remap indices cannot change authored equipment during display.
        if (atlas.mode != "P" or atlas.size != (8192, 1280) or atlas.info.get("transparency") != 0
                or atlas.getpalette()[:768] != tube.base.PALETTE_BYTES
                or set(atlas.get_flattened_data()) & (tube.base.FORBIDDEN - {0})):
            raise ValueError("generator native palette/canvas mismatch")
    expected, pixels = build_generator(report["asset_id"], document, public)
    # Regeneration binds every encoded pixel to immutable hardware and genuine
    # component ancestry. Correct-looking art with forged metadata still refuses.
    if report["sprites"] != expected["sprites"] or encoded != pixels:
        raise ValueError("generator source changes original model/component pixels or ancestry")
    for field, value in expected.items():
        # Presentation labels may explain a report, but every semantic field
        # must match regenerated source ownership and its frozen approval.
        if field != "_documentation" and report[field] != value:
            raise ValueError("generator source changes native ownership or approval")


def generator_contact_pages(report, encoded):
    """Display all paid stages and genuine equipment phases in every physical view.

    @param report Complete source mapping, retaining the original source ancestry.
    @param encoded Actual source PNG whose stored cells supply every review pose.
    @return Direction-indexed deterministic PNG pages for scoped model review.
    """
    pages = {}
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Per-view matrices expose every construction and operating presentation
        # without losing a native section or confusing visual phases with work.
        for direction in DIRECTIONS:
            page = Image.new("RGB", (2640, 2500), (235, 239, 243))
            draw = ImageDraw.Draw(page)
            for state_index, state in enumerate(STATES):
                for frame in range(4):
                    pose = assemble_generator_pose(report, atlas, state, frame, direction).convert("RGBA")
                    x, y = frame * 660, state_index * 500
                    label = f"{direction} {state}: " + (f"native stage{frame}, unfinished" if state == "construction" else f"original glass phase{frame}")
                    draw.text((x + 10, y + 8), label, fill=(30, 40, 50))
                    draw.text((x + 10, y + 24), "Sixteen actual cells; source only; packing/operating activation pending", fill=(30, 40, 50))
                    large = pose.resize((640, 448), Image.Resampling.NEAREST)
                    page.paste(large, (x + 10, y + 42), large.getchannel("A"))
            stream = io.BytesIO()
            page.save(stream, format="PNG")
            pages[direction] = stream.getvalue()
    return pages


def publish_generator(asset_id, check=False, bind=False):
    """Publish deterministic reviewed source artifacts and optionally its public handoff.

    @param asset_id Exact generator family to compose; other family metadata stays exact.
    @param check True compares existing source/mapping/contact bytes read-only.
    @param bind True publishes only source coverage after the caller's model review.
    @return Complete report path beneath the reproducible native-art proof cache.
    @throws ValueError If regeneration differs from retained proof or check requests a write.
    """
    # A verification invocation must never become a source-publication write.
    if check and bind:
        raise ValueError("generator read-only check cannot publish source metadata")
    report, encoded = build_generator(asset_id)
    validate_generator(report, encoded)
    tube.REVIEW.mkdir(parents=True, exist_ok=True)
    report.update(sha256=hashlib.sha256(encoded).hexdigest(), contact_pages=[])
    files = {tube.base.repository_path(report["path"]): encoded}
    stem = MODELS[asset_id][0].lower()
    # A complete handoff includes every physical view's entire state/phase
    # matrix. Missing or stale contact pages invalidate deterministic proof.
    for direction, pixels in generator_contact_pages(report, encoded).items():
        path = tube.REVIEW / (stem + "-" + direction.lower() + "-state-contact.png")
        files[path] = pixels
        report["contact_pages"].append({"direction": direction, "path": path.relative_to(tube.ROOT).as_posix(),
                                        "sha256": hashlib.sha256(pixels).hexdigest()})
    target = tube.REVIEW / (stem + "-source-manifest.json")
    serialized = json.dumps(report, indent=2) + "\n"
    if check:
        # Packing consumers need the exact mapping and every reviewed contact
        # to regenerate together, rather than accepting only the atlas hash.
        if any(path.read_bytes() != pixels for path, pixels in files.items()) or target.read_text(encoding="utf-8") != serialized:
            raise ValueError("generator source proof needs deterministic regeneration")
    else:
        for path, pixels in files.items():
            path.write_bytes(pixels)
        target.write_text(serialized, encoding="utf-8")
    if bind:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
        spec = next(row for row in public["assets"] if row["asset_id"] == asset_id)
        first = report["sprites"][0]
        spec["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                          "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
        originals = list(dict.fromkeys(row["source"] for row in report["sprites"]))
        originals.extend(path for path in dict.fromkeys(row["indicator"]["path"] for row in report["sprites"]) if path not in originals)
        spec["authoring"] = {"date": "2026-10-11", "status": "complete_native_generator_source_packing_operation_pending",
                             "source_record": report["source_record"], "originals": originals,
                             "atlas_manifest": target.relative_to(tube.ROOT).as_posix(), "native_atlas": report["path"],
                             "native_sha256": report["sha256"], "staged_pose_count": 2560,
                             "coverage_complete": True, "runtime_activated": False}
        tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Review explicit originals or compose the reviewed complete source handoff.

    @return None; print the exact receipt path; operating and packed activation stay separate.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--candidate", help="Retained dated generator original/view key")
    mode.add_argument("--ready-views", choices=tuple(MODELS), help="Four selected completed-equipment views")
    mode.add_argument("--construction-views", choices=tuple(MODELS), help="Explicit reviewed incomplete construction stages")
    mode.add_argument("--asset", choices=tuple(MODELS), help="Compose the complete reviewed generator source")
    parser.add_argument("--stages", nargs="+", type=int, default=[0, 1, 2], help="Incomplete native stages for construction review")
    operation = parser.add_mutually_exclusive_group()
    operation.add_argument("--check", action="store_true", help="Read-only deterministic full-source verification")
    operation.add_argument("--publish", action="store_true", help="Bind reviewed complete source, leaving packing/runtime pending")
    arguments = parser.parse_args()
    if arguments.asset:
        print(publish_generator(arguments.asset, arguments.check, arguments.publish))
    elif arguments.construction_views:
        print(review_construction_views(arguments.construction_views, tuple(arguments.stages)))
    else:
        print(review_ready_views(arguments.ready_views) if arguments.ready_views else review_model(arguments.candidate))


if __name__ == "__main__":
    main()
