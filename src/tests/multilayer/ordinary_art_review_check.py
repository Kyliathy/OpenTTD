"""Focused paused review scope/owner checks; no fake native acceptance.

@file ordinary_art_review_check.py Original retained save comparisons and declaration refusals.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19C.
"""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from artifact_recipes import load_catalogue, validate_recipes
from ordinary_art_capture import compare_clocks, compare_owners
from ordinary_art_review import selected_recipe


class PausedReviewTests(unittest.TestCase):
    """Refuse unsafe declarations and independently read original already-paused worlds."""

    def test_original_saved_owners_are_readable_without_historical_indexes(self):
        """Read both exact original worlds without executing or consulting old proof graphs.

        @param self Focused unittest instance.
        """
        contract = json.loads((ROOT / "src/tests/fixtures/graphics/paused-review-worlds.json").read_text())
        # Both capture mechanisms require independently preserved original saves;
        # source-only checks do not claim any fresh ordinary/native acceptance.
        for family, world in contract["worlds"].items():
            with self.subTest(family=family):
                source = resolve_input_path(world["source"]["path"], ROOT)
                actual = compare_owners(source, source, contract)
                self.assertEqual(actual["gameplay_ticks"], 0)
                self.assertEqual(actual["date"]["pause_mode"], 1)
                self.assertEqual(len(actual["unchanged_families"]), 36)

    def test_worker_pause_requires_due_original_job_and_exact_clocks(self):
        """Admit the native load pause without hiding a changed tick, RNG or other flag.

        @param self Focused unittest instance.
        """
        original = dict(pause_mode=1, economy_date=100, tick_counter=50, **{"random_state[0]": 42})
        observed = dict(original, pause_mode=65)
        jobs, schedule = {0: dict(join_date=100)}, dict(running=[1])
        self.assertEqual(compare_clocks(original, observed, jobs, schedule)["output"], 65)
        with self.assertRaises(ValueError):
            compare_clocks(original, observed, {0: dict(join_date=101)}, schedule)
        # Each deviation would silently permit gameplay or foreign pause scope
        # if DATE were ignored wholesale; all must fail independently.
        for field, value in (("tick_counter", 51), ("random_state[0]", 43), ("pause_mode", 129)):
            with self.subTest(field=field), self.assertRaises(ValueError):
                compare_clocks(original, dict(observed, **{field: value}), jobs, schedule)

    def test_ordinary_capture_refuses_fixture_substitution_and_configuration_reuse(self):
        """Keep real GUI startup identity and independent native configurations mandatory.

        @param self Focused unittest instance.
        """
        catalogue = load_catalogue(ROOT)
        # These malformed contracts would turn an empty fixture selection or old
        # configuration into apparent ordinary native proof; each must fail.
        for field, value in (("configuration_reuse", "source_bytes_only"), ("target", "openttd_space_art_test"),
                             ("test_tags", ["[.orbital-art-native]"])):
            with self.subTest(field=field):
                changed = copy.deepcopy(catalogue)
                row = next(item for item in changed["recipes"] if item["id"] == "space-paused-viewports")
                row[field] = value
                with self.assertRaises(ValueError):
                    validate_recipes(changed, ROOT)

    def test_each_family_has_explicit_original_read_only_gui_scope(self):
        """Keep paid work and hidden purchasing cases outside both ordinary leaves.

        @param self Focused unittest instance.
        """
        for family in ("space", "orbital"):
            row = selected_recipe(family)
            self.assertFalse(row["original_parameters"]["live"])
            self.assertEqual(row["test_tags"], [])
            self.assertEqual(row["original_parameters"]["build_targets"], ["openttd_space_art_test", "openttd"])


if __name__ == "__main__":
    unittest.main()
