"""Compare actual selected industry viewports with their independently accepted indexed source pixels.

@file ground_industry_pixel_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB; explicit current source view.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHB.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This verifier reads source pixels and native captures. It does not render a
replacement game scene or modify artwork, inventories, state or source atlases.
"""

import argparse
import json

from PIL import Image

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import checked_reference, reference


def project(poses, atlas, frame, reverse_parts=False, reverse_depth=False):
    """Project accepted source ownership into the actual fixed native viewport coordinates.

    @param poses Exact accepted state/frame back/front rows and world-cell registrations.
    @param atlas Unchanged indexed source atlas; palette index0 remains transparent.
    @param frame Actual native footprint and centre metadata from the application observer.
    @param reverse_parts Negative-control ordering, without changing the native capture.
    @param reverse_depth Negative-control world-cell depth order.
    @return Visible expected source-owned pixel positions, excluding unrelated world background.
    """
    pixels = {}
    # Native CaptureNativeArtViewport centres a real cell at width//2,height//2
    # plus its half-tile. RemapCoords at Normal gives (-32,+16)/(+32,+16)
    # cell bases. Preserve each source anchor, including actual packed offsets.
    origin_x = 320 + (frame["width"] // 2 - frame["height"] // 2) * 32
    origin_y = 180 - (frame["width"] // 2 + frame["height"] // 2 + 1) * 16
    parts = ("front", "back") if reverse_parts else ("back", "front")
    # Ground/back ownership precedes sorted foreground parents. World depth,
    # rather than atlas row placement, determines overlap among physical cells.
    for part in parts:
        rows = sorted((row for row in poses if row["part"] == part),
                      key=lambda row: (sum(row["world_cell"]), row["world_cell"][0]), reverse=reverse_depth)
        for pose in rows:
            dx, dy = pose["world_cell"]
            left, top, width, height = pose["box"]
            source = atlas.crop((left, top, left + width, top + height)).tobytes()
            anchor_x, anchor_y = pose.get("native_packed_anchor", pose["anchor"])
            screen_x = origin_x + (dy - dx) * 32 + anchor_x
            screen_y = origin_y + (dy + dx) * 16 + anchor_y
            # Transparent source pixels expose the actual world; only original
            # opaque source ownership may overwrite another expected part.
            for y in range(height):
                for x in range(width):
                    value = source[y * width + x]
                    if value != 0:
                        pixels[screen_x + x, screen_y + y] = value
    return pixels


def verify(execution, source_view):
    """Require exact source-derived native pixels for every explicitly captured industry scene.

    @param execution Successful native execution JSON; artifact identities are checked before use.
    @param source_view Explicit checked GroundSourceViews with current original source poses/locations.
    @return Exact compared source/capture identities and overlap-order discrimination, without human acceptance.
    """
    proof = handoff.read_json(execution)
    observed = proof["observation"]
    if proof.get("passed") is not True:
        raise ValueError("industry pixels require a successful actual native process")
    captures = {row["path"]: checked_reference(row) for row in observed["captures"]}
    scene_paths = [path for path in captures.values() if path.name == "industry-fixture.json"]
    if len(scene_paths) != 1:
        raise ValueError("industry pixels require exact native scene metadata")
    scene = handoff.read_json(scene_paths[0].relative_to(ROOT).as_posix())
    image_directory = scene_paths[0].parent.parent / "images"
    sources = {row["asset_id"]: row for row in source_view.scope["source_rows"]}
    documents, atlases, reports = {}, {}, []
    # Each actual captured state resolves its own accepted source, never the
    # live selector's formula or a convenient earlier/other model's contact sheet.
    for frame in scene["frames"]:
        source = sources[frame["asset_id"]]
        if frame["asset_id"] not in documents:
            checked_reference({"path": source["manifest"], "sha256": source["manifest_sha256"]})
            checked_reference({"path": source["source_atlas"], "sha256": source["source_sha256"]})
            documents[frame["asset_id"]] = handoff.read_json(source["manifest"])
            atlases[frame["asset_id"]] = Image.open(handoff.bound(source["source_atlas"]))
            if atlases[frame["asset_id"]].mode != "P":
                raise ValueError("industry source is no longer the qualified indexed native atlas")
        document = documents[frame["asset_id"]]
        state, phase = frame.get("state", "construction"), frame.get("frame", 0)
        poses = [row for row in document["source_parts"] if row["state"] == state and row["frame"] == phase]
        if len(poses) != frame["width"] * frame["height"] * 2:
            raise ValueError("industry source lacks the exact native cell/part scene")
        expected = project(poses, atlases[frame["asset_id"]], frame)
        index_path = image_directory / (frame["basename"] + ".idx")
        if index_path.relative_to(ROOT).as_posix() not in captures:
            raise ValueError("industry indexed raster lacks actual native process provenance")
        actual = index_path.read_bytes()
        if len(actual) != 640 * 360:
            raise ValueError("industry native indexed viewport has unexpected dimensions")
        mismatches = []
        # Match every visible expected source byte at its actual native anchor;
        # a nonempty screenshot or matching terrain colour cannot satisfy this.
        for (x, y), value in expected.items():
            if not (0 <= x < 640 and 0 <= y < 360):
                raise ValueError("industry review camera clipped source-owned machinery")
            if actual[y * 640 + x] != value:
                mismatches.append({"point": [x, y], "source": value, "native": actual[y * 640 + x]})
        if mismatches:
            raise ValueError("industry native source pixels differ: " + frame["basename"] + " " + str(mismatches[:5]))
        reversed_parts = project(poses, atlases[frame["asset_id"]], frame, reverse_parts=True)
        reversed_depth = project(poses, atlases[frame["asset_id"]], frame, reverse_depth=True)
        reports.append({"asset_id": frame["asset_id"], "scene": frame["scene"], "state": state, "frame": phase,
                        "source_manifest": {"path": source["manifest"], "sha256": source["manifest_sha256"]},
                        "source_atlas": {"path": source["source_atlas"], "sha256": source["source_sha256"]},
                        "native_indices": reference(index_path), "native_rgb": reference(index_path.with_suffix(".ppm")),
                        "matched_source_pixels": len(expected), "all_source_pixels_exact": True,
                        "reversed_parent_different_pixels": sum(reversed_parts.get(point) != value for point, value in expected.items()),
                        "reversed_depth_different_pixels": sum(reversed_depth.get(point) != value for point, value in expected.items())})
    return {"_documentation": __doc__, "passed": True, "execution": reference(handoff.bound(execution)),
            "native_scenes": len(reports), "matched_source_pixels": sum(row["matched_source_pixels"] for row in reports),
            "frames": reports, "human_acceptance": "pending", "transported_bootstrap_claimed": False}


if __name__ == "__main__":
    raise SystemExit("Use ground_viewport_review.py with explicit current native/source/build inputs; standalone implicit-cache qualification is retired.")
