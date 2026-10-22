"""Typed read-only adapters for the current Space historical evidence closure.

@file artifact_space_graph.py Metadata tracing never executes gameplay or reloads.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group3.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md.
"""

import argparse
from collections import deque
import json
from pathlib import Path
import re

from artifact_inventory import checked_path, classify_reference, file_identity
from artifact_paths import contained, owned_root, repository_root

READERS = dict(bound="surface_original_evidence.bind", products="surface_original_evidence.bind",
    epoch="surface_original_evidence.verify", operation="surface_original_evidence.bind",
    process="surface_original_evidence.bind", leaf="surface_original_evidence.verify")


def reference_edges(document, kind, configuration=None):
    """Adapt named receipt fields; arbitrary JSON strings never become dependencies.

    @param document Parsed record selected by a typed parent edge.
    @param kind Explicit bound, products, epoch, operation, process or leaf schema.
    @param configuration Optional selected process configuration; final saves stay paired.
    @return List of JSON pointer, identity, child schema and semantic role tuples.
    """
    edges = []

    def add(pointer, reference, child="leaf", role="historical_provenance"):
        """Append a declared path/hash identity without guessing other record fields.

        @param pointer JSON pointer identifying the actual named reader input.
        @param reference Exact path/SHA256 record, or None for an absent optional field.
        @param child Explicit schema adapter to use after verifying these bytes.
        @param role Continuation versus historical meaning for this edge.
        @return None; malformed declared identities fail visibly.
        """
        if reference is None:
            return
        # Typed fields must contain both identity parts; silently accepting a
        # partial path would hide a broken historical or continuation dependency.
        if not isinstance(reference, dict) or not {"path", "sha256"} <= reference.keys():
            raise ValueError(f"invalid identity at {pointer}")
        if not re.fullmatch(r"[0-9a-f]{64}", reference["sha256"]):
            raise ValueError(f"invalid SHA256 at {pointer}")
        edges.append((pointer, {key: reference[key] for key in ("path", "sha256")}, child, role))

    if kind == "bound":
        for field, child in (("foundation", "bound"), ("products", "products"), ("source", "leaf")):
            add("/" + field, document.get(field), child)
        versions = document.get("historical_versions", [])
        # Two older receipts name one immutable version bundle instead of a list;
        # preserve their actual schema rather than interpreting dictionary keys.
        if isinstance(versions, dict):
            versions = [versions]
        for index, reference in enumerate(versions):
            add(f"/historical_versions/{index}", reference, "epoch", "source_baseline")
        # The root pair always remains visible. Selecting one configuration only
        # bounds historical process traversal; it cannot hide the other final save.
        for index, row in enumerate(document.get("configurations", [])):
            add(f"/configurations/{index}/output", row.get("output"), role="save")
            if configuration is None or row["configuration"] == configuration:
                for field in ("receipt", "ordinary", "reload_receipt"):
                    add(f"/configurations/{index}/{field}", row.get(field), "operation")
                for field in ("receipts", "lineage"):
                    for number, value in enumerate(row.get(field, [])):
                        selected = value.get("receipt") if field == "lineage" else value
                        add(f"/configurations/{index}/{field}/{number}", selected, "operation")
        # The original passenger planning proof predates configuration aggregates;
        # its explicit receipt list, rather than its numeric stage observations,
        # supplies the immutable native history.
        for index, reference in enumerate(document.get("receipts", [])):
            add(f"/receipts/{index}", reference, "operation")
    elif kind == "products":
        add("/predecessor", document.get("predecessor"), "products")
        for index, product in enumerate(document["products"]):
            add(f"/products/{index}/source", product["source"], role="source")
            add(f"/products/{index}/retained", product["retained"])
    elif kind == "epoch":
        for index, product in enumerate(document["products"]):
            # Before-edit bundles evolved from flat archive rows to the shared
            # source/retained pair. Both retain exact, separately checked identities.
            if isinstance(product["source"], dict):
                add(f"/products/{index}/source", product["source"], role="source")
                add(f"/products/{index}/retained", product["retained"], role="source_baseline")
            else:
                add(f"/products/{index}", product, role="source_baseline")
    elif kind == "operation":
        for field in ("receipt", "source"):
            add("/" + field, document.get(field), "operation" if field == "receipt" else "leaf")
        # Historical ordinary-replay summaries name a static restore plus two
        # already executed branches. Trace those receipts without invoking replay,
        # which would advance each branch by3,000ticks and repeat closed work.
        if "operation" not in document and "continuations" in document:
            for field in ("source_receipt", "restore"):
                add("/" + field, document[field], "operation")
            for index, value in enumerate(document["continuations"]):
                add(f"/continuations/{index}", value, "operation")
            add("/output", document["output"], role="save")
            return edges
        operation = document["operation"]
        for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
            add("/operation/" + field, operation.get(field), role="save" if field in {"source", "before", "output"} else "historical_provenance")
        add("/operation/receipt", operation.get("receipt"), "operation")
        for field, child in (("sources", "leaf"), ("native_processes", "process"), ("historical_versions", "epoch")):
            for index, reference in enumerate(operation.get(field, [])):
                add(f"/operation/{field}/{index}", reference, child)
        for field in ("capital_proposal_receipt", "foundation"):
            add("/operation/" + field, operation.get(field), "bound" if field == "foundation" else "operation")
    elif kind == "process":
        add("/executable", document["executable"])
        for field in ("inputs", "artifacts"):
            for index, reference in enumerate(document[field]):
                add(f"/{field}/{index}", reference, role="save")
    elif kind != "leaf":
        raise ValueError(f"unregistered Space graph schema: {kind}")
    return edges


def graph_cycles(adjacency):
    """Find directed metadata cycles without recursion or duplicate-branch false alarms.

    @param adjacency Mapping of typed metadata identity keys to child keys.
    @return Cycle back-edges; shared predecessors in a DAG are harmless.
    """
    colors, cycles = {}, []
    # A gray child is an ancestor in this actual DFS branch. A black child is
    # already validated shared history, so it must not be reported as a cycle.
    for origin in adjacency:
        if colors.get(origin):
            continue
        pending = [(origin, False)]
        while pending:
            node, closing = pending.pop()
            if closing:
                colors[node] = 2
                continue
            if colors.get(node) == 2:
                continue
            colors[node] = 1
            pending.append((node, True))
            for child in reversed(adjacency.get(node, [])):
                if colors.get(child) == 1:
                    cycles.append([node[0], child[0]])
                elif not colors.get(child):
                    pending.append((child, False))
    return cycles


def current_validation_edges(origin, initial, edges, documents):
    """Select the original current binder's bounded read-set within declared lineage.

    @param origin Exact current bound proof identity.
    @param initial Its independently recorded checkpoint and foundation document.
    @param edges Complete typed declared lineage, including superseded ancestors.
    @param documents Exact metadata documents already read during tracing.
    @return Set of required edge indices, without claiming ancestor source is current.
    """
    grouped = {}
    for index, edge in enumerate(edges):
        grouped.setdefault(edge["document"], []).append((index, edge))
    foundation = initial.get("foundation")
    boundaries = set()
    if foundation:
        value = documents.get((foundation["path"], foundation["sha256"]), {})
        boundaries = {row["receipt"]["path"] for row in value.get("configurations", []) if "receipt" in row}
    pending = deque([(origin["path"], "root")])
    visited, required = set(), set()
    # bind reads one immediate foundation, its flattened retained products, the
    # selected before-edit bundles, this group's ordered operations and final
    # ordinary restores. It never reruns/revalidates all ancestor binders.
    while pending:
        path, mode = pending.popleft()
        if (path, mode) in visited:
            continue
        visited.add((path, mode))
        if path in boundaries:
            continue
        for index, edge in grouped.get(path, []):
            pointer = edge["json_pointer"]
            if mode == "foundation":
                # Only the immediate foundation's flattened products and final
                # configuration inputs are read by this invocation of bind.
                if pointer != "/products" and not pointer.startswith("/configurations/"):
                    continue
            if mode == "products":
                if pointer == "/predecessor":
                    continue
            required.add(index)
            if edge["schema"] == "leaf":
                continue
            target = edge["input"]["path"]
            child_mode = edge["schema"]
            if mode == "root" and pointer == "/foundation":
                child_mode = "foundation"
            # Foundation receipt hashes and outputs are checked, but their paid
            # predecessor operations belong to completed earlier feature scopes.
            if mode == "foundation" and pointer.startswith("/configurations/"):
                continue
            pending.append((target, child_mode))
    return required


def trace_space_references(reference, root=None, configuration=None):
    """Trace declared metadata and hash-check exact originals or named retained aliases.

    @param reference Bound receipt path or exact path/SHA256 reference.
    @param root Optional explicit source checkout; all reads remain contained.
    @param configuration Optional Debug/RelWithDebInfo history scope.
    @return Structured graph with honest errors; no source/fixture/output writes occur.
    """
    root = repository_root(root)
    original = {"path": str(reference)} if isinstance(reference, (str, Path)) else dict(reference)
    cache, documents, aliases, alias_authorities = {}, {}, {}, {}

    def read_identity(path):
        """Reuse only metadata-stable identities during this invocation.

        @param path Exact admitted regular file.
        @return Full verified SHA256 and file length.
        """
        metadata = path.stat()
        signature = (metadata.st_dev, metadata.st_ino, metadata.st_size, metadata.st_mtime_ns, metadata.st_ctime_ns)
        old = cache.get(path)
        # Once observed, a source belongs to this invocation's fixed epoch;
        # replacing a cached identity would mix historical facts from two epochs.
        if old is not None:
            if old[0] != signature:
                raise ValueError(f"input changed during graph traversal: {path}")
            return old[1]
        value = file_identity(path)
        cache[path] = (signature, value)
        return value

    def read_document(identity):
        """Load only an explicitly typed, exact JSON metadata authority.

        @param identity Path/SHA256 record supplied by the owning parent field.
        @return Parsed document after full-byte identity verification.
        """
        path = checked_path(identity["path"], root)
        digest = read_identity(path)[0]
        if digest != identity["sha256"]:
            raise ValueError(f"metadata hash mismatch: {identity['path']}")
        key = (identity["path"], digest)
        if key not in documents:
            documents[key] = json.loads(path.read_text(encoding="utf-8"))
        return documents[key]

    origin_path = checked_path(original["path"], root)
    original["sha256"] = original.get("sha256") or read_identity(origin_path)[0]
    initial = read_document(original)
    errors, edges, visited, adjacency = [], [], set(), {}
    metadata = deque([(original, "bound")])
    # First discover typed metadata and alias declarations. Only then judge
    # historical source bytes, since an authoritative epoch can appear later.
    while metadata:
        identity, kind = metadata.popleft()
        key = (identity["path"], identity["sha256"], kind)
        if key in visited:
            continue
        visited.add(key)
        adjacency[key] = []
        try:
            document = read_document(identity)
            selected = reference_edges(document, kind, configuration)
            if kind in {"products", "epoch"}:
                for product in document["products"]:
                    # Both archive encodings express the same logical-source to
                    # retained-byte authority; neither grants a fresh native pass.
                    if kind == "products" or isinstance(product["source"], dict):
                        source, retained = product["source"], product["retained"]
                    else:
                        source = dict(path=product["source"], sha256=product["sha256"])
                        retained = {field: product[field] for field in ("path", "sha256")}
                    alias_key = (source["path"], source["sha256"])
                    canonical = retained["path"].casefold()
                    # A retained filename cannot testify to two different epochs.
                    if canonical in alias_authorities and alias_authorities[canonical] != retained["sha256"]:
                        raise ValueError(f"conflicting retained alias: {retained['path']}")
                    alias_authorities[canonical] = retained["sha256"]
                    aliases.setdefault(alias_key, {})[retained["path"]] = retained
            for pointer, target, child, role in selected:
                edge = dict(consumer=READERS[kind], document=identity["path"],
                    json_pointer=pointer, input=target, role=role, schema=child,
                    required_for_continuation=False, required_for_current_validation=False)
                edges.append(edge)
                if child != "leaf":
                    child_key = (target["path"], target["sha256"], child)
                    adjacency[key].append(child_key)
                    metadata.append((target, child))
        except (ValueError, KeyError, OSError) as error:
            errors.append(dict(document=identity["path"], schema=kind, error=str(error)))
    # Every original edge is checked independently. Archived aliases can certify
    # old facts, never a changed current save or a freshly linked executable.
    observations = {}
    for edge in edges:
        target = edge["input"]
        key = (target["path"], target["sha256"])
        if key not in observations:
            try:
                row = classify_reference(target, root, role=edge["role"],
                    consumers=(edge["consumer"],), identity_reader=read_identity)
                observed = dict(verification=row.verification, actual_sha256=row.actual_sha256,
                    original_path=row.original_path, retained_aliases=[])
                if row.verification in {"changed", "missing"}:
                    for retained in aliases.get(key, {}).values():
                        archive = classify_reference(retained, root, role="historical_provenance", identity_reader=read_identity)
                        if archive.verification != "present":
                            raise ValueError(f"retained alias is {archive.verification}: {retained['path']}")
                        observed["retained_aliases"].append(retained)
                    if observed["retained_aliases"]:
                        observed["verification"] = "archived_verified"
                observations[key] = observed
            except (ValueError, OSError) as error:
                observations[key] = dict(verification="invalid", error=str(error))
        edge.update(observations[key])
        if edge["verification"] not in {"present", "archived_verified"}:
            errors.append(dict(document=edge["document"], json_pointer=edge["json_pointer"],
                input=target, error=edge["verification"]))
    cycles = graph_cycles(adjacency)
    required = current_validation_edges(original, initial, edges, documents)
    required_documents = {original["path"]}
    # Each required typed edge names the exact metadata file whose absence must
    # fail the current validation, even when parsing that file produced no edges.
    for index in required:
        edge = edges[index]
        edge["required_for_current_validation"] = True
        if edge["schema"] != "leaf":
            required_documents.add(edge["input"]["path"])
    current_errors, historical_issues = [], []
    required_fields = {(edges[index]["document"], edges[index]["json_pointer"]) for index in required}
    # Historical source mismatches remain visible, but only the actual current
    # reader's requirements can block sealing this already completed feature scope.
    for error in errors:
        if "json_pointer" in error:
            selected = (error["document"], error["json_pointer"]) in required_fields
        else:
            selected = error.get("document") in required_documents
        (current_errors if selected else historical_issues).append(error)
    errors = current_errors
    checkpoints = []
    # Preserve independently recorded current expectations with the exact pair;
    # deriving an expected state from a later changed producer is forbidden.
    for row in initial["configurations"]:
        checkpoints.append({key: row[key] for key in ("configuration", "receipt", "output", "observation", "ordinary") if key in row})
        for edge in edges:
            if edge["input"] == row["output"]:
                edge.update(role="continuation_save", required_for_continuation=True)
                if edge["verification"] != "present":
                    errors.append(dict(input=edge["input"], error="current checkpoint requires exact current bytes"))
    # Recheck each identity's metadata at close; source motion invalidates the
    # invocation instead of blessing a mix of observations from different epochs.
    for path, (signature, _) in cache.items():
        try:
            metadata = path.stat()
            if signature != (metadata.st_dev, metadata.st_ino, metadata.st_size, metadata.st_mtime_ns, metadata.st_ctime_ns):
                errors.append(dict(path=str(path), error="input changed before graph close"))
        except OSError as error:
            errors.append(dict(path=str(path), error=str(error)))
    return dict(_documentation=__doc__, schema_version=1, root=original, configuration=configuration,
        complete=not errors and not cycles, metadata_documents=len(visited), distinct_identities=len(observations),
        distinct_files_hashed=len(cache), edges=edges, errors=errors, historical_issues=historical_issues,
        current_validation_edges=len(required), cycles=cycles,
        checkpoints=checkpoints, historical_versions=initial.get("historical_versions", []),
        observer="src/tests/multilayer/acceptance/surface_constructor_retirement_ledger.py:observe",
        gameplay_ticks_executed=0)


def main():
    """Publish a disposable metadata-only Space trace, never an acceptance replay.

    @return None; exit nonzero for missing/corrupt/unsupported required references.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", default="logs/surface-entry-construction-cash-verification.json")
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    root = repository_root()
    result = trace_space_references(args.input, root, args.configuration)
    output = contained(checked_path(args.output, root), owned_root(root, "test-cache"))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: result[key] for key in ("complete", "metadata_documents", "distinct_identities", "distinct_files_hashed", "gameplay_ticks_executed")}))
    print("errors", len(result["errors"]), "cycles", len(result["cycles"]))
    raise SystemExit(0 if result["complete"] else 1)


if __name__ == "__main__":
    main()
