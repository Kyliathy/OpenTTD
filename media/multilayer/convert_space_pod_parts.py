"""Register actual passive-pod tab plates at their two protected original hinge collars.

@file convert_space_pod_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BE
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group64.3
Only actual authored plate pixels enter the declared projected tab volume, including its occlusion over the gray heat shield. Fixed hull pixels outside that volume and all original opacity remain protected; native SpacePod/SpaceDescentJob identity, packets and countdown remain sole owners.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_mining_parts as parts

ASSET = "vehicle.model-18"


def tab_signature(candidate, direction, family):
    """Bind both actual tab-part registrations and their original shell contacts.

    @param candidate Actual own-pod recovery-tab phase with immutable original ancestry.
    @param direction Genuine physical view of both original plates.
    @param family Original shell geometry, protected hinge contacts and projected tab volume.
    @return Stable approval including both separate source rectangles, scales and native roots.
    """
    value = dict(part=parts.part_signature(candidate, direction, family["descent_component_bounds"][direction],
                                           family["approved_base_registration"][direction]),
                 tabs=candidate["native_tab_transforms"][direction], hinges=family["tab_hinges"][direction],
                 edit_pixels=family["tab_edit_pixels"][direction], part_pixels=family["tab_part_edit_pixels"][direction])
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def tab_pose(direction, phase, document=None):
    """Join two actual authored plates at their original collars, protecting fixed hull and opacity.

    @param direction Genuine selected physical pod heading.
    @param phase Actual modestly-open or widely-open mechanical phase0/1, not a new work clock.
    @param document Optional detached source definitions for focused source/refusal checks.
    @return Indexed64x48 pose and original/part/contact/protected-pixel evidence.
    @throws ValueError On absent, foreign, changed, clipped, disconnected or unsafe source geometry.
    """
    document = art.read_record() if document is None else document
    family = document["families"][ASSET]
    # The passive shell requires its own two genuine authored tab phases. A third phase, foreign heading or still image cannot create descent progress or substitute missing source.
    if phase not in (0, 1) or direction not in family["directions"]:
        raise ValueError("missing actual pod phase/heading")
    key = family.get("descent_sources", {}).get(str(phase), {}).get(direction)
    if key is None:
        raise ValueError("missing genuine pod tab source")
    candidate = art.source_candidate(key, document)
    # No probe mechanism, gas effect or differently labelled tab still can supply this passive shell's actual mechanical phase.
    if (candidate["asset_id"] != ASSET or candidate.get("component_kind") != "recovery_tabs"
            or candidate.get("phase") != phase):
        raise ValueError("source is not the declared original pod tab phase")
    # A retained equipment-count failure or unchanged control cell cannot provide a new mechanical phase merely because other views in that original remain useful.
    if direction in candidate.get("unaccepted_directions", {}):
        raise ValueError("unaccepted original pod tab view")
    transforms = candidate.get("native_tab_transforms", {}).get(direction, [])
    hinges = family["tab_hinges"][direction]
    # Exactly two original fittings are assembled independently. A source's extra equipment or unchanged control view cannot authorize a third plate.
    if len(transforms) != 2 or len(hinges) != 2:
        raise ValueError("pod requires exactly two original tab parts")
    editable = {tuple(point) for point in family["tab_edit_pixels"][direction]}
    left, top, right, bottom = family["descent_component_bounds"][direction]
    # Frozen swept space lies inside the native canvas. It never keys source alpha or widens the shell's protected physical surface.
    if (not (0 <= left < right <= 64 and 0 <= top < bottom <= 48) or not editable
            or len(editable) != len(family["tab_edit_pixels"][direction])
            or any(not (left <= x < right and top <= y < bottom) for x, y in editable)):
        raise ValueError("invalid native pod tab swept volume")
    part_pixels = family["tab_part_edit_pixels"][direction]
    # Each actual plate occupies its own projected surface. A source rectangle around one tab cannot paint body fragments into the other tab's neighboring volume.
    if (len(part_pixels) != 2 or any(not points or len({tuple(p) for p in points}) != len(points)
            or any(tuple(p) not in editable for p in points) for points in part_pixels)):
        raise ValueError("invalid individual pod plate volume")
    # Fixed original hinge collars stay foreground. A source rectangle's body edge cannot repaint either joint while supplying an outward moving plate.
    if any(tuple(point) in editable for hinge in hinges for point in hinge["contacts"]):
        raise ValueError("pod tab volume covers its protected hinge collar")
    signature = tab_signature(candidate, direction, family)
    approved = family.get("approved_descent_registration", {}).get(str(phase), {}).get(direction)
    # Later authoring cannot move a plate root, recrop its source or change its swept pixels independently of the selected original shell.
    if approved is not None and approved != signature:
        raise ValueError("approved pod tab registration changed")
    base, original = art.native_view(family["selections"]["free/" + direction], direction, document)
    original_rgb = base.convert("RGB")
    # Near-facing tabs project in front of the original gray heat shield. Only that declared gray surface or transparent space may receive plate pixels; cream cap and orange belt cannot become editable hardware.
    for point in editable:
        colour = original_rgb.getpixel(point)
        if base.getpixel(point) != 0 and (max(colour) > 140 or max(colour) - min(colour) > 45):
            raise ValueError("pod tab volume covers protected original shell hardware")
    pose = base.copy()
    evidence = []
    # Each genuine source plate has its own original hinge reference; uniform resampling cannot distort the pair to compensate for a generator's changed body spacing.
    for index, transform in enumerate(transforms):
        # Only ordinary physical-part extraction and its measured scale/root may vary. Complete original crop/bounds and alpha validation remain the shared source owner's responsibility.
        if set(transform) != {"source_part_box", "source_reference", "native_reference", "uniform_scale"}:
            raise ValueError("pod tab changes complete original view geometry")
        if transform["native_reference"] != hinges[index]["root"]:
            raise ValueError("pod tab misses its original declared hinge")
        source_candidate = copy.deepcopy(candidate)
        source_candidate["native_transforms"][direction].update(transform)
        rgba, source = art.source_pixels(source_candidate, direction, [0, 0, 64, 48])
        component = art.base.quantize(rgba, {})
        rgb = component.convert("RGB")
        added = []
        # Only the declared projected plate volume receives actual source pixels. A near-facing plate may cover its original gray hinge/shield surface; source transparency never cuts a hole through the shell.
        for x, y in part_pixels[index]:
            pixel = component.getpixel((x, y))
            if pixel == 0:
                continue
            colour = rgb.getpixel((x, y))
            # A cream cap or orange belt fragment signals an unsafe part registration. Refuse it instead of recoloring/filtering source pixels into a plausible tab.
            if max(colour) > 170 or max(colour) - min(colour) > 35:
                raise ValueError(f"non-tab source hardware enters swept volume: {direction}/{phase}/{index} {x},{y} {colour}")
            pose.putpixel((x, y), pixel)
            added.append((x, y))
        if not added:
            raise ValueError(f"actual pod tab is hidden: {direction}/{phase}/{index}")
        contacts = hinges[index]["contacts"]
        # Actual newly visible plate pixels must touch this same original opaque hinge, not merely some unrelated point on the capsule's outline.
        if not contacts or any(base.getpixel(tuple(point)) == 0 for point in contacts):
            raise ValueError("invalid original pod hinge contacts")
        distance = min(max(abs(x - cx), abs(y - cy)) for x, y in added for cx, cy in contacts)
        if distance > 1:
            raise ValueError(f"actual pod tab floats away from its hinge: {direction}/{phase}/{index}")
        evidence.append(dict(tab=index, source=source, added_pixels=len(added), hinge=hinges[index], contact_distance=distance))
    # Check every original indexed pixel independently. Appearance never replaces fixed shell/hatch/belt pixels or cuts holes through the original heat shield beneath a moving plate.
    for y in range(48):
        for x in range(64):
            # All original opaque surfaces remain opaque, including under near-facing tabs; a component's background cannot erase the conserved physical shell.
            if base.getpixel((x, y)) != 0 and pose.getpixel((x, y)) == 0:
                raise ValueError("pod tabs erase original shell opacity")
            if (x, y) not in editable:
                if pose.getpixel((x, y)) != base.getpixel((x, y)):
                    raise ValueError("pod tabs change protected original shell pixels")
    return pose, dict(direction=direction, phase=phase, base=original, tabs=evidence, signature=signature,
                      protected_pixels=64 * 48 - len(editable), original_opaque_pixels=sum(p != 0 for p in base.get_flattened_data()))


def review(output):
    """Publish actual two-tab native phase contacts and source/contact evidence.

    @param output Fresh directory below the established repository native-art proof owner.
    @return Hash-bound actual source review, without packing or runtime activation.
    """
    output = output.resolve()
    # Derived proof cannot overwrite originals, earlier selected evidence or shipped packed payloads.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("pod tab review requires a fresh native-art proof directory")
    document = art.read_record()
    directions = document["families"][ASSET]["directions"]
    poses, evidence, differences = {}, [], {}
    # Both real mechanical phases must survive native palette conversion for every genuine physical heading; static aliases are declared separately by the family publisher.
    for direction in directions:
        for phase in (0, 1):
            poses[direction, phase], item = tab_pose(direction, phase, document)
            evidence.append(item)
        difference = sum(a != b for a, b in zip(poses[direction, 0].get_flattened_data(), poses[direction, 1].get_flattened_data(), strict=True))
        if difference < 2:
            raise ValueError("actual pod tab phases are indistinguishable: " + direction)
        differences[direction] = difference
    output.mkdir(parents=True)
    atlas = Image.new("P", (512, 96), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (2048, 544), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    # Native and nearest-enlarged contacts display identical converted pixels; proof labels never enter packaged art.
    for column, direction in enumerate(directions):
        for phase in (0, 1):
            pose = poses[direction, phase]
            atlas.paste(pose, (64 * column, 48 * phase))
            rgba = pose.convert("RGBA")
            contact.paste(rgba, (256 * column + 96, 272 * phase + 20), rgba)
            large = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(large, (256 * column, 272 * phase + 76), large)
            drawing.text((256 * column + 8, 272 * phase + 4), f"{direction} tabs {phase}", fill=(235, 235, 235))
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "pod-tab-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, poses=evidence,
                                     phase_different_pixels=differences, packed=False, runtime=False,
                                     protected_pixels=sum(p["protected_pixels"] for p in evidence),
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Review actual pod tab sources without advancing countdown or creating a recovered shell."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
