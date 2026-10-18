"""Register fixed Assembly hardware while native jobs own all manufacture work and products.

@file convert_space_assembly_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13I
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group74.1
One empty gantry is permanent hardware; a paid product remains its own native hull.
The passive construction coupling grants no stock, service or utility authority.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw

import convert_space_art as art
from tools import orbital_site_art as site
from tools.orbital_art_contract import validate_orbital_contract

ASSET = "module.workshop"
PUBLIC = art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
CANVAS = (192, 160)
FLOOR_ORIGIN = (96, 48)
FLOOR = ((96, 48), (160, 80), (96, 112), (32, 80))
CELLS = tuple((x, y) for y in range(2) for x in range(2))
ORIGINS = tuple((65 + 32 * (y - x), 15 + 16 * (x + y)) for x, y in CELLS)
APPROACH = (160, 96)
CONTACT = (144, 88)
SHARED_COUPLING = "orbital-dock-clamp-v1"
WORK_POLICY = "One actual Assembly manufacturing slot uses its own explicitly connected750J AssemblyWork consumer and exclusive job inputs from an adjacent owned Core/Storage. No general stock, service, producer, free hull, duplicated job, Core construction consumer or art-local energy clock."


def geometry():
    """Expose the actual fixed Assembly floor and original sorting-cell canvases.

    @return Detached native source geometry; this creates no gameplay layout.
    """
    return dict(asset_id=ASSET, native_canvas=list(CANVAS), native_cell=[64, 64],
                world_cells=[list(point) for point in CELLS],
                native_cell_origins=[list(point) for point in ORIGINS],
                floor_origin=list(FLOOR_ORIGIN), floor_corners=[list(point) for point in FLOOR],
                anchor=[-31, -33], native_sort_contact=[31, 48])


def public_contract(document=None):
    """Require the delivered native Assembly family before reading any raster.

    @param document Optional detached public manifest for focused refusal checks.
    @return Unique fixed2x2 Assembly row with actual cell/state requirements.
    @throws ValueError On missing, duplicate or changed public native geometry.
    """
    document = json.loads(PUBLIC.read_text(encoding="utf-8")) if document is None else document
    matches = [row for row in document["assets"] if row["asset_id"] == ASSET]
    if len(matches) != 1:
        raise ValueError("missing or duplicate Assembly public family")
    row = matches[0]
    validate_orbital_contract(row)
    # Source approval cannot change real world cells, palette, native anchor,
    # physical stage count or this family's fixed saved orientation policy.
    if (row["consumer"] != 11 or row["kind"] != "building" or row["native_size"] != [64, 64]
            or row["anchor"] != [-31, -33] or row["parts"] != ["back", "front"]
            or row["geometry"] != "per_tile" or row["bounds"] != [0, 0, 64, 64]
            or row["states"] != ["blueprint", "partial", "complete", "blocked"] or row["frames"] != 2):
        raise ValueError("Assembly public cell/state contract changed")
    return row


def family_contract(document, public=None):
    """Bind the empty gantry to the original single paid manufacturing slot.

    @param document Exact or detached Space original catalogue.
    @param public Optional actual public native geometry contract.
    @return Assembly family with one original manufacturing slot, zero stock/service and no producer.
    @throws ValueError On invented factory slots, source identities, free work, stock or service.
    """
    public_contract(public)
    family = document["families"][ASSET]
    expected = dict(footprint=[2, 2], public_native_size=[64, 64], public_anchor=[-31, -33],
                    native_canvas=list(CANVAS), directions=["none"], native_approach_slots=1,
                    native_service_slots=0, native_stock_cap_per_label=0, native_power_producer=False,
                    native_producer_count=0, native_panel_rate_w=0, native_work_consumer=True,
                    native_work_consumer_count=1, native_work_capacity_j=750, native_manufacture_slots=1,
                    native_world_cells=[list(point) for point in CELLS], native_approach=list(APPROACH),
                    native_construction_contact=list(CONTACT), work_policy=WORK_POLICY)
    # Permanent gantry hardware cannot mint another job or hull. The original
    # dedicated750J consumer and exclusive packets remain the only paid work owners.
    if any(family.get(key) != value for key, value in expected.items()):
        raise ValueError("Assembly source differs from native geometry or single-manufacture ownership")
    return family


def native_body(state="complete", document=None, public=None):
    """Register a genuine Assembly body at its original fixed world footprint.

    @param state Explicit selected physical construction stage.
    @param document Optional detached original catalogue for refusal tests.
    @param public Optional actual public source contract.
    @return Indexed body and measured original floor/coverage evidence.
    @throws ValueError On foreign role, changed approval or invalid projection.
    """
    document = art.read_record() if document is None else document
    family = family_contract(document, public)
    if state not in family["body_sources"]:
        raise ValueError("missing genuine Assembly body state")
    candidate = art.source_candidate(family["body_sources"][state], document)
    # A retained original for another site or physical stage cannot inherit
    # the Assembly approval merely because its canvas happens to fit.
    if (candidate["asset_id"] != ASSET or candidate["directions"] != ["none"]
            or candidate.get("state") != state or candidate.get("component") != f"fixed_assembly_{state}_body"
            or site.registration_signature(candidate) != family["approved_body_registration"][state]):
        raise ValueError("Assembly original body or stage registration changed")
    return site.floor_original(candidate, geometry())


def construction_component(document):
    """Reuse genuine passive metal at the original Assembly construction contact.

    @param document Catalogue retaining exact earlier original ancestry and new registration.
    @return Indexed coupling-only canvas with explicit hardware-reuse evidence.
    @throws ValueError On foreign role, changed ancestry or hardware outside its original contact.
    """
    family = family_contract(document)
    candidate = art.source_candidate(family["construction_source"], document)
    shared = art.source_candidate(SHARED_COUPLING, document)
    # Reuse is physical metal only. It cannot copy Dock service, its second
    # clamp or another cargo/utility controller into the single paid factory.
    if (candidate["asset_id"] != ASSET or candidate["directions"] != ["none"]
            or candidate.get("state") != "complete" or candidate.get("component") != "fixed_assembly_passive_construction_coupling"
            or candidate.get("reused_original_key") != SHARED_COUPLING
            or any(candidate[key] != shared[key] for key in ("path", "sha256", "image_size"))
            or site.registration_signature(candidate) != family["approved_construction_registration"]):
        raise ValueError("Assembly passive coupling ancestry or registration changed")
    transform = candidate["native_transform"]
    if transform["native_relative_bounds"] != [-8, -6, 0, 0]:
        raise ValueError("Assembly passive coupling mounting envelope changed")
    image, evidence = site.registered_original(candidate, CONTACT, geometry())
    x0, y0, x1, y1 = evidence["native_subject_bounds"]
    cx, cy = CONTACT
    # All original metal must remain inboard before assembly. A tile crop
    # cannot repair an oversized construction contact or obstruct the reserved approach.
    if not (cx - 8 <= x0 < x1 <= cx and cy - 6 <= y0 < y1 <= cy):
        raise ValueError("Assembly passive coupling leaves approved inboard envelope")
    return image, dict(evidence, reused_original_key=SHARED_COUPLING,
                       original_generation_receipt=shared["generation_receipt"], floor_contact=list(CONTACT))


def native_base(document=None, public=None):
    """Assemble an empty gantry and console without creating a product or paid work.

    @param document Optional detached catalogue for focused ownership refusals.
    @param public Optional detached public native contract.
    @return Whole indexed Assembly and original attachment/preservation evidence.
    @throws ValueError On detached coupling or an obstructed actual construction approach.
    """
    document = art.read_record() if document is None else document
    body, body_evidence = native_body("complete", document, public)
    coupling, component_evidence = construction_component(document)
    result = body.copy()
    pixels, overlap = [], 0
    # A single passive construction coupling attaches to the paid floor. Every
    # other original gantry/console pixel stays exact; source art grants no job or hull.
    for y in range(CANVAS[1]):
        for x in range(CANVAS[0]):
            value = coupling.getpixel((x, y))
            if not value:
                continue
            pixels.append([x, y])
            overlap += bool(body.getpixel((x, y)))
            result.putpixel((x, y), value)
    if overlap < 3:
        raise ValueError("Assembly passive coupling lacks a genuine deck attachment")
    if result.getpixel(APPROACH):
        raise ValueError("Assembly artwork obstructs the actual construction approach")
    return result, dict(body=body_evidence,
                        coupling=dict(component_evidence, original_pixels=pixels, body_overlap_pixels=overlap),
                        protected_body_canvas_pixels=CANVAS[0] * CANVAS[1] - len(pixels),
                        native_sha256=hashlib.sha256(result.tobytes()).hexdigest())


def review_base(output):
    """Publish measured Assembly hardware and its passive construction contact.

    @param output Fresh repository native-art proof directory.
    @return Exact base review identity; tile/state/packing/runtime gates remain open.
    @throws ValueError On unsafe proof ownership or invalid original assembly.
    """
    output = output.resolve()
    # Fresh proof directories protect accepted originals, earlier evidence and
    # distributed graphics while the current task establishes only source art.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("Assembly base review requires a fresh native-art proof directory")
    native, evidence = native_base()
    rgba = native.convert("RGBA").resize((768, 640), Image.Resampling.NEAREST)
    contact = Image.new("RGB", rgba.size, (38, 43, 49))
    contact.paste(rgba, mask=rgba.getchannel("A"))
    coupling, source = construction_component(art.read_record())
    crop = coupling.crop((136, 82, 144, 88)).convert("RGBA").resize((256, 192), Image.Resampling.NEAREST)
    detail = Image.new("RGB", (256, 216), (38, 43, 49))
    detail.paste(crop, (0, 24), crop)
    ImageDraw.Draw(detail).text((4, 6), "single passive construction coupling", fill="white")
    output.mkdir(parents=True)
    native.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    coupling.save(output / "coupling.png", transparency=0)
    detail.save(output / "coupling-contact.png")
    path = output / "base-source-review.json"
    report = dict(_documentation=__doc__, asset_id=ASSET, base=evidence,
                  complete_source_coverage=False, packed=False, runtime_activated=False,
                  artifacts=[art.identity(output / name) for name in
                             ("native.png", "contact.png", "coupling.png", "coupling-contact.png")])
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return art.identity(path)


def main():
    """Emit only an explicitly requested original Assembly body or base review."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--body-state", choices=("complete", "blueprint", "partial"))
    parser.add_argument("--review-base", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.review_base:
        print(json.dumps(review_base(args.output)))
        return
    output = args.output.resolve()
    # Direct diagnostics stay in the same proof home and cannot overwrite a
    # previously reviewed original, an existing proof or any shipped payload.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("Assembly diagnostic requires a new native-art proof file")
    image, evidence = native_body(args.body_state) if args.body_state else native_base()
    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output, transparency=0)
    print(json.dumps(evidence))


if __name__ == "__main__":
    main()
