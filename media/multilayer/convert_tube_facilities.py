"""Reuse compatible immutable native service source without duplicating game art.

@file convert_tube_facilities.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native depots own occupancy/service; source art creates no power, repair or new facility.
"""

import argparse
import copy
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_facility_art as facilities
import convert_tube_art as tube


ASSET = "facility.tube-service"
TARGET = tube.REVIEW / "tube-service-atlas-manifest.json"


def sources(document):
    """Resolve one explicit existing compatible model and its retained indicator.

    @param document Ground source record containing the selected service reuse contract.
    @return Isolated converter input; the canonical M1 record remains unchanged.
    """
    matches = [row for row in document["families"] if row["asset_id"] == ASSET]
    if len(matches) != 1:
        raise ValueError("missing or duplicate native tube service source")
    spec = matches[0]
    # This reuse is valid only for the same native one-tile service geometry.
    # Another depot/world footprint or copied source name cannot silently
    # create missing construction cells, view states or electrical ownership.
    if (spec["source_mode"] != "retained_native_facility_reuse" or spec["reuse_family"] != "facility.rail-service"
            or spec["reuse_component"] != "facility-status" or spec["footprint"] != [1, 1]
            or spec["tile_variants"] != 1 or spec["directions"] != ["NE", "SE", "SW", "NW"]
            or spec["states"] != facilities.STATES or spec["frames"] != 4 or spec["parts"] != ["back", "front"]
            or spec["anchor"] != [-31, -33] or spec["native_size"] != [64, 64]):
        raise ValueError("native tube service reuse geometry/state mismatch")
    parent = json.loads(tube.base.repository_path(spec["reuse_record"]).read_text(encoding="utf-8"))
    models = [row for row in parent["families"] if row["asset_id"] == spec["reuse_family"]]
    components = [row for row in parent["components"] if row["key"] == spec["reuse_component"]]
    if len(models) != 1 or len(components) != 1:
        # Only the exact original enclosure/status component supplies these
        # pixels; rejected candidates and other models are never fallback art.
        raise ValueError("missing or duplicate retained native service model/component")
    model = copy.deepcopy(models[0])
    model.update(key="tube-service", asset_id=ASSET)
    return {"families": [model], "components": copy.deepcopy(components), "candidates": []}


def build(document=None):
    """Derive128 native service poses through the existing pure facility converter.

    @param document Optional isolated ground source provenance for refusal checks.
    @return Source report and exact indexed PNG bytes, with all M1 originals unchanged.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    selected = sources(document)
    report, files = facilities.derive(selected, source_record=tube.RECORD, output_dir=tube.REVIEW)
    atlas = report["atlases"][0]
    report.update(_documentation="Plan13 Group48B/master10.4; archi-map-layers.md. Native one-tile tube service source reuse; operating/runtime acceptance pending.",
                  asset_id=ASSET, source_reuse={"record": next(row for row in document["families"] if row["asset_id"] == ASSET)["reuse_record"],
                                              "asset_id": "facility.rail-service", "component": "facility-status"},
                  source_coverage_complete=True)
    return report, files[tube.base.repository_path(atlas["path"])]


def validate(report, encoded, document=None):
    """Reject missing poses, phase duplication, false source identity and changed pixels.

    @param report Complete retained source mapping for one actual native depot cell.
    @param encoded Indexed native atlas bytes to inspect.
    @param document Optional isolated source provenance for malformed-input checks.
    @return None; no operating depot, electrical grant or runtime art claim is made.
    """
    atlas = report["atlases"][0]
    required = {(direction, state, frame, "back" if part == 0 else "front")
                for direction in ("NE", "SE", "SW", "NW") for state in facilities.STATES for frame in range(4) for part in range(2)}
    actual = [(row["direction"], row["state"], row["frame"], row["part"]) for row in atlas["sprites"]]
    if len(actual) != len(required) or set(actual) != required:
        # Count alone cannot hide a missing entrance/state or a duplicate phase.
        raise ValueError("incomplete native service state/view/phase/part coverage")
    with Image.open(io.BytesIO(encoded)) as sheet:
        # All parts retain the selected M1 DOS palette, alpha and native canvas;
        # a shifted six-cell draft cannot inherit one-tile source acceptance.
        if (sheet.mode != "P" or sheet.size != (512, 1024) or sheet.info.get("transparency") != 0
                or sheet.getpalette()[:768] != tube.base.PALETTE_BYTES):
            raise ValueError("native tube service palette/canvas mismatch")
        if set(sheet.get_flattened_data()) & (tube.base.FORBIDDEN - {0}):
            raise ValueError("forbidden native tube service palette index")
        # Every world cell and both aligned parts use the actual native depot
        # footprint; source front parts cannot move independently of the roof.
        for row in atlas["sprites"]:
            x, y, width, height = row["box"]
            if (row["anchor"] != [-31, -33] or row["world_cell"] != [0, 0] or row["tile_variant"] != 0
                    or [width, height] != [64, 64] or x < 0 or y < 0 or x + width > 512 or y + height > 1024):
                raise ValueError("native tube service anchor/world-cell mismatch")
            if sheet.crop((x, y, x + width, y + height)).getbbox() is None:
                raise ValueError("empty native tube service source part")
        # Four genuine retained indicator phases must remain distinct in each
        # authored state/view; repeating one panel cannot satisfy animation.
        for direction in ("NE", "SE", "SW", "NW"):
            for state in facilities.STATES:
                signatures = []
                for frame in range(4):
                    row = next(row for row in atlas["sprites"] if (row["direction"], row["state"], row["frame"], row["part"]) == (direction, state, frame, "back"))
                    x, y, width, height = row["box"]
                    signatures.append(sheet.crop((x, y, x + width, y + height)).tobytes())
                if len(set(signatures)) != 4:
                    raise ValueError("native tube service repeats an authored phase")
    expected, pixels = build(document)
    if atlas != expected["atlases"][0] or report["source_reuse"] != expected["source_reuse"]:
        raise ValueError("native tube service false reuse provenance")
    if encoded != pixels:
        raise ValueError("native tube service changes retained source pixels")


def contact_board(report, encoded):
    """Present every actual depot view/state and all four retained lamp phases.

    @param report Complete128-pose source mapping for the one-tile service enclosure.
    @param encoded Exact unchanged native source atlas pixels.
    @return PNG review board showing native and enlarged back/front parts.
    """
    board = Image.new("RGB", (2304, 1280), (48, 52, 58))
    labels = ImageDraw.Draw(board)
    labels.text((12, 12), "ONE-TILE TUBE SERVICE: reused immutable native enclosure/status phases; source only; operating/runtime acceptance pending", fill="white")
    with Image.open(io.BytesIO(encoded)) as atlas:
        # Disjoint state/view panels keep actual far opaque walls and near
        # entrance apertures visible without a speculative multi-cell plaza.
        for row_index, state in enumerate(facilities.STATES):
            for column, direction in enumerate(("NE", "SE", "SW", "NW")):
                left, top = column * 576, row_index * 308 + 40
                labels.text((left + 12, top), state + " / " + direction + ": back3x; front3x; all four phases1x", fill="white")
                for part_index, part in enumerate(("back", "front")):
                    row = next(s for s in report["atlases"][0]["sprites"] if (s["direction"], s["state"], s["frame"], s["part"]) == (direction, state, 0, part))
                    x, y, width, height = row["box"]
                    image = atlas.crop((x, y, x + width, y + height)).convert("RGBA").resize((192, 192), Image.Resampling.NEAREST)
                    board.paste(image, (left + 12 + part_index * 220, top + 24), image)
                for frame in range(4):
                    row = next(s for s in report["atlases"][0]["sprites"] if (s["direction"], s["state"], s["frame"], s["part"]) == (direction, state, frame, "back"))
                    x, y, width, height = row["box"]
                    image = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
                    board.paste(image, (left + 12 + frame * 80, top + 220), image)
    stream = io.BytesIO()
    board.save(stream, format="PNG")
    return stream.getvalue()


def publish(check=False):
    """Write complete service source proof or compare its exact bytes read-only.

    @param check True to verify exact atlas/contact/provenance regeneration.
    @return Bound source report path; packed assets and original M1 proof are untouched.
    """
    report, encoded = build()
    validate(report, encoded)
    contact = contact_board(report, encoded)
    contact_path = tube.REVIEW / "tube-service-contact.png"
    report.update(contact=contact_path.relative_to(tube.ROOT).as_posix(), contact_sha256=hashlib.sha256(contact).hexdigest())
    text = json.dumps(report, indent=2) + "\n"
    atlas = tube.base.repository_path(report["atlases"][0]["path"])
    if check:
        # Reuse cannot inherit old acceptance after source, phase or anchor
        # changes; exact provenance/contact pixels must regenerate together.
        if atlas.read_bytes() != encoded or contact_path.read_bytes() != contact or TARGET.read_text(encoding="utf-8") != text:
            raise ValueError("native tube service proof needs deterministic regeneration")
    else:
        tube.REVIEW.mkdir(parents=True, exist_ok=True)
        atlas.write_bytes(encoded)
        contact_path.write_bytes(contact)
        TARGET.write_text(text, encoding="utf-8")
    return TARGET


def bind_source():
    """Publish complete compatible service source without changing packed metadata.

    @return None; only the new tube service family's source/authoring fields change.
    """
    report = json.loads(publish(check=True).read_text(encoding="utf-8"))
    atlas = report["atlases"][0]
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == ASSET)
    family["source"] = {"status": "authored", "path": atlas["source"], "sha256": atlas["source_sha256"],
                        "provenance": "first_party_retained_compatible_model_reuse", "rights": "first_party_generated_original"}
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_one_tile_service_source_runtime_pending",
                           "source_record": report["source_record"], "source_reuse": report["source_reuse"],
                           "atlas_manifest": TARGET.relative_to(tube.ROOT).as_posix(), "native_atlas": atlas["path"],
                           "native_sha256": atlas["sha256"], "staged_pose_count": 128,
                           "coverage_complete": True, "runtime_activated": False}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Build/check native service source with an explicit publication mode.

    @return None; print the complete source report and preserve refusal diagnostics.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Read-only exact source regeneration")
    parser.add_argument("--publish-source", action="store_true", help="Bind checked complete source without packing")
    args = parser.parse_args()
    if args.check and args.publish_source:
        # Read-only verification must never change the public source contract.
        parser.error("--check cannot be combined with --publish-source")
    print(publish(args.check))
    if args.publish_source:
        bind_source()


if __name__ == "__main__":
    main()
