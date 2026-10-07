"""Stage selected M1 rolling-stock pixels with explicit partial source coverage.

@file convert_vehicle_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18
@see docs/zz-reach2/upgrades/2026-10/assets/mlvs/vehicle-generation-record.json
Originals/candidates stay immutable. Conversion performs native resampling,
declared authored-component frame assembly and DOS palette packing. Missing
states/animation are never filled by copying an unanimated frame.
"""

import argparse
import hashlib
import json

from PIL import Image, ImageDraw

import convert_m1_art as base
from convert_terrain_art import png_bytes


RECORD = base.ASSETS / "vehicle-generation-record.json"
REPORT = base.OUTPUT / "vehicle-details-manifest.json"
REVIEW = base.ASSETS / "vehicle-details-review.png"
DIRECTIONS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]
# Per-view safe envelopes retain the projected rail-unit proportions; the full
# native canvas and public anchor stay identical. Runtime coupler review remains
# required before these staged sources can count as integrated train graphics.
ENVELOPES = {"N": (16, 24), "NE": (24, 24), "E": (32, 24), "SE": (24, 24),
             "S": (16, 24), "SW": (24, 24), "W": (32, 24), "NW": (24, 24)}


def assemble_component(pixels, source, index, direction, sources, cache):
    """Pack an authored cue onto its declared immutable native chassis.

    @param pixels Selected tool-authored pose resampled to the native canvas.
    @param source Provenance and explicit component/reference registration.
    @param index Selected pose index in the source's declared mapping.
    @param direction Actual engine view for the fixed reference chassis.
    @param sources Immutable source-key lookup, separate from gameplay identities.
    @param cache Native reference images cached by source key and engine direction.
    @return Native RGBA frame with original chassis alpha and only authored cue pixels changed.
    @throws ValueError On foreign geometry, unsupported components or unregistered regions.
    """
    reference = sources[source["component_reference"]]
    if reference["asset_id"] != source["asset_id"] or reference["part"] != source["part"] or reference["frame"] != 0:
        # The component can change only this model/part's selected chassis; a
        # foreign vehicle or animated reference would conceal geometry changes.
        raise ValueError("vehicle component has no fixed own-part reference")
    if source["component_name"] != "cab_beacon":
        raise ValueError("unsupported selected vehicle component")
    key = (reference["key"], direction)
    if key not in cache:
        cells, _boxes = base.source_cells(reference)
        pose = reference["directions"].index(direction)
        subject = cells[pose // 4][pose % 4].crop(reference["native_crop_boxes"][pose])
        cache[key] = base.fit(subject, (64, 48), ENVELOPES[direction])
    image = cache[key].copy()
    sx, sy, width, height = source["component_source_boxes"][index]
    tx, ty, target_width, target_height = source["component_target_boxes"][index]
    if (width, height) != (target_width, target_height) or not (0 <= sx <= 64 - width and 0 <= sy <= 48 - height and 0 <= tx <= 64 - width and 0 <= ty <= 48 - height):
        # Component registration uses exact native coordinates, so no scaled or
        # out-of-canvas region can shift the original contact/silhouette geometry.
        raise ValueError("vehicle component is not registered inside its canvas")
    # Select only retained tool-authored pixels inside the explicit status region.
    # The original chassis alpha stays authoritative; another generated body or
    # detached highlight cannot alter the rail unit's silhouette or coupler seam.
    for row in range(height):
        for column in range(width):
            authored = pixels.getpixel((sx + column, sy + row))
            original = image.getpixel((tx + column, ty + row))
            if authored[3] >= 128 and original[3] >= 128:
                # Both pixels must belong to real opaque equipment. Transparent
                # source padding cannot erase the housing or add another light.
                image.putpixel((tx + column, ty + row), (*authored[:3], original[3]))
    return image


def nearest_component_pose(subject, direction):
    """Retain source-pixel cue colors at the same registered native pose scale.

    @param subject Authored source part inside its explicit crop calibration.
    @param direction Actual engine view determining the approved safe envelope.
    @return RGBA native canvas with nearest-neighbor sampling for tiny cue selection.
    """
    maximum = ENVELOPES[direction]
    scale = min(maximum[0] / subject.width, maximum[1] / subject.height)
    width, height = max(1, round(subject.width * scale)), max(1, round(subject.height * scale))
    image = Image.new("RGBA", (64, 48))
    # Equipment bulbs occupy only a few native pixels. Their authored colors
    # must survive reduction without blending into the neighboring blue roof.
    image.paste(subject.resize((width, height), Image.Resampling.NEAREST), ((64 - width) // 2, 40 - height))
    return image


def derive():
    """Convert declared authored poses without fabricating missing combinations.

    @return Native partial coverage manifest and exact output PNG bytes.
    @throws ValueError On altered original/reference bytes or unsupported poses.
    """
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    public = json.loads((base.ASSETS.parent / "multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in public["assets"]}
    grouped = {}
    cache = {}
    fixed_images = {}
    sources = {source["key"]: source for source in document["sources"]}
    coverage = set()
    # Rejects are provenance, never accepted input cells. Hash verification still
    # protects their retained bytes so later revision history stays auditable.
    for source in document["sources"] + document["candidates"]:
        if base.digest(base.repository_path(source["path"])) != source["sha256"]:
            raise ValueError("immutable vehicle source/candidate changed")
        for reference in source["references"]:
            if base.digest(base.repository_path(reference["path"])) != reference["sha256"]:
                raise ValueError("immutable vehicle reference changed")
    # Every selected cell carries its real part, heading, state and authored frame.
    # Coverage remains sparse until each required combination genuinely exists.
    for source in document["sources"]:
        family = families[source["asset_id"]]
        if source["part"] not in family["parts"]:
            raise ValueError("vehicle source invents another physical part")
        if "poses" in source:
            mapping = source["poses"]
        else:
            if source["grid"] != [4, 2] or source["directions"] != DIRECTIONS:
                # A simple base-view sheet declares exactly eight engine views;
                # multiframe/partial sheets require explicit per-cell identities.
                raise ValueError("vehicle source lacks declared view mapping")
            mapping = [{"direction": direction, "state": source["state"], "frame": source["frame"],
                        "source_cell": [index % 4, index // 4]} for index, direction in enumerate(DIRECTIONS)]
        mapped = [tuple(pose["source_cell"]) for pose in mapping]
        excluded = [tuple(cell) for cell in source.get("excluded_cells", [])]
        columns, rows = source["grid"]
        all_cells = {(column, row) for row in range(rows) for column in range(columns)}
        if len(set(mapped + excluded)) != len(mapped + excluded) or set(mapped + excluded) != all_cells:
            # Every generated cell is selected once or explicitly excluded. A
            # missing last row must never masquerade as a full animation sheet.
            raise ValueError("ambiguous vehicle source cell coverage")
        cells, boxes = base.source_cells(source)
        poses = grouped.setdefault(source["asset_id"], [])
        for index, pose in enumerate(mapping):
            direction, state, frame = pose["direction"], pose["state"], pose["frame"]
            if direction not in DIRECTIONS or state not in family["states"] or frame not in range(family["frames"]):
                # The existing public model owns every heading/state/frame;
                # selected cells cannot add a new work phase or local identity.
                raise ValueError("vehicle pose disagrees with public model contract")
            identity = (source["asset_id"], source["part"], state, frame, direction)
            if identity in coverage:
                raise ValueError("duplicate vehicle pose coverage")
            coverage.add(identity)
            # Explicit selected per-view calibration removes other headings'
            # empty cell padding without changing original pixels. Later frames
            # must retain this registration; moving details cannot shift a chassis.
            crop = source["native_crop_boxes"][index]
            column, row = pose["source_cell"]
            subject = cells[row][column].crop(crop)
            pixels = base.fit(subject, (64, 48), ENVELOPES[direction])
            if "component_reference" in source:
                authored_cue = nearest_component_pose(subject, direction)
                pixels = assemble_component(authored_cue, source, index, direction, sources, fixed_images)
            poses.append({"part": source["part"], "state": state, "frame": frame,
                          "direction": direction, "source": source["key"], "source_path": source["path"],
                          "source_box": boxes[row * columns + column], "anchor": family["anchor"],
                          "component": source.get("component_name"),
                          "component_target_box": source.get("component_target_boxes", [None] * len(mapping))[index],
                          "pixels": base.quantize(pixels, cache)})
    outputs, files = [], {}
    # Stable sorting produces an exact sparse atlas; there is no padded pose
    # pretending that idle frame zero satisfies working/service animation.
    for identity, poses in sorted(grouped.items()):
        family = families[identity]
        poses.sort(key=lambda row: (family["parts"].index(row["part"]), family["states"].index(row["state"]), row["frame"], DIRECTIONS.index(row["direction"])))
        sheet = Image.new("P", (8 * 64, len(poses) // 8 * 48), 0)
        sheet.putpalette(base.PALETTE_BYTES)
        sprites = []
        for index, pose in enumerate(poses):
            x, y = index % 8 * 64, index // 8 * 48
            sheet.paste(pose.pop("pixels"), (x, y))
            sprites.append({**pose, "box": [x, y, 64, 48]})
        path = base.OUTPUT / (identity.replace(".", "-") + "-authored.png")
        encoded = png_bytes(sheet)
        files[path] = encoded
        required = {(part, state, frame, direction) for part in family["parts"]
                    for state in family["states"] for frame in range(family["frames"]) for direction in DIRECTIONS}
        present = {(row["part"], row["state"], row["frame"], row["direction"]) for row in sprites}
        missing = sorted(required - present)
        outputs.append({"asset_id": identity, "path": path.relative_to(base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "size": list(sheet.size),
                        "native_cell": [64, 48], "coverage_complete": not missing,
                        "required_pose_count": len(required), "missing_pose_count": len(missing),
                        "missing": [list(row) for row in missing], "sprites": sprites})
    report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18; sparse source coverage only",
              "schema_version": 1, "source_record": RECORD.relative_to(base.ROOT).as_posix(),
              "runtime_activated": False, "atlases": outputs,
              "pose_count": sum(len(row["sprites"]) for row in outputs),
              "occupied_envelopes": {name: list(size) for name, size in ENVELOPES.items()}}
    return report, files


def publish(report, files):
    """Write partial atlases and a labeled native/enlarged source review.

    @param report Exact selected physical part/state/frame coverage.
    @param files Derived repository atlas paths and their encoded bytes.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    for path, encoded in files.items():
        path.write_bytes(encoded)
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    bind_manifest(report)
    height = sum(row["size"][1] * 3 + 64 for row in report["atlases"]) + 12
    review = Image.new("RGB", (1048, height), (44, 47, 51))
    draw = ImageDraw.Draw(review)
    y = 12
    # Actual native size exposes lost shape/detail; nearest-neighbor enlargement
    # keeps every source pixel visible without implying actual viewport loading.
    for row in report["atlases"]:
        image = Image.open(base.ROOT / row["path"]).convert("RGBA")
        draw.text((12, y), row["asset_id"] + f": {len(row['sprites'])}/{row['required_pose_count']} selected poses; directions N NE E SE S SW W NW", fill="white")
        review.paste(image, (12, y + 24), image)
        large = image.resize((image.width * 2, image.height * 2), Image.Resampling.NEAREST)
        review.paste(large, (12, y + image.height + 36), large)
        y += row["size"][1] * 3 + 64
    review.save(REVIEW)


def bind_manifest(report):
    """Bind exact partial source coverage while retaining original provenance.

    @param report Declared model poses and explicit missing combinations from derive.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    path = base.ASSETS.parent / "multilayer_sprite_manifest.json"
    manifest = json.loads(path.read_text(encoding="utf-8"))
    sources = json.loads(RECORD.read_text(encoding="utf-8"))["sources"]
    # A model's selected chassis, separate support and authored components share
    # one public art family. Preserve previous originals and packed ownership;
    # complete idle cues cannot claim the still-missing moving/work/service art.
    for output in report["atlases"]:
        row = next(row for row in manifest["assets"] if row["asset_id"] == output["asset_id"])
        selected = [source for source in sources if source["asset_id"] == output["asset_id"]]
        original = next(source for source in selected if source["part"] == "body" and source.get("frame") == 0)
        original_paths = list(dict.fromkeys(row.get("authoring", {}).get("originals", []) + [source["path"] for source in selected]))
        row["source"] = {"status": "authored" if output["coverage_complete"] else "authored_partial",
                         "path": original["path"], "sha256": original["sha256"],
                         "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
        row["authoring"] = {"date": "2026-10-07", "status": "partial_registered_source_coverage_runtime_pending",
                            "source_record": report["source_record"], "originals": original_paths,
                            "atlas_manifest": REPORT.relative_to(base.ROOT).as_posix(),
                            "native_atlas": output["path"], "native_sha256": output["sha256"],
                            "staged_pose_count": len(output["sprites"]), "required_pose_count": output["required_pose_count"],
                            "missing_pose_count": output["missing_pose_count"], "runtime_activated": False,
                            "coverage_complete": output["coverage_complete"]}
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def check(expected, files):
    """Verify reproducibility, reserved colors, source sparsity and transparency.

    @param expected Coverage derived freshly from immutable selected originals.
    @param files Expected encoded atlas bytes regenerated without writes.
    @throws ValueError If partial coverage or native pixels disagree with records.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    if json.loads(REPORT.read_text(encoding="utf-8")) != expected:
        raise ValueError("vehicle coverage manifest disagrees with originals")
    # Each model must regenerate exactly from its selected cells while retaining
    # honest missing-pose counts; rejection history cannot silently fill a gap.
    for row in expected["atlases"]:
        path = base.repository_path(row["path"])
        if path.read_bytes() != files[path]:
            raise ValueError("vehicle conversion is not reproducible")
        image = Image.open(path)
        if image.mode != "P" or image.info.get("transparency") != 0 or image.getpalette() != base.PALETTE_BYTES:
            # The train uses the same DOS alpha/palette semantics as the supplied
            # profile; reserved indices cannot become animation or livery effects.
            raise ValueError("vehicle palette/transparency mismatch")
        if set(image.get_flattened_data()) & (base.FORBIDDEN - {0}):
            raise ValueError("vehicle uses reserved palette indices")
        if len(row["sprites"]) + row["missing_pose_count"] != row["required_pose_count"]:
            raise ValueError("vehicle sparse coverage is falsely complete")
        native = {}
        # Pose keys keep actual part/state/frame/view ownership explicit; a
        # selected bulb frame cannot stand in for another physical train part.
        for pose in row["sprites"]:
            x, y, width, height = pose["box"]
            sprite = image.crop((x, y, x + width, y + height))
            if not any(sprite.get_flattened_data()):
                raise ValueError("empty selected vehicle pose")
            if any(sprite.getpixel(point) for point in [(0, 0), (63, 0), (0, 47), (63, 47)]):
                raise ValueError("opaque vehicle canvas corner")
            native[(pose["part"], pose["state"], pose["frame"], pose["direction"])] = sprite
        # Idle equipment cues are the only allowed changing pixels in these
        # assembled frames. Independently compare the published chassis, alpha
        # and coupler footprint against frame zero rather than trusting a crop.
        for pose in row["sprites"]:
            if pose["component"] is None:
                continue
            current = native[(pose["part"], pose["state"], pose["frame"], pose["direction"])]
            original = native[(pose["part"], "idle", 0, pose["direction"])]
            x, y, width, height = pose["component_target_box"]
            if current.tobytes() == original.tobytes():
                raise ValueError("authored animation cue disappeared at native palette scale")
            for yy in range(48):
                for xx in range(64):
                    before, after = original.getpixel((xx, yy)), current.getpixel((xx, yy))
                    if (before == 0) != (after == 0):
                        raise ValueError("status animation changes chassis alpha/contacts")
                    if not (x <= xx < x + width and y <= yy < y + height) and before != after:
                        # All frame changes must stay inside the declared real
                        # status component; body or wheel motion is forbidden here.
                        raise ValueError("status frame changes pixels outside authored cue")
        for direction in DIRECTIONS:
            idle = [native[("body", "idle", frame, direction)] for frame in range(4)
                    if ("body", "idle", frame, direction) in native]
            if len(idle) == 4 and len({image.tobytes() for image in idle}) != 4:
                # Four supplied idle cue phases must survive the native palette;
                # duplicated frame-zero pixels cannot conceal an unfilled frame.
                raise ValueError("idle authored cue phases collapse into duplicate pixels")
    print(f"Vehicle source conversion: {len(expected['atlases'])} models / {expected['pose_count']} authored poses; exact regeneration, palette, alpha and sparse coverage pass; runtime pending")


def main():
    """Publish or verify the bounded selected rolling-stock source handoff.

    @return None; fail on invalid retained sources or derived partial coverage.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="verify without writing")
    args = parser.parse_args()
    report, files = derive()
    if not args.check:
        publish(report, files)
    check(report, files)


if __name__ == "__main__":
    main()
