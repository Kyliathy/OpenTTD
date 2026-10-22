"""Compose genuine Assembly construction and native paid-work display sources.

@file convert_space_assembly_states.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17BC.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13I
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group74.3
The empty gantry never creates a product, job, inventory or energy owner.
Only existing console glass changes for paid work or actual native refusal.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_assembly_art as assembly
import convert_space_assembly_tiles as tiles
from tools import orbital_tile_art as cuts
from tools import orbital_site_art as site

STATES = ("blueprint", "partial", "complete", "blocked")
POSES = ("blueprint", "partial", "complete", "working", "blocked")
STATUS_CONTACT = (100, 65)
STATUS_BOUNDS = (97, 63, 104, 68)
BLUE_GLASS = (155, 156, 157, 158)
SHARED_GLASS = "orbital-core-blocked-glass-v1"
WORK_SELECTION = "Normal unpaused original Working manufacture job, positive saved work, exact current last_work_interval, original admitted job/input/berth/panel/dedicated750J consumer owners; saved work_ticks parity selects green frame1 only after actual paid work. Idle, queued reservation, awaiting materialization and completed product remain frame0. No GUI, wall-time, request or art-local work clock."
BLOCK_SELECTION = "Actual paid-frame activation or lost Core dependency, or original pending manufacturing admission/refusal for its real inputs, connected panel/consumer, berth or final capacity. Idle/no-job and a completed real product are canonical complete; absent consumer demand alone is not blocked."


def expected_policy(document):
    """Bind all source phases to their actual construction and manufacturing owners.

    @param document Catalogue with genuine bodies and individually approved glass.
    @return Exact construction, paid-work, refusal and static-alias policy.
    """
    family = document["families"][assembly.ASSET]
    return dict(asset_id=assembly.ASSET, states=list(STATES), directions=["none"], frames=2,
                body_sources=family["body_sources"], body_registrations=family["approved_body_registration"],
                work_source=family["work_source"], work_registration=family["approved_work_registration"],
                status_source=family["status_source"], status_registration=family["approved_status_registration"],
                shared_status_original=SHARED_GLASS, status_contact=list(STATUS_CONTACT),
                status_bounds=list(STATUS_BOUNDS), permitted_blue_underlay=list(BLUE_GLASS),
                tile_policy_sha256=family["approved_tile_policy"], work_policy=assembly.WORK_POLICY,
                construction_policy="Zero saved construction work is blueprint; positive work retains genuine physical partial when stopped or starved; delivered stock alone erects no machinery",
                work_selection_policy=WORK_SELECTION, blocked_selection_policy=BLOCK_SELECTION,
                frame_policy="Complete frame0 is exact accepted blue-glass base; frame1 changes existing glass only for original paid work. Blueprint/partial/blocked frame1 explicitly alias their own frame0",
                product_policy="The sole real materialized product owns its native hull and approach. Never paint a ghost hull, cargo, input quantity or second berth",
                coupling_policy="Unfinished stages omit completed coupling; complete/working/blocked retain every original near-jaw foreground pixel",
                unfinished_shortage_policy="Keep actual blueprint/partial plus original required/delivered/consumed/reason UI")


def checked_policy(document, public=None):
    """Refuse phase drift and invented job authority before reading original images.

    @param document Exact or detached original catalogue.
    @param public Optional actual public source contract.
    @return Exact reviewed original construction and paid-work policy.
    @throws ValueError On unsigned edits, missing original stages or false new signatures.
    """
    tiles.checked_policy(document, public)
    family = document["families"][assembly.ASSET]
    policy = family.get("state_policy", {})
    if cuts.signature(policy) != family.get("approved_state_policy_sha256"):
        raise ValueError("approved Assembly state policy changed")
    # Saved construction and the original paid manufacturing transaction outrank
    # an art hash. No missing physical stage or new display claim may mint work.
    if (set(family["body_sources"]) != {"complete", "blueprint", "partial"}
            or set(family["approved_body_registration"]) != {"complete", "blueprint", "partial"}
            or policy != expected_policy(document)):
        raise ValueError("Assembly state policy differs from native construction or paid manufacturing owners")
    return policy


def display_component(document, phase):
    """Register whole genuine glass inside the original Assembly console window.

    @param document Catalogue with honest generated-green and reused-amber ancestry.
    @param phase Work or blocked material role; neither creates native admission.
    @return Indexed material-only canvas and exact original/reuse evidence.
    @throws ValueError On changed ancestry, frame role, mounting or source bounds.
    """
    if phase not in ("work", "blocked"):
        raise ValueError("unknown Assembly display material phase")
    family = document["families"][assembly.ASSET]
    source_key = "work_source" if phase == "work" else "status_source"
    approval_key = "approved_work_registration" if phase == "work" else "approved_status_registration"
    candidate = art.source_candidate(family[source_key], document)
    role = "fixed_assembly_paid_work_glass" if phase == "work" else "fixed_assembly_shared_status_glass"
    state = "complete" if phase == "work" else "blocked"
    # Generic material has a precise Assembly role; another asset, phase or
    # unsigned registration cannot become a paid-work indicator by coincidence.
    if (candidate["asset_id"] != assembly.ASSET or candidate["directions"] != ["none"]
            or candidate.get("state") != state or candidate.get("component") != role
            or site.registration_signature(candidate) != family[approval_key]):
        raise ValueError("Assembly display original role or registration changed")
    shared = art.source_candidate(SHARED_GLASS, document)
    if phase == "work":
        # Green is a genuinely new imagegen original, not a recoloured binary or
        # claimed reuse. Its source frame and edit target convey no Core work job.
        if (candidate.get("source_frame") != 1 or candidate.get("reused_original_key") is not None
                or candidate.get("generated_from_original_key") != SHARED_GLASS):
            raise ValueError("Assembly work glass requires its genuine frame1 generated ancestry")
    else:
        # Reused amber is unchanged material only. Native Assembly retains its
        # own consumer, actual blocked reason and sole manufacturing product.
        if (candidate.get("reused_original_key") != SHARED_GLASS
                or any(candidate[key] != shared[key] for key in ("path", "sha256", "image_size"))):
            raise ValueError("Assembly amber original shared ancestry changed")
    transform = candidate["native_transform"]
    # A new authoring signature cannot move the warning/work cue onto a post,
    # the deck or a newly invented external fixture.
    if transform["native_contact"] != list(STATUS_CONTACT) or transform["native_bounds"] != list(STATUS_BOUNDS):
        raise ValueError("Assembly display leaves its existing native console")
    image, evidence = site.registered_original(candidate, STATUS_CONTACT, assembly.geometry())
    x0, y0, x1, y1 = evidence["native_subject_bounds"]
    # Only the existing physical console may carry status; reject source overflow
    # before compositing can hide pixels outside the genuine approved glass.
    if not (STATUS_BOUNDS[0] <= x0 < x1 <= STATUS_BOUNDS[2]
            and STATUS_BOUNDS[1] <= y0 < y1 <= STATUS_BOUNDS[3]):
        raise ValueError("Assembly display original leaves approved window before assembly")
    return image, dict(evidence, generation_receipt=candidate["generation_receipt"],
                       reused_original_key=candidate.get("reused_original_key"),
                       generated_from_original_key=candidate.get("generated_from_original_key"))


def with_display(complete, material, phase):
    """Overlay every genuine material pixel while protecting all unrelated hardware.

    @param complete Immutable accepted complete Assembly canvas.
    @param material Whole registered glass canvas; no source pixel is clipped or masked.
    @param phase Exact physical material name for evidence and failure diagnosis.
    @return New whole canvas plus actual changed/protected pixel counts.
    @throws ValueError On material over non-glass or an indistinguishable source cue.
    """
    result = complete.copy()
    changed, protected = 0, 0
    # Every original material pixel must already land on real existing blue
    # console glass. Refuse overflow instead of discarding or repainting pixels.
    for y in range(assembly.CANVAS[1]):
        for x in range(assembly.CANVAS[0]):
            value = material.getpixel((x, y))
            if not value:
                protected += 1
                continue
            if complete.getpixel((x, y)) not in BLUE_GLASS:
                raise ValueError("Assembly display material lands outside existing blue glass")
            result.putpixel((x, y), value)
            changed += value != complete.getpixel((x, y))
    if not changed:
        raise ValueError("Assembly display original vanishes or matches canonical glass")
    return result, dict(phase=phase, changed_pixels=changed, protected_canvas_pixels=protected)


def native_states(document=None, public=None):
    """Assemble four physical states and the actual paid-work complete frame.

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
        images[state], sources[state] = assembly.native_body(state, document, public)
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
    @return All64logical parts, five whole poses and phase/alias evidence.
    @throws ValueError On lost cells, invalid source policy or false foreground ownership.
    """
    document = art.read_record() if document is None else document
    images, evidence = native_states(document, public)
    tile_policy = document["families"][assembly.ASSET]["tile_policy"]
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
    evidence.update(logical_pose_count=len(result), explicit_static_frame_aliases=24,
                    physical_pose_count=5,
                    whole_native_hashes={pose: hashlib.sha256(image.tobytes()).hexdigest()
                                         for pose, image in images.items()})
    return result, images, evidence


def review(output, context=None):
    """Publish original Assembly source coverage without packing or native activation.

    @param output Fresh source images directory in the selected generated owner.
    @param context Explicit current recipe invocation separating images and reports.
    @return Exact full source-manifest identity.
    @throws ValueError On unsafe proof ownership or invalid original state assembly.
    """
    from regenerate_art_sources import source_directories
    output, reports = source_directories(output, context)
    parts, images, evidence = matrix()
    atlas = cuts.indexed_canvas((256, 1024))
    contact = Image.new("RGB", (2880, 504), (38, 43, 49))
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
        rgba = images[pose].convert("RGBA").resize((576, 480), Image.Resampling.NEAREST)
        contact.paste(rgba, (index * 576, 24), rgba)
        pen.text((index * 576 + 8, 6), pose + (" (complete frame1: paid work only)" if pose == "working" else ""), fill="white")
    detail = Image.new("RGB", (1248, 344), (38, 43, 49))
    detail_pen = ImageDraw.Draw(detail)
    # Keep genuine work/display poses distinct in review without adding phases.
    for index, pose in enumerate(("complete", "working", "blocked")):
        rgba = images[pose].crop((94, 60, 107, 70)).convert("RGBA").resize((416, 320), Image.Resampling.NEAREST)
        detail.paste(rgba, (index * 416, 24), rgba)
        detail_pen.text((index * 416 + 8, 6), pose + " original console pixels", fill="white")
    output.mkdir(parents=True, exist_ok=context is not None)
    atlas.save(output / "module-assembly-authored.png", transparency=0)
    contact.save(output / "state-contact.png")
    detail.save(output / "console-contact.png")
    for pose, image in images.items():
        image.save(output / (pose + ".png"), transparency=0)
    path = reports / "source-manifest.json"
    report = dict(_documentation=__doc__, asset_id=assembly.ASSET, **evidence, sprites=sprites,
                  coverage_complete=True, packed=False, runtime_activated=False,
                  artifacts=[art.identity(output / name) for name in
                             ("module-assembly-authored.png", "state-contact.png", "console-contact.png", *(pose + ".png" for pose in POSES))])
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return art.identity(path)


def main():
    """Publish only the explicitly requested Assembly state-source review from the CLI."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.output)))


if __name__ == "__main__":
    main()
