"""Compose reviewed launch-center originals and losslessly cut their real cells.

@file launch_facility_base.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups61CC/61DE.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
This ready/empty source adapter preserves all four genuine views' original pixels, explicit
occlusion and each component's native zone. Native concrete ground stays an
existing renderer input. Occupant/state foregrounds and runtime remain
later gates; empty fronts here describe this base only.
"""

import hashlib
import json

from PIL import Image, ImageDraw

from tools import launch_facility_source as source
from tools import orbital_tile_art as tiles

ROOT = source.site.art.ROOT
ORDER = ("pad", "workshop", "recovery", "storage")
# The same physical areas exchange near/far positions through native quarter
# turns. Machinery behind the workshop cannot be pasted over its near wall.
DEPTH_ORDERS = {
    "NE": ORDER,
    "SE": ("storage", "pad", "workshop", "recovery"),
    "SW": ("storage", "recovery", "workshop", "pad"),
    "NW": ("recovery", "workshop", "pad", "storage"),
}
PARTS = ("back", "front")
EMPTY_FRONT_POLICY = "ready_empty_base_only; actual occupant foregrounds remain Group61E"


def identity(path):
    """Bind one explicit repository input rather than infer a latest version.

    @param path Existing repository-local source or evidence file.
    @return Relative path and exact byte SHA256.
    """
    return dict(path=path.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def bound_json(reference):
    """Read a source document only after its exact identity passes.

    @param reference Explicit repository path and approved SHA256.
    @return Parsed input with no mutation or alternate-file discovery.
    """
    path = ROOT / reference["path"]
    if identity(path) != reference:
        raise ValueError("launch base input identity changed")
    return json.loads(path.read_text(encoding="utf-8"))


def compose(recipe):
    """Compose the empty base from all four reviewed complete native originals.

    @param recipe Curated single-view base recipe with exact component-model identity.
    @return Indexed whole, component/cell ownership maps and measured ancestry/occlusion.
    @throws ValueError On changed model, zone, original, depth policy or missing cell.
    """
    directions = recipe["directions"]
    # Every source pose has one actual native orientation. A multi-view label
    # cannot authorize missing originals or substitute a rotated flat picture.
    if len(directions) != 1 or directions[0] not in DEPTH_ORDERS:
        raise ValueError("launch base requires one actual native view")
    direction = directions[0]
    order = DEPTH_ORDERS[direction]
    # The empty base grants no actor/state art. Its physical near/far ordering
    # follows this actual orientation rather than a caller-supplied substitute.
    if (recipe["asset_id"] != source.ASSET
            or recipe["state"] != "ready_empty" or recipe["frame"] != 0
            or recipe["composition_order"] != list(order)
            or recipe["foreground_policy"] != EMPTY_FRONT_POLICY):
        raise ValueError("launch base pose or reviewed depth policy changed")
    model = bound_json(recipe["component_model"])
    if direction == "NE":
        selected_components = model["selected_components"]
        ground_model = model
    else:
        selected_components = model["selected_components"].get(direction, {})
        predecessor = bound_json(model["selected_ne_base"])
        ground_model = bound_json(predecessor["component_model"])
    if set(selected_components) != set(ORDER):
        raise ValueError("launch base requires all four selected components")
    expected_ground = dict(owner="DrawTile_Station", sprite="SPR_CONCRETE_GROUND", cells=12)
    # Existing native ground supplies every station cell; generated hardware
    # cannot replace that renderer owner or create a different footprint.
    if (recipe["native_ground"] != expected_ground or ground_model["ground"]["owner"] != expected_ground["owner"]
            or ground_model["ground"]["sprite"] != expected_ground["sprite"]):
        raise ValueError("launch base native ground ownership changed")
    native = source.geometry(direction)
    whole = tiles.indexed_canvas(tuple(native["native_canvas"]))
    owners = bytearray(whole.width * whole.height)
    cell_owners = bytearray(len(owners))
    evidence = dict(components={}, occlusions={}, source_pixels={}, visible_pixels={})
    # Components retain their own complete planes. Only ordinary foreground
    # overlap hides pixels in the composite; no pixel is erased to fit a zone.
    for component_index, component in enumerate(order, 1):
        selected = selected_components[component]
        candidate = bound_json(selected["generation_receipt"])
        # A plausible transform is insufficient: only the exact selected,
        # contact-reviewed original may contribute machinery to this base.
        if (candidate["key"] != selected["key"] or candidate["component"] != component
                or candidate["directions"] != [direction] or candidate["state"] != "ready_empty"
                or candidate["status"] != "selected_component"
                or candidate["contact_review"]["status"] != "approved"
                or candidate["model_review"]["status"] != "approved_component_model"
                or candidate["native_transform"] != selected["native_transform"]
                or dict(path=candidate["path"], sha256=candidate["sha256"]) != selected["original"]):
            raise ValueError("launch base component selection changed")
        plane, registration = source.registered_component(candidate)
        zone = source.component_geometry(component, direction)
        # Later-view originals carry an independent reviewed physical turn.
        # Relabeling NE hardware cannot count as genuine rotated source art.
        if direction != "NE" and candidate["physical_view"]["native_rotation"] != native["native_rotation"]:
            raise ValueError("launch base component physical rotation changed")
        actual_cells = [native["world_cells"].index(cell) for cell in zone["world_cells"]]
        evidence["components"][component] = dict(generation_receipt=selected["generation_receipt"], registration=registration)
        count = 0
        # Each visible pixel belongs to a real cell in its own equipment zone.
        # The nearest eligible floor contact supplies deterministic cell seams.
        for y in range(plane.height):
            for x in range(plane.width):
                value = plane.getpixel((x, y))
                if not value:
                    continue
                position = y * plane.width + x
                previous = owners[position]
                if previous:
                    pair = f"{order[previous - 1]}_behind_{component}"
                    evidence["occlusions"][pair] = evidence["occlusions"].get(pair, 0) + 1
                whole.putpixel((x, y), value)
                owners[position] = component_index
                cell_owners[position] = actual_cells[tiles.tile_owner(x, y, zone)] + 1
                count += 1
        evidence["source_pixels"][component] = count
    counts = [cell_owners.count(tile + 1) for tile in range(12)]
    if not all(counts):
        raise ValueError("launch base hides a complete actual native cell")
    evidence.update(native_sha256=hashlib.sha256(whole.tobytes()).hexdigest(),
                    component_ownership_sha256=hashlib.sha256(owners).hexdigest(),
                    cell_ownership_sha256=hashlib.sha256(cell_owners).hexdigest(),
                    visible_pixels={name: owners.count(i + 1) for i, name in enumerate(order)},
                    pixels_per_cell=counts, native_ground=expected_ground,
                    foreground_policy=EMPTY_FRONT_POLICY)
    return whole, bytes(owners), bytes(cell_owners), evidence


def cut_cells(whole, cell_owners, direction="NE"):
    """Store every visible base pixel exactly once in its original native zone.

    @param whole Reviewed DOS-indexed complete ready/empty base.
    @param cell_owners Exact byte-per-pixel native ownership map from composition.
    @param direction Actual native view; default preserves the accepted NE contract.
    @return Twenty-four metadata/image pairs, including explicit empty base fronts.
    """
    native = source.geometry(direction)
    # Canvas/palette and ownership lengths protect fixed anchors and prevent
    # alternate image layouts from masquerading as this twelve-cell model.
    if (whole.mode != "P" or list(whole.size) != native["native_canvas"]
            or whole.getpalette() != list(source.site.art.base.PALETTE_BYTES)
            or len(cell_owners) != whole.width * whole.height):
        raise ValueError("launch base canvas, palette or ownership changed")
    images = {(tile, part): tiles.indexed_canvas((64, 64)) for tile in range(12) for part in PARTS}
    # Cut positions follow actual cell ownership, including overlaps. Copying
    # a full machine onto each tile would duplicate hardware and corrupt depth.
    for y in range(whole.height):
        for x in range(whole.width):
            value = whole.getpixel((x, y))
            owner = cell_owners[y * whole.width + x]
            if bool(value) != bool(owner):
                raise ValueError("launch base pixel and ownership disagree")
            if not value:
                continue
            if not 1 <= owner <= 12:
                raise ValueError("launch base pixel has an invalid native cell")
            ox, oy = native["native_cell_origins"][owner - 1]
            # Zone ownership must also fit the native canvas; a corrupt map
            # cannot shift pixels into a cell merely to make coverage pass.
            if not (0 <= x - ox < 64 and 0 <= y - oy < 64):
                raise ValueError("launch base pixel leaves its cell canvas")
            images[owner - 1, "back"].putpixel((x - ox, y - oy), value)
    pieces = []
    # Retain explicit source positions even when this empty base has no front
    # hardware; later actor/state cuts cannot inherit invented foreground art.
    for (tile, part), image in images.items():
        count = sum(value != 0 for value in image.get_flattened_data())
        if part == "back" and count == 0:
            raise ValueError("launch base omits an actual cell back")
        row = dict(asset_id=source.ASSET, direction=direction, native_layout=native["native_rotation"], state="ready_empty", frame=0,
                   tile_variant=tile, world_cell=native["world_cells"][tile], part=part,
                   native_origin=native["native_cell_origins"][tile], anchor=native["anchor"],
                   structurally_absent=count == 0, opaque_pixels=count,
                   native_sha256=hashlib.sha256(image.tobytes()).hexdigest())
        pieces.append((row, image))
    reassemble(pieces, cell_owners, hashlib.sha256(whole.tobytes()).hexdigest(), direction)
    return pieces


def reassemble(pieces, cell_owners, expected_hash, direction="NE"):
    """Verify stored cell parts independently against visible original ownership.

    @param pieces All actual ready/empty cell/back/front metadata and images.
    @param cell_owners Original per-pixel actual-zone cell assignments.
    @param expected_hash Exact selected complete indexed base pixel SHA256.
    @param direction Actual native view; default preserves the accepted NE contract.
    @return Reassembled whole with no duplication, seam drift or pixel loss.
    @throws ValueError On missing entries, corrupted bytes, pose, palette or ownership.
    """
    native = source.geometry(direction)
    expected = {(tile, part) for tile in range(12) for part in PARTS}
    keys = [(row["tile_variant"], row["part"]) for row, image in pieces]
    # Exact entries matter even for empty fronts; duplicate or absent parts
    # must not be counted as twenty-four valid native source positions.
    if len(keys) != 24 or set(keys) != expected:
        raise ValueError("launch base has missing or duplicate cell parts")
    whole = tiles.indexed_canvas(tuple(native["native_canvas"]))
    if len(cell_owners) != whole.width * whole.height:
        raise ValueError("launch base ownership dimensions changed")
    # Stored rows must agree with this physical orientation and its real
    # world-cell origins, including the two non-square footprint layouts.
    for row, image in pieces:
        tile, part = row["tile_variant"], row["part"]
        count = sum(value != 0 for value in image.get_flattened_data())
        # Metadata and stored pixels must jointly preserve the native pose,
        # geometry and source identity; a locally valid checksum is insufficient.
        if (row["asset_id"] != source.ASSET or row["direction"] != direction or row["native_layout"] != native["native_rotation"]
                or row["state"] != "ready_empty" or row["frame"] != 0
                or row["world_cell"] != native["world_cells"][tile]
                or row["native_origin"] != native["native_cell_origins"][tile]
                or row["anchor"] != native["anchor"] or image.mode != "P" or image.size != (64, 64)
                or image.getpalette() != list(source.site.art.base.PALETTE_BYTES)
                or row["opaque_pixels"] != count or row["structurally_absent"] != (count == 0)
                or hashlib.sha256(image.tobytes()).hexdigest() != row["native_sha256"]
                or (part == "front" and count != 0) or (part == "back" and count == 0)):
            raise ValueError("launch base stored part identity, geometry or pixels changed")
        ox, oy = native["native_cell_origins"][tile]
        # Reading actual stored coordinates catches moved seams, duplicated
        # pixels and cross-zone sorting even when local metadata was rehashed.
        for y in range(64):
            for x in range(64):
                value = image.getpixel((x, y))
                if not value:
                    continue
                point = (ox + x, oy + y)
                position = point[1] * whole.width + point[0]
                if cell_owners[position] != tile + 1 or whole.getpixel(point):
                    raise ValueError("launch base cuts change actual cell ownership")
                whole.putpixel(point, value)
    if hashlib.sha256(whole.tobytes()).hexdigest() != expected_hash:
        raise ValueError("launch base reassembly differs from original indexed pixels")
    return whole


def geometry_review(whole, direction="NE"):
    """Overlay genuine source pixels on a labelled geometry-only floor reference.

    @param whole Complete ready/empty base with transparent DOS index0.
    @param direction Actual native view whose twelve ground outlines are reviewed.
    @return RGB review; outlines are diagnostic guides and never source ground art.
    """
    native = source.geometry(direction)
    review = whole.convert("RGBA")
    background = Image.new("RGBA", whole.size, "#29323c")
    draw = ImageDraw.Draw(background)
    # Outlines expose every actual station cell and any misplaced machinery.
    # The existing game renderer supplies concrete, not these guide colours.
    for x, y in native["world_cells"]:
        nx = native["floor_origin"][0] + 32 * (y - x)
        ny = native["floor_origin"][1] + 16 * (x + y)
        draw.polygon([(nx, ny), (nx + 32, ny + 16), (nx, ny + 32), (nx - 32, ny + 16)],
                     fill="#69747a", outline="#a0b0b8")
    background.alpha_composite(review)
    return background.convert("RGB")
