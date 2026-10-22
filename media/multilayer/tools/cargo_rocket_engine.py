"""Register genuine engine parts behind unchanged cargo-rocket ready originals.

@file cargo_rocket_engine.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17CB.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups62F-62G.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Only retained part pixels, uniform original registration and ordinary source
occlusion are used. Source composition never changes native flight/fuel/custody.
Run from media/multilayer with python -m tools.cargo_rocket_engine.
"""

import hashlib
import json

from PIL import Image, ImageDraw

from tools import cargo_rocket_art as art
from tools import cargo_rocket_directions as directions
from tools import cargo_rocket_states as states

ROOT = art.ROOT
MODEL = ROOT / "media/multilayer/source/space/cargo-rocket-engine-model-v1.json"
OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/cargo-rocket-engine-v1"


def checked_model(model, document):
    """Bind both genuine original frames to the immutable native/source part policy.

    @param model Explicit selected axial engine-part descriptor.
    @param document Append-only Space provenance owner.
    @return None.
    @throws ValueError On geometry, source-policy, qualification or frame drift.
    """
    # The two genuine originals are component art, not a substitute for hull
    # headings, full112-pose coverage or a packed/native rendering handoff.
    if (model["asset_id"] != art.ASSET or model["frames"] != 2
            or set(model["selected"]) != {"0", "1"} or not model["engine_parts_complete"]
            or model["native_size"] != [64, 48] or model["anchor"] != [-32, -32]
            or model["native_mount"] != [32, 34] or model["native_envelope"] != [27, 32, 38, 46]
            or model["part_kind"] != "genuinely_authored_axially_symmetric_engine_plume"
            or model["composite_order"] != "exact original whole ready hull over genuine axial engine part"
            or model["source_coverage"] or model["packed"] or model["runtime"]):
        raise ValueError("cargo rocket engine original/geometry contract changed")
    for reference in (model["state_policy"], model["state_qualification"]):
        if art.source.identity(ROOT/reference["path"]) != reference:
            raise ValueError("cargo rocket engine source-state input changed")
    qualification = art.read_json(ROOT/model["state_qualification"]["path"])
    # The retained source policy fixes original phase ownership. Its dated
    # native proof is historical; real source/fixture checks do not read that
    # old process output or grant paid fuel/progress from an authored flame.
    if not qualification["source_mapping_qualified"] or qualification["policy"] != model["state_policy"]:
        raise ValueError("cargo rocket engine lacks qualified native source policy")
    states.checked_policy(art.read_json(ROOT/model["state_policy"]["path"]))
    if document["families"][art.ASSET]["engine_model"] != art.source.identity(MODEL):
        raise ValueError("cargo rocket engine family/model handoff changed")


def engine_part(frame, document=None, model=None):
    """Decode one exact selected engine original at its frozen physical nozzle mount.

    @param frame Logical engine-effect frame0or1; never a heading or new native timer.
    @param document Optional detached provenance for focused refusal cases.
    @param model Optional detached part selection for focused refusal cases.
    @return Indexed64x48 original part and bound registration/fixture evidence.
    @throws ValueError On missing/rejected/substituted original, registration or pixel drift.
    """
    if type(frame) is not int or frame not in (0, 1):
        # Source-frame addressing cannot silently clamp a missing frame or use
        # native progress/booleans as a new local animation counter.
        raise ValueError("cargo rocket engine frame absent")
    document = art.source.read_record() if document is None else document
    model = art.read_json(MODEL) if model is None else model
    checked_model(model, document)
    selected = model["selected"][str(frame)]
    reference = selected["fixture"]
    if art.source.identity(ROOT/reference["path"]) != reference:
        raise ValueError("immutable cargo rocket engine fixture changed")
    expected = art.read_json(ROOT/reference["path"])
    candidate = art.source.source_candidate(selected["candidate"], document)
    receipt = art.read_json(ROOT/candidate["generation_receipt"]["path"])
    # A hull, other pulse or coherently rehashed original cannot impersonate
    # the reviewed physical component despite plausible colors or dimensions.
    if (receipt != {key: value for key, value in candidate.items() if key != "generation_receipt"}
            or candidate["status"] != "selected_engine_part" or candidate["component"] != "engine_effect"
            or candidate["state"] != "funded_flight_engine_part" or type(candidate["component_frame"]) is not int
            or candidate["component_frame"] != frame
            or candidate["directions"] != ["AXIAL"] or candidate["tool"] != "built-in image_gen"
            or not candidate["transparent_background"] or candidate["key"] != expected["candidate"]
            or expected["component_frame"] != frame or candidate["generation_receipt"] != expected["generation_receipt"]
            or {key: candidate[key] for key in ("path", "sha256")} != expected["original"]
            or art.source.registration_signature(candidate, "AXIAL") != expected["registration_sha256"]):
        raise ValueError("selected cargo rocket engine ancestry/frame/registration changed")
    for actual in candidate["reference_inputs"]:
        if not actual["role"]:
            raise ValueError("cargo rocket engine generation input changed")
        art.source.source_reference(actual, document)
    rgba, evidence = art.source.source_pixels(candidate, "AXIAL", model["native_envelope"])
    pixels = art.source.base.quantize(rgba, {})
    # Every actual native part pixel remains frozen after review; the source
    # mouth is registered by physical contact, never shifted to hide overflow.
    if (candidate["native_transforms"]["AXIAL"]["native_reference"] != model["native_mount"]
            or evidence["native_subject_bounds"] != expected["native_subject_bounds"]
            or hashlib.sha256(pixels.tobytes()).hexdigest() != expected["native_pixels_sha256"]
            or sum(value != 0 for value in pixels.get_flattened_data()) != expected["nontransparent_pixels"]):
        raise ValueError("cargo rocket engine native pixels/contact changed")
    evidence.update(frame=frame, fixture=reference, native_pixels_sha256=expected["native_pixels_sha256"])
    return pixels, evidence


def compose_state(direction, state, frame, document=None, model=None):
    """Compose one logical source pose while retaining every original hull/foot pixel.

    @param direction Genuine already qualified native engine heading.
    @param state Declared public source state; runtime selection remains a later gate.
    @param frame Logical0or1 source frame, selected later from native progress only.
    @param document Optional detached source owner for focused refusal checks.
    @param model Optional detached engine model for focused refusal checks.
    @return Native indexed whole body/effect and explicit static-alias or part evidence.
    @throws ValueError On missing state/frame/heading, changed body/part or native overflow.
    """
    # Public source slots cannot be inferred from an unknown native enum or
    # silently filled by a neighboring heading/frame.
    if state not in art.STATES or type(frame) is not int or frame not in (0, 1):
        raise ValueError("cargo rocket source state/frame absent")
    document = art.source.read_record() if document is None else document
    model = art.read_json(MODEL) if model is None else model
    body, body_evidence = directions.direction_view(direction, document)
    evidence = dict(direction=direction, state=state, frame=frame, body=body_evidence,
                    unchanged_body_pixels=sum(value != 0 for value in body.get_flattened_data()))
    policy = art.read_json(states.POLICY)
    if state in policy["static_alias_states"]:
        # Closed internal loading/preparation, coasting, waiting and actual
        # Turnaround leave the exterior still; both frames alias ready/frame0.
        evidence.update(alias="ready/"+direction+"/0", engine=None)
        return body, evidence
    part, part_evidence = engine_part(frame, document, model)
    whole = part.copy()
    # Whole original ready pixels overdraw their own nozzle/feet naturally.
    # Only its actual transparent background reveals genuine engine material;
    # no guessed body mask, erased pixel or replacement hull is introduced.
    values = [hull if hull else effect for effect, hull in zip(part.get_flattened_data(), body.get_flattened_data())]
    whole.putdata(values)
    bounds = whole.getbbox()
    x0, y0, x1, y1 = policy["effect_part"]["full_native_art_envelope"]
    # The final composite keeps the original public canvas/anchor. Adding a
    # genuine effect cannot authorize silently clipped feet, flame or body.
    if bounds[0] < x0 or bounds[1] < y0 or bounds[2] > x1 or bounds[3] > y1:
        raise ValueError("cargo rocket whole source leaves fixed native art envelope")
    visible = sum(effect != 0 and hull == 0 for effect, hull in zip(part.get_flattened_data(), body.get_flattened_data()))
    evidence.update(alias=None, engine=part_evidence, visible_effect_pixels=visible,
                    occluded_effect_pixels=sum(value != 0 for value in part.get_flattened_data())-visible,
                    whole_bounds=list(bounds), whole_pixels_sha256=hashlib.sha256(whole.tobytes()).hexdigest())
    return whole, evidence


def rebuild():
    """Rebuild all16 genuine-heading burn compositions and readable frame contacts.

    @return Original-part/source-frame proof; complete112-position publication stays open.
    """
    document, model = art.source.read_record(), art.read_json(MODEL)
    checked_model(model, document)
    art.checked_predecessor(document, art.read_json(art.PUBLIC))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT/"burn").mkdir(exist_ok=True)
    atlas = Image.new("P", (512, 96), 0)
    atlas.putpalette(art.source.base.PALETTE_BYTES)
    entries, artifacts = [], []
    # Each frame contact compares all eight genuine hull headings at native
    # size and x8; a shared axial plume never supplies a missing hull orientation.
    for frame in (0, 1):
        part, part_evidence = engine_part(frame, document, model)
        part_path = OUTPUT/("engine-frame"+str(frame)+"-native.png")
        part.save(part_path, transparency=0)
        artifacts.append(art.source.identity(part_path))
        contact = Image.new("RGB", (2048, 880), "#29323c")
        draw = ImageDraw.Draw(contact)
        for index, direction in enumerate(art.DIRECTIONS):
            pixels, evidence = compose_state(direction, "ascent", frame, document, model)
            path = OUTPUT/"burn"/(direction.lower()+"-frame"+str(frame)+".png")
            pixels.save(path, transparency=0)
            atlas.paste(pixels, (index*64, frame*48))
            x, y = index % 4*512, index//4*440
            draw.text((x+8, y+8), direction+" engine frame"+str(frame)+" / SOURCE ONLY / exact hull", fill="white")
            contact.paste(art.opaque_rgb(pixels), (x+16, y+24))
            contact.paste(art.opaque_rgb(pixels).resize((512, 384), Image.Resampling.NEAREST), (x, y+56))
            entries.append(dict(**evidence, image=art.source.identity(path)))
        path = OUTPUT/("burn-frame"+str(frame)+"-contact.png")
        contact.save(path)
        artifacts.append(art.source.identity(path))
    atlas.save(OUTPUT/"burn-native.png", transparency=0)
    artifacts.append(art.source.identity(OUTPUT/"burn-native.png"))
    report = dict(_documentation=__doc__, engine_model=art.source.identity(MODEL),
                  policy=art.source.identity(states.POLICY), frames=entries, artifacts=artifacts,
                  source_coverage=False, packed=False, runtime=False)
    (OUTPUT/"source-review.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return report


if __name__ == "__main__":
    rebuild()
