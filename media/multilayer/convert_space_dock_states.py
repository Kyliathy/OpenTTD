"""Assemble original Dock construction and availability states with explicit still aliases.

@file convert_space_dock_states.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17BC.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13F
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group71.3
The two actual service jobs retain their own work, utility and pause admission.
An unavailable completed facility keeps both physical clamps; art creates no job.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_dock_art as dock
import convert_space_dock_tiles as tiles
from tools import orbital_tile_art as cuts

STATES = ("blueprint", "partial", "complete", "blocked")
STATUS_CONTACT = (94, 60)
STATUS_BOUNDS = (91, 58, 98, 63)
SHARED_GLASS = "orbital-core-blocked-glass-v1"
STATE_OWNERS = {
    "blueprint": "SpaceStructurePhase::Blueprint with zero OrbitalConstructionJob saved work; deliveries alone erect no hardware",
    "partial": "SpaceStructurePhase::Blueprint with positive saved job work; lost dependency or stopped/starved work retains this physical stage",
    "complete": "SpaceStructurePhase::FrameComplete paid physical frame; actual activation, adjacency, service power and jobs separately admit operation",
    "blocked": "Completed paid facility awaiting activation or unavailable through its actual dependency/explicit service source; one paused berth alone never blocks a ready other berth",
}


def expected_policy(document):
    """Describe source states without adding a second construction or service controller.

    @param document Current catalogue retaining all original Dock state registrations.
    @return Exact state, geometry, shared-material and static-frame policy.
    """
    family = document["families"][dock.ASSET]
    return dict(asset_id=dock.ASSET, states=list(STATES), directions=["none"], frames=2,
                state_owners=STATE_OWNERS, body_sources=family["body_sources"],
                body_registrations=family["approved_body_registration"],
                status_source=family["status_source"], status_registration=family["approved_status_registration"],
                shared_status_original=SHARED_GLASS, status_contact=list(STATUS_CONTACT), status_bounds=list(STATUS_BOUNDS),
                tile_policy_sha256=family["approved_tile_policy"],
                frame_policy="Frame1 is an exact explicit alias of frame0 for every fixed physical stage; no art-local clock",
                service_policy="Actual two OrbitalServiceJob receipts, explicit utility source and native UI own per-berth state; fixed clamp pixels supply no service work",
                clamp_policy="Absent during unfinished construction; both original clamps remain exact on complete and blocked paid frames",
                blocked_policy="Amber status glass on the existing rear cabinet only; whole physical deck, two clamps and other pixels stay exact",
                unfinished_shortage_policy="Keep native blueprint/partial stage and original required/delivered/consumed/reason UI; never show a built Dock for missing construction work")


def checked_policy(document, public=None):
    """Refuse missing stages, invented work phases or changed service meaning before raster access.

    @param document Exact or detached source catalogue.
    @param public Optional actual public native geometry contract.
    @return Exact reviewed original state policy.
    @throws ValueError On unsigned changes or newly signed false native state meanings.
    """
    tiles.checked_policy(document, public)
    family = document["families"][dock.ASSET]
    policy = family.get("state_policy", {})
    if cuts.signature(policy) != family.get("approved_state_policy_sha256"):
        raise ValueError("approved Dock state policy changed")
    # Native saved work and two independently admitted service jobs outrank a
    # fresh authoring hash; one missing physical stage cannot count as coverage.
    if (set(family["body_sources"]) != {"complete", "blueprint", "partial"}
            or set(family["approved_body_registration"]) != {"complete", "blueprint", "partial"}
            or policy != expected_policy(document)):
        raise ValueError("Dock state policy differs from native construction or service owners")
    return policy


def status_component(document):
    """Reuse genuine original amber glass only as material in the Dock's existing cabinet.

    @param document Catalogue with explicit shared-original ancestry and Dock registration.
    @return Indexed status-only native canvas and exact source/material evidence.
    @throws ValueError On changed shared ancestry, foreign role, window drift or source overflow.
    """
    family = document["families"][dock.ASSET]
    candidate = art.source_candidate(family["status_source"], document)
    shared = art.source_candidate(SHARED_GLASS, document)
    # Shared glass is an original material component, never borrowed Core
    # power, service capacity or state. Preserve its actual original owner and
    # exact bytes while this Dock owns only a new mounting registration.
    if (candidate["asset_id"] != dock.ASSET or candidate["directions"] != ["none"]
            or candidate.get("state") != "blocked" or candidate.get("component") != "fixed_dock_shared_status_glass"
            or candidate.get("reused_original_key") != SHARED_GLASS
            or any(candidate[key] != shared[key] for key in ("path", "sha256", "image_size"))
            or dock.registration_signature(candidate) != family["approved_status_registration"]):
        raise ValueError("Dock status original, shared ancestry or registration changed")
    transform = candidate["native_transform"]
    # A shared material approval cannot add an external warning beacon or
    # repaint the deck: only the existing rear cabinet window may change.
    if transform["native_contact"] != list(STATUS_CONTACT) or transform["native_bounds"] != list(STATUS_BOUNDS):
        raise ValueError("Dock status glass leaves its existing native cabinet")
    image, evidence = dock.reduced_original(candidate, STATUS_CONTACT)
    x0, y0, x1, y1 = evidence["native_subject_bounds"]
    # Only the existing physical console may carry status; reject source overflow
    # before compositing can hide pixels outside the genuine approved glass.
    if not (STATUS_BOUNDS[0] <= x0 < x1 <= STATUS_BOUNDS[2]
            and STATUS_BOUNDS[1] <= y0 < y1 <= STATUS_BOUNDS[3]):
        raise ValueError("Dock status original leaves approved cabinet before assembly")
    if not any(image.get_flattened_data()):
        raise ValueError("Dock status original vanishes at native scale")
    return image, dict(evidence, reused_original_key=SHARED_GLASS,
                       original_generation_receipt=shared["generation_receipt"])


def native_states(document=None, public=None):
    """Resolve all four physical stages without changing the accepted ready Dock.

    @param document Optional detached catalogue for focused refusal checks.
    @param public Optional public contract; artwork cannot redefine fixed geometry.
    @return Four native original state images and source/preservation evidence.
    @throws ValueError On invalid state ownership, source ancestry or warning silhouette expansion.
    """
    document = art.read_record() if document is None else document
    policy = checked_policy(document, public)
    parts, complete_evidence = tiles.cut_base(document, public)
    tile_policy = complete_evidence["policy"]
    complete = cuts.reassemble(parts, tile_policy["geometry"], tile_policy["foreground_pixels"],
                               tile_policy["base_native_sha256"], "complete")
    images, sources = {}, {}
    for state in ("blueprint", "partial"):
        images[state], sources[state] = dock.native_body(state, document, public)
    images["complete"] = complete
    glass, glass_evidence = status_component(document)
    blocked = complete.copy()
    changed, protected = 0, 0
    # An unavailable completed facility retains the exact paid deck and both
    # physical clamps. Only genuine amber pixels on its existing cabinet change.
    for y in range(dock.CANVAS[1]):
        for x in range(dock.CANVAS[0]):
            value = glass.getpixel((x, y))
            if value:
                if not complete.getpixel((x, y)):
                    raise ValueError("Dock status glass expands the original complete silhouette")
                blocked.putpixel((x, y), value)
                changed += value != complete.getpixel((x, y))
            else:
                protected += 1
    if not changed:
        raise ValueError("Dock blocked status is indistinguishable from complete")
    images["blocked"] = blocked
    sources.update(complete=complete_evidence,
                   blocked=dict(glass_evidence, changed_pixels=changed, protected_canvas_pixels=protected))
    return images, dict(policy=policy, sources=sources)


def matrix(document=None, public=None):
    """Cut all genuine Dock stages and explicitly disclose both declared static frames.

    @param document Optional catalogue; original ancestry is always revalidated.
    @param public Optional actual public source contract.
    @return All64logical part pairs, original whole images and coverage evidence.
    @throws ValueError On invalid source geometry, lost cells or false physical foreground.
    """
    document = art.read_record() if document is None else document
    images, evidence = native_states(document, public)
    tile_policy = document["families"][dock.ASSET]["tile_policy"]
    result = []
    # Each unfinished source has no completed service clamps. Complete and
    # unavailable paid frames retain the same two genuine near-jaw foregrounds.
    for state in STATES:
        foreground = tile_policy["foreground_pixels"] if state in ("complete", "blocked") else []
        canonical = cuts.cut_cells(images[state], tile_policy["geometry"], foreground, state)
        result.extend((dict(row, alias_of=None), image) for row, image in canonical)
        # Every declared cell/frame retains its exact original pixels and aliases;
        # expanding logical coverage cannot invent hardware or physical phases.
        for row, image in canonical:
            alias = dict(state=state, frame=0, direction="none", tile_variant=row["tile_variant"], part=row["part"])
            result.append((dict(row, frame=1, alias_of=alias), image.copy()))
    evidence.update(logical_pose_count=len(result), explicit_static_frame_aliases=32,
                    whole_native_hashes={state: hashlib.sha256(image.tobytes()).hexdigest() for state, image in images.items()})
    return result, images, evidence


def review(output, context=None):
    """Publish full Dock source coverage without packing, native selection or game activation.

    @param output Fresh source images directory in the selected generated owner.
    @param context Explicit current recipe invocation separating images and reports.
    @return Exact original source-manifest identity.
    @throws ValueError On unsafe review ownership or invalid state source assembly.
    """
    from regenerate_art_sources import source_directories
    output, reports = source_directories(output, context)
    parts, images, evidence = matrix()
    atlas = cuts.indexed_canvas((256, 1024))
    contact = Image.new("RGB", (2304, 504), (38, 43, 49))
    pen = ImageDraw.Draw(contact)
    sprites = []
    # The native atlas retains every declared frame and honest empty front.
    # Whole-model contacts show each stage once because frame1 is an exact still.
    for row, image in parts:
        band = (STATES.index(row["state"]) * 2 + row["frame"]) * 2 + cuts.PARTS.index(row["part"])
        box = [row["tile_variant"] * 64, band * 64, 64, 64]
        atlas.paste(image, tuple(box[:2]))
        sprites.append(dict(row, box=box))
    # Show each genuine physical stage once; review enlargement never changes
    # native world-cell ownership or turns a declared still into animation.
    for index, state in enumerate(STATES):
        rgba = images[state].convert("RGBA").resize((576, 480), Image.Resampling.NEAREST)
        contact.paste(rgba, (index * 576, 24), rgba)
        pen.text((index * 576 + 8, 6), state + " (frames0/1 identical)", fill="white")
    output.mkdir(parents=True, exist_ok=context is not None)
    atlas.save(output / "module-dock-authored.png", transparency=0)
    contact.save(output / "state-contact.png")
    # Retain whole original poses for inspection alongside their real cell atlas;
    # these disposable images carry no new source approval or native authority.
    for state, image in images.items():
        image.save(output / (state + ".png"), transparency=0)
    path = reports / "source-manifest.json"
    report = dict(_documentation=__doc__, asset_id=dock.ASSET, **evidence, sprites=sprites,
                  coverage_complete=True, packed=False, runtime_activated=False,
                  artifacts=[art.identity(output / name) for name in
                             ("module-dock-authored.png", "state-contact.png", *(state + ".png" for state in STATES))])
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return art.identity(path)


def main():
    """Publish only the requested original Dock state-source review from the CLI."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
