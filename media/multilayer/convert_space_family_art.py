"""Publish complete authored Space source matrices through the established atlas contract.

@file convert_space_family_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Groups14BB/14BC/14BD/14BE/14BG
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups13C/13CB/13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups63/63B/63C/64/65.3e/65B.3c/66D
This source-only adapter retains exact native anchors, immutable original ancestry and explicit static aliases; it neither packs sprites nor selects gameplay state.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
import convert_space_mining_parts as parts
import convert_space_status_parts as status
import convert_space_load_parts as loads
import convert_space_builder_parts as builder
import convert_space_pod_parts as pod
import convert_space_heavy_parts as heavy
import convert_space_heavy_travel as heavy_travel
import convert_space_hauler_parts as hauler
from tools.orbital_art_contract import heavy_miner_contract, retained_robot_contract, hauler_contract

STATES = ["free", "rendezvous", "attached", "mining", "full", "return", "service", "blocked"]
DIRECTIONS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]
SURVEY_STATES = ["free", "approach", "scanning", "return", "service", "blocked"]
BUILDER_STATES = ["empty", "approach", "steel", "goods", "constructing", "return", "service", "blocked"]
POD_STATES = ["packed", "descending", "recovered", "blocked"]
RETAINED_ASSET = "vehicle.model-16"


def source_states(asset):
    """Resolve the explicit delivered role's source contract without inheriting another craft's work or cargo states.

    @param asset Selected original mining, retained robot, hauler, survey, construction or passive return-shell family.
    @return Exact ordered public state list; unsupported roles require their own source gate.
    @throws ValueError On an unimplemented family profile.
    """
    if asset == parts.ASSET:
        return STATES
    if asset == heavy.ASSET:
        return heavy_miner_contract()["states"]
    if asset == RETAINED_ASSET:
        return list(retained_robot_contract()["source_frame_contract"])
    if asset == hauler.ASSET:
        return hauler_contract()["states"]
    if asset == "vehicle.model-19":
        return SURVEY_STATES
    if asset == builder.ASSET:
        return BUILDER_STATES
    if asset == pod.ASSET:
        return POD_STATES
    raise ValueError("unsupported original Space family profile")


def policy_signature(policy):
    """Bind declared static aliases and actual animation to the reviewed source policy.

    @param policy Complete native-owner/state/phase definitions for this model.
    @return SHA256 over canonical policy; it creates no gameplay owner.
    """
    return hashlib.sha256(json.dumps(policy, sort_keys=True).encode("utf-8")).hexdigest()


def validate_receiving_base(asset, direction, family, document):
    """Require every state branch to use its own approved physical body.

    @param asset Requested native vehicle family; another body's approval cannot authorize its source pixels.
    @param direction Actual original camera needed by this logical pose.
    @param family Receiving family's selected body and frozen world registration.
    @param document Trusted source catalogue whose metadata is checked without repeated image IO.
    @return None; original/receipt byte identities remain checked by the source getters.
    @throws ValueError On absent/duplicate/foreign original geometry or changed receiving-body registration.
    """
    key = family["selections"].get("free/" + direction)
    # Every component eventually renders over the selected body. Resolve that receiver once from metadata so a valid foreign family's native_view approval cannot supply this model's idle, travel or display poses.
    matches = [candidate for candidate in document["candidates"] if candidate["key"] == key]
    if len(matches) != 1:
        raise ValueError("missing or duplicate receiving original body")
    candidate = matches[0]
    # Declared and measured genuine cameras must both belong to this role. A copied candidate key or an invented transform cannot grant another robot's geometry/work identity.
    if (candidate["asset_id"] != asset or direction not in candidate["directions"]
            or direction not in candidate.get("native_transforms", {})):
        raise ValueError("receiving original body belongs to another family or missing genuine heading")
    if art.registration_signature(candidate, direction) != family["approved_base_registration"].get(direction):
        raise ValueError("receiving original body differs from its approved registration")


def validate_state_request(state, direction, frame, document, asset):
    """Validate native source policy and receiving geometry before any image access.

    @param state Public source state; its eventual native selector remains a separate runtime task.
    @param direction Original engine-viewport heading.
    @param frame Public logical frame0/1; static frame1 explicitly aliases actual phase0.
    @param document Current or detached source record for focused refusal checks.
    @param asset Explicit original role/family whose native contract owns the request.
    @return Approved policy row and actual source phase; no image IO or gameplay mutation.
    @throws ValueError On missing approval, malformed policy or absent original coverage.
    """
    states = source_states(asset)
    family = document["families"][asset]
    policy = family.get("state_policy", {})
    if policy_signature(policy) != family.get("approved_state_policy_sha256"):
        raise ValueError("approved native source state policy changed")
    # Each original role must retain every public state, true heading and both logical frames; scope cannot shrink to hide missing originals or borrow another craft's state names.
    if family["states"] != states or direction not in family["directions"] or state not in states or frame not in (0, 1):
        raise ValueError("missing public Space state/direction/frame")
    row = policy[state]
    # Static banks have one actual original phase and animated banks have both. A newly signed authoring policy cannot invent a second static source or hide a missing animated original behind a still.
    expected_phases = [0, 1] if row["animated"] else [0, 0]
    if row.get("phase_alias") != expected_phases:
        raise ValueError("source phase policy does not match actual animation/static ownership")
    if asset == heavy.ASSET:
        # The heavy role additionally consumes its original native public phase contract. Source approval cannot turn its stopped/attached/full/blocked poses into work or suppress actual service/travel phases.
        if row["phase_alias"] != heavy_miner_contract()["source_frame_contract"][state]:
            raise ValueError("heavy source phase policy differs from public native contract")
    if asset == RETAINED_ASSET:
        expected_source = {"idle": "base", "moving": "travel", "working": "base", "service": "service", "blocked": "blocked"}
        # ReadyProbe admits model20 only. Even a freshly signed authoring policy must keep16working byte-identical to idle and retain native travel/service phases without depicting another hull's work, load or extraction.
        if (row["phase_alias"] != retained_robot_contract()["source_frame_contract"][state]
                or row["source"] != expected_source[state]):
            raise ValueError("retained source state differs from public native contract")
    if asset == hauler.ASSET:
        expected_alias = "empty" if state == "service" else None
        # Actual cargo survives every static/moving/blocked branch. Native17has
        # no service producer; even freshly signed metadata must retain its
        # explicit empty alias and may not invent repair work or erase freight.
        if (row["phase_alias"] != hauler_contract()["source_frame_contract"][state]
                or row["source"] != hauler.SOURCE_KINDS[state]
                or row.get("source_state_alias") != expected_alias):
            raise ValueError("hauler source state differs from public native contract")
    phase = row["phase_alias"][frame]
    if direction not in family.get("approved_base_registration", {}):
        raise ValueError("missing original body approval")
    validate_receiving_base(asset, direction, family, document)
    return row, phase


def state_pose(state, direction, frame, document=None, asset=parts.ASSET, source_bank=None):
    """Resolve a logical pose from genuine, independently approved original components.

    @param state Public source state; eventual native selection is a separate runtime task.
    @param direction Genuine original engine-viewport heading.
    @param frame Logical frame0/1; static frame1 explicitly aliases actual phase0.
    @param document Optional detached source record for focused refusal checks.
    @param asset Explicit role/family; the accepted mining API remains the default.
    @param source_bank Optional verified own17bank belonging only to the current conversion.
    @return Indexed native pose and exact source/alias evidence, without inventing animation.
    @throws ValueError On invalid policy/body/source, stale bank or missing original coverage.
    """
    document = art.read_record() if document is None else document
    row, phase = validate_state_request(state, direction, frame, document, asset)
    family = document["families"][asset]
    kind = row["source"]
    if source_bank is not None:
        # A bank is scoped to the original hauler conversion. No other role or
        # arbitrary cached pixels may bypass receiving-body/source validation.
        if asset != hauler.ASSET or not isinstance(source_bank, hauler.HaulerSourceBank):
            raise ValueError("unsupported original source bank")
        pose, evidence = source_bank.pose(kind, phase, direction, document)
    elif kind == "base":
        pose, evidence = art.native_view(family["selections"]["free/" + direction], direction, document)
    elif kind.startswith("hauler-"):
        if asset != hauler.ASSET:
            raise ValueError("hauler source cannot supply another original role")
        if kind in ("hauler-partial", "hauler-full"):
            pose, evidence = hauler.load_pose(kind.removeprefix("hauler-"), direction, document)
        elif kind.startswith("hauler-moving-"):
            pose, evidence = hauler.motion_pose(kind.removeprefix("hauler-moving-"), direction, phase, document)
        else:
            pose, evidence = hauler.blocked_pose(kind.removeprefix("hauler-blocked-"), direction, document)
    elif kind == "heavy-cutter":
        # Only the actual heavy role's reviewed original tooth faces can depict its extraction work; sharing state names with the basic miner grants no source or native work authority.
        if asset != heavy.ASSET or direction not in family.get("approved_mining_registration", {}).get(str(phase), {}):
            raise ValueError("missing actual heavy cutter approval")
        pose, evidence = heavy.drill_pose(direction, phase, document)
    elif kind == "heavy-travel":
        # Heavy travel requires its own two-nozzle visibility/contact review, preserving the naturally occluded S mouth. A basic single-nozzle assembly cannot substitute for this source bank.
        if asset != heavy.ASSET or direction not in family.get("approved_travel_registration", {}).get(str(phase), {}):
            raise ValueError("missing actual heavy travel approval")
        pose, evidence = heavy_travel.travel_pose(direction, phase, document)
    elif kind in ("mining", "travel", "scanner"):
        if direction not in family.get("approved_" + kind + "_registration", {}).get(str(phase), {}):
            raise ValueError("missing actual component approval")
        component = {"mining": "front_mechanism", "travel": "rear_plume", "scanner": "front_scanner"}[kind]
        pose, evidence = parts.component_pose(component, direction, phase, document, asset)
    elif kind in ("full", "service", "blocked"):
        if direction not in family.get("approved_status_registration", {}).get(kind, {}).get(str(phase), {}):
            raise ValueError("missing actual status approval")
        pose, evidence = status.status_pose(kind, direction, phase, document, asset)
    elif kind in (*loads.STATES, "ore"):
        # A static material cue requires that role's own admitted source and actual phase0 approval. Carrier Steel/Goods and heavy ore remain distinct; source reuse grants no native cargo units.
        if kind not in loads.load_states(asset) or phase != 0 or direction not in family.get("approved_load_registration", {}).get(kind, {}):
            raise ValueError("missing actual original load approval")
        pose, evidence = loads.load_pose(kind, direction, document, asset)
    elif kind == "builder":
        # Only the carrier's approved actual manipulator phases can depict Construct work. The source adapter grants no material admission or work opportunity.
        if asset != builder.ASSET or direction not in family.get("approved_builder_registration", {}).get(str(phase), {}):
            raise ValueError("missing actual carrier mechanism approval")
        pose, evidence = builder.builder_pose(direction, phase, document)
    elif kind == "descent":
        # Actual shell tab phases belong only to the separately saved passive pod. A recovery-ready static alias retains a live native job and grants no shell after successful expenditure.
        if asset != pod.ASSET or direction not in family.get("approved_descent_registration", {}).get(str(phase), {}):
            raise ValueError("missing actual passive pod phase approval")
        pose, evidence = pod.tab_pose(direction, phase, document)
    else:
        raise ValueError("unsupported original state source")
    item = dict(state=state, frame=frame, direction=direction, actual_phase=phase,
                explicit_static_alias=not row["animated"] and frame == 1,
                native_owner=row["owner"], source_kind=kind, source=evidence)
    if asset == hauler.ASSET:
        item["source_state_alias"] = row.get("source_state_alias")
    return pose, item


def derive(output, asset=parts.ASSET):
    """Produce a complete selected-role atlas and native contacts without packing or activation.

    @param output Fresh directory under the established native-art proof owner.
    @param asset Explicit original family; the accepted miner remains the default.
    @return Direct source-manifest identity, including the same sprite box layout consumed by existing packers.
    @throws ValueError On missing original coverage, fake animation or unsafe output ownership.
    """
    output = output.resolve()
    # Reproducible derived source belongs to the repository's native-art home and must preserve prior accepted proofs and immutable originals.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("family source publication requires a fresh native-art proof directory")
    document = art.read_record()
    family = document["families"][asset]
    states = source_states(asset)
    directions = family["directions"]
    # Genuine coverage cannot be obtained by repeating a heading, changing the canvas or dropping a public logical phase.
    if directions != DIRECTIONS or family["frames"] != 2 or family["public_native_size"] != [64, 48] or family["public_anchor"] != [-32, -32]:
        raise ValueError("native Space source geometry changed")
    required = len(states) * len(directions) * 2
    source_bank = None
    if asset == hauler.ASSET:
        # Validate every native state/receiving camera before decoding a source.
        # One conversion-owned bank then assembles96physical compositions from
        # their shared originals while every logical lookup still checks drift.
        for state in states:
            for direction in directions:
                validate_state_request(state, direction, 0, document, asset)
        source_bank = hauler.HaulerSourceBank(document)
    atlas = Image.new("P", (64 * len(directions), 48 * len(states) * 2), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    contacts = [Image.new("RGB", (256 * len(directions), 272 * len(states)), (38, 43, 49)) for _ in range(2)]
    drawings = [ImageDraw.Draw(contact) for contact in contacts]
    sprites, evidence, cache, native_hashes, differences = [], [], {}, {}, {}
    # Resolve each actual component once, while retaining all public state/frame entries and declared aliases. Reusing a real source is recorded rather than counted as newly authored artwork.
    for row, state in enumerate(states):
        for frame in range(2):
            for column, direction in enumerate(directions):
                policy = family["state_policy"][state]
                key = policy["source"], policy["phase_alias"][frame], direction
                if source_bank is not None:
                    pose, item = state_pose(state, direction, frame, document, asset, source_bank)
                    cache[key] = pose, item["source"]
                elif key not in cache:
                    pose, item = state_pose(state, direction, frame, document, asset)
                    cache[key] = pose, item["source"]
                pose, original = cache[key]
                item = dict(state=state, frame=frame, direction=direction, actual_phase=key[1], source_kind=key[0],
                            explicit_static_alias=not policy["animated"] and frame == 1,
                            native_owner=policy["owner"], source=original)
                if asset == hauler.ASSET:
                    item["source_state_alias"] = policy.get("source_state_alias")
                evidence.append(item)
                native_hashes[state, frame, direction] = hashlib.sha256(pose.tobytes()).hexdigest()
                box = [64 * column, 48 * (row * 2 + frame), 64, 48]
                atlas.paste(pose, tuple(box[:2]))
                sprites.append(dict(part="body", state=state, frame=frame, direction=direction, box=box,
                                    anchor=[-32, -32], actual_source=key[0], actual_phase=key[1],
                                    explicit_static_alias=item["explicit_static_alias"]))
                rgba = pose.convert("RGBA")
                contacts[frame].paste(rgba, (256 * column + 96, 272 * row + 20), rgba)
                large = rgba.resize((256, 192), Image.Resampling.NEAREST)
                contacts[frame].paste(large, (256 * column, 272 * row + 76), large)
                drawings[frame].text((256 * column + 8, 272 * row + 4), f"{state}/{frame} {direction}", fill=(235, 235, 235))
    # Actual authored animation must survive native conversion in every heading; identical static aliases are allowed only by the frozen explicit policy.
    for state in states:
        for direction in directions:
            identical = native_hashes[state, 0, direction] == native_hashes[state, 1, direction]
            if identical == family["state_policy"][state]["animated"]:
                raise ValueError("source animation/static alias policy differs from actual native pixels")
            differences[state + "/" + direction] = not identical
    output.mkdir(parents=True)
    native = output / f"vehicle-model-{family['native_model']:02}-authored.png"
    atlas.save(native, transparency=0)
    # Separate frame contacts keep each actual native/4x review legible without reducing a giant combined matrix.
    for frame, contact in enumerate(contacts):
        contact.save(output / f"contact-frame-{frame}.png")
    entry = dict(asset_id=asset, **art.identity(native), size=list(atlas.size), native_cell=[64, 48],
                 frames_per_state=2, coverage_complete=True, required_pose_count=required, missing_pose_count=0,
                 missing=[], sprites=sprites)
    source_keys = {value for key, value in family["selections"].items() if key.startswith("free/")}
    # Preserve the direct ancestry of this family without hashing the mutable multi-family catalogue that later survey/builder tasks extend.
    for field in ("mining_sources", "travel_sources", "travel_secondary_sources", "scanner_sources", "load_sources", "builder_sources", "descent_sources"):
        for heads in family.get(field, {}).values():
            source_keys.update(heads.values())
    for phases in family.get("status_sources", {}).values():
        for heads in phases.values():
            source_keys.update(heads.values())
    originals = [art.source_candidate(key, document) for key in sorted(source_keys)]
    report = output / "space-source-manifest.json"
    # Reused actual gas sources remain explicitly registered instances, never new generated role hardware or newly authored directional views.
    instances = [candidate for candidate in originals if candidate.get("instance_of") is not None]
    parents = [art.source_candidate(candidate["instance_of"]["candidate"], document) for candidate in instances]
    # Heavy/retained component records share real originals across multiple authored cues; idle cutter/working aliases also reuse the actual free body. Count unique generated raw images rather than claiming those registrations as extra artwork; older delivered role report semantics remain stable.
    new_original_count = len(originals) - len(instances)
    if asset in (heavy.ASSET, RETAINED_ASSET, hauler.ASSET):
        new_original_count = len({(candidate["path"], candidate["sha256"]) for candidate in originals if candidate.get("instance_of") is None})
    aliases = sum(not row["animated"] for row in family["state_policy"].values()) * len(directions)
    report.write_text(json.dumps(dict(_documentation=__doc__, schema_version=1,
                                     source_record=art.RECORD.relative_to(art.ROOT).as_posix(),
                                     state_policy=family["state_policy"], approved_state_policy_sha256=family["approved_state_policy_sha256"],
                                     source_candidates=originals, shared_original_sources=parents,
                                     new_original_source_count=new_original_count, registered_source_instance_count=len(instances),
                                     atlases=[entry], pose_count=required,
                                     actual_component_pose_count=len(cache), explicit_static_alias_count=aliases,
                                     animation_differences=differences, poses=evidence,
                                     coverage_complete=True, packed=False, runtime_activated=False,
                                     artifacts=[art.identity(output / f"contact-frame-{frame}.png") for frame in range(2)]), indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Publish an explicitly selected delivered-role source matrix without changing shipped payloads."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--asset", choices=(parts.ASSET, "vehicle.model-19", builder.ASSET, pod.ASSET, heavy.ASSET, RETAINED_ASSET, hauler.ASSET), default=parts.ASSET)
    args = parser.parse_args()
    print(json.dumps(derive(args.output, args.asset)))


if __name__ == "__main__":
    main()
