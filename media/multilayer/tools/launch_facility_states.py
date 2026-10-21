"""Compose launch-center source status from genuine material and fixed hardware.

@file launch_facility_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61E.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Only existing workshop glazing changes. Whole original material is uniformly
reduced and every nontransparent material pixel must fit that existing glass.
Native work, phase, actor visibility, cargo and per-cell selection remain with
their actual feature owners; this helper is source conversion only.
"""

from copy import deepcopy
import hashlib

from tools import launch_facility_base as base

ROOT = base.ROOT
STATES = ("construction", "ready", "working", "blocked")
CONTACTS = {"NE": [120, 103], "SE": [124, 123], "SW": [158, 125], "NW": [158, 109]}
MATERIALS = {"working": "orbital-assembly-work-glass-v1", "blocked": "orbital-core-blocked-glass-v1"}
BLUE_GLASS = (128, 129, 130, 131, 132)
FOREGROUND_POLICY = "structurally_absent_for_current_native_hidden_Surface_custody; later visible Surface actor renderers require new acceptance"


def checked_policy(policy):
    """Require original native mapping and the reviewed existing-glass registration.

    @param policy Explicit launch-center state-art recipe.
    @return Exact native state mapping and selected four-view bases.
    @throws ValueError On moved material, invented frames, foregrounds or source authority.
    """
    # A fresh recipe checksum cannot authorize new fixtures, frame clocks,
    # visible hidden actors or material pixels outside the existing glazing.
    if (policy["asset_id"] != base.source.ASSET or policy["glass_contacts"] != CONTACTS
            or policy["permitted_underlay"] != list(BLUE_GLASS)
            or policy["expected_changed_pixels"] != 3 or policy["frame_aliases"] != [0, 0, 0, 0]
            or policy["construction_alias"] != "ready" or policy["foreground_policy"] != FOREGROUND_POLICY
            or set(policy["materials"]) != set(MATERIALS)):
        raise ValueError("launch state source geometry, alias or foreground policy changed")
    mapping = base.bound_json(policy["state_mapping"])
    selection = base.bound_json(mapping["selected_bases"])
    # Complete native owner identities are a current source requirement. A
    # changed visibility or placement producer needs new state/foreground review.
    for reference in mapping["native_inputs"]:
        if base.identity(ROOT / reference["path"]) != reference:
            raise ValueError("launch source native state/visibility owner changed")
    return mapping, selection


def material(policy, direction, state):
    """Register every pixel of a genuine reused whole glass original.

    @param policy Reviewed material reuse and mounting recipe.
    @param direction Actual selected hardware view; generic material grants no new view.
    @param state Working green or blocked amber source role.
    @return Indexed whole material plane with exact original/registration ancestry.
    @throws ValueError On changed original, ancestry, reduction or mounting coordinates.
    """
    reference = policy["materials"][state]
    original = base.source.site.art.source_candidate(MATERIALS[state], base.source.site.art.read_record())
    # Reuse keeps the complete original, prompt and actual image inputs. It
    # borrows colour material only, never another family's work or job authority.
    if (reference["original_key"] != original["key"]
            or reference["original"] != dict(path=original["path"], sha256=original["sha256"])
            or reference["generation_receipt"] != original["generation_receipt"]
            or reference["references"] != original["references"]
            or reference["uniform_scale"] != [1, 192] or reference["source_reference"] != [627, 624]
            or reference["crop_box"] != [0, 0, *original["image_size"]]
            or reference["measured_subject_bbox_in_crop"] != original["native_transform"]["measured_subject_bbox_in_crop"]):
        raise ValueError("launch status material lost its exact original ancestry")
    candidate = deepcopy(original)
    candidate.update(key=f"launch-center-{state}-{direction.lower()}-material-reuse-v1",
                     asset_id=base.source.ASSET, component="existing_workshop_glazing_material",
                     reused_original_key=original["key"])
    candidate["native_transform"].update({key: reference[key] for key in
                                          ("uniform_scale", "source_reference", "crop_box", "measured_subject_bbox_in_crop")})
    contact = CONTACTS[direction]
    candidate["native_transform"].update(native_contact=contact,
                                          native_bounds=[contact[0] - 3, contact[1] - 2, contact[0] + 4, contact[1] + 3])
    plane, evidence = base.source.site.registered_original(candidate, contact, base.source.component_geometry("workshop", direction))
    evidence.update(generation_receipt=reference["generation_receipt"], reused_original_key=original["key"],
                    references=reference["references"])
    return plane, evidence


def with_material(whole, component_owners, direction, plane):
    """Change existing glass while preserving every other source pixel and owner.

    @param whole Immutable selected complete base view.
    @param component_owners Original per-pixel hardware ownership from composition.
    @param direction Actual selected native hardware view.
    @param plane Whole uniformly registered genuine glass material.
    @return New indexed state view and exact changed source coordinates.
    @throws ValueError On material overflow, invisible change or wrong component/underlay.
    """
    result = whole.copy()
    owner = base.DEPTH_ORDERS[direction].index("workshop") + 1
    changed = []
    # No material pixel may be clipped, masked away or placed on machinery.
    # All genuine material pixels must replace existing blue workshop glass.
    for y in range(whole.height):
        for x in range(whole.width):
            value = plane.getpixel((x, y))
            if not value:
                continue
            if whole.getpixel((x, y)) not in BLUE_GLASS or component_owners[y * whole.width + x] != owner:
                raise ValueError("launch status original leaves existing workshop glass")
            result.putpixel((x, y), value)
            if value != whole.getpixel((x, y)):
                changed.append([x, y])
    if len(changed) != 3:
        raise ValueError("launch status original changed its approved native visibility")
    return result, changed


def compose_states(policy, direction):
    """Compose one genuine view's states without constructing another hardware pose.

    @param policy Explicit reviewed state-art recipe with original material ancestry.
    @param direction NE/SE/SW/NW actual selected physical view.
    @return State images, unchanged ownership planes and exact material/source evidence.
    """
    mapping, selection = checked_policy(policy)
    recipe = base.bound_json(selection["selected_views"][direction])
    whole, owners, cells, original = base.compose(recipe)
    images = dict(ready=whole, construction=whole.copy())
    evidence = dict(base_recipe=selection["selected_views"][direction], original=original,
                    materials={}, changed_pixels={}, foreground_policy=FOREGROUND_POLICY)
    # Working/refusal only replace a tiny existing glass segment. No phase
    # changes the pad, arm, recovery marking, empty store, actor or cargo pixels.
    for state in MATERIALS:
        plane, ancestry = material(policy, direction, state)
        images[state], changed = with_material(whole, owners, direction, plane)
        evidence["materials"][state] = ancestry
        evidence["changed_pixels"][state] = changed
    evidence["state_sha256"] = {state: hashlib.sha256(image.tobytes()).hexdigest() for state, image in images.items()}
    return images, owners, cells, evidence


def cut_state(whole, cells, direction, state, frame=0):
    """Retain actual state source parts and explicit current absence of foregrounds.

    @param whole Exact indexed state view, including only declared material changes.
    @param cells Unchanged actual-zone ownership map from its selected base.
    @param direction Actual physical view bound to the native layout.
    @param state One declared public facility source state.
    @param frame Logical still frame0..3; all aliases retain identical native pixels.
    @return Twenty-four actual cell/part metadata and PNG pairs.
    """
    if state not in STATES or frame not in range(4):
        raise ValueError("unknown launch state source pose")
    pieces = base.cut_cells(whole, cells, direction)
    # Current native Surface actors are hidden/stored, not visiting scene
    # sprites. Preserve explicit absent fronts rather than fabricate occlusion.
    for row, image in pieces:
        row.update(state=state, frame=frame, foreground_policy=FOREGROUND_POLICY)
    return pieces


def reassemble_state(pieces, cells, expected_hash, direction, state, frame=0):
    """Independently validate state metadata before lossless native cell reassembly.

    @param pieces Stored actual cell/part metadata and indexed PNGs.
    @param cells Original source actual-zone ownership plane.
    @param expected_hash Exact complete indexed state pixel identity.
    @param direction Actual physical source view.
    @param state Declared source state being reconstructed.
    @param frame Logical still frame0..3.
    @return Reconstructed whole original state view with no added actor or pixel loss.
    """
    if state not in STATES or frame not in range(4):
        raise ValueError("unknown launch state reassembly pose")
    normalized = []
    # Validate caller-visible state facts before adapting the shared empty-base
    # cell verifier. Normalization cannot bless wrong state labels or new fronts.
    for row, image in pieces:
        if row["state"] != state or row["frame"] != frame or row["foreground_policy"] != FOREGROUND_POLICY:
            raise ValueError("launch stored state or foreground authority changed")
        normalized.append((dict(row, state="ready_empty", frame=0), image))
    return base.reassemble(normalized, cells, expected_hash, direction)
