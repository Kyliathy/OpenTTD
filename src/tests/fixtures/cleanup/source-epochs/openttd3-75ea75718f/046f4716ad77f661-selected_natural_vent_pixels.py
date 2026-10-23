"""Match actual native natural-vent survey pixels to original poses using saved owners and fixed field geometry.

@file selected_natural_vent_pixels.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84CD/84C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The original field's top-right cell lies beyond the fixed3x3 collector footprint; its full source pose is verified without changing machinery visibility.
"""
import json
import re
import sys

from PIL import Image

from geothermal_machinery_raster import ROOT, identity
from save_tables import saved_table, read_stream, chunks, geology_snapshot
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_m1_art as packing


def natural_vent_pixels(native):
    """Verify every original opaque vent pixel and complete inactive/cancelled equality at a fixed native field cell.

    @param native Current ordinary paid-world or retained solid-baseline observer report, including all complete unchanged saved owners and actual PNG frames.
    @return Exact natural source/state/frame/range witness; saved field/floor/clock data remain read-only observations, never runtime authorities.
    """
    before = ROOT / native["before"]["path"]
    field = next(row for row in saved_table(before, b"GVNT").values() if row["kind"] == 1)
    tick = saved_table(before, b"DATE")[0]["tick_counter"]; geometry = geology_snapshot(before)
    assert field["layer"] in (1, 3) and field["width"] > 0 and field["height"] > 0
    assert field["x"] + field["width"] <= geometry["width"] and field["y"] + field["height"] <= geometry["height"]
    # Use the fixed natural field edge, beyond the approved3x3 collector
    # footprint, before opening images. Its position is determined by actual
    # GVNT geometry; a building legitimately covering the centre is not a
    # reason to search pixels, crop source art or hide an unsuccessful match.
    x, y = field["x"] + field["width"] - 1, field["y"]
    payload = next(row["payload"] for row in chunks(read_stream(before)[1]) if row["name"] == b"GEOS")
    # The shared geology_snapshot reader validates this exact wire2 header
    # and packed length. Inspect one original saved cell using its documented
    # Underground-then-Deep order; no alternate geology or claim is created.
    plane = 0 if field["layer"] == 1 else 1
    index = plane * geometry["width"] * geometry["height"] + y * geometry["width"] + x
    floor = (payload[39 + index // 4] >> (2 * (index % 4))) & 3
    state = "excavated" if floor == 2 else "surveyed"
    public = json.loads(packing.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "geothermal.vent")
    manifest_path = ROOT / packing.source_binding(family)["atlas_manifest"]; manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    atlas_path = ROOT / manifest["path"]; assert identity(atlas_path)["sha256"] == manifest["sha256"]
    phase = (tick // 8) % family["frames"]
    pose = next(row for row in manifest["sprites"] if (row["state"], row["frame"]) == (state, phase))
    sx, sy, width, height = pose["box"]; source = Image.open(atlas_path).crop((sx, sy, sx + width, sy + height))
    frames = {ROOT / row["path"] for row in native["frames"]}
    active = next(path for path in frames if path.name == "vent-active.png")
    inactive = next(path for path in frames if path.name == "vent-inactive.png")
    cancelled = next(path for path in frames if path.name == "vent-cancelled.png")
    palette_path = ROOT / "src/table/palettes.h"
    text = palette_path.read_text().split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", text)]; assert len(palette) == 256
    # CaptureNativeArtViewport centres the actual first field tile at native
    #2x. Project the chosen saved field cell and full original source anchor
    # from that fixed centre, independently of image colours or decoder trim.
    dx, dy = x - field["x"], y - field["y"]
    left = 320 + ((dy - dx) * 32 + pose["anchor"][0]) * 2
    top = 180 + ((dx + dy) * 16 - 16 + pose["anchor"][1]) * 2
    assert left >= 0 and top >= 0 and left + width * 2 <= 640 and top + height * 2 <= 360
    image = Image.open(active).convert("RGB"); assert image.size == (640, 360)
    a, b = Image.open(inactive).convert("RGB"), Image.open(cancelled).convert("RGB")
    assert a.size == b.size == image.size and a.tobytes() == b.tobytes(), "cancelled natural survey changed native world pixels"
    assert image.tobytes() != a.tobytes(), "active original natural survey produced no native overlay"
    exact = 0
    for offset, colour in enumerate(source.get_flattened_data()):  # Every original opaque point of the complete source canvas must survive the actual native overlay at its saved field/floor/clock pose; no colour tolerance, source mask or pixel search is admitted.
        if colour == 0:
            continue
        for oy in range(2):
            for ox in range(2):
                point = left + (offset % width) * 2 + ox, top + (offset // width) * 2 + oy
                assert image.getpixel(point) == palette[colour], (native["scene"], state, phase, point, image.getpixel(point), palette[colour])
                exact += 1
    assert exact > 0
    return {"asset_id": "geothermal.vent", "state": state, "frame": phase, "native_field": field, "native_tick": tick, "native_floor": floor,
            "native_cell": [x, y, field["layer"]], "source_origin": [left, top], "full_source_canvas": [width, height], "zoom": 1,
            "original_exact_pixels": exact, "active": identity(active), "inactive": identity(inactive), "cancelled": identity(cancelled), "cancelled_rgb_exact_inactive": True,
            "source_manifest": identity(manifest_path), "source_atlas": identity(atlas_path), "palette": identity(palette_path), "public_manifest": identity(packing.PUBLIC),
            "native_offset": family["packed"]["native_offset"], "sprite_count": family["packed"]["sprite_count"],
            "native_selector_source": identity(ROOT / "src/geothermal_gui.cpp"), "native_render_source": identity(ROOT / "src/viewport.cpp")}
