"""Verify registered ordinary Dock foregrounds and overlapping real paid hulls.

@file orbital_final_art_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13L
Every covered heavy-miner pixel must equal a registered nearer native hull pixel.
No tolerance, substituted pose or changed gameplay coordinate can explain a mismatch.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy

from PIL import Image
from space_preview_checks import ROOT, identity, read_stream

HERE = Path(__file__).parent
TABLES = runpy.run_path(str(HERE / "heavy_joined_checks-2.py"))["table_rows"]
LAYOUT = runpy.run_path(str(HERE / "heavy_contained_service_checks-4.py"))["structure_rows"]


def scalar(raw, row, suffix):
    """Read one exact original scalar from the native self-describing save table.

    @param raw Original decompressed current ordinary save.
    @param row Actual original native vehicle or engine row.
    @param suffix Unique canonical field suffix.
    @return Original unsigned value; this selected paid world has nonnegative positions.
    """
    field, = [field for field in row["fields"] if field["path"] == suffix or field["path"].endswith("." + suffix)]
    return int.from_bytes(raw[field["position"]:field["end"]], "big")


def source_pixels(sheet, row, point):
    """Register every opaque source index through its original packed anchor.

    @param sheet Accepted original indexed packed source sheet.
    @param row Exact protected packed sprite record.
    @param point Actual projected native draw point in the original screenshot.
    @return Map of absolute native coordinates to their required source indices.
    """
    x, y, width, height = row["box"]
    sprite = sheet.crop((x, y, x + width, y + height))
    assert hashlib.sha256(sprite.tobytes()).hexdigest() == row["pixel_sha256"]
    left, top = point[0] + row["anchor"][0], point[1] + row["anchor"][1]
    return {(left + offset % width, top + offset // width): value
            for offset, value in enumerate(sprite.tobytes()) if value}


def qualify(selector):
    """Explain ordinary Dock hull occlusion using only actual registered geometry.

    @param selector Exact completed ordinary paid-Dock capture membership.
    @return Bound foreground/visible-hull/covered-hull report and native detail crop.
    """
    member = json.loads(selector.read_text())["report"]
    path = ROOT / member["path"]
    assert identity(path) == member
    report = json.loads(path.read_text())
    assert report["complete"] and report["owner"] == "dock" and report["checkpoint"] == "heavy-partial"
    assert not any(report[field] for field in ("added_stock", "granted_work", "granted_pose"))
    version, raw = read_stream(ROOT / report["after"]["path"])
    assert version == 438
    structures = LAYOUT(raw, 436)["rows"]
    core, = [row for row in structures.values() if row["kind"] == 6]
    dock, = [row for row in structures.values() if row["kind"] == 7]
    jobs = dock["jobs"]
    assert len(jobs) == 2 and all(job["ticks"] > 0 for job in jobs)
    vehicles = {row["index"]: row for row in TABLES(raw, b"VEHS")}
    engines = {row["index"]: row for row in TABLES(raw, b"EIDS")}
    hulls = {}
    # The two actual contained jobs identify native hulls; engine metadata then
    # resolves their canonical model. An unrelated16 cannot impersonate heavy21.
    for job in jobs:
        vehicle = vehicles[job["craft"]]
        engine = engines[scalar(raw, vehicle, "engine_type")]
        model = scalar(raw, engine, "internal_id") - 0xF000
        assert model in (20, 21) and model not in hulls
        qx, qy = (scalar(raw, vehicle, "position." + axis) for axis in ("x", "y"))
        x, y = qx * 16 // 65536 + 8, qy * 16 // 65536 + 8
        assert scalar(raw, vehicle, "x_pos") == x
        hulls[model] = dict(id=job["craft"], generation=job["generation"], x=x, y=y,
                           z=scalar(raw, vehicle, "z_pos"), saved_y=scalar(raw, vehicle, "y_pos"))
    assert set(hulls) == {20, 21}
    near, far = hulls[20], hulls[21]
    assert near["y"] == far["y"] and near["x"] > far["x"] and near["z"] == far["z"] == 0
    assert near["saved_y"] - near["y"] == far["saved_y"] - far["y"]
    orbital_path = ROOT / "logs/proofs/mlm/native-art/space/orbital-packed-manifest.json"
    space_path = ROOT / "logs/proofs/mlm/native-art/space/packed-space-manifest.json"
    orbital = json.loads(orbital_path.read_text())
    space = json.loads(space_path.read_text())
    orbital_sheet = Image.open(ROOT / "media/baseset/openttd/orbital_final.png")
    space_sheet = Image.open(ROOT / "media/baseset/openttd/space_final.png")
    matches_path = path.with_name("ordinary-orbital-pixel-matches.json")
    matches = json.loads(matches_path.read_text())
    assert matches["ordinary_capture"] == member
    records = []
    # Each original live screenshot must independently retain all foreground
    # indices and a fully explained heavy body; earlier/later frames cannot fill gaps.
    for capture, qualified in zip(report["captures"], matches["captures"], strict=True):
        native = Image.open(ROOT / capture["path"])
        assert identity(ROOT / capture["path"])["sha256"] == capture["sha256"]
        assert hashlib.sha256(native.tobytes()).hexdigest() == capture["indexed_sha256"]
        registrations = set()
        for match in qualified["exact_part_matches"]:
            if match["asset_id"] != "module.nucleus":
                continue
            # Only the actual complete Core's tile-zero back supplies a unique
            # field origin. Repeated floor patterns cannot reposition the port.
            for alias in match["logical_aliases"]:
                if alias["part"] == "back" and alias["tile_variant"] == 0:
                    row, = [row for row in orbital["sprites"] if row["native_offset"] == alias["native_offset"]]
                    registrations.add((match["native_box"][0] - row["anchor"][0], match["native_box"][1] - row["anchor"][1]))
        origin, = registrations

        def project(x, y, z=0):
            """Project relative to the matched native Core using original normal-zoom geometry.

            @param x Native local world X. @param y Native local world Y.
            @param z Actual native height. @return Original normal-zoom screenshot point.
            """
            dx, dy = x - core["x"] * 16, y - core["y"] * 16
            return origin[0] + 2 * (dy - dx), origin[1] + dx + dy - z

        fronts = {}
        for row in orbital["sprites"]:
            # Frame1 is an explicit identical still alias. Every real foreground
            # cell is checked, including ten/eight-pixel jaws below the body match threshold.
            if row["asset_id"] == "module.dock" and row["state"] == "complete" and row["part"] == "front" and row["frame"] == 0:
                tile = row["tile_variant"]
                point = project((dock["x"] + tile % 2) * 16, (dock["y"] + tile // 2) * 16)
                fronts.update(source_pixels(orbital_sheet, row, point))
        assert len(fronts) == 18 and all(native.getpixel(xy) == value for xy, value in fronts.items())
        candidates = {}
        for model, manifest, sheet in ((20, space, space_sheet), (21, orbital, orbital_sheet)):
            point = project(hulls[model]["x"], hulls[model]["y"])
            # Original vehicle bounds.origin(-4,-4,0) raises its normal-zoom
            # image eight pixels. This is a draw transform, never a pose edit.
            point = point[0], point[1] - 8
            candidates[model] = [(row, source_pixels(sheet, row, point)) for row in manifest["sprites"]
                                 if row["asset_id"] == "vehicle.model-" + str(model) and row["state"] == "service" and row["direction"] == "NE"]
        near_matches = [(row, pixels) for row, pixels in candidates[20] if all(native.getpixel(xy) == value for xy, value in pixels.items())]
        assert near_matches, (capture["path"], "nearer actual builder is missing")
        explained = []
        # Only compositions using a positively matched nearer hull may account
        # for occlusion; enumerate its real frame aliases, never arbitrary masks.
        for near_row, near_pixels in near_matches:
            # The farther hull must keep substantial directly visible pixels
            # and explain every remaining source discrepancy by that same hull.
            for far_row, far_pixels in candidates[21]:
                covered = {xy for xy, value in far_pixels.items() if native.getpixel(xy) != value}
                # Positive matching pixels must establish the heavy body; every
                # discrepant index must belong to the nearer registered builder.
                if len(far_pixels) - len(covered) >= 400 and covered and all(near_pixels.get(xy) == native.getpixel(xy) for xy in covered):
                    explained.append(dict(heavy_native_offset=far_row["native_offset"], builder_native_offset=near_row["native_offset"],
                                          heavy_opaque=len(far_pixels), heavy_visible=len(far_pixels) - len(covered),
                                          covered_by_builder=len(covered), unexplained_pixels=0, builder_opaque=len(near_pixels)))
        assert explained, (capture["path"], "unexplained heavy hull mismatch")
        all_pixels = set(fronts).union(*(pixels for _, pixels in candidates[20]), *(pixels for _, pixels in candidates[21]))
        left, top = min(x for x, _ in all_pixels) - 48, min(y for _, y in all_pixels) - 48
        right, bottom = max(x for x, _ in all_pixels) + 49, max(y for _, y in all_pixels) + 49
        detail = (ROOT / capture["path"]).with_name(Path(capture["path"]).stem + "-dock-detail.png")
        native.crop((left, top, right, bottom)).save(detail)
        records.append(dict(capture=capture, core_origin=origin, foreground_pixels=18, explained_compositions=explained, detail=identity(detail)))
    output = path.with_name("ordinary-dock-composition.json")
    output.write_text(json.dumps(dict(_documentation=__doc__, ordinary_capture=member, matches=identity(matches_path),
                                     source_save=report["after"], hulls=hulls, manifests=[identity(orbital_path), identity(space_path)],
                                     captures=records, visual_review_complete=False), indent=2) + "\n")
    return identity(output)


def main():
    """Qualify exactly one explicit completed ordinary Dock capture."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selector", required=True, type=Path)
    args = parser.parse_args()
    print(qualify(ROOT / args.selector), flush=True)


if __name__ == "__main__":
    main()
