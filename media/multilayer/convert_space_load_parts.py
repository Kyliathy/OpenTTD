"""Assemble original construction loads and heavy-miner ore behind protected hardware.

@file convert_space_load_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BD
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups63C.2/65.3c
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
HEAVY_ASSET = "vehicle.model-21"


def load_states(asset):
    """Resolve only the selected original role's admitted symbolic load sources.

    @param asset Explicit original carrier or heavy-miner family; other roles have no load source.
    @return Exact ordered source cues, without changing native cargo admission or counts.
    """
    return {ASSET: STATES, HEAVY_ASSET: ("ore",)}.get(asset, ())


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


def load_pose(state, direction, document=None, asset=ASSET):
    """Place one actual static load in its original cradle without repainting carrier hardware.

    @param state Steel/Goods carrier type or heavy-miner ore cue; it does not denote one native cargo unit.
    @param direction Genuine selected physical heading.
    @param document Optional detached source definitions for focused refusal checks.
    @param asset Explicit load-owning original family; existing carrier20 remains the default.
    @return Indexed64x48 pose and exact original/load/foreground evidence.
    @throws ValueError On foreign source, changed ancestry/registration or unsafe cargo geometry.
    """
    document = art.read_record() if document is None else document
    family = document["families"][asset]
    if state not in load_states(asset):
        raise ValueError("unsupported actual original cargo load")
    sources = family.get("load_sources", {}).get(state, {})
    if direction not in sources:
        raise ValueError("missing genuine construction load heading")
    candidate = art.source_candidate(sources[direction], document)
    # Actual native cargo requires this role's own symbolic load original. A free body, another cargo/role or a relabeled still cannot provide this source.
    if (candidate["asset_id"] != asset or candidate.get("component_kind") != "cargo_load"
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
    # The heavy hopper's genuine shadowed interior includes DOS gray16; the carrier retains its original gray24 minimum. This role-specific floor range never authorizes colored rims, panels or bright manipulator surfaces.
    minimum_floor_gray = 16 if asset == HEAVY_ASSET else 24
    # Cargo may occlude only the original reviewed gray bed or its declared air volume. Widening a physical cargo mask cannot silently make original frame hardware editable.
    for x, y in editable:
        old = base.getpixel((x, y))
        colour = base_rgb.getpixel((x, y))
        if old != 0 and not (minimum_floor_gray <= max(colour) < 150 and max(colour) - min(colour) <= 20):
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


def review(output, asset=ASSET):
    """Review the selected role's actual static cargo cues in every genuine heading.

    @param output Fresh directory under the existing repository native-art proof owner.
    @param asset Explicit original load-owning role; carrier20 remains the compatible default.
    @return Direct source review identity; full family/packing/runtime acceptance remains separate.
    """
    output = output.resolve()
    # Native proof must preserve retained originals and earlier source reviews rather than overwrite a previous selected artifact.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("load review requires a fresh native-art proof directory")
    document = art.read_record()
    family = document["families"][asset]
    directions = family["directions"]
    states = load_states(asset)
    if not states:
        raise ValueError("selected original role has no cargo source contract")
    atlas = Image.new("P", (64 * len(directions), 48 * len(states)), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (256 * len(directions), 272 * len(states)), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    images, evidence = {}, []
    # Static cargo poses resolve their genuine originals independently. Neither native unit counts nor fake second animation phases enter this source matrix.
    for row, state in enumerate(states):
        for column, direction in enumerate(directions):
            pose, item = load_pose(state, direction, document, asset)
            evidence.append(item)
            images[state, direction] = pose
            atlas.paste(pose, (64 * column, 48 * row))
            rgba = pose.convert("RGBA")
            contact.paste(rgba, (256 * column + 96, 272 * row + 20), rgba)
            large = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(large, (256 * column, 272 * row + 76), large)
            drawing.text((256 * column + 8, 272 * row + 4), f"{state} {direction} static", fill=(235, 235, 235))
    differences = {}
    if asset == ASSET:
        # Both carrier materials must remain distinguishable in each actual native view; an empty floor or recolored chassis cannot stand in for either missing cargo original.
        for direction in directions:
            difference = sum(a != b for a, b in zip(images["steel", direction].get_flattened_data(), images["goods", direction].get_flattened_data(), strict=True))
            if difference < 4:
                raise ValueError("Steel/Goods source cues are indistinguishable at native scale")
            differences[direction] = difference
    full_differences = {}
    if asset == HEAVY_ASSET:
        # The heavy miner has one static full-ore cue. Its actual load must differ from the original empty hopper in every genuine heading without inventing another material type or logical phase.
        for direction in directions:
            empty, _ = art.native_view(family["selections"]["free/" + direction], direction, document)
            full_differences[direction] = sum(a != b for a, b in zip(empty.get_flattened_data(), images["ore", direction].get_flattened_data(), strict=True))
    output.mkdir(parents=True)
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "load-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=asset, poses=evidence,
                                     actual_phases_per_load=1, steel_goods_differences=differences,
                                     empty_full_differences=full_differences,
                                     protected_pixels=sum(item["protected_pixels"] for item in evidence),
                                     complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Review explicit original load parts without changing native cargo or shipped sprites."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--asset", choices=(ASSET, HEAVY_ASSET), default=ASSET)
    args = parser.parse_args()
    print(json.dumps(review(args.output, args.asset)))


if __name__ == "__main__":
    main()
