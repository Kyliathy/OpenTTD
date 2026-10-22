"""Run the bounded publication/native/input half of the existing recipe checks.

@file artifact_recipes_check-2.py Split entry point; no duplicate expected behavior.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DDB and test budget.
Use separately from part1; these are the original four source/native boundary cases.
"""

import unittest
import artifact_recipes_check as checks

CASES = ("RecipeTests.test_source_publication_and_escaped_outputs_refuse",
         "RecipeTests.test_native_configuration_and_actual_tags_are_required",
         "RecipeTests.test_missing_inputs_need_an_explicit_supplier",
         "RecipeTests.test_original_source_json_fixtures_keep_their_declared_read_only_owner")

if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromNames(CASES, checks)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    raise SystemExit(0 if result.wasSuccessful() else 1)
