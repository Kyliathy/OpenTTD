"""Review original geothermal models against their actual native world floor.

@file convert_geothermal_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups54B/54BB/54BC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original whole models are losslessly sliced into native cells; state/runtime work remains explicit.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube
import convert_facility_art as facilities
from convert_portal_art import native_housing


RECORD = tube.ROOT / "media/multilayer/source/ground/geothermal-generation-record.json"
COLLECTOR_REPORT = tube.REVIEW / "collector-source-manifest.json"
DIRECTIONS = ("NE", "SE", "SW", "NW")


def collector_view_signature(document=None):
    """Bind four genuinely authored ready views and their complete native registration.

    @param document Optional already decoded immutable source/selection record.
    @return Stable SHA256 of the four approved original views and native transforms.
    @throws ValueError If a missing, duplicate or excluded view would become fallback art.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == "facility.vent-collector")
    selections = family["whole_view_selections"]
    if set(selections) != {"NE", "SE", "SW", "NW"}:
        raise ValueError("collector approval needs four actual whole-model views")
    approved = []
    # Every heading names its own genuine whole-model original. Neither a
    # relabelled source nor an excluded draft can acquire a missing physical view.
    for direction in ("NE", "SE", "SW", "NW"):
        source = tube.retained_candidate(selections[direction], document)
        if (source.get("direction") != direction or direction in source.get("excluded_views", [])
                or source["asset_id"] != "facility.vent-collector"):
            raise ValueError("collector approval changes an actual original view")
        approved.append({name: source[name] for name in ("key", "path", "sha256", "references", "direction",
                                                         "native_geometry", "native_crop", "native_subject_size",
                                                         "native_position", "native_floor_corners")})
    if len({row["sha256"] for row in approved}) != 4:
        raise ValueError("collector headings require four genuine original images")
    return hashlib.sha256(json.dumps(approved, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()


def review_ready_views():
    """Compare all four approved original ready views with honest partial coverage.

    @return Ready-view manifest path; construction/phases/front parts remain pending.
    """
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == "facility.vent-collector")
    signature = collector_view_signature(document)
    page = Image.new("RGB", (1560, 1240), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    views = []
    # Genuine neighbouring views retain their own original faces under one
    # camera/light. Comparison never rotates a raster or fills a missing view.
    for index, direction in enumerate(("NE", "SE", "SW", "NW")):
        key = family["whole_view_selections"][direction]
        native, source = collector_base(key)
        pieces, unused, labels = partition_collector(key)
        rgba = native.convert("RGBA")
        visible = Image.new("RGB", native.size, (235, 239, 243))
        visible.paste(rgba, mask=rgba.getchannel("A"))
        left, top = (index % 2) * 780, (index // 2) * 620
        draw.text((left + 12, top + 12), direction + ": genuine original; nine lossless native cells; ready model only", fill=(30, 40, 50))
        page.paste(visible.resize((768, 576), Image.Resampling.NEAREST), (left + 6, top + 38))
        views.append({"direction": direction, "source_key": key, "source": source["path"], "source_sha256": source["sha256"],
                      "native_crop": source["native_crop"], "native_subject_size": source["native_subject_size"],
                      "native_position": source["native_position"], "native_cells": len(pieces),
                      "cell_manifest": (tube.REVIEW / (key + "-cells-review.json")).relative_to(tube.ROOT).as_posix()})
    contact = tube.REVIEW / "collector-ready-views-contact.png"
    page.save(contact)
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md54B.2; archi-map-layers.md; ready original source only",
              "asset_id": family["asset_id"], "ready_view_coverage_complete": True, "source_coverage_complete": False,
              "runtime_activated": False, "ready_native_cells": 36, "approved_views_sha256": signature, "views": views,
              "contact": {"path": contact.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(contact.read_bytes()).hexdigest()}}
    target = tube.REVIEW / "collector-ready-views-manifest.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def collector_construction_signature(document=None):
    """Freeze the real four native construction stages and their original views.

    @param document Optional decoded source record, including reviewed stage selections.
    @return SHA256 of all stage/view identities and complete native registrations.
    @throws ValueError If an incomplete original is missing or final hardware is substituted.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == "facility.vent-collector")
    stages = family["construction_frame_sources"]
    if set(stages) != {"0", "1", "2", "3"}:
        raise ValueError("collector construction needs four actual native stages")
    approved = []
    incomplete_hashes = set()
    # Each paid native quarter retains four genuinely authored model views.
    # Final-looking stage3 deliberately reuses ready hardware; the independent
    # completion gate, not a different model, controls permission to publish Heat.
    for stage in range(4):
        selections = stages[str(stage)]
        if set(selections) != {"NE", "SE", "SW", "NW"}:
            raise ValueError("collector construction needs four genuine views per stage")
        for direction in ("NE", "SE", "SW", "NW"):
            source = tube.retained_candidate(selections[direction], document)
            # No relabelled view, discarded draft or other industry can fill a
            # required native frame and silently alter its physical registration.
            if (source["asset_id"] != family["asset_id"] or source["direction"] != direction
                    or direction in source.get("excluded_views", [])):
                raise ValueError("collector construction changes an actual original view")
            if stage == 3:
                if selections[direction] != family["whole_view_selections"][direction]:
                    raise ValueError("collector final construction stage must retain approved hardware")
            else:
                # Earlier quarters must depict their own unfinished machinery,
                # rather than recolouring completed equipment as paid progress.
                if source.get("model_state") != "construction" or source.get("construction_stage") != stage:
                    raise ValueError("collector incomplete stage needs its actual construction original")
                incomplete_hashes.add(source["sha256"])
            row = {name: source[name] for name in ("key", "path", "sha256", "references", "direction",
                                                   "native_geometry", "native_crop", "native_subject_size",
                                                   "native_position", "native_floor_corners")}
            approved.append({"construction_stage": stage, **row})
    if len(incomplete_hashes) != 12:
        raise ValueError("collector incomplete stages need twelve genuine original images")
    return hashlib.sha256(json.dumps(approved, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()


def review_construction_views():
    """Review original model progression without claiming composed state/runtime coverage.

    @return Path to the sixteen-view native construction-hardware review manifest.
    """
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == "facility.vent-collector")
    signature = collector_construction_signature(document)
    page = Image.new("RGB", (2080, 1680), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    views = []
    # Rows compare genuine access-side views; columns follow actual paid native
    # progress. Stage3 displays frozen hardware only, with its construction-glass
    # composition still explicitly owned by the next equipment-state group.
    for row, direction in enumerate(("NE", "SE", "SW", "NW")):
        for stage in range(4):
            key = family["construction_frame_sources"][str(stage)][direction]
            native, source = collector_base(key)
            pieces, unused_source, unused_roles = partition_collector(key)
            rgba = native.convert("RGBA")
            visible = Image.new("RGB", native.size, (235, 239, 243))
            visible.paste(rgba, mask=rgba.getchannel("A"))
            left, top = stage * 520, row * 420
            label = f"{direction} native stage{stage}: paid work {stage * 74}..{stage * 74 + 73}; unfinished"
            draw.text((left + 8, top + 8), label, fill=(30, 40, 50))
            draw.text((left + 8, top + 24), "Original hardware; final construction glass composed in54BC", fill=(30, 40, 50))
            page.paste(visible.resize((512, 384), Image.Resampling.NEAREST), (left + 4, top + 36))
            views.append({"direction": direction, "construction_stage": stage, "source_key": key,
                          "source": source["path"], "source_sha256": source["sha256"], "native_cells": len(pieces),
                          "native_crop": source["native_crop"], "native_subject_size": source["native_subject_size"],
                          "native_position": source["native_position"], "final_glass_composed": False})
    contact = tube.REVIEW / "collector-construction-views-contact.png"
    page.save(contact)
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md54BB; archi-map-layers.md; original construction hardware only",
              "asset_id": family["asset_id"], "construction_hardware_coverage_complete": True,
              "source_coverage_complete": False, "runtime_activated": False, "native_cells": 144,
              "approved_construction_views_sha256": signature, "views": views,
              "contact": {"path": contact.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(contact.read_bytes()).hexdigest()}}
    target = tube.REVIEW / "collector-construction-views-manifest.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def partition_collector(key):
    """Slice one original native industry view into nine lossless cell sprites.

    @param key Exact whole-campus original with measured canonical native registration.
    @return Nine indexed cell pieces, original/partition provenance and world-cell labels.
    @throws ValueError If any original pixel escapes the real native sprite canvases.
    """
    campus, original = collector_base(key)
    return partition_collector_model(campus, original)


def partition_collector_model(campus, original):
    """Partition an original whole model after its registered equipment insert.

    @param campus Native indexed original with only approved glass pixels composed.
    @param original Checked immutable original metadata and native registration.
    @return Nine lossless indexed pieces, original provenance and world-cell labels.
    @throws ValueError If the complete original cannot fit the actual native cells.
    """
    world_cells = [[x, y] for y in range(3) for x in range(3)]
    pieces = [Image.new("P", (64, 64), 0) for unused in world_cells]
    for piece in pieces:
        piece.putpalette(tube.base.PALETTE_BYTES)
        piece.info["transparency"] = 0
    origins = [(97 + 32 * (y - x), 31 + 16 * (x + y)) for x, y in world_cells]
    # All nine TIDS cells belong to one actual IndustryID. Sprite ownership
    # partitions its already authored surface; it creates no separate machinery,
    # stock or natural source. Nearest floor contact gives every visible pixel
    # exactly one eligible64x64 native owner, retaining the complete source.
    for py in range(campus.height):
        for px in range(campus.width):
            value = campus.getpixel((px, py))
            if value == 0:
                continue
            eligible = [index for index, (ox, oy) in enumerate(origins) if 0 <= px - ox < 64 and 0 <= py - oy < 64]
            if not eligible:
                raise ValueError("collector original pixel leaves native cell canvases")
            owner = min(eligible, key=lambda index: (abs(px - origins[index][0] - 31)
                                                    + 2 * abs(py - origins[index][1] - 48), index))
            ox, oy = origins[owner]
            pieces[owner].putpixel((px - ox, py - oy), value)
    reconstruction = Image.new("P", campus.size, 0)
    reconstruction.putpalette(tube.base.PALETTE_BYTES)
    # Read the actual stored pieces back through their own native offsets.
    # This independently detects duplicate pixels, lost artwork or a misplaced
    # tile instead of comparing the original against a simultaneous copy.
    for piece, (ox, oy) in zip(pieces, origins):
        for ly in range(64):
            for lx in range(64):
                value = piece.getpixel((lx, ly))
                if value == 0:
                    continue
                point = (ox + lx, oy + ly)
                if reconstruction.getpixel(point) != 0:
                    raise ValueError("collector native cells duplicate original pixels")
                reconstruction.putpixel(point, value)
    # Lossless native packing must neither paint a missing connection nor
    # discard an original feature. Each physical native cell also remains
    # represented instead of hiding the whole campus in one giant sprite.
    if reconstruction.tobytes() != campus.tobytes() or any(piece.getbbox() is None for piece in pieces):
        raise ValueError("collector partition loses original pixels or a native cell")
    source = dict(original)
    source["cell_order"] = world_cells
    source["native_cell_origin"] = [97, 31]
    source["partition_policy"] = "unique_nearest_native_floor_contact_within_actual_cell_canvas"
    source["resolved_cell_sources"] = [{"world_cell": point, "source_key": original["key"],
                                        "source": original["path"], "source_sha256": original["sha256"],
                                        "source_crop": original["native_crop"], "native_source_box": [*origin, 64, 64],
                                        "opaque_pixels": sum(value != 0 for value in piece.get_flattened_data())}
                                       for point, origin, piece in zip(world_cells, origins, pieces)]
    source["original_native_sha256"] = hashlib.sha256(tube.indexed_png(campus)).hexdigest()
    return pieces, source, [{"world_cell": point, "pipe_sides": []} for point in world_cells]


def collector_equipment_signature(document=None):
    """Freeze genuine status-glass ancestry and every reviewed native fitting.

    @param document Optional decoded collector source record for isolated validation.
    @return SHA256 of the component identity, exact targets and construction phase policy.
    @throws ValueError If a source fitting, component or actual frame meaning is missing.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == "facility.vent-collector")
    component_record = tube.base.repository_path(family["indicator_record"])
    components = json.loads(component_record.read_text(encoding="utf-8"))
    matches = [row for row in components["components"] if row["key"] == family["indicator_key"]]
    if len(matches) != 1:
        raise ValueError("collector needs one genuine status component")
    component = matches[0]
    facilities.verify_original(component)
    keys = {key for views in family["construction_frame_sources"].values() for key in views.values()}
    # Every original model needs its own physical fitting. The construction
    # frame already denotes paid work, so it cannot also select a blinking phase.
    if (set(family["indicator_targets"]) != keys or family["construction_indicator_phase"] != 0
            or component["states"] != facilities.STATES or component["frames"] != 4 or component["grid"] != [4, 4]):
        raise ValueError("collector equipment/frame coverage mismatch")
    for target in family["indicator_targets"].values():
        # A tiny reviewed glass interior preserves its own metal mounting and
        # every floor contact; a larger rectangle would recolour real machinery.
        if (len(target) != 4 or not all(isinstance(value, int) for value in target)
                or not 1 <= target[2] <= 3 or not 1 <= target[3] <= 3
                or not 0 <= target[0] <= 256 - target[2] or not 0 <= target[1] <= 192 - target[3]):
            raise ValueError("collector indicator target leaves native fitting")
    approved = {"indicator_record": family["indicator_record"], "indicator_key": family["indicator_key"],
                "component": {name: component[name] for name in ("path", "sha256", "references", "grid", "states", "frames")},
                "indicator_targets": family["indicator_targets"], "construction_indicator_phase": 0,
                "foreground_policy": family["foreground_policy"]}
    return hashlib.sha256(json.dumps(approved, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()


def insert_collector_glass(original, component, target):
    """Insert genuine component glass into a whole native model before sectioning.

    @param original Approved complete indexed model with canonical DOS palette.
    @param component Actual authored opaque3x3 RGBA equipment-glass phase.
    @param target Reviewed x,y,width,height inside that model's real glass fitting.
    @return Indexed whole pose; alpha and every pixel outside the fitting stay exact.
    @throws ValueError If an insert would create opacity or leave its physical fitting.
    """
    x, y, width, height = target
    # Registered tiny glass cannot grow into a frame, pipe or transparent floor
    # edge. These bounds also protect isolated malformed conversion inputs.
    if (original.mode != "P" or original.size != (256, 192) or not 1 <= width <= 3 or not 1 <= height <= 3
            or not 0 <= x <= 256 - width or not 0 <= y <= 192 - height):
        raise ValueError("collector indicator target leaves native fitting")
    if component.getchannel("A").getextrema()[0] < 128:
        raise ValueError("collector indicator glass is transparent")
    glass = tube.base.quantize(component.resize((width, height), Image.Resampling.BOX), {})
    pose = original.copy()
    # Work on indexed original pixels directly: resampling/quantizing the whole
    # campus again could alter a steel edge or palette alias outside the glass.
    for dy in range(height):
        for dx in range(width):
            if original.getpixel((x + dx, y + dy)) == 0:
                raise ValueError("collector indicator fitting is not opaque")
            value = glass.getpixel((dx, dy))
            if value == 0:
                raise ValueError("collector indicator glass becomes transparent")
            pose.putpixel((x + dx, y + dy), value)
    pose.info["transparency"] = 0
    return pose


def collector_source_inputs(document=None):
    """Resolve approved native models and the original equipment component once.

    @param document Optional decoded collector source record.
    @return Source family, public family, component, authored phases and native model cache.
    @throws ValueError If frame ownership, native geometry or frozen approvals disagree.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == "facility.vent-collector")
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    spec = next(row for row in public["assets"] if row["asset_id"] == family["asset_id"])
    # Native work/completion and shared publication own the presentation states.
    # Art cannot add a thermal consumer, another work clock or a vehicle aperture.
    if (family["directions"] != list(DIRECTIONS) or family["states"] != facilities.STATES
            or family["frames"] != 4 or family["parts"] != ["back", "front"]
            or spec["states"] != family["states"] or spec["directions"] != family["directions"]
            or spec["frames"] != 4 or spec["parts"] != family["parts"]
            or spec["frame_owner"] != family["frame_owner"] or spec["state_owner"] != family["state_owner"]
            or family["foreground_policy"] != "solid_native_industry_cell_full_original_sections_no_vehicle_aperture"):
        raise ValueError("collector native state/frame/solid-industry contract mismatch")
    approvals = spec["geometry_notes"]
    # Isolated input records must obey the same frozen original registrations
    # as disk-backed conversion; re-reading disk here would hide malformed input.
    if collector_view_signature(document) != approvals.get("approved_collector_ready_views_sha256"):
        raise ValueError("collector ready registration differs from approved views")
    if collector_construction_signature(document) != approvals.get("approved_collector_construction_views_sha256"):
        raise ValueError("collector construction registration differs from approved views")
    if collector_equipment_signature(document) != approvals.get("approved_collector_equipment_sha256"):
        raise ValueError("collector equipment registration differs from approved fittings")
    components = json.loads(tube.base.repository_path(family["indicator_record"]).read_text(encoding="utf-8"))
    component = next(row for row in components["components"] if row["key"] == family["indicator_key"])
    phases = facilities.indicator(component)
    keys = list(dict.fromkeys(key for views in family["construction_frame_sources"].values() for key in views.values()))
    models = {}
    # Cache only sixteen checked original hardware views. Sixty-four state/frame
    # poses reuse their exact models; no provider call or new machinery is needed.
    for key in keys:
        models[key] = collector_base(key, document=document, spec=spec)
    return family, spec, component, phases, models


def build_collector(document=None):
    """Compose complete collector source from original hardware and genuine glass.

    @param document Optional decoded source record for focused malformed-input checks.
    @return Complete1,152-pose provenance report and deterministic indexed atlas bytes.
    """
    family, spec, component, phases, models = collector_source_inputs(document)
    atlas = Image.new("P", (4608, 1024), 0)
    atlas.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    # A construction frame selects its actual paid quarter. Completed modes
    # reuse final hardware and change only authored glass phases, leaving source
    # capacity, stocks, completion and current shared generation untouched.
    for state_index, state in enumerate(facilities.STATES):
        for frame in range(4):
            for view, direction in enumerate(DIRECTIONS):
                stage = frame if state == "construction" else None
                key = family["construction_frame_sources"][str(frame)][direction] if stage is not None else family["whole_view_selections"][direction]
                original, source = models[key]
                phase = family["construction_indicator_phase"] if stage is not None else frame
                target = family["indicator_targets"][key]
                pose = insert_collector_glass(original, phases[state_index][phase], target)
                pieces, unused, unused_roles = partition_collector_model(pose, source)
                for tile, piece in enumerate(pieces):
                    # Every native industry cell is solid and has no capsule or
                    # portal mouth. Its aligned ground/back and sortable/front
                    # both retain the original section, as the existing native
                    # industry drawing permits. No invented cut-plane opens it.
                    for part_index, part in enumerate(("back", "front")):
                        x, y = (view * 18 + tile * 2 + part_index) * 64, (state_index * 4 + frame) * 64
                        atlas.paste(piece, (x, y))
                        sprites.append({"asset_id": family["asset_id"], "state": state, "direction": direction,
                                        "frame": frame, "tile_variant": tile, "world_cell": [tile % 3, tile // 3],
                                        "part": part, "anchor": [-31, -33], "box": [x, y, 64, 64],
                                        "source_key": key, "source": source["path"], "source_sha256": source["sha256"],
                                        "native_crop": source["native_crop"], "native_subject_size": source["native_subject_size"],
                                        "native_position": source["native_position"], "native_construction_stage": stage,
                                        "indicator": {"path": component["path"], "sha256": component["sha256"],
                                                      "state": state, "frame": phase, "target": target}})
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md54BC; archi-map-layers.md; complete original source, packing/runtime pending",
              "schema_version": 1, "asset_id": family["asset_id"], "source_record": RECORD.relative_to(tube.ROOT).as_posix(),
              "path": (tube.REVIEW / "collector-authored.png").relative_to(tube.ROOT).as_posix(), "size": [4608, 1024],
              "native_cell": [64, 64], "anchor": [-31, -33], "pose_count": len(sprites), "sprites": sprites,
              "source_coverage_complete": True, "runtime_activated": False, "foreground_policy": family["foreground_policy"],
              "frame_owner": family["frame_owner"], "state_owner": family["state_owner"],
              "approved_ready_views_sha256": spec["geometry_notes"]["approved_collector_ready_views_sha256"],
              "approved_construction_views_sha256": spec["geometry_notes"]["approved_collector_construction_views_sha256"],
              "approved_equipment_sha256": spec["geometry_notes"]["approved_collector_equipment_sha256"]}
    return report, tube.indexed_png(atlas)


def assemble_collector_pose(report, atlas, state, frame, direction, part="back"):
    """Reassemble actual encoded native cell sections through the world projection.

    @param report Complete source mapping with exact boxes and native world cells.
    @param atlas Decoded indexed source atlas.
    @param state Actual construction, ready, working or blocked presentation state.
    @param frame Native construction stage or completed equipment phase.
    @param direction Actual NE/SE/SW/NW whole-model view.
    @param part Aligned back or sortable front section to inspect.
    @return Indexed256x192 complete pose, retaining canonical palette and alpha.
    """
    pose = Image.new("P", (256, 192), 0)
    pose.putpalette(tube.base.PALETTE_BYTES)
    pose.info["transparency"] = 0
    # Non-overlapping original ownership makes the assembly order immaterial.
    # Native world offsets, not atlas columns, determine each physical section.
    for row in report["sprites"]:
        if (row["state"], row["frame"], row["direction"], row["part"]) != (state, frame, direction, part):
            continue
        x, y, width, height = row["box"]
        tile_x, tile_y = row["world_cell"]
        piece = atlas.crop((x, y, x + width, y + height))
        pose.paste(piece, (97 + 32 * (tile_y - tile_x), 31 + 16 * (tile_x + tile_y)), piece.convert("RGBA").getchannel("A"))
    return pose


def validate_collector(report, encoded, document=None):
    """Reject false coverage/provenance, altered original pixels and invalid source encoding.

    @param report Complete native collector source report.
    @param encoded Actual indexed atlas PNG bytes to validate.
    @param document Optional isolated source record for malformed-input checks.
    @return None; validation does not bind packed IDs or establish loaded runtime use.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == "facility.vent-collector")
    required = {(state, frame, direction, tile, part) for state in facilities.STATES for frame in range(4)
                for direction in DIRECTIONS for tile in range(9) for part in ("back", "front")}
    # A correct atlas may only publish its own native family and fixed canvas;
    # a changed top-level identity cannot redirect source binding to another asset.
    if (report["asset_id"] != family["asset_id"] or report["size"] != [4608, 1024]
            or report["native_cell"] != [64, 64] or report["anchor"] != [-31, -33]):
        raise ValueError("collector source family/native canvas identity mismatch")
    actual = [(row["state"], row["frame"], row["direction"], row["tile_variant"], row["part"]) for row in report["sprites"]]
    # Unique true identities, not a padded sprite count, prove the entire family.
    if len(actual) != len(required) or set(actual) != required or report["pose_count"] != len(required):
        raise ValueError("incomplete collector native state/stage/view/cell/part coverage")
    candidates = {row["key"]: row for row in document["candidates"]}
    # Cheap independent ancestry checks refuse malformed metadata before a full
    # deterministic reconstruction. Source and frame meaning stay explicit.
    for row in report["sprites"]:
        construction = row["state"] == "construction"
        key = family["construction_frame_sources"][str(row["frame"])][row["direction"]] if construction else family["whole_view_selections"][row["direction"]]
        source = candidates[key]
        phase = 0 if construction else row["frame"]
        stage = row["frame"] if construction else None
        # A correct bitmap cannot legitimize false stage ancestry, a moved lamp,
        # another original source or a different native tile/anchor.
        if (row["source_key"] != key or row["source"] != source["path"] or row["source_sha256"] != source["sha256"]
                or row["native_construction_stage"] != stage or row["indicator"]["state"] != row["state"]
                or row["indicator"]["frame"] != phase or row["indicator"]["target"] != family["indicator_targets"][key]
                or row["world_cell"] != [row["tile_variant"] % 3, row["tile_variant"] // 3] or row["anchor"] != [-31, -33]):
            raise ValueError("collector false original/frame/equipment provenance")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Native canonical palette and transparent zero preserve actual alpha;
        # animated or company-remap indices cannot alter the authored machinery.
        if (atlas.mode != "P" or atlas.size != (4608, 1024) or atlas.info.get("transparency") != 0
                or atlas.getpalette()[:768] != tube.base.PALETTE_BYTES
                or set(atlas.get_flattened_data()) & (tube.base.FORBIDDEN - {0})):
            raise ValueError("collector native palette/canvas mismatch")
    expected, pixels = build_collector(document)
    # Rebuild from immutable originals and frozen registrations. Matching only
    # declared hashes would accept a changed floor or steel pixel in the payload.
    if report["sprites"] != expected["sprites"] or encoded != pixels:
        raise ValueError("collector source changes original model/component pixels")
    for field in ("schema_version", "asset_id", "source_record", "path", "size", "native_cell", "anchor",
                  "frame_owner", "state_owner", "foreground_policy", "approved_ready_views_sha256",
                  "approved_construction_views_sha256", "approved_equipment_sha256", "source_coverage_complete", "runtime_activated"):
        if report[field] != expected[field]:
            raise ValueError("collector source changes native ownership or approval")


def collector_contact_pages(report, encoded):
    """Show every native stage/state/phase while preserving original per-cell assembly.

    @param report Complete source report with original model and component ancestry.
    @param encoded Actual indexed collector atlas PNG bytes.
    @return Direction-indexed deterministic PNG review pages.
    """
    result = {}
    with Image.open(io.BytesIO(encoded)) as atlas:
        # One page per genuine access-side view keeps all paid stages and real
        # equipment phases visible together; no template replaces actual source.
        for direction in DIRECTIONS:
            page = Image.new("RGB", (2080, 1680), (235, 239, 243))
            draw = ImageDraw.Draw(page)
            for state_index, state in enumerate(facilities.STATES):
                for frame in range(4):
                    pose = assemble_collector_pose(report, atlas, state, frame, direction)
                    rgba = pose.convert("RGBA")
                    visible = Image.new("RGB", pose.size, (235, 239, 243))
                    visible.paste(rgba, mask=rgba.getchannel("A"))
                    x, y = frame * 520, state_index * 420
                    label = f"{direction} {state}: " + (f"native stage{frame}, unfinished" if state == "construction" else f"original glass phase{frame}")
                    draw.text((x + 8, y + 8), label, fill=(30, 40, 50))
                    draw.text((x + 8, y + 24), "Nine actual native cells; complete source only; packing/runtime pending", fill=(30, 40, 50))
                    page.paste(visible.resize((512, 384), Image.Resampling.NEAREST), (x + 4, y + 36))
            target = io.BytesIO()
            page.save(target, format="PNG")
            result[direction] = target.getvalue()
    return result


def publish_collector(check=False):
    """Generate complete deterministic source proof or compare it read-only.

    @param check True verifies the actual atlas, all review pages and complete mapping.
    @return Complete collector source report path; packed payload and runtime stay unchanged.
    """
    report, encoded = build_collector()
    validate_collector(report, encoded)
    report.update(sha256=hashlib.sha256(encoded).hexdigest(), contact_pages=[])
    files = {tube.base.repository_path(report["path"]): encoded}
    # Every actual view page belongs to the proof. Missing/stale pages cannot
    # inherit acceptance from a different view or an incomplete model contact.
    for direction, pixels in collector_contact_pages(report, encoded).items():
        path = tube.REVIEW / ("collector-" + direction.lower() + "-state-contact.png")
        files[path] = pixels
        report["contact_pages"].append({"direction": direction, "path": path.relative_to(tube.ROOT).as_posix(),
                                        "sha256": hashlib.sha256(pixels).hexdigest()})
    serialized = json.dumps(report, indent=2) + "\n"
    if check:
        if any(path.read_bytes() != pixels for path, pixels in files.items()) or COLLECTOR_REPORT.read_text(encoding="utf-8") != serialized:
            # Packing may consume this family only while its original source,
            # exact pose mapping and all reviewed contacts regenerate together.
            raise ValueError("collector source proof needs deterministic regeneration")
    else:
        for path, pixels in files.items():
            path.write_bytes(pixels)
        COLLECTOR_REPORT.write_text(serialized, encoding="utf-8")
    return COLLECTOR_REPORT


def bind_collector_source(report_path):
    """Publish the complete reviewed collector source without activating its runtime.

    @param report_path Complete report just generated/verified by publish_collector.
    @return None; only this family's source/authoring metadata changes.
    """
    report = json.loads(report_path.read_text(encoding="utf-8"))
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == report["asset_id"])
    first = report["sprites"][0]
    family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                        "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
    originals = list(dict.fromkeys(row["source"] for row in report["sprites"]))
    originals.extend(path for path in dict.fromkeys(row["indicator"]["path"] for row in report["sprites"]) if path not in originals)
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_collector_state_phase_source_runtime_pending",
                           "source_record": report["source_record"], "originals": originals,
                           "atlas_manifest": report_path.relative_to(tube.ROOT).as_posix(), "native_atlas": report["path"],
                           "native_sha256": report["sha256"], "staged_pose_count": report["pose_count"],
                           "coverage_complete": True, "runtime_activated": False}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def original_collector_cells(source):
    """Extract complete original modules with independent cell ancestry.

    @param source Checked immutable ready-module sheet, complete or explicit corrections.
    @return World-cell mapping to indexed native pose and exact original provenance.
    @throws ValueError If a declared crop crosses or truncates its actual original cell.
    """
    cells, boxes = tube.base.source_cells(source)
    order = source["cell_order"]
    # Correction sheets contain only their named actual cells. Ambiguous,
    # repeated or out-of-floor cells cannot override an unrelated native tile.
    if (len(order) != len(boxes) or len(set(map(tuple, order))) != len(order)
            or [row["world_cell"] for row in source["source_cells"]] != order
            or any(point not in [[x, y] for y in range(3) for x in range(3)] for point in order)):
        raise ValueError("collector original has invalid or duplicate world cells")
    result = {}
    # Opacity bands independently retain every whole authored module. Exact
    # original crops prevent a plausible metadata edit from stealing a neighbour.
    for index, registration in enumerate(source["source_cells"]):
        cell = cells[index // source["grid"][0]][index % source["grid"][0]]
        left, top, right, bottom = cell.getchannel("A").point(lambda value: 255 if value >= 128 else 0).getbbox()
        bx, by = boxes[index][:2]
        if registration["crop"] != [bx + left, by + top, bx + right, by + bottom]:
            raise ValueError("collector module crop changes actual original cell")
        pose = tube.base.quantize(native_housing(cell.crop((left, top, right, bottom))), {})
        provenance = {"world_cell": registration["world_cell"], "source_key": source["key"],
                      "source": source["path"], "source_sha256": source["sha256"], "source_crop": registration["crop"]}
        result[tuple(registration["world_cell"])] = (pose, provenance)
    return result


def collector_cells(key):
    """Extract genuine one-tile original modules in actual native world order.

    @param key Exact original nine-module candidate with frozen source-cell crops.
    @return Indexed native cell list, immutable source and actual model role records.
    @throws ValueError If source cells, native geometry or whole original crops disagree.
    """
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    source = tube.retained_candidate(key, document)
    family = next(row for row in document["families"] if row["asset_id"] == source["asset_id"])
    # Original cell art must belong to this actual collector and a real view;
    # relabelled whole-campus art cannot stand in for nine independent cells.
    if (source["asset_id"] != "facility.vent-collector" or source.get("grid") != [3, 3]
            or source.get("direction") not in family["directions"] or source.get("model_state") != "ready"):
        raise ValueError("collector cell source has wrong model or native view")
    collector_base(family["base_source"])
    expected = [[x, y] for y in range(3) for x in range(3)]
    # Every canonical TIDS position occurs once, in the original's declared
    # row-major order. Missing/duplicated cells cannot be hidden by assembly.
    if source.get("cell_order") != expected or [row["world_cell"] for row in source.get("source_cells", [])] != expected:
        raise ValueError("collector original needs all nine actual native cells")
    resolved = original_collector_cells(source)
    replaced = set()
    # A targeted original correction supplies only its named fitting cells.
    # Other modules retain the exact earlier original; no image is rotated or
    # silently borrowed from a different physical view or construction state.
    for correction in source.get("replacements", []):
        original = tube.retained_candidate(correction["source"], document)
        if (original["asset_id"] != source["asset_id"] or original["direction"] != source["direction"]
                or original["model_state"] != source["model_state"]):
            raise ValueError("collector correction changes actual model view/state")
        additions = original_collector_cells(original)
        declared = set(map(tuple, correction["world_cells"]))
        if declared != set(additions) or declared & replaced:
            raise ValueError("collector correction changes unnamed or repeated cells")
        replaced.update(declared)
        resolved.update(additions)
    if not set(map(tuple, source.get("excluded_cells", []))) <= replaced:
        raise ValueError("known excluded collector fitting needs its genuine correction")
    result = [resolved[tuple(point)][0] for point in expected]
    source = dict(source)
    source["resolved_cell_sources"] = [resolved[tuple(point)][1] for point in expected]
    return result, source, family["cell_roles"][source["direction"]]


def review_collector_cells(key, whole=False):
    """Review true cell artwork and its assembly at native world coordinates.

    @param key Exact original whole model or nine-module draft to sample and compose.
    @param whole True slices one complete original model into lossless native cells.
    @return Partial-source review receipt; other views/states and runtime stay pending.
    """
    cells, source, roles = partition_collector(key) if whole else collector_cells(key)
    sheet = Image.new("P", (9 * 64, 64), 0)
    sheet.putpalette(tube.base.PALETTE_BYTES)
    campus = Image.new("RGBA", (256, 192))
    # Native depth follows increasing world X+Y. Logical cell placement uses
    # RemapCoords(Y-X,X+Y), preserving actual industry IDs and shared pipe edges.
    for index in sorted(range(9), key=lambda i: (sum(source["cell_order"][i]), source["cell_order"][i][0])):
        x, y = source["cell_order"][index]
        sheet.paste(cells[index], (index * 64, 0))
        ox, oy = source.get("native_cell_origin", [97, 32])
        campus.alpha_composite(cells[index].convert("RGBA"), (ox + 32 * (y - x), oy + 16 * (x + y)))
    native = Image.new("RGB", campus.size, (235, 239, 243))
    native.paste(campus, mask=campus.getchannel("A"))
    page = Image.new("RGB", (1240, 850), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    model_state = source.get("model_state", "ready")
    stage = source.get("construction_stage")
    # Construction contacts identify the actual native quarter. A generic
    # ready label would misrepresent unfinished machinery as a completed source.
    label = f"CONSTRUCTION STAGE {stage}" if model_state == "construction" else "READY CELLS ONLY"
    draw.text((12, 12), f"COLLECTOR {source['direction']} {label}: original nine-cell assembly; other source states/runtime pending", fill=(30, 40, 50))
    page.paste(native.resize((768, 576), Image.Resampling.NEAREST), (12, 40))
    page.paste(native, (12, 634))
    # Isolated cells retain original material and show intended pipe sides;
    # labels belong only to this review, never to native packed source pixels.
    for index, (cell, role) in enumerate(zip(cells, roles)):
        rgba = cell.convert("RGBA")
        visible = Image.new("RGB", cell.size, (235, 239, 243))
        visible.paste(rgba, mask=rgba.getchannel("A"))
        left, top = 800 + (index % 3) * 144, 56 + (index // 3) * 224
        page.paste(visible.resize((128, 128), Image.Resampling.NEAREST), (left, top))
        draw.text((left, top + 136), str(role["world_cell"]), fill=(30, 40, 50))
        draw.text((left, top + 154), "/".join(role["pipe_sides"]), fill=(30, 40, 50))
    prefix = key + ("-cells" if whole else "")
    paths = [tube.REVIEW / (prefix + suffix) for suffix in ("-native.png", "-assembled.png", "-contact.png")]
    paths[0].write_bytes(tube.indexed_png(sheet))
    campus.save(paths[1])
    page.save(paths[2])
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md54B.2; archi-map-layers.md; partial source review only",
              "source": source["path"], "source_sha256": source["sha256"], "source_cells": source["resolved_cell_sources"],
              "direction": source["direction"], "model_state": model_state, "construction_stage": stage,
              "native_size": [64, 64], "anchor": [-31, -33],
              "native_cells": 9, "source_coverage_complete": False, "runtime_activated": False,
              "partition_policy": source.get("partition_policy"), "original_native_sha256": source.get("original_native_sha256"),
              "artifacts": [{"path": path.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                            for path in paths]}
    receipt = tube.REVIEW / (prefix + "-review.json")
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return receipt


def collector_base(key, document=None, spec=None):
    """Sample a retained collector concept on its complete canonical native floor.

    @param key Exact original collector candidate with reviewed source registration.
    @param document Optional decoded original/selection record for isolated source validation.
    @param spec Optional already decoded public collector contract from the same operation.
    @return Indexed full-campus preview and checked original metadata.
    @throws ValueError If the native contract or original registration disagrees.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    source = tube.retained_candidate(key, document)
    if source.get("direction") in source.get("excluded_views", []):
        raise ValueError("known excluded collector original view")
    if spec is None:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
        spec = next(row for row in public["assets"] if row["asset_id"] == source["asset_id"])
    family = spec
    # Artwork cannot reinterpret the actual nine-cell TIDS industry as one
    # tile, change its anchor or acquire another producer's native footprint.
    if (source["asset_id"] != "facility.vent-collector" or family["consumer"] != 18
            or family["footprint"] != [3, 3] or family["tile_variants"] != 9
            or family["native_size"] != [64, 64] or family["anchor"] != [-31, -33]
            or family["geometry"] != "per_tile"):
        raise ValueError("collector source needs actual nine-cell native geometry")
    expected = {"footprint": [3, 3], "tile_cells": [[x, y] for y in range(3) for x in range(3)],
                "cell_size": [64, 64], "anchor": [-31, -33]}
    if source.get("native_geometry") != expected:
        raise ValueError("collector base changes canonical native cells")
    floor = [[128, 64], [224, 112], [128, 160], [32, 112]]
    if source.get("native_floor_corners") != floor:
        raise ValueError("collector floor leaves fixed native projection")
    approved = family.get("geometry_notes", {}).get("approved_collector_ready_views_sha256")
    # Later state composition must preserve the selected ready models, their
    # complete original crops and integer registration. Revalidating a source
    # file hash alone cannot authorize a different sampled physical model.
    if approved is not None and collector_view_signature(document) != approved:
        raise ValueError("collector native registration differs from approved views")
    construction_approval = family.get("geometry_notes", {}).get("approved_collector_construction_views_sha256")
    # Once reviewed, construction models have the same immutable registration
    # guarantee as ready hardware. A valid raw hash alone cannot move a stage.
    if construction_approval is not None and collector_construction_signature(document) != construction_approval:
        raise ValueError("collector native construction registration differs from approved views")
    with Image.open(tube.base.repository_path(source["path"])) as original:
        left, top, right, bottom = source["native_crop"]
        # A complete measured source crop must stay inside its immutable
        # original; transparent padding cannot hide a missing facility edge.
        if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
            raise ValueError("collector crop leaves original")
        crop = original.convert("RGBA").crop((left, top, right, bottom))
    width, height = source["native_subject_size"]
    x, y = source["native_position"]
    # The explicit native scale fits a three-tile floor and leaves headroom
    # for its low machinery without growing the eventual64x64 cell canvases.
    if not (width == 192 and 96 <= height <= 128 and x in (32, 33) and 32 <= y <= 64 and y + height <= 192):
        raise ValueError("collector base leaves native floor envelope")
    canvas = Image.new("RGBA", (256, 192))
    canvas.paste(crop.resize((width, height), Image.Resampling.BOX), (x, y))
    return tube.base.quantize(canvas, {}), source


def review_collector(key):
    """Publish native and enlarged model references with explicit nine-cell outlines.

    @param key Exact retained original selected for candidate-scale review.
    @return Review receipt path; neither full source nor runtime activation is granted.
    """
    native, source = collector_base(key)
    target = tube.REVIEW / (key + "-native.png")
    contact = tube.REVIEW / (key + "-contact.png")
    receipt = tube.REVIEW / (key + "-review.json")
    tube.REVIEW.mkdir(parents=True, exist_ok=True)
    target.write_bytes(tube.indexed_png(native))
    visible = Image.new("RGB", native.size, (235, 239, 243))
    visible.paste(native.convert("RGBA"), mask=native.convert("RGBA").getchannel("A"))
    outline = visible.copy()
    pen = ImageDraw.Draw(outline)
    # Every actual world cell is shown independently at native32/16 pitch.
    # This review detects a squeezed single-tile campus before cell authoring.
    for y in range(3):
        for x in range(3):
            cx, cy = 128 + 32 * (y - x), 80 + 16 * (x + y)
            pen.line([(cx, cy - 16), (cx + 32, cy), (cx, cy + 16), (cx - 32, cy), (cx, cy - 16)], fill=(30, 160, 185))
    page = Image.new("RGB", (1080, 850), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    draw.text((12, 12), "COLLECTOR BASE ONLY: actual3x3/nine cells; complete per-cell/state source and runtime pending", fill=(30, 40, 50))
    page.paste(visible.resize((768, 576), Image.Resampling.NEAREST), (12, 42))
    page.paste(outline, (804, 64))
    page.paste(visible, (804, 300))
    page.save(contact)
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md Group54B; archi-map-layers.md; model review only",
              "source": source["path"], "source_sha256": source["sha256"], "source_record": RECORD.relative_to(tube.ROOT).as_posix(),
              "native_geometry": source["native_geometry"], "native_floor_corners": source["native_floor_corners"],
              "native_crop": source["native_crop"], "native_subject_size": source["native_subject_size"],
              "native_position": source["native_position"], "source_coverage_complete": False, "runtime_activated": False,
              "artifacts": [{"path": path.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                            for path in (target, contact)]}
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return receipt


def main():
    """Run retained model/cell review without publishing incomplete source families.

    @return None; print the review path or propagate the exact contract refusal.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", help="Retained collector model original to review")
    parser.add_argument("--cells", action="store_true", help="Review the original nine independent modules")
    parser.add_argument("--whole", action="store_true", help="Review lossless native cells from one whole original model")
    parser.add_argument("--ready-views", action="store_true", help="Compare all four approved ready originals")
    parser.add_argument("--construction-views", action="store_true", help="Compare all sixteen native construction hardware views")
    parser.add_argument("--source", action="store_true", help="Generate complete original collector state/phase source")
    parser.add_argument("--check", action="store_true", help="Verify complete collector source proof read-only")
    parser.add_argument("--publish-source", action="store_true", help="Bind verified complete collector source metadata")
    arguments = parser.parse_args()
    # Complete-family commands consume only approved model/equipment registries.
    # Candidate or partial-review switches cannot narrow the published coverage.
    if arguments.source or arguments.check or arguments.publish_source:
        if arguments.candidate or arguments.cells or arguments.whole or arguments.ready_views or arguments.construction_views:
            parser.error("complete collector source consumes only approved registries")
        report_path = publish_collector(check=arguments.check)
        if arguments.publish_source:
            bind_collector_source(report_path)
        print(report_path)
        return
    if arguments.ready_views or arguments.construction_views:
        # Comparison consumes exactly one approved view registry. Mixing both
        # registries or candidate/draft switches would blur its coverage claim.
        if arguments.candidate or arguments.cells or arguments.whole or (arguments.ready_views and arguments.construction_views):
            parser.error("view comparison consumes exactly one approved registry")
        print(review_ready_views() if arguments.ready_views else review_construction_views())
        return
    key = arguments.candidate or ("collector-ready-se-v1" if arguments.cells else "collector-base-se-v1")
    # Whole-model conversion and independent draft review have distinct input
    # provenance; neither switch can silently relabel the other source mode.
    if arguments.whole and arguments.cells:
        parser.error("choose whole-model partition or independent draft cells")
    print(review_collector_cells(key, arguments.whole) if arguments.cells or arguments.whole else review_collector(key))


if __name__ == "__main__":
    main()
