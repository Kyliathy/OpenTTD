"""Read-only consumer and retained-source audit for r2fc-file-cleanup3.md Group5.

@file capture_consumers.py Disposable execution aid; no imports of acceptance drivers.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups5-6/9/21-22.
"""

import ast
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_inventory import checked_path, file_identity


def python_reads_writes(path):
    """Classify actual Python I/O calls and their assembled path expressions.

    @param path Explicit source module; parsing never executes import-time contexts.
    @return Reader/writer/discovery calls with line, expression and bounded name ancestry.
    """
    tree = ast.parse(path.read_text(encoding="utf-8-sig"))
    assignments = {}
    # Path construction commonly spans ROOT/constants/local variables. Follow
    # bounded name ancestry so classification uses actual I/O, not a filename hit.
    for node in ast.walk(tree):
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    assignments.setdefault(target.id, []).append(node.value)
    calls = []
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call):
            continue
        function = node.func
        method = function.attr if isinstance(function, ast.Attribute) else function.id if isinstance(function, ast.Name) else ""
        if method not in {"read_text", "read_bytes", "write_text", "write_bytes", "open", "mkdir", "exists", "is_file", "copyfile", "copytree", "rename", "replace", "prior_artifact", "resolve_input_path"}:
            continue
        # str.replace(old,new) transforms text; Path.replace(destination) moves
        # a file. Counting text replacement as filesystem mutation corrupts this audit.
        if method == "replace" and len(node.args) != 1:
            continue
        expression = ast.unparse(function.value) if isinstance(function, ast.Attribute) else ast.unparse(node)
        ancestry, names = [expression], set()
        pending = list(ast.walk(node))
        # Stop recursive symbolic construction after six generations; unresolved
        # dynamic selectors remain explicit instead of becoming guessed paths.
        for _depth in range(6):
            discovered = []
            for item in pending:
                if isinstance(item, ast.Name) and item.id not in names:
                    names.add(item.id)
                    for value in assignments.get(item.id, []):
                        ancestry.append(ast.unparse(value))
                        discovered.extend(ast.walk(value))
            pending = discovered
        joined = " ".join(ancestry)
        if not re.search(r"logs|LOGS|CACHE|test-cache|reviews|runtime|CONTEXT|context|prior|build|FIXTURES|fixtures", joined):
            continue
        operation = "read"
        if method in {"write_text", "write_bytes", "mkdir"}:
            operation = "write"
        elif method in {"copyfile", "copytree", "rename", "replace"}:
            operation = "read_write"
        elif method in {"exists", "is_file"}:
            operation = "discovery"
        elif method == "open":
            modes = [arg.value for arg in node.args if isinstance(arg, ast.Constant) and isinstance(arg.value, str)]
            modes.extend(item.value.value for item in node.keywords
                if item.arg == "mode" and isinstance(item.value, ast.Constant) and isinstance(item.value.value, str))
            if any(mode in {"w", "wb", "a", "ab", "x", "xb", "w+", "r+"} for mode in modes):
                operation = "write"
        calls.append(dict(line=node.lineno, operation=operation, method=method,
            expression=expression, path_ancestry=sorted(set(ancestry))))
    return calls


def stale_descriptors():
    """Compare native descriptor source epochs with working bytes and the starting HEAD.

    @return Existing stale source mismatches independently attributed to pre-cleanup HEAD.
    """
    result = []
    # Existing source_hash_policy is UTF8 with universal LF normalization. Use
    # the same independent policy for HEAD so cleanup never receives false blame.
    for name in ("native-mine-geometry.json", "native-processor-geometry.json"):
        descriptor = "media/multilayer/contracts/" + name
        document = json.loads((ROOT / descriptor).read_text(encoding="utf-8"))
        for path, expected in document["native_source_sha256"].items():
            actual = hashlib.sha256((ROOT / path).read_text(encoding="utf-8").encode()).hexdigest()
            if actual == expected:
                continue
            previous = subprocess.check_output(["git", "show", "HEAD:" + path], cwd=ROOT,
                env={**os.environ, "GIT_OPTIONAL_LOCKS": "0"}).decode("utf-8").replace("\r\n", "\n")
            result.append(dict(descriptor=descriptor, path=path, expected_sha256=expected,
                current_sha256=actual, head_sha256=hashlib.sha256(previous.encode()).hexdigest(),
                predates_cleanup=hashlib.sha256(previous.encode()).hexdigest() != expected,
                disposition="owner_led_native_refresh_required"))
    return result


def main():
    """Publish reader semantics, candidate-owner dispositions and source-epoch candidates.

    @return None; observe source and history without executing any native/acceptance entry.
    """
    source_paths = []
    for directory in ("src/tests", "media/multilayer", "mlm/tools", "cmake"):
        source_paths.extend((ROOT / directory).rglob("*.py"))
    raw = []
    for path in sorted(set(source_paths)):
        calls = python_reads_writes(path)
        if calls:
            raw.append(dict(source=path.relative_to(ROOT).as_posix(), calls=calls))
    catalogue = json.loads((ROOT / "mlm/review-producers.json").read_text(encoding="utf-8"))
    producers = []
    # Catalogue rows remain the only feature/test-tag authority. Closed paid
    # Space windows are history; other current owners require bounded fresh recipes.
    for index, row in enumerate(catalogue["producers"]):
        path = checked_path(row["producer"], ROOT)
        if not path.is_file():
            raise ValueError(f"catalogued producer is missing: {row['producer']}")
        producers.append(dict(producer=row["producer"], catalogue_json_pointer=f"/producers/{index}",
            disposition="regenerate_through_original_owner",
            needs_bounded_recipe=True, ordinary_paid_continuation_requires_source_plan=True))
    snapshot = json.loads((ROOT / "test-cache/cleanup/boundary.json").read_text(encoding="utf-8"))
    source_epochs = []
    # Unique old harness/source snapshots may be irreplaceable working material.
    # Preserve them conservatively; only explicit CMake-generated probes/tables are
    # reproducible build products. This is independent of Git ignore/tracking state.
    for relative in snapshot["ignored"]:
        path = ROOT / relative
        if path.suffix.lower() not in {".py", ".cpp", ".h", ".ps1", ".cmake"}:
            continue
        if "__pycache__/" in relative:
            continue
        if not path.is_file():
            continue
        generated = relative.startswith(("build/CMakeFiles/", "build/vcpkg_installed/", "build/src/"))
        digest, length = file_identity(checked_path(relative, ROOT))
        source_epochs.append(dict(path=relative, sha256=digest, bytes=length,
            disposition="regenerate" if generated else "preserve_source_epoch"))
    consumers = [
        dict(reader="multilayer_profile_save_fixture.main/prior_artifact", source="src/tests/multilayer_profile_save_fixture.py", selector="--packages-only --prior-run <exact-id>",
            role="five prior editor/metadata acceptance/refusal logs and exact accepted-save presence", disposition="regenerate_selected_prior_stage_or_retain_minimal_independent_semantic_fixture"),
        dict(reader="geology_save_fixture.main/prior_artifact", source="src/tests/geology_save_fixture.py", selector="--refusals-only --prior-run <exact-id>",
            role="native/reload/partial/partial-reload/legacy accepted logs and saves", disposition="regenerate_selected_prior_stage_or_retain_minimal_independent_semantic_fixture"),
        dict(reader="multilayer_state_fixture.save_path/run_case", source="src/tests/multilayer_state_fixture.py", selector="explicit fixture or reserved same-run output; no latest fallback",
            role="immutable save-format input versus newly generated output", disposition="retain_curated_save_and_regenerate_owned_outputs"),
        dict(reader="native_feedback.main", source="src/tests/multilayer/tbm/native_feedback.py", selector="--reload-accepted --prior-run <exact-id>",
            role="one accepted prior save and FEEDBACK_RESULT line for uninterrupted/reload equality", disposition="run_owned_evidence_and_explicit_prior_stage_preparation"),
        dict(reader="native_motion.main", source="src/tests/multilayer/tbm/native_motion.py", selector="--replay <kind> / --pace-check-layer <layer> --prior-run <exact-id>",
            role="matching native motion/pacing log markers, saved fractions and frame observations", disposition="run_owned_evidence_and_explicit_prior_stage_preparation"),
        dict(reader="pacing_files.verify_migration/main", source="src/tests/multilayer/tbm/pacing_files.py", selector="explicit retained 393/394 rate-migration pairs",
            role="fraction-only schema expectations and malformed-copy preview/startup refusals", disposition="retain_curated_pair_and_regenerate_malformed_copies"),
        dict(reader="phase_pair_index_checks-1.main/read_bound", source="src/tests/multilayer/acceptance/phase_pair_index_checks-1.py", selector="both explicit configurations plus --handling-release/--handling-debug",
            role="paid-probe/descent progress steps and module prior_positive_release/report identities; distinguish genuine paid from seeded handling", disposition="seal_historical_paid_pairs_or_explicitly_prepare_selected_synthetic_fixture_never_replay_paid_work"),
    ]
    dispositions = [dict(owner=owner, disposition=action) for owner, action in (
        ("logs", "seal_required_facts/export_all_saves/preserve_unique_source_epochs_then_regenerate_diagnostics"),
        ("test-cache", "preserve_unique_source_epochs/export_all_saves_then_regenerate_declared_prerequisites"),
        ("mlm/reviews", "export_all_saves/seal_required_facts_then_regenerate_selected_review_outputs"),
        ("mlm/runtime", "export_all_saves/preserve_unique_config/source_epochs_then_recreate_every_profile"),
        ("build", "export_all_saves/preserve_configs_and_portable_requirements_then_rebuild_exact_declared_root"),
        ("OpenTTD-baseline", "retain_existing_minimal_independent_source_archive"),
        ("!kit", "retain_valuable_ignored_distribution_and_legacy_saves"))]
    summary = dict(_documentation=__doc__, schema_version=1, consumers=consumers,
        candidate_owners=dispositions, producer_dispositions=producers, stale_native_descriptors=stale_descriptors(),
        source_epoch_candidates=len(source_epochs), source_epoch_bytes=sum(row["bytes"] for row in source_epochs),
        python_reader_writer_files=len(raw), python_io_calls=sum(len(row["calls"]) for row in raw),
        required_unknown_policy="Unknown save-like/config/original source material remains protected; no disposition here grants deletion.",
        historical_space_issues="Superseded ancestor source/operation schemas and old missing consoles are historical provenance; current reader has zero unresolved required identities.",
        underground_status="Group10 provisional five-model code is not built/tested; preserve both before.json and before-definitions.json epochs; do not resume.")
    (ROOT / "test-cache/cleanup/consumer-reads-writes.json").write_text(json.dumps(dict(_documentation=__doc__, modules=raw), indent=2) + "\n", encoding="utf-8")
    (ROOT / "test-cache/cleanup/source-epoch-candidates.json").write_text(json.dumps(dict(_documentation=__doc__, candidates=source_epochs), indent=2) + "\n", encoding="utf-8")
    (ROOT / "src/tests/fixtures/cleanup/consumer-dispositions.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: summary[key] for key in ("python_reader_writer_files", "python_io_calls", "source_epoch_candidates", "source_epoch_bytes")}))
    print("catalogued owners",len(producers),"pre-existing source mismatches",len(summary["stale_native_descriptors"]))


if __name__ == "__main__":
    main()
