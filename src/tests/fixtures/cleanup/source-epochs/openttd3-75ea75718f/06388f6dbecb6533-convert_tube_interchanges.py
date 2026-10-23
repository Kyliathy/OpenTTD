"""Reuse actual native tube platform artwork at a same-layer joined interchange.

@file convert_tube_interchanges.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Rail/road partners retain their own cells/art; the canonical station owns cargo.
"""

import argparse
import hashlib
import json

import convert_tube_terminals as terminals


tube = terminals.tube
ASSET = "facility.tube-interchange"
TARGET = tube.REVIEW / "tube-interchange-atlas-manifest.json"
GEOMETRY = ("directions", "states", "frames", "parts", "native_size", "anchor",
            "footprint", "tile_variants", "variant_semantics", "native_graphics")
IDENTITY = {"source_mode": "retained_native_platform_reuse",
            "reuse_record": tube.RECORD.relative_to(tube.ROOT).as_posix(),
            "reuse_family": "facility.tube-terminal",
            "station_identity": "canonical_same_layer_station_id",
            "partner_families": ["facility.rail-station", "facility.road-terminal"],
            "physical_geometry": "separate_native_transport_cells",
            "inventory_owner": "native_station_goods", "runtime_activated": False}


def family_spec(document):
    """Resolve the exact compatible platform reuse and native ownership contract.

    @param document Retained source record, optionally isolated for refusal checks.
    @return Guarded interchange source specification; no new facility is created.
    """
    matches = [row for row in document["families"] if row["asset_id"] == ASSET]
    if len(matches) != 1:
        raise ValueError("missing or duplicate native interchange source")
    spec = matches[0]
    parent = terminals.family_spec(document)
    # An interchange is a joined arrangement of existing transport cells.
    # Another footprint, mixed track or private stock owner cannot inherit
    # the genuine platform originals merely by copying a source filename.
    if any(spec.get(key) != value for key, value in IDENTITY.items()) or any(spec.get(key) != parent[key] for key in GEOMETRY):
        raise ValueError("native interchange reuse geometry/ownership mismatch")
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == ASSET)
    if any(family[key] != spec[key] for key in GEOMETRY):
        raise ValueError("native interchange public geometry mismatch")
    return spec


def build(document=None):
    """Retain all256 original platform model/status/part poses without editing pixels.

    @param document Optional isolated original provenance and explicit reuse contract.
    @return Interchange source report and exact canonical platform PNG bytes.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    family_spec(document)
    report, encoded = terminals.build(document)
    report.update(_documentation="Plan13 Group48D/master10.4; archi-map-layers.md. Native joined-station tube-cell source reuse; rail/road partner art has its own families. Operating/packing/runtime pending.",
                  asset_id=ASSET, path=(tube.REVIEW / "tube-interchange-authored.png").relative_to(tube.ROOT).as_posix(),
                  source_reuse=IDENTITY.copy())
    return report, encoded


def validate(report, encoded, document=None):
    """Reject false reuse, missing poses, repainted pixels and invented runtime authority.

    @param report Interchange source mapping over actual native tube platform roles.
    @param encoded Exact indexed platform atlas bytes, never a painted transfer campus.
    @param document Optional isolated original source record for malformed-input checks.
    @return None; source acceptance grants no loading, cargo transfer or power.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    family_spec(document)
    # Explicit reuse must preserve family identity and honest source-only
    # status. Shared native cargo ownership does not establish operating transfer.
    if (report.get("asset_id") != ASSET or report.get("source_reuse") != IDENTITY
            or report.get("source_coverage_complete") is not True or report.get("runtime_activated") is not False
            or report.get("path") != (tube.REVIEW / "tube-interchange-authored.png").relative_to(tube.ROOT).as_posix()):
        raise ValueError("native interchange false source/runtime handoff")
    terminals.validate(report, encoded, document)


def publish(check=False):
    """Rebuild the complete retained source/contact proof or compare it read-only.

    @param check True to refuse stale proof without changing any repository file.
    @return Interchange source manifest path; packed assets remain unchanged.
    """
    report, encoded = build()
    validate(report, encoded)
    files = {tube.base.repository_path(report["path"]): encoded}
    report.update(sha256=hashlib.sha256(encoded).hexdigest(), contact_pages=[])
    # Both real axes retain all four native cell roles/states/indicator phases.
    # Their exact existing contact proof is reused with the unchanged model.
    for axis, pixels in terminals.contact_pages(report, encoded).items():
        path = tube.REVIEW / f"tube-interchange-{axis}-contact.png"
        files[path] = pixels
        report["contact_pages"].append({"axis": axis, "path": path.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(pixels).hexdigest()})
    text = json.dumps(report, indent=2) + "\n"
    if check:
        if any(path.read_bytes() != pixels for path, pixels in files.items()) or TARGET.read_text(encoding="utf-8") != text:
            # All retained pixels, provenance and contacts must regenerate
            # together before packing can consume the source alias.
            raise ValueError("native interchange proof needs deterministic regeneration")
    else:
        tube.REVIEW.mkdir(parents=True, exist_ok=True)
        for path, pixels in files.items():
            path.write_bytes(pixels)
        TARGET.write_text(text, encoding="utf-8")
    return TARGET


def bind_source():
    """Publish only complete interchange source metadata, preserving packed identity.

    @return None; native simulation and runtime activation remain later tasks.
    """
    report = json.loads(publish(check=True).read_text(encoding="utf-8"))
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == ASSET)
    parent = next(row for row in public["assets"] if row["asset_id"] == "facility.tube-terminal")
    family["source"] = parent["source"].copy()
    family["authoring"] = {"date": "2026-10-10", "status": "complete_native_joined_platform_reuse_source_runtime_pending",
                           "source_record": report["source_record"], "originals": parent["authoring"]["originals"],
                           "atlas_manifest": TARGET.relative_to(tube.ROOT).as_posix(), "native_atlas": report["path"],
                           "native_sha256": report["sha256"], "staged_pose_count": 256,
                           "coverage_complete": True, "runtime_activated": False,
                           "reuse_family": "facility.tube-terminal", "partner_families": IDENTITY["partner_families"]}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Build or verify native interchange source, with explicit source publication.

    @return None; print the proof path and retain normal refusal diagnostics.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--check", action="store_true", help="Read-only exact source regeneration")
    modes.add_argument("--publish-source", action="store_true", help="Bind complete source coverage without packing")
    args = parser.parse_args()
    print(publish(args.check))
    if args.publish_source:
        bind_source()


if __name__ == "__main__":
    main()
