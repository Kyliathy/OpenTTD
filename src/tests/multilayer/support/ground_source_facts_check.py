"""Verify durable ground expectations and refusal boundaries without old product reads.

@file ground_source_facts_check.py No source publication, native run or baseline update.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17D.
Original source and prefix files remain read-only; only detached fault metadata changes.
"""

import copy
import json
import unittest
from unittest.mock import patch

import ground_source_facts as facts
from source_input_guard import no_old_proofs


class GroundFactsTests(unittest.TestCase):
    """Keep physical expectations independent of disposable staging/native history."""

    def test_durable_originals_and_prefix_verify_without_historical_products(self):
        """Verify actual retained inputs after blocking old proof/log file access.

        @param self Active durable-input check; no generated/native proof is inferred.
        @return None; exact original physical comparison counts remain independently fixed.
        """
        with no_old_proofs(facts.ROOT):
            result = facts.verify_facts()
        self.assertEqual(result["selected_families"], 50)
        self.assertEqual(result["source_families"], 47)
        self.assertEqual(result["pose_count"], 14254)
        self.assertEqual(result["historical_native_mismatches"], 10)
        self.assertFalse(result["native_proof"])

    def test_missing_or_corrupt_expectations_refuse_without_regeneration(self):
        """Refuse a missing fixture or changed expected identity before any source work.

        @param self Active immutable-expectation check; real fixture bytes stay untouched.
        @return None; no fallback to old reports and no automatic baseline creation.
        """
        with patch.object(facts, "FIXTURE", facts.FIXTURE.with_name("absent-ground-source-facts.json")):
            with self.assertRaisesRegex(ValueError, "expectation is missing"):
                facts.load_facts()
        with patch.object(facts, "FIXTURE_SHA256", "0" * 64):
            with self.assertRaisesRegex(ValueError, "no automatic baseline update"):
                facts.load_facts()

    def test_changed_geometry_prefix_and_original_descriptors_refuse(self):
        """Refuse drift in real family geometry, native ranges and original descriptor bytes.

        @param self Active physical-input check; mutations affect private metadata copies only.
        @return None; historical native epochs cannot bless different current source inputs.
        """
        original = facts.load_facts()
        changed = copy.deepcopy(original)
        changed["selected_public_facts"]["mine.COAL"]["anchor"][0] += 1
        with patch.object(facts, "load_facts", return_value=changed):
            with self.assertRaisesRegex(ValueError, "physical/source contract"):
                facts.verify_facts()
        changed = copy.deepcopy(original)
        changed["native_prefix"][0]["packed"]["native_offset"] += 1
        with patch.object(facts, "load_facts", return_value=changed):
            with self.assertRaisesRegex(ValueError, "prefix geometry/range"):
                facts.verify_facts()
        changed = copy.deepcopy(original)
        changed["original_native_descriptors"][0]["sha256"] = "0" * 64
        with patch.object(facts, "load_facts", return_value=changed):
            with self.assertRaisesRegex(ValueError, "original descriptor changed"):
                facts.verify_facts()

    def test_retained_fleet_input_matches_original_m1_approval_and_refuses_fallback(self):
        """Resolve the executed fleet AI input through its original approved byte identity.

        @param self Active provenance check; real M1/source/reference images stay untouched.
        @return None; a changed/removed alias cannot reopen old logs as source input.
        """
        import convert_space_art as originals

        public = json.loads(facts.handoff.PUBLIC.read_text(encoding="utf-8"))
        m1 = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-04")["authoring"]
        reference = dict(path=m1["native_atlas"], sha256=m1["native_sha256"])
        path = facts.ROOT / "media/multilayer/source/ground/fleet-generation-record.json"
        record = json.loads(path.read_text(encoding="utf-8"))
        with no_old_proofs(facts.ROOT):
            resolved = originals.source_reference(reference, record, feature="ground")
            self.assertEqual(facts.identity(resolved, facts.ROOT)["sha256"], reference["sha256"])
            changed = copy.deepcopy(record)
            changed["retained_reference_inputs"][reference["path"]]["sha256"] = "0" * 64
            with self.assertRaisesRegex(ValueError, "identity changed"):
                originals.source_reference(reference, changed, feature="ground")
            changed = copy.deepcopy(record)
            del changed["retained_reference_inputs"][reference["path"]]
            with self.assertRaisesRegex(ValueError, "escapes"):
                originals.source_reference(reference, changed, feature="ground")


if __name__ == "__main__":
    unittest.main(verbosity=2)
