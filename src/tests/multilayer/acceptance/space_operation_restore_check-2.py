"""Bounded adversarial current/completed reload membership checks.

@file space_operation_restore_check-2.py Synthetic process transport only.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group36.
Separate from export checks so each focused runner stays below two minutes.
"""

import json
import unittest

from space_checkpoint import load_foundation, verify_foundation
from space_operation_restore_fixture import RestoreFixture


class OperationRestoreRefusalChecks(unittest.TestCase):
    """Protect the actual input and complete copied ordinary-witness closure.

    @see r2fc2-file-cleanup3.md Group36 version-two operation witnesses.
    """

    def test_missing_actual_input_membership_refuses(self):
        """Require the ordinary child to declare this exact native output as its input.

        @param self Active repository-local synthetic transport test.
        """
        fixture = RestoreFixture(self)
        document = fixture.documents["Debug"]
        path = fixture.root / document["native_process"]["path"]
        process = json.loads(path.read_text(encoding="utf-8"))
        process["inputs"] = []
        document["native_process"] = fixture.operation.base.write(path, process)
        fixture.rewrite_ordinary("Debug")
        with self.assertRaisesRegex(ValueError, "process/input/image membership"):
            fixture.publish()

    def test_completed_version_two_cannot_drop_ordinary_closure(self):
        """Refuse a downgraded copied receipt that removes a required ordinary witness.

        @param self Active repository-local synthetic transport test.
        """
        fixture = RestoreFixture(self)
        successor = fixture.publish()
        document = json.loads(successor.manifest_path.read_text(encoding="utf-8"))
        document["configurations"]["Debug"]["witnesses"].pop("ordinary_output")
        successor.manifest_path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
        modified = load_foundation(successor.manifest_path, root=fixture.root)
        with self.assertRaisesRegex(ValueError, "closure"):
            verify_foundation(modified)


if __name__ == "__main__":
    unittest.main(verbosity=2)
