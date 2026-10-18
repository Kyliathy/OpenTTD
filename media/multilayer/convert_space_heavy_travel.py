"""Register unchanged original gas at the heavy miner's real visible outlets.

@file convert_space_heavy_travel.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65.3b
The heavy model retains two physical nozzles and its original foreground hull.
Reused gas is an explicitly registered original effect, never new generated
hardware, movement, work, cargo or a stopped-craft animation opportunity.
"""
import hashlib
import json

from PIL import Image, ImageDraw

import convert_space_art as art
import convert_space_mining_parts as parts

ASSET = "vehicle.model-21"


def outlet_sources(direction, phase, document):
    """Resolve every visible original nozzle's separately registered gas source.

    @param direction Genuine original heavy-miner heading.
    @param phase Actual authored original gas phase0/1.
    @param document Source catalogue retaining original gas and receiving heavy body.
    @return Ordered candidate/outlet rows with verified original donor ancestry.
    @throws ValueError On absent source, fake hardware or altered original gas geometry.
    """
    family = document["families"][ASSET]
    # Visible source coverage follows actual nozzle occlusion, not a new hardware count. The front-on S original hides the second mouth behind its hopper; all other accepted views expose both mouths.
    if phase not in (0, 1) or direction not in family["directions"]:
        raise ValueError("missing actual heavy travel phase/heading")
    outlets = family.get("travel_outlets", {}).get(direction, [])
    expected = 1 if direction == "S" else 2
    if family.get("physical_outlet_count") != 2 or len(outlets) != expected:
        raise ValueError("heavy travel changes original nozzle visibility/count")
    result = []
    # Each visible physical mouth uses the same genuine original heading and gas phase, with its own measured receiving contact. Repetition is explicit source reuse, never additional generated art.
    for index, outlet in enumerate(outlets):
        field = "travel_sources" if index == 0 else "travel_secondary_sources"
        key = family.get(field, {}).get(str(phase), {}).get(direction)
        if key is None:
            raise ValueError("missing genuine heavy outlet source")
        candidate = parts.registered_component_source(key, direction, document)
        # A hull, role mechanism or relabeled phase cannot become exhaust. Original identity checks additionally preserve prompt, raw bytes, source crop, scale and direction.
        if (candidate["asset_id"] != ASSET or candidate.get("component_kind") != "rear_plume"
                or candidate.get("phase") != phase or candidate.get("instance_of") is None):
            raise ValueError("source is not the declared original heavy gas instance")
        if candidate["native_transforms"][direction]["native_reference"] != outlet["native_reference"]:
            raise ValueError("heavy gas instance misses its original outlet reference")
        result.append((candidate, outlet))
    return result


def travel_signature(direction, phase, document):
    """Bind real nozzle contacts, visibility and unchanged original gas ancestry.

    @param direction Genuine heavy-miner camera heading.
    @param phase Actual authored original gas phase0/1.
    @param document Receiving-body and original-donor source catalogue.
    @return Stable signature over all visible original outlets and their gas parts.
    """
    family = document["families"][ASSET]
    rows = []
    # Every visible outlet contributes its entire physical registration and donor approval; an omitted second mouth or a shifted instance cannot retain this review identity.
    for candidate, outlet in outlet_sources(direction, phase, document):
        rows.append(dict(source=art.registration_signature(candidate, direction), outlet=outlet,
                         original=candidate["instance_of"], donor=candidate["donor_registration"][direction]))
    value = dict(base=family["approved_base_registration"][direction], physical_outlet_count=family["physical_outlet_count"],
                 occlusion=family["travel_occlusion_policy"], instances=rows)
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def travel_pose(direction, phase, document=None):
    """Place genuine gas behind every visible original heavy-miner nozzle.

    @param direction Genuine selected original heading.
    @param phase Actual independent gas phase0/1; no new animation clock.
    @param document Optional detached source definitions for focused refusal checks.
    @return Indexed64x48 pose and individual outlet/contact/protected-pixel evidence.
    @throws ValueError On changed ancestry, floating gas, hidden source or unsafe bounds.
    """
    document = art.read_record() if document is None else document
    family = document["families"][ASSET]
    sources = outlet_sources(direction, phase, document)
    signature = travel_signature(direction, phase, document)
    approved = family.get("approved_travel_registration", {}).get(str(phase), {}).get(direction)
    # A new load/status original must not silently move either existing jet or change which nozzle is occluded after source approval.
    if approved is not None and approved != signature:
        raise ValueError("approved heavy travel registration changed")
    base, original = art.native_view(family["selections"]["free/" + direction], direction, document)
    pose = base.copy()
    evidence = []
    # The original opaque craft occludes every pulse independently. Two visible nozzles require two actual original instances touching their own mouths, even if the effects overlap in projection.
    for candidate, outlet in sources:
        left, top, right, bottom = outlet["bounds"]
        # Invalid effect bounds cannot widen the canvas or authorize clipping a genuine source pulse.
        if not (0 <= left < right <= 64 and 0 <= top < bottom <= 48):
            raise ValueError("heavy gas bounds leave original native canvas")
        rgba, source = art.source_pixels(candidate, direction, [left, top, right, bottom])
        component = art.base.quantize(rgba, {})
        contacts = outlet["contacts"]
        # Every declared contact is an actual retained opaque original nozzle edge. A source label cannot make empty background a physical mounting point.
        if not contacts or any(not (0 <= x < 64 and 0 <= y < 48) or base.getpixel((x, y)) == 0 for x, y in contacts):
            raise ValueError("heavy travel has no actual original outlet contact")
        gas = []
        # Add only actual nontransparent original gas in previously empty space; never replace a nozzle, hopper, gripper, cutter or another opaque hull pixel.
        for y in range(top, bottom):
            for x in range(left, right):
                if base.getpixel((x, y)) == 0 and component.getpixel((x, y)) != 0:
                    gas.append((x, y))
                    pose.putpixel((x, y), component.getpixel((x, y)))
        if not gas:
            raise ValueError("heavy gas instance is hidden by the original craft")
        distance = min(max(abs(x - contact_x), abs(y - contact_y)) for x, y in gas for contact_x, contact_y in contacts)
        if distance != 1:
            raise ValueError("heavy gas does not touch its original outlet")
        evidence.append(dict(name=outlet["name"], original=candidate["instance_of"], donor_registration=candidate["donor_registration"][direction],
                             component=source, bounds=outlet["bounds"], contacts=contacts, contact_distance=distance,
                             added_gas_pixels=len(gas), new_generated_artwork=False, source_pixels_changed=False))
    protected = 0
    # Compare all original opaque hardware and every empty pixel outside the declared outlet volumes. Native presentation cannot repaint the heavy body merely to accommodate a reused effect.
    for y in range(48):
        for x in range(64):
            old, new = base.getpixel((x, y)), pose.getpixel((x, y))
            editable = old == 0 and any(item["bounds"][0] <= x < item["bounds"][2] and item["bounds"][1] <= y < item["bounds"][3] for item in evidence)
            if not editable:
                if old != new:
                    raise ValueError("heavy travel changes protected original hardware")
                protected += 1
    return pose, dict(direction=direction, phase=phase, signature=signature, base=original, outlets=evidence,
                      physical_outlet_count=2, visible_outlet_count=len(evidence), protected_pixels=protected)


def review(output):
    """Publish all sixteen real heavy travel source poses with nozzle-contact proof.

    @param output Fresh directory inside the established native-art proof owner.
    @return Hash-bound source-only review identity; packing and movement remain separate.
    @throws ValueError On absent visible outlets, fake phase changes or unsafe output.
    """
    output = output.resolve()
    # Source review must preserve original rasters and all earlier accepted artifacts rather than replace an existing publication.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("heavy travel review requires a fresh native-art proof directory")
    document = art.read_record()
    directions = document["families"][ASSET]["directions"]
    if directions != ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]:
        raise ValueError("heavy travel review requires eight genuine headings")
    atlas = Image.new("P", (512, 96), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (2048, 544), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    images, evidence = {}, []
    # Resolve both genuine original pulses independently in each viewport. Source reuse remains explicit while every visible outlet passes its own physical contact check.
    for phase in range(2):
        for column, direction in enumerate(directions):
            pose, item = travel_pose(direction, phase, document)
            images[phase, direction] = pose
            evidence.append(item)
            atlas.paste(pose, (64 * column, 48 * phase))
            rgba = pose.convert("RGBA")
            contact.paste(rgba, (256 * column + 96, 272 * phase + 20), rgba)
            large = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(large, (256 * column, 272 * phase + 76), large)
            drawing.text((256 * column + 8, 272 * phase + 4), f"{direction} travel phase{phase}", fill=(235, 235, 235))
    differences = {}
    # Actual gas pulses must remain distinct after the original heavy hull occludes them. An invisible or duplicated second phase is not authored travel animation.
    for direction in directions:
        difference = sum(a != b for a, b in zip(images[0, direction].get_flattened_data(), images[1, direction].get_flattened_data(), strict=True))
        if difference == 0:
            raise ValueError("heavy travel phases are indistinguishable at native scale")
        differences[direction] = difference
    output.mkdir(parents=True)
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "heavy-travel-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, native_cell=[64, 48], anchor=[-32, -32],
                                     pose_count=16, poses=evidence, phase_differences=differences,
                                     physical_outlet_count=2, actual_gas_originals_reused=True, new_generated_artwork=False,
                                     complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)
