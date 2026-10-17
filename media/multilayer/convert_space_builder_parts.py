"""Assemble actual manipulator phases on the protected native construction carrier.

@file convert_space_builder_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BD
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63C.2
Only the declared swept arm volume changes. Native Construct/site admission, materials and paid work remain the sole animation/progress owners.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_mining_parts as parts

ASSET = "vehicle.model-20"


def builder_signature(candidate, direction, family):
    """Bind the original moving arm source and its exact protected-body geometry.

    @param candidate Actual model20 manipulator source, with a genuine authored phase.
    @param direction Genuine viewport heading of the physical mechanism.
    @param family Selected carrier definition, fixed mounts and editable swept volume.
    @return Stable approval over source registration, original body and every editable pixel.
    """
    value = dict(part=parts.part_signature(candidate, direction, family["builder_component_bounds"][direction],
                                           family["approved_base_registration"][direction]),
                 edit_pixels=family["builder_edit_pixels"][direction], mounts=family["builder_mounts"][direction])
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def builder_pose(direction, phase, document=None):
    """Replace original moving silver arm segments without repainting the carrier.

    @param direction Genuine selected original physical heading.
    @param phase Actual reaching or fastening source phase0/1; never a work increment.
    @param document Optional detached source definition for focused refusal checks.
    @return Indexed64x48 pose and exact source/mount/protected-pixel evidence.
    @throws ValueError On missing, foreign or changed source or unsafe mechanism geometry.
    """
    document = art.read_record() if document is None else document
    family = document["families"][ASSET]
    # Both independent mechanical phases and an actual selected heading are required; a still or neighboring picture cannot conceal missing source coverage.
    if phase not in (0, 1) or direction not in family["directions"]:
        raise ValueError("missing actual builder phase/heading")
    key = family.get("builder_sources", {}).get(str(phase), {}).get(direction)
    if key is None:
        raise ValueError("missing genuine builder source heading")
    candidate = art.source_candidate(key, document)
    # The carrier must use its own actual manipulators, rather than borrowing a drill/scanner or relabeling another phase.
    if (candidate["asset_id"] != ASSET or candidate.get("component_kind") != "front_builder"
            or candidate.get("phase") != phase):
        raise ValueError("source is not the declared original builder mechanism")
    left, top, right, bottom = family["builder_component_bounds"][direction]
    editable = {tuple(point) for point in family["builder_edit_pixels"][direction]}
    # The swept volume is a frozen physical part of the original carrier, never permission to widen edits into the bed or rear hardware.
    if (not (0 <= left < right <= 64 and 0 <= top < bottom <= 48) or not editable
            or len(editable) != len(family["builder_edit_pixels"][direction])
            or any(not (left <= x < right and top <= y < bottom) for x, y in editable)):
        raise ValueError("invalid native builder swept volume")
    signature = builder_signature(candidate, direction, family)
    approved = family.get("approved_builder_registration", {}).get(str(phase), {}).get(direction)
    # Approval binds the original mounts and all editable pixels; later source authoring cannot move the arm joint or authorize repainting the chassis.
    if approved is not None and signature != approved:
        raise ValueError("approved builder registration changed")
    base, original = art.native_view(family["selections"]["free/" + direction], direction, document)
    rgba, source = art.source_pixels(candidate, direction, [0, 0, 64, 48])
    component = art.base.quantize(rgba, {})
    pose = base.copy()
    # The original folded arms must disappear within their own swept volume, including source transparency, so extending a wrist cannot leave ghost grippers. Every other original pixel stays exact.
    for x, y in editable:
        pose.putpixel((x, y), component.getpixel((x, y)))
    changed = sum(a != b for a, b in zip(base.get_flattened_data(), pose.get_flattened_data(), strict=True))
    if changed < 4:
        raise ValueError("actual builder mechanism is unreadable at native scale")
    # Each declared original mounting joint must still have actual silver/gray mechanism pixels touching it. A floating gripper cannot pass on source coverage alone.
    for mount_x, mount_y in family["builder_mounts"][direction]:
        nearby = [(x, y) for x, y in editable if max(abs(x - mount_x), abs(y - mount_y)) <= 1]
        if not any(pose.getpixel(point) != 0 for point in nearby):
            raise ValueError(f"actual builder arm misses its original mount: {direction}/{phase} {mount_x},{mount_y}")
    protected = 0
    # Compare all original pixels outside the actual swept volume, including transparent space and the cargo bed between the two arms.
    for y in range(48):
        for x in range(64):
            if (x, y) not in editable:
                if pose.getpixel((x, y)) != base.getpixel((x, y)):
                    raise ValueError("builder mechanism changes protected original hardware")
                protected += 1
    return pose, dict(direction=direction, phase=phase, base=original, component=source, signature=signature,
                      bounds=[left, top, right, bottom], edit_pixels=family["builder_edit_pixels"][direction],
                      mounts=family["builder_mounts"][direction], changed_pixels=changed, protected_pixels=protected)


def review(output):
    """Publish both actual arm phases with exact native and enlarged source contacts.

    @param output Fresh directory under the established native-art proof owner.
    @return Hash-bound source review; packing and runtime remain separate gates.
    """
    output = output.resolve()
    # Review evidence preserves earlier selected proof and cannot overwrite an original or a shipped runtime payload.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("builder review requires a fresh native-art proof directory")
    document = art.read_record()
    directions = document["families"][ASSET]["directions"]
    images, evidence, differences = {}, [], {}
    # Every genuine heading resolves both real source phases independently; repeated static pictures cannot stand in for mechanism motion.
    for direction in directions:
        for phase in (0, 1):
            images[direction, phase], item = builder_pose(direction, phase, document)
            evidence.append(item)
        difference = sum(a != b for a, b in zip(images[direction, 0].get_flattened_data(), images[direction, 1].get_flattened_data(), strict=True))
        if difference < 4:
            raise ValueError("actual builder phases are indistinguishable: " + direction)
        differences[direction] = difference
    output.mkdir(parents=True)
    atlas = Image.new("P", (512, 96), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (2048, 544), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    # Both contact scales show identical converted pixels; review text never enters the atlas or gameplay art.
    for column, direction in enumerate(directions):
        for phase in (0, 1):
            pose = images[direction, phase]
            atlas.paste(pose, (64 * column, 48 * phase))
            rgba = pose.convert("RGBA")
            contact.paste(rgba, (256 * column + 96, 272 * phase + 20), rgba)
            large = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(large, (256 * column, 272 * phase + 76), large)
            drawing.text((256 * column + 8, 272 * phase + 4), f"{direction} arm phase {phase}", fill=(235, 235, 235))
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "builder-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, poses=evidence,
                                     phase_different_pixels=differences,
                                     protected_pixels=sum(item["protected_pixels"] for item in evidence),
                                     complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Review actual construction-arm sources without changing native work or shipped sprites."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
