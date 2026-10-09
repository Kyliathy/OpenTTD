"""Explicit, content-bound evidence selection for multi-layer art validators.

@file evidence.py Artifact ownership only; sprite identity stays in the public contract.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Groups 6-10.
"""

import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "logs/proofs/mlm/native-art"
DEFAULT_MANIFEST = CACHE / "evidence.json"
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import contained, identifier


def identity(path):
    """Bind an existing repository artifact to its exact bytes.

    @param path Concrete input or generated evidence selected by a producer.
    @return Portable path/hash record; paths outside the checkout are refused.
    """
    path = contained(path, ROOT)
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


class EvidenceSet:
    """One explicitly named proof set containing selected runs and source identities.

    @see r2fc-file-cleanup.md tasks 6.1-6.4; no latest-run or docs fallback.
    """

    def __init__(self, path=DEFAULT_MANIFEST):
        """Read the complete selection manifest before accessing proof products.

        @param path Exact manifest inside the proof cache or a selected review run.
        @return None; absent cache instructs the caller to regenerate real evidence.
        """
        self.path = contained(Path(path) if Path(path).is_absolute() else ROOT / path, ROOT)
        # Evidence selection cannot make a documentation folder a runtime owner.
        # Both admitted roots are generated storage, independent of Git tracking.
        if not any(self.path.is_relative_to(owner) for owner in (CACHE, ROOT / "mlm/reviews")):
            raise ValueError("evidence manifest must belong to logs/proofs/mlm/native-art or an explicit review run")
        if not self.path.is_file():
            raise ValueError("proof cache is absent; run the source and native phases of regenerate_art_proofs.py")
        self.document = json.loads(self.path.read_text(encoding="utf-8"))
        if self.document.get("schema_version") != 1:
            raise ValueError("unsupported evidence selection schema")

    def selected(self, section, name):
        """Resolve a declared artifact and refuse changed or missing bytes.

        @param section Manifest collection, normally reports or logs.
        @param name Semantic identity within that collection.
        @return Existing, hash-verified path; no basename search or fallback occurs.
        """
        record = self.document[section][name]
        path = contained(ROOT / record["path"], ROOT)
        if not any(path.is_relative_to(owner) for owner in (ROOT / "logs", ROOT / "mlm/reviews")):
            raise ValueError("selected proof must belong to generated evidence storage")
        if identity(path) != {key: record[key] for key in ("path", "sha256")}:
            raise AssertionError(f"selected {section}/{name} evidence changed")
        return path

    def log(self, name):
        """Resolve the exact native log selected for a semantic proof obligation.

        @param name Declared native check/capture identity.
        @return Hash-verified repository-relative log path.
        """
        return self.selected("logs", name).relative_to(ROOT).as_posix()

    def report(self, name):
        """Read a declared native capture report and validate its routing provenance.

        @param name Semantic capture identity from the selected evidence set.
        @return Parsed report after exact file, frame and producer validation.
        """
        path = self.selected("reports", name)
        report = json.loads(path.read_text(encoding="utf-8"))
        self.validate_capture(report)
        return report

    def output(self, name):
        """Resolve one explicitly declared generated manifest/gallery destination.

        @param name Output role, such as runtime, packed, sound or gallery.
        @return Path in the selected generated owner; no source file can be replaced.
        """
        path = contained(ROOT / self.document["outputs"][name], ROOT)
        if not any(path.is_relative_to(owner) for owner in (CACHE, ROOT / "mlm/reviews")):
            raise ValueError("proof publication cannot replace source or fixture inputs")
        return path

    def validate_capture(self, report):
        """Refuse stale pixels and undeclared producer or cross-run substitutions.

        @param report Actual native capture record, including explicitly historical records.
        @return None; relocation provenance is never promoted into a new native pass.
        """
        historical = report.get("provenance", {}).get("kind") == "historical_relocation"
        catalogue = json.loads((ROOT / "mlm/review-producers.json").read_text(encoding="utf-8"))
        producers = {row["producer"]: row for row in catalogue["producers"]}
        if not historical:
            producer = producers[report["producer"]]
            run_id = identifier(report["run_id"])
            assignments = {(row["feature"], row["scenario"]) for row in producer["features"]}
        # Every frame carries its actual native feature path. A shared gallery
        # may combine explicitly selected reports, but a report may not borrow
        # similarly named frames from another invocation or undeclared feature.
        for frame in report["frames"]:
            path = contained(ROOT / frame["path"], ROOT)
            assert identity(path)["sha256"] == frame["sha256"], "native frame hash changed"
            if historical:
                assert path.is_relative_to(CACHE), "historical proof is outside its declared cache"
            else:
                parts = path.relative_to(ROOT / "mlm/reviews").parts
                assert len(parts) == 5 and parts[2] == run_id and parts[3] == "images", "frame belongs to another invocation"
                assert (parts[0], parts[1]) in assignments, "frame feature is not declared by its producer"
                assert frame["feature"] == parts[0] and frame["scenario"] == parts[1], "frame routing metadata disagrees"
        source = contained(ROOT / report["source"], ROOT)
        assert identity(source)["sha256"] == report["source_sha256"], "native capture source changed"
        logs = [name for name, record in self.document["logs"].items() if record["path"] == report["log"]]
        assert logs, "capture log was not declared by the selected evidence manifest"
        self.selected("logs", logs[0])
