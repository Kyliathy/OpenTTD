"""Select floor-level cable branches from immutable authored native axis originals.

@file convert_tube_conduits.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47G.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
TubePowerConnection owns physical paths; GetUtilityPowerProducer owns electrical status.
No pixel creates a corridor, power account, grant or foreground obstacle.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube


PORTS = {"NE": (47, 8), "SE": (47, 24), "SW": (15, 24), "NW": (15, 8)}
AXES = {"NE": "X", "SE": "Y", "SW": "X", "NW": "Y"}
TARGET = tube.REVIEW / "tube-conduit-atlas-manifest.json"


def validate_axis(whole, axis):
    """Reject an off-axis, oversized or incomplete floor cable original.

    @param whole Indexed native64x31 full cable at the common ground anchor.
    @param axis Actual independently authored X or Y geometry.
    @return None; floor cable cannot substitute a tall wall or another orientation.
    """
    bounds = whole.getbbox()
    if bounds is None:
        raise ValueError("empty native conduit original")
    left, top, right, bottom = bounds
    # Actual service corridors meet tile edges through a thin floor cable;
    # a slab/large machine or shifted short mark cannot represent that path.
    if (axis not in ("X", "Y") or left < 13 or right > 50 or top < 5 or bottom > 27
            or right - left < 30 or bottom - top < 14):
        raise ValueError("native conduit leaves its floor contact footprint")
    if set(whole.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
        raise ValueError("forbidden native conduit palette index")
    # The full authored cable stays within three pixels of the actual native
    # centerline. This catches wrong slope/views without trusting source labels.
    for offset, value in enumerate(whole.get_flattened_data()):
        x, y = offset % 64, offset // 64
        residual = abs(2 * y + x - 63) if axis == "X" else abs(2 * y - x - 1)
        if value != 0 and residual > 6:
            raise ValueError("native conduit has wrong axis or a tall floor obstruction")
    contacts = ((47, 8), (15, 24)) if axis == "X" else ((15, 8), (47, 24))
    # Couplings and the small central sleeve must survive source resampling;
    # a missing original end cannot be painted in by the branch selector.
    for px, py in (*contacts, (31, 16)):
        if not any(whole.getpixel((x, y)) != 0 for y in range(py - 1, py + 2) for x in range(px - 1, px + 2)):
            raise ValueError("native conduit has a missing floor contact")


def branch(whole, direction):
    """Select one actual center-to-edge half without drawing replacement pixels.

    @param whole Complete original native cable with both opposing physical ends.
    @param direction Actual saved path's next/previous physical edge.
    @return Exact original subset; central sleeve is shared by overlapping branches.
    """
    if direction not in PORTS:
        raise ValueError("unknown native conduit branch")
    result = whole.copy()
    pixels = []
    # Every corridor tile presents only its actual adjacent path cells. The
    # compact common sleeve remains complete; the opposite uninstalled arm
    # is removed without rotating, shifting or recolouring the retained source.
    for offset, value in enumerate(whole.get_flattened_data()):
        x, y = offset % 64, offset // 64
        central = 29 <= x <= 33 and 14 <= y <= 18
        selected = x >= 31 if direction in ("NE", "SE") else x <= 31
        pixels.append(value if central or selected else 0)
    result.putdata(pixels)
    return result


def sources(document):
    """Bind six real state/axis originals before selecting the twelve path halves.

    @param document Decoded immutable generation record with explicit source frames.
    @return State/axis map of checked original metadata, native transform and pixels.
    """
    families = [row for row in document["families"] if row["asset_id"] == "tube.conduit"]
    if len(families) != 1:
        raise ValueError("missing or duplicate native conduit family")
    family = families[0]
    # Native service cables are floor overlays. A false shell/foreground or
    # altered anchor would hide capsule occupancy or break neighbour contacts.
    if (family["directions"] != list(PORTS) or family["states"] != list(tube.STATES)
            or family["parts"] != ["body"] or family["native_size"] != [64, 31]
            or family["anchor"] != [-31, 0] or family["center_pixel"] != [31, 16]):
        raise ValueError("native conduit branch/ground contract mismatch")
    required = {(state, axis) for state in tube.STATES for axis in ("X", "Y")}
    identities = [(row["state"], row["axis"]) for row in family["axis_selections"]]
    if len(identities) != len(required) or set(identities) != required:
        # A duplicate source cannot hide a missing independently authored axis.
        raise ValueError("incomplete native conduit state/axis coverage")
    result = {}
    # Actual retained geometry and lifecycle must agree with each selection;
    # a requested caption or powered recolour is not an authored missing state.
    for selection in family["axis_selections"]:
        original = tube.retained_candidate(selection["source"], document)
        if original["status"].startswith("rejected"):
            raise ValueError("rejected native conduit original")
        transform = original.get("native_transform")
        identity = selection["state"], selection["axis"]
        if "source_frame" in selection:
            matches = [row for row in original["frames"] if (row["state"], row["axis"]) == tuple(selection["source_frame"])]
            if len(matches) != 1:
                raise ValueError("missing or duplicate native conduit original frame")
            transform = matches[0]
        if transform is None or (transform["state"], transform["axis"]) != identity:
            raise ValueError("native conduit state/axis substitution")
        whole = tube.native_base(original, transform)
        validate_axis(whole, identity[1])
        result[identity] = original, transform, whole
    return result


def build(document=None):
    """Build all twelve exact source branch/state bodies without packing.

    @param document Optional isolated decoded provenance for refusal checks.
    @return Complete source mapping and indexed PNG bytes; no gameplay owner changes.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    originals = sources(document)
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.conduit")
    # The real corridor geometry, ground draw order and shared native anchor
    # are authoritative, independent of camera, capsule heading and track mask.
    if (family["directions"] != list(PORTS) or family["states"] != list(tube.STATES)
            or family["parts"] != ["body"] or family["frames"] != 1
            or family["native_size"] != [64, 31] or family["anchor"] != [-31, 0]):
        raise ValueError("native conduit public geometry mismatch")
    atlas = Image.new("P", (256, 93), 0)
    atlas.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    # Stable state/physical-branch order supplies only existing path arms.
    # Every branch retains exact original pixels and the same floor anchor.
    for row, state in enumerate(tube.STATES):
        for column, direction in enumerate(PORTS):
            original, transform, whole = originals[state, AXES[direction]]
            x, y = column * 64, row * 31
            atlas.paste(branch(whole, direction), (x, y))
            sprites.append({"state": state, "direction": direction, "part": "body", "frame": 0, "tile_variant": 0,
                            "anchor": [-31, 0], "box": [x, y, 64, 31], "source": original["path"],
                            "source_sha256": original["sha256"], "native_transform": transform})
    report = {"_documentation": "Plan13 Group47G/master10.4; archi-map-layers.md. Twelve authored ground conduit source poses; packing/runtime pending.",
              "schema_version": 1, "asset_id": "tube.conduit", "source_record": tube.RECORD.relative_to(tube.ROOT).as_posix(),
              "path": (tube.REVIEW / "tube-conduit-authored.png").relative_to(tube.ROOT).as_posix(),
              "native_cell": [64, 31], "size": [256, 93], "anchor": [-31, 0], "sprites": sprites, "pose_count": 12,
              "source_coverage_complete": True, "runtime_activated": False,
              "draw_order": "floor body below vehicles/buildings; actual saved path owns branch selection; no foreground wall"}
    return report, tube.indexed_png(atlas)


def validate(report, encoded, document=None):
    """Reject incomplete branches, changed originals, palette and false source mappings.

    @param report Complete physical branch/state mapping for twelve poses.
    @param encoded Indexed PNG atlas bytes to inspect.
    @param document Optional isolated immutable source record.
    @return None; source acceptance creates no installed path or electrical availability.
    """
    required = {(state, direction, "body") for state in tube.STATES for direction in PORTS}
    identities = [(row["state"], row["direction"], row["part"]) for row in report["sprites"]]
    if len(identities) != len(required) or set(identities) != required:
        # Count and actual identities must both establish complete path coverage.
        raise ValueError("incomplete native conduit branch/state coverage")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # All static floor components share the accepted DOS palette and alpha;
        # animated/reserved colours cannot impersonate real electrical status.
        if (atlas.mode != "P" or atlas.size != (256, 93) or atlas.info.get("transparency") != 0
                or atlas.getpalette()[:768] != tube.base.PALETTE_BYTES):
            raise ValueError("native conduit palette/canvas mismatch")
        if set(atlas.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
            raise ValueError("forbidden native conduit palette index")
        # Every local crop must keep the shared floor contact and nonempty
        # authored branch. Ground bodies have no invented capsule-front mask.
        for row in report["sprites"]:
            x, y, width, height = row["box"]
            if (row["anchor"] != [-31, 0] or [width, height] != [64, 31]
                    or x < 0 or y < 0 or x + width > 256 or y + height > 93):
                raise ValueError("native conduit anchor/box mismatch")
            if atlas.crop((x, y, x + width, y + height)).getbbox() is None:
                raise ValueError("empty native conduit branch")
    expected, pixels = build(document)
    if report["sprites"] != expected["sprites"]:
        raise ValueError("native conduit false original provenance")
    if pixels != encoded:
        raise ValueError("native conduit changes original branch pixels")


def contact_board(report, encoded):
    """Show every branch and true axis/turn assemblies at the native floor pitch.

    @param report Complete twelve-pose source mapping.
    @param encoded Exact unchanged indexed source atlas.
    @return PNG review board with all states/branches and actual neighbouring joins.
    """
    board = Image.new("RGB", (1536, 1500), (48, 52, 58))
    labels = ImageDraw.Draw(board)
    labels.text((12, 12), "FLOOR SERVICE CONDUITS: exact retained half selection; no foreground/track/power owner; packing/runtime pending", fill="white")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Native half-branches recombine to straight or legal adjacent-cell
        # turns without painting unused arms. Every panel retains fixed light.
        for column, state in enumerate(tube.STATES):
            left = column * 512
            labels.text((left + 12, 45), state + ": native branches8x / native floor contacts4x", fill="white")
            images = {}
            for index, direction in enumerate(PORTS):
                row = next(s for s in report["sprites"] if (s["state"], s["direction"]) == (state, direction))
                x, y, width, height = row["box"]
                images[direction] = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
                image = images[direction].resize((256, 124), Image.Resampling.NEAREST)
                board.paste(image, (left + index % 2 * 256, 80 + index // 2 * 150), image)
                labels.text((left + index % 2 * 256 + 12, 65 + index // 2 * 150), direction, fill="white")
            for row_index, paths in enumerate((("NE", "SW"), ("NW", "SE"), ("NE", "SE"), ("SW", "NW"))):
                scene = Image.new("RGBA", (128, 64))
                for direction in paths:
                    scene.alpha_composite(images[direction], (32, 16))
                # The neighbouring cell contributes only its matching opposite
                # physical branch, preserving actual32/16 world contact pitch.
                direction = paths[0]
                px, py = PORTS[direction]
                dx, dy = (32 if px > 31 else -32), (16 if py > 16 else -16)
                opposite = {"NE": "SW", "SE": "NW", "SW": "NE", "NW": "SE"}[direction]
                scene.alpha_composite(images[opposite], (32 + dx, 16 + dy))
                image = scene.resize((512, 256), Image.Resampling.NEAREST)
                # Disjoint panels preserve every actual source contact rather
                # than letting a later enlarged scene cover a prior join.
                board.paste(image, (left, 390 + row_index * 260), image)
                labels.text((left + 12, 390 + row_index * 260), "/".join(paths), fill="white")
    stream = io.BytesIO()
    board.save(stream, format="PNG")
    return stream.getvalue()


def publish(check=False):
    """Rebuild deterministic source proof or compare it read-only.

    @param check True to compare exact source/contact/provenance bytes.
    @return Complete source report path; packed/runtime assets remain separate.
    """
    report, encoded = build()
    validate(report, encoded)
    contact = contact_board(report, encoded)
    contact_path = tube.REVIEW / "tube-conduit-contact.png"
    report.update(sha256=hashlib.sha256(encoded).hexdigest(), contact=contact_path.relative_to(tube.ROOT).as_posix(),
                  contact_sha256=hashlib.sha256(contact).hexdigest())
    text = json.dumps(report, indent=2) + "\n"
    atlas = tube.base.repository_path(report["path"])
    if check:
        # Changed original/registration cannot silently inherit an earlier
        # passing contact review or source coverage claim.
        if atlas.read_bytes() != encoded or contact_path.read_bytes() != contact or TARGET.read_text(encoding="utf-8") != text:
            raise ValueError("native conduit proof needs deterministic regeneration")
    else:
        tube.REVIEW.mkdir(parents=True, exist_ok=True)
        atlas.write_bytes(encoded)
        contact_path.write_bytes(contact)
        TARGET.write_text(text, encoding="utf-8")
    return TARGET


def bind_source():
    """Publish complete floor cable source while preserving packed identities.

    @return None; only this family's source/authoring metadata changes.
    """
    report = json.loads(publish(check=True).read_text(encoding="utf-8"))
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.conduit")
    first = report["sprites"][0]
    family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                        "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_conduit_branch_state_source_runtime_pending",
                           "source_record": report["source_record"], "atlas_manifest": TARGET.relative_to(tube.ROOT).as_posix(),
                           "native_atlas": report["path"], "native_sha256": report["sha256"], "staged_pose_count": 12,
                           "coverage_complete": True, "runtime_activated": False}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Build/check native cable branches with explicit public source publication.

    @return None; print the exact proof path and preserve failures.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Read-only exact source regeneration")
    parser.add_argument("--publish-source", action="store_true", help="Bind checked source without packing")
    args = parser.parse_args()
    if args.check and args.publish_source:
        # Read-only identity checking must never update public source metadata.
        parser.error("--check cannot be combined with --publish-source")
    print(publish(args.check))
    if args.publish_source:
        bind_source()


if __name__ == "__main__":
    main()
