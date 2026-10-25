"""Check explicit current source expectation handoff roles without a native world.

@file merged_sprite_expectations_check.py
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Deliberate faults live in the selected generated review run; no original payload
or policy is changed and no recursive cleanup is performed.
"""
import argparse
import copy
import json
from pathlib import Path
import unittest

from artifact_paths import RunContext
from artifact_rebuild import identity
from artifact_recipes import load_catalogue, recipe_order
import sprite_expected_payload as source

EXPECTATIONS = None


class MergedExpectationChecks(unittest.TestCase):
    """Preserve independent source truth, precise native bindings and required evidence."""

    @classmethod
    def setUpClass(cls):
        """Choose one explicit current bundle and separate disposable fault outputs.

        @param cls Active handoff contract suite; no latest-run discovery is used.
        """
        cls.document = json.loads(EXPECTATIONS.read_text(encoding="utf-8"))
        cls.context = RunContext(source.ROOT, "multilayer-art", "merged-expectation-contracts", cls.document["run_id"])
        cls.context.reports.mkdir(parents=True, exist_ok=True)
        cls.reference = identity(EXPECTATIONS, source.ROOT)

    def fault(self, document, label):
        """Create a coherently hashed metadata fault without changing its real supplier.

        @param self Active handoff check.
        @param document Detached deliberately invalid bundle.
        @param label Fixed local fault filename suffix.
        @return Exact fault report identity for the real current selector.
        """
        path = self.context.reports / ("fault-" + label + ".json")
        path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
        return identity(path, source.ROOT)

    def test_actual_five_family_bundle_has_ten_distinct_native_bindings(self):
        """Accept only the actual independently original-derived current payloads.

        @param self Active positive handoff check; native decoding remains unclaimed.
        """
        bundle = source.checked_expectation_bundle(self.reference, self.context)
        self.assertEqual(set(bundle["families"]), set(source.load_facts()))
        self.assertEqual(sum(len(row) for row in bundle["input_bindings"].values()), 10)
        self.assertFalse(bundle["native_loading_verified"])
        self.assertFalse(bundle["publishes_source"])

    def test_rehashed_wrong_family_configuration_run_and_native_claim_refuse(self):
        """Reject semantic substitutions even when the outer report hash is current.

        @param self Active family/configuration/invocation boundary check.
        """
        for label in ("family", "configuration", "run", "native"):
            document = copy.deepcopy(self.document)
            if label == "family":
                document["families"].pop("ground")
            elif label == "configuration":
                document["input_bindings"]["Debug"]["m1"]["configuration"] = "RelWithDebInfo"
            elif label == "run":
                document["run_id"] = "foreign-source-run"
            else:
                document["native_loading_verified"] = True
            with self.assertRaises(ValueError):
                source.checked_expectation_bundle(self.fault(document, label), self.context)

    def test_required_source_evidence_cannot_be_replaced_by_optional_diagnostics(self):
        """Require actual source-policy proof while leaving diagnostic logs optional.

        @param self Active evidence-role check.
        """
        document = copy.deepcopy(self.document)
        document["diagnostics"] = {"optional": True, "path": "logs/absent-optional-expectation-diagnostic.txt"}
        source.checked_expectation_bundle(self.fault(document, "optional-diagnostic"), self.context)
        document.pop("required_source_evidence")
        with self.assertRaisesRegex(ValueError, "missing required source evidence"):
            source.checked_expectation_bundle(self.fault(document, "missing-required"), self.context)

    def test_changed_source_policy_or_actual_candidate_refuses(self):
        """Keep independently frozen source truth separate from the actual GRF role.

        @param self Active source/candidate identity check.
        """
        for key in ("source_policy", "actual_candidate"):
            document = copy.deepcopy(self.document)
            document[key]["sha256"] = "0" * 64
            with self.assertRaises(ValueError):
                source.checked_expectation_bundle(self.fault(document, key), self.context)

    def test_native_leaf_prepares_only_its_original_required_expectation_prefixes(self):
        """Preserve minimal decoder leaf closures through the existing recipe graph.

        @param self Active source/packing/expectation dependency selection check.
        """
        recipes = {row["id"]: row for row in load_catalogue(source.ROOT)["recipes"]}
        expected = {"m1": {"m1"}, "space": {"space"}, "orbital": {"orbital"},
                    "launch-fleet": {"m1", "space", "orbital", "launch-fleet"}, "ground": set(source.load_facts())}
        # Original full-prefix protocols require their actual preceding source
        # bytes; independent smaller scopes must not prepare unrelated families.
        for family, required in expected.items():
            selected = recipe_order(source.recipe_family(family) + "-native-loader", recipes)
            actual = {recipes[name]["original_parameters"]["family"] for name in selected if name.endswith("-expected-payload")}
            self.assertEqual(actual, required)
            self.assertNotIn("merged-art-candidates", selected)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--expectations", type=Path, required=True)
    arguments = parser.parse_args()
    EXPECTATIONS = arguments.expectations.resolve()
    unittest.main(argv=[__file__], verbosity=2)
