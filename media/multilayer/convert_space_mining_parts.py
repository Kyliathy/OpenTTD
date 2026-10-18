"""Assemble actual authored mining mechanisms and travel plumes over protected original native craft pixels.

@file convert_space_mining_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups63.3/63B.2/63C.2/65.3b/65B.3a/66C.1
Original tool pixels alone change inside a declared physical mechanism; this source converter owns no mining work, body lease, cargo or runtime sprite.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art

ASSET = "vehicle.model-15"


def part_signature(candidate, direction, bounds, base_signature, outlet_contacts=None):
    """Bind authored mechanism registration and the original protected base.

    @param candidate Exact actual source for this mechanism phase.
    @param direction Genuine physical heading.
    @param bounds Declared native mechanism replacement rectangle.
    @param base_signature Approved original body registration.
    @param outlet_contacts Optional explicit original rear-edge pixels for a registered shared pulse; None preserves existing original-part signatures.
    @return Immutable diagnostic signature; it grants no source or runtime acceptance.
    """
    value = dict(source=art.registration_signature(candidate, direction), bounds=bounds, base=base_signature)
    if outlet_contacts is not None:
        value["outlet_contacts"] = outlet_contacts
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def registered_component_source(key, direction, document):
    """Resolve an actual original or a strictly registered instance of the same authored gas effect.

    @param key Explicit original or registered-instance key; an instance never claims new generated artwork.
    @param direction Same genuine physical heading in the original and its registered effect instance.
    @param document Original source catalogue with approved donor and receiving body registration.
    @return Exact original source pixels with only the instance's declared native outlet changed.
    @throws ValueError On a foreign part, changed source geometry/ancestry or absent original approval.
    """
    candidate = art.source_candidate(key, document)
    instance = candidate.get("instance_of")
    if instance is None:
        return candidate
    donor = art.source_candidate(instance["candidate"], document)
    # Only the actual blue gas effect may be registered on the distinct robot/hauler/survey/carrier/heavy bodies. Each receiver freezes its own original rear-edge contacts; one-nozzle16/17 gain no builder20 work, miner21 hardware or service admission from this source reuse.
    if (candidate["asset_id"] not in ("vehicle.model-16", "vehicle.model-17", "vehicle.model-19", "vehicle.model-20", "vehicle.model-21") or instance["source_asset_id"] != ASSET
            or donor["asset_id"] != ASSET or donor.get("instance_of") is not None
            or candidate.get("component_kind") != "rear_plume" or donor.get("component_kind") != "rear_plume"
            or candidate.get("phase") != donor.get("phase")):
        raise ValueError("unsupported shared original Space effect")
    # Registered effect instances retain the actual original prompt/tool/raw/receipt/input identities. Target registration is not a new generation receipt or a source-image edit.
    for field in ("path", "sha256", "generation_receipt", "references", "actual_size", "prompt", "tool", "transparent_background"):
        if candidate[field] != donor[field]:
            raise ValueError("shared effect original ancestry changed")
    # The gas retains its real heading and exact source crop, physical subpart, source root and scale. Only its outlet point on the receiving original craft may differ; no rotation, mirror or guessed alpha repair is permitted.
    if direction not in donor["native_transforms"] or direction not in candidate["native_transforms"]:
        raise ValueError("missing genuine shared effect heading")
    original_transform = dict(donor["native_transforms"][direction])
    instance_transform = dict(candidate["native_transforms"][direction])
    original_transform.pop("native_reference", None)
    instance_transform.pop("native_reference", None)
    if original_transform != instance_transform:
        raise ValueError("shared effect changes original source geometry")
    family = document["families"][ASSET]
    phase = str(donor["phase"])
    approved = family.get("approved_travel_registration", {}).get(phase, {}).get(direction)
    actual = part_signature(donor, direction, family["travel_component_bounds"][direction], family["approved_base_registration"][direction])
    # An unchanged raw file alone cannot authorize borrowing a provisional/rejected donor assembly. Its original approved component registration must still match the dated instance snapshot.
    if approved is None or actual != approved or candidate.get("donor_registration", {}).get(direction) != approved:
        raise ValueError("missing or changed original shared effect approval")
    return candidate


def component_pose(kind, direction, phase, document=None, asset=ASSET):
    """Replace only one declared original physical part with an actual authored phase.

    @param kind Original front mining/scanner mechanism or authored rear plume; travel preserves every original opaque craft pixel, including the nozzle.
    @param direction Genuine selected physical viewport heading.
    @param phase Actual authored mechanical phase0 or1; never a work increment.
    @param document Optional detached source record for focused refusal checks.
    @param asset Explicit original family owning this base and part; default retains the accepted mining source API.
    @return Indexed native pose and exact original/part/protected-pixel evidence.
    @throws ValueError On absent source, changed registration, unsupported phase or malformed physical replacement region.
    """
    document = art.read_record() if document is None else document
    family = document["families"][asset]
    if kind not in ("front_mechanism", "rear_plume", "front_scanner"):
        raise ValueError("unsupported original Space component")
    prefix = {"front_mechanism": "mining", "rear_plume": "travel", "front_scanner": "scanner"}[kind]
    if phase not in (0, 1):
        raise ValueError("missing authored mining phase")
    if direction not in family["directions"]:
        raise ValueError("missing genuine mining heading")
    sources = family.get(prefix + "_sources", {}).get(str(phase), {})
    if direction not in sources:
        raise ValueError("missing authored mining source heading")
    key = sources[direction]
    candidate = registered_component_source(key, direction, document)
    if direction not in candidate["native_transforms"]:
        raise ValueError("missing genuine mining source heading")
    # Only this original mining mechanism/phase may replace its physical part; a full-body redraw or another role cannot authorize changing protected hull pixels.
    if candidate["asset_id"] != asset or candidate.get("component_kind") != kind or candidate.get("phase") != phase:
        raise ValueError("source is not the declared authored Space component")
    bounds = family[prefix + "_component_bounds"][direction]
    left, top, right, bottom = bounds
    # Native source parts must be nonempty and inside the original fixed canvas; clipping cannot silently remove either hook or duplicate a source rectangle outside its owner.
    if not (0 <= left < right <= 64 and 0 <= top < bottom <= 48):
        raise ValueError("mining component leaves native canvas")
    outlet_contacts = family.get("travel_outlet_contacts", {}).get(direction) if kind == "rear_plume" else None
    signature = part_signature(candidate, direction, bounds, family["approved_base_registration"][direction], outlet_contacts)
    approved = family.get("approved_" + prefix + "_registration", {}).get(str(phase), {}).get(direction)
    # Later load/work/service authoring must retain the same original mechanism and protected body ancestry after source approval.
    if approved is not None and signature != approved:
        raise ValueError("approved mining mechanism registration changed")
    base_key = family["selections"]["free/" + direction]
    base, base_evidence = art.native_view(base_key, direction, document)
    rgba, part_evidence = art.source_pixels(candidate, direction, [0, 0, 64, 48])
    # The source's ordinary physical effect rectangle contains the authored rear plume. Its complete visible output must fit the declared effect region; no clipping can fake source coverage.
    if kind == "rear_plume":
        visible = part_evidence["native_subject_bounds"]
        if visible[0] < left or visible[1] < top or visible[2] > right or visible[3] > bottom:
            raise ValueError(f"authored nozzle leaves its physical region: {direction} {visible} {bounds}")
    component = art.base.quantize(rgba, {}).crop(bounds)
    if not any(component.get_flattened_data()):
        raise ValueError("authored mining component is empty")
    pose = base.copy()
    if kind in ("front_mechanism", "front_scanner"):
        # Transparent source pixels replace only this original front tool/dish and its mounting collar; a real stroke/sweep cannot leave ghost hardware from the free pose. Original bus/paddles/nozzle outside that declared physical region remain exact.
        pose.paste(component, (left, top))
    else:
        # The immutable original spacecraft, including its existing nozzle, occludes the authored plume. Travel never replaces any opaque hull/tool/paddle/nozzle pixel or cuts a gap at the joint.
        for y in range(top, bottom):
            for x in range(left, right):
                if base.getpixel((x, y)) == 0:
                    pose.putpixel((x, y), component.getpixel((x - left, y - top)))
    protected = 0
    changed = 0
    # Verify every native pixel, including opacity, outside the declared moving part. A visually similar regenerated bus cannot substitute for exact protected original pixels.
    for y in range(48):
        for x in range(64):
            old, new = base.getpixel((x, y)), pose.getpixel((x, y))
            # Working hardware may replace its own declared part; travel may add an authored plume only in originally transparent space.
            if left <= x < right and top <= y < bottom and (kind in ("front_mechanism", "front_scanner") or old == 0):
                changed += old != new
            else:
                if old != new:
                    raise ValueError("mining component changes protected body pixels")
                protected += 1
    if outlet_contacts is not None:
        # Shared gas must touch its own original opaque rear hull/nozzle edge in each actual phase. Source/canvas coverage alone allowed the first unaccepted survey pulses to float away from the craft.
        if not outlet_contacts or any(not (0 <= x < 64 and 0 <= y < 48) or base.getpixel((x, y)) == 0 for x, y in outlet_contacts):
            raise ValueError("invalid original rear outlet contact")
        gas = [(x, y) for y in range(top, bottom) for x in range(left, right) if base.getpixel((x, y)) == 0 and pose.getpixel((x, y)) != 0]
        if not gas:
            raise ValueError("registered actual pulse is hidden by the original craft")
        distance = min(max(abs(x - old_x), abs(y - old_y)) for x, y in gas for old_x, old_y in outlet_contacts)
        if distance != 1:
            raise ValueError("registered pulse does not touch its original rear outlet")
    evidence = dict(direction=direction, phase=phase, kind=kind, base=base_evidence, component=part_evidence, bounds=bounds,
                    signature=signature, protected_pixels=protected, changed_pixels_from_base=changed)
    if candidate.get("instance_of") is not None:
        evidence["source_instance"] = dict(original=candidate["instance_of"], donor_registration=candidate["donor_registration"][direction],
                                           new_generated_artwork=False, source_pixels_changed=False)
    return pose, evidence


def mining_pose(direction, phase, document=None):
    """Assemble an actual authored front drill/hook phase over its exact original body.

    @param direction Genuine selected engine-viewport heading.
    @param phase Original authored cutting phase0 or1.
    @param document Optional detached original record for focused refusal checks.
    @return Native pose and exact protected-part evidence.
    """
    return component_pose("front_mechanism", direction, phase, document)


def travel_pose(direction, phase, document=None):
    """Add an actual authored rear pulse behind the complete immutable original spacecraft.

    @param direction Genuine selected engine-viewport heading.
    @param phase Original authored longer/shorter pulse0 or1; no movement opportunity is created.
    @param document Optional detached original record for focused refusal checks.
    @return Native pose and exact original-pixel protection evidence.
    """
    return component_pose("rear_plume", direction, phase, document)


def review(output, kind="front_mechanism", asset=ASSET):
    """Review actual two-phase native components in every genuine heading.

    @param output Fresh directory under the established repository native-art proof owner.
    @param kind Actual original front mechanism or authored rear plume being reviewed.
    @param asset Explicit family owning the original base/part; mining remains the compatible default.
    @return Hash-bound native/4x contact, genuine phase differences and protected-pixel totals; source/runtime remain incomplete.
    """
    output = output.resolve()
    # Derived proof cannot overwrite previous selected reviews, authored originals or shipped packed payloads.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("mining review requires a fresh native-art proof directory")
    document = art.read_record()
    directions = document["families"][asset]["directions"]
    images, evidence, differences = {}, [], {}
    # Every actual phase/heading resolves independently; phase repetition or an unauthored opposite view cannot pass through a convenient reused source.
    for direction in directions:
        for phase in (0, 1):
            images[(direction, phase)], record = component_pose(kind, direction, phase, document, asset)
            evidence.append(record)
        different = sum(a != b for a, b in zip(images[(direction, 0)].get_flattened_data(), images[(direction, 1)].get_flattened_data(), strict=True))
        minimum = 2 if kind == "rear_plume" else 4
        if different < minimum:
            raise ValueError("authored component phases are indistinguishable at native scale: " + direction)
        differences[direction] = different
    output.mkdir(parents=True)
    atlas = Image.new("P", (512, 96), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (2048, 544), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    # Native/nearest-enlarged rows show the exact same converted pixels; labels describe proof only and never enter packed artwork.
    for index, direction in enumerate(directions):
        for phase in (0, 1):
            native = images[(direction, phase)]
            atlas.paste(native, (64 * index, 48 * phase))
            rgba = native.convert("RGBA")
            contact.paste(rgba, (256 * index + 96, 272 * phase + 20), rgba)
            enlarged = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(enlarged, (256 * index, 272 * phase + 76), enlarged)
            drawing.text((256 * index + 8, 272 * phase + 4), f"{direction} phase {phase}", fill=(235, 235, 235))
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / {"front_mechanism": "mining-source-review.json", "front_scanner": "scanner-source-review.json", "rear_plume": "travel-source-review.json"}[kind]
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=asset, component_kind=kind, poses=evidence, phase_different_pixels=differences,
                                     complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Review one explicit actual mining-part source batch without activating runtime art."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--component", choices=("front_mechanism", "rear_plume", "front_scanner"), default="front_mechanism")
    parser.add_argument("--asset", choices=(ASSET, "vehicle.model-19", "vehicle.model-20"), default=ASSET)
    args = parser.parse_args()
    print(json.dumps(review(args.output, args.component, args.asset)))


if __name__ == "__main__":
    main()
