"""Focused actual-owner preparation and runtime selection checks for Space recovery.

@file space_checkpoint_recipe_check.py Independent saves and synthetic build transport only.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group15.
Actual application compilation/load execution is a separate Group15 integration gate.
"""

import os
import unittest
from unittest.mock import patch

import space_checkpoint_recipe as recipe
from space_checkpoint_foundation_fixture import FoundationFixture
from space_checkpoint_load import identity


class SpaceRecipeChecks(unittest.TestCase):
    """Preserve explicit zero-tick scope and the original strict native runtime home.

    @see r2fc-file-cleanup3.md15; no game process is launched by these focused tests.
    """

    def test_prepare_verifies_original_and_current_build_without_executing(self):
        """Check actual retained closure/current build metadata before native execution.

        @param self Active isolated synthetic transport test.
        @return None; both load entry points are blocked during preparation.
        """
        fixture = FoundationFixture(self)
        request = dict(stage="prepare", configuration="RelWithDebInfo", paid_work_authorized=False,
            publishes_source=False, inputs={"checkpoint-contract": identity(fixture.root / "src/tests/fixtures/checkpoints/space/checkpoint.json", fixture.root)},
            build=dict(status="passed", result=fixture.build_results["RelWithDebInfo"]))
        with patch.object(recipe, "execute_load_check", side_effect=AssertionError("prepare launched native")), \
                patch.object(recipe, "execute_static_check", side_effect=AssertionError("prepare launched ordinary")):
            result = recipe.execute_stage(request, fixture.root)
        self.assertEqual(len(result["checks"]), 2)
        self.assertEqual(result["configuration"], "RelWithDebInfo")

    def test_native_home_inherits_exact_run_and_restores_parent(self):
        """Select native/main while preserving the parent's explicit invocation identity.

        @param self Active environment-contract test.
        @return None; environment changes are scoped to the serial child adapter.
        """
        fixture = FoundationFixture(self)
        before = dict(OTTD_TEST_RUN_ID="recipe-adapter", OTTD_TEST_PROFILE="rebuild",
                      OTTD_TEST_PROCESS="main", OTTD_TEST_RUNTIME_DIR="parent-stage-home")
        with patch.dict(os.environ, before):
            with recipe.native_children(dict(run_id="recipe-adapter"), fixture.root):
                self.assertEqual(os.environ["OTTD_TEST_RUN_ID"], "recipe-adapter")
                self.assertEqual(os.environ["OTTD_TEST_PROFILE"], "native")
                self.assertEqual(os.environ["OTTD_TEST_RUNTIME_DIR"], str(fixture.root / "mlm/runtime/native/recipe-adapter/main"))
            self.assertEqual({key: os.environ[key] for key in before}, before)
            with self.assertRaisesRegex(ValueError, "explicit inherited invocation"):
                with recipe.native_children(dict(run_id="different"), fixture.root):
                    self.fail("wrong invocation entered native scope")

    def test_paid_or_wrong_configuration_scope_refuses_before_reading(self):
        """A request cannot extend recovery into a paid stage or guessed configuration.

        @param self Active pure scope-refusal test.
        @return None; malformed work requests cannot touch source or invoke any owner.
        """
        original = dict(stage="verify", configuration="Debug", paid_work_authorized=False, publishes_source=False)
        for field, value in (("stage", "work"), ("configuration", "Release"),
                             ("paid_work_authorized", True), ("publishes_source", True)):
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "zero-tick recipe scope"):
                recipe.execute_stage(dict(original, **{field: value}), None)


if __name__ == "__main__":
    unittest.main(verbosity=2)
