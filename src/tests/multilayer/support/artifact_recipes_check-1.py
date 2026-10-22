"""Run the bounded dependency/inspection half of the existing recipe checks.

@file artifact_recipes_check-1.py Split entry point; original cases stay in their owner.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DDB and test budget.
Use separately from part2 so catalogue growth never requires an unbounded burst.
"""

import unittest
import artifact_recipes_check as checks

CASES = ("RecipeTests.test_selected_closure_is_bounded_and_inspection_cannot_write",
         "RecipeTests.test_unknown_dependencies_and_supplying_roles_refuse",
         "RecipeTests.test_cycles_and_duplicate_roles_refuse")

if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromNames(CASES, checks)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    raise SystemExit(0 if result.wasSuccessful() else 1)
