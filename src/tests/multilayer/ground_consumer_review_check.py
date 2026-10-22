"""Focused independent Ground consumer expectation and deferred-owner refusals.

@file ground_consumer_review_check.py Real retained bytes; no synthetic native acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20.
"""
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_rebuild import identity
from run_ground_consumer_checks import verify_consumer_inputs


class GroundConsumerChecks(unittest.TestCase):
    """Keep original source tuples and all later-owner exclusions independently binding."""

    def setUp(self):
        """Copy only the small immutable source expectations into owned repository scratch.

        @param self Active focused unittest owner.
        """
        cache = ROOT / "test-cache/cleanup"
        temporary = tempfile.TemporaryDirectory(prefix="ground-consumer-", dir=cache)
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        roles = {"pose-fixture": "src/tests/fixtures/graphics/ground-pose-tuples.bin",
                 "consumer-expectations": "src/tests/fixtures/graphics/ground-consumer-expectations.json",
                 "dispatch": "media/multilayer/contracts/ground-consumer-dispatch.json"}
        # Every role keeps its original repository-relative identity; these are
        # independent inputs, never a made-up successful native process/report.
        for relative in roles.values():
            target = self.root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((ROOT / relative).read_bytes())
        self.inputs = {role: identity(self.root / relative, self.root) for role, relative in roles.items()}

    def test_original_domain_and_deferred_scope_are_complete(self):
        """Keep all47source families and50dispatch owners with26explicitly deferred.

        @param self Active focused unittest owner.
        """
        actual = verify_consumer_inputs(dict(inputs=self.inputs), self.root)
        self.assertEqual(actual["source_poses"], 14254)
        self.assertEqual(actual["dispatch_counts"], dict(current_bound=21, current_existing=3, later_owner=26))
        self.assertFalse(actual["viewport_claimed"])
        self.assertFalse(actual["whole_underground_complete"])

    def test_rehashed_changed_tuple_cannot_create_its_own_baseline(self):
        """Refuse modified tuple bytes even after both declared input hashes are replaced.

        @param self Active focused unittest owner.
        """
        path = self.root / self.inputs["pose-fixture"]["path"]
        changed = bytearray(path.read_bytes())
        changed[-1] ^= 1
        path.write_bytes(changed)
        self.inputs["pose-fixture"] = identity(path, self.root)
        contract_path = self.root / self.inputs["consumer-expectations"]["path"]
        contract = json.loads(contract_path.read_text(encoding="utf-8"))
        contract["fixture"].update(self.inputs["pose-fixture"])
        contract_path.write_text(json.dumps(contract), encoding="utf-8")
        self.inputs["consumer-expectations"] = identity(contract_path, self.root)
        with self.assertRaisesRegex(ValueError, "original tuple bytes"):
            verify_consumer_inputs(dict(inputs=self.inputs), self.root)

    def test_rehashed_dispatch_cannot_promote_a_later_owner(self):
        """Refuse a scope edit independently of the truthful new dispatch file checksum.

        @param self Active focused unittest owner.
        """
        path = self.root / self.inputs["dispatch"]["path"]
        dispatch = json.loads(path.read_text(encoding="utf-8"))
        row = next(row for row in dispatch["families"] if row["dispatch"] == "later_owner")
        row["dispatch"] = "current_bound"
        path.write_text(json.dumps(dispatch), encoding="utf-8")
        self.inputs["dispatch"] = identity(path, self.root)
        with self.assertRaisesRegex(ValueError, "ownership partition"):
            verify_consumer_inputs(dict(inputs=self.inputs), self.root)


if __name__ == "__main__":
    unittest.main()
