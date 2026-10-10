"""Review retained tube originals through the existing deterministic DOS conversion.

@file convert_tube_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups47B-47C.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.4.
Conversion changes presentation only; no track, service or quantity is created.
"""

import argparse
import io
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

import convert_m1_art as base


ROOT = Path(__file__).resolve().parents[2]
RECORD = ROOT / "media/multilayer/source/ground/tube-generation-record.json"
REVIEW = ROOT / "logs/proofs/mlm/native-art/ground"
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
STATES = ("construction", "powered", "unpowered")
AXES = {"NE": "X", "SE": "Y"}
CORNERS = ("N", "S", "W", "E")
CURVE_NEIGHBOURS = {
    "N": (("SE", (-32, -16)), ("NE", (32, -16))),
    "S": (("NE", (-32, 16)), ("SE", (32, 16))),
    "W": (("SE", (-32, -16)), ("NE", (-32, 16))),
    "E": (("NE", (32, -16)), ("SE", (32, 16))),
}


def retained_candidate(key, document=None):
    """Resolve an immutable original and every declared input before conversion.

    @param key Explicit retained candidate; never choose the newest cache file.
    @param document Optional already decoded provenance, including independently checked selections.
    @return Candidate metadata with checked source/reference byte identities.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    matches = [row for row in document["candidates"] if row["key"] == key]
    if len(matches) != 1:
        raise ValueError("missing or duplicate retained tube candidate")
    candidate = matches[0]
    if candidate["path"] is None:
        raise ValueError("tube candidate has no retained original")
    # A source or reference change requires an explicit sibling revision;
    # silently rebaking different pixels would invalidate earlier selection.
    for item in [candidate] + candidate["references"]:
        if base.digest(base.repository_path(item["path"])) != item["sha256"]:
            raise ValueError(f"immutable tube source/reference changed: {item['path']}")
    return candidate


def native_base(candidate, frame=None):
    """Resample a selected source canvas without inventing geometry or states.

    @param candidate Checked retained powered straight original and declared native transform.
    @param frame Optional explicit state-sheet selection polygon, crop and native transform.
    @return Indexed64x31 review sprite with declared anchor(-31,0).
    """
    transform = candidate["native_transform"] if frame is None else frame
    # Whole-canvas translations align the independently authored mouth
    # centers to the original native axes. An out-of-contract transform
    # cannot disguise a different footprint or an unusable source silhouette.
    if (transform["native_size"] != [64, 31] or transform["anchor"] != [-31, 0]
            or transform["axis"] not in ("X", "Y") or transform["resampling"] != "BOX"):
        raise ValueError("unsupported native straight transform")
    translation = transform["translation"]
    # Only a small integer registration is allowed: a changed source must not
    # escape its native canvas or hide a different model behind a large shift.
    if len(translation) != 2 or any(type(value) is not int or abs(value) > 3 for value in translation):
        raise ValueError("unsupported native straight translation")
    with Image.open(base.repository_path(candidate["path"])) as original:
        selected = original.convert("RGBA")
        if frame is not None:
            polygon, crop = frame["source_polygon"], frame["source_crop"]
            # Explicit disjoint regions recover independently authored sheet
            # poses whose transparent gutters are diagonal, rather than
            # clipping a mouth at an assumed rectangular grid boundary.
            if len(polygon) < 3 or len(crop) != 4 or crop[2] <= crop[0] or crop[3] <= crop[1]:
                raise ValueError("invalid tube source selection")
            mask = Image.new("L", selected.size, 0)
            ImageDraw.Draw(mask).polygon([tuple(point) for point in polygon], fill=255)
            selected.putalpha(ImageChops.multiply(selected.getchannel("A"), mask))
            # A virtual canvas may extend into transparent padding outside the
            # source sheet. Pillow supplies alpha zero there; no pixels are drawn.
            selected = selected.crop(tuple(crop))
        scaled = selected.resize((64, 31), Image.Resampling.BOX)
    # The recorded translation preserves both source ends and
    # alpha; it neither draws replacement shells nor rotates another view.
    aligned = Image.new("RGBA", (64, 31))
    aligned.paste(scaled, tuple(transform["translation"]))
    return base.quantize(aligned, {})


def indexed_png(image):
    """Encode the shared palette and transparent index reproducibly.

    @param image Native indexed tube canvas or atlas.
    @return Exact PNG bytes suitable for later packing and identity checks.
    """
    stream = io.BytesIO()
    image.save(stream, format="PNG", transparency=0)
    return stream.getvalue()


def native_curve(candidate, frame=None):
    """Resample a retained native corner without changing its authored pixels.

    @param candidate Checked original with explicit native registration.
    @param frame Optional state/view source selection overriding the original's base transform.
    @return Native indexed64x31 corner with anchor(-31,0).
    """
    transform = candidate["native_transform"] if frame is None else frame
    # A native corner remains one installed Upper/Lower/Left/Right piece.
    # Only declared crops and small translations register it to that tile;
    # a rotation, altered anchor or changed size cannot stand in for a view.
    if (transform["corner"] not in CORNERS or transform["native_size"] != [64, 31]
            or transform["anchor"] != [-31, 0] or transform["resampling"] != "BOX"):
        raise ValueError("unsupported native tube corner transform")
    crop, translation = transform["source_crop"], transform["translation"]
    if len(crop) != 4 or crop[2] <= crop[0] or crop[3] <= crop[1]:
        # Explicit alpha padding is allowed, but an empty/inverted virtual
        # canvas cannot align the two physical mouths of a native corner.
        raise ValueError("invalid native tube corner crop")
    if len(translation) != 2 or any(type(value) is not int or abs(value) > 3 for value in translation):
        # Large shifts could hide a missing mouth instead of registering it.
        raise ValueError("unsupported native tube corner translation")
    with Image.open(base.repository_path(candidate["path"])) as original:
        selected = original.convert("RGBA")
        if "source_polygon" in transform:
            mask = Image.new("L", selected.size, 0)
            ImageDraw.Draw(mask).polygon([tuple(point) for point in transform["source_polygon"]], fill=255)
            selected.putalpha(ImageChops.multiply(selected.getchannel("A"), mask))
        selected = selected.crop(tuple(crop)).resize((64, 31), Image.Resampling.BOX)
    canvas = Image.new("RGBA", (64, 31))
    canvas.paste(selected, tuple(translation))
    return base.quantize(canvas, {})


def review_curve_candidate(key, selected_corner=None):
    """Review a retained corner against both of its actual adjacent straight axes.

    @param key Explicit retained base candidate; no cache or implicit view discovery.
    @param selected_corner Optional explicit powered corner within a retained multi-view sheet.
    @return JSON report path binding native pixels and the neighbour contact board.
    """
    candidate = retained_candidate(key)
    frame = None
    if selected_corner is not None:
        frames = [row for row in candidate["frames"] if (row["corner"], row["state"]) == (selected_corner, "powered")]
        if len(frames) != 1:
            raise ValueError("missing or duplicate retained powered tube corner")
        frame = frames[0]
    transform = candidate["native_transform"] if frame is None else frame
    corner = transform["corner"]
    native = native_curve(candidate, frame)
    originals = {"NE": "tube-straight-base-v1", "SE": "tube-straight-y-base-v1"}
    joined = Image.new("RGBA", (192, 96))
    # Opposite edge mouths meet at the native32/16 tile pitch. Each corner
    # touches both original straight axes, exposing incorrect end fittings
    # rather than judging the isolated casing by its enlarged appearance.
    for direction, (dx, dy) in CURVE_NEIGHBOURS[corner]:
        neighbour = native_base(retained_candidate(originals[direction])).convert("RGBA")
        joined.alpha_composite(neighbour, (64 + dx, 32 + dy))
    joined.alpha_composite(native.convert("RGBA"), (64, 32))
    board = Image.new("RGB", (1536, 700), (48, 52, 58))
    labels = ImageDraw.Draw(board)
    labels.text((16, 16), f"NATIVE TUBE CORNER {corner}: retained original, palette registration and two actual straight neighbours", fill="white")
    enlarged = native.convert("RGBA").resize((512, 248), Image.Resampling.NEAREST)
    board.paste(enlarged, (16, 70), enlarged)
    enlarged = joined.resize((960, 480), Image.Resampling.NEAREST)
    board.paste(enlarged, (560, 70), enlarged)
    pixels = native.convert("RGBA")
    board.paste(pixels, (16, 380), pixels)
    labels.text((16, 430), "Native64x31 above; anchor(-31,0); fixed top-left light; source coverage and runtime acceptance remain separate", fill="white")
    labels.text((16, 600), "Other corners/lifecycle states/foreground parts require their own source mapping before family closure", fill="white")
    REVIEW.mkdir(parents=True, exist_ok=True)
    suffix = "" if frame is None else f"-{corner}"
    native_path, contact_path = REVIEW / f"{key}{suffix}-native.png", REVIEW / f"{key}{suffix}-contact.png"
    native_path.write_bytes(indexed_png(native))
    board.save(contact_path)
    report = {"_documentation": "Plan13 Group47C / master10.4; native corner base review, not complete packing/runtime acceptance.",
              "source": candidate["path"], "source_sha256": candidate["sha256"], "native_transform": transform,
              "native": native_path.relative_to(ROOT).as_posix(), "native_sha256": base.digest(native_path),
              "contact": contact_path.relative_to(ROOT).as_posix(), "contact_sha256": base.digest(contact_path),
              "source_coverage_complete": False, "runtime_activated": False}
    target = REVIEW / f"{key}{suffix}-review.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def foreground(whole, axis):
    """Keep only the near rim for drawing after an actual moving capsule.

    @param whole Complete native indexed casing; its pixels remain unchanged.
    @param axis Native undirected X or Y track axis, with fixed engine projection.
    @return Aligned transparent near-rim subset; the back sprite retains the whole casing.
    """
    front = whole.copy()
    pixels = []
    # Native edge contacts define the centre line. The screen-lower side is
    # the foreground rim for either axis; retaining exact source indices makes
    # back -> capsule -> front composition independent of power accounting.
    for index, value in enumerate(whole.get_flattened_data()):
        x, y = index % 64, index // 64
        line = 31.5 - x * 0.5 if axis == "X" else 0.5 + x * 0.5
        pixels.append(value if y > line else 0)
    front.putdata(pixels)
    return front


def curve_foreground(whole, corner):
    """Keep low facing rims while leaving the actual moving capsule chord clear.

    @param whole Complete authored corner; end coupling plates remain in its back layer.
    @param corner Native N/S/W/E corner identity, never a vehicle travel heading.
    @return Exact nonempty source subset for drawing after the capsule.
    """
    result = whole.copy()
    pixels = []
    # Horizontal corners have one screen-lower facing rim. Vertical corners
    # have two visible flanks at equal projected depth; both low facing sides
    # frame the capsule. Their cross-coupling plates stay beneath the vehicle,
    # and the foreground aperture follows the native chord through both mouths.
    for index, value in enumerate(whole.get_flattened_data()):
        x, y = index % 64, index // 64
        if corner in ("N", "S"):
            chord = 7 if corner == "N" else 23
            keep = y > chord + 1
        else:
            chord = 15 if corner == "W" else 47
            keep = abs(x - chord) > 1
        pixels.append(value if keep else 0)
    result.putdata(pixels)
    return result


def straight_sources(document):
    """Require one real original for every selected straight state and axis.

    @param document Decoded source provenance and explicit family frame selections.
    @return Checked map from state/direction to original, source selection and native pixels.
    """
    families = [row for row in document.get("families", []) if row["asset_id"] == "tube.straight"]
    if len(families) != 1:
        raise ValueError("missing or duplicate selected straight family")
    selections = families[0]["selections"]
    required = {(state, direction) for state in STATES for direction in AXES}
    identities = [(row["state"], row["direction"]) for row in selections]
    # An authored status cannot replace a missing pose or allow two originals
    # to compete for the same lifecycle/axis in later deterministic packing.
    if len(identities) != len(set(identities)) or set(identities) != required:
        raise ValueError("incomplete or duplicate tube state/axis coverage")
    result = {}
    # Bind each public lifecycle/axis to one retained original and its reviewed
    # selection. Later packing never discovers a convenient substitute frame.
    for selection in selections:
        candidate = retained_candidate(selection["source"], document)
        identity = selection["state"], selection["direction"]
        frame = None
        if "source_frame" in selection:
            matches = [row for row in candidate["frames"]
                       if (row["state"], row["direction"]) == tuple(selection["source_frame"])]
            if len(matches) != 1:
                raise ValueError("missing or duplicate retained tube frame")
            frame = matches[0]
        transform = candidate["native_transform"] if frame is None else frame
        # Reusing a powered frame as an unpowered/construction pose would hide
        # a missing authored state. A reversed traversal alone may reuse an axis.
        source_identity = ("powered", "NE" if transform["axis"] == "X" else "SE") if frame is None else (frame["state"], frame["direction"])
        if source_identity != identity or transform["axis"] != AXES[identity[1]]:
            raise ValueError("tube frame substituted across a state or axis")
        pixels = native_base(candidate, frame)
        if pixels.getbbox() is None:
            raise ValueError("empty authored tube casing")
        result[identity] = candidate, frame, pixels
    return result


def build_straight(document=None):
    """Build twelve source poses and their complete retained-original mapping.

    @param document Optional provenance for focused refusal tests; defaults to the retained record.
    @return Report and exact encoded atlas; no files or runtime IDs are changed.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    sources = straight_sources(document)
    manifest = json.loads(PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in manifest["assets"] if row["asset_id"] == "tube.straight")
    # The authoring matrix must match its public consumer contract, including
    # all parts. It cannot silently publish a partial atlas as a full family.
    if (family["directions"] != list(AXES) or family["states"] != list(STATES)
            or family["parts"] != ["back", "front"] or family["frames"] != 1
            or family["native_size"] != [64, 31] or family["anchor"] != [-31, 0]):
        raise ValueError("straight tube public pose/geometry contract mismatch")
    sheet = Image.new("P", (256, 93), 0)
    sheet.putpalette(base.PALETTE_BYTES)
    sprites = []
    # Stable public state/view/part order is the future packer's input. Each
    # part derives from the same checked whole frame, with identical anchors.
    for row, state in enumerate(STATES):
        for column, direction in enumerate(AXES):
            candidate, frame, whole = sources[state, direction]
            front = foreground(whole, AXES[direction])
            for part_index, (part, pixels) in enumerate((("back", whole), ("front", front))):
                x, y = (column * 2 + part_index) * 64, row * 31
                sheet.paste(pixels, (x, y))
                sprites.append({"state": state, "direction": direction, "frame": 0, "part": part,
                                "anchor": [-31, 0], "box": [x, y, 64, 31], "source": candidate["path"],
                                "source_sha256": candidate["sha256"], "source_frame": None if frame is None else [state, direction],
                                "native_transform": candidate["native_transform"] if frame is None else frame})
    report = {"_documentation": "Plan13 Group47B / master10.4; archi-map-layers.md. Complete straight source art; packing/runtime acceptance pending.",
              "schema_version": 1, "asset_id": "tube.straight", "source_record": RECORD.relative_to(ROOT).as_posix(),
              "path": (REVIEW / "tube-straight-authored.png").relative_to(ROOT).as_posix(),
              "native_cell": [64, 31], "size": list(sheet.size), "anchor": [-31, 0], "sprites": sprites,
              "foreground_policy": "back=whole authored casing; front=exact near-rim subset below native axis centreline",
              "pose_count": len(sprites), "source_coverage_complete": True, "runtime_activated": False}
    return report, indexed_png(sheet)


def validate_straight(report, encoded):
    """Check complete indexed coverage and exact back/front recomposition.

    @param report Source pose mapping generated from retained originals.
    @param encoded Encoded indexed native source atlas to validate independently.
    @return None; reject incomplete parts, anchors, palette or changed casing pixels.
    """
    expected = {(state, direction, part) for state in STATES for direction in AXES for part in ("back", "front")}
    identities = [(row["state"], row["direction"], row["part"]) for row in report["sprites"]]
    # Every authored pose must exist once; a correct total alone could still
    # conceal a duplicated front rim or an absent unpowered view.
    if set(identities) != expected or len(identities) != len(expected):
        raise ValueError("straight tube atlas has incomplete part coverage")
    with Image.open(io.BytesIO(encoded)) as sheet:
        if sheet.mode != "P" or sheet.size != (256, 93) or sheet.info.get("transparency") != 0:
            raise ValueError("straight tube atlas palette/canvas/transparency mismatch")
        if sheet.getpalette()[:768] != base.PALETTE_BYTES:
            raise ValueError("straight tube atlas DOS palette mismatch")
        if set(sheet.get_flattened_data()) & (base.FORBIDDEN - {0}):
            raise ValueError("straight tube atlas contains forbidden palette indices")
        frames = {}
        for sprite in report["sprites"]:
            x, y, width, height = sprite["box"]
            # An anchor/crop mismatch would make the foreground drift from its
            # casing and the native neighbours even when every pose is present.
            if sprite["anchor"] != [-31, 0] or [width, height] != [64, 31] or x < 0 or y < 0 or x + width > 256 or y + height > 93:
                raise ValueError("straight tube atlas anchor/box mismatch")
            pixels = sheet.crop((x, y, x + width, y + height))
            if pixels.getbbox() is None:
                raise ValueError("empty straight tube atlas part")
            frames[sprite["state"], sprite["direction"], sprite["part"]] = pixels
        # Front pixels are an exact, nonempty proper subset of the retained
        # casing. Compositing both parts without a vehicle must recover it.
        for state in STATES:
            for direction in AXES:
                back = list(frames[state, direction, "back"].get_flattened_data())
                front = list(frames[state, direction, "front"].get_flattened_data())
                if front == back or any(near != 0 and near != far for far, near in zip(back, front)):
                    raise ValueError("straight tube foreground alters the original casing")


def straight_review(report, encoded):
    """Arrange all native states, foreground subsets and actual tile joins.

    @param report Complete straight source mapping; no gameplay state is inferred from it.
    @param encoded Exact native indexed atlas whose pixels are presented unchanged.
    @return Encoded diagnostic contact board with nearest-neighbour enlargement.
    """
    board = Image.new("RGB", (1800, 1160), (48, 52, 58))
    labels = ImageDraw.Draw(board)
    labels.text((16, 12), "STRAIGHT TUBE SOURCE REVIEW: native64x31 / anchor(-31,0); packing and runtime integration pending", fill="white")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Compare the same state on both independently authored axes. The
        # foreground subset and three-tile run expose occlusion and seam drift
        # that a large isolated original would conceal.
        for row, state in enumerate(STATES):
            for column, direction in enumerate(AXES):
                left, top = column * 900, row * 370 + 40
                images = {}
                for part in ("back", "front"):
                    sprite = next(item for item in report["sprites"] if (item["state"], item["direction"], item["part"]) == (state, direction, part))
                    x, y, width, height = sprite["box"]
                    images[part] = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
                labels.text((left + 14, top), f"{state} / {AXES[direction]} / {direction}: complete casing5x and exact near rim5x", fill="white")
                for offset, part in ((14, "back"), (370, "front")):
                    enlarged = images[part].resize((320, 155), Image.Resampling.NEAREST)
                    board.paste(enlarged, (left + offset, top + 30), enlarged)
                joined = Image.new("RGBA", (128, 63))
                positions = ((64, 0), (32, 16), (0, 32)) if direction == "NE" else ((0, 0), (32, 16), (64, 32))
                for position in positions:
                    joined.alpha_composite(images["back"], position)
                enlarged = joined.resize((384, 189), Image.Resampling.NEAREST)
                board.paste(enlarged, (left + 14, top + 170), enlarged)
                board.paste(images["back"], (left + 440, top + 230), images["back"])
                labels.text((left + 440, top + 280), "Native above; three actual world tiles at left", fill="white")
    stream = io.BytesIO()
    board.save(stream, format="PNG")
    return stream.getvalue()


def publish_straight(check=False):
    """Publish reproducible source art or compare it without replacing files.

    @param check True for a read-only source/artifact identity verification.
    @return Source report path; runtime packing and original sources are untouched.
    """
    import hashlib
    report, encoded = build_straight()
    validate_straight(report, encoded)
    report["sha256"] = hashlib.sha256(encoded).hexdigest()
    contact = straight_review(report, encoded)
    contact_path = REVIEW / "tube-straight-states-contact.png"
    report["contact"] = contact_path.relative_to(ROOT).as_posix()
    report["contact_sha256"] = hashlib.sha256(contact).hexdigest()
    target = REVIEW / "tube-atlas-manifest.json"
    text = json.dumps(report, indent=2) + "\n"
    atlas = base.repository_path(report["path"])
    if check:
        # Regeneration must match both pixels and provenance, so a changed
        # source/selection cannot inherit an older successful review silently.
        if atlas.read_bytes() != encoded or contact_path.read_bytes() != contact or target.read_text(encoding="utf-8") != text:
            raise ValueError("straight tube source proof needs deterministic regeneration")
    else:
        REVIEW.mkdir(parents=True, exist_ok=True)
        atlas.write_bytes(encoded)
        contact_path.write_bytes(contact)
        target.write_text(text, encoding="utf-8")
    return target


def bind_straight_source():
    """Publish complete authored source coverage while preserving packed identities.

    @return None; update only the straight family's source/authoring metadata.
    """
    target = publish_straight(check=True)
    report = json.loads(target.read_text(encoding="utf-8"))
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.straight")
    originals = list(dict.fromkeys(row["source"] for row in report["sprites"]))
    first = report["sprites"][0]
    family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                        "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_axis_state_part_source_runtime_pending",
                           "source_record": report["source_record"], "originals": originals,
                           "atlas_manifest": target.relative_to(ROOT).as_posix(), "native_atlas": report["path"],
                           "native_sha256": report["sha256"], "staged_pose_count": report["pose_count"],
                           "coverage_complete": True, "runtime_activated": False}
    PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def review_candidate(key):
    """Publish native/enlarged silhouette and repeated actual tile-contact review.

    @param key Explicit retained original; its declared native axis controls tile pitch.
    @return Report path binding the retained original and deterministic previews.
    """
    candidate = retained_candidate(key)
    transform = candidate["native_transform"]
    native = native_base(candidate)
    REVIEW.mkdir(parents=True, exist_ok=True)
    native_path = REVIEW / f"{key}-native.png"
    native.save(native_path, transparency=0)
    pixels = native.convert("RGBA")
    joined = Image.new("RGBA", (128, 63))
    # Native X tiles advance SW(-32,+16), Y tiles SE(+32,+16). This review
    # exposes seam/port drift that fitting one isolated silhouette can hide.
    positions = ((64, 0), (32, 16), (0, 32)) if transform["axis"] == "X" else ((0, 0), (32, 16), (64, 32))
    for left, top in positions:
        joined.alpha_composite(pixels, (left, top))
    sheet = Image.new("RGB", (1536, 640), (48, 52, 58))
    labels = ImageDraw.Draw(sheet)
    labels.text((16, 16), "RETAINED TUBE BASE: deterministic palette/anchor review; runtime integration pending", fill="white")
    sheet.paste(pixels.resize((512, 248), Image.Resampling.NEAREST), (16, 72), pixels.resize((512, 248), Image.Resampling.NEAREST))
    enlarged = joined.resize((896, 441), Image.Resampling.NEAREST)
    sheet.paste(enlarged, (600, 72), enlarged)
    sheet.paste(pixels, (16, 360), pixels)
    labels.text((16, 42), "One native64x31 sprite enlarged8x; opaque floor/track is absent", fill="white")
    labels.text((600, 42), f"Three native {transform['axis']} tiles at their actual world pitch; inspect matching mouths", fill="white")
    labels.text((16, 410), f"Native pixels above; anchor(-31,0); full original -> BOX64x31 -> shift{transform['translation']} -> shared DOS quantizer", fill="white")
    labels.text((16, 560), "Single selected powered original. Complete state/axis/part coverage is reported separately by --straight.", fill="white")
    sheet_path = REVIEW / f"{key}-contact.png"
    sheet.save(sheet_path)
    report = {"_documentation": "Plan13 Group47B / master10.4; archi-map-layers.md. Base-source review, not packed/runtime acceptance.",
              "source": candidate["path"], "source_sha256": candidate["sha256"], "native_size": [64, 31], "anchor": [-31, 0],
              "resampling": "BOX", "translation": transform["translation"], "axis": transform["axis"], "native": native_path.relative_to(ROOT).as_posix(),
              "native_sha256": base.digest(native_path), "contact": sheet_path.relative_to(ROOT).as_posix(),
              "contact_sha256": base.digest(sheet_path), "opaque_bounds": native.getbbox(),
              "forbidden_indices": sorted(set(native.get_flattened_data()) & (base.FORBIDDEN - {0})),
              "source_coverage_complete": False, "runtime_activated": False}
    target = REVIEW / f"{key}-review.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Review an explicitly selected source without discovering cache revisions.

    @return None; print the bound report path and retain failure diagnostics.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", default="tube-straight-base-v1")
    parser.add_argument("--straight", action="store_true", help="Build the selected twelve-pose straight source atlas")
    parser.add_argument("--check", action="store_true", help="Compare complete straight source proof without writing")
    parser.add_argument("--publish-source", action="store_true", help="Bind checked straight source coverage in the public manifest")
    parser.add_argument("--curve-candidate", help="Review one retained native Upper/Lower/Left/Right base original")
    parser.add_argument("--corner", choices=CORNERS, help="Select an explicit powered corner from a retained multi-view source")
    args = parser.parse_args()
    # A corner preview is a write operation separate from a straight-family
    # read-only check/publication; contradictory modes must never pick a branch
    # that silently refreshes artifacts during --check.
    if args.curve_candidate is not None and (args.straight or args.check or args.publish_source):
        parser.error("--curve-candidate cannot be combined with straight-family options")
    if args.corner is not None and args.curve_candidate is None:
        # A view label cannot discover an unspecified original implicitly.
        parser.error("--corner requires --curve-candidate")
    if args.check and not args.straight:
        # Read-only comparison belongs to complete family coverage; it cannot
        # accidentally refresh a single-base preview and appear to validate it.
        parser.error("--check requires --straight")
    # Publishing is an explicit source-contract update after a complete proof;
    # a read-only check or a single-base preview must never write that contract.
    if args.publish_source and (not args.straight or args.check):
        parser.error("--publish-source requires --straight without --check")
    if args.curve_candidate is not None:
        print(review_curve_candidate(args.curve_candidate, args.corner))
    else:
        print(publish_straight(args.check) if args.straight else review_candidate(args.candidate))
    if args.publish_source:
        bind_straight_source()


if __name__ == "__main__":
    main()
