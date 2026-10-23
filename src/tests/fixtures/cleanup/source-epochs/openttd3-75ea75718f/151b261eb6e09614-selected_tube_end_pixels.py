"""Verify original end-cap pixels already visible in the accepted paid joined topology fixture.

@file selected_tube_end_pixels.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84CC.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Fixed original branch geometry and logged native neighbour power locate the witness before opening images; no world or bitmap is edited.
"""
import json
import re
from pathlib import Path
import sys

from PIL import Image

from geothermal_machinery_raster import ROOT, identity
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_m1_art as packing


def atlas_for(asset, public):
    """Resolve an immutable original through the existing public source/packing owner.

    @param asset Existing exact tube source family identity.
    @param public Sole current public sprite-family manifest.
    @return Source manifest path, complete manifest and actual indexed source image after hash verification.
    """
    family = next(row for row in public["assets"] if row["asset_id"] == asset)
    path = ROOT / packing.source_binding(family)["atlas_manifest"]; manifest = json.loads(path.read_text(encoding="utf-8"))
    assert identity(ROOT / manifest["path"])["sha256"] == manifest["sha256"]
    return path, manifest, Image.open(ROOT / manifest["path"])


def end_cap_pixels(report, public, packed, loaded_base):
    """Verify the actual retained X-arm endpoint against both original source parts at both native zooms.

    @param report Current joined model10/13 native travel observation from its exact accepted writer.
    @param public Existing authoritative public family IDs/ranges/source bindings.
    @param packed Existing immutable decoded payload pose table; no new sprite allocation occurs.
    @param loaded_base Independently derived common native loaded base from actual capsule image/public pose pairs.
    @return Four exact cap witnesses, including source points that an uncapped straight casing cannot explain.
    """
    assert report["layout"] == 1
    view = next(row for row in report["native"]["views"] if row["view"] == 3); assert view["mask"] == 3
    neighbour = next(row for row in view["rims"] if (row["dx"], row["dy"], row["mask"]) == (-32, 16, 1))
    # The unchanged accepted writer pays for X arms x30..34 at y28. From
    # its crossing(32,28), the logged collinear X neighbour(33,28) and actual
    # terminal(34,28) share the same native branch/source. Their SW mouth is
    # open. No actor, camera or fabricated grant supplies the power state.
    state = "powered" if neighbour["energized"] else "unpowered"
    cap_path, cap, cap_image = atlas_for("tube.end", public)
    base_path, base, base_image = atlas_for("tube.straight", public)
    palette_path = ROOT / "src/table/palettes.h"
    text = palette_path.read_text().split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", text)]
    assert len(palette) == 256
    uncapped = Image.new("P", (64, 31), 0)
    for part in ("back", "front"):  # The independent uncapped source composition proves a cap witness cannot be supplied by the plain X casing beneath it.
        pose = next(row for row in base["sprites"] if (row["state"], row["direction"], row["part"]) == (state, "NE", part))
        x, y, width, height = pose["box"]; piece = base_image.crop((x, y, x + width, y + height))
        uncapped.paste(piece, (0, 0), piece.point([0] + [255] * 255, mode="L"))
    witnesses = []
    for part in ("back", "front"):  # Both full original cap parts retain every opaque point; neither a hand-picked colour nor the shared straight shell establishes this family.
        pose = next(row for row in cap["sprites"] if (row["state"], row["direction"], row["part"]) == (state, "SW", part))
        x, y, width, height = pose["box"]; assert (width, height) == (64, 31)
        source = cap_image.crop((x, y, x + width, y + height))
        points = [(offset % width, offset // width, palette[index]) for offset, index in enumerate(source.get_flattened_data()) if index]
        distinct = sum(rgb != palette[uncapped.getpixel((px, py))] for px, py, rgb in points); assert distinct > 0
        native_pose = next(row for row in packed["sprites"] if (row["asset_id"], row["state"], row["direction"], row["part"]) == ("tube.end", state, "SW", part))
        for zoom, scale in ((2, 1), (1, 2)):  # Native viewport geometry fixes the entire endpoint canvas before reading pixels; the source, camera and original branch cannot move to improve a match.
            frame_ref = next(row["file"] for row in view["frames"] if (row["zoom"], row["repeat"]) == (zoom, 0))
            left = 320 + (-64 + pose["anchor"][0]) * scale; top = 180 + (32 - 16 + pose["anchor"][1]) * scale
            assert 0 <= left and left + width * scale <= 640 and 0 <= top and top + height * scale <= 360
            image = Image.open(ROOT / frame_ref["path"]).convert("RGB"); assert image.size == (640, 360)
            for px, py, rgb in points:  # Every authored point must survive actual native drawing, including points visibly distinct from the uncapped straight source; no masking, tolerance, clipping or pixel search is allowed.
                for dy in range(scale):
                    for dx in range(scale):
                        assert image.getpixel((left + px * scale + dx, top + py * scale + dy)) == rgb, (report["model"], zoom, part, px, py)
            witnesses.append({"asset_id": "tube.end", "state": state, "direction": "SW", "part": part, "zoom": zoom, "frame": frame_ref,
                              "expected_packed_sprite": loaded_base + native_pose["native_offset"], "source_origin": [left, top], "full_source_canvas": [width, height],
                              "original_exact_pixels": len(points) * scale * scale, "distinct_from_uncapped_straight_pixels": distinct * scale * scale,
                              "native_neighbour_state": neighbour, "native_endpoint_offset": [-64, 32],
                              "source_manifest": identity(cap_path), "source_atlas": identity(ROOT / cap["path"]), "palette": identity(palette_path),
                              "uncapped_manifest": identity(base_path), "fixture_source": identity(ROOT / "src/tests/transport_tube_travel_sprite_load.cpp")})
    return witnesses
