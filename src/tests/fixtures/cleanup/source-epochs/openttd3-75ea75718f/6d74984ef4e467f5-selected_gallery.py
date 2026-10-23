"""Adapt sealed selected native receipts for the existing M1 gallery owner.

@file selected_gallery.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84C-84D.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84ED; successive delivery snapshots preserve protected contracts.
Sprite identities/ranges belong exclusively to the public contract and pack scope.
This adapter preserves dated native acceptance without requiring disposable PPMs.
"""

import copy
import json
from pathlib import Path

import activate_m1_art as activation

ROOT = activation.ROOT
ROLES = ("selected-purchase", "selected-structures", "selected-movement", "selected-geothermal")
PUBLIC = activation.PUBLIC.relative_to(ROOT).as_posix()
# Publication changes delivery metadata only. Everything else, including source,
# geometry, packing and coverage, remains byte-equivalent as parsed JSON.
PACKED_DELIVERY_FIELDS = ("_documentation", "runtime_activated", "runtime_asset_ids", "runtime_binding_manifest", "gallery_manifest")


def read(path):
    """Read an explicit immutable JSON artifact.

    @param path Existing artifact path, never a latest-run search.
    @return Parsed JSON value.
    """
    return json.loads(Path(path).read_text(encoding="utf-8"))


def checked(reference, cache):
    """Verify one retained artifact once per gallery validation.

    @param reference Exact repository-relative path/hash and optional byte count.
    @param cache Per-validation identities already checked; conflicting records refuse.
    @return Existing contained path after exact identity verification.
    """
    key = reference["path"]
    if key not in cache:
        path = (ROOT / key).resolve()
        assert path.is_relative_to(ROOT), "gallery reference escapes repository"
        cache[key] = activation.evidence_identity(path)
    actual = cache[key]
    for field in ("path", "sha256", "bytes"):
        if field in reference:
            assert actual[field] == reference[field], "stale selected gallery artifact: " + key
    return ROOT / key


def references(value):
    """Walk nested sealed receipts without interpreting private native state.

    @param value JSON receipt, sequence or scalar.
    @return Iterator of exact path/hash records embedded by owning producers.
    """
    if isinstance(value, dict):
        # Identity records are leaves; other dictionaries describe native proof.
        if {"path", "sha256"}.issubset(value):
            yield value
        else:
            for child in value.values():
                yield from references(child)
    elif isinstance(value, list):
        # Lists retain the producer's original ordering of native states/views.
        for child in value:
            yield from references(child)


def protected_public(public):
    """Remove only explicitly mutable family delivery fields.

    @param public Sole public sprite contract before or after selected publication.
    @return Deep copy preserving every contract/source/geometry/coverage field.
    """
    result = copy.deepcopy(public)
    selected = set(read(activation.pack.SCOPE)["runtime_asset_ids"])
    for family in result["assets"]:  # Delivery cannot rewrite authored pixels, source approval, pose coverage or gameplay geometry.
        if family["asset_id"] not in selected:
            continue
        family.pop("runtime", None)
        family.get("authoring", {}).pop("runtime_activated", None)
        family.get("authoring", {}).pop("status", None)
        family.get("source", {}).pop("runtime_activated", None)
    return result


def protected_packed(packed):
    """Remove the packing owner's documented runtime-publication fields.

    @param packed Actual complete ninety-family packing report.
    @return Deep copy retaining all ranges, poses, source hashes and payload data.
    """
    result = copy.deepcopy(packed)
    for field in PACKED_DELIVERY_FIELDS:
        result.pop(field, None)
    return result


def prepare(selection, receipts, output):
    """Create a named candidate using the same EvidenceSet and immutable B2 handoff.

    @param selection Existing explicit sixty-nine-family binding evidence selection.
    @param receipts Exact purchase, structure, movement and geothermal closure paths, in that order.
    @param output Absent named evidence manifest beneath the native-art proof cache.
    @return New selection path; neither the base selection nor public metadata changes.
    """
    output = (ROOT / output).resolve()
    assert output.is_relative_to(activation.CACHE) and not output.exists(), "selected evidence destination must be absent in proof cache"
    assert len(receipts) == len(ROLES)
    document = copy.deepcopy(selection.document)
    assert "selected_native_producers" in document, "selected callback review is required first"
    if "selected_gallery_boundary" in document:
        # A later guidance-only review may retain earlier native observations.
        # Verify the entire existing delivery lineage before freezing another
        # snapshot; a changed source/range cannot acquire an alias by republishing.
        validate_boundary(read(activation.PUBLIC), read(selection.output("packed")), selection, {})
    document["base_evidence"] = activation.evidence_identity(selection.path)
    for role, name in zip(ROLES, receipts):  # Each completed stage has a distinct proof obligation; one receipt cannot stand in for a missing stage.
        document["reports"][role] = activation.evidence_identity((ROOT / name).resolve())
    output.parent.mkdir(parents=True, exist_ok=True)
    snapshots = {}
    for role, path in (("public", activation.PUBLIC), ("packed", selection.output("packed"))):
        target = output.parent / (role + "-before-delivery.json")
        assert not target.exists(), "never replace a frozen delivery boundary"
        target.write_bytes(path.read_bytes())
        snapshots[role] = activation.evidence_identity(target)
    document["selected_gallery_boundary"] = {**snapshots, "scope": activation.evidence_identity(activation.pack.SCOPE)}
    if "selected_gallery_boundary" in selection.document:
        document["selected_gallery_boundary"]["previous_selection"] = document["base_evidence"]
    document["outputs"]["runtime"] = (output.parent / "runtime-manifest.json").relative_to(ROOT).as_posix()
    document["outputs"]["gallery"] = (output.parent / "gallery/m1-runtime-gallery.json").relative_to(ROOT).as_posix()
    document["_documentation"] = "r2mlvs14-remaining-assets-and-runtime-integration.md84C-84D; selected native gallery,retained dated43-family prefix,immutable prepublication boundaries;human approval deferred"
    output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return output


def delivery_boundaries(selection, cache):
    """Walk only exact named predecessor selections of this delivery lineage.

    @param selection Current explicit EvidenceSet, including an unpublished candidate.
    @param cache Per-validation checked identities of immutable predecessor files.
    @return Iterator of current and ancestral frozen boundary records; cycles refuse.
    """
    seen = set()
    current = selection
    # Each prior publication can explain a retained native manifest identity.
    # Follow only the hash-bound base selection, never arbitrary snapshots or
    # the mutable default manifest, and refuse cycles rather than widening scope.
    while True:
        assert current.path not in seen, "cyclic selected delivery lineage"
        seen.add(current.path)
        boundary = current.document["selected_gallery_boundary"]
        yield boundary
        previous = boundary.get("previous_selection")
        if previous is None:
            return
        assert previous == current.document["base_evidence"], "delivery predecessor differs from selected base evidence"
        path = checked(previous, cache).resolve()
        assert path != activation.DEFAULT_MANIFEST.resolve(), "delivery predecessor must be an immutable named selection"
        current = activation.EvidenceSet(path)


def validate_boundary(public, packed, selection, cache):
    """Refuse changed protected source/range fields across delivery publication.

    @param public Candidate current public sprite contract.
    @param packed Candidate current packing report.
    @param selection Named EvidenceSet with exact prepublication snapshots.
    @param cache Per-validation checked identity map.
    @return Current frozen public/packing documents after every named predecessor also passes.
    """
    protected_contract, protected_payload = protected_public(public), protected_packed(packed)
    current_frozen = None
    # Delivery metadata can legitimately differ between old and fresh native
    # captures. Every admitted snapshot must still preserve ALL source, geometry,
    # range, coverage and payload fields, including ancestors no longer published.
    for boundary in delivery_boundaries(selection, cache):
        frozen_public = read(checked(boundary["public"], cache))
        frozen_packed = read(checked(boundary["packed"], cache))
        checked(boundary["scope"], cache)
        assert protected_contract == protected_public(frozen_public), "protected public contract changed after native gallery evidence"
        assert protected_payload == protected_packed(frozen_packed), "protected packing contract changed after native gallery evidence"
        if current_frozen is None:
            current_frozen = frozen_public, frozen_packed
    return current_frozen


def validate_retained(proof, public_snapshots, packed_snapshots, cache):
    """Check retained pixels, original saves and producer sources, excluding disposable containers.

    @param proof Completed exact outer receipt from a selected native producer.
    @param public_snapshots Exact public snapshot records already verified by validate_boundary.
    @param packed_snapshots Exact packing snapshot records already verified by validate_boundary.
    @param cache Per-validation checked identities; duplicate references cost no extra I/O.
    @return None; native PPM/runtime copies and dated binaries are not required after sealing.
    """
    aliases = {PUBLIC: public_snapshots, activation.PACKED.relative_to(ROOT).as_posix(): packed_snapshots}
    for reference in references(proof):  # Sealed receipts own native execution/pixel projection. Retained original PNG/SAV/log/source bytes must still be exact; raw PPMs and rebuilt binaries remain disposable provenance.
        name = reference["path"]
        if name in aliases:
            matches = [snapshot for snapshot in aliases[name] if reference["sha256"] == snapshot["sha256"]]
            assert matches, "native proof names a different prepublication contract"
            if "bytes" in reference:
                assert any(snapshot["bytes"] == reference["bytes"] for snapshot in matches), "native proof contract byte count differs"
        elif name.startswith(("src/", "media/")):
            checked(reference, cache)
        elif Path(name).suffix.lower() in (".png", ".sav", ".log"):
            checked(reference, cache)


def bind(mapped, asset, frames, context, public):
    """Merge a producer's current family/frame/state mapping through public identity.

    @param mapped Existing per-family current observations, independent of range allocation.
    @param asset Existing public asset ID established by the native/source witness.
    @param frames Exact complete PNG identities from that observation.
    @param context Producer-owned original state/configuration/fixture description.
    @param public Existing public asset lookup, the sole range/geometry authority.
    @return None; shared scenes remain shared evidence.
    """
    assert asset in public, "unknown gallery family"
    row = mapped.setdefault(asset, {"frames": {}, "native_views": []})
    for frame in frames:
        row["frames"][frame["path"]] = frame
    row["native_views"].append(context)


def current_mappings(proofs, public, packed, cache):
    """Adapt completed producer schemas without constructing another sprite registry.

    @param proofs Four already hash-verified closure documents in ROLES order.
    @param public Sole public asset lookup.
    @param packed Actual packed range owner.
    @param cache Per-validation checked identity map.
    @return Current mappings covering all twenty-six additions and refreshed collector.
    """
    purchase, structures, movement, geothermal = proofs
    mapped = {}
    for proof in (movement, geothermal):  # These producers already map actual native IDs/poses to public ranges; reuse their explicit state/fixture boundaries verbatim.
        for row in proof["families"]:
            assert row["native_offset"] == public[row["asset_id"]]["packed"]["native_offset"]
            assert row["sprite_count"] == public[row["asset_id"]]["packed"]["sprite_count"]
            for context in row["native_views"]:
                bind(mapped, row["asset_id"], row["frames"], context, public)
    for report in purchase["reports"]:  # Preview policy is real native available/locked rendering, not a purchase transaction or a full purchase window.
        assert report["configuration"] in ("Debug", "RelWithDebInfo")
        for witness in report["source_witnesses"]:
            asset = witness["asset_id"]
            assert witness["native_offset"] == public[asset]["packed"]["native_offset"]
            assert witness["sprite_count"] == public[asset]["packed"]["sprite_count"]
            bind(mapped, asset, [witness["frame"]], {"configuration": report["configuration"], "world": report["world"], "original": report["original"],
                 "state_owner": "actual native engine/company/date availability", "fixture_classification": "native purchase-preview canvas;not transaction/full window", "witness": witness}, public)
    for report in structures["reports"]:  # The source witness identifies the actual facility/conduit family. Capsule movement has its stronger original useful-world mappings above.
        native = report["native"]
        source = read(checked(native["packed_pixels"]["manifest"], cache))
        asset = source.get("asset_id")
        if asset is None:
            continue
        frames = [frame.get("file", frame) for frame in native["frames"]]
        bind(mapped, asset, frames, {"configuration": report["configuration"], "case": report.get("case", report.get("scene")), "report": report["report"],
             "original_writer": native["writer"], "original_input": native["input"], "native_fields": native.get("fields"),
             "state_owner": "actual native rail/facility/installed conduit geometry,publication,reservation and service state",
             "fixture_classification": native.get("fixture_classification", "prepared-floor/material presentation fixture;paid native commands/status boundary,not useful bootstrap")}, public)
    assert mapped.keys() == activation.SELECTED_RUNTIME_ADDITIONS | {"facility.vent-collector"}, "selected current family coverage is incomplete or future"
    return mapped


def validate(public, packed, runtime, selection):
    """Verify the complete selected gallery using current closures and retained prefix.

    @param public Current public metadata or an isolated corruption-test candidate.
    @param packed Current complete packing report.
    @param runtime Candidate binding rows from the existing activation owner.
    @param selection Explicit existing EvidenceSet, never an inferred latest run.
    @return Verified sixty-nine family rows, unique native frames and closure summary.
    """
    cache = {}
    validate_boundary(public, packed, selection, cache)
    boundary = selection.document["selected_gallery_boundary"]
    boundaries = list(delivery_boundaries(selection, cache))
    bindings = activation.validate_binding_review(public, packed)
    assert runtime["bindings"] == bindings, "false/incomplete gallery consumer bindings"
    assert runtime["packed_payload"] == packed["packed_grf"], "gallery payload differs from runtime"
    checked(packed["packed_grf"], cache)
    assert activation.digest(ROOT / "build/baseset/openttd.grf") == packed["packed_grf"]["sha256"], "game distribution differs from selected gallery"
    old = read(checked(selection.document["retained_published_gallery"], cache))
    retained_runtime = read(checked(selection.document["retained_published_runtime"], cache))
    assert old["technical_acceptance"] == "verified" and old["selected_families"] == 43 and old["selected_sprites"] == 3495
    assert old["packed_payload"] == packed["packed_grf"] == retained_runtime["packed_payload"]
    assert {row["asset_id"] for row in old["families"]} == {row["asset_id"] for row in retained_runtime["bindings"]}
    proofs = [read(selection.selected("reports", role)) for role in ROLES]
    for proof in proofs:  # Opaque sealed native acceptance supplies execution, full-world and exact original-pixel checks. Fresh hashes keep retained sources/frames/worlds observable after cleanup.
        assert proof["native_version"] == 434 and proof["native_cases"] > 0 and proof["native_assertions"] > 0
        assert proof["complete_cross_reader_frame_pairs"] and proof["complete_repeat_frame_pairs"] and proof["complete_cross_reader_world_pairs"]
        validate_retained(proof, [row["public"] for row in boundaries], [row["packed"] for row in boundaries], cache)
    assets = {family["asset_id"]: family for family in public["assets"]}
    mapped = current_mappings(proofs, assets, packed, cache)
    rows, unique = [], {}
    previous = {row["asset_id"]: row for row in old["families"]}
    old_frames = {row["path"]: row for row in old["frames"]}
    for binding in bindings:  # Earlier review stays explicitly dated. Only current delivered additions and collector receive new native-scene claims; retained terrain never becomes an active body consumer.
        asset = binding["asset_id"]
        family = assets[asset]
        # Older and selected-ground producers expose coverage in their own
        # established schema locations; neither representation grants runtime
        # authority, and both remain protected by the frozen public contract.
        coverage = family.get("authoring", {}).get("coverage_complete", family["source"].get("source_coverage_complete", False))
        assert coverage and family["source"]["status"] == "authored"
        if asset in mapped:
            native = mapped[asset]
            frames = list(native["frames"].values())
            configurations = {context["configuration"] for context in native["native_views"]}
            assert configurations == {"Debug", "RelWithDebInfo"}, "current gallery family lacks one reader"
            contexts, kind = native["native_views"], "current_selected_native"
        else:
            prior = previous[asset]
            assert (prior["native_offset"], prior["sprite_count"]) == (binding["native_offset"], binding["sprite_count"])
            frames = [old_frames[name] for name in prior["frames"]]
            contexts, kind = [], "retained_dated_review"
        assert frames, "gallery family has no actual native frames"
        for frame in frames:
            checked(frame, cache)
            unique[frame["path"]] = {**frame, "review_kind": kind}
        rows.append({"asset_id": asset, "native_offset": binding["native_offset"], "sprite_count": binding["sprite_count"], "consumer": binding["consumer"],
                     "frames": [frame["path"] for frame in frames], "evidence": binding["evidence"], "native_views": contexts, "review_kind": kind,
                     "technical_acceptance": "retained_source_only" if binding["retained_only"] else "verified", "user_feedback": "deferred"})
    assert len(rows) == 69 and sum(row["sprite_count"] for row in rows) == 10557
    movement_logs = {report["native"]["log"]["path"] for report in proofs[2]["reports"]}
    # The first gallery reused the SAME Forge process for purchase and movement.
    # A refreshed purchase may instead name a distinct current observation; only
    # matching native log identities justify subtracting duplicate execution.
    forge_observations = [report["native"]["native"] for report in proofs[0]["reports"] if report["world"] == "forge"]
    shared_forge = [native for native in forge_observations if native["log"]["path"] in movement_logs]
    summary = {"selected_families": 69, "selected_sprites": 10557, "current_families": len(mapped), "retained_review_families": 42,
               "active_consumers": sum(not binding["retained_only"] for binding in bindings), "native_version": 434,
               "closures": {role: selection.document["reports"][role] for role in ROLES}, "retained_gallery": selection.document["retained_published_gallery"],
               "boundary": boundary, "native_cases_unique": sum(proof["native_cases"] for proof in proofs) - sum(row.get("case_runs", row.get("cases", 0)) for row in shared_forge),
               "native_assertions_unique": sum(proof["native_assertions"] for proof in proofs) - sum(row["assertions"] for row in shared_forge),
               "scope_note": f"Four exact closures reference{sum(proof['native_cases'] for proof in proofs)} cases;{len(shared_forge)} exact Forge observations are shared by purchase/movement. Retained43-family review refreshes collector,leaving42dated rows."}
    return rows, list(unique.values()), summary
