"""Partition original launch-center hardware around real per-zone visitor contacts.

@file launch_facility_foregrounds.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61EA.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
All source pixels retain their colour, opacity, component and actual world-cell
owner. Reviewed geometry separates occluding shell/arm from open interiors;
no source pixel is erased, cropped to fit, moved or created. Foreground scope
is the visitor belonging to the actual native zone, not a global scene layer.
"""

import hashlib
import json

from PIL import Image, ImageDraw

from tools import launch_facility_states as status
from tools.launch_facility_foreground_guides import project_cell

base = status.base
ROOT = base.ROOT
MODEL = ROOT / "media/multilayer/source/space/launch-center-foreground-model-v1.json"
STATES = status.STATES
CONTACTS = status.CONTACTS
MATERIALS = status.MATERIALS
FOREGROUND_POLICY = "original_structural_shells_and_near_arm_with_actual_open_cavities_back; per_native_zone_visitor_scope"
MARKED_CELLS = {
    "NE": {"pad": [0, 0], "recovery": [3, 0]},
    "SE": {"pad": [2, 0], "recovery": [2, 3]},
    "SW": {"pad": [3, 2], "recovery": [0, 2]},
    "NW": {"pad": [0, 3], "recovery": [0, 0]},
}


def checked_model(model):
    """Check genuine model ancestry, actual zone contacts and physical partition roles.

    @param model Curated four-view occupied-source contact/region brief.
    @return Selected base recipes after all exact input bindings pass.
    @throws ValueError On source identity, altered contact, actor canvas or region-role drift.
    """
    selection = base.bound_json(model["selected_bases"])
    mapping = base.bound_json(model["state_mapping"])
    checkpoint = base.bound_json(model["status_art_checkpoint"])
    # Occupied partitions extend the same real bodies and native owner model;
    # another source/custody authority cannot inherit their accepted geometry.
    if (model["asset_id"] != base.source.ASSET or model["foreground_policy"] != FOREGROUND_POLICY
            or mapping["selected_bases"] != model["selected_bases"]
            or checkpoint["state_mapping"] != model["state_mapping"]
            or set(model["views"]) != set(base.DEPTH_ORDERS)):
        raise ValueError("launch foreground source/owner model changed")
    status.checked_policy(checkpoint)
    public = json.loads((ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    assets = {row["asset_id"]: row for row in public["assets"]}
    # Diagnostic visitor bounds come from the separate public actor family;
    # an enlarged made-up actor canvas cannot justify broader foreground cuts.
    for asset, geometry in model["separate_actor_canvases"].items():
        if geometry != {key: assets[asset][key] for key in ("native_size", "anchor", "bounds")}:
            raise ValueError("launch foreground separate actor canvas changed")
    for direction, view in model["views"].items():
        native = base.source.geometry(direction)
        recipe = base.bound_json(selection["selected_views"][direction])
        components = base.bound_json(recipe["component_model"])["selected_components"]
        selected = components if direction == "NE" else components[direction]
        if view["native_rotation"] != native["native_rotation"] or set(view["contacts"]) != set(base.ORDER):
            raise ValueError("launch foreground rotation changed")
        # Native custody remains the actual first world cell. Physical art
        # mounting is explicit and must stay inside its original equipment zone.
        for component, contact in view["contacts"].items():
            zone = base.source.component_geometry(component, direction)
            custody = zone["world_cells"][0]
            expected = project_cell(native, custody)
            physical = contact["physical_display_contact"]
            marked = contact["physical_marked_cell"]
            if (contact["native_custody_world_cell"] != custody or contact["native_custody_contact"] != expected
                    or contact["display_offset_from_native_custody"] != [physical[i] - expected[i] for i in (0, 1)]
                    or contact["declared_actor_canvas_box"] != [physical[0] - 32, physical[1] - 32, physical[0] + 32, physical[1] + 16]):
                raise ValueError("launch foreground native custody or physical mounting changed")
            if component == "workshop":
                if physical != selected[component]["native_transform"]["native_reference"]:
                    raise ValueError("launch foreground workshop reference changed")
            elif (marked != MARKED_CELLS[direction].get(component, custody)
                  or marked not in zone["world_cells"] or physical != project_cell(native, marked)):
                raise ValueError("launch foreground marking leaves its actual native zone")
        regions = view["foreground_regions"]
        # Closed rear walls and flat recovery have different physical roles;
        # a self-consistent mask hash cannot fabricate an opening or near arm.
        if (set(regions) != set(base.ORDER)
                or regions["workshop"]["selection"] != "structural_shell"
                or regions["storage"]["selection"] != "structural_shell"
                or regions["recovery"]["selection"] != "absent_flat_plate"
                or regions["pad"]["selection"] != ("near_arm" if direction in ("NE", "NW") else "absent_far_arm")
                or (direction in ("NE", "NW") and len(regions["pad"]["polygon"]) < 3)
                or (direction in ("SE", "SW") and regions["pad"]["polygon"])):
            raise ValueError("launch foreground physical surface roles changed")
        for component, count in (("workshop", 1), ("storage", 4)):
            region = regions[component]
            # Genuine rear views are closed. Visible front views retain their
            # real doorway/bay interiors, never an arbitrary hole through a roof.
            if (len(region["interior_polygons"]) != (count if direction in ("NE", "NW") else 0)
                    or region["interior_palette"] != list(range(1, 19))):
                raise ValueError("launch foreground opening or original material policy changed")
    return selection


def polygon_mask(points, size):
    """Rasterize a reviewed geometry selector without altering original source pixels.

    @param points Native screen vertices for one physical surface/interior.
    @param size Unchanged whole-source canvas dimensions.
    @return Binary diagnostic/partition selector; it is never replacement source alpha.
    """
    mask = Image.new("1", size)
    if points:
        # A selector outside the actual canvas is an invalid source decision,
        # not permission for the rasterizer to silently clip its geometry.
        if len(points) < 3 or any(not (0 <= x < size[0] and 0 <= y < size[1]) for x, y in points):
            raise ValueError("launch foreground selector leaves actual source canvas")
        ImageDraw.Draw(mask).polygon([tuple(point) for point in points], fill=1)
    return mask


def foreground_map(model, direction, whole, components):
    """Assign original visible structural pixels to their own zone's foreground.

    @param model Exact reviewed physical surface/cavity brief.
    @param direction Genuine selected native view.
    @param whole Original indexed state image; colour changes do not move its pixels.
    @param components Original byte-per-pixel component ownership from composition.
    @return Byte mask, per-component foreground counts and retained interior counts.
    """
    regions = model["views"][direction]["foreground_regions"]
    if len(components) != whole.width * whole.height:
        raise ValueError("launch foreground component ownership dimensions changed")
    masks = {}
    # Cavity selectors act only on their own original gray interior material.
    # Structural glazing/cream/ochre remains intact even along opening boundaries.
    for component, region in regions.items():
        if region["selection"] == "structural_shell":
            masks[component] = [polygon_mask(points, whole.size) for points in region["interior_polygons"]]
        elif region["selection"] == "near_arm":
            masks[component] = polygon_mask(region["polygon"], whole.size)
    foreground = bytearray(whole.width * whole.height)
    counts = {name: 0 for name in base.ORDER}
    interiors = {name: 0 for name in base.ORDER}
    order = base.DEPTH_ORDERS[direction]
    # Every visible pixel keeps its existing component/cell. Selection changes
    # only back/front ownership, never the palette value, location or opacity.
    for y in range(whole.height):
        for x in range(whole.width):
            position = y * whole.width + x
            value = whole.getpixel((x, y))
            owner = components[position]
            if bool(value) != bool(owner) or owner not in range(5):
                raise ValueError("launch foreground source and component ownership disagree")
            if not value:
                continue
            component = order[owner - 1]
            region = regions[component]
            selected = False
            if region["selection"] == "structural_shell":
                interior = value in region["interior_palette"] and any(mask.getpixel((x, y)) for mask in masks[component])
                selected = not interior
                interiors[component] += interior
            elif region["selection"] == "near_arm":
                selected = bool(masks[component].getpixel((x, y)))
            if selected:
                foreground[position] = 1
                counts[component] += 1
    return bytes(foreground), counts, interiors


def compose_states(model, direction):
    """Extend accepted whole state pixels with occupied-source foreground ownership.

    @param model Exact occupied-source geometry/region model.
    @param direction Genuine selected facility view.
    @return Unchanged state images, component/cell owners, foreground mask and evidence.
    """
    checked_model(model)
    checkpoint = base.bound_json(model["status_art_checkpoint"])
    images, owners, cells, evidence = status.compose_states(checkpoint, direction)
    foreground, counts, interiors = foreground_map(model, direction, images["ready"], owners)
    evidence.update(foreground_policy=FOREGROUND_POLICY, foreground_sha256=hashlib.sha256(foreground).hexdigest(),
                    foreground_pixels=counts, retained_interiors=interiors)
    if model["occupied_foregrounds_complete"]:
        # A qualified model freezes the actual original-pixel cut, not merely
        # a region label. Rehashing changed geometry needs a new physical review.
        approved = model["views"][direction]
        if (evidence["foreground_sha256"] != approved["approved_foreground_sha256"]
                or counts != approved["approved_foreground_pixels"]
                or interiors != approved["approved_retained_interiors"]):
            raise ValueError("launch occupied source differs from its reviewed original-pixel partition")
    return images, owners, cells, foreground, evidence


def cut_state(whole, cells, foreground, direction, state, frame=0):
    """Partition actual cells into original back/front pixels with fixed anchors.

    @param whole Exact original indexed state image.
    @param cells Original actual-zone cell ownership map.
    @param foreground Qualified per-pixel occupied structural selection.
    @param direction Genuine physical view.
    @param state Actual public facility source state.
    @param frame Logical still frame0..3.
    @return All twenty-four independently observable native source parts.
    """
    native = base.source.geometry(direction)
    # Fixed dimensions and DOS palette prevent alternate canvases from moving
    # native seams or smuggling replacement pixels through a cut operation.
    if (state not in STATES or frame not in range(4) or whole.mode != "P"
            or list(whole.size) != native["native_canvas"]
            or whole.getpalette() != list(base.source.site.art.base.PALETTE_BYTES)
            or len(cells) != whole.width * whole.height or len(foreground) != len(cells)):
        raise ValueError("launch occupied source pose, palette or ownership size changed")
    images = {(tile, part): base.tiles.indexed_canvas((64, 64)) for tile in range(12) for part in base.PARTS}
    # Copy each original pixel once, retaining its actual cell and native
    # coordinate even when the visitor will be drawn between its two parts.
    for y in range(whole.height):
        for x in range(whole.width):
            position = y * whole.width + x
            value, owner, front = whole.getpixel((x, y)), cells[position], foreground[position]
            if bool(value) != bool(owner) or front not in (0, 1) or (front and not value):
                raise ValueError("launch occupied source ownership claims absent hardware")
            if not value:
                continue
            if not 1 <= owner <= 12:
                raise ValueError("launch occupied source has an invalid actual cell")
            ox, oy = native["native_cell_origins"][owner - 1]
            if not (0 <= x - ox < 64 and 0 <= y - oy < 64):
                raise ValueError("launch occupied source pixel leaves its original cell")
            images[owner - 1, "front" if front else "back"].putpixel((x - ox, y - oy), value)
    pieces = []
    # Keep explicit absent parts as real logical positions. A fully occluding
    # closed wall may own a front-only cell; no fake back pixel fills that slot.
    for (tile, part), image in images.items():
        count = sum(value != 0 for value in image.get_flattened_data())
        pieces.append((dict(asset_id=base.source.ASSET, direction=direction, native_layout=native["native_rotation"],
                            state=state, frame=frame, tile_variant=tile, world_cell=native["world_cells"][tile],
                            part=part, native_origin=native["native_cell_origins"][tile], anchor=native["anchor"],
                            foreground_policy=FOREGROUND_POLICY, structurally_absent=count == 0, opaque_pixels=count,
                            native_sha256=hashlib.sha256(image.tobytes()).hexdigest()), image))
    reassemble_state(pieces, cells, foreground, hashlib.sha256(whole.tobytes()).hexdigest(), direction, state, frame)
    return pieces


def reassemble_state(pieces, cells, foreground, expected_hash, direction, state, frame=0):
    """Validate occupied source parts independently against original pixel/zone ownership.

    @param pieces All stored indexed cell/part metadata and images.
    @param cells Exact source actual-zone cell ownership.
    @param foreground Exact reviewed physical structural foreground selection.
    @param expected_hash Immutable accepted whole-state indexed pixel identity.
    @param direction Genuine physical view.
    @param state Actual public source state.
    @param frame Logical still frame0..3.
    @return Whole original state reconstructed without loss, drift or duplication.
    @throws ValueError On changed cuts, anchors, palette, pixels, geometry or coverage.
    """
    native = base.source.geometry(direction)
    keys = [(row["tile_variant"], row["part"]) for row, image in pieces]
    whole = base.tiles.indexed_canvas(tuple(native["native_canvas"]))
    # Coverage and map sizes remain exact even for empty back/front entries;
    # twenty-four duplicate or relabelled files cannot stand in for real cells.
    if (state not in STATES or frame not in range(4) or len(keys) != 24
            or set(keys) != {(tile, part) for tile in range(12) for part in base.PARTS}
            or len(cells) != whole.width * whole.height or len(foreground) != len(cells)):
        raise ValueError("launch occupied source coverage or ownership size changed")
    for row, image in pieces:
        tile, part = row["tile_variant"], row["part"]
        count = sum(value != 0 for value in image.get_flattened_data())
        # Exact native metadata and stored bytes jointly qualify a part.
        # Rehashing moved art cannot authorize another view, zone or state.
        if (row["asset_id"] != base.source.ASSET or row["direction"] != direction
                or row["native_layout"] != native["native_rotation"] or row["state"] != state or row["frame"] != frame
                or row["world_cell"] != native["world_cells"][tile] or row["native_origin"] != native["native_cell_origins"][tile]
                or row["anchor"] != native["anchor"] or row["foreground_policy"] != FOREGROUND_POLICY
                or image.mode != "P" or image.size != (64, 64) or image.getpalette() != list(base.source.site.art.base.PALETTE_BYTES)
                or row["opaque_pixels"] != count or row["structurally_absent"] != (count == 0)
                or row["native_sha256"] != hashlib.sha256(image.tobytes()).hexdigest()):
            raise ValueError("launch occupied stored source geometry, metadata or pixels changed")
        ox, oy = native["native_cell_origins"][tile]
        # Read stored pixels in their original whole-canvas positions. A pixel
        # transferred between back/front must agree with the reviewed cut map.
        for y in range(64):
            for x in range(64):
                value = image.getpixel((x, y))
                if not value:
                    continue
                point = (ox + x, oy + y)
                position = point[1] * whole.width + point[0]
                if (cells[position] != tile + 1 or foreground[position] != (part == "front")
                        or whole.getpixel(point)):
                    raise ValueError("launch occupied source changes physical cell or foreground ownership")
                whole.putpixel(point, value)
    if hashlib.sha256(whole.tobytes()).hexdigest() != expected_hash:
        raise ValueError("launch occupied source loses or changes original state pixels")
    if any(bool(value) != bool(cells[i]) for i, value in enumerate(whole.get_flattened_data())):
        raise ValueError("launch occupied source retains phantom cell ownership")
    return whole
