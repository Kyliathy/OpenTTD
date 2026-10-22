"""Capture the selected original Ground native observer with explicit current inputs.

@file ground_selected_closure_checks.py Original native writer and pixel/owner scope remain binding.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups79FHB/79FHC/79FHCC/79FHD.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from ground_viewport_capture import run_capture
from ground_industry_pixel_checks import project
from tools import ground_art_handoff as handoff
from tools.ground_payload_binding import reference


def collector_pixels(indices, source_view):
    """Require exact accepted retained source pixels and independently discriminate the actual access view.

    @param indices Actual current native640x360 indexed collector viewport.
    @param source_view Explicit checked original Ground/M1 source view for this current capture.
    @return Original source identity, exact matched construction pixels and wrong-view controls.
    """
    family = source_view.families["facility.vent-collector"]
    poses, atlas = source_view.poses(family["asset_id"])
    data = indices.read_bytes()
    if len(data) != 640 * 360:
        raise ValueError("retained current collector indexed viewport has unexpected dimensions")
    views = []
    # The immutable original funded-world input has construction-stage0
    # machinery. Compare every original back/front cell and all declared access
    # views; only one complete source can explain the actual native pixels.
    for direction in family["directions"]:
        rows = [row for key, row in poses.items() if key[0] == "construction" and key[1] == 0 and key[2] == direction]
        if len(rows) != 18:
            raise ValueError("retained collector source lacks its complete nine-cell back/front pose")
        expected = project(rows, atlas, {"width": 3, "height": 3})
        wrong = sum(data[y * 640 + x] != value for (x, y), value in expected.items())
        views.append({"direction": direction, "source_pixels": len(expected), "different_pixels": wrong})
    matches = [row for row in views if row["different_pixels"] == 0]
    if len(matches) != 1 or matches[0]["direction"] != "SE" or matches[0]["source_pixels"] != 8930:
        raise ValueError("current retained collector pixels do not match the original complete native source")
    return {"asset_id": family["asset_id"], "state": "construction", "frame": 0, "view": "SE", "matched_source_pixels": 8930,
            "source_atlas": reference(handoff.bound(family["authoring"]["native_atlas"])),
            "source_manifest": reference(handoff.bound(family["authoring"]["atlas_manifest"])), "view_controls": views}


def capture(context, mode, configuration, application, source, world, source_view):
    """Delegate the original bounded native observer without historical cache or build discovery.

    @param context Fresh explicit current output/runtime invocation.
    @param mode This original producer's exact registered native capture mechanism.
    @param configuration Actual independently built current native configuration.
    @param application Explicit current viewport executable closed against its selected build.
    @param source Exact retained original review world, never an active continuation.
    @param world Independent original complete-raster comparison contract for this mode.
    @param source_view Explicit checked original Ground/M1 source/pose view.
    @return Actual native process, source/pixel/container provenance and unchanged-input facts.
    """
    if mode not in ('collector',):
        raise ValueError("this original Ground producer does not own the selected capture mode")
    return run_capture(context, mode, configuration, application, source, world, source_view,
                       Path(__file__).relative_to(ROOT).as_posix())


def main():
    """Dispatch only this original producer's current explicitly selected native review scope."""
    from ground_viewport_review import main as selected_main
    selected_main(default_mode='collector', allowed_modes=('collector',))


if __name__ == "__main__":
    main()
