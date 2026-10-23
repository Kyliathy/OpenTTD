"""Compose exact native X/Y crossing source from immutable authored casings.

@file convert_tube_crossings.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47F.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native TRACK_BIT_CROSS owns connections/reservations; this selects original pixels only.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube
import convert_tube_junctions as junctions


TARGET = tube.REVIEW / "tube-crossing-atlas-manifest.json"


def sources(document):
    """Bind the crossing to only its two installed native authored casings.

    @param document Retained source record with complete straight lifecycle selections.
    @return Checked contributor pixel map and per-state exact original identities.
    """
    families = [row for row in document["families"] if row["asset_id"] == "tube.crossing"]
    if len(families) != 1:
        raise ValueError("missing or duplicate native crossing family")
    family = families[0]
    # Native crossing has exactly X/Y and one fixed camera geometry. Another
    # donor, lifecycle or direction would paint an uninstalled connection.
    if (family["trackbits"] != 3 or family["native_tracks"] != ["X", "Y"] or family["donor_family"] != "tube.straight"
            or family["directions"] != ["none"] or family["states"] != list(tube.STATES)
            or family["parts"] != ["back", "front"] or family["anchor"] != [-31, 0]
            or family["native_size"] != [64, 31]):
        raise ValueError("native crossing source topology/state/part mismatch")
    originals = tube.straight_sources(document)
    pixels, metadata = {}, {}
    # Every actual authored straight state/view supplies its own pixels; no
    # recolouring or construction/powered substitution fills a source gap.
    for state in tube.STATES:
        metadata[state] = []
        for index, axis in enumerate(("NE", "SE")):
            original, frame, whole = originals[state, axis]
            if original["status"].startswith("rejected"):
                raise ValueError("rejected native crossing contributor")
            transform = original["native_transform"] if frame is None else frame
            pixels[state, index] = {"whole": list(whole.get_flattened_data()),
                                    "front": list(tube.foreground(whole, junctions.TRACKS[index]).get_flattened_data())}
            metadata[state].append({"native_track": junctions.TRACKS[index], "source": original["path"],
                                    "source_sha256": original["sha256"], "native_transform": transform})
    return pixels, metadata


def build(document=None):
    """Build six source parts in stable native state/part order.

    @param document Optional decoded provenance for focused malformed-source checks.
    @return Complete source mapping and exact indexed PNG bytes; packing remains separate.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    pixels, metadata = sources(document)
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.crossing")
    # Source must implement the real fixed crossing geometry and foreground;
    # the old generic four-door draft is not a native view contract.
    if (family["directions"] != ["none"] or family["states"] != list(tube.STATES)
            or family["parts"] != ["back", "front"] or family["frames"] != 1
            or family["native_size"] != [64, 31] or family["anchor"] != [-31, 0]):
        raise ValueError("native crossing public geometry mismatch")
    atlas = Image.new("P", (128, 93), 0)
    atlas.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    # Both actual installed chords trim intersecting rims. The shared source
    # selector never includes a corner donor or manufactures an absent arm.
    for row, state in enumerate(tube.STATES):
        images = junctions.compose_tracks(state, (0, 1), pixels)
        for column, part in enumerate(("back", "front")):
            x, y = column * 64, row * 31
            atlas.paste(images[column], (x, y))
            sprites.append({"state": state, "direction": "none", "part": part, "frame": 0, "tile_variant": 0,
                            "trackbits": 3, "anchor": [-31, 0], "box": [x, y, 64, 31], "contributors": metadata[state]})
    report = {"_documentation": "Plan13 Group47F/master10.4; archi-map-layers.md. Six authored crossing source parts; packing/runtime pending.",
              "schema_version": 1, "asset_id": "tube.crossing", "source_record": tube.RECORD.relative_to(tube.ROOT).as_posix(),
              "path": (tube.REVIEW / "tube-crossing-authored.png").relative_to(tube.ROOT).as_posix(),
              "native_cell": [64, 31], "size": [128, 93], "anchor": [-31, 0], "pose_count": 6, "sprites": sprites,
              "source_coverage_complete": True, "runtime_activated": False,
              "composition": "exact X/Y original pixel selection; trim another installed native channel; both front paths clear"}
    return report, tube.indexed_png(atlas)


def validate(report, encoded, document=None):
    """Reject missing parts, false donors, altered pixels and covered crossing routes.

    @param report Actual source pose/provenance mapping for the complete crossing.
    @param encoded Indexed source atlas bytes to inspect.
    @param document Optional isolated retained source record.
    @return None; only source presentation is accepted, with no runtime claim.
    """
    required = {(state, "none", part) for state in tube.STATES for part in ("back", "front")}
    actual = [(row["state"], row["direction"], row["part"]) for row in report["sprites"]]
    if len(actual) != len(required) or set(actual) != required:
        # Unique coverage must agree with count before any atlas cell is read.
        raise ValueError("incomplete native crossing state/part coverage")
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    sources(document)
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Shared native DOS colours and transparent index are part of the
        # handoff; animated/reserved colours cannot impersonate power state.
        if (atlas.mode != "P" or atlas.size != (128, 93) or atlas.info.get("transparency") != 0
                or atlas.getpalette()[:768] != tube.base.PALETTE_BYTES):
            raise ValueError("native crossing palette/canvas mismatch")
        if set(atlas.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
            raise ValueError("forbidden native crossing palette index")
        # Independent actual native X/Y centreline coordinates detect a front
        # that covers either capsule route even if both layers were repainted.
        channels = [(x, y) for y in range(31) for x in range(15, 48)
                    if abs(2 * y + x - 63) <= 2 or abs(2 * y - x - 1) <= 2]
        for row in report["sprites"]:
            x, y, width, height = row["box"]
            if (row["anchor"] != [-31, 0] or [width, height] != [64, 31]
                    or x < 0 or y < 0 or x + width > 128 or y + height > 93):
                raise ValueError("native crossing anchor/box mismatch")
            image = atlas.crop((x, y, x + width, y + height))
            if image.getbbox() is None:
                raise ValueError("empty native crossing source part")
            if row["part"] == "front" and any(image.getpixel(point) != 0 for point in channels):
                # The pressure shell must leave both installed paths visible.
                raise ValueError("native crossing foreground covers actual route")
    expected, pixels = build(document)
    if report["sprites"] != expected["sprites"]:
        raise ValueError("native crossing false contributor provenance")
    if pixels != encoded:
        raise ValueError("native crossing changes original source pixels")


def contact_board(report, encoded):
    """Present the three true states and all four native neighbour contacts.

    @param report Complete source mapping with stable state/part order.
    @param encoded Exact unchanged indexed atlas bytes.
    @return PNG contact board bytes with native and nearest-neighbour enlargement.
    """
    board = Image.new("RGB", (1536, 820), (48, 52, 58))
    labels = ImageDraw.Draw(board)
    labels.text((12, 12), "NATIVE X/Y CROSSING: only installed X/Y source donors; two real paths remain clear; packing/runtime pending", fill="white")
    originals = tube.straight_sources(json.loads(tube.RECORD.read_text(encoding="utf-8")))
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Equal state panels expose actual contacts at32/16 native pitch rather
        # than a decorative four-way hub which could imply legal corner turns.
        for column, state in enumerate(tube.STATES):
            left = column * 512
            labels.text((left + 12, 45), f"{state}: whole4x / foreground4x", fill="white")
            images = {}
            for index, part in enumerate(("back", "front")):
                row = next(s for s in report["sprites"] if (s["state"], s["part"]) == (state, part))
                x, y, width, height = row["box"]
                images[part] = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
                enlarged = images[part].resize((256, 124), Image.Resampling.NEAREST)
                board.paste(enlarged, (left + 12, 80 + index * 150), enlarged)
            scene = Image.new("RGBA", (192, 96))
            for axis, dx, dy in junctions.NEIGHBOURS.values():
                scene.alpha_composite(originals[state, axis][2].convert("RGBA"), (64 + dx, 32 + dy))
            scene.alpha_composite(images["back"], (64, 32))
            enlarged = scene.resize((384, 192), Image.Resampling.NEAREST)
            board.paste(enlarged, (left + 12, 410), enlarged)
            board.paste(images["back"], (left + 350, 80), images["back"])
    stream = io.BytesIO()
    board.save(stream, format="PNG")
    return stream.getvalue()


def publish(check=False):
    """Build deterministic source proof or verify it read-only.

    @param check True to compare existing exact atlas/contact/mapping bytes.
    @return Complete bound source report path; packed files remain untouched.
    """
    report, encoded = build()
    validate(report, encoded)
    contact = contact_board(report, encoded)
    contact_path = tube.REVIEW / "tube-crossing-contact.png"
    report.update(sha256=hashlib.sha256(encoded).hexdigest(), contact=contact_path.relative_to(tube.ROOT).as_posix(),
                  contact_sha256=hashlib.sha256(contact).hexdigest())
    text = json.dumps(report, indent=2) + "\n"
    atlas = tube.base.repository_path(report["path"])
    if check:
        # Provenance and actual output must regenerate together; a stale proof
        # cannot inherit acceptance after a source/registration change.
        if atlas.read_bytes() != encoded or contact_path.read_bytes() != contact or TARGET.read_text(encoding="utf-8") != text:
            raise ValueError("native crossing proof needs deterministic regeneration")
    else:
        tube.REVIEW.mkdir(parents=True, exist_ok=True)
        atlas.write_bytes(encoded)
        contact_path.write_bytes(contact)
        TARGET.write_text(text, encoding="utf-8")
    return TARGET


def bind_source():
    """Publish verified crossing source while preserving packed metadata.

    @return None; only this family's source/authoring fields change.
    """
    report = json.loads(publish(check=True).read_text(encoding="utf-8"))
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.crossing")
    first = report["sprites"][0]["contributors"][0]
    family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                        "provenance": "first_party_original_source_composition", "rights": "first_party_generated_original"}
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_crossing_state_part_source_runtime_pending",
                           "source_record": report["source_record"], "atlas_manifest": TARGET.relative_to(tube.ROOT).as_posix(),
                           "native_atlas": report["path"], "native_sha256": report["sha256"], "staged_pose_count": 6,
                           "coverage_complete": True, "runtime_activated": False}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Build/check native crossing sources with an explicit publication mode.

    @return None; report the exact proof path and retain failure diagnostics.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Read-only exact regeneration")
    parser.add_argument("--publish-source", action="store_true", help="Bind verified source coverage without packing")
    args = parser.parse_args()
    if args.check and args.publish_source:
        # Read-only identity verification must never update the public contract.
        parser.error("--check cannot be combined with --publish-source")
    print(publish(args.check))
    if args.publish_source:
        bind_source()


if __name__ == "__main__":
    main()
