"""Register shared processor hardware while original recipes own stock and work.

@file convert_space_processor_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13J
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group75.1
One fixed3x3 model serves four native kinds. Source metadata is a disclosure
contract, never a second recipe executor, inventory, power account or clock.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

import convert_space_art as art
from tools import orbital_site_art as site
from tools.orbital_art_contract import validate_orbital_contract

ASSET = "module.processor"
PUBLIC = art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
CANVAS = (256, 192)
FLOOR_ORIGIN = (128, 64)
FLOOR = ((128, 64), (224, 112), (128, 160), (32, 112))
CELLS = tuple((x, y) for y in range(3) for x in range(3))
ORIGINS = tuple((97 + 32 * (y - x), 31 + 16 * (x + y)) for x, y in CELLS)
APPROACH = (224, 128)
CONTACT = (208, 120)
KINDS = ["Steelworks", "Goodsworks", "Refinery", "IceProcessor"]
SHARED_COUPLING = "orbital-dock-clamp-v1"
WORK_POLICY = "Original OrbitalModule::processing and shared recipe executors alone earn admitted native opportunities. OSTL/OGDS each spend55500J over74opportunities through their own processing.consumer; OREF/PICE earn74unpowered ticks with no electrical consumer. Saved actual choice, original packets, current last_native_tick and real work/batch receipts own activity; disabled is idle and GUI time grants nothing."
# These checked source disclosures describe existing content. Native execution
# continues to read GetOrbitalRecipeDefinition and the original saved choice.
RECIPES = {
    "Steelworks": dict(default="OSTL", alternatives=["OSTL"], inputs={"NORE": 4}, outputs={"STEL": 2},
                      work_ticks=74, batch_j=55500, grant_j=750, labels=["NORE", "STEL"], residue_sixteenths=32),
    "Goodsworks": dict(default="OGDS", alternatives=["OGDS"], inputs={"STEL": 2, "RARE": 1}, outputs={"GOOD": 4},
                      work_ticks=74, batch_j=55500, grant_j=750, labels=["STEL", "RARE", "GOOD"], residue_sixteenths=16),
    "Refinery": dict(default="OREF", alternatives=["OREF", "PICE"], inputs={"NORE": 4}, outputs={"RFMT": 2},
                    work_ticks=74, batch_j=0, grant_j=0, labels=["NORE", "RFMT", "WICE", "PROP"], residue_sixteenths=0),
    "IceProcessor": dict(default="PICE", alternatives=["OREF", "PICE"], inputs={"WICE": 2}, outputs={"PROP": 1},
                        work_ticks=74, batch_j=0, grant_j=0, labels=["NORE", "RFMT", "WICE", "PROP"], residue_sixteenths=0),
}


def geometry():
    """Expose the original fixed processor floor and nine native sorting canvases.

    @return Detached geometry; no gameplay layout or rotation is created.
    """
    return dict(asset_id=ASSET, native_canvas=list(CANVAS), native_cell=[64, 64],
                world_cells=[list(point) for point in CELLS],
                native_cell_origins=[list(point) for point in ORIGINS],
                floor_origin=list(FLOOR_ORIGIN), floor_corners=[list(point) for point in FLOOR],
                anchor=[-31, -33], native_sort_contact=[31, 48])


def public_contract(document=None):
    """Require the shared native four-kind processor contract before source reads.

    @param document Optional detached manifest for focused refusal fixtures.
    @return Unique fixed3x3 row preserving native cells, parts, states and identity.
    @throws ValueError On missing, duplicate or changed native geometry/ownership.
    """
    document = json.loads(PUBLIC.read_text(encoding="utf-8")) if document is None else document
    matches = [row for row in document["assets"] if row["asset_id"] == ASSET]
    if len(matches) != 1:
        raise ValueError("missing or duplicate processor public family")
    row = matches[0]
    validate_orbital_contract(row)
    expected = dict(consumer=11, kind="building", native_size=[64, 64], footprint=[3, 3],
                    anchor=[-31, -33], parts=["back", "front"], geometry="per_tile", bounds=[0, 0, 64, 64],
                    states=["blueprint", "partial", "complete", "blocked"], frames=2,
                    directions=["none"], tile_variants=9, native_build_kinds=KINDS)
    if any(row.get(key) != value for key, value in expected.items()):
        raise ValueError("processor public geometry or four-kind mapping changed")
    return row


def family_contract(document, public=None):
    """Protect actual family-specific packet/work ownership behind the shared model.

    @param document Exact or detached Space source catalogue.
    @param public Optional detached public source contract for refusals.
    @return Processor source family with explicit bounded recipe disclosures.
    @throws ValueError On invented stock, utility, service, choices or native geometry.
    """
    public_contract(public)
    family = document["families"][ASSET]
    expected = dict(footprint=[3, 3], public_native_size=[64, 64], public_anchor=[-31, -33],
                    native_canvas=list(CANVAS), directions=["none"], native_build_kinds=KINDS,
                    native_approach_slots=1, native_service_slots=0, native_power_producer=False,
                    native_producer_count=0, native_stock_policy="actual_recipe_family_label_union_1024; foreign_labels_zero",
                    native_recipe_disclosures=RECIPES, native_manufacture_slots=0,
                    native_world_cells=[list(point) for point in CELLS], native_approach=list(APPROACH),
                    native_construction_contact=list(CONTACT), work_policy=WORK_POLICY)
    if any(family.get(key) != value for key, value in expected.items()):
        raise ValueError("processor source differs from native recipe, geometry or work ownership")
    return family


def native_body(state="complete", document=None, public=None):
    """Register one genuine processor construction stage on its original nine cells.

    @param state Explicit selected physical body state.
    @param document Optional detached original catalogue for refusal checks.
    @param public Optional detached actual public geometry contract.
    @return Indexed body and independent source-floor/coverage evidence.
    @throws ValueError On foreign stage/role, changed approval or invalid projection.
    """
    document = art.read_record() if document is None else document
    family = family_contract(document, public)
    if state not in family["body_sources"]:
        raise ValueError("missing genuine processor body state")
    candidate = art.source_candidate(family["body_sources"][state], document)
    # A shared graphics row does not permit another facility's hardware or a
    # different construction stage to inherit the approved source registration.
    if (candidate["asset_id"] != ASSET or candidate["directions"] != ["none"]
            or candidate.get("state") != state or candidate.get("component") != f"fixed_processor_{state}_body"
            or site.registration_signature(candidate) != family["approved_body_registration"][state]):
        raise ValueError("processor original body or stage registration changed")
    return site.floor_original(candidate, geometry())


def construction_component(document):
    """Place unchanged passive metal at the original processor handling approach.

    @param document Catalogue preserving original Dock metal and explicit new reuse.
    @return Indexed whole coupling and immutable ancestry/contact evidence.
    @throws ValueError On foreign hardware, changed registration or an enlarged contact.
    """
    family = family_contract(document)
    candidate = art.source_candidate(family["construction_source"], document)
    shared = art.source_candidate(SHARED_COUPLING, document)
    # Only physical metal is reused. Dock service accounts, a second clamp and
    # generic warehouse capacity cannot follow it into this processing site.
    if (candidate["asset_id"] != ASSET or candidate["directions"] != ["none"]
            or candidate.get("state") != "complete" or candidate.get("component") != "fixed_processor_passive_construction_coupling"
            or candidate.get("reused_original_key") != SHARED_COUPLING
            or any(candidate[key] != shared[key] for key in ("path", "sha256", "image_size"))
            or site.registration_signature(candidate) != family["approved_construction_registration"]):
        raise ValueError("processor passive coupling ancestry or registration changed")
    if candidate["native_transform"]["native_relative_bounds"] != [-8, -6, 0, 0]:
        raise ValueError("processor passive coupling envelope changed")
    image, evidence = site.registered_original(candidate, CONTACT, geometry())
    x0, y0, x1, y1 = evidence["native_subject_bounds"]
    cx, cy = CONTACT
    # The entire real component stays inboard; later tile cutting cannot hide
    # overflow or turn the reserved approach into another occupied world cell.
    if not (cx - 8 <= x0 < x1 <= cx and cy - 6 <= y0 < y1 <= cy):
        raise ValueError("processor passive coupling leaves approved inboard envelope")
    return image, dict(evidence, reused_original_key=SHARED_COUPLING,
                       original_generation_receipt=shared["generation_receipt"], floor_contact=list(CONTACT))


def native_base(document=None, public=None):
    """Attach the one passive coupling without changing any other machine pixel.

    @param document Optional exact or detached source catalogue.
    @param public Optional detached public geometry contract.
    @return Indexed complete machine and original pixel-preservation evidence.
    @throws ValueError On missing floor contact or an obstructed real approach.
    """
    document = art.read_record() if document is None else document
    body, body_evidence = native_body("complete", document, public)
    coupling, component_evidence = construction_component(document)
    result = body.copy()
    pixels, overlap = [], 0
    # Attach only pixels from the unchanged original component. Every unrelated
    # machine/deck pixel remains exact and no source pixel creates stock or work.
    for y in range(CANVAS[1]):
        for x in range(CANVAS[0]):
            value = coupling.getpixel((x, y))
            if not value:
                continue
            pixels.append([x, y])
            overlap += bool(body.getpixel((x, y)))
            result.putpixel((x, y), value)
    if overlap < 3:
        raise ValueError("processor passive coupling lacks a genuine deck attachment")
    if result.getpixel(APPROACH):
        raise ValueError("processor artwork obstructs the actual construction approach")
    return result, dict(body=body_evidence,
                        coupling=dict(component_evidence, original_pixels=pixels, body_overlap_pixels=overlap),
                        protected_body_canvas_pixels=CANVAS[0] * CANVAS[1] - len(pixels),
                        native_sha256=hashlib.sha256(result.tobytes()).hexdigest())


def review_base(output):
    """Publish the measured base and native-scale contact before later state cuts.

    @param output Fresh repository-owned native-art proof directory.
    @return Base review identity, retaining missing stages/packing/runtime as pending.
    @throws ValueError On unsafe proof ownership or invalid original registration.
    """
    output = output.resolve()
    # Fresh evidence protects earlier accepted sources and shipped graphics;
    # this source task cannot overwrite a previous proof or distributed payload.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("processor base review requires a fresh native-art proof directory")
    native, evidence = native_base()
    rgba = native.convert("RGBA").resize((1024, 768), Image.Resampling.NEAREST)
    contact = Image.new("RGB", rgba.size, (38, 43, 49))
    contact.paste(rgba, mask=rgba.getchannel("A"))
    coupling, _ = construction_component(art.read_record())
    output.mkdir(parents=True)
    native.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    coupling.save(output / "coupling.png", transparency=0)
    path = output / "base-source-review.json"
    report = dict(_documentation=__doc__, asset_id=ASSET, base=evidence,
                  complete_source_coverage=False, packed=False, runtime_activated=False,
                  artifacts=[art.identity(output / name) for name in ("native.png", "contact.png", "coupling.png")])
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return art.identity(path)


def main():
    """Emit only an explicitly requested processor body or bounded base review."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--body-state", choices=("complete", "blueprint", "partial"))
    parser.add_argument("--review-base", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.review_base:
        print(json.dumps(review_base(args.output)))
        return
    output = args.output.resolve()
    # Diagnostics use fresh repository proof files, preserving original bytes
    # and all previously accepted evidence even when a source check fails.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("processor diagnostic requires a new native-art proof file")
    image, evidence = native_body(args.body_state) if args.body_state else native_base()
    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output, transparency=0)
    print(json.dumps(evidence))


if __name__ == "__main__":
    main()
