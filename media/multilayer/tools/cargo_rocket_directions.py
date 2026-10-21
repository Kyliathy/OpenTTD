"""Validate frozen genuine ready headings without inventing rocket phase source.

@file cargo_rocket_directions.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups62B-62D.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Public Handoff Contract.
Each selected heading has an immutable original/receipt/registration/pixel fixture.
Run from media/multilayer with python -m tools.cargo_rocket_directions.
"""

import hashlib
import json

from PIL import Image, ImageDraw

from tools import cargo_rocket_art as art
from tools import cargo_rocket_direction_guides as guides

ROOT = art.ROOT
MODEL = guides.CONTRACT
OUTPUT = guides.OUTPUT


def checked_model(model, document):
    """Check the genuine ready subset against its source owner and public headings.

    @param model Explicit ready-direction selection descriptor.
    @param document Current append-only Space generation record.
    @return None.
    @throws ValueError On missing inputs, duplicate headings, false coverage or geometry drift.
    """
    family = document["families"][art.ASSET]
    selected = model["selected"]
    expected_views = {name: dict(yaw_from_ne=value[0], service_face=value[1]) for name, value in guides.VIEWS.items()}
    # Ready yaw completion needs all eight genuine sources; neither a partial
    # subset nor ready coverage can authorize the separate 112-pose handoff.
    if (model["asset_id"] != art.ASSET or model["native_size"] != [64, 48]
            or model["anchor"] != [-32, -32] or model["native_contact"] != [32, 32]
            or model["views"] != expected_views or not set(selected) <= set(art.DIRECTIONS)
            or "NE" not in selected or model["genuine_ready_views_complete"] != (set(selected) == set(art.DIRECTIONS))
            or model["source_coverage"] or model["packed"] or model["runtime"]):
        raise ValueError("cargo rocket ready direction contract/coverage changed")
    for reference in (model["source_model"], model["base_fixture"]):
        if art.source.identity(ROOT / reference["path"]) != reference:
            raise ValueError("cargo rocket direction base input changed")
    if family["direction_model"] != art.source.identity(MODEL):
        raise ValueError("cargo rocket family direction handoff changed")
    # The family and model name the same genuine ready subset; a second hidden
    # selection cannot slip into later conversion outside its reviewed fixture.
    expected_selections = {"ready/"+name: row["candidate"] for name, row in selected.items()}
    if family["selections"] != expected_selections:
        raise ValueError("cargo rocket family/model ready selections disagree")


def direction_view(direction, document=None, model=None):
    """Convert one genuine selected heading and enforce its immutable reviewed pixels.

    @param direction Exact native engine heading; missing views are never aliased.
    @param document Optional detached provenance for refusal checks.
    @param model Optional detached selection descriptor for refusal checks.
    @return Indexed native pixels and original evidence plus frozen fixture identity.
    @throws ValueError On source/fixture/registration/heading/pixel drift.
    """
    document = art.source.read_record() if document is None else document
    model = art.read_json(MODEL) if model is None else model
    checked_model(model, document)
    if direction not in model["selected"]:
        raise ValueError("missing genuine rocket ready heading")
    selected = model["selected"][direction]
    reference = selected["fixture"]
    if art.source.identity(ROOT / reference["path"]) != reference:
        raise ValueError("immutable rocket heading fixture changed")
    expected = art.read_json(ROOT / reference["path"])
    candidate = art.source.source_candidate(selected["candidate"], document)
    # A cross-heading alias or coherently rehashed transform cannot replace the
    # actual reviewed original; each fixture binds both ancestry and registration.
    if (candidate["key"] != expected["candidate"] or candidate["generation_receipt"] != expected["generation_receipt"]
            or {name: candidate[name] for name in ("path", "sha256")} != expected["original"]
            or direction not in candidate["native_transforms"]
            or art.source.registration_signature(candidate, direction) != expected["registration_sha256"]):
        raise ValueError("selected rocket heading ancestry/registration changed")
    if direction != "NE":
        physical = model["views"][direction]
        # The original front/right/rear landmark is physical model identity,
        # never permission to relabel a copied NE or a rejected draft.
        if (candidate["status"] != "selected_direction" or expected["direction"] != direction
                or candidate["physical_view"] != dict(direction=direction, **physical)
                or expected["yaw_from_ne"] != physical["yaw_from_ne"]
                or expected["service_face"] != physical["service_face"]):
            raise ValueError("cargo rocket genuine physical yaw changed")
    pixels, evidence = art.candidate_view(candidate["key"], direction, document)
    # Exact indexed values, alpha footprint and native bounds remain frozen
    # after visual review even when a changed source looks plausible at scale.
    if (hashlib.sha256(pixels.tobytes()).hexdigest() != expected["native_pixels_sha256"]
            or evidence["native_subject_bounds"] != expected["native_subject_bounds"]
            or sum(value != 0 for value in pixels.get_flattened_data()) != expected["nontransparent_pixels"]):
        raise ValueError("cargo rocket selected native heading pixels changed")
    evidence.update(fixture=reference, native_pixels_sha256=expected["native_pixels_sha256"])
    return pixels, evidence


def rebuild():
    """Rebuild the selected subset contact in actual engine order and declare omissions.

    @return Source-only evidence; public manifest/native/runtime remain untouched.
    """
    document = art.source.read_record()
    model = art.read_json(MODEL)
    checked_model(model, document)
    art.checked_predecessor(document, art.read_json(art.PUBLIC))
    headings = [name for name in art.DIRECTIONS if name in model["selected"]]
    atlas = Image.new("P", (64*len(headings), 48), 0)
    atlas.putpalette(art.source.base.PALETTE_BYTES)
    contact = Image.new("RGB", (512*len(headings), 440), "#29323c")
    draw = ImageDraw.Draw(contact)
    evidence = []
    # Native order exposes neighboring physical yaw changes; every column is
    # the exact same frozen pixels at native size and nearest-neighbor x8.
    for index, direction in enumerate(headings):
        pixels, original = direction_view(direction, document, model)
        atlas.paste(pixels, (index*64, 0))
        draw.text((index*512+8, 8), direction+" READY / fixed32,32 / SOURCE ONLY", fill="white")
        contact.paste(art.opaque_rgb(pixels), (index*512+16, 24))
        contact.paste(art.opaque_rgb(pixels).resize((512, 384), Image.Resampling.NEAREST), (index*512, 56))
        evidence.append(original)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    atlas.save(OUTPUT / "selected-ready-native.png", transparency=0)
    contact.save(OUTPUT / "selected-ready-contact.png")
    columns = min(4, len(headings))
    grid = Image.new("RGB", (512*columns, 440*((len(headings)+columns-1)//columns)), "#29323c")
    # Four columns keep all eight native/x8 review panels readable without a
    # viewer shrinking a very wide strip. These are identical diagnostic pixels.
    for index in range(len(headings)):
        grid.paste(contact.crop((index*512, 0, (index+1)*512, 440)), ((index % columns)*512, (index//columns)*440))
    grid.save(OUTPUT / "selected-ready-grid.png")
    report = dict(_documentation=__doc__, direction_model=art.source.identity(MODEL),
                  helper=art.source.identity(ROOT / "media/multilayer/tools/cargo_rocket_directions.py"),
                  headings=headings, missing_headings=[name for name in art.DIRECTIONS if name not in headings],
                  views=evidence, genuine_ready_views_complete=model["genuine_ready_views_complete"],
                  source_coverage=False, packed=False, runtime=False,
                  artifacts=[art.source.identity(OUTPUT / name) for name in ("selected-ready-native.png", "selected-ready-contact.png", "selected-ready-grid.png")])
    (OUTPUT / "source-review.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return report


if __name__ == "__main__":
    rebuild()
