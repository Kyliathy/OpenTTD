"""Assemble original Power construction and availability states with explicit still aliases.

@file convert_space_power_states.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17BC.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13H
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group73.3
Original paid panel, availability and shared utility retain their native owners.
Unavailable Power keeps both PV banks and its coupling; art creates no energy.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_power_art as power
import convert_space_power_tiles as tiles
from tools import orbital_tile_art as cuts
from tools import orbital_site_art as site

STATES = ("blueprint", "partial", "complete", "blocked")
STATUS_CONTACT = (94, 74)
STATUS_BOUNDS = (91, 72, 98, 77)
SHARED_GLASS = "orbital-core-blocked-glass-v1"
STATE_OWNERS = {
    "blueprint": "Blueprint with zero saved OrbitalConstructionJob work; deliveries alone erect no PV hardware or generation",
    "partial": "Blueprint with positive saved work; unfinished PV/control hardware remains partial when stopped or starved",
    "complete": "Paid FrameComplete hardware; the one original panel's own activation, availability and Core dependency separately admit output",
    "blocked": "Completed paid Power with zero GetOrbitalModulePanelRate through activation, actual Core dependency, own power_enabled or original producer validity; keep all PV hardware exact",
}


def expected_policy(document):
    """Describe physical Power stages without adding a producer or energy controller.

    @param document Current catalogue with genuine stage/material registrations.
    @return Exact native panel availability, geometry and static-frame policy.
    """
    family = document["families"][power.ASSET]
    return dict(asset_id=power.ASSET, states=list(STATES), directions=["none"], frames=2,
                state_owners=STATE_OWNERS, body_sources=family["body_sources"],
                body_registrations=family["approved_body_registration"],
                status_source=family["status_source"], status_registration=family["approved_status_registration"],
                shared_status_original=SHARED_GLASS, status_contact=list(STATUS_CONTACT), status_bounds=list(STATUS_BOUNDS),
                tile_policy_sha256=family["approved_tile_policy"],
                frame_policy="Frame1 is an exact explicit alias of frame0; fixed PV hardware has no art-local fold, work or energy clock",
                utility_policy=power.UTILITY_POLICY,
                availability_policy="Only the original paid producer and its own power_enabled, activation and actual owned operational Core dependency admit output; no external utility_source, work consumer or service job",
                rate_selection_policy="GetOrbitalModulePanelRate owns availability. Lack of consumer demand or merely pausing the neighbouring Core panel does not block a valid independent Power panel; art never measures or credits energy",
                coupling_policy="Absent on unfinished construction; exact single passive construction coupling remains on complete and blocked paid frames",
                blocked_policy="Only genuine amber material on existing control-box blue glass changes; both PV banks, paid deck and coupling stay exact",
                unfinished_shortage_policy="Keep blueprint/partial plus native required/delivered/consumed/reason UI; positive deliveries alone grant neither hardware nor generation")


def checked_policy(document, public=None):
    """Refuse missing stages, invented work phases or changed utility ownership before raster access.

    @param document Exact or detached source catalogue.
    @param public Optional actual public native geometry contract.
    @return Exact reviewed original state policy.
    @throws ValueError On unsigned changes or newly signed false native state meanings.
    """
    tiles.checked_policy(document, public)
    family = document["families"][power.ASSET]
    policy = family.get("state_policy", {})
    if cuts.signature(policy) != family.get("approved_state_policy_sha256"):
        raise ValueError("approved Power state policy changed")
    # Native saved work and the original paid producer outrank an art hash;
    # no missing stage, duplicated source or borrowed panel flag counts as coverage.
    if (set(family["body_sources"]) != {"complete", "blueprint", "partial"}
            or set(family["approved_body_registration"]) != {"complete", "blueprint", "partial"}
            or policy != expected_policy(document)):
        raise ValueError("Power state policy differs from native construction or utility owners")
    return policy


def status_component(document):
    """Reuse genuine amber material only in Power's existing control-box glass.

    @param document Catalogue with explicit shared-original ancestry and Power registration.
    @return Indexed status-only native canvas and exact source/material evidence.
    @throws ValueError On changed shared ancestry, foreign role, window drift or source overflow.
    """
    family = document["families"][power.ASSET]
    candidate = art.source_candidate(family["status_source"], document)
    shared = art.source_candidate(SHARED_GLASS, document)
    # Shared glass is material only. The original Power producer remains its
    # sole energy owner; no Core panel, service job or utility account is copied.
    if (candidate["asset_id"] != power.ASSET or candidate["directions"] != ["none"]
            or candidate.get("state") != "blocked" or candidate.get("component") != "fixed_power_shared_status_glass"
            or candidate.get("reused_original_key") != SHARED_GLASS
            or any(candidate[key] != shared[key] for key in ("path", "sha256", "image_size"))
            or site.registration_signature(candidate) != family["approved_status_registration"]):
        raise ValueError("Power status original, shared ancestry or registration changed")
    transform = candidate["native_transform"]
    # A shared material approval cannot add an external warning beacon or
    # repaint the deck: only the existing control-box window may change.
    if transform["native_contact"] != list(STATUS_CONTACT) or transform["native_bounds"] != list(STATUS_BOUNDS):
        raise ValueError("Power status glass leaves its existing native control box")
    image, evidence = site.registered_original(candidate, STATUS_CONTACT, power.geometry())
    x0, y0, x1, y1 = evidence["native_subject_bounds"]
    # Only the existing physical console may carry status; reject source overflow
    # before compositing can hide pixels outside the genuine approved glass.
    if not (STATUS_BOUNDS[0] <= x0 < x1 <= STATUS_BOUNDS[2]
            and STATUS_BOUNDS[1] <= y0 < y1 <= STATUS_BOUNDS[3]):
        raise ValueError("Power status original leaves approved control box before assembly")
    if not any(image.get_flattened_data()):
        raise ValueError("Power status original vanishes at native scale")
    return image, dict(evidence, reused_original_key=SHARED_GLASS,
                       original_generation_receipt=shared["generation_receipt"])


def native_states(document=None, public=None):
    """Resolve all four physical stages without changing the accepted ready Power.

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
        images[state], sources[state] = power.native_body(state, document, public)
    images["complete"] = complete
    glass, glass_evidence = status_component(document)
    blocked = complete.copy()
    changed, protected = 0, 0
    # An unavailable completed facility retains the exact paid deck and passive
    # coupling. Only genuine amber pixels on its existing control box change.
    for y in range(power.CANVAS[1]):
        for x in range(power.CANVAS[0]):
            value = glass.getpixel((x, y))
            if value:
                if not complete.getpixel((x, y)):
                    raise ValueError("Power status glass expands the original complete silhouette")
                blocked.putpixel((x, y), value)
                changed += value != complete.getpixel((x, y))
            else:
                protected += 1
    if not changed:
        raise ValueError("Power blocked status is indistinguishable from complete")
    images["blocked"] = blocked
    sources.update(complete=complete_evidence,
                   blocked=dict(glass_evidence, changed_pixels=changed, protected_canvas_pixels=protected))
    return images, dict(policy=policy, sources=sources)


def matrix(document=None, public=None):
    """Cut all genuine Power stages and explicitly disclose both declared static frames.

    @param document Optional catalogue; original ancestry is always revalidated.
    @param public Optional actual public source contract.
    @return All64logical part pairs, original whole images and coverage evidence.
    @throws ValueError On invalid source geometry, lost cells or false physical foreground.
    """
    document = art.read_record() if document is None else document
    images, evidence = native_states(document, public)
    tile_policy = document["families"][power.ASSET]["tile_policy"]
    result = []
    # Unfinished sources have no completed passive construction coupling. Complete and
    # unavailable paid frames retain the same genuine near-jaw foreground.
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
    """Publish full Power source coverage without packing, native selection or game activation.

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
    atlas.save(output / "module-power-authored.png", transparency=0)
    contact.save(output / "state-contact.png")
    # Retain whole original poses for inspection alongside their real cell atlas;
    # these disposable images carry no new source approval or native authority.
    for state, image in images.items():
        image.save(output / (state + ".png"), transparency=0)
    path = reports / "source-manifest.json"
    report = dict(_documentation=__doc__, asset_id=power.ASSET, **evidence, sprites=sprites,
                  coverage_complete=True, packed=False, runtime_activated=False,
                  artifacts=[art.identity(output / name) for name in
                             ("module-power-authored.png", "state-contact.png", *(state + ".png" for state in STATES))])
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return art.identity(path)


def main():
    """Publish only the requested original Power state-source review from the CLI."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
