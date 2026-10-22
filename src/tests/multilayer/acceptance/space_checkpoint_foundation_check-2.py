"""Bounded pre-publication refusal checks for typed original qualification.

@file space_checkpoint_foundation_check-2.py Corrupt references/configurations/process/builds.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group12.
"""

import json
from pathlib import Path
import unittest

from space_checkpoint import load_foundation
from space_checkpoint_foundation_fixture import FoundationFixture
import space_checkpoint_foundation as owner


class FoundationRefusalChecks(unittest.TestCase):
    """Refuse partial, wrong or legacy inputs before a selectable foundation exists.

    @param self Active isolated synthetic transport test.
    """

    def setUp(self):
        """Create synthetic result references around independently retained saves.

        @param self Active test instance.
        @return None; no actual application, old paid chain or build runs.
        """
        self.fixture = FoundationFixture(self)

    def test_missing_configuration_and_corrupt_reference_fail_before_publication(self):
        """One successful configuration cannot silently substitute for the other.

        @param self Active isolated test.
        @return None; neither incomplete nor tampered results publish a foundation.
        """
        debug = self.fixture.native_results.pop("Debug")
        with self.assertRaisesRegex(ValueError, "both configurations"):
            self.fixture.qualify()
        self.fixture.native_results["Debug"] = debug
        self.fixture.native_results["RelWithDebInfo"] = dict(self.fixture.native_results["RelWithDebInfo"], sha256="0" * 64)
        with self.assertRaisesRegex(ValueError, "input changed"):
            self.fixture.qualify()
        self.assertFalse(self.fixture.destination.exists())

    def test_failed_build_and_wrong_image_cannot_qualify(self):
        """Metadata-only source/image presence cannot replace successful selected builds.

        @param self Active isolated test.
        @return None; failed stages or a mismatched ordinary target refuse publication.
        """
        reference = self.fixture.build_results["RelWithDebInfo"]
        build = owner.read(reference, self.fixture.root)
        build["steps"][0]["exit_status"] = 1
        self.fixture.build_results["RelWithDebInfo"] = self.fixture.write(self.fixture.root / reference["path"], build)
        with self.assertRaisesRegex(ValueError, "failed step"):
            self.fixture.qualify()
        build["steps"][0]["exit_status"] = 0
        build["targets"]["openttd"] = build["targets"]["openttd_surface_capital_test"]
        self.fixture.build_results["RelWithDebInfo"] = self.fixture.write(self.fixture.root / reference["path"], build)
        with self.assertRaisesRegex(ValueError, "actual selected build images"):
            self.fixture.qualify()
        self.assertFalse(self.fixture.destination.exists())

    def test_console_projection_cannot_hide_other_changed_image_bytes(self):
        """The subsystem allowance never permits replacing or patching the ordinary code.

        @param self Active isolated test.
        @return None; changing an unrelated PE byte fails the current-image comparison.
        """
        ordinary = owner.read(self.fixture.ordinary_results["RelWithDebInfo"], self.fixture.root)
        application = self.fixture.root / ordinary["application"]["path"]
        executable = self.fixture.root / ordinary["console_application"]["path"]
        payload = bytearray(executable.read_bytes())
        payload[-1] ^= 1
        executable.write_bytes(payload)
        with self.assertRaisesRegex(ValueError, "beyond the permitted PE"):
            owner.verify_console_projection(application, executable, executable.parent, self.fixture.root)

    def test_legacy_receipt_and_new_operation_shape_are_not_qualified_originals(self):
        """Explicit discriminators reject legacy complete=true and future operation receipts.

        @param self Active isolated test.
        @return None; no compatibility receipt or implicit history conversion is created.
        """
        self.fixture.destination.write_text(json.dumps(dict(complete=True, configuration="Debug", operation={})), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "exact typed fields"):
            load_foundation(self.fixture.destination, root=self.fixture.root)
        with self.assertRaises(ValueError):
            load_foundation(self.fixture.destination.with_name("foundation.json.pending"), root=self.fixture.root)

    def test_raw_original_witness_cannot_replace_the_qualification_copy(self):
        """A matching old raw path cannot restore dependency on disposable earlier runs.

        @param self Active isolated test.
        @return None; the completed foundation requires its own copied evidence role.
        """
        foundation = self.fixture.qualify()
        document = json.loads(foundation.manifest_path.read_text(encoding="utf-8"))
        native = owner.read(self.fixture.native_results["Debug"], self.fixture.root)
        document["configurations"]["Debug"]["witnesses"]["native_terminal"] = native["terminal"]
        foundation.manifest_path.write_text(json.dumps(document), encoding="utf-8")
        selected = load_foundation(foundation.manifest_path, root=self.fixture.root)
        with self.assertRaisesRegex(ValueError, "another run/role"):
            owner.verify_foundation(selected)


if __name__ == "__main__":
    unittest.main(verbosity=2)
