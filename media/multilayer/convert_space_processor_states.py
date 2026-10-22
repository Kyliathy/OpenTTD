"""Compose genuine Processor construction and native processing display sources.

@file convert_space_processor_states.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17BC.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13J
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group75.3
The shared machine never creates a recipe, inventory, work or energy owner.
Only existing console glass changes for actual processing or native refusal.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_processor_art as processor
import convert_space_processor_tiles as tiles
from tools import orbital_tile_art as cuts
from tools import orbital_site_art as site

STATES = ("blueprint", "partial", "complete", "blocked")
POSES = ("blueprint", "partial", "complete", "working", "blocked")
STATUS_CONTACT = (125, 110)
STATUS_BOUNDS = (122, 108, 128, 113)
BLUE_GLASS = tuple(range(149, 159))
SHARED_GLASS = "orbital-core-blocked-glass-v1"
SHARED_WORK_GLASS = "orbital-assembly-work-glass-v1"
WORK_SELECTION = "Normal unpaused enabled operational original processor with valid actual saved choice, admitted original packets/output/history and exact last_native_tick == synchronized tick+1. OSTL/OGDS use processing.source/consumer/private electrical work and paid_lifetime_j/750 parity; OREF/PICE use no electrical consumer and parity of74*completed native batches+unpowered_work_ticks. Only an actual current native work/batch receipt permits frame1. Disabled/paused/stale/missing work is canonical complete; GUI time/request creates no phase."
BLOCK_SELECTION = "Actual paid-frame activation/lost Core dependency or enabled original GetOrbitalRecipeWait Input/Output/Power/Allocation/Limit/Inactive. Disabled is canonical idle; Work/None are not shortages. Keep actual unfinished construction stage with original required/delivered/consumed/reason UI; no invented electrical outage for OREF/PICE."



def expected_policy(document):
    """Bind all source phases to their actual construction and processing owners.

    @param document Catalogue with genuine bodies and individually approved glass.
    @return Exact construction, native-work, refusal and static-alias policy.
    """
    family = document["families"][processor.ASSET]
    return dict(asset_id=processor.ASSET, states=list(STATES), directions=["none"], frames=2,
                body_sources=family["body_sources"], body_registrations=family["approved_body_registration"],
                work_source=family["work_source"], work_registration=family["approved_work_registration"],
                status_source=family["status_source"], status_registration=family["approved_status_registration"],
                shared_status_original=SHARED_GLASS, status_contact=list(STATUS_CONTACT),
                status_bounds=list(STATUS_BOUNDS), permitted_blue_underlay=list(BLUE_GLASS),
                tile_policy_sha256=family["approved_tile_policy"], work_policy=processor.WORK_POLICY,
                construction_policy="Zero saved construction work is blueprint; positive work retains genuine physical partial when stopped or starved; delivered stock alone erects no machinery",
                work_selection_policy=WORK_SELECTION, blocked_selection_policy=BLOCK_SELECTION,
                frame_policy="Complete frame0 is exact accepted blue-glass base; frame1 changes existing glass only for original admitted processing work. Blueprint/partial/blocked frame1 explicitly alias their own frame0",
                packet_policy="Original shared batch executor alone consumes actual input packets and admits bounded output packets for the saved choice. No painted cargo, free inventory, second recipe or berth",
                coupling_policy="Unfinished stages omit completed coupling; complete/working/blocked retain every original near-jaw foreground pixel",
                unfinished_shortage_policy="Keep actual blueprint/partial plus original required/delivered/consumed/reason UI")


def checked_policy(document, public=None):
    """Refuse phase drift and invented recipe authority before reading original images.

    @param document Exact or detached original catalogue.
    @param public Optional actual public source contract.
    @return Exact reviewed original construction and native-work policy.
    @throws ValueError On unsigned edits, missing original stages or false new signatures.
    """
    tiles.checked_policy(document, public)
    family = document["families"][processor.ASSET]
    policy = family.get("state_policy", {})
    if cuts.signature(policy) != family.get("approved_state_policy_sha256"):
        raise ValueError("approved Processor state policy changed")
    # Saved construction and the original native processing transaction outrank
    # an art hash. No missing physical stage or new display claim may mint work.
    if (set(family["body_sources"]) != {"complete", "blueprint", "partial"}
            or set(family["approved_body_registration"]) != {"complete", "blueprint", "partial"}
            or policy != expected_policy(document)):
        raise ValueError("Processor state policy differs from native construction or processing owners")
    return policy


def display_component(document, phase):
    """Register whole genuine glass inside the original processor console window.

    @param document Catalogue with honest unchanged green and amber material ancestry.
    @param phase Work or blocked material role; neither creates native admission.
    @return Indexed material-only canvas and exact original/reuse evidence.
    @throws ValueError On changed ancestry, frame role, mounting or source bounds.
    """
    if phase not in ("work", "blocked"):
        raise ValueError("unknown Processor display material phase")
    family = document["families"][processor.ASSET]
    source_key = "work_source" if phase == "work" else "status_source"
    approval_key = "approved_work_registration" if phase == "work" else "approved_status_registration"
    candidate = art.source_candidate(family[source_key], document)
    role = "fixed_processor_native_work_glass" if phase == "work" else "fixed_processor_shared_status_glass"
    state = "complete" if phase == "work" else "blocked"
    # Generic material has a precise Processor role; another asset, phase or
    # unsigned registration cannot become a processing indicator by coincidence.
    if (candidate["asset_id"] != processor.ASSET or candidate["directions"] != ["none"]
            or candidate.get("state") != state or candidate.get("component") != role
            or site.registration_signature(candidate) != family[approval_key]):
        raise ValueError("Processor display original role or registration changed")
    shared_key = SHARED_WORK_GLASS if phase == "work" else SHARED_GLASS
    shared = art.source_candidate(shared_key, document)
    # Both materials are explicit unchanged original reuses. Processing keeps
    # its own electrical or unpowered owner; no Assembly job follows green glass.
    if (candidate.get("reused_original_key") != shared_key
            or candidate.get("generated_from_original_key") is not None
            or any(candidate[key] != shared[key] for key in ("path", "sha256", "image_size"))):
        raise ValueError("processor display shared original ancestry changed")
    # Extra source-frame metadata is outside the common registration digest;
    # check it explicitly so a signed reuse cannot advertise an unfinished cue.
    if phase == "work" and candidate.get("source_frame") != 1:
        raise ValueError("processor work glass requires its real complete frame1 role")
    transform = candidate["native_transform"]
    # A new authoring signature cannot move the warning/work cue onto a drum,
    # the deck or a newly invented external fixture.
    if transform["native_contact"] != list(STATUS_CONTACT) or transform["native_bounds"] != list(STATUS_BOUNDS):
        raise ValueError("Processor display leaves its existing native console")
    image, evidence = site.registered_original(candidate, STATUS_CONTACT, processor.geometry())
    x0, y0, x1, y1 = evidence["native_subject_bounds"]
    # Only the existing physical console may carry status; reject source overflow
    # before compositing can hide pixels outside the genuine approved glass.
    if not (STATUS_BOUNDS[0] <= x0 < x1 <= STATUS_BOUNDS[2]
            and STATUS_BOUNDS[1] <= y0 < y1 <= STATUS_BOUNDS[3]):
        raise ValueError("Processor display original leaves approved window before composition")
    return image, dict(evidence, generation_receipt=candidate["generation_receipt"],
                       reused_original_key=candidate.get("reused_original_key"),
                       generated_from_original_key=candidate.get("generated_from_original_key"))


def with_display(complete, material, phase):
    """Overlay every genuine material pixel while protecting all unrelated hardware.

    @param complete Immutable accepted complete Processor canvas.
    @param material Whole registered glass canvas; no source pixel is clipped or masked.
    @param phase Exact physical material name for evidence and failure diagnosis.
    @return New whole canvas plus actual changed/protected pixel counts.
    @throws ValueError On material over non-glass or an indistinguishable source cue.
    """
    result = complete.copy()
    changed, protected = 0, 0
    # Every original material pixel must already land on real existing blue
    # console glass. Refuse overflow instead of discarding or repainting pixels.
    for y in range(processor.CANVAS[1]):
        for x in range(processor.CANVAS[0]):
            value = material.getpixel((x, y))
            if not value:
                protected += 1
                continue
            if complete.getpixel((x, y)) not in BLUE_GLASS:
                raise ValueError("Processor display material lands outside existing blue glass")
            result.putpixel((x, y), value)
            changed += value != complete.getpixel((x, y))
    if not changed:
        raise ValueError("Processor display original vanishes or matches canonical glass")
    return result, dict(phase=phase, changed_pixels=changed, protected_canvas_pixels=protected)


def native_states(document=None, public=None):
    """Assemble four physical states and the actual processing complete frame.

    @param document Optional detached catalogue for focused ownership refusals.
    @param public Optional actual public source contract.
    @return Five whole original poses and exact preservation/source evidence.
    @throws ValueError On invalid state ownership, original ancestry or glass attachment.
    """
    document = art.read_record() if document is None else document
    policy = checked_policy(document, public)
    parts, complete_evidence = tiles.cut_base(document, public)
    tile_policy = complete_evidence["policy"]
    complete = cuts.reassemble(parts, tile_policy["geometry"], tile_policy["foreground_pixels"],
                               tile_policy["base_native_sha256"], "complete")
    images, sources = {}, {}
    for state in ("blueprint", "partial"):
        images[state], sources[state] = processor.native_body(state, document, public)
    images["complete"] = complete
    sources["complete"] = complete_evidence
    # Work and refusal preserve the entire paid machine and real approach.
    # Their independent genuine materials alter existing console glass only.
    for material_phase, pose in (("work", "working"), ("blocked", "blocked")):
        material, original = display_component(document, material_phase)
        images[pose], preservation = with_display(complete, material, material_phase)
        sources[pose] = dict(original, **preservation)
    return images, dict(policy=policy, sources=sources)


def matrix(document=None, public=None):
    """Publish all actual world-cell/state/frame parts with honest still aliases.

    @param document Optional catalogue; original ancestry always remains checked.
    @param public Optional actual public native geometry contract.
    @return All144logical parts, five whole poses and phase/alias evidence.
    @throws ValueError On lost cells, invalid source policy or false foreground ownership.
    """
    document = art.read_record() if document is None else document
    images, evidence = native_states(document, public)
    tile_policy = document["families"][processor.ASSET]["tile_policy"]
    result = []
    # Only completed hardware owns the passive jaw. Only the completed work
    # frame has a second real pose; all other frame1 rows disclose exact stills.
    for state in STATES:
        foreground = tile_policy["foreground_pixels"] if state in ("complete", "blocked") else []
        for frame in range(2):
            pose = "working" if state == "complete" and frame == 1 else state
            canonical = cuts.cut_cells(images[pose], tile_policy["geometry"], foreground, state, frame)
            # Every declared cell/frame retains its exact original pixels and aliases;
            # expanding logical coverage cannot invent hardware or physical phases.
            for row, image in canonical:
                alias = None
                # Only complete hardware has genuine work motion; unfinished and
                # blocked frame1 entries remain exact stills of their own stage.
                if frame == 1 and state != "complete":
                    alias = dict(state=state, frame=0, direction="none", tile_variant=row["tile_variant"], part=row["part"])
                result.append((dict(row, alias_of=alias), image))
    evidence.update(logical_pose_count=len(result), explicit_static_frame_aliases=54,
                    physical_pose_count=5,
                    whole_native_hashes={pose: hashlib.sha256(image.tobytes()).hexdigest()
                                         for pose, image in images.items()})
    return result, images, evidence


def review(output, context=None):
    """Publish original Processor source coverage without packing or native activation.

    @param output Fresh source images directory in the selected generated owner.
    @param context Explicit current recipe invocation separating images and reports.
    @return Exact full source-manifest identity.
    @throws ValueError On unsafe proof ownership or invalid original state processor.
    """
    from regenerate_art_sources import source_directories
    output, reports = source_directories(output, context)
    parts, images, evidence = matrix()
    atlas = cuts.indexed_canvas((576, 1024))
    contact = Image.new("RGB", (2304, 1200), (38, 43, 49))
    pen = ImageDraw.Draw(contact)
    sprites = []
    # Native rectangles retain actual anchors and explicitly absent fronts.
    # Whole-pose contacts include both distinct complete display phases.
    for row, image in parts:
        band = (STATES.index(row["state"]) * 2 + row["frame"]) * 2 + cuts.PARTS.index(row["part"])
        box = [row["tile_variant"] * 64, band * 64, 64, 64]
        atlas.paste(image, tuple(box[:2]))
        sprites.append(dict(row, box=box))
    # Keep genuine work/display poses distinct in review without adding phases.
    for index, pose in enumerate(POSES):
        rgba = images[pose].convert("RGBA").resize((768, 576), Image.Resampling.NEAREST)
        contact.paste(rgba, ((index % 3) * 768, (index // 3) * 600 + 24), rgba)
        pen.text(((index % 3) * 768 + 8, (index // 3) * 600 + 6), pose + (" (complete frame1: actual native work only)" if pose == "working" else ""), fill="white")
    detail = Image.new("RGB", (1536, 344), (38, 43, 49))
    detail_pen = ImageDraw.Draw(detail)
    # Keep genuine work/display poses distinct in review without adding phases.
    for index, pose in enumerate(("complete", "working", "blocked")):
        rgba = images[pose].crop((118, 105, 134, 115)).convert("RGBA").resize((512, 320), Image.Resampling.NEAREST)
        detail.paste(rgba, (index * 512, 24), rgba)
        detail_pen.text((index * 512 + 8, 6), pose + " original console pixels", fill="white")
    output.mkdir(parents=True, exist_ok=context is not None)
    atlas.save(output / "module-processor-authored.png", transparency=0)
    contact.save(output / "state-contact.png")
    detail.save(output / "console-contact.png")
    for pose, image in images.items():
        image.save(output / (pose + ".png"), transparency=0)
    path = reports / "source-manifest.json"
    report = dict(_documentation=__doc__, asset_id=processor.ASSET, **evidence, sprites=sprites,
                  coverage_complete=True, packed=False, runtime_activated=False,
                  artifacts=[art.identity(output / name) for name in
                             ("module-processor-authored.png", "state-contact.png", "console-contact.png", *(pose + ".png" for pose in POSES))])
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return art.identity(path)


def main():
    """Publish only the explicitly requested Processor state-source review from the CLI."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
