"""Publish original FuelStore construction states without unsupported operation.

@file convert_space_fuel_store_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13HB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group73B.3
Native activation excludes FuelStore. Required blocked art explicitly aliases
the paid completed frame; artwork creates no fuel, warning or inventory owner.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_fuel_store_art as fuel
import convert_space_fuel_store_tiles as tiles
from tools import orbital_tile_art as cuts

STATES = ("blueprint", "partial", "complete", "blocked")
STATE_ALIASES = {"blocked": "complete"}
STATE_OWNERS = {
    "blueprint": "Zero saved OrbitalConstructionJob work; deliveries alone erect no tank hardware",
    "partial": "Positive saved construction work; unfinished open vessels persist when stopped or starved",
    "complete": "Paid FrameComplete tank hardware only; unsupported native activation grants no module functionality",
    "blocked": "Explicit complete-frame source alias: no supported FuelStore operating-status owner exists",
}


def expected_policy(document):
    """Describe actual physical stages and the explicit unsupported-state alias.

    @param document Current catalogue with genuine stage and coupling registrations.
    @return Exact original construction, activation and static-alias policy.
    """
    family = document["families"][fuel.ASSET]
    return dict(asset_id=fuel.ASSET, states=list(STATES), directions=["none"], frames=2,
                state_owners=STATE_OWNERS, body_sources=family["body_sources"],
                body_registrations=family["approved_body_registration"],
                tile_policy_sha256=family["approved_tile_policy"],
                source_state_aliases=STATE_ALIASES,
                frame_policy="Both frames are explicit stills; no art-local construction, fuel or utility clock",
                activation_policy=fuel.FRAME_POLICY,
                coupling_policy="Absent on unfinished construction; exact passive coupling retained on complete and its blocked alias",
                blocked_policy="No operating warning, colour change or invented status owner; blocked is pixel-identical to complete",
                unfinished_shortage_policy="Keep actual blueprint/partial plus native required/delivered/consumed/reason UI")


def checked_policy(document, public=None):
    """Refuse invented activation or missing physical stages before raster access.

    @param document Exact or detached original catalogue.
    @param public Optional actual public native geometry contract.
    @return Exact reviewed state and explicit unsupported-activation policy.
    @throws ValueError On unsigned drift or newly signed false native state meanings.
    """
    tiles.checked_policy(document, public)
    family = document["families"][fuel.ASSET]
    policy = family.get("state_policy", {})
    if cuts.signature(policy) != family.get("approved_state_policy_sha256"):
        raise ValueError("approved FuelStore state policy changed")
    # Saved construction and native unsupported activation outrank an art hash.
    # No missing original or freshly signed warning may create useful fuel.
    if (set(family["body_sources"]) != {"complete", "blueprint", "partial"}
            or set(family["approved_body_registration"]) != {"complete", "blueprint", "partial"}
            or policy != expected_policy(document)):
        raise ValueError("FuelStore state policy differs from native construction or unsupported activation")
    return policy


def native_states(document=None, public=None):
    """Resolve genuine construction stages and retain the complete frame exactly.

    @param document Optional detached catalogue for focused ownership refusals.
    @param public Optional actual public native geometry contract.
    @return Four required whole images with explicit complete-to-blocked ancestry.
    @throws ValueError On invalid source ancestry, activation policy or physical geometry.
    """
    document = art.read_record() if document is None else document
    policy = checked_policy(document, public)
    parts, complete_evidence = tiles.cut_base(document, public)
    tile_policy = complete_evidence["policy"]
    complete = cuts.reassemble(parts, tile_policy["geometry"], tile_policy["foreground_pixels"],
                               tile_policy["base_native_sha256"], "complete")
    images, sources = {}, {}
    for state in ("blueprint", "partial"):
        images[state], sources[state] = fuel.native_body(state, document, public)
    images.update(complete=complete, blocked=complete.copy())
    sources.update(complete=complete_evidence,
                   blocked=dict(alias_of="complete", reason=fuel.FRAME_POLICY,
                                changed_pixels=0, protected_canvas_pixels=192 * 160))
    return images, dict(policy=policy, sources=sources, source_state_aliases=STATE_ALIASES)


def matrix(document=None, public=None):
    """Cut every logical pose while disclosing both frame and unsupported-state aliases.

    @param document Optional source catalogue; original ancestry is always revalidated.
    @param public Optional actual public source contract.
    @return All64logical parts, whole images and exact alias/coverage evidence.
    @throws ValueError On invalid geometry, lost cells or false foreground ownership.
    """
    document = art.read_record() if document is None else document
    images, evidence = native_states(document, public)
    policy = document["families"][fuel.ASSET]["tile_policy"]
    result = []
    # Only paid complete hardware contains the passive coupling. Its blocked
    # source alias retains that exact near jaw; unfinished stages have no front.
    for state in STATES:
        foreground = policy["foreground_pixels"] if state in ("complete", "blocked") else []
        canonical = cuts.cut_cells(images[state], policy["geometry"], foreground, state)
        target = STATE_ALIASES.get(state, state)
        for frame in range(2):
            for row, image in canonical:
                # Alias to the actual physical original, including all blocked
                # parts. The second frame never acquires a new work or fuel phase.
                alias = None
                if frame == 1 or state in STATE_ALIASES:
                    alias = dict(state=target, frame=0, direction="none",
                                 tile_variant=row["tile_variant"], part=row["part"])
                result.append((dict(row, frame=frame, alias_of=alias), image.copy()))
    evidence.update(logical_pose_count=len(result), explicit_static_frame_aliases=32,
                    explicit_state_alias_parts=8, explicit_aliases=40, physical_state_count=3,
                    whole_native_hashes={state: hashlib.sha256(image.tobytes()).hexdigest()
                                         for state, image in images.items()})
    return result, images, evidence


def review(output):
    """Publish complete source coverage without activating unsupported native facilities.

    @param output Fresh repository native-art proof directory.
    @return Exact full source-manifest identity.
    @throws ValueError On unsafe output ownership or invalid source assembly.
    """
    output = output.resolve()
    # Fresh proofs preserve approved originals and previous evidence. Source
    # publication cannot overwrite shipped graphics or native runtime data.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("FuelStore state review requires a fresh native-art proof directory")
    parts, images, evidence = matrix()
    atlas = cuts.indexed_canvas((256, 1024))
    contact = Image.new("RGB", (2304, 504), (38, 43, 49))
    pen = ImageDraw.Draw(contact)
    sprites = []
    # Store all required native world-cell rectangles, including honest empty
    # fronts and explicitly aliased states, for deterministic later packing.
    for row, image in parts:
        band = (STATES.index(row["state"]) * 2 + row["frame"]) * 2 + cuts.PARTS.index(row["part"])
        box = [row["tile_variant"] * 64, band * 64, 64, 64]
        atlas.paste(image, tuple(box[:2]))
        sprites.append(dict(row, box=box))
    for index, state in enumerate(STATES):
        rgba = images[state].convert("RGBA").resize((576, 480), Image.Resampling.NEAREST)
        contact.paste(rgba, (index * 576, 24), rgba)
        label = state + (" = complete (unsupported activation)" if state == "blocked" else " (frames0/1 identical)")
        pen.text((index * 576 + 8, 6), label, fill="white")
    output.mkdir(parents=True)
    atlas.save(output / "module-fuel-store-authored.png", transparency=0)
    contact.save(output / "state-contact.png")
    for state, image in images.items():
        image.save(output / (state + ".png"), transparency=0)
    path = output / "source-manifest.json"
    report = dict(_documentation=__doc__, asset_id=fuel.ASSET, **evidence, sprites=sprites,
                  coverage_complete=True, packed=False, runtime_activated=False,
                  artifacts=[art.identity(output / name) for name in
                             ("module-fuel-store-authored.png", "state-contact.png", *(state + ".png" for state in STATES))])
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return art.identity(path)


def main():
    """Publish only the explicitly requested FuelStore state-source review from the CLI."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
