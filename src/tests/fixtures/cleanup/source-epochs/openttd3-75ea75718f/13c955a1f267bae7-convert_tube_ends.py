"""Register immutable end-cap originals to actual native tube mouth contacts.

@file convert_tube_ends.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47E.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
GetTubeGuidewayEnds owns physical selection; source conversion grants no connection or power.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageChops, ImageDraw

import convert_tube_art as tube


PORTS = {"NE": (47, 8), "SE": (47, 24), "SW": (15, 24), "NW": (15, 8)}
APPROACHES = {"NE": (-32, 16), "SE": (-32, -16), "SW": (32, -16), "NW": (32, 16)}
TARGET = tube.REVIEW / "tube-end-atlas-manifest.json"


def native_end(candidate, frame=None):
    """Resample one retained cap with an explicit source selection and native registration.

    @param candidate Checked immutable end original and optional base transform.
    @param frame Optional declared state/view source crop overriding the base transform.
    @return Indexed64x31 component at anchor(-31,0), without painting or rotating pixels.
    """
    transform = candidate["native_transform"] if frame is None else frame
    # Physical end facings differ from undirected axes. Every actual view
    # keeps the same world canvas/anchor and an explicit retained original.
    if (transform["direction"] not in PORTS or transform["native_size"] != [64, 31]
            or transform["anchor"] != [-31, 0] or transform["resampling"] != "BOX"):
        raise ValueError("unsupported native tube end transform")
    crop, translation = transform["source_crop"], transform["translation"]
    # Only positive virtual canvases and small integer registration are valid;
    # a large shift cannot conceal an oversized stub or another facing.
    if (len(crop) != 4 or crop[2] <= crop[0] or crop[3] <= crop[1]
            or len(translation) != 2 or any(type(value) is not int or abs(value) > 3 for value in translation)):
        raise ValueError("unsupported native tube end crop/translation")
    with Image.open(tube.base.repository_path(candidate["path"])) as original:
        selected = original.convert("RGBA")
        if "source_polygon" in transform:
            mask = Image.new("L", selected.size, 0)
            ImageDraw.Draw(mask).polygon([tuple(point) for point in transform["source_polygon"]], fill=255)
            selected.putalpha(ImageChops.multiply(selected.getchannel("A"), mask))
        selected = selected.crop(tuple(crop)).resize((64, 31), Image.Resampling.BOX)
    aligned = Image.new("RGBA", (64, 31))
    aligned.paste(selected, tuple(translation))
    return tube.base.quantize(aligned, {})


def foreground(whole, direction):
    """Keep the cap's low facing plate after the vehicle layer.

    @param whole Complete native cap; its upper trim remains in the whole back layer.
    @param direction Actual physical end mouth, never a vehicle route/power state.
    @return Exact aligned lower-face source subset; a closed cap may cover its own dead-end mouth.
    """
    _, py = PORTS[direction]
    result = whole.copy()
    # The low pressure face closes a genuine physical end. Its upper trim
    # stays behind any capsule; only the original lower-facing pixels redraw
    # in front. This is presentation and cannot stop an otherwise valid route.
    result.putdata([value if offset // 64 >= py - 2 else 0 for offset, value in enumerate(whole.get_flattened_data())])
    return result


def validate_cap_geometry(whole, direction):
    """Reject a misplaced/oversized cap or the wrong actual isometric face plane.

    @param whole Native indexed original selected for this physical end.
    @param direction Required directed physical mouth in the public end contract.
    @return None; inspect actual alpha geometry rather than trusting a requested caption.
    """
    bounds = whole.getbbox()
    if bounds is None:
        raise ValueError("empty native pressure end cap")
    left, top, right, bottom = bounds
    px, py = PORTS[direction]
    # A local low plate must fit its existing mouth. A long stub, tiny unreadable
    # mark or another port's sprite cannot be registered as an end-cap view.
    if (right - left < 6 or bottom - top < 5 or left < px - 7 or right > px + 8
            or top < py - 8 or bottom > py + 4):
        raise ValueError("native pressure cap leaves its local mouth footprint")
    heights = []
    # The opaque top edge rises/falls along the actual projected cross-section.
    # Median-sized left/right thirds avoid one curved rim pixel deciding the
    # result and catch the repeated-NE-plane error in a claimed SE/NW source.
    for x in range(left, right):
        rows = [y for y in range(top, bottom) if whole.getpixel((x, y)) != 0]
        if rows:
            heights.append((x, rows[0]))
    third = max(1, (right - left) // 3)
    left_heights = [y for x, y in heights if x < left + third]
    right_heights = [y for x, y in heights if x >= right - third]
    if not left_heights or not right_heights:
        # Missing coupling sides cannot establish a real directed pressure face.
        raise ValueError("native pressure cap has incomplete face edges")
    slope = sum(right_heights) / len(right_heights) - sum(left_heights) / len(left_heights)
    expected = 1 if direction in ("NE", "SW") else -1
    if expected * slope <= 0:
        raise ValueError("native pressure cap has the wrong physical face plane")
    if set(whole.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
        raise ValueError("native pressure cap uses a forbidden palette index")


def review_candidate(key):
    """Inspect one explicit end original at native scale and against its existing casing.

    @param key Retained base candidate; no cache/latest-file discovery is allowed.
    @return Bound JSON report path for this base only, without claiming complete source/runtime coverage.
    """
    candidate = tube.retained_candidate(key)
    transform = candidate["native_transform"]
    direction = transform["direction"]
    native = native_end(candidate)
    validate_cap_geometry(native, direction)
    state = transform["state"]
    originals = tube.straight_sources(json.loads(tube.RECORD.read_text(encoding="utf-8")))
    axis = "NE" if direction in ("NE", "SW") else "SE"
    opposite = APPROACHES[direction]
    scene = Image.new("RGBA", (192, 96))
    # One existing full casing and its actual approach retain the fixed32/16
    # world pitch. The cap supplements only the unconnected end mouth.
    casing = originals[state, axis][2].convert("RGBA")
    scene.alpha_composite(casing, (64 + opposite[0], 32 + opposite[1]))
    scene.alpha_composite(casing, (64, 32))
    scene.alpha_composite(native.convert("RGBA"), (64, 32))
    board = Image.new("RGB", (1536, 700), (48, 52, 58))
    labels = ImageDraw.Draw(board)
    labels.text((16, 16), f"PHYSICAL TUBE END {direction}: retained original, native64x31 / anchor(-31,0); packing/runtime pending", fill="white")
    enlarged = native.convert("RGBA").resize((512, 248), Image.Resampling.NEAREST)
    board.paste(enlarged, (16, 70), enlarged)
    enlarged = scene.resize((960, 480), Image.Resampling.NEAREST)
    board.paste(enlarged, (560, 70), enlarged)
    pixels = native.convert("RGBA")
    board.paste(pixels, (16, 380), pixels)
    labels.text((16, 430), "Small local plate only; existing native query selects physical ends independently of power/signal/turn waits", fill="white")
    labels.text((16, 600), "Other actual facings/states and complete aligned part coverage remain required", fill="white")
    tube.REVIEW.mkdir(parents=True, exist_ok=True)
    native_path, contact_path = tube.REVIEW / f"{key}-native.png", tube.REVIEW / f"{key}-contact.png"
    native_path.write_bytes(tube.indexed_png(native))
    board.save(contact_path)
    target = tube.REVIEW / f"{key}-review.json"
    report = {"_documentation": "Plan13 Group47E/master10.4; native end base review only; archi-map-layers.md.",
              "source": candidate["path"], "source_sha256": candidate["sha256"], "native_transform": transform,
              "native": native_path.relative_to(tube.ROOT).as_posix(), "native_sha256": tube.base.digest(native_path),
              "contact": contact_path.relative_to(tube.ROOT).as_posix(), "contact_sha256": tube.base.digest(contact_path),
              "source_coverage_complete": False, "runtime_activated": False}
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def selected_sources(document):
    """Resolve the exact twelve authored physical end/state originals.

    @param document Decoded immutable generation record with explicit frame selections.
    @return State/direction map of original metadata, transform and indexed pixels.
    """
    families = [row for row in document.get("families", []) if row["asset_id"] == "tube.end"]
    if len(families) != 1:
        raise ValueError("missing or duplicate selected tube end family")
    family = families[0]
    identities = [(row["state"], row["direction"]) for row in family["selections"]]
    required = {(state, direction) for state in tube.STATES for direction in PORTS}
    # Every physical mouth needs its own authored lifecycle; duplicate rows
    # cannot hide an absent opposite view or an unpowered model substitution.
    if len(identities) != len(required) or set(identities) != required:
        raise ValueError("incomplete or duplicate native end state coverage")
    result = {}
    # Explicit selections bind actual reviewed geometry, independently of the
    # requested grid captions. No latest-cache discovery or rotated substitute.
    for selection in family["selections"]:
        candidate = tube.retained_candidate(selection["source"], document)
        if candidate["status"].startswith("rejected"):
            raise ValueError("rejected tube end original selected")
        identity = selection["state"], selection["direction"]
        transform = candidate.get("native_transform")
        frame = None
        if "source_frame" in selection:
            matches = [row for row in candidate["frames"] if (row["state"], row["direction"]) == tuple(selection["source_frame"])]
            if len(matches) != 1:
                raise ValueError("missing or duplicate retained native end frame")
            frame = transform = matches[0]
        # State labels and face orientation are both part of the public
        # physical presentation contract; a matching pose count proves neither.
        if transform is None or (transform["state"], transform["direction"]) != identity:
            raise ValueError("native tube end state/view substitution")
        if identity in {(row["state"], row["direction"]) for row in candidate.get("excluded_frames", [])}:
            raise ValueError("excluded native tube end frame selected")
        whole = native_end(candidate, frame)
        validate_cap_geometry(whole, identity[1])
        result[identity] = candidate, transform, whole
    return result


def build(document=None):
    """Build all24 physical mouth/state/part source poses without packing.

    @param document Optional decoded provenance for isolated malformed-input checks.
    @return Exact source report and indexed PNG bytes; originals are unchanged.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    sources = selected_sources(document)
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.end")
    # Caps decorate actual native mouths on the existing tile; changing this
    # matrix or anchor would hide a missing physical facing behind metadata.
    if (family["directions"] != list(PORTS) or family["states"] != list(tube.STATES)
            or family["parts"] != ["back", "front"] or family["frames"] != 1
            or family["anchor"] != [-31, 0] or family["native_size"] != [64, 31]):
        raise ValueError("native tube end public contract mismatch")
    sheet = Image.new("P", (512, 93), 0)
    sheet.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    # Stable public state/direction/part order feeds later source packing.
    # Back retains the complete plate; its low exact subset redraws in front.
    for row, state in enumerate(tube.STATES):
        for column, direction in enumerate(PORTS):
            original, transform, whole = sources[state, direction]
            for part_index, (part, pixels) in enumerate((("back", whole), ("front", foreground(whole, direction)))):
                x, y = (column * 2 + part_index) * 64, row * 31
                sheet.paste(pixels, (x, y))
                sprites.append({"state": state, "direction": direction, "part": part, "frame": 0, "tile_variant": 0,
                                "anchor": [-31, 0], "box": [x, y, 64, 31], "source": original["path"],
                                "source_sha256": original["sha256"], "native_transform": transform})
    report = {"_documentation": "Plan13 Group47E/master10.4; archi-map-layers.md. Complete authored end source; packing/runtime pending.",
              "schema_version": 1, "asset_id": "tube.end", "source_record": tube.RECORD.relative_to(tube.ROOT).as_posix(),
              "path": (tube.REVIEW / "tube-end-authored.png").relative_to(tube.ROOT).as_posix(),
              "native_cell": [64, 31], "size": list(sheet.size), "anchor": [-31, 0], "sprites": sprites,
              "pose_count": len(sprites), "source_coverage_complete": True, "runtime_activated": False,
              "foreground_policy": "whole back plus exact low pressure-face subset; a cap may cover its own physical dead-end mouth"}
    return report, tube.indexed_png(sheet)


def validate(report, encoded, document=None):
    """Check coverage, native geometry and exact retained source/part identities.

    @param report Decoded complete source mapping with24 expected pose identities.
    @param encoded Indexed atlas bytes whose actual pixels must match their originals.
    @param document Optional provenance for independent malformed-input checks.
    @return None; physical closed caps intentionally need no traversable aperture.
    """
    required = {(state, direction, part) for state in tube.STATES for direction in PORTS for part in ("back", "front")}
    identities = [(row["state"], row["direction"], row["part"]) for row in report["sprites"]]
    if len(identities) != len(required) or set(identities) != required:
        # A count alone cannot establish every original cap's aligned front.
        raise ValueError("incomplete native tube end part coverage")
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    sources = selected_sources(document)
    with Image.open(io.BytesIO(encoded)) as sheet:
        # The shared native DOS palette/transparency is part of source handoff;
        # reserved animated colours cannot enter a static pressure plate.
        if (sheet.mode != "P" or sheet.size != (512, 93) or sheet.info.get("transparency") != 0
                or sheet.getpalette()[:768] != tube.base.PALETTE_BYTES):
            raise ValueError("native tube end palette/canvas mismatch")
        if set(sheet.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
            raise ValueError("forbidden native tube end palette index")
        # Bind each exact original/transform and crop so metadata cannot bless
        # pixels from another state, a painted extension or a shifted cap.
        for row in report["sprites"]:
            x, y, width, height = row["box"]
            if (row["anchor"] != [-31, 0] or [width, height] != [64, 31]
                    or x < 0 or y < 0 or x + width > 512 or y + height > 93):
                raise ValueError("native tube end anchor/box mismatch")
            direction = row["direction"]
            original, transform, whole = sources[row["state"], direction]
            if (row["source"] != original["path"] or row["source_sha256"] != original["sha256"]
                    or row["native_transform"] != transform):
                raise ValueError("native tube end false source provenance")
            expected = whole if row["part"] == "back" else foreground(whole, direction)
            actual = sheet.crop((x, y, x + width, y + height))
            if actual.getbbox() is None:
                raise ValueError("empty native tube end part")
            if actual.tobytes() != expected.tobytes():
                raise ValueError("native tube end changes original source pixels")


def contact_board(report, encoded):
    """Compare every pressure face/state against its actual native approach.

    @param report Complete24-pose mapping for the indexed atlas.
    @param encoded Exact unchanged source atlas bytes.
    @return PNG board showing native and enlarged source/foreground/contacts.
    """
    board = Image.new("RGB", (2048, 1240), (48, 52, 58))
    labels = ImageDraw.Draw(board)
    labels.text((14, 12), "PHYSICAL TUBE ENDS: complete source only; native GetTubeGuidewayEnds selects mouths; packing/runtime pending", fill="white")
    originals = tube.straight_sources(json.loads(tube.RECORD.read_text(encoding="utf-8")))
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Equal panels preserve actual physical port positions and native32/16
        # pitch; construction/unpowered art has no connectivity/power authority.
        for row, state in enumerate(tube.STATES):
            for column, direction in enumerate(PORTS):
                left, top = column * 512, row * 400 + 40
                images = {}
                for part in ("back", "front"):
                    sprite = next(s for s in report["sprites"] if (s["state"], s["direction"], s["part"]) == (state, direction, part))
                    x, y, width, height = sprite["box"]
                    images[part] = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
                labels.text((left + 12, top), f"{state} / {direction}: cap4x; low front2x", fill="white")
                for offset, part, zoom in ((12, "back", 4), (300, "front", 2)):
                    image = images[part].resize((64 * zoom, 31 * zoom), Image.Resampling.NEAREST)
                    board.paste(image, (left + offset, top + 28), image)
                joined = Image.new("RGBA", (192, 96))
                dx, dy = APPROACHES[direction]
                axis = "NE" if direction in ("NE", "SW") else "SE"
                casing = originals[state, axis][2].convert("RGBA")
                joined.alpha_composite(casing, (64 + dx, 32 + dy))
                joined.alpha_composite(casing, (64, 32))
                joined.alpha_composite(images["back"], (64, 32))
                enlarged = joined.resize((384, 192), Image.Resampling.NEAREST)
                board.paste(enlarged, (left + 16, top + 170), enlarged)
                board.paste(images["back"], (left + 425, top + 300), images["back"])
    stream = io.BytesIO()
    board.save(stream, format="PNG")
    return stream.getvalue()


def publish(check=False):
    """Rebuild exact source proof or compare it without writing.

    @param check True for read-only source/contact/provenance identity verification.
    @return Bound complete source report path; no packed asset is modified.
    """
    report, encoded = build()
    validate(report, encoded)
    contact = contact_board(report, encoded)
    contact_path = tube.REVIEW / "tube-end-contact.png"
    report.update(sha256=hashlib.sha256(encoded).hexdigest(), contact=contact_path.relative_to(tube.ROOT).as_posix(),
                  contact_sha256=hashlib.sha256(contact).hexdigest())
    text = json.dumps(report, indent=2) + "\n"
    atlas = tube.base.repository_path(report["path"])
    if check:
        # Exact regeneration prevents a changed original/registration from
        # inheriting a previous passing contact or provenance review.
        if atlas.read_bytes() != encoded or contact_path.read_bytes() != contact or TARGET.read_text(encoding="utf-8") != text:
            raise ValueError("native tube end proof needs deterministic regeneration")
    else:
        tube.REVIEW.mkdir(parents=True, exist_ok=True)
        atlas.write_bytes(encoded)
        contact_path.write_bytes(contact)
        TARGET.write_text(text, encoding="utf-8")
    return TARGET


def bind_source():
    """Publish complete authored source while preserving every packed identity.

    @return None; change only the end family's source/authoring metadata.
    """
    report = json.loads(publish(check=True).read_text(encoding="utf-8"))
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.end")
    first = report["sprites"][0]
    family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                        "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_end_state_part_source_runtime_pending",
                           "source_record": report["source_record"], "originals": list(dict.fromkeys(row["source"] for row in report["sprites"])),
                           "atlas_manifest": TARGET.relative_to(tube.ROOT).as_posix(), "native_atlas": report["path"],
                           "native_sha256": report["sha256"], "staged_pose_count": report["pose_count"],
                           "coverage_complete": True, "runtime_activated": False}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Review one explicit original or build/check the complete end source family.

    @return None; print the exact bound report path and preserve failures.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", help="Review one explicitly named original only")
    parser.add_argument("--check", action="store_true", help="Read-only complete source regeneration")
    parser.add_argument("--publish-source", action="store_true", help="Bind checked complete source without packing")
    args = parser.parse_args()
    # Candidate review and full source publication have different completeness
    # claims; a read-only invocation must never refresh public source metadata.
    if (args.check and args.publish_source) or (args.candidate and (args.check or args.publish_source)):
        parser.error("candidate/check/publication modes cannot be combined")
    print(review_candidate(args.candidate) if args.candidate else publish(args.check))
    if args.publish_source:
        bind_source()


if __name__ == "__main__":
    main()
