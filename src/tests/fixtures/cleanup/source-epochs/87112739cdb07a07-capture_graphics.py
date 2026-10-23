"""Capture named graphics input edges without importing publishers or native producers.

@file capture_graphics.py Disposable metadata audit; no generated pixels certify acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group4.
"""

import ast
from collections import deque
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_inventory import checked_path
from artifact_space_graph import graph_cycles

PATH_FIELDS = {"path", "source", "original", "originals", "selected_originals", "rejected_originals",
    "source_record", "atlas_manifest", "native_atlas", "manifest", "source_atlas", "source_payload",
    "payload", "native_payload", "binding_manifest", "log", "terminal", "binary", "executable",
    "baseline", "before", "geometry_descriptor", "qualification", "source_handoff"}
HISTORY_FIELDS = {"files", "predecessor", "before", "baseline", "native_proof", "historical_versions"}


def role_for(path, historical=False):
    """Classify ownership without promoting old outputs into permanent sources.

    @param path Repository-relative identity declared by a named metadata field.
    @param historical Whether the reader explicitly selected a frozen source epoch.
    @return Semantic storage role; EXEs never become future retained input payloads.
    """
    if path.endswith(".exe"):
        return "historical_executable"
    if historical:
        return "historical_epoch"
    if path.startswith("media/baseset/"):
        return "canonical_shipping_asset"
    if path.startswith("media/"):
        return "original_or_source_contract"
    if path.startswith(("src/", "regression/")):
        return "source_or_curated_input"
    if path.endswith((".log", ".txt")):
        return "required_transcript"
    if path.endswith("packed-sprites.bin"):
        return "generated_expected_payload"
    return "generated_review_product"


def metadata_edges(document):
    """Read only named path fields, including nested atlas/candidate/frame records.

    @param document Source, scope, capture, policy or selection metadata object.
    @return Declared path/hash/role/pointer records; prose strings never become edges.
    """
    pending = [(document, "", False)]
    found = {}
    # Source metadata nests original generation receipts, frame references and
    # frozen predecessor snapshots. Preserve that semantic context for each edge.
    while pending:
        value, pointer, historical = pending.pop()
        if isinstance(value, list):
            pending.extend((row, f"{pointer}/{index}", historical) for index, row in enumerate(value))
            continue
        if not isinstance(value, dict):
            continue
        for key, child in value.items():
            next_pointer = pointer + "/" + key
            is_history = historical or key in HISTORY_FIELDS
            if isinstance(child, (dict, list)):
                pending.append((child, next_pointer, is_history))
                # An originals list can use path strings rather than identities;
                # retain those explicit aliases as unbound source declarations.
                if key in PATH_FIELDS and isinstance(child, list):
                    for index, name in enumerate(child):
                        if isinstance(name, str):
                            pending.append(({key: name}, f"{next_pointer}/{index}", is_history))
                continue
            # Only declared repository namespaces are input identities. Machine
            # commands and prose containing slashes remain provenance/tool context.
            if key not in PATH_FIELDS or not isinstance(child, str) or not child.startswith(("media/", "logs/", "mlm/", "src/", "test-cache/", "build/")):
                continue
            # A few geometry descriptors name several compiled source owners in
            # one semicolon-delimited field. Follow the explicit owners separately;
            # their descriptive combined spelling is not a missing filename.
            if key == "source" and ";" in child:
                pending.extend(({"source": name.strip()}, next_pointer, is_history) for name in child.split(";"))
                continue
            digest = value.get("sha256") if key == "path" else value.get(key + "_sha256")
            if key == "source_atlas":
                digest = digest or value.get("source_sha256")
            if key == "manifest":
                digest = digest or value.get("manifest_sha256")
            role = role_for(child, is_history)
            found.setdefault((child, digest, role), dict(path=child, sha256=digest,
                role=role, json_pointer=next_pointer, hash_authority="declared" if digest else "unbound_path_declaration"))
    return list(found.values())


def literal_metadata_inputs(path):
    """Extract explicit source-policy JSON literals without executing their module.

    @param path Python producer/contract source selected by a metadata @file declaration.
    @return Contained repository-relative metadata literals named by that original owner.
    """
    tree = ast.parse(path.read_text(encoding="utf-8-sig"))
    return sorted({node.value for node in ast.walk(tree)
        if isinstance(node, ast.Constant) and isinstance(node.value, str)
        and node.value.startswith(("media/", "logs/", "src/tests/fixtures/")) and node.value.endswith(".json")})


def main():
    """Record source-to-proof edges, old snapshot constraints and real payload owners.

    @return None; publish disposable enumeration and a compact durable reader map.
    """
    public_path = "media/multilayer/contracts/multilayer_sprite_manifest.json"
    public = json.loads((ROOT / public_path).read_text(encoding="utf-8"))
    scope_paths = [f"media/multilayer/{name}-pack-scope.json" for name in ("m1", "space", "orbital", "launch", "ground")]
    pending = deque([public_path, *scope_paths, "logs/proofs/mlm/native-art/evidence.json"])
    sources = {}
    # Resolve producer documentation only within existing authored tool owners;
    # duplicate basenames stay ambiguous rather than becoming a guessed generator.
    for path in (ROOT / "media/multilayer").rglob("*.py"):
        sources.setdefault(path.name, []).append(path)
    visited, edges, producers, missing, adjacency = set(), [], set(), set(), {}
    while pending:
        relative = pending.popleft()
        if relative in visited:
            continue
        visited.add(relative)
        path = checked_path(relative, ROOT)
        if not path.is_file():
            missing.add(relative)
            continue
        document = json.loads(path.read_text(encoding="utf-8"))
        adjacency[relative] = []
        declared = metadata_edges(document)
        description = document.get("_documentation", "")
        # Some immutable contracts store structured documentation rather than
        # prose. Only actual prose can declare an @file producer identity.
        if not isinstance(description, str):
            description = ""
        names = re.findall(r"@file\s+([a-zA-Z0-9_-]+\.py)", description)
        # A report's original documented generator and that generator's named
        # source policies are independent roots; no report import initializes runs.
        for name in names:
            matches = sources.get(name, [])
            if len(matches) == 1:
                producer = matches[0]
                producers.add(producer.relative_to(ROOT).as_posix())
                for selected in literal_metadata_inputs(producer):
                    declared.append(dict(path=selected, sha256=None, role="original_or_source_contract" if selected.startswith("media/") else "historical_epoch",
                        json_pointer="@file/source-policy", hash_authority="producer_literal"))
        for row in declared:
            target = checked_path(row["path"], ROOT)
            row.update(document=relative, exists=target.is_file())
            edges.append(row)
            if not row["exists"]:
                missing.add(row["path"])
            # Frozen before-file arrays are history, not a request to reopen all
            # old application/source trees. Other named metadata reads are followed.
            if row["path"].endswith(".json") and row["role"] not in {"historical_epoch", "historical_executable"}:
                adjacency[relative].append(row["path"])
                pending.append(row["path"])
    fixtures = []
    # Each expected payload has its original producer and actual native loader;
    # generated expected bytes must be prepared before that independent consumer.
    for name, producer, native, tag in (
        ("m1", "src/tests/multilayer_packed_art_fixture.py", "src/tests/multilayer_packed_load.cpp", "[.m1-packed-art-load]"),
        ("space", "src/tests/space_art_fixture.py", "src/tests/multilayer_packed_load.cpp", "[.space-packed-art-load]"),
        ("orbital", "src/tests/orbital_art_fixture.py", "src/tests/multilayer_packed_load.cpp", "[.orbital-packed-art-load]"),
        ("launch-fleet", "src/tests/multilayer/launch_fleet_payload_fixture.py", "src/tests/multilayer_packed_load.cpp", "[.launch-fleet-packed-art-load]"),
        ("ground", "src/tests/multilayer/ground_payload_fixture.py", "src/tests/ground_packed_load.cpp", "[.ground-packed-art-load]")):
        assert (ROOT / producer).is_file()
        assert tag in (ROOT / native).read_text(encoding="utf-8")
        fixtures.append(dict(id=name, producer=producer, native_reader=native, native_tag=tag,
            old_output=f"logs/proofs/mlm/native-art/expected/{name}-packed-sprites.bin",
            requires_output_only_adapter=True, publishes_canonical_payload=name == "ground"))
    summary = dict(_documentation=__doc__, schema_version=1, public=public_path,
        family_count=len(public["assets"]), scopes=scope_paths,
        source_generators=sorted(producers), expected_payloads=fixtures,
        metadata_documents=len(visited), declared_edges=len(edges), missing=sorted(missing),
        publication_entry_points=[f"media/multilayer/pack_{name}_art.py:publish" for name in ("m1", "space", "orbital", "launch_fleet")]
            + ["src/tests/multilayer/ground_payload_fixture.py:produce"],
        regression_policy="Use original pure derive functions with explicit generated destinations; never call publication implicitly.",
        baseline_policy="Retain minimal independent source/expectation snapshots; regenerate atlases/payloads/reports/native frames; seal old process facts without EXEs.")
    # This is only a dependency index into the sole public media authority;
    # copying its source/packed/runtime declarations would create a second catalogue.
    summary["families"] = [dict(asset_id=row["asset_id"], public_json_pointer=f"/assets/{index}")
        for index, row in enumerate(public["assets"])]
    cycles = graph_cycles({(key, "metadata", "graphics"): [(value, "metadata", "graphics") for value in values]
        for key, values in adjacency.items()})
    summary["source_proof_cycles"] = cycles
    summary["cycle_break_roots"] = ["approved original media pixels", "source transformation/policy records",
        "independent expected geometry/pixel contracts", "minimal frozen prefix/approval facts"]
    summary["missing_classification"] = [dict(path=name,
        disposition="regenerate_native_capture" if name.endswith(".ppm") else "historical_process_fact",
        png_counterpart_present=(ROOT / Path(name).with_suffix(".png")).is_file() if name.endswith(".ppm") else None)
        for name in sorted(missing)]
    summary["retired_authoring_policy"] = "Old rejected/selected-original and exact pre-publication status assertions remain historical; current regression observes selected originals without replaying those authoring epochs."
    absent_originals = {edge["path"] for edge in edges if not edge["exists"] and edge["role"] == "original_or_source_contract"}
    if absent_originals:
        raise ValueError(f"missing durable original/source contracts: {sorted(absent_originals)}")
    (ROOT / "test-cache/cleanup/graphics-edges.json").write_text(json.dumps(dict(_documentation=__doc__, edges=edges, adjacency=adjacency), indent=2) + "\n", encoding="utf-8")
    (ROOT / "src/tests/fixtures/cleanup/graphics-dependencies.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: summary[key] for key in ("family_count", "metadata_documents", "declared_edges")}))
    print("documented source generators", len(producers), "missing declarations", len(missing))


if __name__ == "__main__":
    main()
