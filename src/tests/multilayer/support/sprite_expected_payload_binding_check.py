"""Verify typed current source expectation inputs with actual original M1 production.

@file sprite_expected_payload_binding_check.py Generated fault copies never modify source.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group18.
"""

import argparse
import copy
import json
from pathlib import Path
import sys
import unittest

from artifact_paths import RunContext
from artifact_rebuild import identity
from source_input_guard import no_old_proofs
import sprite_expected_payload as source

RUN_ID = RECIPE = None


class SpriteExpectationBindings(unittest.TestCase):
    """Bind exact real source payloads while refusing coherent current metadata/data faults."""

    @classmethod
    def setUpClass(cls):
        """Generate one actual current M1 payload from its explicitly selected completed packing.

        @param cls Active typed source input test class.
        @return None; the original producer runs with old-proof/log I/O refused.
        """
        cls.context = RunContext(source.ROOT, "multilayer-art", "m1-expectation-binding-check", RUN_ID)
        root = source.ROOT / "mlm/reviews/rebuild" / RECIPE / RUN_ID / "reports/rebuild.json"
        state = json.loads(root.read_text(encoding="utf-8"))
        # Only the explicit completed current run supplies this source input;
        # cleanup or transfer requires rebuilding its selected packing first.
        if state["complete"] is not True or state["status"] != "verified" or state["run_id"] != RUN_ID:
            raise ValueError("typed expectation checks require completed explicit original packing")
        reference = state["nodes"]["m1-derived-inputs"]["outputs"]["manifest"]
        with no_old_proofs(source.ROOT):
            cls.result = source.produce("m1", cls.context, reference)
        cls.document = json.loads((source.ROOT / cls.result["manifest"]["path"]).read_text(encoding="utf-8"))

    def fault(self, document, label):
        """Write one deliberate typed negative fixture in this test's actual current reports.

        @param self Active explicit source expectation boundary check.
        @param document Detached fault document; actual producer report stays exact.
        @param label Fixed generated fault filename suffix.
        @return Exact current fault-file identity for the real selector.
        """
        path = self.context.reports / ("fault-" + label + ".json")
        document["_documentation"] = "Owner: r2fc-file-cleanup3.md18; disposable coherent typed source expectation fault."
        path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
        return identity(path, source.ROOT)

    def test_actual_typed_source_report_preserves_original_payload_identity(self):
        """Accept the actual current source producer without granting native acceptance.

        @param self Active actual typed report selection check.
        @return None; exact old independent bytes are observed through current roles.
        """
        with no_old_proofs(source.ROOT):
            path = source.checked_payload("m1", self.result["manifest"], self.context)
        self.assertEqual(identity(path, source.ROOT), self.result["expected_payload"])
        self.assertFalse(self.document["native_loading_verified"])
        self.assertEqual(self.document["record_count"], 3561)

    def test_coherent_wrong_family_and_producer_reports_refuse(self):
        """Refuse rehashed reports claiming another original protocol or producer epoch.

        @param self Active semantic current source provenance check.
        @return None; deliberate faults remain disposable test reports.
        """
        # Correct binary bytes cannot authorize a changed family or producer.
        # Each coherent fault reaches the real typed selector after rehashing.
        for key, value in (("family", "space"), ("producer", {"path": "foreign.py", "sha256": "0" * 64})):
            document = copy.deepcopy(self.document)
            document[key] = value
            reference = self.fault(document, key)
            with no_old_proofs(source.ROOT):
                with self.assertRaisesRegex(ValueError, "family/invocation/status"):
                    source.checked_payload("m1", reference, self.context)

    def test_coherent_changed_bytes_and_foreign_writer_refuse(self):
        """Refuse altered binary pixels even when the current fault report is rehashed.

        @param self Active independent payload/source-writer comparison.
        @return None; original expected payload bytes and producer report stay untouched.
        """
        actual = source.ROOT / self.result["expected_payload"]["path"]
        data = bytearray(actual.read_bytes())
        data[-1] ^= 1
        path = self.context.data / "fault-changed-source.bin"
        path.write_bytes(data)
        document = copy.deepcopy(self.document)
        document["expected_payload"] = identity(path, source.ROOT)
        reference = self.fault(document, "changed-source")
        with no_old_proofs(source.ROOT):
            with self.assertRaisesRegex(ValueError, "independent original bytes"):
                source.checked_payload("m1", reference, self.context)
        foreign = RunContext(source.ROOT, "multilayer-art", "foreign-source-writer", self.context.run_id)
        foreign.data.mkdir(parents=True, exist_ok=True)
        path = foreign.data / "fault-foreign-writer.bin"
        path.write_bytes(actual.read_bytes())
        document["expected_payload"] = identity(path, source.ROOT)
        reference = self.fault(document, "foreign-writer")
        with no_old_proofs(source.ROOT):
            with self.assertRaisesRegex(ValueError, "foreign writer"):
                source.checked_payload("m1", reference, self.context)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--recipe", default="m1-derived-inputs")
    arguments = parser.parse_args()
    RUN_ID, RECIPE = arguments.run_id, arguments.recipe
    unittest.main(argv=[sys.argv[0]], verbosity=2)
