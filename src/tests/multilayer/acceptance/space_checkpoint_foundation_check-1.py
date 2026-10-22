"""Bounded publication/history qualification tests; no native game execution.

@file space_checkpoint_foundation_check-1.py Completed witness/current-image role checks.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group12.
"""

import json
from pathlib import Path
import unittest
from unittest.mock import patch

from space_checkpoint import FoundationKind, load_foundation, verify_foundation
from space_checkpoint_foundation_fixture import FoundationFixture
import space_checkpoint_foundation as owner


class FoundationPublicationChecks(unittest.TestCase):
    """Verify typed publication and completed witness roles with independent saves.

    @param self Active isolated synthetic transport test, never actual qualification.
    """

    def setUp(self):
        """Build both synthetic result sets inside repository-local scratch.

        @param self Active test instance.
        @return None; the parent fixture owns only its allocated temporary root.
        """
        self.fixture = FoundationFixture(self)

    def test_qualification_publishes_both_sources_and_no_paid_permission(self):
        """A typed paired foundation preserves exact originals and consumed scopes.

        @param self Active isolated test.
        @return None; all successful transport is explicitly synthetic test data.
        """
        foundation = self.fixture.qualify()
        self.assertEqual(foundation.foundation_type, FoundationKind.SEALED_CHECKPOINT)
        with patch.object(Path, "mkdir", side_effect=AssertionError("verification writes")), \
                patch.object(Path, "write_text", side_effect=AssertionError("verification writes")), \
                patch.object(Path, "write_bytes", side_effect=AssertionError("verification writes")), \
                patch("subprocess.Popen", side_effect=AssertionError("verification launches process")):
            facts = verify_foundation(load_foundation(foundation.manifest_path, "Debug", self.fixture.root))
        self.assertFalse(facts["authorization_granted"])
        self.assertEqual(facts["next_task"], "Plan11/15CSGZVEEI")
        self.assertEqual(len({row["source"]["sha256"] for row in facts["configurations"].values()}), 2)

    def test_completed_foundation_does_not_require_old_exes_or_live_source_epoch(self):
        """A later build/source edit cannot turn completed qualification into an EXE archive.

        @param self Active isolated test.
        @return None; only copied small witnesses and original saves remain required.
        """
        foundation = self.fixture.qualify()
        references = []
        for configuration in owner.CONFIGURATIONS:
            build = owner.read(self.fixture.build_results[configuration], self.fixture.root)
            ordinary = owner.read(self.fixture.ordinary_results[configuration], self.fixture.root)
            references.extend([*build["targets"].values(), ordinary["console_application"]])
        for relative in {reference["path"] for reference in references} | set(owner.QUALIFICATION_SOURCES):
            (self.fixture.root / relative).unlink()
        facts = verify_foundation(foundation)
        self.assertFalse(facts["historical_images_required"])
        self.assertEqual(set(facts["configurations"]), set(owner.CONFIGURATIONS))

    def test_tampered_or_missing_copied_witness_cannot_verify(self):
        """Completed identity facts still require their actual compact physical witnesses.

        @param self Active isolated test.
        @return None; historical classification never conceals a required missing file.
        """
        foundation = self.fixture.qualify()
        document = json.loads(foundation.manifest_path.read_text(encoding="utf-8"))
        reference = document["configurations"]["Debug"]["witnesses"]["native_terminal"]
        path = self.fixture.root / reference["path"]
        original = path.read_bytes()
        path.write_bytes(original + b"tampered")
        with self.assertRaisesRegex(ValueError, "input changed"):
            verify_foundation(foundation)
        path.write_bytes(original)
        path.unlink()
        with self.assertRaises(FileNotFoundError):
            verify_foundation(foundation)


if __name__ == "__main__":
    unittest.main(verbosity=2)
