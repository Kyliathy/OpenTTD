"""Focused recipe boundary checks; no producer execution, native tests or build calls.

@file artifact_recipes_check.py Validate selected dependency closure and safe inspection.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups6/17B/17CB.
"""

import copy
from pathlib import Path
import unittest
from unittest.mock import patch

from artifact_recipes import inspect_recipe, load_catalogue, recipe_order, validate_recipes
from artifact_rebuild import verify_retained_inputs


class RecipeTests(unittest.TestCase):
    """Mutate only in-memory copies of the real catalogue to exercise refusal boundaries.

    @param self Active focused check; source, fixtures and generated namespaces remain unchanged.
    """

    def setUp(self):
        """Read the original declarations once for this independent negative case.

        @param self Active recipe boundary check.
        @return None; each case receives a private mutable catalogue dictionary.
        """
        self.catalogue = copy.deepcopy(load_catalogue())

    def recipe(self, name):
        """Select an exact in-memory leaf without filesystem or latest-run discovery.

        @param self Active in-memory test catalogue.
        @param name Existing recipe ID.
        @return Mutable declaration used only by this negative case.
        """
        return next(row for row in self.catalogue["recipes"] if row["id"] == name)

    def test_selected_closure_is_bounded_and_inspection_cannot_write(self):
        """Inspect a selected pending checkpoint without reserving or executing anything.

        @param self Active read-only boundary check.
        @return None; intercept all file-write/native-process entry points.
        """
        with patch.object(Path, "mkdir", side_effect=AssertionError("inspection wrote a directory")), \
                patch.object(Path, "write_text", side_effect=AssertionError("inspection wrote text")), \
                patch.object(Path, "write_bytes", side_effect=AssertionError("inspection wrote bytes")), \
                patch("subprocess.Popen", side_effect=AssertionError("inspection executed a producer")):
            inspected = inspect_recipe("tube-checkpoint", "Debug")
        self.assertEqual(inspected["order"], ["tube-checkpoint"])
        self.assertFalse(inspected["executable"])
        self.assertFalse(inspected["reserves_run_id"])
        self.assertEqual(inspected["missing_inputs"], [])
        self.assertEqual(inspected["pending_groups"]["tube-checkpoint"], [21])
        recipes = validate_recipes(self.catalogue)
        selected = recipe_order("space-native-loader", recipes)
        self.assertEqual(selected, ["space-source-vehicle-model-15", "space-source-vehicle-model-19",
                                    "space-source-vehicle-model-20", "space-source-vehicle-model-18",
                                    "space-source-asteroid-nickel", "space-source-asteroid-ice",
                                    "space-source-asteroid-rare-earth", "space-derived-inputs",
                                    "space-expected-payload", "space-native-loader"])
        self.assertNotIn("ground-native-loader", selected)

    def test_unknown_dependencies_and_supplying_roles_refuse(self):
        """Reject a missing producer recipe and an undeclared output role independently.

        @param self Active in-memory catalogue.
        @return None; errors name the exact unresolved dependency or role.
        """
        self.recipe("launch-current-producers")["requires"][0]["recipe"] = "does-not-exist"
        with self.assertRaisesRegex(ValueError, "unknown recipe dependency"):
            validate_recipes(self.catalogue)
        self.catalogue = copy.deepcopy(load_catalogue())
        self.recipe("launch-current-producers")["requires"][0]["role"] = "does-not-exist"
        with self.assertRaisesRegex(ValueError, "unknown supplying role"):
            validate_recipes(self.catalogue)

    def test_cycles_and_duplicate_roles_refuse(self):
        """Shared prerequisites are valid; cycles and ambiguous result identities fail.

        @param self Active in-memory catalogue.
        @return None; role and dependency order cannot silently pick a winner.
        """
        self.recipe("m1-source-base")["requires"].append(dict(recipe="m1-derived-inputs", role="manifest", **{"as": "cycle"}))
        with self.assertRaisesRegex(ValueError, "cyclic"):
            validate_recipes(self.catalogue)
        self.catalogue = copy.deepcopy(load_catalogue())
        self.recipe("space-derived-inputs")["outputs"].append(dict(role="manifest", path="data/other.json"))
        with self.assertRaisesRegex(ValueError, "duplicate output role"):
            validate_recipes(self.catalogue)
        self.catalogue = copy.deepcopy(load_catalogue())
        self.recipe("space-expected-payload")["requires"][0]["as"] = "tool-requirements"
        with self.assertRaisesRegex(ValueError, "duplicate prerequisite input role"):
            validate_recipes(self.catalogue)

    def test_source_publication_and_escaped_outputs_refuse(self):
        """A rebuild cannot authorize publication or an output outside its review run.

        @param self Active in-memory catalogue.
        @return None; shared containment rejects traversal before any write occurs.
        """
        self.recipe("space-derived-inputs")["publishes_source"] = True
        with self.assertRaisesRegex(ValueError, "cannot publish source"):
            validate_recipes(self.catalogue)
        self.catalogue = copy.deepcopy(load_catalogue())
        self.recipe("space-derived-inputs")["outputs"][0]["path"] = "../../../../src/overwrite.json"
        with self.assertRaisesRegex(ValueError, "escapes"):
            validate_recipes(self.catalogue)

    def test_native_configuration_and_actual_tags_are_required(self):
        """A native proof cannot reuse another configuration or invent a passing selector.

        @param self Active in-memory catalogue.
        @return None; even disabled declarations validate all actual supplied tags.
        """
        self.recipe("space-native-loader")["configuration_reuse"] = "source_bytes_only"
        with self.assertRaisesRegex(ValueError, "cross-configuration"):
            validate_recipes(self.catalogue)
        self.catalogue = copy.deepcopy(load_catalogue())
        self.recipe("space-native-loader")["test_tags"] = ["[fictional-aggregate-suite]"]
        with self.assertRaisesRegex(ValueError, "no original source"):
            validate_recipes(self.catalogue)
        with self.assertRaisesRegex(ValueError, "explicit configuration"):
            inspect_recipe("space-checkpoint")

    def test_missing_inputs_need_an_explicit_supplier(self):
        """A future fixture is classified by its provider group; unknown missing data fails.

        @param self Active in-memory catalogue.
        @return None; no diagnostic/cache fallback can replace the selected input.
        """
        declaration = self.recipe("space-checkpoint")["retained_inputs"][-1]
        declaration["path"] = "src/tests/fixtures/checkpoints/space/absent-checkpoint.json"
        declaration.pop("provider_group")
        with self.assertRaisesRegex(ValueError, "missing required input"):
            validate_recipes(self.catalogue)

    def test_original_source_json_fixtures_keep_their_declared_read_only_owner(self):
        """Allow existing source fixtures while refusing changed hashes and owner escapes.

        @param self Active input-boundary check; no fixture or source file is modified.
        @return None; source scripts and escaped JSON never gain retained-fixture authority.
        """
        reference = dict(role="source-fixture", path="src/tests/multilayer/launch-center-foreground-fixture.json",
                         sha256="0d368930220db965a2e6b030ebd536ca2861420c35f2021f3cd4f75b9d5a4f0e")
        actual = verify_retained_inputs(dict(retained_inputs=[reference]))
        self.assertEqual(actual["source-fixture"], {key: reference[key] for key in ("path", "sha256")})
        wrong = dict(reference, sha256="0" * 64)
        with self.assertRaisesRegex(ValueError, "declared identity"):
            verify_retained_inputs(dict(retained_inputs=[wrong]))
        wrong = dict(reference, path="src/tests/multilayer/../../../CMakeUserPresets.json")
        with self.assertRaisesRegex(ValueError, "escapes"):
            verify_retained_inputs(dict(retained_inputs=[wrong]))
        wrong = dict(reference, path="src/tests/multilayer/support/artifact_rebuild.py")
        with self.assertRaisesRegex(ValueError, "outside declared owners"):
            verify_retained_inputs(dict(retained_inputs=[wrong]))


if __name__ == "__main__":
    unittest.main()
