"""Verify bounded child declaration checks without weakening supervisor validation.

@file artifact_recipe_selection_check.py No producer/native execution or filesystem mutation.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DE.
"""

import copy
import unittest
from unittest.mock import patch

import artifact_recipes as recipes


class SelectedRecipeTests(unittest.TestCase):
    """Keep child checks bounded while preserving exact selected source and role gates."""

    def setUp(self):
        """Read current declarations as private metadata for each focused refusal case.

        @param self Active selected-declaration boundary check.
        @return None; real catalogue/source bytes remain unchanged.
        """
        self.catalogue = recipes.load_catalogue()

    def selected(self, name):
        """Find one exact mutable declaration in this test's private catalogue.

        @param self Active private metadata owner.
        @param name Existing canonical recipe ID.
        @return Selected dictionary; no filename/default discovery is used.
        """
        return next(row for row in self.catalogue["recipes"] if row["id"] == name)

    def test_original_validator_receives_only_actual_selected_closure(self):
        """Preserve real selected source/input checks without scanning unrelated families.

        @param self Active bounded-scope check.
        @return None; the actual original validator still runs on both genuine leaves.
        """
        with patch.object(recipes, "validate_recipes", wraps=recipes.validate_recipes) as validate:
            result = recipes.validate_selection(self.catalogue, "ground-source-facility-utility-riser")
        expected = {"ground-source-facility-utility-riser", "ground-source-geothermal-conduit"}
        self.assertEqual(set(result), expected)
        selected = validate.call_args.args[0]["recipes"]
        self.assertEqual({row["id"] for row in selected}, expected)

    def test_supervisor_still_rejects_unrelated_invalid_catalogue(self):
        """Keep full root validation authoritative while allowing bounded current child checks.

        @param self Active supervisor/child separation check.
        @return None; an invalid unrelated declaration is never runnable or silently repaired.
        """
        bad = copy.deepcopy(self.selected("ground-source-control-tube"))
        bad.update(id="invalid-unrelated", producer="media/multilayer/absent-original-producer.py")
        self.catalogue["recipes"].insert(0, bad)
        selected = recipes.validate_selection(self.catalogue, "ground-source-control-tube")
        self.assertEqual(set(selected), {"ground-source-control-tube"})
        with self.assertRaisesRegex(ValueError, "original recipe producer is absent"):
            recipes.validate_recipes(self.catalogue)
        with self.assertRaisesRegex(ValueError, "original recipe producer is absent"):
            recipes.validate_selection(self.catalogue, "invalid-unrelated")

    def test_unknown_duplicate_and_cyclic_dependencies_refuse(self):
        """Reject graph ambiguity before it can select another producer or hidden fallback.

        @param self Active dependency boundary check.
        @return None; mutations affect copied declarations only.
        """
        with self.assertRaisesRegex(ValueError, "unknown recipe dependency"):
            recipes.validate_selection(self.catalogue, "absent-selection")
        self.catalogue["recipes"].append(copy.deepcopy(self.catalogue["recipes"][0]))
        with self.assertRaisesRegex(ValueError, "duplicate recipe ID"):
            recipes.validate_selection(self.catalogue, "ground-source-control-tube")
        self.catalogue = recipes.load_catalogue()
        row = self.selected("ground-source-control-tube")
        row["requires"] = [{"recipe": row["id"], "role": "manifest", "as": "cycle"}]
        with self.assertRaisesRegex(ValueError, "cyclic"):
            recipes.validate_selection(self.catalogue, row["id"])

    def test_selected_publication_and_missing_inputs_remain_forbidden(self):
        """Run actual existing selected execution/input guards after bounded discovery.

        @param self Active selected source authorization boundary check.
        @return None; unrelated metadata cannot weaken selected safety or input requirements.
        """
        row = self.selected("ground-source-control-tube")
        row["publishes_source"] = True
        with self.assertRaisesRegex(ValueError, "cannot publish source"):
            recipes.validate_selection(self.catalogue, row["id"])
        row["publishes_source"] = False
        row["retained_inputs"][0]["path"] = "src/tests/fixtures/graphics/absent-original-input.json"
        with self.assertRaisesRegex(ValueError, "missing required input"):
            recipes.validate_selection(self.catalogue, row["id"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
