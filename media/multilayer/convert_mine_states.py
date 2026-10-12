"""Complete Coal source coverage through genuine native construction and equipment glass.

@file convert_mine_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group27D/task27.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The existing mine/geology/Industry owners select presentation; source-only art
cannot change reserves, output, work, time, paid floor or packed sprite ranges.
"""

import argparse
import copy
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_mine_construction as construction


mine = construction.mine
OUTPUT = mine.OUTPUT
PREFIX = "coal-mine-source"
STATES = ("construction", "working", "blocked", "depleted")


def original_lenses(record):
    """Resolve original working/paused/off glass at the existing native control fitting.

    @param record Mine originals with frozen equipment component geometry/ancestry.
    @return State-indexed four-phase lists of genuinely authored opaque3x3 RGBA glass.
    @throws ValueError On changed fitting, relabelled state, unsafe crop or false off glass.
    """
    family = record["families"]["mine.COAL"]
    geometry = family["operating_component_geometry"]
    source, component = mine.component_record(record, geometry["depleted_selection"])
    # The real hut fitting alone communicates operation. Foreign components,
    # moved fittings or relabelled ready lamps cannot stand in for depletion.
    if (mine.signature({"geometry": geometry, "off_component": component}) != family["approved_operating_component_geometry_sha256"]
            or geometry["hut_variant"] != 7 or geometry["world_cell"] != [1, 2]
            or geometry["cue_target"] != [30, 43, 3, 3]
            or geometry["m1_component"] != "facility-status"
            or geometry["working_state"] != "working" or geometry["working_frames"] != [0, 1, 2, 3]
            or geometry["blocked_state"] != "blocked" or geometry["blocked_frame"] != 0
            or geometry["depleted_state"] != "depleted" or geometry["depleted_frame"] != 0
            or component["state"] != "depleted" or component["frame"] != 0
            or component["variant"] != 7 or component["world_cell"] != [1, 2]
            or component["native_size"] != [3, 3] or component["resampling"] != "BOX"
            or component["alpha_crop_threshold"] != 16
            or source["selection"]["status"] != "approved_native_components"):
        raise ValueError("mine operating component changes native fitting or original state ancestry")
    mine.facilities.verify_original(source)
    donor = json.loads(mine.facilities.RECORD.read_text(encoding="utf-8"))
    status = next(row for row in donor["components"] if row["key"] == geometry["m1_component"])
    if status["sha256"] != geometry["m1_component_sha256"]:
        raise ValueError("mine operating original M1 glass changed")
    phases = mine.facilities.indicator(status)
    with Image.open(mine.base.repository_path(source["path"])) as raw:
        image = raw.convert("RGBA")
    window, glass = component["source_window"], component["glass_window"]
    # Whole original clearance and the ordinary inner-glass rectangle must
    # validate before native sampling; alpha erasure cannot manufacture glass.
    if (list(image.size) != source["image_size"] or len(window) != 4
            or not 0 <= window[0] < window[2] <= image.width
            or not 0 <= window[1] < window[3] <= image.height
            or not window[0] <= glass[0] < glass[2] <= window[2]
            or not window[1] <= glass[1] < glass[3] <= window[3]):
        raise ValueError("mine depleted glass crop leaves its retained original")
    crop = image.crop(window)
    bounds = crop.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
    if bounds is None or list(bounds) != component["measured_alpha16_bbox"]:
        raise ValueError("mine depleted original bounds changed")
    if min(bounds[0], bounds[1], crop.width - bounds[2], crop.height - bounds[3]) < 16:
        raise ValueError("mine depleted original lacks transparent clearance")
    if any(alpha >= 16 and red > 220 and green < 60 and blue > 220 for red, green, blue, alpha in crop.get_flattened_data()):
        raise ValueError("mine depleted original includes separator magenta")
    off = image.crop(glass).resize((3, 3), Image.Resampling.BOX)
    # Depleted is genuinely unlit neutral glass. A green/cyan/red original
    # relabelled as off would confuse depletion with availability or refusal.
    if off.getchannel("A").getextrema()[0] < 128 or any(max(pixel[:3]) - min(pixel[:3]) > 12 for pixel in off.get_flattened_data()):
        raise ValueError("mine depleted original is not opaque neutral off glass")
    return {"working": phases[2], "blocked": [phases[3][0]] * 4, "depleted": [off] * 4}


def apply_lens(original, lens, target):
    """Insert original glass while preserving every physical apparatus/cargo/floor pixel.

    @param original Indexed approved completed control cell with native alpha/palette.
    @param lens Genuine original opaque3x3 equipment glass.
    @param target Existing native control-hut fitting, never an expanded state mask.
    @return Indexed native cell; all pixels outside the actual fitting remain exact.
    """
    if target != [30, 43, 3, 3]:
        raise ValueError("mine operating glass leaves its real native fitting")
    changed = mine.base.quantize(mine.facilities.insert_indicator(original.convert("RGBA"), lens, target), {})
    # Even a valid equipment insert cannot alter ground/stock or erase output
    # on depletion. Native reserve/output quantities remain their sole owners.
    for y in range(64):
        for x in range(64):
            old, new = original.getpixel((x, y)), changed.getpixel((x, y))
            if bool(old) != bool(new):
                raise ValueError("mine operating glass changes original alpha")
            if not (30 <= x < 33 and 43 <= y < 46):
                if new != old:
                    raise ValueError("mine operating glass changes apparatus/cargo outside fitting")
    mine.validate_cell(changed, 7)
    return changed


def review_board(poses):
    """Show all16 source state/phase campuses at native registration and readable2x scale.

    @param poses State/phase mappings of nine independently owned original back/front pairs.
    @return RGB comparison with actual native owners disclosed; no game acceptance claim.
    """
    board = Image.new("RGB", (2080, 1464), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((12, 8), "SOURCE ONLY: fixed3x3 Coal mine;16 actual state/phases; separate loading floor untouched; packed ranges and game display pending", fill="black")
    # Displaying every state/phase exposes the actual construction progression
    # and static pause/depletion aliases without hiding them in one whole pose.
    for index, state in enumerate(STATES):
        for frame in range(4):
            x, y = frame * 520 + 4, index * 356 + 40
            draw.text((x + 8, y), f"{state} frame{frame}" + (" (static0 alias)" if state in ("blocked", "depleted") else ""), fill="black")
            scene = mine.campus(poses[(state, frame)]).convert("RGBA").resize((512, 320), Image.Resampling.NEAREST)
            board.paste(scene, (x, y + 24), scene)
    return board


def build(record, public):
    """Build complete288-slot source coverage from verified construction and original equipment.

    @param record Retained base/construction/state originals and frozen native registrations.
    @param public Existing shared mine.COAL geometry/state contract, with no native activation.
    @return Complete source report and deterministic PNG/GIF/manifest bytes before publication.
    """
    _construction_report, construction_files = construction.build(record, public)
    lenses = original_lenses(record)
    target = record["families"]["mine.COAL"]["operating_component_geometry"]["cue_target"]
    with Image.open(io.BytesIO(construction_files[OUTPUT / "coal-mine-construction-atlas.png"])) as original:
        construction_atlas = original.copy()
    atlas = Image.new("P", (1152, 1024), 0)
    atlas.putpalette(mine.base.PALETTE_BYTES)
    poses, rows = {}, []
    # Source phases follow the actual native policy: construction0..3 and
    # synchronized working presentation; paused/depleted remain static aliases.
    # No source state changes physical stock or invents a production clock.
    for state_index, state in enumerate(STATES):
        for frame in range(4):
            poses[(state, frame)] = {}
            for variant in range(9):
                x, source_y = variant * 128, (frame if state == "construction" else 3) * 64
                whole = construction_atlas.crop((x, source_y, x + 64, source_y + 64))
                # Only completed operating states use the genuine control-hut
                # fitting; construction and every other machinery cell stay exact.
                if state != "construction" and variant == 7:
                    whole = apply_lens(whole, lenses[state][frame], target)
                pair = mine.source_parts(whole)
                poses[(state, frame)][variant] = pair
                for index, (part, sprite) in enumerate(zip(mine.PARTS, pair)):
                    if sprite.getbbox() is None:
                        raise ValueError("mine full source lacks an actual native owner part")
                    px, py = x + index * 64, (state_index * 4 + frame) * 64
                    atlas.paste(sprite, (px, py))
                    rows.append({"asset_id": "mine.COAL", "state": state, "frame": frame, "direction": "none", "native_layout": 0,
                                 "tile_variant": variant, "world_cell": [variant % 3, variant // 3], "part": part,
                                 "box": [px, py, 64, 64], "anchor": [-31, -33], "structurally_absent": False,
                                 "nonempty_pixels": sum(value != 0 for value in sprite.get_flattened_data()),
                                 "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest()})
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md Group27D/task27.3; archi-map-layers.md; source only")
    buffer = io.BytesIO()
    atlas.save(buffer, format="PNG", transparency=0, pnginfo=metadata, optimize=False, compress_level=9)
    atlas_path = OUTPUT / f"{PREFIX}-authored.png"
    files = {atlas_path: buffer.getvalue()}
    buffer = io.BytesIO()
    review_board(poses).save(buffer, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files[OUTPUT / f"{PREFIX}-review.png"] = buffer.getvalue()
    frames = [mine.campus(poses[("working", frame)]).resize((768, 480), Image.Resampling.NEAREST) for frame in range(4)]
    buffer = io.BytesIO()
    frames[0].save(buffer, format="GIF", save_all=True, append_images=frames[1:], duration=500, loop=0,
                   transparency=0, disposal=2, optimize=False, comment=b"r2mlvs14 Group27D; archi-map-layers.md; source-only four-phase comparison")
    files[OUTPUT / f"{PREFIX}-working.gif"] = buffer.getvalue()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md Group27D/task27.3; archi-map-layers.md",
              "asset_id": "mine.COAL", "source_coverage_complete": True, "runtime_activated": False,
              "source": atlas_path.relative_to(mine.ROOT).as_posix(), "source_sha256": hashlib.sha256(files[atlas_path]).hexdigest(),
              "directions": ["none"], "states": list(STATES), "frames": 4, "native_layout": 0, "native_size": [64, 64], "anchor": [-31, -33],
              "footprint": [3, 3], "tile_variants": 9, "source_slots": 288, "nonempty_source_parts": 288, "structurally_absent_parts": 0,
              "source_parts": rows, "native_state_policy": public["native_state_policy"],
              "operating_component_geometry": record["families"]["mine.COAL"]["operating_component_geometry"],
              "stock_policy": "All apparatus/floor/Coal pixels outside the actual control fitting remain exact in working/paused/depleted; final output is never erased",
              "remaining": "Master10.4D selected packing/native binding/decoded pixels/ordinary game proof"}
    report["artifacts"] = [{"path": path.relative_to(mine.ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items()]
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def validate_public_source(public, report):
    """Bind complete public source claims to the exact native atlas and retained original.

    @param public Existing shared Coal row with authored source metadata.
    @param report Fresh complete source report from all actual original stage/cell/state owners.
    @return None; a packed range or runtime claim is refused at this source-only boundary.
    """
    mine.validate_mine_contract(public)
    authoring = public.get("authoring", {})
    # Counts alone cannot authorize delivery. The exact atlas, original and
    # current native geometry/state policy identify the complete source handoff.
    if (report["asset_id"] != "mine.COAL" or report["source_slots"] != 288
            or report["nonempty_source_parts"] != 288 or report["structurally_absent_parts"] != 0
            or report["source"] != (OUTPUT / f"{PREFIX}-authored.png").relative_to(mine.ROOT).as_posix()
            or public["source"]["status"] != "authored" or not authoring.get("coverage_complete")
            or authoring.get("native_atlas") != report["source"] or authoring.get("native_sha256") != report["source_sha256"]
            or authoring.get("atlas_manifest") != (OUTPUT / f"{PREFIX}-manifest.json").relative_to(mine.ROOT).as_posix()
            or authoring.get("staged_pose_count") != 288 or authoring.get("nonempty_source_parts") != 288
            or authoring.get("structurally_absent_parts") != 0 or authoring.get("runtime_activated") is not False
            or public["packed"]["offset"] is not None or public["packed"]["sha256"] is not None):
        raise ValueError("mine complete public source or exact atlas binding changed")
    if mine.base.digest(mine.base.repository_path(public["source"]["path"])) != public["source"].get("sha256"):
        raise ValueError("mine public retained original identity changed")
    if mine.base.digest(mine.base.repository_path(report["source"])) != report["source_sha256"]:
        raise ValueError("mine public native source atlas pixels changed")


def publish_source(record, document, report):
    """Publish only Coal source metadata through the existing shared family owner.

    @param record Retained original selections; only family delivery metadata changes.
    @param document Existing shared manifest; only mine.COAL source metadata changes.
    @param report Complete validated288-slot source report, never native runtime proof.
    @return None; no packed sprite range or native rendering activation occurs.
    """
    updated, public_document = copy.deepcopy(record), copy.deepcopy(document)
    family = updated["families"]["mine.COAL"]
    row = next(row for row in public_document["assets"] if row["asset_id"] == "mine.COAL")
    base_source, _component = mine.component_record(updated, family["base_cells"][0])
    selected = {entry["source"] for entry in family["base_cells"]}
    # The full handoff includes every selected construction original, rather
    # than publishing a base filename that conceals missing native phases.
    for selections in family["construction_cells"].values():
        selected.update(entry["source"] for entry in selections)
    selected.add(family["operating_component_geometry"]["depleted_selection"]["source"])
    originals = [source["path"] for source in updated["candidates"] if source["id"] in selected]
    originals.extend([family["control_reuse"]["source_path"], family["construction_reuse"]["source_path"],
                      "media/multilayer/source/m1/raw/facility-status-phases-v1.png"])
    row["source"] = {"status": "authored", "path": base_source["path"], "sha256": base_source["sha256"],
                     "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
    row["authoring"].update(date="2026-10-12", status="complete_native_mine_source_runtime_pending", originals=sorted(set(originals)),
                            atlas_manifest=(OUTPUT / f"{PREFIX}-manifest.json").relative_to(mine.ROOT).as_posix(),
                            native_atlas=report["source"], native_sha256=report["source_sha256"], staged_pose_count=288,
                            nonempty_source_parts=288, structurally_absent_parts=0, coverage_complete=True,
                            runtime_activated=False, remaining=report["remaining"])
    family.update(status="complete_native_mine_source_runtime_pending", source_coverage_complete=True,
                  atlas_manifest=row["authoring"]["atlas_manifest"], native_atlas=report["source"], native_sha256=report["source_sha256"],
                  remaining=report["remaining"])
    validate_public_source(row, report)
    mine.RECORD.write_text(json.dumps(updated, indent=2) + "\n", encoding="utf-8")
    mine.PUBLIC.write_text(json.dumps(public_document, indent=2) + "\n", encoding="utf-8")


def main():
    """Publish complete source originals without changing actual native packed routes.

    @return None; original/refusal validation precedes generated artifacts and source metadata.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    operation = parser.add_mutually_exclusive_group(required=True)
    operation.add_argument("--review", action="store_true")
    operation.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    record = json.loads(mine.RECORD.read_text(encoding="utf-8"))
    document = json.loads(mine.PUBLIC.read_text(encoding="utf-8"))
    public = next(row for row in document["assets"] if row["asset_id"] == "mine.COAL")
    report, files = build(record, public)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for path, payload in files.items():
        path.write_bytes(payload)
    if args.publish:
        publish_source(record, document, report)
    print(json.dumps({key: report[key] for key in ("source_slots", "nonempty_source_parts", "source_coverage_complete", "runtime_activated")}))


if __name__ == "__main__":
    main()
