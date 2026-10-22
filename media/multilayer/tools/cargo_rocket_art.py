"""Register genuine cargo-rocket originals and review their fixed native contact.

@file cargo_rocket_art.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17CB.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Source review never creates a hull, starts a flight, publishes coverage or packs art.
Run from media/multilayer with python -m tools.cargo_rocket_art to rebuild review.
"""

import hashlib
import json

from PIL import Image, ImageDraw

import convert_space_art as source
from tools import launch_facility_base as facility
from tools import launch_facility_foregrounds as occupied

ROOT = source.ROOT
MODEL = ROOT / "media/multilayer/source/space/cargo-rocket-model-v1.json"
FIXTURE = ROOT / "src/tests/multilayer/cargo-rocket-source-fixture.json"
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/cargo-rocket-base-v1"
ASSET = "vehicle.model-14"
DIRECTIONS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]
STATES = ["ready", "preparing", "ascent", "orbit", "recovery", "blocked", "service"]


def read_json(path):
    """Read one explicitly named repository document.

    @param path Concrete JSON file, without latest-file discovery.
    @return Decoded document.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def semantic_digest(value):
    """Compare accepted predecessor metadata independently of JSON whitespace.

    @param value Captured family, prefix or public row.
    @return Canonical SHA256 used by the immutable pre-rocket fixture.
    """
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def checked_predecessor(document, public, source_only=False):
    """Protect every completed source and unrelated public family during authoring.

    @param document Live Space source record, allowing rocket-only appends.
    @param public Live shared manifest, with no geometry or packed-range exception.
    @param source_only Reconstruct originals while later native/publication epochs remain history.
    @return None.
    @throws ValueError On predecessor, retained original or payload drift.
    """
    fixture = read_json(FIXTURE)
    # Rocket authoring may append only its own provenance; accepted art and
    # public envelopes cannot drift merely because a new family is in progress.
    if (semantic_digest(document["candidates"][:fixture["candidate_count"]]) != fixture["candidate_prefix_sha256"]
            or semantic_digest(document["excluded_sources"][:fixture["exclusion_count"]]) != fixture["exclusion_prefix_sha256"]
            or semantic_digest({key: value for key, value in public.items() if key != "assets"}) != fixture["public_envelope_sha256"]):
        raise ValueError("cargo rocket predecessor prefix/envelope changed")
    if source_only:
        # Current source recovery preserves original signed prefixes and actual
        # retained originals. Later completed-family/native publication metadata
        # remains historical; the selected converter separately validates its own
        # physical public contract and all independent pixel/registration fixtures.
        for name, expected in fixture["retained_originals"].items():
            source.source_reference(dict(path=name, sha256=expected), document)
        return
    # Completed family metadata and all other public rows remain independently
    # frozen, including the newly qualified launch-center occupied source.
    for key, expected in fixture["completed_source_families"].items():
        if semantic_digest(document["families"].get(key)) != expected:
            raise ValueError("completed Space family changed: " + key)
    rows = {row["asset_id"]: row for row in public["assets"]}
    if len(rows) != len(public["assets"]) or set(rows) != set(fixture["other_public_rows"]) | {ASSET}:
        raise ValueError("public family missing, duplicated or appended")
    for key, expected in fixture["other_public_rows"].items():
        if semantic_digest(rows[key]) != expected:
            raise ValueError("unrelated public source changed: " + key)
    # Checking the actual retained bytes protects rejected originals too; their
    # ancestry cannot be silently rewritten to make a later candidate pass.
    for name, expected in {**fixture["retained_originals"], **fixture["protected_payload"]}.items():
        if source.identity(ROOT / name)["sha256"] != expected:
            raise ValueError("protected original/payload changed: " + name)
    if source.identity(ROOT / fixture["completed_launch_handoff"]["path"]) != fixture["completed_launch_handoff"]:
        raise ValueError("completed launch source handoff changed")


def checked_model(model):
    """Bind the rocket brief to actual paid hull/flight owners and public geometry.

    @param model Cargo-rocket source brief; no new runtime state is authorized.
    @return None.
    @throws ValueError On owner, contact, palette, anchor or source-envelope drift.
    """
    row = next(row for row in read_json(PUBLIC)["assets"] if row["asset_id"] == ASSET)
    expected = dict(directions=DIRECTIONS, frames=2, states=STATES, native_size=[64, 48],
                    anchor=[-32, -32], bounds=[0, 0, 64, 48], parts=["body"],
                    geometry="per_view", tile_variants=1, palette="dos_8bpp", transparent_index=0)
    # These are the original 112 logical positions and fixed landing reference;
    # expanding the canvas or moving the contact cannot rescue a poor original.
    if (model["asset_id"] != ASSET or model["native_model"] != 14
            or model["required_logical_poses"] != 112 or model["native_contact"] != [32, 32]
            or model["body_native_envelope"] != [16, 2, 48, 38]
            or model["native_art_envelope"] != [2, 2, 62, 46]):
        raise ValueError("cargo rocket native source geometry changed")
    for name, value in expected.items():
        if row[name] != value or model["public_geometry"][name] != value:
            raise ValueError("cargo rocket public geometry changed: " + name)
    # Native source hashes retain the original source-model epoch as history.
    # Current physical fit uses the unchanged independent retained fixtures;
    # only separate native recipes qualify payment, payload, burns and phases.
    for reference in [model["source_predecessor_fixture"], model["launch_fit_reference"]]:
        if source.identity(ROOT / reference["path"]) != reference:
            raise ValueError("cargo rocket bound owner/input changed")


def candidate_view(key, direction, document=None, model=None):
    """Read one whole genuine ready body using the approved original transform.

    @param key Exact retained original candidate key.
    @param direction Genuine engine heading contained in that original.
    @param document Optional detached record for refusal checks.
    @param model Optional detached brief for geometry/owner refusal checks.
    @return Indexed native pixels and measured registration evidence.
    @throws ValueError On ancestry, crop, scale, clearance or registration drift.
    """
    document = source.read_record() if document is None else document
    model = read_json(MODEL) if model is None else model
    checked_model(model)
    candidate = source.source_candidate(key, document)
    receipt = read_json(ROOT / candidate["generation_receipt"]["path"])
    # A selected silhouette must retain its executed request and actual input
    # roles. A rehashed image or receipt alone cannot authorize new source art.
    if (receipt != {name: value for name, value in candidate.items() if name != "generation_receipt"}
            or candidate["asset_id"] != ASSET or candidate["state"] != "ready"
            or candidate["tool"] != "built-in image_gen" or not candidate["transparent_background"]):
        raise ValueError("cargo rocket original/receipt identity changed")
    for reference in candidate["reference_inputs"]:
        if not reference["role"]:
            raise ValueError("cargo rocket image input changed")
        source.source_reference(reference, document)
    pixels, evidence = source.native_view(key, direction, document)
    selected = model.get("selected_base")
    # Later phase/direction work cannot move or repaint the accepted NE body,
    # even if its family registration and receipt are coherently rehashed.
    if selected is not None and selected["candidate"] == key:
        signature = source.registration_signature(candidate, direction)
        actual = hashlib.sha256(pixels.tobytes()).hexdigest()
        if signature != selected["registration_sha256"] or actual != selected["native_pixels_sha256"]:
            raise ValueError("selected cargo rocket base changed")
    bounds = evidence["native_subject_bounds"]
    x0, y0, x1, y1 = model["body_native_envelope"]
    # Ready hulls reserve the lower exhaust lane for genuinely authored phase
    # effects. The broader full-art envelope is not a ready-body clipping waiver.
    if bounds[0] < x0 or bounds[1] < y0 or bounds[2] > x1 or bounds[3] > y1:
        raise ValueError("cargo rocket ready body leaves its fixed envelope")
    return pixels, evidence


def opaque_rgb(pixels):
    """Show exact indexed source pixels on a plain diagnostic background.

    @param pixels Native indexed sprite or facility composition.
    @return RGB diagnostic image with no changes to the source artwork.
    """
    result = Image.new("RGB", pixels.size, "#29323c")
    rgba = pixels.convert("RGBA")
    result.paste(rgba, (0, 0), rgba)
    return result


def fit_scene(actor, model, direction, component, fixture):
    """Mount one separate rocket inside the existing zone's physical cut order.

    @param actor Original rocket native pixels, with reference32,32.
    @param model Exact qualified facility occupied-source model.
    @param direction Genuine facility view used only for source fit.
    @param component Pad, workshop or recovery contact; never a cargo assignment.
    @param fixture Original facility states, owners and foreground mask.
    @return RGB source-only diagram and explicit visibility counts.
    """
    images, owners, cells, foreground, evidence = fixture
    whole = images["ready"]
    contact = model["views"][direction]["contacts"][component]["physical_display_contact"]
    ox, oy = contact[0] - 32, contact[1] - 32
    scene = facility.tiles.indexed_canvas(whole.size)
    visible = set()
    # A visitor belongs between its own zone's back/front surfaces. Other
    # machinery keeps the already accepted physical order and may occlude it.
    for owner, name in enumerate(facility.DEPTH_ORDERS[direction], 1):
        passes = (False, True) if name == component else (None,)
        for front in passes:
            if front is True:
                # Only nontransparent source pixels enter the diagram; this
                # never creates a native craft or reveals a Surface Hidden hull.
                for y in range(actor.height):
                    for x in range(actor.width):
                        value = actor.getpixel((x, y))
                        if value:
                            point = (ox + x, oy + y)
                            scene.putpixel(point, value)
                            visible.add(point)
            for index, value in enumerate(whole.get_flattened_data()):
                if owners[index] != owner:
                    continue
                # The physical partition applies only to this visitor's own
                # zone; a global foreground overlay would reorder other zones.
                if front is not None and bool(foreground[index]) != front:
                    continue
                point = (index % whole.width, index // whole.width)
                scene.putpixel(point, value)
                visible.discard(point)
    total = sum(value != 0 for value in actor.get_flattened_data())
    return opaque_rgb(scene), dict(component=component, physical_contact=contact, source_pixels=total,
                                   visible_pixels=len(visible), occluded_pixels=total - len(visible))


def rebuild(key="cargo-rocket-ready-ne-v1", direction="NE"):
    """Rebuild explicit native/base-fit diagnostics from the immutable original.

    @param key Exact base candidate, never discovered by modification date.
    @param direction Its genuine heading; base review defaults to NE only.
    @return Hash-bound source-only review; no source publication/selection.
    """
    model = read_json(MODEL)
    document = source.read_record()
    checked_predecessor(document, read_json(PUBLIC))
    pixels, registration = candidate_view(key, direction, document, model)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    pixels.save(OUTPUT / "native.png", transparency=0)
    contact = Image.new("RGB", (680, 440), "#29323c")
    drawing = ImageDraw.Draw(contact)
    drawing.text((12, 10), direction + " ready: native64x48 and x8; SOURCE ONLY; contact32,32", fill="white")
    contact.paste(opaque_rgb(pixels), (30, 80))
    contact.paste(opaque_rgb(pixels).resize((512, 384), Image.Resampling.NEAREST), (144, 45))
    contact.save(OUTPUT / "contact.png")
    template = Image.new("RGB", (512, 424), "#29323c")
    drawing = ImageDraw.Draw(template)
    drawing.text((8, 4), "NE ready geometry only: 64x48, anchor(-32,-32)", fill="white")
    drawing.rectangle((16*8, 24+2*8, 48*8, 24+38*8), outline="#56dce7", width=2)
    drawing.line((29*8, 24+32*8, 35*8, 24+32*8), fill="#f36798", width=2)
    drawing.line((32*8, 24+29*8, 32*8, 24+35*8), fill="#f36798", width=2)
    template.save(OUTPUT / "geometry-template.png")
    fit_model = read_json(ROOT / model["launch_fit_reference"]["path"])
    fixture = occupied.compose_states(fit_model, "NE")
    fits = Image.new("RGB", (1280, 992), "#29323c")
    drawing = ImageDraw.Draw(fits)
    observations = []
    # All three real rocket-handling zones are checked at their accepted source
    # contacts. These diagrams do not assign native custody, paid stock or phase.
    for index, component in enumerate(("pad", "workshop", "recovery")):
        scene, observation = fit_scene(pixels, fit_model, "NE", component, fixture)
        x, y = index % 2 * 640, index // 2 * 496
        drawing.text((x+8, y+8), "NE " + component + " SOURCE FIT ONLY", fill="white")
        fits.paste(scene.resize((640, 448), Image.Resampling.NEAREST), (x, y+40))
        observations.append(observation)
    fits.save(OUTPUT / "launch-fit.png")
    report = dict(_documentation=__doc__, candidate=key, direction=direction, registration=registration,
                  source_model=source.identity(MODEL), predecessor=source.identity(FIXTURE), fit=observations,
                  source_coverage=False, packed=False, runtime=False,
                  artifacts=[source.identity(OUTPUT / name) for name in ("native.png", "contact.png", "geometry-template.png", "launch-fit.png")])
    (OUTPUT / "source-review.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


if __name__ == "__main__":
    rebuild()
