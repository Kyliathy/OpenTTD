"""Check the prepared capture handoff without executing or fabricating native acceptance.

@file prepared_viewport_roles_check.py Pure transport fixtures and a mocked native reader only.
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Fixtures persist under test-cache/merge-evidence; no recursive cleanup is registered.
"""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[4]
sys.path[:0] = [str(ROOT / "src/tests/multilayer/support"), str(ROOT / "src/tests/multilayer")]
from artifact_rebuild import identity, load_recipe, snapshot
from artifact_recipe_stage import validated_prerequisite_runs
import space_source_review as review

DOC = "@see archi-historical-debug.md and r2fc-file-cleanup3.md; synthetic transport fixture, never native qualification."


class PreparedViewportRoles(unittest.TestCase):
    """Keep exact current source/selector ownership separate from fresh capture outputs."""

    def write(self, relative, value):
        """Persist one explicit transport fixture with its truthful byte identity.

        @param self Active isolated fixture owner.
        @param relative Repository-relative path beneath this persistent scratch root.
        @param value JSON metadata or source text; no fixture is executed.
        @return Exact path/hash identity.
        """
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(value, dict):
            value = dict(value)
            value.setdefault("_documentation", DOC)
            value = json.dumps(value, indent=2) + "\n"
        path.write_text(value, encoding="utf-8")
        return identity(path, self.root)

    def setUp(self):
        """Create one real pure-source signature and detached native-reader response.

        @param self Active unit owner; the native checker is mocked explicitly in every call.
        """
        cache = ROOT / "test-cache/merge-evidence"
        cache.mkdir(parents=True, exist_ok=True)
        self.root = Path(tempfile.mkdtemp(prefix="prepared-viewport-", dir=cache))
        self.write("CMakeLists.txt", "# @see archi-historical-debug.md; transport-only root.\n")
        self.write("src/tests/producer.py", '"""@file producer.py @see archi-historical-debug.md; never executed."""\n')
        # The actual source-signature owner must hash actual current shared
        # policy bytes, rather than a mocked success or guessed source digest.
        for name in ("artifact_recipe_stage.py", "artifact_rebuild.py", "artifact_recipes.py",
                     "artifact_paths.py", "artifact_inventory.py", "artifact_space_graph.py"):
            relative = "src/tests/multilayer/support/" + name
            self.write(relative, (ROOT / relative).read_text(encoding="utf-8"))
        declaration = dict(id="source-fixture", producer="src/tests/producer.py", feature="art", scenario="source",
            target=None, test_tags=[], tag_sources=[], execution="pure_conversion", enabled=True, pending_groups=[],
            publishes_source=False, stages=["inspect", "build", "prepare", "verify"], configuration_required=False,
            configuration_reuse="source_bytes_only", comparison="independent_source_bytes", tools=[], retained_inputs=[],
            requires=[], outputs=[dict(role="manifest", path="reports/source.json")], original_parameters={})
        self.write("mlm/review-producers.json", dict(recipe_schema_version=1, recipes=[declaration]))
        self.supplied = self.write("mlm/reviews/art/source/source-run/reports/source.json", dict(kind="transport_source_fixture"))
        self.stage_path = "mlm/reviews/art/source/source-run/reports/stage.json"
        self.stage = dict(kind="artifact_recipe_stage_result", complete=True, outputs=dict(manifest=self.supplied))
        stage_reference = self.write(self.stage_path, self.stage)
        self.source = dict(kind="artifact_rebuild_result", complete=True, status="verified", run_id="source-run",
            signature=snapshot(load_recipe("source-fixture", root=self.root), self.root),
            nodes={"source-fixture": dict(status="verified", outputs=dict(manifest=self.supplied),
                stages=dict(verify=dict(status="passed", result=stage_reference)))})
        self.source_path = "mlm/reviews/rebuild/source-fixture/source-run/reports/rebuild.json"
        self.decoder_path = "mlm/reviews/art/decoder/source-run/reports/transport.json"
        self.selector_reference = self.write("mlm/reviews/space/launch-current-producers/source-run/reports/transport.json",
                                             dict(kind="transport_selector_fixture"))
        self.build = dict(path="mlm/reviews/build/fixture/build-run/reports/transport.json", sha256="0" * 64)
        self.selector = dict(run_id="source-run", build_result=self.build)
        self.recipe = dict(id="launch-occupied-viewports", requires=[
            dict(recipe="launch-current-producers", role="manifest", **{"as": "source-selector"}),
            dict(recipe="source-fixture", role="manifest", **{"as": "facility-source"})])
        self.request = dict(configuration="Debug", build=dict(result=self.build), inputs={
            "source-selector": self.selector_reference, "facility-source": self.supplied})
        self.rewrite_source()

    def rewrite_source(self):
        """Update truthful source/decoder fixture references without inventing execution.

        @param self Active unit owner whose explicitly synthetic state was changed.
        """
        self.request["prepared_source"] = self.write(self.source_path, self.source)
        self.selector["decoder_prerequisite"] = self.write(self.decoder_path,
            dict(kind="transport_decoder_fixture", source_generation=self.request["prepared_source"]))

    def inspect(self, family="launch"):
        """Exercise real source/role policy while replacing only native qualification.

        @param self Active unit owner; returned mapping remains transport metadata only.
        @param family Actual registered launch or Ground selector boundary exercised by this fixture.
        @return Exact supplier-run map from the actual dispatcher helper.
        """
        with patch.object(review, "checked_result", return_value=self.selector) as native:
            result = validated_prerequisite_runs(self.request, self.recipe, self.root)
        native.assert_called_once_with(self.selector_reference, family, "Debug", self.root)
        return result

    def test_exact_source_and_current_selector_bind_only_declared_inputs(self):
        """Permit the exact two prerequisites without granting ownership of a whole old run.

        @param self Active transport unit owner.
        """
        self.assertEqual(self.inspect(), {(row["path"], row["sha256"]): "source-run"
                                         for row in self.request["inputs"].values()})

    def test_incomplete_source_graph_cannot_claim_current_inputs(self):
        """Refuse a plausible identity after source completion is explicitly revoked.

        @param self Active transport unit owner.
        """
        self.source["complete"] = False
        self.rewrite_source()
        with self.assertRaisesRegex(ValueError, "completed pure source"):
            self.inspect()

    def test_ground_capture_uses_the_same_exact_supplier_boundary(self):
        """Exercise Ground's existing registered adapter and its own typed selector reader.

        @param self Active transport unit owner.
        """
        self.recipe["id"] = "ground-viewport-collector"
        self.recipe["requires"][0]["recipe"] = "ground-current-producers"
        self.selector_reference = self.write("mlm/reviews/selected-ground/current-source-consumers/source-run/reports/transport.json",
                                             dict(kind="transport_selector_fixture"))
        self.request["inputs"]["source-selector"] = self.selector_reference
        self.assertEqual(len(self.inspect("ground")), 2)

    def test_other_adapters_receive_no_cross_run_allowance(self):
        """Preserve ordinary same-run ownership for every unrelated adapter.

        @param self Active transport unit owner.
        """
        self.recipe["id"] = "unregistered-fixture"
        with patch.object(review, "checked_result") as native:
            self.assertEqual(validated_prerequisite_runs(self.request, self.recipe, self.root), {})
        native.assert_not_called()

    def test_current_producer_change_invalidates_the_old_source_graph(self):
        """Refuse a re-readable old graph after its producing source bytes change.

        @param self Active transport unit owner.
        """
        self.write("src/tests/producer.py", '"""@see archi-historical-debug.md; changed transport-only producer."""\n')
        with self.assertRaisesRegex(ValueError, "inputs/code/declarations changed"):
            self.inspect()

    def test_native_result_cannot_replace_a_pure_source_role(self):
        """Keep source bytes bound to their own producing recipe and output role.

        @param self Active transport unit owner.
        """
        self.request["inputs"]["facility-source"] = self.selector_reference
        with self.assertRaisesRegex(ValueError, "pure prerequisite recipe/role/run changed"):
            self.inspect()

    def test_selector_must_use_the_actual_capture_build(self):
        """Refuse old-image selector metadata even when its source run still matches.

        @param self Active transport unit owner.
        """
        self.selector["build_result"] = dict(path="build/old.json", sha256="1" * 64)
        with self.assertRaisesRegex(ValueError, "selector/source/build handoff disagrees"):
            self.inspect()

    def test_selector_must_use_the_exact_completed_source_run(self):
        """Refuse a different invocation rather than accepting equal-looking source labels.

        @param self Active transport unit owner.
        """
        self.selector["run_id"] = "old-run"
        with self.assertRaisesRegex(ValueError, "selector/source/build handoff disagrees"):
            self.inspect()

    def test_native_checker_refusal_cannot_be_replaced_by_a_complete_flag(self):
        """Require the actual typed native checker before granting any cross-run role.

        @param self Active transport unit owner.
        """
        with patch.object(review, "checked_result", side_effect=ValueError("actual native proof refused")):
            with self.assertRaisesRegex(ValueError, "actual native proof refused"):
                validated_prerequisite_runs(self.request, self.recipe, self.root)

    def test_node_output_must_equal_the_actual_passed_stage_output(self):
        """Refuse a hand-edited node whose output differs from its actual stage witness.

        @param self Active transport unit owner.
        """
        different = self.write("mlm/reviews/art/source/source-run/data/different.json", dict(kind="transport_other_fixture"))
        self.stage["outputs"]["manifest"] = different
        self.source["nodes"]["source-fixture"]["stages"]["verify"]["result"] = self.write(self.stage_path, self.stage)
        self.rewrite_source()
        with self.assertRaisesRegex(ValueError, "differs from its passed stage"):
            self.inspect()


if __name__ == "__main__":
    unittest.main(verbosity=2)
