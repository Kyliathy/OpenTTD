"""Cut the selected original Core into its fixed native world-tile source parts.

@file convert_space_module_tiles.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17BC.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13E
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group70.2
Every original pixel has one tile/part owner. The near coupling cheek alone is
foreground; this source assembly creates no native berth, craft or work owner.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_module_art as core

PARTS = ("back", "front")
NEAR_CHEEK = (203, 118, 208, 123)


def signature(policy):
    """Bind source cuts and their physical foreground assignment to one reviewed policy.

    @param policy Exact Core tile/source/foreground contract.
    @return Canonical SHA256; source approval supplies no gameplay permission.
    """
    return hashlib.sha256(json.dumps(policy, sort_keys=True).encode("utf-8")).hexdigest()


def proposed_policy(document):
    """Describe fixed tile geometry using the accepted base's immutable evidence.

    @param document Current Core source catalogue, including its accepted70.1 review.
    @return Exact source-part policy ready for publication or comparison.
    @throws ValueError On changed base evidence or receiver registrations.
    """
    family = document["families"][core.ASSET]
    reference = family["base_source_review"]
    # The old review identity remains a historical approval fact in the frozen
    # policy. Reconstruct measured base/foreground facts from actual originals;
    # checked_policy still compares every field against its independent signed
    # policy, so regeneration cannot approve changed pixels or registrations.
    _, base = core.native_base(family["base_selection"], document)
    # Cut approval cannot launder a different hull or coupling: the original
    # whole-model evidence and both receiving registrations must still agree.
    if (base["signature"] != family["approved_base_registration"]
            or base["coupling"]["signature"] != family["approved_coupling_registration"]):
        raise ValueError("Core cuts require the accepted original base registration")
    # Only measured original pixels on the near cheek may occlude a visiting
    # hull; the receiver cavity and rear cheek must stay behind that hull.
    x0, y0, x1, y1 = NEAR_CHEEK
    foreground = [point for point in base["coupling_pixels"] if x0 <= point[0] < x1 and y0 <= point[1] < y1]
    return dict(asset_id=core.ASSET, state="complete", direction="none", native_layout=0,
                world_cells=[list(point) for point in core.CELLS],
                native_cell_origins=[list(point) for point in core.ORIGINS],
                native_cell=[64, 64], anchor=[-31, -33], parts=list(PARTS),
                pixel_assignment="unique_nearest_native_floor_contact_within_actual_cell_canvas",
                base_native_sha256=base["native_sha256"], base_review=reference,
                body_registration=base["signature"], coupling_registration=base["coupling"]["signature"],
                foreground_region=list(NEAR_CHEEK), foreground_pixels=foreground,
                foreground_owner="Original near coupling cheek only; receiver cavity, rear cheek and platform remain back",
                native_approach=list(core.APPROACH), native_coupling_contact=list(core.COUPLING))


def checked_policy(document, public=None):
    """Refuse policy or world-cell drift against original source geometry and pixels.

    @param document Current or detached source catalogue for focused refusal checks.
    @param public Optional public manifest; fixed native geometry remains authoritative.
    @return Approved exact Core cut policy, after rechecking original base pixels and registrations.
    @throws ValueError On unsigned changes or newly signed geometry/foreground drift.
    """
    core.public_contract(public)
    family = document["families"][core.ASSET]
    policy = family.get("tile_policy", {})
    if signature(policy) != family.get("approved_tile_policy"):
        raise ValueError("approved Core tile policy changed")
    # Source-local metadata cannot rotate cells, move their anchors or enlarge
    # foreground over a visiting hull, even under a fresh authoring signature.
    if (family["native_world_cells"] != [list(point) for point in core.CELLS]
            or family["native_canvas"] != list(core.CANVAS) or policy != proposed_policy(document)):
        raise ValueError("Core tile policy differs from actual native geometry or original foreground")
    return policy


def indexed_canvas(size):
    """Allocate transparent native storage without generating source artwork.

    @param size Exact canvas dimensions for a native part or whole reconstruction.
    @return DOS-indexed image with the original transparent index and palette.
    """
    result = Image.new("P", size, 0)
    result.putpalette(art.base.PALETTE_BYTES)
    result.info["transparency"] = 0
    return result


def tile_owner(x, y):
    """Assign one original source pixel to an eligible native floor contact.

    @param x Whole native source column, without a camera or tile-plane offset.
    @param y Whole native source row shared by original body and coupling pixels.
    @return Stable tile index in the actual x-fast3x3 world-cell order.
    @throws ValueError On pixels outside all actual native cell canvases.
    """
    eligible = [i for i, (ox, oy) in enumerate(core.ORIGINS) if 0 <= x - ox < 64 and 0 <= y - oy < 64]
    if not eligible:
        raise ValueError("Core original pixel leaves native tile canvases")
    distances = [(abs(x - core.ORIGINS[i][0] - 31) + 2 * abs(y - core.ORIGINS[i][1] - 48), i) for i in eligible]
    return min(distances)[1]


def reassemble(pieces, policy):
    """Read stored tile parts back independently and require exact original coverage.

    @param pieces Sequence of metadata/image pairs for all nine back/front cells.
    @param policy Verified fixed projection and accepted whole native-pixel identity.
    @return Reconstructed original indexed whole model.
    @throws ValueError On absent/duplicate/moved/changed parts, palettes or pixels.
    """
    expected = {(tile, part) for tile in range(9) for part in PARTS}
    actual = [(row["tile_variant"], row["part"]) for row, image in pieces]
    # Each real cell needs one unambiguous part of each depth, including honest
    # empty foregrounds; missing or duplicated entries cannot count as coverage.
    if len(actual) != len(expected) or set(actual) != expected:
        raise ValueError("missing or duplicate Core native tile part")
    whole = indexed_canvas(core.CANVAS)
    foreground = {tuple(point) for point in policy["foreground_pixels"]}
    # Re-read actual stored pixels through each real tile origin. Comparing an
    # independently rebuilt image catches holes, duplicates and shifted seams.
    for row, image in pieces:
        tile = row["tile_variant"]
        origin = core.ORIGINS[tile]
        # Stored cells must preserve the actual paid footprint and DOS palette;
        # plausible pixel hashes cannot excuse moved hardware or changed sorting.
        if (row["world_cell"] != list(core.CELLS[tile]) or row["native_origin"] != list(origin)
                or row["anchor"] != [-31, -33] or image.mode != "P" or image.size != (64, 64)
                or bytes(image.getpalette()) != bytes(art.base.PALETTE_BYTES)):
            raise ValueError("Core stored tile geometry or palette changed")
        count = sum(value != 0 for value in image.get_flattened_data())
        # Metadata must disclose actual source absence, rather than claim a
        # transparent reservation as delivered foreground or hide real hardware.
        if row["structurally_absent"] != (count == 0) or row["opaque_pixels"] != count:
            raise ValueError("Core structurally absent part disagrees with original pixels")
        if hashlib.sha256(image.tobytes()).hexdigest() != row["native_sha256"]:
            raise ValueError("Core stored tile pixels changed")
        for y in range(64):
            for x in range(64):
                value = image.getpixel((x, y))
                if value == 0:
                    continue
                point = (origin[0] + x, origin[1] + y)
                # Valid combined pixels alone cannot excuse assigning hardware
                # to another sorting cell or placing a cavity above a hull.
                if tile_owner(*point) != tile or ((point in foreground) != (row["part"] == "front")):
                    raise ValueError("Core native cuts change physical cell or foreground ownership")
                if whole.getpixel(point) != 0:
                    raise ValueError("Core native cuts duplicate original pixels")
                whole.putpixel(point, value)
    if hashlib.sha256(whole.tobytes()).hexdigest() != policy["base_native_sha256"]:
        raise ValueError("Core cuts differ from accepted whole pixels")
    return whole


def cut_base(document=None, public=None):
    """Resolve original ancestry and losslessly assign the complete Core's source pixels.

    @param document Optional detached source catalogue; no cached image bypasses approval.
    @param public Optional public manifest for native geometry refusal checks.
    @return All18native part pairs plus source/geometry/reconstruction evidence.
    @throws ValueError On invalid source ownership, palette, geometry or original coverage.
    """
    document = art.read_record() if document is None else document
    policy = checked_policy(document, public)
    family = document["families"][core.ASSET]
    original, source = core.native_base(family["base_selection"], document, public)
    if hashlib.sha256(original.tobytes()).hexdigest() != policy["base_native_sha256"]:
        raise ValueError("Core selected original differs from accepted base pixels")
    images = {(tile, part): indexed_canvas((64, 64)) for tile in range(9) for part in PARTS}
    foreground = {tuple(point) for point in policy["foreground_pixels"]}
    # Nearest projected floor contact assigns each original pixel once within
    # eligible native canvases. These are cuts of one station, not nine copied
    # machines. Only the original near-side cheek belongs above a visiting hull.
    for y in range(core.CANVAS[1]):
        for x in range(core.CANVAS[0]):
            value = original.getpixel((x, y))
            if value == 0:
                continue
            tile = tile_owner(x, y)
            part = "front" if (x, y) in foreground else "back"
            ox, oy = core.ORIGINS[tile]
            images[tile, part].putpixel((x - ox, y - oy), value)
    pieces = []
    # Empty front parts disclose a real lack of foreground hardware. Back
    # parts must still represent every paid footprint cell in the one model.
    for (tile, part), image in images.items():
        count = sum(value != 0 for value in image.get_flattened_data())
        if part == "back" and count == 0:
            raise ValueError("Core cuts omit an actual world-tile body")
        row = dict(asset_id=core.ASSET, state="complete", frame=0, direction="none", native_layout=0,
                   tile_variant=tile, world_cell=list(core.CELLS[tile]), part=part,
                   native_origin=list(core.ORIGINS[tile]), anchor=[-31, -33],
                   structurally_absent=count == 0, opaque_pixels=count,
                   native_sha256=hashlib.sha256(image.tobytes()).hexdigest())
        pieces.append((row, image))
    rebuilt = reassemble(pieces, policy)
    if rebuilt.tobytes() != original.tobytes():
        raise ValueError("Core native cuts failed lossless reconstruction")
    return pieces, dict(policy=policy, policy_sha256=signature(policy), source=source,
                        reconstructed_native_sha256=hashlib.sha256(rebuilt.tobytes()).hexdigest(),
                        source_opaque_pixels=sum(value != 0 for value in original.get_flattened_data()),
                        foreground_pixel_count=len(foreground), logical_part_count=18,
                        nonempty_part_count=sum(not row["structurally_absent"] for row, image in pieces),
                        structurally_absent_part_count=sum(row["structurally_absent"] for row, image in pieces))


def review(output):
    """Publish one bounded complete-state cut atlas and readable source contacts.

    @param output Fresh repository native-art proof directory.
    @return Exact cut-review identity; functional states and native activation remain pending.
    @throws ValueError On unsafe output ownership or invalid native source parts.
    """
    output = output.resolve()
    # New proof directories preserve accepted source images and distributed
    # payloads; publishing cuts cannot replace a prior reviewed artifact.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("Core tile review requires a fresh native-art proof directory")
    pieces, evidence = cut_base()
    atlas = indexed_canvas((64 * 9, 64 * 2))
    contact = Image.new("RGB", (192 * 9, 440), (38, 43, 49))
    pen = ImageDraw.Draw(contact)
    sprites = []
    # Store actual parts in tile order, then show the same pixels at3x. Labels
    # explain genuinely absent foreground without fabricating replacement art.
    for row, image in pieces:
        tile, part = row["tile_variant"], row["part"]
        band = PARTS.index(part)
        box = [tile * 64, band * 64, 64, 64]
        atlas.paste(image, tuple(box[:2]))
        sprites.append(dict(row, box=box))
        rgba = image.convert("RGBA").resize((192, 192), Image.Resampling.NEAREST)
        contact.paste(rgba, (tile * 192, band * 220 + 24), rgba)
        label = f"{tile}:{row['world_cell']} {part}"
        if row["structurally_absent"]:
            label += " (absent)"
        pen.text((tile * 192 + 4, band * 220 + 6), label, fill="white")
    rebuilt = reassemble(pieces, evidence["policy"])
    whole_contact = Image.new("RGB", (1024, 768), (38, 43, 49))
    rgba = rebuilt.convert("RGBA").resize((1024, 768), Image.Resampling.NEAREST)
    whole_contact.paste(rgba, mask=rgba.getchannel("A"))
    output.mkdir(parents=True)
    atlas.save(output / "native-cuts.png", transparency=0)
    rebuilt.save(output / "reconstructed.png", transparency=0)
    contact.save(output / "tile-contact.png")
    whole_contact.save(output / "reconstructed-contact.png")
    report = output / "tile-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=core.ASSET, **evidence,
                                     sprites=sprites, complete_source_coverage=False, packed=False,
                                     runtime_activated=False,
                                     artifacts=[art.identity(output / name) for name in
                                                ("native-cuts.png", "reconstructed.png", "tile-contact.png", "reconstructed-contact.png")]),
                                 indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Publish the explicit Core tile-source review without rebuilding native graphics."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
