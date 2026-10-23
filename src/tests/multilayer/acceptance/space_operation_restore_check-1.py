"""Focused current/completed ordinary witness boundaries for typed operations.

@file space_operation_restore_check-1.py Synthetic process transport only.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group36.
"""

import json
from pathlib import Path
import unittest
from unittest.mock import patch

from space_checkpoint import ROOT, verify_foundation, export_foundation
from space_operation_restore_fixture import RestoreFixture


class OperationRestoreChecks(unittest.TestCase):
    """Require actual selected reload roles without retaining old images forever.

    @see r2fc2-file-cleanup3.md Group36 version-two operation witnesses.
    """

    def test_exported_reload_verifies_without_images_or_processes(self):
        """Keep copied ordinary saves/transcripts authoritative after disposable images retire.

        @param self Active repository-local synthetic transport test.
        """
        fixture = RestoreFixture(self)
        successor = fixture.publish()
        retained = export_foundation(successor)
        self.assertTrue(fixture.root.is_relative_to(ROOT / "test-cache"))
        # Only this case's tiny synthetic images are deleted; completed receipt
        # verification must neither require them nor launch/write another run.
        for image in fixture.root.rglob("*.exe"):
            image.unlink()
        with patch("subprocess.Popen", side_effect=AssertionError("verification launched process")), \
                patch.object(Path, "mkdir", side_effect=AssertionError("verification wrote")), \
                patch.object(Path, "write_text", side_effect=AssertionError("verification wrote")):
            facts = verify_foundation(retained)
        self.assertFalse(facts["historical_images_required"])
        document = json.loads(retained.manifest_path.read_text(encoding="utf-8"))
        self.assertIn("ordinary_output", document["configurations"]["Debug"]["witnesses"])

    def test_borrowed_other_world_reload_refuses(self):
        """Refuse a successful-looking ordinary process selected from the other world.

        @param self Active repository-local synthetic transport test.
        """
        fixture = RestoreFixture(self)
        fixture.documents["Debug"]["source"] = fixture.operation.documents["RelWithDebInfo"]["output"]
        fixture.rewrite_ordinary("Debug")
        with self.assertRaisesRegex(ValueError, "different task/save/configuration"):
            fixture.publish()

if __name__ == "__main__":
    unittest.main(verbosity=2)
