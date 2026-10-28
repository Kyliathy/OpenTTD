"""Focused, non-destructive checks for the independent donor bootstrap boundary.

@file donor_source_stage_check.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/m2o26-a-execution-notes.md Group 2 (the m2o26 plan itself is in Git history at d49c3596).
Synthetic trees stay under test-cache/merge-oct26/stage-contracts. There is no
recursive teardown, repository creation, Git mutation or real donor modification.
Run: python -B src/tests/multilayer/support/donor_source_stage_check.py
"""

import json
import os
from pathlib import Path
import shutil
import unittest
from unittest.mock import patch
import uuid

from artifact_inventory import file_identity
from artifact_paths import repository_root
from package_transfer import stage_donor_source


class DonorSourceStageTests(unittest.TestCase):
    """Verify exact-byte bootstrap copies and refusal before source-side writes.

    Each test owns a fresh retained synthetic tree; failures never trigger deletion.
    """

    def setUp(self):
        """Create a minimal source closure beneath this project's output owner.

        @param self Current isolated test case and its invocation-owned paths.
        """
        self.root = repository_root()
        self.scratch = self.root / "test-cache/merge-oct26/stage-contracts" / uuid.uuid4().hex
        self.source = self.scratch / "donor"
        self.target = self.scratch / "staged"
        (self.source / "src").mkdir(parents=True)
        self.members = []
        # These three authored marker files model the complete minimal bootstrap
        # closure; no real checkout, executable or Git metadata is constructed.
        for relative, data in (("CMakeLists.txt", b"# synthetic source"),
                               ("src/CMakeLists.txt", b"# synthetic child"),
                               ("vcpkg.json", b"{}")):
            target = self.source / relative
            target.write_bytes(data)
            digest, size = file_identity(target)
            self.members.append(dict(path=relative, sha256=digest, bytes=size))

    def test_exact_independent_copy_without_preservation_or_git(self):
        """Copy the declared closure without invoking source-side recovery audits.

        @param self Fixture containing fresh source and output roots.
        """
        with patch("package_transfer.verify_cleanup_inputs") as audit, \
                patch("package_transfer.apply_save_exports") as export, \
                patch("package_transfer._git") as git:
            result = stage_donor_source(self.target, self.source, self.members, self.root)
        audit.assert_not_called()
        export.assert_not_called()
        git.assert_not_called()
        self.assertEqual(result["files"], 3)
        manifest = json.loads((self.target / ".donor-source-manifest.json").read_text())
        self.assertFalse(manifest["git_metadata_copied"])
        # Modifying the private copy must never modify a donor inode through a
        # hard link, even though both files originally contain identical bytes.
        (self.target / "vcpkg.json").write_bytes(b'{"private":true}')
        self.assertEqual((self.source / "vcpkg.json").read_bytes(), b"{}")

    def test_source_target_and_existing_output_refuse(self):
        """Refuse overlays and a destination inside the read-only donor owner.

        @param self Fixture whose original bytes must remain unchanged.
        """
        with self.assertRaises(ValueError):
            stage_donor_source(self.source / "nested", self.source, self.members, self.root)
        self.target.mkdir()
        with self.assertRaisesRegex(ValueError, "fresh destination"):
            stage_donor_source(self.target, self.source, self.members, self.root)

    def test_escape_metadata_and_case_collision_refuse(self):
        """Reject foreign paths and duplicate identities before publishing output.

        @param self Fixture used only for source-boundary admission.
        """
        # Each malformed spelling could otherwise select a foreign input or
        # smuggle generated evidence into the supposedly original source epoch.
        for spelling in ("../outside", "src/../outside", ".git/config", ".GIT/config", "logs/old.exe", "BUILD/old.exe"):
            with self.subTest(spelling=spelling), self.assertRaises(ValueError):
                bad = dict(self.members[0], path=spelling)
                stage_donor_source(self.target, self.source, [*self.members, bad], self.root)
        duplicate = dict(self.members[0], path="cmakelists.txt")
        with self.assertRaisesRegex(ValueError, "case-colliding"):
            stage_donor_source(self.target, self.source, [*self.members, duplicate], self.root)
        self.assertFalse(self.target.exists())

    def test_hard_linked_input_refuses(self):
        """Refuse shared source inodes before creating an allegedly isolated tree.

        @param self Fixture containing only synthetic files under test-cache.
        """
        os.link(self.source / "vcpkg.json", self.scratch / "shared-config.json")
        with self.assertRaisesRegex(ValueError, "hard-linked donor input"):
            stage_donor_source(self.target, self.source, self.members, self.root)
        self.assertFalse(self.target.exists())

    def test_changed_source_before_copy_refuses(self):
        """Require the source identity observed by the caller, not merely its name.

        @param self Fixture containing a deliberately stale member observation.
        """
        (self.source / "vcpkg.json").write_bytes(b"changed")
        with self.assertRaisesRegex(ValueError, "changed before staging"):
            stage_donor_source(self.target, self.source, self.members, self.root)
        self.assertFalse(self.target.exists())

    def test_change_during_copy_has_no_completion_manifest(self):
        """Leave raced copies incomplete instead of certifying a false source epoch.

        @param self Fixture whose own synthetic source may be changed by the double.
        """
        actual_copy = shutil.copyfileobj

        def change_after_copy(incoming, outgoing):
            """Simulate another writer after exact bytes reached the private copy.

            @param incoming Open synthetic source stream owned by this test.
            @param outgoing Open invocation-owned destination stream.
            """
            actual_copy(incoming, outgoing)
            Path(incoming.name).write_bytes(b"changed during copy")

        with patch("package_transfer.shutil.copyfileobj", side_effect=change_after_copy):
            with self.assertRaisesRegex(ValueError, "changed during staging"):
                stage_donor_source(self.target, self.source, self.members, self.root)
        self.assertFalse((self.target / ".donor-source-manifest.json").exists())


if __name__ == "__main__":
    unittest.main()
