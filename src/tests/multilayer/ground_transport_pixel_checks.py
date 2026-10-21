"""Check accepted transport source pixels against actual native UI captures.

@file ground_transport_pixel_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHC.
@see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
Only source-owned pixels and native display geometry are compared; no art is generated.
"""

import argparse
import json

from PIL import Image

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import checked_reference, reference
import pack_m1_art


def verify_purchases(execution):
    """Require every available/locked native purchase canvas to equal its accepted source.

    @param execution Repository-relative successful purchase execution report.
    @return Hash-bound exact pixel evidence, without body or station coverage claims.
    """
    proof = handoff.read_json(execution)
    observation = proof["observation"]
    # A successful process must also have emitted the complete native matrix;
    # neither an empty renderer nor stale screenshots constitute purchase proof.
    if proof.get("passed") is not True or observation["exit_code"] != 0 or observation["actual_purchase_views"] != 10:
        raise ValueError("purchase pixels require ten successful actual native views")
    artifacts = {row["path"]: checked_reference(row) for row in observation["artifacts"]}
    reports = [path for path in artifacts.values() if path.name == "transport-purchases.json"]
    if len(reports) != 1:
        raise ValueError("purchase captures lack unique native metadata")
    scene = handoff.read_json(reports[0].relative_to(ROOT).as_posix())
    expected_matrix = {(model, available) for model in range(4, 9) for available in (False, True)}
    if len(scene["frames"]) != 10 or {(row["model"], row["available"]) for row in scene["frames"]} != expected_matrix:
        raise ValueError("purchase capture matrix changed")
    scope, before, public = handoff.read_scope()
    families = {row["asset_id"]: row for row in public["assets"]}
    documents, images, results = {}, {}, []
    # Read model4 from its protected M1 producer, and appended models from the
    # ground producer. Both expose immutable indexed source poses, not screenshots.
    for frame in scene["frames"]:
        family = families[frame["asset_id"]]
        if frame["asset_id"] != "purchase.model-{:02}".format(frame["model"]):
            raise ValueError("purchase model does not identify its source family")
        if frame["model"] == 4:
            poses, atlas = pack_m1_art.source_poses(family, documents, images)
        else:
            poses, atlas = handoff.source_poses(family, scope, before, documents, images)
        pose = poses[("available" if frame["available"] else "locked", 0, "none", 0, "body")]
        left, top, width, height = pose["box"]
        indices = atlas.crop((left, top, left + width, top + height)).tobytes()
        if any(198 <= value <= 205 for value in indices):
            raise ValueError("purchase source unexpectedly requires a company-colour remap")
        native_path = reports[0].parent.parent / "images" / (frame["basename"] + ".ppm")
        if native_path.relative_to(ROOT).as_posix() not in artifacts:
            raise ValueError("purchase raster lacks actual process provenance")
        actual = Image.open(native_path).convert("RGB")
        if actual.size != (160, 80) or frame["baseline"] != [80, 40]:
            raise ValueError("native purchase canvas geometry changed")
        expected = Image.new("RGB", actual.size, tuple(scene["palette"][0]))
        anchor_x, anchor_y = pose["anchor"]
        count = 0
        # Place original opaque indexed pixels at their declared GUI baseline.
        # Comparing the entire canvas additionally detects unexpected neighbours,
        # clipping, duplicate parents or a wrong engine's available/locked sprite.
        for y in range(height):
            for x in range(width):
                value = indices[y * width + x]
                if value == 0:
                    continue
                expected.putpixel((80 + anchor_x + x, 40 + anchor_y + y), tuple(scene["palette"][value]))
                count += 1
        if actual.tobytes() != expected.tobytes():
            raise ValueError("actual native purchase pixels differ: " + frame["basename"])
        results.append({"asset_id": frame["asset_id"], "available": frame["available"], "opaque_pixels": count,
                        "native_rgb": reference(native_path),
                        "source_atlas": reference(handoff.bound(family["authoring"]["native_atlas"])),
                        "source_manifest": reference(handoff.bound(family["authoring"]["atlas_manifest"]))})
    return {"_documentation": __doc__, "passed": True, "execution": reference(handoff.bound(execution)),
            "frames": results, "native_purchase_views": 10, "full_canvas_pixels_compared": 128000,
            "matched_source_pixels": sum(row["opaque_pixels"] for row in results),
            "source_pixels_and_anchors_exact": True, "map_body_coverage": False, "station_foreground_coverage": False}


def main():
    """Verify one actual purchase epoch and retain its repository-local evidence.

    @return None; a mismatch fails before publishing a successful report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execution", required=True)
    parser.add_argument("--report", required=True)
    args = parser.parse_args()
    output = handoff.bound(args.report)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("native pixel diagnostics belong beneath repository logs")
    result = verify_purchases(args.execution)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: result[key] for key in ("passed", "native_purchase_views", "matched_source_pixels")}))


if __name__ == "__main__":
    main()
