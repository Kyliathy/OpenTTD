"""Verify current Ground packing roots and refusal boundaries without old products.

@file ground_packing_contract_check.py No producer generation, native run or publication.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DE.
"""

import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

import PIL
from artifact_paths import RunContext, repository_root
from source_input_guard import no_old_proofs

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_ground_art as packing
import regenerate_ground_sources as source


class GroundPackingRoots(unittest.TestCase):
    """Preserve original physical/range facts without reopening historical publication closure."""

    def test_compact_current_scope_requires_no_old_products(self):
        """Read complete original source/prefix/range identities from retained facts only.

        @param self Active compact current input-root check.
        @return None; the old2145-file snapshot, native audit and executables are dispensable.
        """
        with no_old_proofs(ROOT):
            scope, before, _public = packing.handoff.read_scope(current_sources=True)
        self.assertEqual(len(scope["asset_ids"]), 47)
        self.assertEqual(len(before["native_prefix"]), 62)
        self.assertEqual(scope["sprite_count"], 14254)
        self.assertEqual((scope["native_start"], scope["native_end_exclusive"]), (7791, 22045))
        self.assertNotIn("files", before)
        self.assertNotIn("predecessor_payload", before)

    def test_partial_selection_and_missing_source_reports_refuse(self):
        """Reject incomplete current ownership rather than fall back to old filesystem paths.

        @param self Active exact current source/output selection check.
        @return None; no actual output directory or source producer is created.
        """
        context = RunContext(ROOT, "multilayer-art", "ground-packing-contract", "cleanup17de-contract-01")
        with no_old_proofs(ROOT):
            with self.assertRaisesRegex(ValueError, "both source reports and output context"):
                packing.derive(source_manifests={})
            with self.assertRaisesRegex(ValueError, "both source reports and output context"):
                packing.derive(context=context)
            with self.assertRaisesRegex(ValueError, "every exact selected physical source"):
                packing.derive(source_manifests={}, context=context)

    def test_changed_encoder_and_prior_native_range_refuse(self):
        """Refuse changed tool versions and source-independent prefix range drift.

        @param self Active immutable original packing expectation check.
        @return None; all mutations are copied public metadata or mocked tool versions.
        """
        with patch.object(PIL, "__version__", "0.0-unit-mismatch"), no_old_proofs(ROOT):
            with self.assertRaisesRegex(ValueError, "pinned toolchain"):
                packing.handoff.read_scope(current_sources=True)
        public = json.loads(packing.PUBLIC.read_text(encoding="utf-8"))
        changed = copy.deepcopy(public)
        prefix = source.load_facts()["native_prefix"][0]["asset_id"]
        next(row for row in changed["assets"] if row["asset_id"] == prefix)["packed"]["native_offset"] += 1
        with no_old_proofs(ROOT):
            with self.assertRaisesRegex(ValueError, "published native prefix range"):
                packing.handoff.read_scope(changed, current_sources=True)


if __name__ == "__main__":
    unittest.main(verbosity=2)
