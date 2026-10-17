"""Review authored native access-panel states over the immutable small miner.

@file convert_space_status_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Groups14BB/14BC/14BD
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups63.3/63B.2/63C.2
Native cargo, blocking and paid service remain the sole gameplay owners. This converter changes only declared panel pixels, preserving the complete original craft opacity.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_mining_parts as parts

STATES = (("full", 0), ("blocked", 0), ("service", 0), ("service", 1))


def status_signature(candidate, direction, bounds, base_signature, edit_pixels=None):
    """Bind actual panel registration and any explicitly selected original hatch-pixel mask.

    @param candidate Retained actual original panel source.
    @param direction Genuine physical heading in that source.
    @param bounds Approved native upper-access region.
    @param base_signature Approved immutable body registration.
    @param edit_pixels Optional frozen original hatch-face/bezel coordinates; None retains the accepted rectangular miner contract.
    @return Stable original-part approval including every editable native pixel.
    """
    signature = parts.part_signature(candidate, direction, bounds, base_signature)
    if edit_pixels is None:
        return signature
    return hashlib.sha256(json.dumps(dict(part=signature, edit_pixels=edit_pixels), sort_keys=True).encode("utf-8")).hexdigest()


def status_pose(state, direction, phase=0, document=None, asset=parts.ASSET):
    """Place one actual authored panel on its original opaque access surface.

    @param state Full-hold, blocked interlock or paid service panel identity.
    @param direction Genuine selected engine-viewport heading.
    @param phase Actual service phase0/1; static full/blocked have only phase0.
    @param document Optional detached provenance for focused refusal checks.
    @param asset Explicit original craft family; the accepted miner remains the compatible default.
    @return Indexed64x48 pose and original/component protection evidence.
    @throws ValueError On absent state, fake phase, changed ancestry or registration.
    """
    document = art.read_record() if document is None else document
    family = document["families"][asset]
    if (state, phase) not in STATES:
        raise ValueError("missing actual authored status phase")
    sources = family.get("status_sources", {}).get(state, {}).get(str(phase), {})
    if direction not in sources:
        raise ValueError("missing genuine status heading")
    candidate = art.source_candidate(sources[direction], document)
    # A different role, cue or mechanical phase cannot borrow this panel's registration to alter the original miner.
    if (candidate["asset_id"] != asset or candidate.get("component_kind") != "status_panel"
            or candidate.get("panel_state") != state or candidate.get("phase") != phase):
        raise ValueError("source is not the declared authored status panel")
    bounds = family["status_component_bounds"][direction]
    left, top, right, bottom = bounds
    # Panel ownership is limited to the existing small access surface; malformed bounds must never authorize edits outside the native canvas.
    if not (0 <= left < right <= 64 and 0 <= top < bottom <= 48):
        raise ValueError("status panel leaves native canvas")
    edit_pixels = family.get("status_edit_pixels", {}).get(direction)
    allowed = None if edit_pixels is None else {tuple(point) for point in edit_pixels}
    # A narrow hatch mask is an explicit part of original native geometry, never a source-background repair. Empty, duplicate or out-of-region coordinates cannot authorize edits to unrelated hardware.
    if allowed is not None:
        if not allowed or len(allowed) != len(edit_pixels) or any(not (left <= x < right and top <= y < bottom) for x, y in allowed):
            raise ValueError("invalid original hatch pixel mask")
    signature = status_signature(candidate, direction, bounds, family["approved_base_registration"][direction], edit_pixels)
    approved = family.get("approved_status_registration", {}).get(state, {}).get(str(phase), {}).get(direction)
    # Once reviewed, neither a shifted panel nor a widened edit region may silently replace the selected source registration.
    if approved is not None and signature != approved:
        raise ValueError("approved status registration changed")
    base, original = art.native_view(family["selections"]["free/" + direction], direction, document)
    rgba, source = art.source_pixels(candidate, direction, bounds)
    component = art.base.quantize(rgba, {})
    pose = base.copy()
    changed = 0
    # A status panel occupies an already opaque access surface. A frozen hatch mask additionally preserves original hull shading inside the surrounding region. Never clear the hull or extend opacity at fractional-alpha bezel edges.
    for y in range(top, bottom):
        for x in range(left, right):
            old, new = base.getpixel((x, y)), component.getpixel((x, y))
            if old != 0 and new != 0 and (allowed is None or (x, y) in allowed):
                pose.putpixel((x, y), new)
                changed += old != new
    if changed == 0:
        raise ValueError("authored status cue is invisible at native size")
    protected = 0
    # Verify all original pixels outside the exact physical hatch mask, including hull pixels inside the wider registration region, and preserve opacity everywhere. Rectangle-only proof missed shading changes beneath an earlier unaccepted panel assembly.
    for y in range(48):
        for x in range(64):
            old, new = base.getpixel((x, y)), pose.getpixel((x, y))
            if (old == 0) != (new == 0):
                raise ValueError("status cue changes original craft opacity")
            editable = left <= x < right and top <= y < bottom and (allowed is None or (x, y) in allowed)
            if not editable:
                if old != new:
                    raise ValueError("status cue changes protected hardware")
                protected += 1
    return pose, dict(state=state, phase=phase, direction=direction, base=original, component=source,
                      signature=signature, bounds=bounds, edit_pixels=edit_pixels, changed_pixels=changed, protected_pixels=protected)


def review(output, asset=parts.ASSET):
    """Publish measured source contacts for all real static and service panels.

    @param output Fresh directory under the existing repository native-art proof owner.
    @param asset Explicit native family whose own actual panel originals are reviewed.
    @return Direct review identity; full source coverage, packing and runtime remain pending.
    """
    output = output.resolve()
    # A new review must preserve every retained original and earlier selected proof rather than overwrite them.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("status review requires a fresh native-art proof directory")
    document = art.read_record()
    family = document["families"][asset]
    directions = family["directions"]
    # The scanner and carrier have no miner full-hold panel. Review only their own authored panel states; the carrier's actual Steel/Goods load originals supply its cargo cues separately.
    states = [row for row in STATES if row[0] in family["status_sources"]]
    atlas = Image.new("P", (64 * len(directions), 48 * len(states)), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (256 * len(directions), 272 * len(states)), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    poses, evidence = {}, []
    # Every cue/heading uses its genuine source; static cues have one authored pose, while service must resolve both independently authored mechanical phases.
    for row, (state, phase) in enumerate(states):
        for column, direction in enumerate(directions):
            pose, item = status_pose(state, direction, phase, document, asset)
            poses[state, phase, direction] = pose
            evidence.append(item)
            atlas.paste(pose, (64 * column, 48 * row))
            rgba = pose.convert("RGBA")
            contact.paste(rgba, (256 * column + 96, 272 * row + 20), rgba)
            large = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(large, (256 * column, 272 * row + 76), large)
            drawing.text((256 * column + 8, 272 * row + 4), f"{state}/{phase} {direction}", fill=(235, 235, 235))
    differences = {}
    # A still-frame duplicate cannot masquerade as service animation; the actual connector positions must survive the original palette quantizer in every view.
    for direction in directions:
        difference = sum(a != b for a, b in zip(poses["service", 0, direction].get_flattened_data(),
                                              poses["service", 1, direction].get_flattened_data(), strict=True))
        if difference < 1:
            raise ValueError("actual service phases are identical at native size")
        differences[direction] = difference
    output.mkdir(parents=True)
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "status-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=asset, poses=evidence,
                                     service_phase_differences=differences,
                                     protected_pixels=sum(item["protected_pixels"] for item in evidence),
                                     complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Review explicit original status parts without changing gameplay or shipped sprites."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--asset", choices=(parts.ASSET, "vehicle.model-19", "vehicle.model-20"), default=parts.ASSET)
    args = parser.parse_args()
    print(json.dumps(review(args.output, args.asset)))


if __name__ == "__main__":
    main()
