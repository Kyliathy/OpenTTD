"""Assemble original Core construction/status sources into its fixed native matrix.

@file convert_space_module_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13E
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group70.3
Saved paid construction, activation and panel availability own presentation.
Every second frame is an explicit still alias; artwork grants no work or power.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_module_art as core
import convert_space_module_tiles as tiles

STATES = ("blueprint", "partial", "complete", "blocked")
STATUS_CONTACT = (128, 108)
STATUS_BOUNDS = (125, 105, 132, 111)
STATE_OWNERS = {
    "blueprint": "SpaceStructurePhase::Blueprint with zero saved work; delivered job inputs alone do not erect hardware",
    "partial": "SpaceStructurePhase::Blueprint with positive OrbitalConstructionJob saved work; stopped or starved jobs retain this stage",
    "complete": "SpaceStructurePhase::FrameComplete; full paid physical frame, with facilities admitted separately by IsOrbitalModuleOperational",
    "blocked": "Completed paid frame awaiting activation or with its actual module panel disabled; unfinished shortages retain blueprint/partial and native reason UI",
}


def expected_policy(document):
    """Describe the permitted static stages without creating a second state machine.

    @param document Current source catalogue with original construction/status approvals.
    @return Exact source/frame/geometry contract for public matrix coverage.
    """
    family = document["families"][core.ASSET]
    return dict(asset_id=core.ASSET, states=list(STATES), directions=["none"], frames=2,
                state_owners=STATE_OWNERS, native_orientation="fixed_axis_aligned",
                tile_policy_sha256=family["approved_tile_policy"],
                construction_sources=family["construction_sources"],
                construction_registrations=family["approved_construction_registration"],
                blocked_source=family["blocked_source"], blocked_registration=family["approved_blocked_registration"],
                status_contact=list(STATUS_CONTACT), status_bounds=list(STATUS_BOUNDS),
                frame_policy="Frame1 is an exact declared alias of frame0 in every state; fixed Core hardware has no authored work motion",
                construction_coupling="Absent before the paid physical frame is complete",
                blocked_pixel_policy="Original amber status glass only; all other complete body/coupling pixels remain exact",
                cancelled_policy="Native retained-input cancellation disclosure remains its original owner; no cancelled source state is invented")


def checked_policy(document, public=None):
    """Refuse state/phase drift before opening any new construction original.

    @param document Exact or detached Core source catalogue.
    @param public Optional public contract for focused geometry refusal checks.
    @return Approved source policy that preserves native state and static-frame meanings.
    @throws ValueError On unsigned state changes, invented phase clocks or missing sources.
    """
    tiles.checked_policy(document, public)
    family = document["families"][core.ASSET]
    policy = family.get("state_policy", {})
    if tiles.signature(policy) != family.get("approved_state_policy_sha256"):
        raise ValueError("approved Core state policy changed")
    # A new source signature cannot promote delivered inputs to built hardware,
    # animate a stopped job or substitute a generic blocked hull for an unbuilt site.
    if (set(family["construction_sources"]) != {"blueprint", "partial"}
            or set(family["approved_construction_registration"]) != {"blueprint", "partial"}
            or policy != expected_policy(document)):
        raise ValueError("Core state policy differs from native construction or static frame owners")
    return policy


def construction_body(state, document, public=None):
    """Register one genuine unbuilt/part-built original through the shared floor checks.

    @param state Blueprint or partial physical construction stage only.
    @param document Catalogue retaining that stage's immutable original and approval.
    @param public Optional actual fixed3x3 public contract.
    @return Original indexed construction body and its measured source evidence.
    @throws ValueError On a foreign role, changed stage registration or invalid geometry.
    """
    if state not in ("blueprint", "partial"):
        raise ValueError("unsupported Core construction source state")
    family = document["families"][core.ASSET]
    key = family["construction_sources"][state]
    candidate = art.source_candidate(key, document)
    # Stage-specific approval belongs to this Core, not a foreign vehicle or an
    # arbitrary second completed hull carrying a plausible transform signature.
    if (candidate["asset_id"] != core.ASSET or candidate.get("state") != state
            or candidate.get("component") != f"fixed_core_{state}_body"
            or core.registration_signature(candidate) != family["approved_construction_registration"][state]):
        raise ValueError("Core construction original or stage registration changed")
    # Reuse the exact established whole-floor validator with this separately
    # verified stage approval in a detached metadata adapter. The real complete
    # approval, catalogue, source bytes and native geometry are never modified.
    adapted = copy.deepcopy(document)
    adapted["families"][core.ASSET]["approved_base_registration"] = family["approved_construction_registration"][state]
    native, evidence = core.native_body(key, adapted, public)
    return native, dict(evidence, candidate=key, state=state)


def status_component(document):
    """Reduce the complete original amber glass into its fixed existing window location.

    @param document Catalogue containing the genuine isolated Core status original.
    @return Indexed whole-canvas status component and measured source evidence.
    @throws ValueError On changed ancestry, registration, crop, clearance or source overflow.
    """
    family = document["families"][core.ASSET]
    candidate = art.source_candidate(family["blocked_source"], document)
    # Only this Core's original status glass may replace the existing window;
    # source approval cannot admit a different module, hull or warning geometry.
    if (candidate["asset_id"] != core.ASSET or candidate.get("component") != "fixed_core_blocked_glass"
            or candidate["directions"] != ["none"] or candidate.get("state") != "blocked"
            or core.registration_signature(candidate) != family["approved_blocked_registration"]):
        raise ValueError("Core blocked original or registration changed")
    transform = candidate["native_transform"]
    if transform["native_contact"] != list(STATUS_CONTACT) or transform["native_bounds"] != list(STATUS_BOUNDS):
        raise ValueError("Core status glass leaves its native window")
    with Image.open(art.ROOT / candidate["path"]) as original:
        original = original.convert("RGBA")
    # This separate-file source needs no guessed mask or transparent padding
    # outside the original image; every original glass pixel is retained.
    if list(original.size) != candidate["image_size"] or transform["crop_box"] != [0, 0, *original.size]:
        raise ValueError("Core status original dimensions or whole-source crop changed")
    bounds = original.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if bounds is None or list(bounds) != transform["measured_subject_bbox_in_crop"]:
        raise ValueError("Core status original measurement changed")
    if min(bounds[0], bounds[1], original.width - bounds[2], original.height - bounds[3]) < 16:
        raise ValueError("Core status original lacks transparent clearance")
    if any(art.source_separator(pixel) for pixel in original.get_flattened_data()):
        raise ValueError("Core status source separator enters original")
    numerator, denominator = transform["uniform_scale"]
    if not (0 < numerator < denominator):
        raise ValueError("Core status requires a positive uniform reduction")
    scale = numerator / denominator
    small = original.resize((round(original.width * scale), round(original.height * scale)), Image.Resampling.BOX)
    rx, ry = transform["source_reference"]
    offset = (STATUS_CONTACT[0] - round(rx * scale), STATUS_CONTACT[1] - round(ry * scale))
    visible = small.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if visible is None:
        raise ValueError("Core status reduction loses original glass")
    placed = [visible[i] + offset[i % 2] for i in range(4)]
    # Reject overflow before compositing: a small output canvas cannot hide
    # clipped original warning glass or enlarge its approved physical window.
    if not (STATUS_BOUNDS[0] <= placed[0] < placed[2] <= STATUS_BOUNDS[2]
            and STATUS_BOUNDS[1] <= placed[1] < placed[3] <= STATUS_BOUNDS[3]):
        raise ValueError("Core status original leaves approved window before assembly")
    native = Image.new("RGBA", core.CANVAS)
    native.alpha_composite(small, offset)
    indexed = art.base.quantize(native, {})
    if not any(indexed.get_flattened_data()):
        raise ValueError("Core status glass vanishes at native scale")
    return indexed, dict(candidate=candidate["key"], source=art.identity(art.ROOT / candidate["path"]),
                         signature=core.registration_signature(candidate), native_bounds=placed,
                         native_sha256=hashlib.sha256(indexed.tobytes()).hexdigest())


def native_states(document=None, public=None):
    """Resolve four physical stages while retaining the accepted complete Core exactly.

    @param document Optional detached catalogue for focused refusal checks.
    @param public Optional public contract; artwork cannot override native geometry.
    @return State images and complete original/source-preservation evidence.
    @throws ValueError On invalid sources, changed complete art or status silhouette expansion.
    """
    document = art.read_record() if document is None else document
    policy = checked_policy(document, public)
    parts, base = tiles.cut_base(document, public)
    complete = tiles.reassemble(parts, base["policy"])
    images, evidence = {}, {}
    for state in ("blueprint", "partial"):
        images[state], evidence[state] = construction_body(state, document, public)
    images["complete"] = complete
    glass, glass_evidence = status_component(document)
    blocked = complete.copy()
    changed, protected = 0, 0
    # Status replaces only its actual original window pixels. A blocked paid
    # frame must retain the exact built deck, PV banks, locker and real coupling.
    for y in range(core.CANVAS[1]):
        for x in range(core.CANVAS[0]):
            value = glass.getpixel((x, y))
            if value:
                if complete.getpixel((x, y)) == 0:
                    raise ValueError("Core status glass expands original complete silhouette")
                blocked.putpixel((x, y), value)
                changed += value != complete.getpixel((x, y))
            else:
                protected += 1
    if not changed:
        raise ValueError("Core blocked status is indistinguishable from complete")
    images["blocked"] = blocked
    evidence.update(complete=base, blocked=dict(**glass_evidence, changed_pixels=changed, protected_canvas_pixels=protected))
    return images, dict(policy=policy, sources=evidence)


def matrix(document=None, public=None):
    """Cut the four original stages and disclose every static frame alias explicitly.

    @param document Optional source catalogue; original ancestry is always rechecked.
    @param public Optional public manifest for native footprint refusal cases.
    @return All144logical native part rows/images and original source evidence.
    @throws ValueError On clipped, duplicated or missing source pixels or empty footprint backs.
    """
    document = art.read_record() if document is None else document
    images, evidence = native_states(document, public)
    tile_policy = document["families"][core.ASSET]["tile_policy"]
    result = []
    # Each stage is one genuine whole model. Use the same physical world-cell
    # owner in every stage; unfinished hardware has no completed coupling cheek.
    for state in STATES:
        whole = images[state]
        foreground = tile_policy["foreground_pixels"] if state in ("complete", "blocked") else []
        front = {tuple(point) for point in foreground}
        parts = {(tile, part): tiles.indexed_canvas((64, 64)) for tile in range(9) for part in tiles.PARTS}
        for y in range(core.CANVAS[1]):
            for x in range(core.CANVAS[0]):
                value = whole.getpixel((x, y))
                if not value:
                    continue
                tile = tiles.tile_owner(x, y)
                ox, oy = core.ORIGINS[tile]
                parts[tile, "front" if (x, y) in front else "back"].putpixel((x - ox, y - oy), value)
        canonical = []
        for (tile, part), image in parts.items():
            count = sum(value != 0 for value in image.get_flattened_data())
            if part == "back" and count == 0:
                raise ValueError("Core construction source loses an actual footprint cell")
            row = dict(asset_id=core.ASSET, state=state, frame=0, direction="none", native_layout=0,
                       tile_variant=tile, world_cell=list(core.CELLS[tile]), part=part,
                       native_origin=list(core.ORIGINS[tile]), anchor=[-31, -33],
                       structurally_absent=count == 0, opaque_pixels=count,
                       native_sha256=hashlib.sha256(image.tobytes()).hexdigest(), alias_of=None)
            canonical.append((row, image))
        # Independent stored-part reconstruction also covers construction
        # originals; exact complete-only hash/foreground are replaced solely by
        # this verified stage's original hash and its declared real hardware.
        policy = dict(tile_policy, foreground_pixels=foreground,
                      base_native_sha256=hashlib.sha256(whole.tobytes()).hexdigest())
        tiles.reassemble(canonical, policy)
        result.extend(canonical)
        for row, image in canonical:
            alias = dict(state=state, frame=0, direction="none", tile_variant=row["tile_variant"], part=row["part"])
            result.append((dict(row, frame=1, alias_of=alias), image.copy()))
    evidence.update(logical_pose_count=len(result), explicit_static_frame_aliases=72,
                    whole_native_hashes={state: hashlib.sha256(image.tobytes()).hexdigest() for state, image in images.items()})
    return result, images, evidence


def review(output):
    """Publish full original Core source coverage without native packing or activation.

    @param output Fresh repository native-art proof directory.
    @return Exact full source-manifest identity and source-only coverage summary.
    @throws ValueError On unsafe proof ownership, old output paths or invalid originals.
    """
    output = output.resolve()
    # New proof publication must preserve immutable earlier art and remain in
    # the repository review home; it cannot replace shipped graphics or an old proof.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("Core state review requires a fresh native-art proof directory")
    parts, images, evidence = matrix()
    atlas = tiles.indexed_canvas((576, 1024))
    contact = Image.new("RGB", (2048, 408), (38, 43, 49))
    pen = ImageDraw.Draw(contact)
    rows = []
    # Native atlas bands retain all declared frames, including honest empty
    # fronts. The whole-model contact shows physical stages once because each
    # second frame is an explicitly verified still alias, never invented motion.
    for row, image in parts:
        band = (STATES.index(row["state"]) * 2 + row["frame"]) * 2 + tiles.PARTS.index(row["part"])
        box = [row["tile_variant"] * 64, band * 64, 64, 64]
        atlas.paste(image, tuple(box[:2]))
        rows.append(dict(row, box=box))
    for index, state in enumerate(STATES):
        rgba = images[state].convert("RGBA").resize((512, 384), Image.Resampling.NEAREST)
        contact.paste(rgba, (index * 512, 24), rgba)
        pen.text((index * 512 + 8, 6), state + " (frames0/1 identical)", fill="white")
    output.mkdir(parents=True)
    atlas.save(output / "module-nucleus-authored.png", transparency=0)
    contact.save(output / "state-contact.png")
    for state, image in images.items():
        image.save(output / (state + ".png"), transparency=0)
    report = dict(_documentation=__doc__, asset_id=core.ASSET, **evidence, sprites=rows,
                  coverage_complete=True, packed=False, runtime_activated=False,
                  artifacts=[art.identity(output / name) for name in
                             ("module-nucleus-authored.png", "state-contact.png", *(state + ".png" for state in STATES))])
    path = output / "source-manifest.json"
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return art.identity(path)


def main():
    """Run only the explicit Core original-source matrix publication requested by CLI."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
