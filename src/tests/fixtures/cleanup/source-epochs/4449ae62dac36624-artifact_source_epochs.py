"""Preserve unique source history while recording reproducible scratch separately.

@file artifact_source_epochs.py Bounded Group5 source-candidate disposition/export.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group9.4.
Immutable archived bytes retain their original documentation/identity. Identical
live source is reused only with a hash-bound reconstruction reference; verification
must refresh changed references from original snapshots before cleanup releases them.
"""

import argparse
import json
import os
from pathlib import Path

from artifact_inventory import checked_path, file_identity
from artifact_paths import repository_root
from artifact_preservation import preserve_file, publish_document

EXTENSIONS = {".py", ".cpp", ".c", ".h", ".hpp", ".cmake", ".ps1"}
MANIFEST = "src/tests/fixtures/cleanup/source-epochs.json"


def live_source_index(root):
    """Index exact source bytes in explicit authored owners, excluding fixtures.

    @param root Checkout whose current source may reconstruct identical snapshots.
    @return SHA256 to one canonical source identity; no generated tree is scanned.
    """
    identities = {}
    files = [root / "CMakeLists.txt", root / "clean.ps1"]
    # Review/runtime/cache output is excluded at the owner boundary. Historical
    # fixture source cannot establish that live authored code is reconstructible.
    for relative in ("src", "media/multilayer", "mlm/tools", "cmake"):
        owner = checked_path(relative, root)
        for parent, directories, names in os.walk(owner, followlinks=False):
            directories[:] = [name for name in directories if name not in ("fixtures", "__pycache__", ".git")]
            for name in names:
                path = Path(parent) / name
                if path.suffix in EXTENSIONS or path.name == "CMakeLists.txt":
                    files.append(path)
    for path in files:
        if not path.is_file():
            continue
        checked_path(path, root)
        digest = file_identity(path)[0]
        identities.setdefault(digest, dict(path=path.relative_to(root).as_posix(), sha256=digest))
    return identities


def preserve_source_epochs(candidates, root, current=None):
    """Retain irrecoverable source epochs and explain generated/live equivalents.

    @param candidates Explicit Group5 path/hash/disposition candidate rows.
    @param root Checkout retaining the original snapshots until cleanup verifies.
    @param current Optional explicit hash-bound live-source index for focused tests.
    @return Durable source history contract, with every candidate accounted for.
    """
    canonical = live_source_index(root) if current is None else current
    payloads = {}
    generated = []
    # Distinct historical versions cannot be inferred from filenames or current
    # HEAD. Preserve unique bytes once; every old snapshot spelling remains an alias.
    for row in candidates:
        source = checked_path(row["path"], root)
        digest = row["sha256"]
        # A familiar CMake filename is disposable only inside a declared build;
        # an authored template with the same name must retain its source bytes.
        cmake_generated = row["path"].startswith(("build/", "logs/proofs/mlm/clean-checkout/build/")) and source.name in {"CPackConfig.cmake", "CPackProperties.cmake", "CPackSourceConfig.cmake", "CTestTestfile.cmake"}
        # The inventory's explicit compiler/dependency/generated classifications
        # and CMake's own configuration outputs are rebuild inputs, not authored
        # source epochs. Preserve their identities as non-dereferenced facts.
        if row["disposition"] == "regenerate" or cmake_generated:
            generated.append(dict(original=dict(path=row["path"], sha256=digest),
                                  generator="repository CMake/vcpkg/tool bootstrap; r2fc-file-cleanup3.md Group13"))
            continue
        if file_identity(source)[0] != digest:
            raise ValueError(f"original source epoch changed: {row['path']}")
        if digest in payloads:
            payloads[digest]["original_paths"].append(row["path"])
            continue
        if digest in canonical:
            retained = canonical[digest]
            if file_identity(checked_path(retained["path"], root))[0] != digest:
                raise ValueError("live source reconstruction identity changed")
            disposition = "reconstruct_from_identical_live_source"
        else:
            destination = f"src/tests/fixtures/cleanup/source-epochs/{digest[:16]}-{source.name}"
            retained = preserve_file(source, destination, digest, root)
            disposition = "retained_unique_historical_source"
        payloads[digest] = dict(sha256=digest, original_paths=[row["path"]], reference=retained,
                               disposition=disposition, bytes=row["bytes"])
    result = dict(_documentation="Owner: r2fc-file-cleanup3.md9; unique source history, not an executable or generated review archive. Verify live reconstruction references again before cleanup.",
        schema_version=1, kind="source_epoch_preservation", payloads=list(payloads.values()),
        generated=generated, original_candidates=len(candidates), no_executable_archive=True)
    verify_source_epochs(result, root)
    return result


def verify_source_epochs(document, root):
    """Verify every durable historical source/reconstruction reference read-only.

    @param document Explicit published source epoch contract.
    @param root Current checkout; no original ignored paths are required here.
    @return Number of unique protected source payloads after exact hash checks.
    """
    for row in document["payloads"]:
        reference = row["reference"]
        if reference["sha256"] != row["sha256"] or file_identity(checked_path(reference["path"], root))[0] != row["sha256"]:
            raise ValueError(f"source epoch reference changed; refresh before cleanup: {reference['path']}")
    count = sum(len(row["original_paths"]) for row in document["payloads"]) + len(document["generated"])
    if count != document["original_candidates"]:
        raise ValueError("source epoch disposition coverage is incomplete")
    return len(document["payloads"])


def seal_underground_source_checkpoint(root):
    """Seal the paused provisional source independently of historical qualification.

    @param root Checkout whose Group10 fleet implementation remains unqualified.
    @return Durable paused source checkpoint with two distinct original epochs.
    """
    owner = "src/tests/fixtures/checkpoints/underground"
    history_path = checked_path(f"{owner}/history.json", root)
    history = json.loads(history_path.read_text(encoding="utf-8"))
    provisional = []
    # These are the actual incomplete source owners named by the paused handover.
    # Copy them exactly before later cleanup/CMake changes advance live source.
    for original in ("src/transport_tube_fleet.h", "src/transport_tube_fleet.cpp", "src/transport_tube_engine.cpp", "src/CMakeLists.txt"):
        source = checked_path(original, root)
        digest = file_identity(source)[0]
        retained = preserve_file(source, f"{owner}/provisional/{original}", digest, root)
        provisional.append(dict(original=dict(path=original, sha256=digest), retained=retained))
    # Every explicit live historical source/terminal/epoch must still exist.
    # Nested EXE/image strings remain historical identities, never retained inputs.
    for witness in history["live_retained_inputs"]:
        if file_identity(checked_path(witness["retained"]["path"], root))[0] != witness["retained"]["sha256"]:
            raise ValueError("underground historical witness changed")
    history["source_preservation_pending"] = False
    publish_document(history_path, history, root)
    document = dict(_documentation="Owner: r2fc-file-cleanup3.md9/r2mlvs17; paused source checkpoint; partial five-model code has not been built or tested.",
        schema_version=1, kind="paused_underground_source_checkpoint",
        history=dict(path=history_path.relative_to(root).as_posix(), sha256=file_identity(history_path)[0]),
        source_epochs=history["source_epochs"], provisional_sources=provisional,
        status=history["status"], fresh_build_qualified=False, feature_work_paused=True,
        no_executable_archive=True, live_path_policy="Only history.live_retained_inputs.retained and provisional_sources.retained are required physical history; nested old paths remain facts.")
    publish_document(f"{owner}/checkpoint.json", document, root)
    return document


def main():
    """Verify by default; explicitly preserve selected inventoried source versions.

    @return None; prints dispositions without building or resuming a feature.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--export", action="store_true")
    parser.add_argument("--candidates", type=Path)
    args = parser.parse_args()
    root = repository_root()
    if args.export:
        if args.candidates is not None:
            candidates = json.loads(checked_path(args.candidates, root).read_text(encoding="utf-8"))["candidates"]
        else:
            old = json.loads((root / MANIFEST).read_text(encoding="utf-8"))
            candidates = [dict(path=original, sha256=row["sha256"], bytes=row["bytes"], disposition="preserve_source_epoch")
                          for row in old["payloads"] for original in row["original_paths"]]
            candidates.extend(dict(path=row["original"]["path"], sha256=row["original"]["sha256"], bytes=0, disposition="regenerate")
                              for row in old["generated"])
        result = preserve_source_epochs(candidates, root)
        publish_document(MANIFEST, result, root)
        # The paused source checkpoint is immutable once published; refreshing
        # unrelated live reconstruction references must not recapture its epoch.
        if not (root / "src/tests/fixtures/checkpoints/underground/checkpoint.json").exists():
            seal_underground_source_checkpoint(root)
    else:
        result = json.loads((root / MANIFEST).read_text(encoding="utf-8"))
        verify_source_epochs(result, root)
    print(json.dumps(dict(unique_source_payloads=len(result["payloads"]), generated_candidates=len(result["generated"]),
        retained_unique=sum(row["disposition"] == "retained_unique_historical_source" for row in result["payloads"]),
        reconstruct_from_live=sum(row["disposition"] == "reconstruct_from_identical_live_source" for row in result["payloads"])), indent=2))


if __name__ == "__main__":
    main()
