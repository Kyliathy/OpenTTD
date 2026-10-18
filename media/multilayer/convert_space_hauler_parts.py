"""Keep original hauler freight behind its reviewed foreground hardware.

@file convert_space_hauler_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups66B/66C/66D
Bay polygons describe source assembly only. Native packets and mass capacity
own quantity; these masks grant no cargo, handling, service or runtime sprites.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_mining_parts as parts
import convert_space_status_parts as status

ASSET = "vehicle.model-17"
SOURCE_KINDS = {"empty": "base", "partial": "hauler-partial", "full": "hauler-full",
                "moving-empty": "hauler-moving-empty", "moving-partial": "hauler-moving-partial",
                "moving-full": "hauler-moving-full", "service": "base", "blocked": "hauler-blocked-empty",
                "blocked-partial": "hauler-blocked-partial", "blocked-full": "hauler-blocked-full"}


def polygon_pixels(points):
    """Rasterize a reviewed native geometry polygon without painting source art.

    @param points Ordered inclusive integer vertices inside the original64x48 canvas.
    @return Native pixel coordinates belonging to this diagnostic/assembly mask.
    @throws ValueError On degenerate or out-of-canvas geometry.
    """
    # A geometry mask may describe only this craft's original canvas. Padding,
    # repeated vertices or an empty polygon cannot silently expand cargo rights.
    if (len(points) < 3 or len({tuple(point) for point in points}) != len(points)
            or any(len(point) != 2 or any(type(value) is not int for value in point)
                   or not (0 <= point[0] < 64 and 0 <= point[1] < 48) for point in points)):
        raise ValueError("invalid hauler bay polygon")
    mask = Image.new("1", (64, 48))
    ImageDraw.Draw(mask).polygon([tuple(point) for point in points], fill=1)
    return {(x, y) for y in range(48) for x in range(64) if mask.getpixel((x, y))}


def bay_signature(family, direction):
    """Bind cargo volume to one selected original body and its fixed world contact.

    @param family Original17 source family containing per-heading bay geometry.
    @param direction Genuine selected engine-viewport heading.
    @return Stable approval over body selection, registration and complete depth policy.
    """
    payload = dict(asset_id=ASSET, direction=direction, native_size=family["public_native_size"],
                   anchor=family["public_anchor"], source=family["selections"]["free/" + direction],
                   base_registration=family["approved_base_registration"][direction],
                   geometry=family["cargo_bays"][direction])
    return hashlib.sha256(json.dumps(payload, sort_keys=True).encode("utf-8")).hexdigest()


def bay_geometry(direction, document=None):
    """Resolve the approved physical bay volume and foreground for actual hauler17.

    @param direction Genuine heading whose empty body was independently reviewed.
    @param document Optional detached provenance record for focused refusal checks.
    @return Editable volume, protected foreground, floor footprint and exact geometry evidence.
    @throws ValueError On unapproved geometry, role/registration drift or unsafe depth ownership.
    """
    document = art.read_record() if document is None else document
    family = document["families"][ASSET]
    # A cargo part cannot change the canvas/world anchor, borrow another role's
    # deck or infer a camera absent from the selected original compass.
    if (family["native_model"] != 17 or family["native_role"] != "orbital_cargo_hauler"
            or family["public_native_size"] != [64, 48] or family["public_anchor"] != [-32, -32]
            or direction not in family["directions"]):
        raise ValueError("hauler bay role or native geometry changed")
    candidate = art.source_candidate(family["selections"]["free/" + direction], document)
    # Floor/air permissions belong to this exact genuine body. A plausible hull
    # with another transform cannot inherit a previously approved cargo mask.
    if (candidate["asset_id"] != ASSET or direction not in candidate["directions"]
            or art.registration_signature(candidate, direction) != family["approved_base_registration"][direction]):
        raise ValueError("hauler bay original registration changed")
    signature = bay_signature(family, direction)
    if signature != family.get("approved_cargo_bay_registration", {}).get(direction):
        raise ValueError("approved hauler bay geometry changed")
    geometry = family["cargo_bays"][direction]
    floor = polygon_pixels(geometry["floor_polygon"])
    footprint = polygon_pixels(geometry["cargo_footprint"])
    foreground = set().union(*(polygon_pixels(points) for points in geometry["foreground_polygons"].values()))
    height = geometry["cargo_height"]
    # All loads use one reviewed physical height. The footprint must contact the
    # real recessed floor; a taller or floating part cannot gain more capacity.
    if (type(height) is not int or height != 4 or not footprint.issubset(floor)
            or tuple(geometry["floor_reference"]) not in footprint):
        raise ValueError("hauler cargo loses its original floor contact")
    volume = {(x, y - rise) for x, y in footprint for rise in range(height + 1)}
    # Project upward only within the native bay. Near rails/end fixtures/head
    # remain in front; cargo may cover the specifically reviewed far-side depth.
    if any(not (0 <= x < 64 and 0 <= y < 48) for x, y in volume):
        raise ValueError("hauler cargo volume leaves its native canvas")
    editable = volume - foreground
    if len(editable) < 20:
        raise ValueError("hauler cargo volume is unreadable")
    return editable, foreground, footprint, dict(direction=direction, geometry=geometry, signature=signature,
                                                base_registration=family["approved_base_registration"][direction],
                                                editable_pixels=[list(point) for point in sorted(editable)],
                                                protected_foreground_pixels=len(foreground))


def review_bays(output):
    """Plot frozen source geometry over the original compass without authoring cargo.

    @param output Fresh directory inside the repository's existing native-art proof home.
    @return Hash-bound eight-view bay/foreground review; source coverage and runtime stay pending.
    """
    output = output.resolve()
    # Preserve immutable originals and earlier reviews. Diagnostic masks must
    # never overwrite either a previous source decision or a shipped payload.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("hauler bay review requires a fresh native-art proof directory")
    document = art.read_record()
    family = document["families"][ASSET]
    contact = Image.new("RGB", (640 * 4, 560 * 2), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    evidence = []
    # Review each genuine camera against its own immutable body; no mirrored
    # geometry or earlier carrier's hardware mask supplies a missing heading.
    for index, direction in enumerate(family["directions"]):
        editable, foreground, _, item = bay_geometry(direction, document)
        base, original = art.native_view(family["selections"]["free/" + direction], direction, document)
        item["base"] = original
        item["occluded_original_pixels"] = sum(base.getpixel(point) != 0 for point in editable)
        item["cargo_air_pixels"] = sum(base.getpixel(point) == 0 for point in editable)
        item["protected_outside_pixels"] = 64 * 48 - len(editable)
        evidence.append(item)
        image = base.convert("RGBA")
        overlay = Image.new("RGBA", image.size)
        overlay_draw = ImageDraw.Draw(overlay)
        # These colors are explanatory overlays, never cargo source pixels:
        # amber is admissible volume; cyan marks original foreground hardware.
        for point in editable:
            overlay_draw.point(point, fill=(255, 175, 25, 105))
        for point in foreground:
            if base.getpixel(point) != 0:
                overlay_draw.point(point, fill=(30, 220, 245, 85))
        combined = Image.alpha_composite(image, overlay).resize((640, 480), Image.Resampling.NEAREST)
        column, row = index % 4, index // 4
        contact.paste(combined, (640 * column, 560 * row + 40), combined)
        drawing.text((640 * column + 12, 560 * row + 8), f"{direction}: amber cargo volume / cyan foreground / height4", fill="white")
    output.mkdir(parents=True)
    contact.save(output / "bay-contact.png")
    report = output / "bay-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, views=evidence,
                                     depth_policy="Authored cargo may occlude only the approved projected bay/far-side depth. Original near hardware and every outside pixel remain exact.",
                                     cargo_originals_authored=False, complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / "bay-contact.png")]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def load_signature(candidate, direction, family):
    """Bind one authored quantity part to the original signed bay and floor contact.

    @param candidate Immutable own17 cargo original with one actual static quantity.
    @param direction Genuine selected physical camera.
    @param family Original hauler source family containing the approved bay geometry.
    @return Stable cargo approval; no cargo count or capacity is encoded here.
    """
    payload = dict(component=art.registration_signature(candidate, direction),
                   load_state=candidate["load_state"], direction=direction,
                   bay=family["approved_cargo_bay_registration"][direction])
    return hashlib.sha256(json.dumps(payload, sort_keys=True).encode("utf-8")).hexdigest()


def load_pose(state, direction, document=None):
    """Place actual original freight within its bay and behind original foreground.

    @param state Partial or full native quantity category, never a unit/commodity assertion.
    @param direction Genuine selected engine-viewport heading.
    @param document Optional detached source record for focused ancestry/geometry refusals.
    @return Indexed64x48 original-body composition and cargo/foreground evidence.
    @throws ValueError On foreign quantity, changed approval, floating cargo or outside pixels.
    """
    document = art.read_record() if document is None else document
    family = document["families"][ASSET]
    if state not in ("partial", "full"):
        raise ValueError("unsupported hauler cargo quantity")
    editable, foreground, _, geometry = bay_geometry(direction, document)
    candidate = art.source_candidate(family["load_sources"][state][direction], document)
    # Only this barge's actual static cargo original may represent its quantity.
    # Another role, a mislabeled full bank or an invented phase supplies no load.
    if (candidate["asset_id"] != ASSET or candidate.get("component_kind") != "hauler_cargo_load"
            or candidate.get("load_state") != state or candidate.get("phase") != 0
            or direction not in candidate["directions"]):
        raise ValueError("source is not the declared original hauler quantity")
    transform = candidate["native_transforms"][direction]
    if transform.get("native_reference") != geometry["geometry"]["floor_reference"]:
        raise ValueError("hauler cargo changed its original floor reference")
    signature = load_signature(candidate, direction, family)
    if signature != family.get("approved_load_registration", {}).get(state, {}).get(direction):
        raise ValueError("approved hauler cargo registration changed")
    base, original = art.native_view(family["selections"]["free/" + direction], direction, document)
    rgba, source = art.source_pixels(candidate, direction, family["native_art_envelope"])
    cargo = art.base.quantize(rgba, {})
    authored = {(x, y) for y in range(48) for x in range(64) if cargo.getpixel((x, y)) != 0}
    # Foreground masking models real depth, not source repair. Every other
    # authored pixel must already fit the frozen physical bay before assembly.
    if authored - editable - foreground:
        raise ValueError("authored hauler cargo leaves its approved bay")
    placed = authored & editable
    # At least60percent of the actual cutout must remain visible. A mostly
    # hidden or tiny misplaced case cannot pass merely by hitting a few pixels.
    if len(placed) < 8 or len(placed) * 5 < len(authored) * 3:
        raise ValueError("actual hauler cargo is unreadable or excessively occluded")
    pose = base.copy()
    # Actual cargo covers only its reviewed floor/far-side depth; near rails,
    # command head and nozzle are copied byte-for-byte from the empty body.
    for point in placed:
        pose.putpixel(point, cargo.getpixel(point))
    changed = 0
    added_opacity = 0
    protected = 0
    # Check the entire native canvas, including originally empty air. Freight
    # may add opacity inside its bay but cannot erase the deck or repaint hull.
    for y in range(48):
        for x in range(64):
            old, new = base.getpixel((x, y)), pose.getpixel((x, y))
            if old != 0 and new == 0:
                raise ValueError("hauler cargo erases original opaque hardware")
            changed += old != new
            added_opacity += old == 0 and new != 0
            if (x, y) not in editable:
                if old != new:
                    raise ValueError("hauler cargo changes protected original hardware")
                protected += 1
    return pose, dict(state=state, direction=direction, actual_authored_phase=0, base=original, component=source,
                      bay_signature=geometry["signature"], signature=signature,
                      authored_pixels=len(authored), placed_pixels=len(placed),
                      foreground_occluded_pixels=len(authored & foreground), changed_pixels=changed,
                      added_cargo_opacity=added_opacity, protected_outside_pixels=protected)


def review_loads(output):
    """Compare empty/partial/full freight in every original camera at native and4x.

    @param output Fresh directory under the existing native-art proof owner.
    @return Exact sixteen-load-source review; motion/blocked/full matrix remain separate.
    """
    output = output.resolve()
    # Source reviews preserve the original raster/receipts and earlier proofs;
    # publishing a fresh contact cannot replace a shipped or approved artifact.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("hauler load review requires a fresh native-art proof directory")
    document = art.read_record()
    family = document["families"][ASSET]
    atlas = Image.new("P", (64 * 8, 48 * 3), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (256 * 8, 272 * 3), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    evidence = []
    quantities = {}
    # Compare all quantities in each genuine camera so masking cannot turn a
    # full load into a partial one or conceal a missing original direction.
    for column, direction in enumerate(family["directions"]):
        poses = {}
        rows = {}
        for row, state in enumerate(("empty", "partial", "full")):
            if state == "empty":
                pose, _ = art.native_view(family["selections"]["free/" + direction], direction, document)
            else:
                pose, item = load_pose(state, direction, document)
                evidence.append(item)
                rows[state] = item
            poses[state] = pose
            atlas.paste(pose, (column * 64, row * 48))
            rgba = pose.convert("RGBA")
            contact.paste(rgba, (column * 256 + 96, row * 272 + 20), rgba)
            large = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(large, (column * 256, row * 272 + 76), large)
            drawing.text((column * 256 + 8, row * 272 + 4), f"{direction} {state} static", fill="white")
        difference = sum(a != b for a, b in zip(poses["partial"].get_flattened_data(),
                                               poses["full"].get_flattened_data(), strict=True))
        # Full means native count at its real capacity, not three cargo units.
        # The symbolic full source must nevertheless show substantially more
        # actual freight than partial after the original foreground is applied.
        if difference < 12 or rows["full"]["placed_pixels"] * 5 < rows["partial"]["placed_pixels"] * 8:
            raise ValueError("hauler partial/full quantities are indistinguishable")
        quantities[direction] = dict(partial_full_differences=difference,
                                     partial_visible_pixels=rows["partial"]["placed_pixels"],
                                     full_visible_pixels=rows["full"]["placed_pixels"])
    output.mkdir(parents=True)
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "load-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, poses=evidence,
                                     actual_load_pose_count=16, compared_empty_body_count=8,
                                     actual_phases_per_quantity=1, quantity_differences=quantities,
                                     complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]),
                                indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def _compose_motion(quantity, direction, phase, body, empty, pulse, source_body, source_pulse):
    """Add already verified gas to an already verified actual-quantity body.

    @param quantity Native empty/partial/full category; never a cargo-unit assertion.
    @param direction Genuine physical camera shared by all verified input images.
    @param phase Original gas phase0 or1, not a new animation clock.
    @param body Actual selected quantity pixels, verified by native_view or load_pose.
    @param empty The same original empty hull, verified by native_view.
    @param pulse Original empty hull plus registered gas, verified by component_pose.
    @param source_body Exact original/static-load evidence from the verified body producer.
    @param source_pulse Exact donor/receiving-contact evidence from the verified gas producer.
    @return Indexed moving pose and evidence that every actual freight/hardware pixel survived.
    @throws ValueError On changed original hardware or gas overlapping actual freight.
    """
    pose = body.copy()
    gas = []
    # Derive only the actual gas addition from the shared empty-body producer.
    # Copying that whole producer would erase freight; any occupied loaded pixel
    # must instead refuse the overlap and retain its original quantity owner.
    for y in range(48):
        for x in range(64):
            old, moving = empty.getpixel((x, y)), pulse.getpixel((x, y))
            if old == moving:
                continue
            if old != 0 or moving == 0:
                raise ValueError("hauler motion changes original opaque hardware")
            if body.getpixel((x, y)) != 0:
                raise ValueError("hauler gas overlaps original cargo")
            pose.putpixel((x, y), moving)
            gas.append([x, y])
    if not gas:
        raise ValueError("actual hauler pulse is empty")
    return pose, dict(quantity=quantity, direction=direction, phase=phase, body=source_body, plume=source_pulse,
                      quantity_body_sha256=hashlib.sha256(body.tobytes()).hexdigest(), gas_pixels=gas,
                      protected_quantity_body_pixels=3072 - len(gas))


def motion_pose(quantity, direction, phase, document=None):
    """Compose one actual nozzle pulse without discarding the selected cargo load.

    @param quantity Actual empty/partial/full native quantity category.
    @param direction Genuine selected physical heading.
    @param phase Original approved gas phase0 or1.
    @param document Optional detached source catalogue for focused refusal checks.
    @return Indexed64x48 moving body and exact original cargo/gas source evidence.
    @throws ValueError On unsupported quantity, changed source/contact or cargo overlap.
    """
    if quantity not in ("empty", "partial", "full"):
        raise ValueError("unsupported hauler motion quantity")
    document = art.read_record() if document is None else document
    family = document["families"][ASSET]
    empty, source_empty = art.native_view(family["selections"]["free/" + direction], direction, document)
    body, source_body = (empty, source_empty) if quantity == "empty" else load_pose(quantity, direction, document)
    pulse, source_pulse = parts.component_pose("rear_plume", direction, phase, document, ASSET)
    return _compose_motion(quantity, direction, phase, body, empty, pulse, source_body, source_pulse)


def review_motion(output):
    """Review both real gas phases over all three original freight quantities.

    @param output Fresh directory under the repository's native-art proof owner.
    @return Hash-bound48-pose motion review; blocked/full-family/runtime remain pending.
    """
    output = output.resolve()
    # An independent motion review preserves immutable originals, earlier load
    # acceptance and shipped sprites rather than overwriting their artifacts.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("hauler motion review requires a fresh native-art proof directory")
    document = art.read_record()
    family = document["families"][ASSET]
    atlas = Image.new("P", (512, 288), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (2048, 1632), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    evidence = []
    differences = {}
    # Within this one conversion, independently verify each original body/load
    # and each donor pulse once, then reuse those exact inputs across quantities.
    # No global cache may hide changed metadata/ancestry in a later conversion.
    for column, direction in enumerate(family["directions"]):
        empty, source_empty = art.native_view(family["selections"]["free/" + direction], direction, document)
        bodies = {"empty": (empty, source_empty)}
        for quantity in ("partial", "full"):
            bodies[quantity] = load_pose(quantity, direction, document)
        pulses = {phase: parts.component_pose("rear_plume", direction, phase, document, ASSET) for phase in (0, 1)}
        for index, (quantity, (body, source_body)) in enumerate(bodies.items()):
            poses = []
            for phase, (pulse, source_pulse) in pulses.items():
                pose, item = _compose_motion(quantity, direction, phase, body, empty, pulse, source_body, source_pulse)
                poses.append(pose)
                evidence.append(item)
                row = index * 2 + phase
                atlas.paste(pose, (column * 64, row * 48))
                rgba = pose.convert("RGBA")
                contact.paste(rgba, (column * 256 + 96, row * 272 + 20), rgba)
                large = rgba.resize((256, 192), Image.Resampling.NEAREST)
                contact.paste(large, (column * 256, row * 272 + 76), large)
                drawing.text((column * 256 + 8, row * 272 + 4), f"{direction} moving-{quantity} {phase}", fill="white")
            difference = sum(a != b for a, b in zip(poses[0].get_flattened_data(), poses[1].get_flattened_data(), strict=True))
            if difference < 2:
                raise ValueError("hauler original motion phases are indistinguishable")
            differences[quantity + "/" + direction] = difference
    output.mkdir(parents=True)
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "travel-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, poses=evidence,
                                     actual_motion_poses=48, original_quantity_bodies=24, actual_gas_components=16,
                                     physical_outlets_per_body=1, phase_differences=differences,
                                     complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]),
                                indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def display_geometry_signature(family, direction):
    """Bind the existing diagnostic face to its immutable original command head.

    @param family Own17 source family with frozen display pixels and body registration.
    @param direction Genuine selected physical camera.
    @return Stable geometry approval, independent of a future status raster.
    """
    payload = dict(asset_id=ASSET, direction=direction, body=family["selections"]["free/" + direction],
                   body_registration=family["approved_base_registration"][direction],
                   bounds=family["status_component_bounds"][direction], edit_pixels=family["status_edit_pixels"][direction])
    return hashlib.sha256(json.dumps(payload, sort_keys=True).encode("utf-8")).hexdigest()


def display_geometry(direction, document):
    """Require the original display mask before any blocked-source composition.

    @param direction Genuine camera whose original diagnostic face was inspected.
    @param document Current or detached source catalogue; no global approval cache.
    @return Frozen editable display coordinates and their exact geometry signature.
    @throws ValueError On changed body, widened display ownership or missing approval.
    """
    family = document["families"][ASSET]
    candidate = art.source_candidate(family["selections"]["free/" + direction], document)
    # The small display belongs to this actual barge. Another role's head or a
    # changed body transform cannot inherit its previously inspected navy mask.
    if (candidate["asset_id"] != ASSET or direction not in candidate["directions"]
            or art.registration_signature(candidate, direction) != family["approved_base_registration"][direction]):
        raise ValueError("hauler display original registration changed")
    signature = display_geometry_signature(family, direction)
    if signature != family.get("approved_display_geometry", {}).get(direction):
        raise ValueError("approved hauler display geometry changed")
    points = family["status_edit_pixels"][direction]
    editable = {tuple(point) for point in points}
    left, top, right, bottom = family["status_component_bounds"][direction]
    # Only the existing face may illuminate. Empty/duplicate/out-of-canvas masks
    # cannot authorize a new lamp, larger screen or a change to surrounding hull.
    if (not editable or len(editable) != len(points) or not (0 <= left < right <= 64 and 0 <= top < bottom <= 48)
            or any(not (left <= x < right and top <= y < bottom) for x, y in editable)):
        raise ValueError("invalid original hauler display pixels")
    return editable, signature


def _compose_blocked(quantity, direction, body, empty, blocked, editable, geometry_signature, source_body, source_display):
    """Apply an actual amber diagnostic face without replacing the quantity body.

    @param quantity Original empty/partial/full native quantity category.
    @param direction Genuine shared physical camera.
    @param body Verified original body with its actual selected cargo quantity.
    @param empty Independently verified original empty body.
    @param blocked Verified original empty body with its authored blocked display.
    @param editable Frozen original navy-face coordinates from display_geometry.
    @param geometry_signature Exact original display/body ownership approval.
    @param source_body Verified actual quantity-body evidence.
    @param source_display Verified original blocked-display source evidence.
    @return Indexed blocked quantity pose and complete foreground/opacity evidence.
    @throws ValueError On non-display changes, cargo overlap or an unreadable/non-amber cue.
    """
    pose = body.copy()
    original_rgb, blocked_rgb = empty.convert("RGB"), blocked.convert("RGB")
    changed = []
    # Check the complete original display producer before copying its changes.
    # A blocked empty hull must never erase cargo or replace unrelated hardware.
    for y in range(48):
        for x in range(64):
            point = x, y
            old, new = empty.getpixel(point), blocked.getpixel(point)
            if point not in editable:
                if old != new:
                    raise ValueError("blocked source changes outside the original hauler display")
                continue
            red, green, blue = original_rgb.getpixel(point)
            # These pixels were inspected as the original dark navy core;
            # lighter shaded edges remain protected with the bezel. Even a new
            # signature cannot turn surrounding steel, cargo or trim into a lamp.
            if old == 0 or blue - red < 30 or blue <= green or blue > 110:
                raise ValueError("hauler status ownership is not the original navy face")
            if body.getpixel(point) != old:
                raise ValueError("hauler display overlaps actual cargo")
            if old == new:
                continue
            red, green, blue = blocked_rgb.getpixel(point)
            # Blocking uses steady amber on the existing screen. Transparent
            # source edges or pale housing cannot masquerade as the status cue.
            if new == 0 or red < 90 or red < green or green < blue + 10:
                raise ValueError("authored hauler blocked cue is not opaque amber")
            pose.putpixel(point, new)
            changed.append([x, y])
    if not changed:
        raise ValueError("actual hauler blocked display is unreadable")
    return pose, dict(quantity=quantity, direction=direction, actual_authored_phase=0,
                      body=source_body, display=source_display, display_geometry_signature=geometry_signature,
                      quantity_body_sha256=hashlib.sha256(body.tobytes()).hexdigest(), changed_display_pixels=changed,
                      protected_quantity_body_pixels=3072 - len(editable), original_opacity_preserved=True)


def blocked_pose(quantity, direction, document=None):
    """Show actual blocking while retaining the original empty/partial/full body.

    @param quantity Original native freight quantity category, never a service state.
    @param direction Genuine selected engine-viewport heading.
    @param document Optional detached original-source definitions for refusal checks.
    @return Indexed64x48 static blocked source and exact cargo/display evidence.
    @throws ValueError On missing approval, foreign source, changed display or lost cargo.
    """
    if quantity not in ("empty", "partial", "full"):
        raise ValueError("unsupported hauler blocked quantity")
    document = art.read_record() if document is None else document
    family = document["families"][ASSET]
    editable, signature = display_geometry(direction, document)
    if direction not in family.get("approved_status_registration", {}).get("blocked", {}).get("0", {}):
        raise ValueError("missing actual hauler blocked source approval")
    empty, source_empty = art.native_view(family["selections"]["free/" + direction], direction, document)
    body, source_body = (empty, source_empty) if quantity == "empty" else load_pose(quantity, direction, document)
    blocked, source_display = status.status_pose("blocked", direction, 0, document, ASSET)
    return _compose_blocked(quantity, direction, body, empty, blocked, editable, signature, source_body, source_display)


def review_blocked(output):
    """Review the actual blocked display over all three original cargo quantities.

    @param output Fresh directory under the repository's existing native-art proof owner.
    @return Hash-bound24-pose blocked review; complete matrix/packing/runtime stay separate.
    """
    output = output.resolve()
    # A new status review preserves all original imagery and dated load/motion
    # acceptance; it cannot overwrite a prior review or distributed payload.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("hauler blocked review requires a fresh native-art proof directory")
    document = art.read_record()
    family = document["families"][ASSET]
    atlas = Image.new("P", (512, 144), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (2048, 816), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    evidence = []
    # Verify each actual display once and compose it over three independently
    # verified quantity bodies within this conversion. No empty-body fallback
    # or global source cache can conceal missing cargo or changed ancestry.
    for column, direction in enumerate(family["directions"]):
        editable, signature = display_geometry(direction, document)
        if direction not in family.get("approved_status_registration", {}).get("blocked", {}).get("0", {}):
            raise ValueError("missing actual hauler blocked source approval")
        empty, source_empty = art.native_view(family["selections"]["free/" + direction], direction, document)
        blocked, source_display = status.status_pose("blocked", direction, 0, document, ASSET)
        for row, quantity in enumerate(("empty", "partial", "full")):
            body, source_body = (empty, source_empty) if quantity == "empty" else load_pose(quantity, direction, document)
            pose, item = _compose_blocked(quantity, direction, body, empty, blocked, editable, signature, source_body, source_display)
            evidence.append(item)
            atlas.paste(pose, (column * 64, row * 48))
            rgba = pose.convert("RGBA")
            contact.paste(rgba, (column * 256 + 96, row * 272 + 20), rgba)
            large = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(large, (column * 256, row * 272 + 76), large)
            drawing.text((column * 256 + 8, row * 272 + 4), f"{direction} blocked-{quantity} static", fill="white")
    output.mkdir(parents=True)
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "blocked-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, poses=evidence,
                                     actual_blocked_poses=24, actual_display_components=8, actual_phases_per_display=1,
                                     unsupported_service_alias="empty", complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]),
                                indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


class HaulerSourceBank:
    """Reuse verified original component images only within one source conversion.

    Every lookup rechecks metadata and immutable source ancestry. There is no
    global image cache, source approval bypass or gameplay quantity authority.
    """

    def __init__(self, document, directions=None):
        """Resolve actual bodies, cargo, pulses and displays once per selected camera.

        @param self Fresh conversion-owned source bank.
        @param document Current approved source catalogue; later changes invalidate this bank.
        @param directions Optional genuine camera subset for focused previews; full derive uses all8.
        @return None; build96actual compositions for the complete eight-camera bank.
        """
        self._signature = hashlib.sha256(json.dumps(document, sort_keys=True).encode("utf-8")).hexdigest()
        self._poses = {}
        family = document["families"][ASSET]
        directions = family["directions"] if directions is None else directions
        # A preview subset can omit unused cameras, but cannot duplicate or
        # invent headings and thereby masquerade as complete source coverage.
        if not directions or len(set(directions)) != len(directions) or any(d not in family["directions"] for d in directions):
            raise ValueError("invalid hauler source-bank cameras")
        # Verify each genuine original input once, then assemble all quantities
        # from those exact pixels. Repeated logical stills/service aliases reuse
        # real components instead of opening the same large image repeatedly.
        for direction in directions:
            body_key = family["selections"]["free/" + direction]
            candidate = art.source_candidate(body_key, document)
            # Shared components do not transfer vehicle identity: the receiving
            # hull must be this hauler's measured original camera.
            if candidate["asset_id"] != ASSET or direction not in candidate["directions"]:
                raise ValueError("hauler source bank received a foreign body")
            empty, source_empty = art.native_view(body_key, direction, document)
            bodies = {"empty": (empty, source_empty)}
            for quantity in ("partial", "full"):
                bodies[quantity] = load_pose(quantity, direction, document)
            pulses = {phase: parts.component_pose("rear_plume", direction, phase, document, ASSET) for phase in (0, 1)}
            editable, signature = display_geometry(direction, document)
            if direction not in family.get("approved_status_registration", {}).get("blocked", {}).get("0", {}):
                raise ValueError("missing actual hauler blocked source approval")
            blocked, source_display = status.status_pose("blocked", direction, 0, document, ASSET)
            for quantity, (body, source_body) in bodies.items():
                source_keys = [body_key]
                if quantity != "empty":
                    source_keys.append(family["load_sources"][quantity][direction])
                kind = "base" if quantity == "empty" else "hauler-" + quantity
                self._poses[kind, 0, direction] = body, source_body, tuple(source_keys)
                for phase, (pulse, source_pulse) in pulses.items():
                    pose, evidence = _compose_motion(quantity, direction, phase, body, empty, pulse, source_body, source_pulse)
                    keys = (*source_keys, family["travel_sources"][str(phase)][direction])
                    self._poses["hauler-moving-" + quantity, phase, direction] = pose, evidence, keys
                pose, evidence = _compose_blocked(quantity, direction, body, empty, blocked, editable, signature, source_body, source_display)
                keys = (*source_keys, family["status_sources"]["blocked"]["0"][direction])
                self._poses["hauler-blocked-" + quantity, 0, direction] = pose, evidence, keys
        self.component_pose_count = len(self._poses)

    def pose(self, kind, phase, direction, document):
        """Read a verified composition while refusing stale metadata or original bytes.

        @param self Bank created for this single conversion and exact source catalogue.
        @param kind Explicit original hauler source kind, including base for service's empty alias.
        @param phase Actual source phase; logical still aliases must already resolve to0.
        @param direction Genuine camera included in this bank.
        @param document Current catalogue whose ownership/approvals must remain exact.
        @return Independent indexed image and evidence copies; no mutable cached data escapes.
        @throws ValueError On changed metadata/ancestry or an absent verified composition.
        """
        signature = hashlib.sha256(json.dumps(document, sort_keys=True).encode("utf-8")).hexdigest()
        if signature != self._signature:
            raise ValueError("hauler source bank metadata changed")
        key = kind, phase, direction
        if key not in self._poses:
            raise ValueError("missing verified hauler source-bank pose")
        pose, evidence, source_keys = self._poses[key]
        # Reuse image decoding only. Source/receipt/reference bytes are checked
        # again so a cached bitmap cannot conceal changed immutable ancestry.
        for source_key in source_keys:
            art.source_candidate(source_key, document)
        return pose.copy(), copy.deepcopy(evidence)


def main():
    """Review only the selected hauler's frozen source geometry.

    @return None; print the repository-local review identity.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument("--loads", action="store_true")
    selection.add_argument("--motion", action="store_true")
    selection.add_argument("--blocked", action="store_true")
    args = parser.parse_args()
    review = review_blocked if args.blocked else review_motion if args.motion else review_loads if args.loads else review_bays
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
