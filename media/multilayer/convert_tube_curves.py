"""Convert retained native tube corner originals to complete aligned source parts.

@file convert_tube_curves.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Source presentation only: no installed tracks, utility quantities or packed IDs change.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube


TARGET = tube.REVIEW / "tube-curves-atlas-manifest.json"


def selected_sources(document):
    """Bind every native corner/lifecycle to one actual immutable source frame.

    @param document Retained generation record with explicit family selections.
    @return Checked state/corner map containing original metadata, transform and indexed pixels.
    """
    families = [row for row in document.get("families", []) if row["asset_id"] == "tube.bend45"]
    if len(families) != 1:
        raise ValueError("missing or duplicate selected tube corner family")
    selections = families[0]["selections"]
    required = {(state, corner) for state in tube.STATES for corner in tube.CORNERS}
    identities = [(row["state"], row["direction"]) for row in selections]
    # A complete count cannot hide a missing corner or a duplicate lifecycle;
    # the public geometry matrix must exist once before any atlas is authored.
    if len(identities) != len(required) or set(identities) != required:
        raise ValueError("incomplete or duplicate native corner state coverage")
    result = {}
    # Resolve actual source geometry, not requested sheet captions. The
    # generator may swap cells or omit a panel; only reviewed explicit frame
    # selections can bind those pixels to a public native corner.
    for selection in selections:
        candidate = tube.retained_candidate(selection["source"], document)
        if candidate["status"].startswith("rejected"):
            raise ValueError("rejected tube corner original selected")
        identity = selection["state"], selection["direction"]
        transform = candidate.get("native_transform")
        frame = None
        if "source_frame" in selection:
            matches = [row for row in candidate["frames"] if (row["state"], row["corner"]) == tuple(selection["source_frame"])]
            if len(matches) != 1:
                raise ValueError("missing or duplicate retained native corner frame")
            frame = transform = matches[0]
        # Reusing a different lifecycle or mirrored-looking source would
        # falsely fill the matrix while breaking actual mouth geometry/state.
        if transform is None or (transform["state"], transform["corner"]) != identity:
            raise ValueError("native tube corner state/view substitution")
        excluded = {(row["state"], row["corner"]) for row in candidate.get("excluded_frames", [])}
        if identity in excluded:
            raise ValueError("excluded native tube corner frame selected")
        result[identity] = candidate, transform, tube.native_curve(candidate, frame)
    return result


def build(document=None):
    """Build the complete public24-pose matrix without publishing or packing it.

    @param document Optional decoded provenance for independent malformed-input tests.
    @return Report and exact indexed PNG bytes; retained originals stay untouched.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    sources = selected_sources(document)
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.bend45")
    # Native Upper/Lower/Left/Right and shared placement are authoritative;
    # conversion cannot silently redefine views, parts or geometry to pass.
    if (family["directions"] != list(tube.CORNERS) or family["states"] != list(tube.STATES)
            or family["parts"] != ["back", "front"] or family["frames"] != 1
            or family["anchor"] != [-31, 0] or family["native_size"] != [64, 31]):
        raise ValueError("native tube corner public contract mismatch")
    sheet = Image.new("P", (512, 93), 0)
    sheet.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    # Stable public state/view/part order feeds later packing. Low coupling
    # plates stay in the whole back layer; the front keeps only facing rims
    # with the actual capsule chord/mouth aperture left clear.
    for row, state in enumerate(tube.STATES):
        for column, corner in enumerate(tube.CORNERS):
            original, transform, whole = sources[state, corner]
            for part_index, (part, pixels) in enumerate((("back", whole), ("front", tube.curve_foreground(whole, corner)))):
                x, y = (column * 2 + part_index) * 64, row * 31
                sheet.paste(pixels, (x, y))
                sprites.append({"state": state, "direction": corner, "part": part, "frame": 0, "tile_variant": 0,
                                "anchor": [-31, 0], "box": [x, y, 64, 31], "source": original["path"],
                                "source_sha256": original["sha256"], "native_transform": transform})
    report = {"_documentation": "Plan13 Group47C/master10.4; archi-map-layers.md. Complete native corner source art; packing/runtime acceptance pending.",
              "schema_version": 1, "asset_id": "tube.bend45", "source_record": tube.RECORD.relative_to(tube.ROOT).as_posix(),
              "path": (tube.REVIEW / "tube-curves-authored.png").relative_to(tube.ROOT).as_posix(),
              "native_cell": [64, 31], "size": list(sheet.size), "anchor": [-31, 0], "sprites": sprites,
              "pose_count": len(sprites), "source_coverage_complete": True, "runtime_activated": False,
              "foreground_policy": "back retains the whole authored casing/low coupling plates; front retains exact facing-rim source pixels outside the native capsule chord aperture"}
    return report, tube.indexed_png(sheet)


def validate(report, encoded):
    """Check exact pose coverage, palette, placement and actual foreground clearance.

    @param report Complete source mapping, independent of runtime or packed acceptance.
    @param encoded Actual indexed atlas bytes to inspect.
    @return None; reject missing parts, changed pixels or a covered capsule chord.
    """
    required = {(state, corner, part) for state in tube.STATES for corner in tube.CORNERS for part in ("back", "front")}
    identities = [(row["state"], row["direction"], row["part"]) for row in report["sprites"]]
    if len(identities) != len(required) or set(identities) != required:
        # Count and identity must agree so a duplicate cannot hide a missing rim.
        raise ValueError("incomplete native tube corner part coverage")
    with Image.open(io.BytesIO(encoded)) as sheet:
        # All source parts use the same native DOS canvas and transparent
        # index as the accepted straights; animated colours cannot enter art.
        if (sheet.mode != "P" or sheet.size != (512, 93) or sheet.info.get("transparency") != 0
                or sheet.getpalette()[:768] != tube.base.PALETTE_BYTES):
            raise ValueError("native tube corner palette/canvas mismatch")
        if set(sheet.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
            raise ValueError("forbidden native tube corner palette index")
        parts = {}
        # Check every actual crop, not only the overall sheet dimensions.
        # Both parts must stay nonempty and share their native ground anchor.
        for row in report["sprites"]:
            x, y, width, height = row["box"]
            if (row["anchor"] != [-31, 0] or [width, height] != [64, 31]
                    or x < 0 or y < 0 or x + width > 512 or y + height > 93):
                raise ValueError("native tube corner anchor/box mismatch")
            image = sheet.crop((x, y, x + width, y + height))
            if image.getbbox() is None:
                raise ValueError("empty native tube corner part")
            parts[row["state"], row["direction"], row["part"]] = image
        chords = {"N": [(x, 7) for x in range(17, 46)], "S": [(x, 23) for x in range(17, 46)],
                  "W": [(15, y) for y in range(8, 23)], "E": [(47, y) for y in range(8, 23)]}
        # A source subset recomposes exactly with the whole back. Independent
        # native vehicle path coordinates additionally ensure the foreground
        # cannot cover a capsule through the casing's low end coupling plates.
        for state in tube.STATES:
            for corner in tube.CORNERS:
                back, front = parts[state, corner, "back"], parts[state, corner, "front"]
                far, near = list(back.get_flattened_data()), list(front.get_flattened_data())
                if near == far or any(n != 0 and n != f for f, n in zip(far, near)):
                    raise ValueError("native tube corner foreground changes source pixels")
                if any(front.getpixel(point) != 0 for point in chords[corner]):
                    raise ValueError("native tube corner foreground covers actual capsule path")


def contact_board(report, encoded):
    """Present every native state/view, near rim and actual two-neighbour join.

    @param report Exact source frame mapping for the complete24-pose matrix.
    @param encoded Indexed atlas pixels to display unchanged.
    @return PNG contact board bytes, with nearest-neighbour enlargement only.
    """
    board = Image.new("RGB", (2048, 1240), (48, 52, 58))
    labels = ImageDraw.Draw(board)
    labels.text((14, 12), "NATIVE TUBE CORNERS: complete source states/views/parts; actual track paths remain owned by the engine; packing/runtime pending", fill="white")
    originals = tube.straight_sources(json.loads(tube.RECORD.read_text(encoding="utf-8")))
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Equal panels compare actual corner silhouettes and lifecycle cues.
        # The two touching straight tiles use the same state, pitch and anchors.
        for row, state in enumerate(tube.STATES):
            for column, corner in enumerate(tube.CORNERS):
                left, top = column * 512, row * 400 + 40
                images = {}
                for part in ("back", "front"):
                    sprite = next(s for s in report["sprites"] if (s["state"], s["direction"], s["part"]) == (state, corner, part))
                    x, y, width, height = sprite["box"]
                    images[part] = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
                labels.text((left + 12, top), f"{state} / {corner}: casing4x; foreground2x", fill="white")
                for offset, part, zoom in ((12, "back", 4), (300, "front", 2)):
                    image = images[part].resize((64 * zoom, 31 * zoom), Image.Resampling.NEAREST)
                    board.paste(image, (left + offset, top + 28), image)
                joined = Image.new("RGBA", (192, 96))
                for direction, (dx, dy) in tube.CURVE_NEIGHBOURS[corner]:
                    neighbour = originals[state, direction][2].convert("RGBA")
                    joined.alpha_composite(neighbour, (64 + dx, 32 + dy))
                joined.alpha_composite(images["back"], (64, 32))
                enlarged = joined.resize((384, 192), Image.Resampling.NEAREST)
                board.paste(enlarged, (left + 16, top + 170), enlarged)
                board.paste(images["back"], (left + 425, top + 300), images["back"])
    stream = io.BytesIO()
    board.save(stream, format="PNG")
    return stream.getvalue()


def publish(check=False):
    """Rebuild exact source proof or compare it without modifying any artifact.

    @param check True for read-only regeneration/identity verification.
    @return Source report path; no public contract or packed payload is changed here.
    """
    report, encoded = build()
    validate(report, encoded)
    contact = contact_board(report, encoded)
    contact_path = tube.REVIEW / "tube-curves-contact.png"
    report.update(sha256=hashlib.sha256(encoded).hexdigest(), contact=contact_path.relative_to(tube.ROOT).as_posix(),
                  contact_sha256=hashlib.sha256(contact).hexdigest())
    text = json.dumps(report, indent=2) + "\n"
    atlas = tube.base.repository_path(report["path"])
    if check:
        # Both exact pixels and provenance must regenerate, preventing changed
        # originals/registrations from inheriting an older successful review.
        if atlas.read_bytes() != encoded or contact_path.read_bytes() != contact or TARGET.read_text(encoding="utf-8") != text:
            raise ValueError("native tube corner proof needs deterministic regeneration")
    else:
        tube.REVIEW.mkdir(parents=True, exist_ok=True)
        atlas.write_bytes(encoded)
        contact_path.write_bytes(contact)
        TARGET.write_text(text, encoding="utf-8")
    return TARGET


def bind_source():
    """Bind checked corner source coverage without altering existing packed identities.

    @return None; publish only this family's source/authoring metadata.
    """
    report = json.loads(publish(check=True).read_text(encoding="utf-8"))
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.bend45")
    first = report["sprites"][0]
    family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                        "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_corner_state_part_source_runtime_pending",
                           "source_record": report["source_record"], "originals": list(dict.fromkeys(row["source"] for row in report["sprites"])),
                           "atlas_manifest": TARGET.relative_to(tube.ROOT).as_posix(), "native_atlas": report["path"],
                           "native_sha256": report["sha256"], "staged_pose_count": report["pose_count"],
                           "coverage_complete": True, "runtime_activated": False}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Build or check explicit retained native corner source coverage.

    @return None; print the bound report path, preserving failure diagnostics.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Compare complete source regeneration without writing")
    parser.add_argument("--publish-source", action="store_true", help="Bind complete source coverage after its read-only proof")
    args = parser.parse_args()
    if args.check and args.publish_source:
        # A read-only identity check must never refresh the public contract.
        parser.error("--check cannot be combined with --publish-source")
    print(publish(args.check))
    if args.publish_source:
        bind_source()


if __name__ == "__main__":
    main()
