"""Focused complete-raster and actual registered Ground capture scope refusals.

@file ground_viewport_review_check.py Transport metadata unit checks; no simulated native acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB.
"""
import hashlib
from pathlib import Path
import tempfile
import unittest

from ground_viewport_review import ROOT, selected_recipe
from ground_viewport_capture import MODES, verify_original_rasters
from artifact_paths import RunContext
from artifact_rebuild import identity


class GroundViewportChecks(unittest.TestCase):
    """Keep independent original raster bytes and exact actual producer scopes mandatory."""

    def setUp(self):
        """Create a tiny contained byte-comparison fixture without any native success flag.

        @param self Focused immutable-expectation transport test owner.
        """
        temporary = tempfile.TemporaryDirectory(prefix="ground-raster-scope-", dir=ROOT / "test-cache/cleanup")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.context = RunContext(self.root, "selected-ground", "retained-collector", "unit-scope")
        self.context.images.mkdir(parents=True)
        self.path = self.context.images / "native.idx"
        expected = b"independent original indexed bytes"
        self.path.write_bytes(expected)
        self.world = dict(expected_captures={self.path.name: dict(sha256=hashlib.sha256(expected).hexdigest(), bytes=len(expected))})
        self.process = dict(artifacts=[identity(self.path, self.root)])

    def test_rehashed_wrong_native_bytes_cannot_become_their_own_expectation(self):
        """Refuse altered actual bytes after their declared process checksum is truthfully updated.

        @param self Focused complete-byte comparison test owner.
        """
        self.assertEqual(verify_original_rasters(self.context, self.process, self.world), {self.path.name: self.path})
        changed = bytearray(self.path.read_bytes())
        changed[0] ^= 1
        self.path.write_bytes(changed)
        self.process["artifacts"] = [identity(self.path, self.root)]
        with self.assertRaisesRegex(ValueError, "original complete capture"):
            verify_original_rasters(self.context, self.process, self.world)

    def test_partial_or_duplicate_native_domain_refuses(self):
        """Keep every original raster exactly once instead of accepting a matching subset.

        @param self Focused complete-domain comparison test owner.
        """
        with self.assertRaisesRegex(ValueError, "domain"):
            verify_original_rasters(self.context, dict(artifacts=[]), self.world)
        with self.assertRaisesRegex(ValueError, "duplicate"):
            verify_original_rasters(self.context, dict(artifacts=self.process["artifacts"] * 2), self.world)

    def test_all_original_modes_have_isolated_actual_native_owners(self):
        """Keep actual target/tags, three-image build and six independent artifact scenarios.

        @param self Focused sole-catalogue producer boundary test owner.
        """
        scenarios = set()
        for mode, specification in MODES.items():
            row = selected_recipe(mode)
            self.assertEqual(row["test_tags"], [specification["tag"]])
            self.assertEqual(row["target"], "openttd_ground_viewport_test")
            self.assertEqual(len(row["original_parameters"]["build_targets"]), 3)
            self.assertFalse(row["original_parameters"]["paid_progress_claimed"])
            scenarios.add((row["feature"], row["scenario"]))
        self.assertEqual(len(scenarios), 6)


if __name__ == "__main__":
    unittest.main()
