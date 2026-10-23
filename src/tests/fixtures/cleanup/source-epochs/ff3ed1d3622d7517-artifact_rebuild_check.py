"""Focused real-child recipe restart, dependency and source-scope checks.

@file artifact_rebuild_check.py No native game, paid workflow or source publication.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group14.
Synthetic catalogues exercise the actual executor with a real bounded text
producer and independently supplied expectations; real recipes remain disabled.
"""

import copy
import json
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch

import artifact_rebuild as rebuild
import artifact_recipe_stage as dispatcher
from artifact_rebuild_fixture import validate_fixture_stage

ROOT = Path(__file__).resolve().parents[4]


class RecipeRebuildChecks(unittest.TestCase):
    """Exercise actual selected-closure execution and exact interrupted-run identity.

    @see r2fc-file-cleanup3.md Group14 explicit restart and refusal boundaries.
    """

    def setUp(self):
        """Allocate an isolated source root containing one registered two-node test graph.

        @param self Active repository-local test.
        @return None; parent cleanup owns only this temporary root.
        """
        cache = ROOT / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="recipe-restart-", dir=cache)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.producer = self.root / rebuild.DISPATCHER
        self.producer.parent.mkdir(parents=True)
        shutil.copyfile(Path(__file__).with_name("artifact_rebuild_fixture.py"), self.producer)
        shutil.copyfile(Path(rebuild.__file__), self.producer.with_name("artifact_rebuild.py"))
        # The isolated root carries actual shared validation/routing policy
        # bytes too, so restart checks exercise the same source authority.
        for name in ("artifact_recipes.py", "artifact_paths.py", "artifact_inventory.py", "artifact_space_graph.py"):
            shutil.copyfile(Path(__file__).with_name(name), self.producer.with_name(name))
        (self.root / "CMakeLists.txt").write_text("# @see r2fc-file-cleanup3.md14; synthetic root.\n", encoding="utf-8")
        self.input = self.root / "src/tests/fixtures/recipe-contract.json"
        self.input.parent.mkdir(parents=True)
        self.contract = dict(_documentation="Owner: r2fc-file-cleanup3.md14; independent text fixture.",
                             value="selected recipe", expected="SELECTED RECIPE")
        self.write_contract()
        base = dict(id="base", producer=rebuild.DISPATCHER, feature="fixture", scenario="base", target=None,
            test_tags=[], tag_sources=[], execution="pure_conversion", enabled=True, pending_groups=[], publishes_source=False,
            stages=["inspect", "build", "prepare", "verify"], configuration_required=False,
            configuration_reuse="source_bytes_only", comparison="independent_source_bytes", tools=[],
            retained_inputs=[dict(role="contract", path=self.input.relative_to(self.root).as_posix())], requires=[],
            outputs=[dict(role="payload", path="data/payload.txt"), dict(role="manifest", path="reports/product.json")],
            original_parameters={}, native_timeout_seconds=60)
        leaf = copy.deepcopy(base)
        leaf.update(id="leaf", scenario="leaf", requires=[dict(recipe="base", role="payload", **{"as": "prior-payload"})])
        self.catalogue = dict(_documentation="Owner: r2fc-file-cleanup3.md14; synthetic test graph only.",
                              schema_version=2, recipe_schema_version=1, producers=[], recipes=[base, leaf])
        self.catalogue_path = self.root / "mlm/review-producers.json"
        self.catalogue_path.parent.mkdir()
        self.write_catalogue()

    def write_contract(self):
        """Write the independent fixture contract before execution or deliberate tampering.

        @param self Active isolated test.
        @return None; no real project's input or source is modified.
        """
        self.input.write_text(json.dumps(self.contract), encoding="utf-8")

    def write_catalogue(self):
        """Publish only the isolated test's synthetic registered recipes.

        @param self Active test catalogue owner.
        @return None; the canonical real producer catalogue remains unchanged.
        """
        self.catalogue_path.write_text(json.dumps(self.catalogue), encoding="utf-8")

    def state(self, run_id):
        """Read one explicitly named generated invocation report.

        @param self Active test root owner.
        @param run_id Exact synthetic run identity.
        @return Actual executor state after success/failure, without latest-run lookup.
        """
        path = self.root / "mlm/reviews/rebuild/leaf" / run_id / "reports/rebuild.json"
        return json.loads(path.read_text(encoding="utf-8"))

    def fail_leaf(self, run_id):
        """Execute the base successfully and deliberately fail the leaf's first verify.

        @param self Active isolated test.
        @param run_id Explicit new invocation identity.
        @return Actual failed state, retaining the completed predecessor and prepare proof.
        """
        self.contract["fail_recipe"] = "leaf"
        self.write_contract()
        with self.assertRaisesRegex(ValueError, "leaf/verify failed"):
            rebuild.prepare_recipe("leaf", root=self.root, run_id=run_id)
        return self.state(run_id)

    def test_inspection_validates_graph_without_writes_or_child_processes(self):
        """Inspect exact declarations/inputs without importing or executing the producer.

        @param self Active isolated test.
        @return None; all write/process entry points are blocked during inspection.
        """
        with patch.object(Path, "mkdir", side_effect=AssertionError("inspection writes")), \
                patch.object(Path, "write_text", side_effect=AssertionError("inspection writes")), \
                patch("subprocess.Popen", side_effect=AssertionError("inspection runs child")):
            selected = rebuild.load_recipe("leaf", root=self.root)
        self.assertEqual(selected["order"], ["base", "leaf"])
        self.assertTrue(selected["executable"])
        self.assertFalse(selected["reserves_run_id"])

    def test_real_children_complete_ordered_graph_and_duplicate_ids_refuse(self):
        """Publish success only after both actual text checks and all declared outputs.

        @param self Active isolated test.
        @return None; no native gameplay or source asset is touched.
        """
        before = self.input.read_bytes()
        result = rebuild.prepare_recipe("leaf", root=self.root, run_id="success")
        self.assertEqual(result, rebuild.identity(self.root / result["path"], self.root))
        state = self.state("success")
        self.assertTrue(state["complete"])
        self.assertEqual([state["nodes"][name]["status"] for name in ("base", "leaf")], ["verified", "verified"])
        self.assertEqual(self.input.read_bytes(), before)
        with self.assertRaises(FileExistsError):
            rebuild.prepare_recipe("leaf", root=self.root, run_id="success")
        with self.assertRaisesRegex(ValueError, "already completed"):
            rebuild.prepare_recipe("leaf", root=self.root, resume="success")

    def test_explicit_resume_reuses_only_checked_completed_prerequisites(self):
        """Retry actual failed verification while retaining passed base and prepare outcomes.

        @param self Active isolated test.
        @return None; failed native-like child status cannot advance the graph.
        """
        failed = self.fail_leaf("retry")
        self.assertFalse(failed["complete"])
        self.assertEqual(failed["nodes"]["base"]["status"], "verified")
        self.assertEqual(failed["nodes"]["leaf"]["stages"]["verify"]["exit_status"], 7)
        rebuild.prepare_recipe("leaf", root=self.root, resume="retry")
        completed = self.state("retry")
        self.assertTrue(completed["complete"])
        self.assertEqual(len(completed["nodes"]["base"]["history"]), 2)
        self.assertEqual(len(completed["nodes"]["leaf"]["history"]), 3)
        self.assertEqual(completed["nodes"]["leaf"]["attempts"], dict(prepare=1, verify=2))

    def test_changed_input_producer_configuration_and_prerequisite_refuse(self):
        """Reject altered restart authority and changed physical predecessor output.

        @param self Active isolated test.
        @return None; refusal occurs before another producer subprocess launches.
        """
        failed = self.fail_leaf("changed")
        original_input, original_source = self.input.read_bytes(), self.producer.read_bytes()
        policy = self.producer.with_name("artifact_recipes.py")
        # Each change targets a separate restart authority. Restore between
        # probes so a prior fault cannot mask the next refusal boundary.
        for path, original in ((self.input, original_input), (self.producer, original_source), (policy, policy.read_bytes())):
            path.write_bytes(original + b" ")
            with patch("subprocess.Popen", side_effect=AssertionError("changed resume launched")), \
                    self.assertRaisesRegex(ValueError, "identity/source/input"):
                rebuild.prepare_recipe("leaf", root=self.root, resume="changed")
            path.write_bytes(original)
        with self.assertRaisesRegex(ValueError, "identity/source/input"):
            rebuild.prepare_recipe("leaf", "Debug", root=self.root, resume="changed")
        reference = failed["nodes"]["base"]["outputs"]["payload"]
        (self.root / reference["path"]).write_text("changed prerequisite", encoding="utf-8")
        with patch("subprocess.Popen", side_effect=AssertionError("bad prerequisite launched")), \
                self.assertRaisesRegex(ValueError, "prerequisite output changed"):
            rebuild.prepare_recipe("leaf", root=self.root, resume="changed")

    def test_snapshot_observes_shared_payload_once_and_rehashes_next_epoch(self):
        """Deduplicate one pass without reusing stale bytes or conflicting declared identities.

        @param self Active isolated source snapshot test.
        @return None; only private temporary fixtures change between observations.
        """
        selection = rebuild.load_recipe("leaf", root=self.root)
        with patch.object(rebuild, "identity", wraps=rebuild.identity) as observe:
            first = rebuild.snapshot(selection, self.root)
        calls = [call for call in observe.call_args_list if Path(call.args[0]) == self.input]
        self.assertEqual(len(calls), 1)
        self.contract["value"] = "next independent input epoch"
        self.write_contract()
        second = rebuild.snapshot(selection, self.root)
        self.assertNotEqual(first["inputs"]["base"], second["inputs"]["base"])
        selection["recipes"]["leaf"]["retained_inputs"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "declared identity"):
            rebuild.snapshot(selection, self.root)

    def test_snapshot_refuses_input_and_alias_changes_during_observation(self):
        """Close every unique input and alias authority after an observation pass.

        @param self Active isolated concurrent-input refusal check.
        @return None; injected mutations affect only this test's temporary repository tree.
        """
        selection = rebuild.load_recipe("leaf", root=self.root)
        identity = rebuild.identity
        change_alias = False

        def changed_input(path, root):
            """Mutate a private input epoch immediately after measuring its old bytes.

            @param path Actual path requested by the snapshot observer.
            @param root Exact isolated source root.
            @return Actual original observation; the closing guard must reject subsequent changes.
            """
            result = identity(path, root)
            if Path(path) == self.input:
                if change_alias:
                    alias = self.input.parent / "multilayer-inputs.json"
                    alias.write_text(json.dumps(dict(_documentation="Owner: r2fc-file-cleanup3.md17DE; isolated alias epoch.",
                                                     schema_version=2, inputs={})), encoding="utf-8")
                else:
                    self.contract["value"] = "concurrent private input change"
                    self.write_contract()
            return result

        with patch.object(rebuild, "identity", side_effect=changed_input):
            with self.assertRaisesRegex(ValueError, "changed while collecting snapshot"):
                rebuild.snapshot(selection, self.root)
        change_alias = True
        with patch.object(rebuild, "identity", side_effect=changed_input):
            with self.assertRaisesRegex(ValueError, "alias catalogue changed"):
                rebuild.snapshot(selection, self.root)

    def test_zero_status_without_measured_result_never_succeeds(self):
        """A no-op child cannot turn preexisting paths into fresh verification.

        @param self Active isolated test.
        @return None; actual missing stage result leaves the invocation incomplete.
        """
        self.contract["no_op"] = True
        self.write_contract()
        with self.assertRaises(FileNotFoundError):
            rebuild.prepare_recipe("leaf", root=self.root, run_id="no-op")
        self.assertFalse(self.state("no-op")["complete"])
        self.assertEqual(self.state("no-op")["status"], "failed")

    def test_cycles_source_publication_and_paid_execution_are_excluded(self):
        """Reject unsupported work modes before reservation, even in a registered graph.

        @param self Active isolated test.
        @return None; no recipe flag can grant paid scope or source publication.
        """
        original = copy.deepcopy(self.catalogue)
        self.catalogue["recipes"][0]["requires"] = [dict(recipe="leaf", role="payload", **{"as": "cycle"})]
        self.write_catalogue()
        with self.assertRaisesRegex(ValueError, "cyclic"):
            rebuild.load_recipe("leaf", root=self.root)
        for field, value, message in (("publishes_source", True, "cannot publish"), ("execution", "paid_work", "unknown recipe execution")):
            self.catalogue = copy.deepcopy(original)
            self.catalogue["recipes"][0][field] = value
            self.write_catalogue()
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, message):
                rebuild.prepare_recipe("leaf", root=self.root, run_id="forbidden")
        self.assertFalse((self.root / "mlm/runtime/.runs/forbidden").exists())

    def test_running_owner_cannot_be_reclaimed_and_stopped_lock_requires_resume(self):
        """Keep explicit restart from racing a still active supervisor or child.

        @param self Active isolated test.
        @return None; process identity checks never terminate any process.
        """
        lock = self.root / "execution.lock"
        rebuild.acquire_lock(lock, False)
        with self.assertRaisesRegex(ValueError, "active execution owner"):
            rebuild.acquire_lock(lock, True)
        lock.write_text(json.dumps(dict(owner_pid=None, child_pid=None)), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "active execution owner"):
            rebuild.acquire_lock(lock, False)
        rebuild.acquire_lock(lock, True)

    def test_dispatcher_binds_real_checks_and_refuses_disabled_direct_requests(self):
        """Exercise production result publication and its direct-call registration guard.

        @param self Active isolated actual-handler test, never a gameplay qualification.
        @return None; retained-input changes and pending declarations cannot dispatch.
        """
        selection = rebuild.load_recipe("base", root=self.root)
        signature = rebuild.snapshot(selection, self.root)
        context = rebuild.create_run_context("fixture", "base", "dispatcher-case", self.root)
        outputs = {row["role"]: (context.directory / row["path"]).relative_to(self.root).as_posix()
                   for row in selection["recipes"]["base"]["outputs"]}
        request = dict(schema_version=1, kind="artifact_recipe_stage_request", recipe="base", stage="verify",
            declaration=selection["recipes"]["base"], configuration=None, run_id=context.run_id,
            inputs=signature["inputs"]["base"], sources=signature["sources"], outputs=outputs,
            result_path=(context.reports / "actual-result.json").relative_to(self.root).as_posix(),
            publishes_source=False, paid_work_authorized=False)
        path = context.reports / "request.json"
        path.write_text(json.dumps(request), encoding="utf-8")
        with patch.dict(dispatcher.HANDLERS, dict(base=validate_fixture_stage), clear=True):
            result = dispatcher.execute_request(path, self.root)
        self.assertTrue(result["complete"])
        self.assertEqual(set(result["outputs"]), {"payload", "manifest"})
        original = self.input.read_bytes()
        self.input.write_bytes(original + b" ")
        with self.assertRaisesRegex(ValueError, "input/source changed"):
            dispatcher.execute_request(path, self.root)
        self.input.write_bytes(original)
        self.catalogue["recipes"][0].update(enabled=False, pending_groups=[999])
        self.write_catalogue()
        with self.assertRaisesRegex(ValueError, "unregistered, changed or pending"):
            dispatcher.execute_request(path, self.root)


if __name__ == "__main__":
    unittest.main(verbosity=2)
