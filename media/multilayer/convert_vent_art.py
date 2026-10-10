"""Convert genuine natural vent information/detail without changing geology or terrain.

@file convert_vent_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group54C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Natural source membership/rate, paid excavation and active survey keep their native owners.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube


RECORD = tube.ROOT / "media/multilayer/source/ground/geothermal-generation-record.json"
TARGET = tube.REVIEW / "vent-source-manifest.json"
STATES = ("hidden", "surveyed", "excavated")


def select_vent_source_state(in_field, paid_excavated, survey_visible):
    """Select a source-preview state from the existing owners' visibility facts.

    @param in_field Actual Deep natural-field membership, already qualified by the native profile.
    @param paid_excavated Canonical terrain is Excavated; partial work does not qualify.
    @param survey_visible Actual same-layer/date-valid active public survey permits information.
    @return Source state, or None outside a real field; this does not bind runtime rendering.
    """
    if not in_field:
        return None
    if paid_excavated:
        return "excavated"
    if survey_visible:
        return "surveyed"
    return "hidden"


def source_inputs(document=None):
    """Resolve one immutable natural overlay and the exact public source geometry.

    @param document Optional decoded retained source record for isolated validation.
    @return Source family, checked original candidate and public native contract.
    @throws ValueError If source ownership, visible states or native geometry disagree.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == "geothermal.vent")
    source = tube.retained_candidate(family["selected_source"], document)
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    spec = next(row for row in public["assets"] if row["asset_id"] == family["asset_id"])
    geometry = {"native_size": [64, 31], "anchor": [-31, 0], "footprint": [1, 1], "directions": ["none"], "parts": ["body"]}
    # One per-tile overlay describes part of the saved multi-tile field. It may
    # neither replace hardness/minerals nor acquire a thermal-consumer state.
    if (source["asset_id"] != family["asset_id"] or source["native_geometry"] != geometry
            or any(spec[name] != value or family[name] != value for name, value in geometry.items())
            or spec["states"] != list(STATES) or family["states"] != list(STATES)
            or spec["frames"] != 4 or family["frames"] != 4 or spec["tile_variants"] != 1
            or spec["state_owner"] != family["state_owner"] or spec["frame_owner"] != family["frame_owner"]
            or spec["hidden_policy"] != "fully_transparent_no_source_pixels"
            or spec["empty_allowed_states"] != ["hidden"]
            or spec["render_layer"] != "ground_overlay_preserves_hardness_and_mineral_markers"
            or spec["native_draw_offset"] != [0, 0]):
        raise ValueError("natural vent native visibility/field geometry mismatch")
    # Only the two visible rows need original detail. Hidden frames select actual
    # transparent original padding; a visible unknown badge would leak a source.
    if (source["grid"] != [4, 2] or source["visible_states"] != ["surveyed", "excavated"]
            or set(source["native_crop_boxes"]) != {"surveyed", "excavated"}
            or any(len(boxes) != 4 for boxes in source["native_crop_boxes"].values())):
        raise ValueError("natural vent original phase coverage mismatch")
    return family, source, spec


def registration_signature(document=None):
    """Freeze actual original/guide identities and all native source registrations.

    @param document Optional decoded natural source record.
    @return SHA256 covering visible/hidden crops and complete canonical geometry.
    """
    unused, source, unused_spec = source_inputs(document)
    approved = {name: source[name] for name in ("key", "path", "sha256", "references", "size", "grid", "visible_states",
                                                "native_crop_boxes", "hidden_crop", "native_geometry",
                                                "native_sample_size", "native_position")}
    return hashlib.sha256(json.dumps(approved, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()


def build(document=None):
    """Sample all natural overlay states without painting a floor or exposing hidden fields.

    @param document Optional isolated retained source record.
    @return Complete twelve-pose provenance report and deterministic indexed PNG bytes.
    @throws ValueError If any crop clips original detail or hidden padding contains a source.
    """
    family, source, spec = source_inputs(document)
    signature = registration_signature(document)
    if signature != spec.get("geometry_notes", {}).get("approved_natural_vent_source_sha256"):
        raise ValueError("natural vent registration differs from approved source")
    atlas = Image.new("P", (256, 93), 0)
    atlas.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    width, height = source["native_sample_size"]
    dx, dy = source["native_position"]
    # Explicit placement samples sparse detail inside the fixed native tile;
    # it cannot grow into a neighboring world cell or change the ground anchor.
    if not (1 <= width <= 64 and 1 <= height <= 31 and 0 <= dx <= 64 - width and 0 <= dy <= 31 - height):
        raise ValueError("natural vent sample leaves native tile")
    with Image.open(tube.base.repository_path(source["path"])) as original:
        original = original.convert("RGBA")
        # Hidden is deliberately absent in every phase. Visible rows retain
        # their own genuine authored detail and never borrow another row's state.
        for state_index, state in enumerate(STATES):
            for frame in range(4):
                box = source["hidden_crop"] if state == "hidden" else source["native_crop_boxes"][state][frame]
                left, top, right, bottom = box
                # Complete source windows stay inside their immutable original;
                # implicit padding could conceal clipped or missing natural detail.
                if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
                    raise ValueError("natural vent source crop leaves original")
                crop = original.crop(box)
                if state == "hidden":
                    if crop.getchannel("A").getextrema()[1] != 0:
                        raise ValueError("hidden natural vent reveals original pixels")
                pose = Image.new("RGBA", (64, 31))
                pose.paste(crop.resize((width, height), Image.Resampling.BOX), (dx, dy))
                if state == "hidden":
                    # The generic quantizer correctly refuses missing artwork.
                    # This state instead requires proven original alpha-zero
                    # absence, so it deliberately emits canonical transparent0.
                    native = Image.new("P", (64, 31), 0)
                    native.putpalette(tube.base.PALETTE_BYTES)
                else:
                    native = tube.base.quantize(pose, {})
                atlas.paste(native, (frame * 64, state_index * 31))
                sprites.append({"asset_id": family["asset_id"], "state": state, "direction": "none", "frame": frame,
                                "part": "body", "tile_variant": 0, "anchor": [-31, 0], "box": [frame * 64, state_index * 31, 64, 31],
                                "source": source["path"], "source_sha256": source["sha256"], "source_key": source["key"],
                                "source_crop": box, "native_sample_size": [width, height], "native_position": [dx, dy]})
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md54C; archi-map-layers.md; complete natural overlay source, native binding pending",
              "schema_version": 1, "asset_id": family["asset_id"], "source_record": RECORD.relative_to(tube.ROOT).as_posix(),
              "path": (tube.REVIEW / "vent-authored.png").relative_to(tube.ROOT).as_posix(), "size": [256, 93],
              "native_size": [64, 31], "anchor": [-31, 0], "pose_count": len(sprites), "sprites": sprites,
              "field_owner": family["field_owner"], "visibility_policy": family["visibility_policy"],
              "state_owner": family["state_owner"], "frame_owner": family["frame_owner"], "render_layer": family["render_layer"],
              "approved_source_sha256": signature, "source_coverage_complete": True, "runtime_activated": False}
    return report, tube.indexed_png(atlas)


def validate(report, encoded, document=None):
    """Reject hidden information leaks, opaque replacement ground and false source custody.

    @param report Complete source/phase mapping to inspect.
    @param encoded Actual native indexed PNG bytes.
    @param document Optional isolated original/registration record.
    @return None; accepted source neither changes geology nor binds native rendering.
    """
    required = {(state, frame) for state in STATES for frame in range(4)}
    actual = [(row["state"], row["frame"]) for row in report["sprites"]]
    # True unique states/phases are required; duplicate hidden sprites cannot
    # pad the count while omitting a visible native presentation.
    if len(actual) != 12 or set(actual) != required or report["pose_count"] != 12:
        raise ValueError("natural vent source state/phase coverage mismatch")
    diamond = Image.new("1", (64, 31))
    ImageDraw.Draw(diamond).polygon([(31, 0), (63, 15), (31, 30), (0, 15)], fill=1)
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Canonical native encoding keeps transparent absence and prohibits
        # palette animation/company remap from manufacturing another source cue.
        if (atlas.mode != "P" or atlas.size != (256, 93) or atlas.info.get("transparency") != 0
                or atlas.getpalette()[:768] != tube.base.PALETTE_BYTES
                or set(atlas.get_flattened_data()) & (tube.base.FORBIDDEN - {0})):
            raise ValueError("natural vent palette/canvas mismatch")
        phases = {state: [] for state in STATES}
        # Every visible overlay stays sparse and inside its real native ground
        # diamond. Hidden ground cannot reveal even one field-specific pixel.
        for row in report["sprites"]:
            expected_box = [row["frame"] * 64, STATES.index(row["state"]) * 31, 64, 31]
            # A correct source PNG cannot authorize a moved world anchor,
            # another view/part or a crop outside the declared native pose.
            if (row["asset_id"] != "geothermal.vent" or row["direction"] != "none" or row["part"] != "body"
                    or row["tile_variant"] != 0 or row["anchor"] != [-31, 0] or row["box"] != expected_box):
                raise ValueError("natural vent native pose geometry mismatch")
            x, y, width, height = row["box"]
            pose = atlas.crop((x, y, x + width, y + height))
            phases[row["state"]].append(pose.tobytes())
            count = sum(value != 0 for value in pose.get_flattened_data())
            if row["state"] == "hidden":
                if count != 0:
                    raise ValueError("hidden natural vent reveals source pixels")
            else:
                if not 1 <= count < 64 * 31 * .4:
                    raise ValueError("natural vent paints replacement terrain or omits detail")
                for py in range(31):
                    for px in range(64):
                        if pose.getpixel((px, py)) != 0 and diamond.getpixel((px, py)) == 0:
                            # A thermal detail cannot spill into a neighboring
                            # field/tile through a shared ordinary terrain edge.
                            raise ValueError("natural vent original detail leaves native diamond")
        for state in ("surveyed", "excavated"):
            if len(set(phases[state])) != 4:
                raise ValueError("natural vent repeats an original thermal detail phase")
    expected, pixels = build(document)
    for field in ("schema_version", "asset_id", "source_record", "path", "size", "native_size", "anchor", "sprites",
                  "field_owner", "visibility_policy", "state_owner", "frame_owner", "render_layer", "approved_source_sha256",
                  "source_coverage_complete", "runtime_activated"):
        if report[field] != expected[field]:
            raise ValueError("natural vent false original/visibility provenance")
    if encoded != pixels:
        raise ValueError("natural vent changes original source pixels")


def contact_page(report, encoded):
    """Compare all states/phases over actual paid floors of every rock hardness.

    @param report Complete source mapping with native state ownership.
    @param encoded Actual native overlay atlas bytes.
    @return Deterministic PNG contact; its backgrounds remain review context only.
    """
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    hardness = next(row for row in public["assets"] if row["asset_id"] == "terrain.hardness")
    floor_path = tube.base.repository_path(hardness["authoring"]["native_atlas"])
    if hashlib.sha256(floor_path.read_bytes()).hexdigest() != hardness["authoring"]["native_sha256"]:
        raise ValueError("natural vent review hardness source changed")
    page = Image.new("RGB", (1040, 1380), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    with Image.open(io.BytesIO(encoded)) as atlas, Image.open(floor_path) as floors:
        # Each row keeps its existing hardness floor; only the genuine sparse
        # detail overlays it. Hidden frames must look exactly like plain rock.
        for row in report["sprites"]:
            state_index = STATES.index(row["state"])
            frame = row["frame"]
            x, y, width, height = row["box"]
            overlay = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
            left, top = frame * 260, state_index * 460
            label = "hidden / no source pixels" if row["state"] == "hidden" else row["state"] + " / genuine phase" + str(frame)
            draw.text((left + 8, top + 8), label, fill=(30, 40, 50))
            for rock in range(4):
                floor = floors.crop((3 * 64, rock * 31, 4 * 64, (rock + 1) * 31)).convert("RGBA")
                composite = Image.alpha_composite(floor, overlay)
                visible = Image.new("RGB", (64, 31), (235, 239, 243))
                visible.paste(composite, mask=composite.getchannel("A"))
                page.paste(visible.resize((192, 93), Image.Resampling.NEAREST), (left + 34, top + 34 + rock * 104))
    target = io.BytesIO()
    page.save(target, format="PNG")
    return target.getvalue()


def publish(check=False, bind=False):
    """Generate/verify complete natural source and optionally publish its metadata.

    @param check True compares atlas/mapping/contact bytes without regenerating files.
    @param bind True publishes only this family's complete source/authoring metadata.
    @return Complete source report path; packed payload/native runtime stay unchanged.
    """
    report, encoded = build()
    validate(report, encoded)
    contact = tube.REVIEW / "vent-state-contact.png"
    contact_bytes = contact_page(report, encoded)
    report.update(sha256=hashlib.sha256(encoded).hexdigest(),
                  contact={"path": contact.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(contact_bytes).hexdigest()})
    files = {tube.base.repository_path(report["path"]): encoded, contact: contact_bytes}
    serialized = json.dumps(report, indent=2) + "\n"
    if check:
        if any(path.read_bytes() != pixels for path, pixels in files.items()) or TARGET.read_text(encoding="utf-8") != serialized:
            # Source identity, phase coverage and actual reviewed context must
            # regenerate together before later native packing can consume them.
            raise ValueError("natural vent source needs deterministic regeneration")
    else:
        for path, pixels in files.items():
            path.write_bytes(pixels)
        TARGET.write_text(serialized, encoding="utf-8")
    if bind:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
        family = next(row for row in public["assets"] if row["asset_id"] == report["asset_id"])
        first = report["sprites"][0]
        family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                            "provenance": "first_party_original_natural_overlay", "rights": "first_party_generated_original"}
        family["authoring"] = {"date": "2026-10-10", "status": "complete_natural_vent_overlay_source_native_binding_pending",
                               "source_record": report["source_record"], "originals": [first["source"]],
                               "atlas_manifest": TARGET.relative_to(tube.ROOT).as_posix(), "native_atlas": report["path"],
                               "native_sha256": report["sha256"], "staged_pose_count": 12, "coverage_complete": True,
                               "runtime_activated": False}
        tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    return TARGET


def main():
    """Run deterministic natural source conversion without introducing a runtime selector.

    @return None; print the source report or propagate its exact refusal.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Verify exact existing source proof read-only")
    parser.add_argument("--publish-source", action="store_true", help="Publish complete verified natural source metadata")
    arguments = parser.parse_args()
    print(publish(check=arguments.check, bind=arguments.publish_source))


if __name__ == "__main__":
    main()
