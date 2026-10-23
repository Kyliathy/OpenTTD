"""Compose native tube junction source art from retained authored track casings.

@file convert_tube_junctions.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only original pixels are selected/trimmed. Native TrackBits remain the sole topology owner.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube
import convert_tube_curves as curves


TRACKS = ("X", "Y", "Upper", "Lower", "Left", "Right")
PORTS = (("NE", "SW"), ("NW", "SE"), ("NW", "NE"), ("SW", "SE"), ("NW", "SW"), ("NE", "SE"))
CONTACTS = {"NE": (47, 8), "SE": (47, 24), "SW": (15, 24), "NW": (15, 8)}
NEIGHBOURS = {"NE": ("NE", 32, -16), "SE": ("SE", 32, 16), "SW": ("NE", -32, 16), "NW": ("SE", -32, -16)}
TARGET = tube.REVIEW / "tube-junction-atlas-manifest.json"


def layouts():
    """Return stable native shared-edge layouts, excluding separately rendered cases.

    @return 54 complete native TrackBits masks in increasing order; no gameplay registry is created.
    """
    # Native six-bit track ownership admits all nonempty flat combinations.
    # Six singles, X/Y crossing and two independent parallel-curve pairs have
    # no shared edge and retain their original source/render families.
    return [mask for mask in range(1, 64) if mask.bit_count() >= 2 and mask not in (3, 12, 48)]


def track_indices(mask):
    """Resolve only the installed native pieces for one junction source variant.

    @param mask Complete native six-track bitset; no unknown/depot/wormhole bits are allowed.
    @return Canonical track-index order for a real shared-edge layout.
    """
    if mask not in layouts():
        raise ValueError("not a native shared-edge tube junction layout")
    return [index for index in range(6) if mask & (1 << index)]


def installed_ports(mask):
    """Derive visible physical mouths from the exact installed native pieces.

    @param mask Checked shared-edge native TrackBits mask.
    @return Set of physical edge labels; no missing arm is added for visual symmetry.
    """
    return {port for index in track_indices(mask) for port in PORTS[index]}


def channel(index):
    """Describe the actual projected capsule chord and its narrow source aperture.

    @param index Native X/Y/Upper/Lower/Left/Right track index.
    @return Set of native pixel offsets that foreground casing must leave clear.
    """
    result = set()
    # Native movement chords, extended through their matching mouth contacts,
    # trim only casing crossed by another installed piece. These masks select
    # existing source pixels; they neither paint track nor authorize movement.
    for y in range(31):
        for x in range(64):
            tests = (15 <= x <= 47 and abs(2 * y + x - 63) <= 2,
                     15 <= x <= 47 and abs(2 * y - x - 1) <= 2,
                     15 <= x <= 47 and abs(y - 7) <= 1,
                     15 <= x <= 47 and abs(y - 23) <= 1,
                     7 <= y <= 24 and abs(x - 15) <= 1,
                     7 <= y <= 24 and abs(x - 47) <= 1)
            if tests[index]:
                result.add(y * 64 + x)
    return result


def primitive_sources(document):
    """Resolve all checked authored states for the six existing native casings.

    @param document Retained originals and explicit straight/corner frame selections.
    @return State/index map containing source metadata and whole/foreground indexed pixels.
    """
    straight = tube.straight_sources(document)
    corner = curves.selected_sources(document)
    result = {}
    # The originals already contain every engine view and lifecycle. Junction
    # composition reuses their exact anchors/light/palette instead of rotating
    # a bitmap or synthesizing an absent geometric/state source.
    for state in tube.STATES:
        for index, direction in enumerate(("NE", "SE", "N", "S", "W", "E")):
            if index < 2:
                original, frame, whole = straight[state, direction]
                transform = original["native_transform"] if frame is None else frame
                front = tube.foreground(whole, TRACKS[index])
            else:
                original, transform, whole = corner[state, direction]
                front = tube.curve_foreground(whole, direction)
            result[state, index] = {"source": original["path"], "source_sha256": original["sha256"],
                                    "native_transform": transform, "whole": list(whole.get_flattened_data()),
                                    "front": list(front.get_flattened_data())}
    return result


def compose(state, mask, sources):
    """Join only installed casings and open their shared actual capsule channels.

    @param state Authored lifecycle shared by every installed contributor.
    @param mask Complete installed native TrackBits layout.
    @param sources Checked native original pixels for all six track shapes/states.
    @return Whole and foreground indexed source parts with identical64x31 anchors.
    """
    return compose_tracks(state, track_indices(mask), sources)


def compose_tracks(state, active, sources):
    """Trim/join an explicit set of already authored native track casings.

    @param state Original construction/powered/unpowered lifecycle shared by contributors.
    @param active Explicit unique native track indices; caller owns family/topology restrictions.
    @param sources Checked original whole/front pixels at the common native anchor.
    @return Whole and foreground images selecting only unchanged contributor pixels.
    """
    # The shared pixel selector permits only actual native pieces. Family
    # wrappers separately distinguish shared-edge junctions from X/Y crossing;
    # duplicate/unknown tracks cannot fabricate another source connection.
    if not active or len(set(active)) != len(active) or any(index not in range(6) for index in active):
        raise ValueError("invalid explicit native tube source tracks")
    channels = {index: channel(index) for index in active}
    all_channels = set().union(*channels.values())
    whole, front = [0] * (64 * 31), [0] * (64 * 31)
    # A rim crossing another installed route is trimmed to create the actual
    # shared opening. No unrelated piece supplies pixels, so an absent arm can
    # never appear simply because a generic junction bitmap had four doors.
    for index in active:
        others = set().union(*(points for other, points in channels.items() if other != index))
        original = sources[state, index]
        for offset, value in enumerate(original["whole"]):
            if value != 0 and offset not in others:
                whole[offset] = value
        # Low end coupling plates remain beneath capsules. The foreground
        # additionally leaves every installed chord clear through both mouths.
        for offset, value in enumerate(original["front"]):
            if value != 0 and offset not in all_channels:
                front[offset] = value
    # Foreground wins where authored casings meet, matching normal painter
    # order. Keeping those same pixels in the whole back makes recomposition
    # exact without manufacturing a colour or shifting an original silhouette.
    for offset, value in enumerate(front):
        if value != 0:
            whole[offset] = value
    images = []
    for pixels in (whole, front):
        image = Image.new("P", (64, 31), 0)
        image.putpalette(tube.base.PALETTE_BYTES)
        image.putdata(pixels)
        images.append(image)
    return tuple(images)


def build(document=None):
    """Build all324 checked topology/state/part source poses without publishing them.

    @param document Optional decoded source record for focused malformed-input checks.
    @return Exact source report and indexed atlas bytes, independent of packed/runtime activation.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    selected = [row for row in document["families"] if row["asset_id"] == "tube.junction"]
    if len(selected) != 1:
        raise ValueError("missing or duplicate selected tube junction family")
    selection = selected[0]
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.junction")
    # The source/public geometry contract agrees on every installed topology,
    # shared anchor and contributing family. A partial/different matrix cannot
    # inherit the source-complete status of the original selected profile.
    if (family["directions"] != ["none"] or family["parts"] != ["back", "front"]
            or family["native_layouts"] != layouts() or family["tile_variants"] != 54
            or family["states"] != list(tube.STATES) or family["frames"] != 1
            or family["native_size"] != [64, 31] or family["anchor"] != [-31, 0]
            or selection["native_layouts"] != layouts()
            or selection["states"] != list(tube.STATES)
            or selection["source_mode"] != "composition_of_retained_authored_primitives"
            or selection["input_families"] != ["tube.straight", "tube.bend45"]):
        raise ValueError("tube junction native topology/source contract mismatch")
    sources = primitive_sources(document)
    sheet = Image.new("P", (54 * 128, 93), 0)
    sheet.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    # Variants are native topology indices, never world footprint cells.
    # Both parts reuse the same original contributors and native anchor.
    for row, state in enumerate(tube.STATES):
        for variant, mask in enumerate(layouts()):
            for part_index, (part, pixels) in enumerate(zip(("back", "front"), compose(state, mask, sources))):
                x, y = (variant * 2 + part_index) * 64, row * 31
                sheet.paste(pixels, (x, y))
                sprites.append({"state": state, "direction": "none", "frame": 0, "part": part, "tile_variant": variant,
                                "native_track_bits": mask, "source_frames": [f"{state}:{TRACKS[index]}" for index in track_indices(mask)],
                                "ports": sorted(installed_ports(mask)), "anchor": [-31, 0], "box": [x, y, 64, 31]})
    frames = {f"{state}:{TRACKS[index]}": {key: value for key, value in source.items() if key not in ("whole", "front")}
              for (state, index), source in sources.items()}
    report = {"_documentation": "Plan13 Group47D/master10.4; archi-map-layers.md. Authored-source composition only; native topology and quantities remain with their existing owners.",
              "schema_version": 1, "asset_id": "tube.junction", "source_record": tube.RECORD.relative_to(tube.ROOT).as_posix(),
              "path": (tube.REVIEW / "tube-junction-authored.png").relative_to(tube.ROOT).as_posix(),
              "size": list(sheet.size), "native_cell": [64, 31], "anchor": [-31, 0], "native_layouts": layouts(),
              "primitive_frames": frames, "sprites": sprites, "pose_count": len(sprites),
              "source_coverage_complete": True, "runtime_activated": False,
              "composition_policy": "Only installed authored casings; trim a piece where another installed native chord crosses it; foreground clears every actual installed chord; exact original colours/coordinates retained."}
    return report, tube.indexed_png(sheet)


def validate(report, encoded, document=None):
    """Inspect topology coverage, original-pixel provenance, missing arms and paths.

    @param report Explicit source mapping for every selected native junction.
    @param encoded Actual indexed atlas bytes to inspect independently.
    @param document Optional decoded retained source record for boundary tests.
    @return None; refuse a missing/extra piece, painted pixel, covered path or placement/palette mismatch.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    required = {(state, mask, part) for state in tube.STATES for mask in layouts() for part in ("back", "front")}
    identities = [(row["state"], row["native_track_bits"], row["part"]) for row in report["sprites"]]
    if (len(identities) != len(required) or set(identities) != required
            or report["pose_count"] != len(required) or report["native_layouts"] != layouts()):
        # An exact total alone could hide one omitted topology/state/part.
        raise ValueError("incomplete native tube junction pose coverage")
    sources = primitive_sources(document)
    expected_frames = {f"{state}:{TRACKS[index]}": {key: value for key, value in source.items() if key not in ("whole", "front")}
                       for (state, index), source in sources.items()}
    if report["primitive_frames"] != expected_frames:
        # Actual pixels and their declared provenance must bind the same
        # checked originals/transforms, rather than a stale or invented hash.
        raise ValueError("tube junction primitive provenance mismatch")
    with Image.open(io.BytesIO(encoded)) as sheet:
        # Preserve the same native DOS palette/transparency as all retained
        # casings; new topology is not permission to introduce animated pixels.
        if (sheet.mode != "P" or sheet.size != (6912, 93) or sheet.info.get("transparency") != 0
                or sheet.getpalette()[:768] != tube.base.PALETTE_BYTES):
            raise ValueError("tube junction native palette/canvas mismatch")
        if set(sheet.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
            raise ValueError("tube junction contains a forbidden palette index")
        parts = {}
        # Every variant binds its exact installed contributors, ports, crop
        # and anchor before inspecting pixel-level paths and provenance.
        for row in report["sprites"]:
            state, mask = row["state"], row["native_track_bits"]
            active = track_indices(mask)
            variant = layouts().index(mask)
            expected_box = [(variant * 2 + (row["part"] == "front")) * 64, tube.STATES.index(state) * 31, 64, 31]
            if (row["anchor"] != [-31, 0] or row["box"] != expected_box or row["tile_variant"] != variant
                    or row["direction"] != "none" or row["frame"] != 0):
                raise ValueError("tube junction native anchor/variant box mismatch")
            if row["source_frames"] != [f"{state}:{TRACKS[index]}" for index in active] or row["ports"] != sorted(installed_ports(mask)):
                raise ValueError("tube junction contributes an absent native track or arm")
            x, y, width, height = row["box"]
            image = sheet.crop((x, y, x + width, y + height))
            if image.getbbox() is None:
                raise ValueError("empty native tube junction part")
            pixels = list(image.get_flattened_data())
            # Each visible pixel must already exist at that same native
            # coordinate in an installed authored piece. No new paint, flat
            # rotation, shifted sprite or uninstalled donor can fill a gap.
            for offset, value in enumerate(pixels):
                if value != 0 and not any(value == sources[state, index]["whole"][offset] for index in active):
                    raise ValueError("tube junction contains a nonoriginal or uninstalled pixel")
            if row["part"] == "front":
                blocked = set().union(*(channel(index) for index in active))
                if any(pixels[offset] != 0 for offset in blocked):
                    raise ValueError("tube junction foreground covers an installed native path")
            absent = set(CONTACTS) - installed_ports(mask)
            # Missing mouths stay visibly empty, independently of the source
            # binding/count check; a symmetric generic housing must not imply
            # a fourth physical arm on an actual three-mouth junction.
            for port in absent:
                px, py = CONTACTS[port]
                if any(image.getpixel((px + dx, py + dy)) != 0 for dx in (-1, 0, 1) for dy in (-1, 0, 1)):
                    raise ValueError("tube junction paints an absent native mouth")
            parts[state, mask, row["part"]] = pixels
        # Parts recombine without any changed colour, preserving the selected
        # source appearance when no actual vehicle lies between their layers.
        for state in tube.STATES:
            for mask in layouts():
                back, front = parts[state, mask, "back"], parts[state, mask, "front"]
                if back == front or any(near != 0 and near != far for far, near in zip(back, front)):
                    raise ValueError("tube junction foreground changes original casing pixels")


def review_pages(report, encoded):
    """Display every actual junction with state-matched adjacent original tracks.

    @param report Complete324-pose source mapping.
    @param encoded Exact indexed atlas whose pixels are displayed unchanged.
    @return Six PNG review pages containing all54 topologies and all three lifecycle states.
    """
    straight = tube.straight_sources(json.loads(tube.RECORD.read_text(encoding="utf-8")))
    pages = {}
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Nine native layouts per page keep all states and physical neighbour
        # contacts legible. No generic four-door scene replaces actual mouths.
        for page in range(6):
            board = Image.new("RGB", (1536, 1960), (48, 52, 58))
            labels = ImageDraw.Draw(board)
            labels.text((14, 12), f"NATIVE JUNCTION SOURCE PAGE{page + 1}/6: real TrackBits, construction/powered/unpowered; packing/runtime pending", fill="white")
            for item, mask in enumerate(layouts()[page * 9:(page + 1) * 9]):
                left, top = item % 3 * 512, item // 3 * 640 + 40
                labels.text((left + 12, top), f"bits{mask:02x}: {'/'.join(TRACKS[index] for index in track_indices(mask))}", fill="white")
                for row, state in enumerate(tube.STATES):
                    scene = Image.new("RGBA", (192, 96))
                    for port in sorted(installed_ports(mask)):
                        direction, dx, dy = NEIGHBOURS[port]
                        neighbour = straight[state, direction][2].convert("RGBA")
                        scene.alpha_composite(neighbour, (64 + dx, 32 + dy))
                    sprite = next(s for s in report["sprites"] if (s["state"], s["native_track_bits"], s["part"]) == (state, mask, "back"))
                    x, y, width, height = sprite["box"]
                    pixels = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
                    scene.alpha_composite(pixels, (64, 32))
                    enlarged = scene.resize((384, 192), Image.Resampling.NEAREST)
                    labels.text((left + 12, top + 24 + row * 198), state, fill="white")
                    board.paste(enlarged, (left + 44, top + 40 + row * 198), enlarged)
            stream = io.BytesIO()
            board.save(stream, format="PNG")
            pages[tube.REVIEW / f"tube-junction-contact-{page + 1}.png"] = stream.getvalue()
    return pages


def publish(check=False):
    """Publish reproducible junction source proof or verify it without writing.

    @param check True for exact read-only regeneration of pixels, mapping and review pages.
    @return Source report path; packed IDs and runtime remain untouched.
    """
    report, encoded = build()
    validate(report, encoded)
    pages = review_pages(report, encoded)
    report.update(sha256=hashlib.sha256(encoded).hexdigest(), reviews=[{"path": path.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(data).hexdigest()} for path, data in pages.items()])
    text = json.dumps(report, indent=2) + "\n"
    atlas = tube.base.repository_path(report["path"])
    if check:
        # Exact page/pixel/mapping agreement prevents changed selections from
        # inheriting a historical source-complete review silently.
        if atlas.read_bytes() != encoded or TARGET.read_text(encoding="utf-8") != text:
            raise ValueError("tube junction source proof needs deterministic regeneration")
        for path, data in pages.items():
            if path.read_bytes() != data:
                raise ValueError("tube junction native contact review changed")
    else:
        tube.REVIEW.mkdir(parents=True, exist_ok=True)
        atlas.write_bytes(encoded)
        TARGET.write_text(text, encoding="utf-8")
        for path, data in pages.items():
            path.write_bytes(data)
    return TARGET


def bind_source():
    """Publish checked original composition without modifying packed/native identities.

    @return None; update only the junction family's source/authoring metadata.
    """
    report = json.loads(publish(check=True).read_text(encoding="utf-8"))
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "tube.junction")
    originals = list(dict.fromkeys(row["source"] for row in report["primitive_frames"].values()))
    first = next(iter(report["primitive_frames"].values()))
    family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                        "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original",
                        "conversion": "composition_of_retained_authored_primitives"}
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_topology_state_part_source_runtime_pending",
                           "source_record": report["source_record"], "originals": originals,
                           "atlas_manifest": TARGET.relative_to(tube.ROOT).as_posix(), "native_atlas": report["path"],
                           "native_sha256": report["sha256"], "staged_pose_count": report["pose_count"],
                           "coverage_complete": True, "runtime_activated": False}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Build/check complete junction source proof from explicit immutable originals.

    @return None; print the source report path and retain precise failures.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Compare complete source proof without writing")
    parser.add_argument("--publish-source", action="store_true", help="Bind checked source composition in the public manifest")
    args = parser.parse_args()
    if args.check and args.publish_source:
        # A read-only source identity check must never refresh its public contract.
        parser.error("--check cannot be combined with --publish-source")
    print(publish(args.check))
    if args.publish_source:
        bind_source()


if __name__ == "__main__":
    main()
