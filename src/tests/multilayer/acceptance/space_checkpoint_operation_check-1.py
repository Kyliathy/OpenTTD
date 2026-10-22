"""Bounded successor publication and explicit continuation selection checks.

@file space_checkpoint_operation_check-1.py Synthetic transport, independent original saves.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group12B.
"""

import json
from pathlib import Path
import unittest
from unittest.mock import patch

from space_checkpoint import FoundationKind, load_foundation, verify_foundation
from space_checkpoint_operation_fixture import OperationFixture
from space_continuation import select_continuation
import space_checkpoint_foundation as owner


class OperationPublicationChecks(unittest.TestCase):
    """Exercise exact completed/current role boundaries without running an image.

    @see r2fc-file-cleanup3.md Group12B synthetic publication contract.
    """

    def test_successor_selects_owned_output_without_old_executables(self):
        """Verify paired publication, explicit selection and completed witness tamper refusal.

        @param self Active repository-local test fixture.
        @return None; synthetic records never qualify actual gameplay or native images.
        """
        fixture = OperationFixture(self)
        successor = fixture.publish()
        self.assertEqual(successor.foundation_type, FoundationKind.OPERATION_RECEIPT)
        document = json.loads(successor.manifest_path.read_text(encoding="utf-8"))
        # Current operations have already closed their image/source checks.
        # Subsequent verification must keep bounded witnesses instead of EXEs.
        for configuration in owner.CONFIGURATIONS:
            build = owner.read(fixture.base.build_results[configuration], fixture.root)
            for reference in build["targets"].values():
                (fixture.root / reference["path"]).unlink()
        for relative in owner.QUALIFICATION_SOURCES:
            (fixture.root / relative).unlink()
        with patch("subprocess.Popen", side_effect=AssertionError("selection launched process")), \
                patch.object(Path, "mkdir", side_effect=AssertionError("selection wrote")), \
                patch.object(Path, "write_text", side_effect=AssertionError("selection wrote")):
            facts = verify_foundation(successor)
            selected = select_continuation(successor, "Debug", document["configurations"]["Debug"]["output"]["path"])
        self.assertFalse(facts["authorization_granted"])
        self.assertEqual(selected.source_identity, document["configurations"]["Debug"]["output"])
        typed_debug = load_foundation(successor.manifest_path, "Debug", fixture.root)
        with self.assertRaisesRegex(ValueError, "configuration"):
            select_continuation(typed_debug, "RelWithDebInfo", selected.source)
        with self.assertRaisesRegex(ValueError, "differs"):
            select_continuation(successor, "Debug", document["configurations"]["Debug"]["source"]["path"])
        with self.assertRaisesRegex(ValueError, "cycle"):
            owner.verify_foundation(successor, _seen={successor.manifest_path})
        terminal = fixture.root / document["configurations"]["Debug"]["witnesses"]["native_terminal"]["path"]
        terminal.write_bytes(terminal.read_bytes() + b"tampered")
        with self.assertRaisesRegex(ValueError, "input changed"):
            verify_foundation(successor)


if __name__ == "__main__":
    unittest.main(verbosity=2)
