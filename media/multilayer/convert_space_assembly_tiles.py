"""Cut the accepted original Assembly into its four genuine fixed back/front world cells.

@file convert_space_assembly_tiles.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17BC.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13I
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group74.2
Only the original near coupling jaw belongs above a visiting construction craft.
The receiver and deck stay back; source cuts create no service or inventory.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_assembly_art as assembly
from tools import orbital_tile_art as cuts

NEAR_JAW = (-8, -4, -4, -1)


def geometry():
    """Describe the actual fixed Assembly source cells independently of mutable metadata.

    @return Exact native geometry for the four public world-cell source parts.
    """
    return dict(asset_id=assembly.ASSET, native_canvas=list(assembly.CANVAS), native_cell=[64, 64],
                world_cells=[list(point) for point in assembly.CELLS],
                native_cell_origins=[list(point) for point in assembly.ORIGINS],
                anchor=[-31, -33], native_sort_contact=[31, 48])


def proposed_policy(document):
    """Bind original near-jaw ownership to immutable accepted base evidence.

    @param document Current Assembly catalogue with its accepted74.1 base review.
    @return Exact source/geometry/physical foreground policy.
    @throws ValueError On changed accepted base evidence or original registrations.
    """
    family = document["families"][assembly.ASSET]
    reference = family["base_source_review"]
    # The old review identity remains a historical approval fact in the frozen
    # policy. Reconstruct measured base/foreground facts from actual originals;
    # checked_policy still compares every field against its independent signed
    # policy, so regeneration cannot approve changed pixels or registrations.
    _, base = assembly.native_base(document)
    # A source policy cannot launder a changed deck or another coupling under the
    # existing physical-depth review, even with a fresh authoring signature.
    if (base["body"]["signature"] != family["approved_body_registration"]["complete"]
            or base["coupling"]["signature"] != family["approved_construction_registration"]):
        raise ValueError("Assembly cuts require the accepted original body and passive coupling")
    foreground = []
    x0, y0, x1, y1 = NEAR_JAW
    # Select only measured near-jaw pixels in the single passive coupling. Its
    # dark receiver and far cheek remain behind a visiting construction hull.
    cx, cy = assembly.CONTACT
    foreground.extend(point for point in base["coupling"]["original_pixels"]
                      if x0 <= point[0] - cx < x1 and y0 <= point[1] - cy < y1)
    return dict(asset_id=assembly.ASSET, state="complete", frame=0, direction="none", native_layout=0,
                geometry=geometry(), parts=list(cuts.PARTS), base_review=reference,
                base_native_sha256=base["native_sha256"], body_registration=base["body"]["signature"],
                construction_registration=family["approved_construction_registration"],
                foreground_relative_region=list(NEAR_JAW), foreground_pixels=foreground,
                foreground_owner="Original near jaw only on the single passive construction coupling; receiver, far jaw and floor remain back",
                native_approach=list(assembly.APPROACH), native_construction_contact=list(assembly.CONTACT))


def checked_policy(document, public=None):
    """Refuse tile, rotation and physical-foreground drift against original image geometry.

    @param document Current or detached Assembly source catalogue.
    @param public Optional public manifest for focused native geometry refusal checks.
    @return Exact approved original cut policy.
    @throws ValueError On unsigned policy changes or newly signed false source geometry.
    """
    assembly.family_contract(document, public)
    family = document["families"][assembly.ASSET]
    policy = family.get("tile_policy", {})
    if cuts.signature(policy) != family.get("approved_tile_policy"):
        raise ValueError("approved Assembly tile policy changed")
    if policy != proposed_policy(document):
        raise ValueError("Assembly tile policy differs from native geometry or original near jaws")
    return policy


def cut_base(document=None, public=None):
    """Recheck immutable originals and losslessly partition the complete Assembly source.

    @param document Optional detached source catalogue for focused refusal checks.
    @param public Optional actual public source contract.
    @return All eight real back/front part pairs and original reconstruction evidence.
    @throws ValueError On changed source ancestry, physical geometry or original pixel coverage.
    """
    document = art.read_record() if document is None else document
    policy = checked_policy(document, public)
    original, source = assembly.native_base(document, public)
    if hashlib.sha256(original.tobytes()).hexdigest() != policy["base_native_sha256"]:
        raise ValueError("Assembly source differs from accepted base pixels")
    pieces = cuts.cut_cells(original, policy["geometry"], policy["foreground_pixels"], "complete")
    return pieces, dict(policy=policy, policy_sha256=cuts.signature(policy), source=source,
                        reconstructed_native_sha256=policy["base_native_sha256"],
                        logical_part_count=8, foreground_pixel_count=len(policy["foreground_pixels"]),
                        nonempty_part_count=sum(not row["structurally_absent"] for row, image in pieces),
                        structurally_absent_part_count=sum(row["structurally_absent"] for row, image in pieces))


def review(output):
    """Publish complete-state source cuts with whole-model and individual-part contacts.

    @param output Fresh repository native-art proof directory.
    @return Exact bounded source-review identity, without full functional or runtime claims.
    @throws ValueError On unsafe output ownership or invalid original/source cuts.
    """
    output = output.resolve()
    # Fresh proof ownership preserves accepted earlier art and shipped payloads;
    # cutting adds evidence without replacing either original or prior review.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("Assembly tile review requires a fresh native-art proof directory")
    pieces, evidence = cut_base()
    atlas = cuts.indexed_canvas((256, 128))
    contact = Image.new("RGB", (768, 440), (38, 43, 49))
    pen = ImageDraw.Draw(contact)
    sprites = []
    # Show the same actual original tile pixels at3x; missing physical fronts
    # remain explicitly absent, never filled with a fabricated foreground gate.
    for row, image in pieces:
        tile, band = row["tile_variant"], cuts.PARTS.index(row["part"])
        box = [tile * 64, band * 64, 64, 64]
        atlas.paste(image, tuple(box[:2]))
        sprites.append(dict(row, box=box))
        rgba = image.convert("RGBA").resize((192, 192), Image.Resampling.NEAREST)
        contact.paste(rgba, (tile * 192, band * 220 + 24), rgba)
        label = f"{tile}:{row['world_cell']} {row['part']}"
        if row["structurally_absent"]:
            label += " (absent)"
        pen.text((tile * 192 + 4, band * 220 + 6), label, fill="white")
    policy = evidence["policy"]
    rebuilt = cuts.reassemble(pieces, policy["geometry"], policy["foreground_pixels"],
                              policy["base_native_sha256"], "complete")
    whole_contact = Image.new("RGB", (768, 640), (38, 43, 49))
    rgba = rebuilt.convert("RGBA").resize(whole_contact.size, Image.Resampling.NEAREST)
    whole_contact.paste(rgba, mask=rgba.getchannel("A"))
    output.mkdir(parents=True)
    atlas.save(output / "native-cuts.png", transparency=0)
    rebuilt.save(output / "reconstructed.png", transparency=0)
    contact.save(output / "tile-contact.png")
    whole_contact.save(output / "reconstructed-contact.png")
    path = output / "tile-source-review.json"
    path.write_text(json.dumps(dict(_documentation=__doc__, asset_id=assembly.ASSET, **evidence,
                                   sprites=sprites, complete_source_coverage=False, packed=False, runtime_activated=False,
                                   artifacts=[art.identity(output / name) for name in
                                              ("native-cuts.png", "reconstructed.png", "tile-contact.png", "reconstructed-contact.png")]),
                               indent=2) + "\n", encoding="utf-8")
    return art.identity(path)


def main():
    """Publish only the explicitly requested Assembly tile-source proof from the CLI."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
