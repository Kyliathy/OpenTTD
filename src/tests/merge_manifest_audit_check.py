"""Keep binary preservation distinct from normalized text-source equivalence.

@file merge_manifest_audit_check.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/m2o23-4-come-space.md Group59.
Identity-only fixtures persist under test-cache/merge-build; no cleanup deletes
originals and no image/save decoder or candidate-derived expectation is used.
"""
from pathlib import Path
import unittest
import uuid

import merge_manifest_audit as audit


class ManifestEquivalenceBoundary(unittest.TestCase):
    """Reject binary corruption that merely resembles a text line-ending change."""

    def fixture(self, suffix):
        """Prepare independently authored old/current bytes in a persistent isolated root.

        @param self Active preservation boundary case.
        @param suffix Explicit text or binary identity-only fixture extension.
        @return Original manifest row and its isolated root.
        """
        root = audit.ROOT / "test-cache/merge-build" / ("manifest-" + uuid.uuid4().hex)
        root.mkdir(parents=True)
        original = root / "retained.dat"
        original.write_bytes(b"/** Identity-only fixture; @see archi-historical-debug.md. */\r\n")
        current = root / ("current" + suffix)
        current.write_bytes(b"/** Identity-only fixture; @see archi-historical-debug.md. */\n")
        reference = audit.identify(original, root)
        row = dict(path=current.name, decision="semantic_deferred", owner_groups=[59],
                   retained=dict(destination=original.name, sha256=reference["sha256"], bytes=reference["bytes"]))
        return row, root

    def test_binary_line_ending_difference_is_not_equivalence(self):
        """Preserve every binary byte even when a replacement resembles newline conversion.

        @param self Active binary original/refusal case.
        """
        row, root = self.fixture(".bin")
        result = audit.disposition(row, root)
        self.assertEqual(result["status"], "explicitly_unresolved")
        self.assertNotEqual(result["retained"]["sha256"], result["current"]["sha256"])

    def test_text_line_ending_difference_remains_equivalent(self):
        """Retain the explicit reviewed Git text policy independently of raw file hashes.

        @param self Active text-source equivalence case.
        """
        row, root = self.fixture(".cpp")
        result = audit.disposition(row, root)
        self.assertEqual(result["status"], "equivalent")
        self.assertNotEqual(result["retained"]["sha256"], result["current"]["sha256"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
