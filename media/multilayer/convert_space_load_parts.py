"""Assemble original Steel/Goods load art behind the protected construction carrier hardware.

@file convert_space_load_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BD
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63C.2
@see docs/zz-rhr/upgrades/2026-10/r2mlsp-z-z-r-construction-art-notes.md
Native CargoList/type/weight owns the actual load. Source assembly changes only a reviewed cargo-volume mask and grants no packets, recipe input, work or runtime sprite.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_mining_parts as parts

ASSET = "vehicle.model-20"
STATES = ("steel", "goods")


def load_signature(candidate, direction, bounds, base_signature, edit_pixels, foreground):
    """Bind actual load registration, cargo volume and original foreground hardware.

    @param candidate Actual selected symbolic load original with immutable tool ancestry.
    @param direction Genuine engine-viewport heading.
    @param bounds Declared native projected cargo-volume bounds.
    @param base_signature Approved original empty carrier registration.
    @param edit_pixels Explicit native cargo-volume pixels; source alpha repair is never permitted.
    @param foreground Original arm/fixture regions that stay in front of the load.
    @return Stable part approval, including every editable pixel and foreground cut.
    """
    value = dict(part=parts.part_signature(candidate, direction, bounds, base_signature),
                 edit_pixels=edit_pixels, foreground=foreground)
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def load_pose(state, direction, document=None):
    """Place one actual static load in its original cradle without repainting carrier hardware.

    @param state Steel or Goods type cue; it does not denote one native cargo unit.
    @param direction Genuine selected physical heading.
    @param document Optional detached source definitions for focused refusal checks.
    @return Indexed64x48 pose and exact original/load/foreground evidence.
    @throws ValueError On foreign source, changed ancestry/registration or unsafe cargo geometry.
    """
    document = art.read_record() if document is None else document
    family = document["families"][ASSET]
    if state not in STATES:
        raise ValueError("unsupported actual construction load")
    sources = family.get("load_sources", {}).get(state, {})
    if direction not in sources:
        raise ValueError("missing genuine construction load heading")
    candidate = art.source_candidate(sources[direction], document)
    # Actual carrier cargo requires its own symbolic load original. A free body, another cargo/role or a relabeled still cannot provide this source.
    if (candidate["asset_id"] != ASSET or candidate.get("component_kind") != "cargo_load"
            or candidate.get("load_state") != state or candidate.get("phase") != 0):
        raise ValueError("source is not the declared original cargo load")
    bounds = family["load_component_bounds"][direction]
    left, top, right, bottom = bounds
    edit_pixels = family["load_edit_pixels"][direction]
    editable = {tuple(point) for point in edit_pixels}
    foreground = family["load_foreground_regions"][direction]
    # Only the declared projected cradle may receive original cargo pixels. Empty, duplicated or out-of-canvas points cannot widen this ownership implicitly.
    if (not (0 <= left < right <= 64 and 0 <= top < bottom <= 48) or not editable
            or len(editable) != len(edit_pixels) or any(not (left <= x < right and top <= y < bottom) for x, y in editable)):
        raise ValueError("invalid native cargo volume")
    # Original front arms remain foreground even where the cargo projects behind them; a source mask cannot authorize covering a folded gripper or its mounting collar.
    if any(any(a <= x < c and b <= y < d for a, b, c, d in foreground) for x, y in editable):
        raise ValueError("cargo volume overlaps original foreground hardware")
    signature = load_signature(candidate, direction, bounds, family["approved_base_registration"][direction], edit_pixels, foreground)
    approved = family.get("approved_load_registration", {}).get(state, {}).get(direction)
    # After source review, neither load placement nor foreground protection may drift independently of the selected empty original.
    if approved is not None and signature != approved:
        raise ValueError("approved construction load registration changed")
    base, original = art.native_view(family["selections"]["free/" + direction], direction, document)
    rgba, source = art.source_pixels(candidate, direction, family["native_art_envelope"])
    loaded = art.base.quantize(rgba, {})
    base_rgb = base.convert("RGB")
    pose = base.copy()
    changed = 0
    # Cargo may occlude the original gray bed or occupy its declared air volume. Original colored frame/panels/hatch and bright manipulator surfaces cannot become editable by accidentally broadening a gray-interior mask.
    for x, y in editable:
        old = base.getpixel((x, y))
        colour = base_rgb.getpixel((x, y))
        if old != 0 and not (24 <= max(colour) < 150 and max(colour) - min(colour) <= 20):
            raise ValueError("cargo mask includes original carrier frame hardware")
        actual = loaded.getpixel((x, y))
        if actual != 0:
            pose.putpixel((x, y), actual)
            changed += old != actual
    if changed < 4:
        raise ValueError("actual cargo load is unreadable at native scale")
    protected = 0
    added_opacity = 0
    # Inspect every native pixel, including transparent space. Cargo adds opacity only inside its approved projected volume and never erases the original floor or changes outside hardware.
    for y in range(48):
        for x in range(64):
            old, new = base.getpixel((x, y)), pose.getpixel((x, y))
            if old != 0 and new == 0:
                raise ValueError("cargo source erases original opaque carrier pixels")
            added_opacity += old == 0 and new != 0
            if (x, y) not in editable:
                if old != new:
                    raise ValueError("cargo source changes protected original hardware")
                protected += 1
    return pose, dict(state=state, direction=direction, actual_authored_phase=0, base=original, component=source,
                      bounds=bounds, edit_pixels=edit_pixels, foreground=foreground, signature=signature,
                      changed_pixels=changed, protected_pixels=protected, added_cargo_opacity=added_opacity)


def review(output):
    """Review both actual static cargo types in every genuine heading.

    @param output Fresh directory under the existing repository native-art proof owner.
    @return Direct source review identity; full family/packing/runtime acceptance remains separate.
    """
    output = output.resolve()
    # Native proof must preserve retained originals and earlier source reviews rather than overwrite a previous selected artifact.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("load review requires a fresh native-art proof directory")
    document = art.read_record()
    directions = document["families"][ASSET]["directions"]
    atlas = Image.new("P", (64 * len(directions), 48 * len(STATES)), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (256 * len(directions), 272 * len(STATES)), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    images, evidence = {}, []
    # Static cargo poses resolve their genuine originals independently. Neither native unit counts nor fake second animation phases enter this source matrix.
    for row, state in enumerate(STATES):
        for column, direction in enumerate(directions):
            pose, item = load_pose(state, direction, document)
            evidence.append(item)
            images[state, direction] = pose
            atlas.paste(pose, (64 * column, 48 * row))
            rgba = pose.convert("RGBA")
            contact.paste(rgba, (256 * column + 96, 272 * row + 20), rgba)
            large = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(large, (256 * column, 272 * row + 76), large)
            drawing.text((256 * column + 8, 272 * row + 4), f"{state} {direction} static", fill=(235, 235, 235))
    differences = {}
    # Both admitted materials must remain distinguishable in each actual native view; an empty floor or recolored chassis cannot stand in for one missing cargo original.
    for direction in directions:
        difference = sum(a != b for a, b in zip(images["steel", direction].get_flattened_data(), images["goods", direction].get_flattened_data(), strict=True))
        if difference < 4:
            raise ValueError("Steel/Goods source cues are indistinguishable at native scale")
        differences[direction] = difference
    output.mkdir(parents=True)
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "load-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, poses=evidence,
                                     actual_phases_per_load=1, steel_goods_differences=differences,
                                     protected_pixels=sum(item["protected_pixels"] for item in evidence),
                                     complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Review actual selected construction load parts without changing native cargo or shipped sprites."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
