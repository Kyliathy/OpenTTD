"""Register actual heavy-miner cutter phases over the immutable original chassis.

@file convert_space_heavy_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65
Only the original rotating cutter face can change. Native work, cargo, leases,
service and simulation time remain owned by their existing gameplay consumers.
"""
import hashlib
import json

from PIL import Image, ImageDraw

import convert_space_art as art
import convert_space_mining_parts as parts

ASSET = "vehicle.model-21"


def drill_signature(candidate, direction, family):
    """Bind the genuine cutter phase, original mount and protected native body.

    @param candidate Actual heavy-miner original with its declared cutter phase.
    @param direction Genuine engine-view heading in that original.
    @param family Selected heavy source owner with exact cutter pixels and mount.
    @return Stable approval over original ancestry and every editable cutter pixel.
    """
    value = dict(part=parts.part_signature(candidate, direction, family["mining_component_bounds"][direction],
                                           family["approved_base_registration"][direction]),
                 edit_pixels=family["mining_edit_pixels"][direction], mount=family["mining_mounts"][direction])
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def drill_pose(direction, phase, document=None):
    """Resolve the actual idle or rotated cutter face without replacing its collar.

    @param direction Genuine selected heavy-miner camera heading.
    @param phase Actual original cutter phase0/1; never a mining work increment.
    @param document Optional detached source record for focused refusal checks.
    @return Indexed64x48 source and exact cutter/mount/protected-pixel evidence.
    @throws ValueError On missing ancestry, changed approval or unsafe cutter ownership.
    """
    document = art.read_record() if document is None else document
    family = document["families"][ASSET]
    # Both authored cutter phases belong to this actual heavy role. A neighboring heading or retained construction robot cannot supply missing work art.
    if phase not in (0, 1) or direction not in family["directions"]:
        raise ValueError("missing actual heavy cutter phase/heading")
    key = family.get("mining_sources", {}).get(str(phase), {}).get(direction)
    if key is None:
        raise ValueError("missing genuine heavy cutter original")
    candidate = art.source_candidate(key, document)
    # Source metadata cannot relabel an unrelated body or another phase as this cutter. The same original idle sheet is explicitly the phase0 source.
    if (candidate["asset_id"] != ASSET or candidate.get("component_kind") != "heavy_cutter"
            or candidate.get("phase") != phase):
        raise ValueError("source is not the declared original heavy cutter")
    left, top, right, bottom = family["mining_component_bounds"][direction]
    points = family["mining_edit_pixels"][direction]
    editable = {tuple(point) for point in points}
    # Cutter ownership is the frozen original steel cone face. Empty, duplicated or out-of-region pixels cannot authorize edits to its yellow collar, arms or electronics housing.
    if (not (0 <= left < right <= 64 and 0 <= top < bottom <= 48) or not editable
            or len(editable) != len(points) or any(not (left <= x < right and top <= y < bottom) for x, y in editable)):
        raise ValueError("invalid original heavy cutter pixels")
    signature = drill_signature(candidate, direction, family)
    approved = family.get("approved_mining_registration", {}).get(str(phase), {}).get(direction)
    # Approval freezes the actual cone, mount and original body together; changing a source transform or broadening its pixel ownership requires a new review.
    if approved is not None and approved != signature:
        raise ValueError("approved heavy cutter registration changed")
    base, original = art.native_view(family["selections"]["free/" + direction], direction, document)
    rgba, source = art.source_pixels(candidate, direction, family["native_art_envelope"])
    component = art.base.quantize(rgba, {})
    pose = base.copy()
    # Only tooth shading rotates around the fixed cutter axis. Preserve its original silhouette and every original collar/chassis pixel; source transparency cannot erase hardware or repair a defective source sheet.
    for point in editable:
        old, new = base.getpixel(point), component.getpixel(point)
        if old == 0:
            raise ValueError("heavy cutter ownership includes original empty space")
        if new != 0:
            pose.putpixel(point, new)
    mount = tuple(family["mining_mounts"][direction])
    # A rotating face must remain connected through the original cone to its unchanged collar. The editable teeth need not touch the collar directly; this bounded physical-part walk checks the actual retained shaft between them.
    if not (left <= mount[0] < right and top <= mount[1] < bottom) or base.getpixel(mount) == 0:
        raise ValueError("heavy cutter misses its original mount")
    connected = {mount}
    pending = [mount]
    # Walk only opaque original cutter pixels inside its reviewed physical bounds. Neither adjacent grippers nor a path through the rest of the chassis can bridge a detached replacement head.
    while pending:
        x, y = pending.pop()
        for next_y in range(max(top, y - 1), min(bottom, y + 2)):
            for next_x in range(max(left, x - 1), min(right, x + 2)):
                point = next_x, next_y
                # Every connection must be present in both the immutable cone and the assembled phase; a source pixel cannot invent a missing shaft.
                if point not in connected and base.getpixel(point) != 0 and pose.getpixel(point) != 0:
                    connected.add(point)
                    pending.append(point)
    if not editable.issubset(connected):
        raise ValueError("heavy cutter face is detached from its original mount")
    changed = 0
    protected = 0
    # Verify the entire native canvas, including transparency. Only the declared original tooth pixels may differ; static phase0 must retain the exact original body.
    for y in range(48):
        for x in range(64):
            old, new = base.getpixel((x, y)), pose.getpixel((x, y))
            if (old == 0) != (new == 0):
                raise ValueError("heavy cutter changes original craft opacity")
            changed += old != new
            if (x, y) not in editable:
                if old != new:
                    raise ValueError("heavy cutter changes protected original hardware")
                protected += 1
    # A genuine second rotational phase must survive the native palette. Phase0 is the actual unchanged idle cone, never a separately counted redraw.
    if (phase == 0 and changed != 0) or (phase == 1 and changed < 2):
        raise ValueError("heavy cutter phase is not readable at native scale")
    return pose, dict(direction=direction, phase=phase, base=original, component=source, signature=signature,
                      bounds=[left, top, right, bottom], edit_pixels=points, mount=list(mount),
                      changed_pixels=changed, protected_pixels=protected, original_opacity_preserved=True)


def review_drill(output):
    """Publish both real cutter phases in all eight original native headings.

    @param output Fresh directory within the existing repository native-art proof owner.
    @return Hash-bound source review identity; other state sources and runtime stay pending.
    @throws ValueError On changed originals, unreadable phases or unsafe output ownership.
    """
    output = output.resolve()
    # Review output preserves immutable originals and all dated producer evidence. It cannot overwrite shipped payloads or silently replace a previous accepted review.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("heavy cutter review requires a fresh native-art proof directory")
    document = art.read_record()
    family = document["families"][ASSET]
    directions = family["directions"]
    if directions != ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]:
        raise ValueError("heavy cutter review requires eight genuine headings")
    atlas = Image.new("P", (64 * len(directions), 96), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contact = Image.new("RGB", (256 * len(directions), 544), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    poses, evidence = {}, []
    # Each heading resolves its own actual original twice: unchanged idle cone and independently authored tooth phase. A missing camera or phase cannot be filled by copying its neighbor.
    for phase in range(2):
        for column, direction in enumerate(directions):
            pose, item = drill_pose(direction, phase, document)
            poses[phase, direction] = pose
            evidence.append(item)
            atlas.paste(pose, (64 * column, 48 * phase))
            rgba = pose.convert("RGBA")
            contact.paste(rgba, (256 * column + 96, 272 * phase + 20), rgba)
            large = rgba.resize((256, 192), Image.Resampling.NEAREST)
            contact.paste(large, (256 * column, 272 * phase + 76), large)
            drawing.text((256 * column + 8, 272 * phase + 4), f"{direction} cutter phase{phase}", fill=(235, 235, 235))
    differences = {}
    # Animation must survive palette conversion in every genuine view. Original hull opacity, fixed anchors and exact protected pixels are already checked by each source assembly.
    for direction in directions:
        difference = sum(a != b for a, b in zip(poses[0, direction].get_flattened_data(),
                                               poses[1, direction].get_flattened_data(), strict=True))
        if difference < 2:
            raise ValueError("heavy cutter phases are indistinguishable at native scale")
        differences[direction] = difference
    originals = [art.source_candidate(key, document) for key in sorted({value for heads in family["mining_sources"].values() for value in heads.values()})]
    output.mkdir(parents=True)
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "heavy-cutter-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, native_cell=[64, 48], anchor=[-32, -32],
                                     source_candidates=originals, pose_count=16, poses=evidence,
                                     phase_differences=differences, original_opacity_preserved=True,
                                     unchanged_idle_phase=True, complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)
