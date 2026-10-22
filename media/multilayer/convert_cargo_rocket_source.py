"""Export and decode the complete genuine cargo-rocket source/alias handoff.

@file convert_cargo_rocket_source.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups17C/17CB.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62G.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Original bodies and engine parts supply24canonical images/112logical positions.
Source qualification neither packs sprites nor selects a live native pose. The
focused cargo_rocket_source_checks.py publisher owns subsequent source updates.
"""

import hashlib
import json

from PIL import Image, ImageDraw

from tools import cargo_rocket_art as art
from tools import cargo_rocket_directions as directions
from tools import cargo_rocket_engine as engine
from tools import cargo_rocket_states as states

ROOT = art.ROOT
POLICY = ROOT/"media/multilayer/source/space/cargo-rocket-state-art-v1.json"
OUTPUT = ROOT/"logs/proofs/mlm/native-art/space/cargo-rocket-source-v1"
MANIFEST = OUTPUT/"source-manifest.json"
VERIFICATION = OUTPUT/"verification.json"
FIELDS = ("direction", "state", "frame", "tile_variant", "part")


def key(row):
    """Identify one declared logical source position independently of pixel similarity.

    @param row Actual or canonical pose metadata.
    @return Immutable direction/state/frame/tile/part identity.
    """
    return tuple(row[name] for name in FIELDS)


def pose(direction, state, frame):
    """Address an actual declared whole-body source slot without allocating native IDs.

    @param direction Genuine native engine heading.
    @param state Public rocket source state.
    @param frame Explicit logical frame0or1.
    @return Established direct source-pose reference.
    """
    return dict(direction=direction, state=state, frame=frame, tile_variant=0, part="body")


def write_json(path, document):
    """Write one explicitly owned source/report document with stable formatting.

    @param path Repository-local source/report destination.
    @param document JSON-compatible source evidence or public metadata.
    @return None.
    """
    path.write_text(json.dumps(document, indent=2)+"\n", encoding="utf-8")


def bound_json(reference):
    """Read an exact repository input after verifying its immutable identity.

    @param reference Explicit relative path and SHA256 from its source owner.
    @return Decoded dictionary, never a recency-selected or external input.
    @throws ValueError On missing, changed or outside-repository source data.
    """
    path = (ROOT/reference["path"]).resolve()
    # Source provenance is repository-owned; even a matching digest cannot
    # turn an arbitrary workstation file into a selected original/proof.
    if not path.is_relative_to(ROOT) or not path.is_file():
        raise ValueError("rocket source bound input is missing or outside repository")
    if art.source.identity(path) != reference:
        raise ValueError("rocket source bound input changed")
    value = art.read_json(path)
    if not isinstance(value, dict):
        raise ValueError("rocket source bound input is not a document")
    return value


def checked_art(policy=None, document=None):
    """Require actual native, genuine-view and engine qualification before source export.

    @param policy Optional detached material handoff for focused refusal checks.
    @param document Optional detached append-only source record for refusal checks.
    @return Qualified policy, current provenance, ready fixtures and burn fixture.
    @throws ValueError On absent parts, false scope, stale producers or substituted ancestry.
    """
    policy = art.read_json(POLICY) if policy is None else policy
    document = art.source.read_record() if document is None else document
    family = document["families"][art.ASSET]
    # Material completeness requires the real preceding proofs and original
    # inputs. Copying a completion flag cannot authorize missing poses or art.
    if (policy.get("asset_id") != art.ASSET or policy.get("state_art_complete") is not True
            or policy.get("source_coverage") is not False or policy.get("packed") is not False
            or policy.get("runtime") is not False or family["state_art"] != art.source.identity(POLICY)
            or policy.get("static_alias_states") != ["ready", "preparing", "orbit", "blocked", "service"]
            or policy.get("active_states") != ["ascent", "recovery"]):
        raise ValueError("rocket complete original state-art qualification is unavailable")
    inputs = {name: bound_json(policy[name]) for name in
              ("state_policy", "state_qualification", "ready_qualification", "direction_model", "engine_model", "engine_qualification")}
    phase, ready, motor = inputs["state_qualification"], inputs["ready_qualification"], inputs["engine_qualification"]
    # All three producer stages must refer to these same physical bodies,
    # source policy and original engine parts; another family's success is insufficient.
    if (phase.get("source_mapping_qualified") is not True or phase["policy"] != policy["state_policy"]
            or ready.get("ready_views_qualified") is not True or ready.get("ready_fit_qualified") is not True
            or ready["direction_model"] != policy["direction_model"]
            or motor.get("engine_parts_qualified") is not True or motor.get("body_compositions_qualified") is not True
            or motor["engine_model"] != policy["engine_model"]
            or family["engine_model"] != policy["engine_model"] or family["direction_model"] != policy["direction_model"]):
        raise ValueError("rocket source stages do not share the qualified original inputs")
    # Source-era proof identities remain historical facts in these unchanged
    # retained contracts. Fresh conversion/decoding checks actual original and
    # independent registration/pixel fixtures, never old success counters.
    states.checked_policy(inputs["state_policy"])
    directions.checked_model(inputs["direction_model"], document)
    art.checked_model(art.read_json(art.MODEL))
    public = art.read_json(art.PUBLIC)
    row = next(row for row in public["assets"] if row["asset_id"] == art.ASSET)
    before = art.read_json(art.FIXTURE)["rocket_before"]
    # Independent native publication later updates packed/runtime evidence.
    # Source conversion owns the unchanged physical canvas, state and company
    # policy; native ranges/decoding are separately verified by packing recipes.
    if ({name: value for name, value in row.items() if name not in ("source", "authoring", "packed", "runtime")}
            != {name: value for name, value in before.items() if name not in ("source", "authoring", "packed", "runtime")}):
        raise ValueError("rocket source public geometry or source policy changed")
    ready_fixtures = {}
    # Each genuine heading keeps its independent original/registration/pixel
    # fixture. Pixel symmetry never supplies another heading or missing receipt.
    for heading in art.DIRECTIONS:
        selected = inputs["direction_model"]["selected"][heading]
        fixture, candidate = audited_original(selected, heading, document)
        if candidate["state"] != "ready":
            raise ValueError("rocket source selected ready state changed")
        ready_fixtures[heading] = fixture
    # Both genuinely authored pulse originals remain selected, with their
    # exact frame identities. A changed record cannot reuse one pulse twice.
    for frame in (0, 1):
        fixture, candidate = audited_original(inputs["engine_model"]["selected"][str(frame)], "AXIAL", document)
        if (type(candidate["component_frame"]) is not int or candidate["component_frame"] != frame
                or fixture["component_frame"] != frame or candidate["status"] != "selected_engine_part"
                or candidate["state"] != "funded_flight_engine_part" or candidate["component"] != "engine_effect"):
            raise ValueError("rocket source selected engine frame changed")
    burn_fixture = bound_json(motor["composition_fixture"])
    # Reviewed whole-body pixels must belong to this engine and phase owner;
    # an unrelated successful composition cannot supply the missing contract.
    if burn_fixture["engine_model"] != policy["engine_model"] or burn_fixture["policy"] != policy["state_policy"]:
        raise ValueError("rocket source burn-pixel fixture changed physical owner")
    return policy, document, ready_fixtures, burn_fixture


def audited_original(selection, heading, document):
    """Bind a selected original, full request receipt and registration to its frozen fixture.

    @param selection Qualified candidate and immutable fixture reference.
    @param heading Genuine source direction, or AXIAL for an isolated engine part.
    @param document Current append-only generation record or detached refusal input.
    @return Exact fixture and verified selected original metadata.
    @throws ValueError On changed original, receipt, generation input or registration.
    """
    fixture = bound_json(selection["fixture"])
    candidate = art.source.source_candidate(selection["candidate"], document)
    receipt = bound_json(candidate["generation_receipt"])
    # Full executed receipts protect physical view, source transforms and
    # prompt/input roles as well as raw pixels. Rehashing metadata is insufficient.
    if (candidate["key"] != fixture["candidate"] or candidate["asset_id"] != art.ASSET
            or candidate["generation_receipt"] != fixture["generation_receipt"]
            or receipt != {name: value for name, value in candidate.items() if name != "generation_receipt"}
            or {name: candidate[name] for name in ("path", "sha256")} != fixture["original"]
            or heading not in candidate["native_transforms"]
            or art.source.registration_signature(candidate, heading) != fixture["registration_sha256"]
            or candidate["tool"] != "built-in image_gen" or candidate["transparent_background"] is not True):
        raise ValueError("rocket source selected original/receipt/registration changed")
    # All actual generation inputs are immutable provenance, including geometry
    # guides stored as rebuildable proof artifacts rather than shipped artwork.
    for reference in candidate["reference_inputs"]:
        if not reference["role"]:
            raise ValueError("rocket source actual generation input changed")
        art.source.source_reference(reference, document)
    return fixture, candidate


def ready_evidence(document, fixtures, policy):
    """Reconstruct expected ready provenance independently of a derived atlas report.

    @param document Verified current source record containing eight genuine originals.
    @param fixtures Immutable ready registration/native-pixel expectations.
    @param policy Qualified material policy referencing the actual direction model.
    @return Complete expected per-heading evidence without rerendering original pixels.
    """
    selected = bound_json(policy["direction_model"])["selected"]
    result = {}
    # The report may describe only the already qualified measured transforms;
    # plausible but unrelated source identities cannot accompany accepted PNGs.
    for heading in art.DIRECTIONS:
        candidate = next(row for row in document["candidates"] if row["key"] == selected[heading]["candidate"])
        fixture = fixtures[heading]
        result[heading] = dict(direction=heading, original=fixture["original"], transform=candidate["native_transforms"][heading],
                               native_subject_bounds=fixture["native_subject_bounds"], fixture=selected[heading]["fixture"],
                               native_pixels_sha256=fixture["native_pixels_sha256"])
    return result


def canonical_target(row, policy):
    """Choose only declared same-heading static or equivalent funded-state aliases.

    @param row Actual public source slot; required coverage is checked separately.
    @param policy Qualified original material policy.
    @return Direct canonical pose and truthful alias class or no alias class.
    """
    if row["state"] in policy["static_alias_states"]:
        # Closed internal loading/handling, coasting and waiting retain the
        # same exterior; their logical frames introduce no visible hull motion.
        target = pose(row["direction"], "ready", 0)
        return target, None if key(row) == key(target) else "static"
    # Both already-funded flight legs use the same physical axial engine.
    # Recovery preserves its actual frame instead of becoming a static alias.
    target = pose(row["direction"], "ascent", row["frame"])
    return target, None if row["state"] == "ascent" else "equivalent_active_state"


def expected_hashes(ready_fixtures, burn_fixture):
    """Resolve independent reviewed native pixel expectations for all24canonical poses.

    @param ready_fixtures Eight immutable genuine ready-view fixtures.
    @param burn_fixture Frozen all16whole-body/engine composition expectations.
    @return Canonical pose-to-native-byte SHA256 map, independent of derived reports.
    """
    expected = {key(pose(heading, "ready", 0)): ready_fixtures[heading]["native_pixels_sha256"] for heading in art.DIRECTIONS}
    # Engine fixture values were frozen after visual/source ownership review;
    # a rehashed PNG/report cannot redefine these original whole-body pixels.
    for heading in art.DIRECTIONS:
        for frame in (0, 1):
            expected[key(pose(heading, "ascent", frame))] = burn_fixture["poses"][heading+"/"+str(frame)]["whole_pixels_sha256"]
    return expected


def review_contacts(images, output=OUTPUT):
    """Save both logical frames for every heading/state at native and readable x4 scale.

    @param images The24canonical whole native images, addressed by canonical pose key.
    @param output Explicit source images owner; selected runs never write old proof paths.
    @return Seven explicit source-only contact identities; no game rendering claim.
    """
    result = []
    policy = art.read_json(POLICY)
    # A separate complete per-state sheet makes all public frames inspectable;
    # captions declare exact aliases rather than suggesting invented activity.
    for state in art.STATES:
        contact = Image.new("RGB", (2048, 624), "#29323c")
        draw = ImageDraw.Draw(contact)
        for index, heading in enumerate(art.DIRECTIONS):
            x, y = index % 4*512, index//4*312
            draw.text((x+8, y+8), heading+" "+state+" / logical frames0,1 / SOURCE ONLY", fill="white")
            for frame in (0, 1):
                target, alias = canonical_target(pose(heading, state, frame), policy)
                pixels = images[key(target)]
                draw.text((x+frame*256+8, y+28), "frame"+str(frame)+(" alias" if alias else " original"), fill="white")
                contact.paste(art.opaque_rgb(pixels), (x+frame*256+8, y+48))
                contact.paste(art.opaque_rgb(pixels).resize((256, 192), Image.Resampling.NEAREST), (x+frame*256, y+112))
        path = output/(state+"-contact.png")
        contact.save(path)
        result.append(art.source.identity(path))
    return result


def decode_contacts(report, crops, policy, output=OUTPUT):
    """Verify every stored state/frame contact displays the actual decoded canonical pixels.

    @param report Complete handoff carrying seven exact review image identities.
    @param crops Independently validated canonical atlas images.
    @param policy Qualified source alias policy with no runtime mutation authority.
    @param output Exact current source images owner, or explicit historical proof owner.
    @return None after all112native and enlarged contact panels match their real source.
    @throws ValueError On missing, changed, stale, clipped or relabelled review images.
    """
    expected = [art.source.identity(output/(state+"-contact.png")) for state in art.STATES]
    if report["contacts"] != expected:
        raise ValueError("rocket source complete contact identities changed")
    # Every visible panel must show its own actual frame, including direct
    # aliases. A green coverage report cannot conceal missing review artwork.
    for state, reference in zip(art.STATES, expected):
        with Image.open(ROOT/reference["path"]) as contact:
            # Complete review requires readable, unclipped native/x4 panels
            # on the expected source-only canvas, independently of its file hash.
            if contact.mode != "RGB" or contact.size != (2048, 624):
                raise ValueError("rocket source review contact canvas changed")
            for index, heading in enumerate(art.DIRECTIONS):
                x, y = index % 4*512, index//4*312
                for frame in (0, 1):
                    target, _ = canonical_target(pose(heading, state, frame), policy)
                    pixels = art.opaque_rgb(crops[key(target)])
                    native = contact.crop((x+frame*256+8, y+48, x+frame*256+72, y+96))
                    enlarged = contact.crop((x+frame*256, y+112, x+frame*256+256, y+304))
                    # Both scales show the exact selected source slot; a stale
                    # diagram cannot stand in for either actual animation frame.
                    if (native.tobytes() != pixels.tobytes()
                            or enlarged.tobytes() != pixels.resize((256, 192), Image.Resampling.NEAREST).tobytes()):
                        raise ValueError("rocket source review contact pixels differ from decoded source")


def derive(output=OUTPUT, context=None):
    """Rebuild complete original source coverage without using ignored PNGs as artwork.

    @param output Exact owned source-proof directory; arbitrary destinations are refused.
    @param context Explicit selected source invocation separating images and reports.
    @return Full112-position source report with24canonical images and88direct aliases.
    @throws ValueError On missing qualified originals, unsafe output or frozen-pixel drift.
    """
    if context is None:
        if output.resolve() != OUTPUT.resolve():
            raise ValueError("rocket source conversion requires its explicit owned proof directory")
        reports = output
    else:
        from regenerate_art_sources import source_directories
        output, reports = source_directories(output, context)
    policy, document, ready_fixtures, burn_fixture = checked_art()
    art.checked_predecessor(document, art.read_json(art.PUBLIC), source_only=context is not None)
    engine_model = bound_json(policy["engine_model"])
    engine.checked_model(engine_model, document)
    parts = {frame: engine.engine_part(frame, document, engine_model)[0] for frame in (0, 1)}
    images, evidence = {}, {}
    # Canonical bodies are always regenerated from their own genuine originals.
    # Exact whole body pixels occlude real parts without guessed masks or new art.
    for heading in art.DIRECTIONS:
        body, source_evidence = directions.direction_view(heading, document)
        images[key(pose(heading, "ready", 0))] = body
        evidence[heading] = source_evidence
        for frame in (0, 1):
            whole = parts[frame].copy()
            whole.putdata([hull if hull else effect for effect, hull in zip(parts[frame].get_flattened_data(), body.get_flattened_data())])
            images[key(pose(heading, "ascent", frame))] = whole
    expected = expected_hashes(ready_fixtures, burn_fixture)
    # No artifact is overwritten until every newly regenerated canonical image
    # matches its independently frozen original/body/part pixel expectation.
    if set(images) != set(expected) or any(hashlib.sha256(image.tobytes()).hexdigest() != expected[name] for name, image in images.items()):
        raise ValueError("rocket source regenerated original pixels changed")
    output.mkdir(parents=True, exist_ok=True)
    canonical_directory = output if context is not None else output / "canonical"
    canonical_directory.mkdir(exist_ok=True)
    atlas = Image.new("P", (512, 144), 0)
    atlas.putpalette(art.source.base.PALETTE_BYTES)
    canonical = []
    # Ready and the two active frames occupy three deterministic native rows.
    # These are physical source poses; no runtime sprite IDs are allocated here.
    for line, (state, frame) in enumerate((("ready", 0), ("ascent", 0), ("ascent", 1))):
        for column, heading in enumerate(art.DIRECTIONS):
            row = pose(heading, state, frame)
            pixels = images[key(row)]
            path = canonical_directory/(state+"-"+heading.lower()+"-"+str(frame)+".png")
            pixels.save(path, transparency=0)
            box = [column*64, line*48, 64, 48]
            atlas.paste(pixels, tuple(box[:2]))
            canonical.append(dict(row, asset_id=art.ASSET, native_size=[64, 48], anchor=[-32, -32],
                                  native_contact=[32, 32], box=box, alias_of=None, alias_kind=None,
                                  native_sha256=expected[key(row)], opaque_pixels=sum(value != 0 for value in pixels.get_flattened_data()),
                                  opaque_bbox=list(pixels.getbbox()), structurally_absent=False, company_mask=[],
                                  image=art.source.identity(path)))
    atlas_path, mask_path = output/"cargo-rocket-authored.png", output/"company-mask.png"
    atlas.save(atlas_path, transparency=0)
    # The selected original has no company-livery region. An explicit empty
    # mask preserves the public policy without inventing or recoloring hardware.
    Image.new("L", atlas.size, 0).save(mask_path)
    targets = {key(row): row for row in canonical}
    logical = []
    for state in art.STATES:
        for frame in (0, 1):
            for heading in art.DIRECTIONS:
                slot = pose(heading, state, frame)
                target_pose, alias_kind = canonical_target(slot, policy)
                target = targets[key(target_pose)]
                logical.append(dict(target, **{name: slot[name] for name in ("state", "frame")},
                                    alias_of=None if alias_kind is None else target_pose, alias_kind=alias_kind))
    report = dict(_documentation=__doc__, schema_version=1, asset_id=art.ASSET, source_record=art.source.RECORD.relative_to(ROOT).as_posix(),
                  policy=art.source.identity(POLICY), converter=art.source.identity(ROOT/"media/multilayer/convert_cargo_rocket_source.py"),
                  directions=list(art.DIRECTIONS), states=list(art.STATES), frames=2, tile_variants=1, parts=["body"],
                  native_size=[64, 48], anchor=[-32, -32], native_contact=[32, 32],
                  canonical_parts=canonical, sprites=logical, evidence=evidence,
                  logical_pose_count=112, canonical_part_count=24, explicit_alias_count=88,
                  explicit_static_alias_count=72, equivalent_state_alias_count=16,
                  missing_pose_count=0, coverage_complete=True, packed=False, runtime_activated=False,
                  atlas=art.source.identity(atlas_path), company_mask=dict(kind="explicit_empty", pixels=0, image=art.source.identity(mask_path)),
                  contacts=review_contacts(images, output))
    write_json(reports / "source-manifest.json", report)
    return report


def decode(report, atlas=None, mask=None, context=None):
    """Decode every saved logical PNG position against independent genuine-pixel fixtures.

    @param report Complete source report or detached metadata refusal input.
    @param atlas Optional actual opened indexed PNG, also used for pixel corruption cases.
    @param mask Optional actual explicit company mask or detached corruption input.
    @param context Optional current source invocation binding all generated input owners.
    @return All112logical metadata/image pairs resolved through direct canonical targets.
    @throws ValueError On missing/duplicate/forged source, aliases, pixels, palette or geometry.
    """
    policy, document, ready_fixtures, burn_fixture = checked_art()
    expected = expected_hashes(ready_fixtures, burn_fixture)
    required = {key(pose(heading, state, frame)) for heading in art.DIRECTIONS for state in art.STATES for frame in (0, 1)}
    observed = [key(row) for row in report["sprites"]]
    # Coverage is the actual complete public product, not just a success flag
    # or aggregate counter; source and native/game acceptance remain separate.
    if (report["asset_id"] != art.ASSET or report["directions"] != art.DIRECTIONS or report["states"] != art.STATES
            or type(report["frames"]) is not int or type(report["tile_variants"]) is not int
            or report["frames"] != 2 or report["tile_variants"] != 1 or report["parts"] != ["body"]
            or len(observed) != 112 or set(observed) != required or report["logical_pose_count"] != 112
            or report["canonical_part_count"] != 24 or report["explicit_alias_count"] != 88
            or report["explicit_static_alias_count"] != 72 or report["equivalent_state_alias_count"] != 16
            or report["missing_pose_count"] != 0 or report["coverage_complete"] is not True
            or report["packed"] is not False or report["runtime_activated"] is not False
            or report["native_size"] != [64, 48] or report["anchor"] != [-32, -32] or report["native_contact"] != [32, 32]
            or report["policy"] != art.source.identity(POLICY)
            or report["evidence"] != ready_evidence(document, ready_fixtures, policy)
            or report["converter"] != art.source.identity(ROOT/"media/multilayer/convert_cargo_rocket_source.py")):
        raise ValueError("rocket source complete coverage/provenance/native scope changed")
    output = OUTPUT if context is None else context.images
    canonical_directory = output / "canonical" if context is None else output
    atlas_path, mask_path = output/"cargo-rocket-authored.png", output/"company-mask.png"
    # Exact owned artifact paths and current bytes bind the real stored inputs;
    # callers cannot substitute another family's image or an external PNG.
    if (report["atlas"] != art.source.identity(atlas_path)
            or report["company_mask"] != dict(kind="explicit_empty", pixels=0, image=art.source.identity(mask_path))):
        raise ValueError("rocket source stored atlas/mask identity changed")
    atlas = Image.open(atlas_path) if atlas is None else atlas
    mask = Image.open(mask_path) if mask is None else mask
    # Native loading expects the canonical DOS indices/P0 transparency. The
    # explicit empty mask grants no company recoloring of original hardware.
    if (atlas.mode != "P" or atlas.size != (512, 144) or atlas.getpalette() != art.source.base.PALETTE_BYTES
            or atlas.info.get("transparency") != 0 or mask.mode != "L" or mask.size != atlas.size or mask.getbbox() is not None
            or set(atlas.get_flattened_data()) & (set(art.source.base.FORBIDDEN)-{0})):
        raise ValueError("rocket source indexed palette/transparency or explicit mask changed")
    canonical = {key(row): row for row in report["canonical_parts"]}
    # Canonical coverage is an exact set of physical images, preventing a
    # duplicated row from compensating numerically for an absent heading/frame.
    if len(canonical) != 24 or len(report["canonical_parts"]) != 24 or set(canonical) != set(expected):
        raise ValueError("rocket source canonical coverage is missing or duplicated")
    crops = {}
    # Canonical boxes, saved images and indexed bytes are checked independently
    # of their declared hashes. Rehashing altered pixels cannot change a fixture.
    for name, row in canonical.items():
        heading, state, frame, tile, part = name
        line = 0 if state == "ready" else 1+frame
        box = [art.DIRECTIONS.index(heading)*64, line*48, 64, 48]
        path = canonical_directory/(state+"-"+heading.lower()+"-"+str(frame)+".png")
        # Every source crop keeps its actual fixed contact/canvas and direct
        # physical identity; geometry edits cannot be disguised as alias metadata.
        if (row["box"] != box or any(type(value) is not int for value in row["box"])
                or type(frame) is not int or type(tile) is not int or row["asset_id"] != art.ASSET
                or row["native_size"] != [64, 48] or row["anchor"] != [-32, -32]
                or row["native_contact"] != [32, 32] or row["company_mask"] != []
                or row["alias_of"] is not None or row["alias_kind"] is not None
                or row["image"] != art.source.identity(path)):
            raise ValueError("rocket source canonical native rectangle/target changed")
        x, y, width, height = box
        pixels = atlas.crop((x, y, x+width, y+height))
        saved = Image.open(path)
        # Frozen original pixels are independent of all generated hashes, so
        # coherently rehashing a changed atlas and individual image still fails.
        if (saved.mode != "P" or saved.size != (64, 48) or saved.getpalette() != art.source.base.PALETTE_BYTES
                or saved.info.get("transparency") != 0 or saved.tobytes() != pixels.tobytes()
                or hashlib.sha256(pixels.tobytes()).hexdigest() != expected[name]
                or row["native_sha256"] != expected[name]
                or row["opaque_pixels"] != sum(value != 0 for value in pixels.get_flattened_data())
                or row["opaque_bbox"] != list(pixels.getbbox()) or row["structurally_absent"] is not False):
            raise ValueError("rocket source decoded pixels differ from qualified original body/engine")
        crops[name] = pixels
    result, counts = [], {"static": 0, "equivalent_active_state": 0, None: 0}
    # Every alias retains its exact heading/frame policy and canonical image.
    # Active recovery shares its funded engine frame, never an idle still frame.
    for row in report["sprites"]:
        target_pose, alias_kind = canonical_target(row, policy)
        target = canonical[key(target_pose)]
        expected_alias = None if alias_kind is None else target_pose
        # Alias address fields are exact source integers. Python's False==0
        # must not admit malformed metadata that another decoder would reject.
        if expected_alias is not None:
            alias = row["alias_of"]
            if (not isinstance(alias, dict) or set(alias) != set(FIELDS)
                    or type(alias["frame"]) is not int or type(alias["tile_variant"]) is not int):
                raise ValueError("rocket source alias target has invalid source address types")
        # A logical slot may share only its declared physical pose, retaining
        # the original body, heading, active frame, palette mask and contact.
        if (row["asset_id"] != art.ASSET or type(row["frame"]) is not int or type(row["tile_variant"]) is not int
                or row["alias_of"] != expected_alias or row["alias_kind"] != alias_kind
                or row["native_size"] != [64, 48] or row["anchor"] != [-32, -32] or row["native_contact"] != [32, 32]
                or row["box"] != target["box"] or any(type(value) is not int for value in row["box"])
                or row["image"] != target["image"]
                or row["native_sha256"] != target["native_sha256"] or row["opaque_pixels"] != target["opaque_pixels"]
                or row["opaque_bbox"] != target["opaque_bbox"] or row["structurally_absent"] is not False or row["company_mask"] != []):
            raise ValueError("rocket source direct alias/frame/native geometry changed")
        counts[alias_kind] += 1
        result.append((row, crops[key(target_pose)].copy()))
    if counts != {"static": 72, "equivalent_active_state": 16, None: 24}:
        raise ValueError("rocket source actual alias coverage differs from declared policy")
    decode_contacts(report, crops, policy, output)
    return result


def publish(report):
    """Publish only this family's qualified complete source/authoring metadata.

    @param report Actual saved and focused-tested112-position source handoff.
    @return Current source-manifest identity after protected metadata checks.
    @throws ValueError On incomplete source, stale focused verification or payload drift.
    """
    checked_art()
    if art.read_json(MANIFEST) != report:
        raise ValueError("rocket source report differs from its actual saved handoff")
    decode(report)
    verification = art.read_json(VERIFICATION)
    # Public source follows actual saved-image/alias/refusal verification,
    # not a hand-written completion flag or an unrelated family's green test.
    if (verification["status"] != "passed" or verification["cases"] != 7
            or verification["source_manifest"] != art.source.identity(MANIFEST)
            or verification["converter"] != art.source.identity(ROOT/"media/multilayer/convert_cargo_rocket_source.py")
            or verification["tests"] != art.source.identity(ROOT/"src/tests/multilayer/cargo_rocket_source_checks.py")):
        raise ValueError("rocket source current focused publication qualification is absent")
    public, document = art.read_json(art.PUBLIC), art.source.read_record()
    art.checked_predecessor(document, public)
    row = next(row for row in public["assets"] if row["asset_id"] == art.ASSET)
    before = dict(row["packed"])
    primary = bound_json(art.read_json(ROOT/"src/tests/multilayer/cargo-rocket-base-fixture.json")["generation_receipt"])
    reference = art.source.identity(MANIFEST)
    row["source"] = dict(status="authored", path=primary["path"], sha256=primary["sha256"],
                         provenance="first_party_original_builtin_image_gen", rights="first_party_generated_original",
                         composition="whole_genuine_ready_hulls_with_qualified_axial_engine_parts", composition_recipe=report["policy"])
    row["authoring"] = dict(date="2026-10-20", status="source_complete_runtime_pending", source_record=report["source_record"],
                            coverage_complete=True, runtime_activated=False, atlas_manifest=reference["path"],
                            atlas_manifest_sha256=reference["sha256"], native_atlas=report["atlas"]["path"], native_sha256=report["atlas"]["sha256"],
                            required_pose_count=112, staged_pose_count=112, missing_pose_count=0,
                            actual_component_pose_count=24, canonical_part_count=24, explicit_alias_count=88,
                            explicit_static_alias_count=72, equivalent_state_alias_count=16,
                            selected_owned_original_count=10, company_mask=report["company_mask"])
    document["families"][art.ASSET].update(status="source_complete_runtime_pending", source_coverage=True,
                                          coverage_complete=True, source_manifest=reference)
    # This mutation is only the already-owned public source row/family. The
    # original packed namespace/offset/payload and all predecessor rows stay exact.
    if row["packed"] != before:
        raise ValueError("rocket source attempted to modify a packed range")
    art.checked_predecessor(document, public)
    write_json(art.PUBLIC, public)
    write_json(art.source.RECORD, document)
    return reference


if __name__ == "__main__":
    derive()
