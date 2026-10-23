"""Focused prior-stage semantic/save selection and diagnostic-removal checks.

@file prior_stage_evidence_check.py Transport fixtures do not claim native execution.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group21.
@see docs/zz-reach2/architecture/archi-historical-debug.md.
"""
import json
from pathlib import Path
import tempfile
import unittest

from artifact_paths import RunContext
from prior_stage_evidence import load_stage, prior_save, publish_stage, semantic_text, stage_path

ROOT = Path(__file__).resolve().parents[4]


class PriorStageEvidenceChecks(unittest.TestCase):
    """Preserve explicit prior selection and real saved identities after diagnostics disappear."""

    def setUp(self):
        """Create an isolated repository-shaped transport fixture inside project test-cache.

        @param self Current compact semantic transport test owner.
        """
        temporary = tempfile.TemporaryDirectory(prefix="prior-stage-", dir=ROOT / "test-cache/cleanup")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        fixtures = self.root / "src/tests/fixtures"
        fixtures.mkdir(parents=True)
        (self.root / "CMakeLists.txt").write_text("# @see archi-historical-debug.md; isolated transport fixture.\n")
        self.source = fixtures / "original.sav"
        self.original = (ROOT / "src/tests/fixtures/state/r2mlvs14-b-native-new.sav").read_bytes()
        self.source.write_bytes(self.original)
        self.producer = self.root / "src/producer.py"
        self.producer.write_text('"""@see docs/zz-reach2/architecture/archi-historical-debug.md; transport fixture."""\n')
        self.application = self.root / "fixture.exe"
        self.application.write_bytes(b"transport identity, never executed")
        self.context = RunContext(self.root, "state", "metadata", "explicit-prior")
        self.context.reports.mkdir(parents=True)
        self.context.saves.mkdir(parents=True)
        self.output = self.context.saves / "accepted.sav"
        self.output.write_bytes(self.original)
        self.name = "metadata-case.log"
        publish_stage(self.context, self.name, ["ACTUAL_FIXTURE_BOUNDARY accepted"], "Debug", self.application,
                      [self.producer], [self.source], {self.output.name: self.output, "refused.sav": None})

    def test_semantics_survive_diagnostic_and_executable_removal(self):
        """Keep exact prior save/observations without retaining diagnostic logs or old images.

        @param self Diagnostic-independent continuation test owner.
        """
        self.context.logs.mkdir(parents=True)
        log = self.context.logs / self.name
        log.write_text("optional debug chatter")
        log.unlink()
        self.application.unlink()
        self.assertEqual(semantic_text(self.context, "explicit-prior", self.name, "Debug"), "ACTUAL_FIXTURE_BOUNDARY accepted")
        self.assertEqual(prior_save(self.context, "explicit-prior", self.name, "accepted.sav", "Debug"), self.output)
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_missing_prior_and_wrong_run_refuse(self):
        """Never guess another completed invocation or reconstruct success from a save filename.

        @param self Missing prior-stage selection refusal owner.
        """
        with self.assertRaisesRegex(ValueError, "semantics missing"):
            load_stage(self.context, "unselected-prior", self.name, "Debug")
        document = json.loads(stage_path(self.context, self.name).read_text())
        document["run_id"] = "other-run"
        stage_path(self.context, self.name).write_text(json.dumps(document))
        with self.assertRaisesRegex(ValueError, "scope disagrees"):
            load_stage(self.context, "explicit-prior", self.name, "Debug")

    def test_wrong_configuration_or_missing_source_epoch_refuses(self):
        """Preserve independent image/source roles instead of accepting a merely complete flag.

        @param self Configuration/source identity refusal owner.
        """
        with self.assertRaisesRegex(ValueError, "scope disagrees"):
            load_stage(self.context, "explicit-prior", self.name, "RelWithDebInfo")
        document = json.loads(stage_path(self.context, self.name).read_text())
        document["source_epoch"] = []
        stage_path(self.context, self.name).write_text(json.dumps(document))
        with self.assertRaisesRegex(ValueError, "scope disagrees"):
            load_stage(self.context, "explicit-prior", self.name, "Debug")

    def test_changed_save_or_unknown_output_role_refuses(self):
        """Saved output bytes and declared role identity both remain prerequisites.

        @param self Saved continuation identity refusal owner.
        """
        with self.assertRaisesRegex(ValueError, "saved-output role"):
            prior_save(self.context, "explicit-prior", self.name, "missing.sav", "Debug")
        self.output.write_bytes(b"changed actual saved bytes")
        with self.assertRaisesRegex(ValueError, "identity changed"):
            load_stage(self.context, "explicit-prior", self.name, "Debug")

    def test_refused_case_cannot_acquire_a_save_later(self):
        """Keep refusals explicit and fail if a stale/new writer violates their absent-save role.

        @param self Refused output preservation test owner.
        """
        self.assertIsNone(prior_save(self.context, "explicit-prior", self.name, "refused.sav", "Debug"))
        (self.context.saves / "refused.sav").write_bytes(self.original)
        with self.assertRaisesRegex(ValueError, "refused case"):
            load_stage(self.context, "explicit-prior", self.name, "Debug")

    def test_path_traversal_refuses(self):
        """Keep the selected invocation and case basename inside their original owner.

        @param self Prior-stage path containment refusal owner.
        """
        with self.assertRaises(ValueError):
            load_stage(self.context, "../other", self.name, "Debug")
        with self.assertRaises(ValueError):
            stage_path(self.context, "../case.log")


if __name__ == "__main__":
    unittest.main()
