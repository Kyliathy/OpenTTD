"""Convert immutable platform models and authored status components to native source.

@file convert_tube_terminals.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native station identity/packets/platform capacity remain their existing owners.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageChops, ImageDraw

import convert_tube_art as tube
import convert_facility_art as facilities


TARGET = tube.REVIEW / "tube-terminal-atlas-manifest.json"
AXES = {"NE": "X", "SE": "Y"}


def native_platform(candidate, frame=None):
    """Register an original platform cell without inventing source pixels or views.

    @param candidate Checked immutable source and explicit native transform.
    @param frame Optional actual state/axis/role frame with an isolated source region.
    @return Indexed64x64 source cell at the actual building anchor(-31,-33).
    """
    transform = candidate["native_transform"] if frame is None else frame
    # Every native platform role occupies one physical cell on one real axis.
    # Reversed headings and a decorative campus cannot supply a missing view.
    if (transform["native_size"] != [64, 64] or transform["anchor"] != [-31, -33]
            or transform["axis"] not in ("X", "Y") or transform["role"] not in range(4)
            or transform["state"] not in ("construction", "ready", "working", "blocked")
            or transform["resampling"] != "BOX"):
        raise ValueError("unsupported native platform transform")
    identity = transform["state"], transform["axis"], transform["role"]
    if identity in {(row["state"], row["axis"], row["role"]) for row in candidate.get("excluded_frames", [])}:
        # Every converter path must reject a known relocated/model-changing
        # frame, including direct family composition without a review command.
        raise ValueError("excluded native platform original frame")
    crop, shift = transform["source_crop"], transform["translation"]
    # Explicit transparent padding aligns independently authored models to the
    # actual floor contact; large shifts cannot conceal a different footprint.
    if (len(crop) != 4 or crop[2] <= crop[0] or crop[3] <= crop[1] or len(shift) != 2
            or any(type(value) is not int or abs(value) > 3 for value in shift)):
        raise ValueError("unsupported native platform source registration")
    with Image.open(tube.base.repository_path(candidate["path"])) as original:
        selected = original.convert("RGBA")
        if "source_polygon" in transform:
            mask = Image.new("L", selected.size, 0)
            ImageDraw.Draw(mask).polygon([tuple(point) for point in transform["source_polygon"]], fill=255)
            selected.putalpha(ImageChops.multiply(selected.getchannel("A"), mask))
        selected = selected.crop(tuple(crop)).resize((64, 64), Image.Resampling.BOX)
    aligned = Image.new("RGBA", (64, 64))
    aligned.paste(selected, tuple(shift))
    return tube.base.quantize(aligned, {})


def foreground(whole, axis):
    """Keep the exact low near platform ledge outside the actual capsule chord.

    @param whole Full native authored platform cell, including low end coupling plates.
    @param axis Actual native X or Y through-platform orientation.
    @return Aligned exact original front subset; the full back stays beneath vehicles.
    """
    if axis not in ("X", "Y"):
        raise ValueError("unknown native platform axis")
    result = whole.copy()
    pixels = []
    # Native building-canvas paths are the ground chords shifted by33 pixels.
    # The front may redraw only the screen-lower loading ledge; flat coupling
    # plates stay behind/below vehicles and cannot hide their actual route.
    for offset, value in enumerate(whole.get_flattened_data()):
        x, y = offset % 64, offset // 64
        depth = 2 * y + x - 129 if axis == "X" else 2 * y - x - 67
        pixels.append(value if depth > 3 else 0)
    result.putdata(pixels)
    return result


def validate_platform_contacts(whole, axis, role=0):
    """Check actual platform contact geometry independently of source captions.

    @param whole Native indexed authored cell before foreground selection.
    @param axis Actual independently authored native X or Y through direction.
    @param role Native default graphic role; bare0 stays low, building/canopy1..3 may extend upward.
    @return None; reject an oversized campus, absent loading ledge or changed palette.
    """
    bounds = whole.getbbox()
    if bounds is None:
        raise ValueError("empty native bare platform")
    left, top, right, bottom = bounds
    # Bare loading ledges sit on one floor cell and stay low. Taller building
    # and canopy roles have separate originals; a large depot cannot stand in
    # for a missing bare station cell or spill onto unpaid neighbouring floor.
    minimum_top = 30 if role == 0 else 0
    if (axis not in ("X", "Y") or role not in range(4) or left < 5 or right > 59 or top < minimum_top or bottom > 64
            or right - left < 38 or bottom - top < 18):
        raise ValueError("native bare platform leaves its local floor footprint")
    contacts = ((41, 38), (53, 44), (9, 54), (21, 60)) if axis == "X" else ((9, 44), (21, 38), (41, 60), (53, 54))
    # Both side ledges must reach both true through mouths. A source with one
    # missing edge cannot be completed by painted pixels or repeated captions.
    for px, py in contacts:
        if not any(whole.getpixel((x, y)) != 0 for y in range(py - 2, min(64, py + 3)) for x in range(px - 2, px + 3)):
            raise ValueError("native bare platform misses an actual ledge contact")
    if set(whole.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
        raise ValueError("forbidden native platform palette index")


def review_candidate(key, role=None, axis=None, state="ready"):
    """Show one original cell/near ledge and its actual through-platform joins.

    @param key Explicit retained source candidate; no latest-file discovery is allowed.
    @param role Optional exact native role from a reviewed multi-frame original.
    @param axis Optional required X/Y when a sheet contains both actual axes.
    @param state Required authored lifecycle for multi-frame originals.
    @return Bound base-only proof path; full state/view/role coverage remains pending.
    """
    candidate = tube.retained_candidate(key)
    if candidate["status"].startswith("rejected"):
        raise ValueError("rejected platform original selected")
    frame = None
    if "frames" in candidate:
        matches = [row for row in candidate["frames"] if row["role"] == role and row["state"] == state and (axis is None or row["axis"] == axis)]
        if len(matches) != 1:
            # An explicit actual role/axis is required; sheet captions cannot
            # silently choose a duplicate or missing native platform source.
            raise ValueError("missing or duplicate explicit native platform frame")
        frame = matches[0]
    transform = candidate["native_transform"] if frame is None else frame
    whole = native_platform(candidate, frame)
    validate_platform_contacts(whole, transform["axis"], transform["role"])
    front = foreground(whole, transform["axis"])
    board = Image.new("RGB", (1536, 850), (48, 52, 58))
    labels = ImageDraw.Draw(board)
    labels.text((16, 14), f"NATIVE PLATFORM {transform['axis']} / role{transform['role']}: one retained original; full source/packing/runtime pending", fill="white")
    for index, (label, pixels) in enumerate((("whole4x", whole), ("exact near ledge4x", front))):
        enlarged = pixels.convert("RGBA").resize((256, 256), Image.Resampling.NEAREST)
        board.paste(enlarged, (16 + index * 300, 70), enlarged)
        labels.text((16 + index * 300, 45), label, fill="white")
    scene = Image.new("RGBA", (192, 128))
    steps = ((-32, 16), (0, 0), (32, -16)) if transform["axis"] == "X" else ((-32, -16), (0, 0), (32, 16))
    # Three repeated cells use real32/16 world pitch and shared anchors. The
    # proof checks through ledge/mouth continuity, never another campus floor.
    for dx, dy in steps:
        scene.alpha_composite(whole.convert("RGBA"), (64 + dx, 32 + dy))
    enlarged = scene.resize((768, 512), Image.Resampling.NEAREST)
    board.paste(enlarged, (700, 70), enlarged)
    native = whole.convert("RGBA")
    board.paste(native, (16, 380), native)
    labels.text((16, 470), "64x64 / anchor(-31,-33); capsule path remains clear in the foreground; flat coupling plates remain below vehicles", fill="white")
    tube.REVIEW.mkdir(parents=True, exist_ok=True)
    name = key if frame is None else f"{key}-{state}-{transform['axis']}-role{role}"
    native_path = tube.REVIEW / f"{name}-native.png"
    contact_path = tube.REVIEW / f"{name}-contact.png"
    native_path.write_bytes(tube.indexed_png(whole))
    board.save(contact_path)
    report = {"_documentation": "Plan13 Group48C.1/master10.4; archi-map-layers.md. Single native platform base review only.",
              "source": candidate["path"], "source_sha256": candidate["sha256"], "native_transform": transform,
              "native": native_path.relative_to(tube.ROOT).as_posix(), "native_sha256": tube.base.digest(native_path),
              "contact": contact_path.relative_to(tube.ROOT).as_posix(), "contact_sha256": tube.base.digest(contact_path),
              "source_coverage_complete": False, "runtime_activated": False}
    target = tube.REVIEW / f"{name}-review.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def family_spec(document):
    """Resolve the one actual platform source contract and its declared model reuse.

    @param document Decoded ground provenance with explicit selected platform models.
    @return Checked source family; no new station, cargo or electrical owner is created.
    """
    matches = [row for row in document["families"] if row["asset_id"] == "facility.tube-terminal"]
    if len(matches) != 1:
        raise ValueError("missing or duplicate native platform source family")
    spec = matches[0]
    # Both native axes/four roles use one world-cell canvas. Installed working
    # and blocked buildings keep their physical model; genuine original lamp
    # components alone present those states, with no painted vehicle/cargo.
    if (spec["directions"] != list(AXES) or spec["states"] != facilities.STATES or spec["frames"] != 4
            or spec["parts"] != ["back", "front"] or spec["native_size"] != [64, 64]
            or spec["anchor"] != [-31, -33] or spec["footprint"] != [1, 1] or spec["tile_variants"] != 4
            or spec["variant_semantics"] != "native_station_graphics_role"
            or spec["native_graphics"] != {"NE": [0, 2, 4, 6], "SE": [1, 3, 5, 7]}
            or spec["model_state_for"] != {"construction": "construction", "ready": "ready", "working": "ready", "blocked": "ready"}):
        raise ValueError("native platform source geometry/model-state contract mismatch")
    return spec


def selected_models(document):
    """Bind all sixteen actual construction/installed native model cells.

    @param document Immutable original provenance and explicit state/axis/role selections.
    @return Checked model map containing original metadata, actual transform and indexed pixels.
    """
    spec = family_spec(document)
    identities = [(row["state"], row["axis"], row["role"]) for row in spec["selections"]]
    required = {(state, axis, role) for state in ("construction", "ready") for axis in AXES.values() for role in range(4)}
    if len(identities) != len(required) or set(identities) != required:
        # A complete count cannot hide an absent axis/role or substitute the
        # installed model for genuinely unfinished construction geometry.
        raise ValueError("incomplete or duplicate native platform model coverage")
    originals, result = {}, {}
    # Resolve only explicitly selected actual frames. Shared ready geometry
    # is reused honestly for installed states rather than relabelled as a new
    # generated model; known relocated source frames remain excluded.
    for selection in spec["selections"]:
        key = selection["source"]
        if key not in originals:
            originals[key] = tube.retained_candidate(key, document)
        original = originals[key]
        if original["status"].startswith("rejected"):
            raise ValueError("rejected native platform source selected")
        identity = selection["state"], selection["axis"], selection["role"]
        transform, frame = original.get("native_transform"), None
        if "source_frame" in selection:
            matches = [row for row in original["frames"] if [row["state"], row["axis"], row["role"]] == selection["source_frame"]]
            if len(matches) != 1:
                raise ValueError("missing or duplicate retained native platform frame")
            frame = transform = matches[0]
        if transform is None or (transform["state"], transform["axis"], transform["role"]) != identity:
            raise ValueError("native platform model state/axis/role substitution")
        whole = native_platform(original, frame)
        validate_platform_contacts(whole, identity[1], identity[2])
        result[identity] = original, transform, whole
    return result


def original_indicator(spec):
    """Read the existing authoritative authored four-state/four-phase component.

    @param spec Platform source contract naming the canonical M1 component record/key.
    @return Exact component provenance and genuine reduced RGBA phases.
    """
    # The original shared component is the sole source of lamp art. Another
    # private state palette or an unrelated file cannot silently replace it.
    if spec["indicator_record"] != facilities.RECORD.relative_to(tube.ROOT).as_posix() or spec["indicator_key"] != "facility-status":
        raise ValueError("native platform indicator source contract mismatch")
    parent = json.loads(facilities.RECORD.read_text(encoding="utf-8"))
    matches = [row for row in parent["components"] if row["key"] == spec["indicator_key"]]
    if len(matches) != 1:
        raise ValueError("missing or duplicate native platform indicator original")
    return matches[0], facilities.indicator(matches[0])


def build(document=None):
    """Assemble256 exact native model/state/phase/back-front source poses.

    @param document Optional isolated provenance for focused malformed-input checks.
    @return Complete source mapping and indexed PNG bytes; packing/runtime remain separate.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    spec = family_spec(document)
    models = selected_models(document)
    component, phases = original_indicator(spec)
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == spec["asset_id"])
    # Public native geometry must agree before authoring. No old four-door
    # campus or silent role omission can inherit platform source completeness.
    if any(family[key] != spec[key] for key in ("directions", "states", "frames", "parts", "native_size", "anchor", "footprint", "tile_variants", "variant_semantics", "native_graphics")):
        raise ValueError("native platform public geometry mismatch")
    if set(spec["cue_targets"]) != {"X", "Y"}:
        raise ValueError("missing native platform cue axis")
    atlas = Image.new("P", (1024, 1024), 0)
    atlas.putpalette(tube.base.PALETTE_BYTES)
    sprites, cache = [], {}
    # Stable state/axis/role/phase/part order keeps one true model canvas and
    # uses only original authored glass inside an existing opaque ledge fitting.
    # Native vehicles/packets still supply occupancy and cargo in the real game.
    for state_index, state in enumerate(facilities.STATES):
        for axis_index, (direction, axis) in enumerate(AXES.items()):
            if len(spec["cue_targets"][axis]) != 4:
                raise ValueError("missing native platform cue role")
            for role in range(4):
                original, transform, model = models[spec["model_state_for"][state], axis, role]
                target = spec["cue_targets"][axis][role]
                x, y, width, height = target
                # Status glass belongs on the rear ledge, away from real
                # capsule paths and cell contacts. All nine original material
                # pixels must remain opaque; component insertion never adds alpha.
                depth = 2 * (y + 2) + x + 2 - 129 if axis == "X" else 2 * (y + 2) - x - 67
                if [width, height] != [3, 3] or not (20 <= x <= 42 and 36 <= y <= 54) or depth >= -4:
                    raise ValueError("native platform cue leaves its rear ledge fitting")
                for frame in range(4):
                    pose = facilities.insert_indicator(model.convert("RGBA"), phases[state_index][frame], target)
                    back = tube.base.quantize(pose, cache)
                    front = foreground(back, axis)
                    for part_index, (part, pixels) in enumerate((("back", back), ("front", front))):
                        sx = ((axis_index * 4 + role) * 2 + part_index) * 64
                        sy = (state_index * 4 + frame) * 64
                        atlas.paste(pixels, (sx, sy))
                        sprites.append({"state": state, "direction": direction, "axis": axis, "tile_variant": role,
                                        "native_station_graphics": role * 2 + axis_index, "frame": frame, "part": part,
                                        "anchor": [-31, -33], "box": [sx, sy, 64, 64], "source": original["path"],
                                        "source_sha256": original["sha256"], "native_transform": transform,
                                        "model_state": spec["model_state_for"][state],
                                        "indicator": {"path": component["path"], "sha256": component["sha256"],
                                                      "state": state, "frame": frame, "target": target}})
    report = {"_documentation": "Plan13 Group48C/master10.4; archi-map-layers.md. Complete native platform models and genuine authored status phases; operating/packing/runtime pending.",
              "schema_version": 1, "asset_id": "facility.tube-terminal", "source_record": tube.RECORD.relative_to(tube.ROOT).as_posix(),
              "path": (tube.REVIEW / "tube-terminal-authored.png").relative_to(tube.ROOT).as_posix(),
              "native_cell": [64, 64], "size": [1024, 1024], "anchor": [-31, -33], "pose_count": len(sprites), "sprites": sprites,
              "source_coverage_complete": True, "runtime_activated": False,
              "model_state_for": spec["model_state_for"], "indicator_record": spec["indicator_record"],
              "foreground_policy": "whole authored model/status back; exact lower ledge subset outside actual native capsule chord; flat coupling plates stay below vehicles"}
    return report, tube.indexed_png(atlas)


def validate(report, encoded, document=None):
    """Reject missing poses, false source/state ancestry and obstructed native paths.

    @param report Complete256-pose mapping with actual model and authored glass identities.
    @param encoded Native indexed atlas bytes to inspect.
    @param document Optional isolated retained provenance for refusal tests.
    @return None; accepted source does not establish operation, packing or runtime binding.
    """
    required = {(state, direction, role, frame, part) for state in facilities.STATES for direction in AXES
                for role in range(4) for frame in range(4) for part in ("back", "front")}
    actual = [(row["state"], row["direction"], row["tile_variant"], row["frame"], row["part"]) for row in report["sprites"]]
    if len(actual) != len(required) or set(actual) != required:
        # Unique count and true native identities must both establish coverage.
        raise ValueError("incomplete native platform state/axis/role/phase/part coverage")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # The native DOS palette/transparent index and fixed per-cell canvas
        # preserve existing source packing and prohibit fake animated colours.
        if (atlas.mode != "P" or atlas.size != (1024, 1024) or atlas.info.get("transparency") != 0
                or atlas.getpalette()[:768] != tube.base.PALETTE_BYTES):
            raise ValueError("native platform palette/canvas mismatch")
        if set(atlas.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
            raise ValueError("forbidden native platform palette index")
        signatures = {}
        # Every front is checked against independent actual building-canvas
        # capsule chords, not only source-subset agreement. Exact copied pixels
        # cannot legitimize a front wall across a real through route.
        for row in report["sprites"]:
            x, y, width, height = row["box"]
            if (row["anchor"] != [-31, -33] or [width, height] != [64, 64]
                    or x < 0 or y < 0 or x + width > 1024 or y + height > 1024):
                raise ValueError("native platform anchor/box mismatch")
            image = atlas.crop((x, y, x + width, y + height))
            if image.getbbox() is None:
                raise ValueError("empty native platform source part")
            if row["part"] == "front":
                for py in range(33, 64):
                    for px in range(15, 48):
                        distance = abs(2 * py + px - 129) if row["axis"] == "X" else abs(2 * py - px - 67)
                        if distance <= 2 and image.getpixel((px, py)) != 0:
                            raise ValueError("native platform foreground covers actual capsule route")
            else:
                key = row["state"], row["direction"], row["tile_variant"]
                signatures.setdefault(key, []).append(image.tobytes())
        if any(len(set(values)) != 4 for values in signatures.values()):
            # Only actual authored lamp phases count; four duplicate sprite
            # identities cannot replace the genuine state component artwork.
            raise ValueError("native platform repeats an authored indicator phase")
    expected, pixels = build(document)
    if report["sprites"] != expected["sprites"] or report["model_state_for"] != expected["model_state_for"]:
        raise ValueError("native platform false model/component provenance")
    if encoded != pixels:
        raise ValueError("native platform changes original model/component pixels")


def contact_pages(report, encoded):
    """Present all native roles/states and all four real status phases per axis.

    @param report Complete256-pose source mapping.
    @param encoded Exact unchanged indexed source atlas bytes.
    @return Axis-keyed PNG review page bytes; no game art is drawn or changed.
    """
    pages = {}
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Two disjoint axis pages expose every actual native role/state and
        # front cut. Native phase strips make repeated/missing status art visible.
        for direction, axis in AXES.items():
            board = Image.new("RGB", (2048, 1320), (48, 52, 58))
            labels = ImageDraw.Draw(board)
            labels.text((12, 12), f"NATIVE PLATFORM {axis}: four actual graphics roles; retained physical models/authored lamps; operating/packing/runtime pending", fill="white")
            for state_index, state in enumerate(facilities.STATES):
                for role in range(4):
                    left, top = role * 512, state_index * 320 + 40
                    labels.text((left + 12, top), f"{state} role{role}: whole/front3x; phases1x", fill="white")
                    for part_index, part in enumerate(("back", "front")):
                        row = next(s for s in report["sprites"] if (s["state"], s["direction"], s["tile_variant"], s["frame"], s["part"]) == (state, direction, role, 0, part))
                        x, y, width, height = row["box"]
                        image = atlas.crop((x, y, x + width, y + height)).convert("RGBA").resize((192, 192), Image.Resampling.NEAREST)
                        board.paste(image, (left + part_index * 220, top + 24), image)
                    for frame in range(4):
                        row = next(s for s in report["sprites"] if (s["state"], s["direction"], s["tile_variant"], s["frame"], s["part"]) == (state, direction, role, frame, "back"))
                        x, y, width, height = row["box"]
                        image = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
                        board.paste(image, (left + 12 + frame * 80, top + 225), image)
            stream = io.BytesIO()
            board.save(stream, format="PNG")
            pages[axis] = stream.getvalue()
    return pages


def publish(check=False):
    """Rebuild complete source proof or verify its exact bytes read-only.

    @param check True to compare actual atlas, all contacts and source mapping.
    @return Bound complete source report path; no packed file is changed.
    """
    report, encoded = build()
    validate(report, encoded)
    files = {tube.base.repository_path(report["path"]): encoded}
    report.update(sha256=hashlib.sha256(encoded).hexdigest(), contact_pages=[])
    # Every actual axis page belongs to this proof, so a stale/unreviewed
    # direction cannot inherit a successful source handoff from another page.
    for axis, pixels in contact_pages(report, encoded).items():
        path = tube.REVIEW / f"tube-terminal-{axis}-contact.png"
        files[path] = pixels
        report["contact_pages"].append({"axis": axis, "path": path.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(pixels).hexdigest()})
    text = json.dumps(report, indent=2) + "\n"
    if check:
        if any(path.read_bytes() != pixels for path, pixels in files.items()) or TARGET.read_text(encoding="utf-8") != text:
            # Original pixels/registrations and traceable source states must
            # regenerate together before later packing/runtime may consume them.
            raise ValueError("native platform proof needs deterministic regeneration")
    else:
        tube.REVIEW.mkdir(parents=True, exist_ok=True)
        for path, pixels in files.items():
            path.write_bytes(pixels)
        TARGET.write_text(text, encoding="utf-8")
    return TARGET


def bind_source():
    """Publish complete platform source without changing packed IDs or runtime flags.

    @return None; only this family's source/authoring metadata is updated.
    """
    report = json.loads(publish(check=True).read_text(encoding="utf-8"))
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "facility.tube-terminal")
    first = report["sprites"][0]
    family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                        "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
    originals = list(dict.fromkeys(row["source"] for row in report["sprites"]))
    originals.extend(path for path in dict.fromkeys(row["indicator"]["path"] for row in report["sprites"]) if path not in originals)
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_platform_role_state_phase_source_runtime_pending",
                           "source_record": report["source_record"], "originals": originals,
                           "atlas_manifest": TARGET.relative_to(tube.ROOT).as_posix(), "native_atlas": report["path"],
                           "native_sha256": report["sha256"], "staged_pose_count": 256,
                           "coverage_complete": True, "runtime_activated": False}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Review one selected original or build/check the complete platform family.

    @return None; print the exact proof path and preserve refusal diagnostics.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", help="Review one explicit original/frame only")
    parser.add_argument("--role", type=int, choices=range(4), help="Exact native role for multi-frame originals")
    parser.add_argument("--axis", choices=("X", "Y"), help="Required source axis when a sheet has both")
    parser.add_argument("--state", default="ready", choices=("construction", "ready", "working", "blocked"))
    parser.add_argument("--check", action="store_true", help="Read-only complete source regeneration")
    parser.add_argument("--publish-source", action="store_true", help="Bind complete source coverage without packing")
    args = parser.parse_args()
    # Candidate review has no completeness claim; read-only verification must
    # never update public metadata or accept ambiguous per-frame source options.
    if (args.check and args.publish_source) or (args.candidate and (args.check or args.publish_source)):
        parser.error("candidate/check/publication modes cannot be combined")
    if args.candidate:
        print(review_candidate(args.candidate, args.role, args.axis, args.state))
    else:
        if args.role is not None or args.axis is not None or args.state != "ready":
            # Exact role/state selectors apply only to a named original review.
            parser.error("role/axis/state selectors require --candidate")
        print(publish(args.check))
        if args.publish_source:
            bind_source()


if __name__ == "__main__":
    main()
