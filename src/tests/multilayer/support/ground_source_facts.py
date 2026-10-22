"""Preserve independent ground source facts without retaining a publication archive.

@file ground_source_facts.py Export only from exact original scoped source reports.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups17D/17DB-17DE.
Ordinary verification reads the immutable fixture and durable source owners.
Historical native descriptor/source epochs remain facts, never a current pass.
"""

import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys

from artifact_paths import contained, repository_root
from artifact_rebuild import identity

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
from tools import ground_art_handoff as handoff

FIXTURE = ROOT / "src/tests/fixtures/graphics/ground-source-facts.json"
FIXTURE_SHA256 = "989b4d24dd721b5bbab56235ced96f47823ba7d4a3fa8df7a167fb2c85f64b56"
POSE_FIELDS = ("state", "frame", "direction", "tile_variant", "part", "box", "anchor",
               "native_packed_anchor", "native_graphics", "native_track_bits", "world_cell",
               "native_layout", "native_origin", "native_contact", "alias_of", "foreground_policy",
               "structurally_absent", "source_pixels_sha256", "company_color_pixels", "company_color_mask_sha256")
PUBLIC_FIELDS = (*handoff.GEOMETRY, "kind", "palette", "transparent_index", "bounds", "source")


def semantic_digest(value):
    """Hash physical metadata independently of presentation whitespace.

    @param value Original or selected current physical geometry/pose projection.
    @return Stable SHA256; this function never creates an expected baseline.
    """
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()


def public_projection(family):
    """Select independent physical/source fields without native qualification metadata.

    @param family Existing public ground family at its original or current epoch.
    @return Detached native geometry, original source and palette expectations.
    """
    return {name: copy.deepcopy(family.get(name)) for name in PUBLIC_FIELDS}


def pose_digest(poses):
    """Bind every native-consumed pose, pixel, absence, mask and compensated anchor.

    @param poses Complete normalized identity-keyed original or current ground source rows.
    @return Matrix SHA256 in canonical logical order, excluding generated path spelling.
    """
    rows = [{name: row.get(name) for name in POSE_FIELDS} for _, row in sorted(poses.items())]
    return semantic_digest(rows)


def derived_source_inputs(selected, contracts, documents):
    """Resolve original media ownership where old public source paths name derived atlases.

    @param selected Frozen selected public physical/source projections.
    @param contracts Verified original report/atlas identities and physical matrix expectations.
    @param documents Exact original source reports already checked by the existing owner.
    @return Original media records/input identities; old generated PNGs remain historical facts.
    """
    result = {}
    # Four legacy rows use a generated atlas as their public source spelling.
    # Preserve that fact while selecting actual original inputs from its record.
    for asset, family in selected.items():
        source = family["source"]
        if source["path"].startswith("media/"):
            continue
        original = contracts[asset]["original_atlas"]
        if {name: source[name] for name in ("path", "sha256")} != original:
            raise ValueError("ground non-media source is not its explicitly qualified derived atlas")
        report = documents[contracts[asset]["original_report"]["path"]]
        path = contained(ROOT / report["source_record"], ROOT / "media")
        record = json.loads(path.read_text(encoding="utf-8"))
        inputs = {}
        # Keep every original draft/input for this actual family without making
        # unselected sibling families hidden reconstruction prerequisites.
        for candidate in record["candidates"]:
            if candidate["asset_id"] != asset:
                continue
            actual = identity(contained(ROOT / candidate["path"], ROOT / "media"), ROOT)
            if actual["sha256"] != candidate["sha256"]:
                raise ValueError("ground original candidate pixels changed: " + candidate["path"])
            inputs[actual["path"]] = actual
            # Actual prompt input images remain original source provenance;
            # their current exact media bytes are retained independently of galleries.
            for reference in candidate.get("references", []):
                name = reference["path"] if isinstance(reference, dict) else reference
                actual = identity(contained(ROOT / name, ROOT / "media"), ROOT)
                inputs[actual["path"]] = actual
        result[asset] = dict(historical_derived_source=original, original_record=identity(path, ROOT),
                             original_inputs=[inputs[name] for name in sorted(inputs)])
    return result


def original_facts():
    """Read exact original source reports and reduce historical staging to comparison facts.

    @return Compact independent expectations; old images/EXEs/reports are not copied.
    @throws ValueError When scoped original report/pixel/prefix identities no longer agree.
    """
    scope, before, public = handoff.read_scope()
    scope_ref = identity(handoff.SCOPE, ROOT)
    rows = {row["asset_id"]: row for row in public["assets"]}
    original = {row["asset_id"]: row for row in before["public"]["assets"]}
    selected = {}
    # Existing source authority must still agree with the historical physical
    # contract. Later public runtime/container metadata is a different epoch.
    for asset in scope["selected_asset_ids"]:
        current = public_projection(rows[asset])
        if current != public_projection(original[asset]):
            raise ValueError("ground original physical public facts differ: " + asset)
        selected[asset] = current
    prefix = []
    # Preserve only actual native range/geometry facts, not old qualification
    # binaries, logs or full authoring/runtime rows from the staging snapshot.
    for row in before["native_prefix"]:
        asset = row["asset_id"]
        prefix.append(dict(asset_id=asset,
                           geometry={name: original[asset].get(name) for name in handoff.GEOMETRY},
                           packed={name: row["packed"].get(name) for name in ("route", "native_offset", "sprite_count")}))
    paths = {before["public"]["legacy"]["reference_sheet"], "media/baseset/openttd/map_layers.nfo"}
    # The existing packing scopes identify actual prefix sheets/declarations.
    # Their original file hashes suffice; no whole application archive is needed.
    for prior in before["prior_scopes"].values():
        paths.update(prior[name] for name in ("png", "nfo", "header"))
    files = {row["path"]: row for row in before["files"]}
    protected = []
    # Every indispensable prefix byte must match the original snapshot before
    # its compact identity becomes a future comparison expectation.
    for name in sorted(paths):
        actual = identity(contained(ROOT / name, ROOT), ROOT)
        if actual["sha256"] != files[name]["sha256"]:
            raise ValueError("ground original native prefix file changed: " + name)
        protected.append(actual)
    contracts, documents, images = {}, {}, {}
    # Original scope hashes qualify each old report/atlas before reduction.
    # Ordinary verification will compare current poses to these fixed matrices.
    for reference in scope["source_rows"]:
        asset = reference["asset_id"]
        poses, image = handoff.source_poses(rows[asset], scope, before, documents, images)
        contracts[asset] = dict(original_report=dict(path=reference["manifest"], sha256=reference["manifest_sha256"]),
                               original_atlas=dict(path=reference["source_atlas"], sha256=reference["source_sha256"]),
                               pose_count=len(poses), pose_contract_sha256=pose_digest(poses))
    descriptors, mismatches = [], []
    from tools import ground_mine_contract, ground_processor_contract
    # Measure the original descriptor discrepancy without rebasing its hashes
    # or presenting a historical compiled proof as a current native result.
    for owner in (ground_mine_contract, ground_processor_contract):
        descriptors.append(identity(owner.DESCRIPTOR, ROOT))
        descriptor = json.loads(owner.DESCRIPTOR.read_text(encoding="utf-8"))
        for name, expected in descriptor["native_source_sha256"].items():
            actual = owner.source_digest(ROOT / name)
            if actual != expected:
                mismatches.append(dict(descriptor=owner.DESCRIPTOR.relative_to(ROOT).as_posix(), source=name,
                                       historical_sha256=expected, observed_at_migration_sha256=actual,
                                       role="historical_native_epoch_requires_separate_current_owner_proof"))
    return dict(_documentation=__doc__, schema_version=1, kind="independent_ground_source_facts",
                scope=scope_ref, historical_baseline=identity(ROOT / scope["baseline"], ROOT),
                historical_source_audit=scope["source_audit"],
                native_start=scope["native_start"], native_end_exclusive=scope["native_end_exclusive"],
                pose_fields=list(POSE_FIELDS), selected_public_facts=selected, native_prefix=prefix,
                protected_prefix_files=protected, source_pose_contracts=contracts,
                derived_source_inputs=derived_source_inputs(selected, contracts, documents),
                original_native_descriptors=descriptors, historical_native_mismatches=mismatches)


def export_facts():
    """Preserve indispensable expectations from original reports once, never update a baseline.

    @return Exact fixture identity; a differing existing fixture refuses replacement.
    """
    document = original_facts()
    encoded = (json.dumps(document, indent=2) + "\n").encode("utf-8")
    target = contained(FIXTURE, ROOT / "src/tests/fixtures/graphics")
    if target.exists():
        if target.read_bytes() != encoded:
            raise ValueError("existing independent ground source facts cannot be regenerated or replaced")
    else:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(encoded)
    return identity(target, ROOT)


def load_facts():
    """Read the exact independently captured expectation without old product access.

    @return Immutable physical/source/prefix facts, with native epochs explicitly historical.
    @throws ValueError On missing/corrupt expectations or changed original source scope.
    """
    target = contained(FIXTURE, ROOT / "src/tests/fixtures/graphics")
    if not target.is_file():
        raise ValueError("independent ground source expectation is missing; restore its original retained bytes")
    if identity(target, ROOT)["sha256"] != FIXTURE_SHA256:
        raise ValueError("independent ground source expectation changed; no automatic baseline update")
    document = json.loads(target.read_text(encoding="utf-8"))
    # The original scope and matrix projection define this comparison role.
    # A valid JSON file cannot substitute another family set or native epoch.
    if document["scope"] != identity(handoff.SCOPE, ROOT) or document["pose_fields"] != list(POSE_FIELDS):
        raise ValueError("independent ground source scope or physical projection changed")
    return document


def verify_original_descriptor(document, path):
    """Bind physical source reconstruction to the exact original native descriptor.

    @param document Parsed original geometry; detached or edited geometry is refused.
    @param path Existing media owner of that historical native descriptor.
    @return Exact independently retained descriptor identity, never current native proof.
    """
    actual = identity(contained(path, ROOT / "media/multilayer/contracts"), ROOT)
    facts = load_facts()
    # Only the independently frozen descriptor can authorize historical source
    # geometry; a caller cannot bypass native freshness with invented geometry.
    if actual not in facts["original_native_descriptors"] or document != json.loads(Path(path).read_text(encoding="utf-8")):
        raise ValueError("physical source geometry differs from its original native descriptor")
    return actual


def verify_facts():
    """Verify durable original/public/prefix inputs while leaving old native epochs historical.

    @return Measured comparison counts; no generated report or native process is required.
    """
    facts = load_facts()
    public = json.loads(handoff.PUBLIC.read_text(encoding="utf-8"))
    rows = {row["asset_id"]: row for row in public["assets"]}
    # Source reconstruction cannot change original physical fields or substitute
    # another raw original while borrowing this family's approved atlas identity.
    for asset, expected in facts["selected_public_facts"].items():
        if public_projection(rows[asset]) != expected:
            raise ValueError("ground retained physical/source contract changed: " + asset)
        source = expected["source"]
        if source["path"].startswith("media/"):
            if identity(contained(ROOT / source["path"], ROOT / "media"), ROOT)["sha256"] != source["sha256"]:
                raise ValueError("ground original source pixels changed: " + asset)
        else:
            original = facts["derived_source_inputs"][asset]
            # A historical derived-source spelling cannot redirect a read into
            # disposable storage; actual regeneration starts from retained originals.
            if {name: source[name] for name in ("path", "sha256")} != original["historical_derived_source"]:
                raise ValueError("ground derived-source historical identity changed")
            for reference in [original["original_record"], *original["original_inputs"]]:
                if identity(contained(ROOT / reference["path"], ROOT / "media"), ROOT) != reference:
                    raise ValueError("ground derived source original inputs changed")
    # Actual prefix ranges and geometry remain independent of later shared-GRF
    # checksums and old rendering receipts; no native pass is inferred here.
    for expected in facts["native_prefix"]:
        row = rows[expected["asset_id"]]
        if ({name: row.get(name) for name in handoff.GEOMETRY} != expected["geometry"]
                or {name: row["packed"].get(name) for name in expected["packed"]} != expected["packed"]):
            raise ValueError("ground native prefix geometry/range changed")
    # Only indispensable retained prefix/descriptor files are dereferenced;
    # historical baseline/report/EXE/log identities remain non-read facts.
    for reference in [*facts["protected_prefix_files"], *facts["original_native_descriptors"]]:
        if identity(contained(ROOT / reference["path"], ROOT), ROOT) != reference:
            raise ValueError("ground retained prefix or original descriptor changed")
    import convert_space_art as originals
    fleet = json.loads((ROOT / "media/multilayer/source/ground/fleet-generation-record.json").read_text(encoding="utf-8"))
    m1 = rows["vehicle.model-04"]["authoring"]
    # The actual original fleet AI input was an already approved M1 atlas.
    # Its exact retained media copy is source provenance, never an old proof read.
    originals.source_reference(dict(path=m1["native_atlas"], sha256=m1["native_sha256"]), fleet, feature="ground")
    return dict(selected_families=len(facts["selected_public_facts"]),
                source_families=len(facts["source_pose_contracts"]),
                pose_count=sum(row["pose_count"] for row in facts["source_pose_contracts"].values()),
                protected_prefix_files=len(facts["protected_prefix_files"]),
                historical_native_mismatches=len(facts["historical_native_mismatches"]),
                retained_fleet_generation_inputs=1, native_proof=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--export", action="store_true", help="one-time preservation from exact original source reports")
    arguments = parser.parse_args()
    print(json.dumps(export_facts() if arguments.export else verify_facts(), indent=2))
