"""Compare actual vehicle map/list pixels with unchanged accepted indexed body sources.

@file ground_transport_body_pixel_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB; explicit current source view.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHC.
@see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
Native depot scenery may occlude map bodies; the ordinary list renderer keeps
its native clipping rectangle. Both boundaries are reported explicitly.
No sprite or gameplay state is written.
"""

import argparse
import json

from PIL import Image

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import checked_reference, reference
import pack_m1_art

LABELS = {
    4: ["COAL"],
    5: ["COAL", "IORE", "CORE", "BAUX", "SALT", "LITH", "NORE", "COBT", "RARE", "RFMT", "STEL", "SCMP", "RBPT", "ELEC", "BATY", "SMIK", "GOOD"],
    6: ["URAN", "RARE"],
    7: ["COAL", "IORE", "CORE", "BAUX", "SALT", "LITH", "NORE", "COBT", "ERTH", "ROCK"],
    8: ["URAN", "RARE"],
}


def expected_matrix(mode):
    """Declare the independently required native body, load, view, phase and list scope.

    @param mode Rail4/5/6 or road7/8 native observer burst.
    @return Exact expected frame identities, including every supported cargo source meaning.
    """
    models = (4, 5, 6) if mode == "rail" else (7, 8)
    expected = set()
    # Exhaust all physical presentation axes on the first registered cargo;
    # additional cargo meanings need both loaded states in map and list views.
    for model in models:
        for variant, label in enumerate(LABELS[model]):
            states = range(5) if variant == 0 else (1, 2)
            if model == 4:
                states = (0, 3, 4)  # Actual zero-capacity powered hauler, rather than a cargo wagon.
            for state in states:
                for phase in (range(2) if variant == 0 else (0,)):
                    for direction in (range(8) if variant == 0 else (6,)):
                        expected.add((model, label, state, phase, direction, "map"))
                expected.add((model, label, state, 0, 6, "list"))
    return expected


def source_key(frame, family):
    """Resolve expected source meaning from independently disclosed native fixture boundaries.

    @param frame Actual vehicle model, packet state, cargo label, view and phase metadata.
    @param family Accepted source state/view declarations.
    @return Exact accepted source identity tuple.
    """
    state = frame["state_index"]
    phase = frame["frame"]
    if frame["model"] == 4:
        # Retained M1 hauler art follows actual motion, depot and exchange
        # assignment, rather than the appended wagons' packet fullness states.
        state = 4 if state == 3 else 1 if state < 3 else 0
        # Its retained idle/service source lamps also have two native phases;
        # only the ordinary static list context supplies phase0.
    variant = frame.get("source_variant", 0)
    if frame["model"] in (5, 7):
        variant = LABELS[frame["model"]].index(frame["cargo_label"])
    return family["states"][state], phase, family["directions"][frame["direction"]], variant, "body"


def verify(execution, source_view):
    """Require exact accepted body pixels in each provenance-bound native map/list observation.

    @param execution Repository-relative successful rail or road execution report.
    @param source_view Explicit checked GroundSourceViews with current original source poses/locations.
    @return Pixel and explicit native-depot occlusion observations for the exact process.
    """
    proof = handoff.read_json(execution)
    observed = proof["observation"]
    if proof.get("passed") is not True or observed["exit_code"] != 0:
        raise ValueError("body pixels require a successful actual native process")
    artifacts = {row["path"]: checked_reference(row) for row in observed["artifacts"]}
    metadata = [path for path in artifacts.values() if path.name == "transport-" + proof["mode"] + ".json"]
    if len(metadata) != 1:
        raise ValueError("body pixels lack unique actual native scene metadata")
    scene = handoff.read_json(metadata[0].relative_to(ROOT).as_posix())
    actual_matrix = {(row["model"], row["cargo_label"], row["state_index"], row["frame"], row["direction"], row.get("context", "map"))
                     for row in scene["frames"]}
    required = expected_matrix(proof["mode"])
    # Duplicate or missing native frames cannot turn a partial cargo/view
    # matrix into a complete body proof merely by preserving a row count.
    if len(scene["frames"]) != len(required) or actual_matrix != required:
        raise ValueError("actual native body matrix lacks exact required cargo/state/view coverage")
    families = source_view.families
    documents, images, sources, results = {}, {}, {}, []
    # Each native frame resolves an unchanged accepted source and actual company
    # recolour table. Public catalogue numbers never stand in for runtime IDs.
    for frame in scene["frames"]:
        asset = frame["asset_id"]
        family = families[asset]
        if asset != "vehicle.model-{:02}".format(frame["model"]):
            raise ValueError("native vehicle model does not identify its accepted body family")
        if asset not in sources:
            sources[asset] = source_view.poses(asset)
        poses, atlas = sources[asset]
        key = source_key(frame, family)
        pose = poses[key]
        left, top, width, height = pose["box"]
        indices = atlas.crop((left, top, left + width, top + height)).tobytes()
        basename = frame["basename"]
        native_path = metadata[0].parent.parent / "images" / (basename + ".ppm")
        if native_path.relative_to(ROOT).as_posix() not in artifacts:
            raise ValueError("body RGB capture lacks actual native process provenance")
        actual_rgb = Image.open(native_path).convert("RGB")
        is_list = frame.get("context") == "list"
        actual = None
        index_path = native_path.with_suffix(".idx")
        if not is_list:
            origin, contact = frame["native_sorting_origin"], frame["native_contact_offset"]
            x, y, z = [a + b for a, b in zip(origin, contact)]
            if frame["baseline"] != [320 + 2 * (y - x), 180 + x + y - z]:
                raise ValueError("body baseline differs from native sorting origin/contact projection")
            if index_path.relative_to(ROOT).as_posix() not in artifacts:
                raise ValueError("body indexed capture lacks actual native process provenance")
            actual = index_path.read_bytes()
            if len(actual) != 640 * 360 or actual_rgb.size != (640, 360):
                raise ValueError("body native viewport size changed")
        elif actual_rgb.size != (256, 80):
            raise ValueError("body native list size changed")
        anchor_x, anchor_y = pose["anchor"]
        matched, mismatches, clipped = 0, [], 0
        # Original opaque pixels retain exact source anchors and native company
        # palette ownership. Map scenery is excluded from body-only expectations.
        for y in range(height):
            for x in range(width):
                value = indices[y * width + x]
                if value == 0:
                    continue
                sx, sy = frame["baseline"][0] + anchor_x + x, frame["baseline"][1] + anchor_y + y
                # DrawVehicleImage owns a native Rect(64,0,223,79). Its first
                # chain element can extend before that scroll boundary; those
                # clipped source pixels cannot be asserted as visible evidence.
                if is_list and not (64 <= sx <= 223 and 0 <= sy <= 79):
                    clipped += 1
                    continue
                remapped = frame["remap"][value]
                if is_list:
                    equal = actual_rgb.getpixel((sx, sy)) == tuple(scene["palette"][remapped])
                else:
                    equal = actual[sy * 640 + sx] == remapped
                if equal:
                    matched += 1
                else:
                    mismatches.append([sx, sy, value])
        # Service map bodies share the actual depot's independently sorted
        # roof. Their visible native list source is required separately.
        if mismatches and (is_list or frame["state_index"] != 3):
            raise ValueError("body source mismatch {} key={} matched={} missing={} first={}".format(basename, key, matched, len(mismatches), mismatches[:3]))
        if matched == 0 and (is_list or frame["state_index"] != 3):
            raise ValueError("native body capture contains no accepted source pixels")
        results.append({"asset_id": asset, "context": "list" if is_list else "map", "source_key": list(key),
                        "matched_source_pixels": matched, "native_depot_occluded_source_pixels": len(mismatches),
                        "native_list_clipped_source_pixels": clipped,
                        "native_rgb": reference(native_path), "native_indices": None if is_list else reference(index_path),
                        "source_atlas": reference(handoff.bound(family["authoring"]["native_atlas"])),
                        "source_manifest": reference(handoff.bound(family["authoring"]["atlas_manifest"]))})
    return {"_documentation": __doc__, "passed": True, "execution": reference(handoff.bound(execution)),
            "frames": results, "native_body_frames": len(results),
            "matched_source_pixels": sum(row["matched_source_pixels"] for row in results),
            "source_pixels_and_anchors_exact_outside_native_depot_occlusion": True,
            "station_foreground_coverage": False, "human_acceptance": "pending"}


def main():
    """Require the explicit current native producer, which seals these exact pixel obligations."""
    raise SystemExit("Use ground_viewport_review.py with explicit --mode, --selector-result, --build-result, --source-result and --run-id; this pixel verifier requires the selected current source view.")


if __name__ == "__main__":
    main()
