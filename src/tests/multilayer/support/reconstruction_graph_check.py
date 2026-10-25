"""Check bounded reconstruction selection and retained inputs with old caches unreadable.

@file reconstruction_graph_check.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Cache unavailability is an input-read audit fence, never a directory move/delete.
Actual retained Space/donor inputs stay immutable; no native process is launched.
"""
import argparse
import copy
import json
import os
from pathlib import Path
import sys
import unittest
import uuid

from artifact_paths import RunContext
from artifact_rebuild import identity, load_recipe
from artifact_recipes import inspect_recipe
from sprite_native_loader import check_candidate_result

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/acceptance"))
from verify_donor_inputs import verify_retained_inputs
from space_checkpoint import load_foundation, verify_foundation

NATIVE_RESULT = None
FOUNDATION = ROOT / "src/tests/fixtures/checkpoints/space/continuations/5d8ca01b552778970bde977259f25037e935d3c87de779803e221280d398252c/reports/foundation.json"


class UnavailableOldCaches:
    """Refuse actual file reads/scans beneath old generated owners or Bwithout mutating them."""

    def __init__(self):
        """Begin an inactive read fence; audit hooks become inert again at context exit."""
        self.active = False
        self.forbidden = [ROOT / name for name in ("build", "logs", "mlm/reviews", "test-cache")]
        self.forbidden.append(ROOT.parent / "OpenTTD3")
        sys.addaudithook(self.audit)

    def audit(self, event, arguments):
        """Block only actual path reads/scans while the input-only inspection is active.

        @param event Python filesystem audit event, supplied by the interpreter.
        @param arguments Original structured event arguments; file descriptors are not path lookups.
        """
        # The gate simulates missing old data, not a global filesystem sandbox.
        # Only input operations in the selected verification interval participate.
        if not self.active or event not in ("open", "os.scandir", "os.listdir"):
            return
        candidate = arguments[0]
        if not isinstance(candidate, (str, bytes, os.PathLike)):
            return
        path = Path(os.fsdecode(candidate)).resolve()
        if any(path.is_relative_to(owner) for owner in self.forbidden):
            raise AssertionError("selected retained input borrowed unavailable cache/donor: " + str(path))

    def __enter__(self):
        """Activate the selected input-read fence.

        @return This explicit fence, without renaming or deleting any directory.
        """
        self.active = True
        return self

    def __exit__(self, exception_type, exception, traceback):
        """Deactivate the audit fence while preserving any verification failure.

        @param exception_type Original exception class, if the selected input failed.
        @param exception Original exception object; never suppressed.
        @param traceback Original exception traceback; never modified.
        @return False so all failed input observations remain failures.
        """
        self.active = False
        return False


class ReconstructionGraphChecks(unittest.TestCase):
    """Keep selected prerequisite failures strict and unrelated providers out of bounded inspection."""

    def setUp(self):
        """Create a persistent tiny declaration fixture without cache cleanup.

        @param self Active current reconstruction selection check.
        """
        self.root = ROOT / "test-cache/merge-reconstruction" / uuid.uuid4().hex
        (self.root / "src/tests/fixtures").mkdir(parents=True)
        (self.root / "mlm").mkdir()
        (self.root / "CMakeLists.txt").write_text("# @see r2fc-file-cleanup3.md; declaration-only fixture.\n")
        (self.root / "src/tests/producer.py").write_text('"""@see r2fc-file-cleanup3.md; declaration-only fixture, no functions."""\n')
        self.input = self.root / "src/tests/fixtures/input.dat"
        self.input.write_text("@see r2fc-file-cleanup3.md; independent declaration fixture input.\n")
        self.recipe = dict(id="selected", producer="src/tests/producer.py", feature="inputs", scenario="selected",
            target=None, test_tags=[], tag_sources=[], execution="read_only", enabled=True, pending_groups=[], publishes_source=False,
            stages=["inspect", "build", "prepare", "verify"], configuration_required=False, configuration_reuse="forbid",
            comparison="historical_metadata", retained_inputs=[dict(role="original", path="src/tests/fixtures/input.dat")],
            requires=[], outputs=[dict(role="manifest", path="reports/input.json")], original_parameters={}, tools=["python"], native_timeout_seconds=None)
        self.catalogue = dict(_documentation="@see r2fc-file-cleanup3.md; isolated selected graph fixture.", recipe_schema_version=1,
                              recipes=[self.recipe, dict(self.recipe, id="unrelated", producer="src/tests/absent.py")])
        self.publish()

    def publish(self):
        """Write only this synthetic fixture's declaration catalogue.

        @param self Active fake source owner; the real sole catalogue is unchanged.
        """
        (self.root / "mlm/review-producers.json").write_text(json.dumps(self.catalogue), encoding="utf-8")

    def test_selected_graph_ignores_unrelated_missing_provider(self):
        """A small valid input check cannot require unrelated undeveloped source/interfaces.

        @param self Active bounded dependency-closure check.
        """
        selected = load_recipe("selected", root=self.root)
        self.assertTrue(selected["executable"])
        self.assertEqual(set(selected["recipes"]), {"selected"})
        self.assertTrue(inspect_recipe("selected", root=self.root)["executable"])

    def test_missing_selected_input_and_global_duplicate_id_still_refuse(self):
        """Narrow selection never weakens its own required inputs or globally ambiguous IDs.

        @param self Active selected prerequisite/identity refusal check.
        """
        self.input.rename(self.input.with_suffix(".absent"))
        with self.assertRaisesRegex(ValueError, "missing required input"):
            load_recipe("selected", root=self.root)
        self.input.with_suffix(".absent").rename(self.input)
        self.catalogue["recipes"].append(copy.deepcopy(self.recipe))
        self.publish()
        with self.assertRaisesRegex(ValueError, "duplicate recipe"):
            load_recipe("selected", root=self.root)

    def test_routine_selected_graph_cannot_publish_canonical_sources(self):
        """A routine recipe never gains publication authority through bounded inspection.

        @param self Active candidate-versus-publication separation check.
        """
        self.recipe["publishes_source"] = True
        self.publish()
        with self.assertRaisesRegex(ValueError, "cannot publish"):
            load_recipe("selected", root=self.root)

    def test_actual_space_successor_verifies_without_generated_caches(self):
        """Recheck the actual chosen successor's complete historical witnesses and closed policy.

        @param self Active retained-only Space recovery input check; zero gameplay ticks.
        """
        with UnavailableOldCaches():
            actual = verify_foundation(load_foundation(FOUNDATION, root=ROOT))
        self.assertEqual(actual["next_task"], "Plan11/15CSGZVEEJ")
        self.assertFalse(actual["authorization_granted"])
        self.assertFalse(actual["historical_images_required"])
        self.assertEqual(set(actual["configurations"]), {"Debug", "RelWithDebInfo"})

    def test_actual_donor_payloads_and_aliases_need_no_old_cache_or_donor(self):
        """Every preserved donor input remains supplied by its durable Aowner.

        @param self Active complete preserved-input boundary check, not a gameplay regression.
        """
        with UnavailableOldCaches():
            observed = verify_retained_inputs(ROOT)
        self.assertEqual(observed["payloads"], 7547)
        self.assertGreater(observed["aliases"], observed["payloads"])

    def test_aggregate_requires_actual_opened_candidate_and_process_output(self):
        """Transported old component proof cannot shed its real candidate/origin chain.

        @param self Active pure reader check over explicit57observations; no new current native acceptance claim.
        """
        actual = json.loads(NATIVE_RESULT.read_text(encoding="utf-8"))
        self.assertEqual(check_candidate_result(actual, ROOT), actual["actual_candidate"])
        missing = copy.deepcopy(actual)
        missing.pop("native_candidate_observation")
        with self.assertRaisesRegex(ValueError, "lacks actual candidate"):
            check_candidate_result(missing, ROOT)
        changed = copy.deepcopy(actual)
        changed["actual_candidate"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "witness changed"):
            check_candidate_result(changed, ROOT)
        observed = json.loads((ROOT / actual["native_candidate_observation"]["path"]).read_text())
        context = RunContext(ROOT, "merge-reconstruction", "foreign-origin", actual["run_id"]).for_feature("merge-reconstruction", "foreign-origin")
        copy_path = context.output("reports", "copied-origin.json")
        copy_path.write_text(json.dumps(observed), encoding="utf-8")
        changed = dict(actual, native_candidate_observation=identity(copy_path, ROOT))
        with self.assertRaisesRegex(ValueError, "roles disagree"):
            check_candidate_result(changed, ROOT)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--native-result", type=Path, required=True, help="Explicit57component observation for pure chain-reader tests only.")
    arguments = parser.parse_args()
    NATIVE_RESULT = arguments.native_result.resolve()
    unittest.main(argv=[sys.argv[0]], verbosity=2)
