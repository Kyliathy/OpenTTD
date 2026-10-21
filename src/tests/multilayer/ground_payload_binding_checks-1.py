"""Qualify exact selected-ground packed publication and refuse false or unrelated evidence.

@file ground_payload_binding_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79E.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All corruption fixtures are detached dictionaries; no immutable artifact is edited.
"""

import copy
import json
from pathlib import Path
import time
import unittest

from ground_pack_check_support import ROOT, handoff
from tools import ground_payload_binding as binding


class GroundPayloadBindingChecks(unittest.TestCase):
    """Require complete source/native/cache proof and exact62-plus47 public transitions."""

    @classmethod
    def setUpClass(cls):
        """Read actual current qualified receipts and public predecessor facts.

        @param cls Active focused publication/refusal suite.
        @return None.
        """
        cls.scope, cls.before, cls.public = handoff.read_scope()
        cls.proof = handoff.read_json(binding.PROOF.relative_to(ROOT).as_posix())
        cls.report = handoff.read_json(cls.scope["report"])

    def test_actual_full_native_cached_proof_allows_only47_bindings_and62_checksums(self):
        """Real qualified bytes authorize exact own bindings and no other source/definition changes.

        @param self Active actual-data positive transition qualification.
        @return None.
        """
        packed, proposed = binding.prepare()
        self.assertEqual(len(packed["families"]), 47)
        self.assertEqual(len(packed["authorized_public_updates"]), 109)
        self.assertEqual(len(packed["packed_grf"]["authorized_prefix_checksum_updates"]), 62)
        self.assertTrue(packed["native_loading_verified"])
        self.assertFalse(packed["runtime_activated"])
        current = {row["asset_id"]: row for row in proposed["assets"]}
        prefix = {row["asset_id"] for row in self.before["native_prefix"]}
        new = set(self.scope["asset_ids"])
        # Normalize only the explicitly owned packed fields and compare the
        # complete predecessor row, protecting saved namespace/local IDs and
        # every original source/authoring/geometry field of all public families.
        for original in self.before["public"]["assets"]:
            asset_id = original["asset_id"]
            row = copy.deepcopy(current[asset_id])
            if asset_id in new:
                row["packed"] = original["packed"]
            elif asset_id in prefix:
                row["packed"]["sha256"] = original["packed"]["sha256"]
            self.assertEqual(row, original, asset_id)

    def test_missing_native_configuration_or_disabled_generator_gate_refuses(self):
        """One source/native process cannot replace both native configurations and cached packaging.

        @param self Active missing-process/distribution refusal fixture.
        @return None.
        """
        proof = copy.deepcopy(self.proof)
        del proof["native"]["RelWithDebInfo"]
        with self.assertRaisesRegex(ValueError, "native/generator-disabled qualification"):
            binding.prepare(proof=proof)
        proof = copy.deepcopy(self.proof)
        proof["generator_free"]["generator_discovery_disabled"] = False
        with self.assertRaisesRegex(ValueError, "native/generator-disabled qualification"):
            binding.prepare(proof=proof)

    def test_partial_or_hash_only_native_coverage_refuses(self):
        """Source flags and partial pixel coverage cannot become a published complete family range.

        @param self Active full-byte coverage refusal fixture.
        @return None.
        """
        proof = copy.deepcopy(self.proof)
        proof["native"]["Debug"]["observation"]["new_sprites"] -= 1
        with self.assertRaisesRegex(ValueError, "actual native observation"):
            binding.prepare(proof=proof)
        proof = copy.deepcopy(self.proof)
        proof["generator_free"]["native"]["Debug"]["observation"]["full_pixel_bytes_exact"] = False
        with self.assertRaisesRegex(ValueError, "actual native observation"):
            binding.prepare(proof=proof)

    def test_wrong_container_identity_refuses(self):
        """A different GRF cannot inherit the native/cache acceptance of this actual complete payload.

        @param self Active foreign-container proof fixture.
        @return None.
        """
        proof = copy.deepcopy(self.proof)
        proof["payload"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "payload/checksum qualification"):
            binding.prepare(proof=proof)

    def test_existing_native_range_change_refuses(self):
        """Published Space/launch/collector ranges retain every offset and geometry identity.

        @param self Active complete-prefix range refusal fixture.
        @return None.
        """
        public = copy.deepcopy(self.public)
        name = self.before["native_prefix"][-1]["asset_id"]
        next(row for row in public["assets"] if row["asset_id"] == name)["packed"]["native_offset"] += 1
        with self.assertRaisesRegex(ValueError, "published native prefix range"):
            binding.prepare(public=public)

    def test_selected_source_or_saved_definition_id_change_refuses(self):
        """New native bindings cannot rewrite immutable originals or draft definition-local IDs.

        @param self Active source/namespace refusal fixture.
        @return None.
        """
        public = copy.deepcopy(self.public)
        row = next(row for row in public["assets"] if row["asset_id"] == self.scope["asset_ids"][0])
        row["source"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "public source/geometry/binding"):
            binding.prepare(public=public)
        public = copy.deepcopy(self.public)
        row = next(row for row in public["assets"] if row["asset_id"] == self.scope["asset_ids"][0])
        row["packed"]["namespace"] = "foreign_definition_namespace"
        with self.assertRaisesRegex(ValueError, "public source/geometry/binding"):
            binding.prepare(public=public)

    def test_unrelated_family_or_public_envelope_change_refuses(self):
        """Unselected/deferred families and the profile's public definition envelope remain exact.

        @param self Active unrelated-owner refusal fixture.
        @return None.
        """
        selected = set(self.scope["asset_ids"]) | {row["asset_id"] for row in self.before["native_prefix"]}
        public = copy.deepcopy(self.public)
        row = next(row for row in public["assets"] if row["asset_id"] not in selected)
        row["source"]["status"] = "foreign_source"
        with self.assertRaisesRegex(ValueError, "public source/geometry/binding"):
            binding.prepare(public=public)
        public = copy.deepcopy(self.public)
        public["foreign_profile_owner"] = True
        with self.assertRaisesRegex(ValueError, "family set or definition envelope"):
            binding.prepare(public=public)

    def test_repeat_transition_is_idempotent_and_receipt_source_is_unmodified(self):
        """Reapplying the exact qualified transition changes no IDs and no historical source report.

        @param self Active pure transition/idempotence check.
        @return None.
        """
        sha = self.proof["payload"]["sha256"]
        proposed, changes = binding.public_transition(self.public, self.before, self.scope, self.report, sha)
        repeated, second = binding.public_transition(proposed, self.before, self.scope, self.report, sha)
        self.assertEqual(proposed, repeated)
        self.assertEqual(changes, second)
        self.assertEqual(self.report, handoff.read_json(self.scope["report"]))
        self.assertIsNone(self.report["packed_grf"])


if __name__ == "__main__":
    started = time.perf_counter()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(GroundPayloadBindingChecks))
    scope = handoff.read_scope()[0]
    inputs = [Path(__file__), Path(binding.__file__), binding.PROOF, handoff.SCOPE, handoff.bound(scope["baseline"])]
    receipt = {"_documentation": __doc__, "passed": result.wasSuccessful(), "cases": result.testsRun,
               "seconds": round(time.perf_counter() - started, 3), "failures": len(result.failures), "errors": len(result.errors),
               "inputs": [binding.reference(path) for path in inputs], "runtime_activated": False}
    binding.QUALIFICATION.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    raise SystemExit(0 if result.wasSuccessful() else 1)
