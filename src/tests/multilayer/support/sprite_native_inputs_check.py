"""Verify actual Python/C++ sprite input binding without starting game/world code.

@file sprite_native_inputs_check.py Requires the actual shared-owner probe and selected source run.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group18B.
Probe observations qualify input ownership only; actual native decoder checks follow18C.
"""

import argparse
import copy
import json
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch
import uuid

from artifact_paths import RunContext, contained
from artifact_rebuild import identity
import runner
import sprite_expected_payload as source
from sprite_native_inputs import PREFIX, bind_expected_payload, observe_expected_payload

APPLICATION = RUN_ID = RECIPE = CONFIGURATION = EXPECTATIONS = None


class SpriteNativeInputChecks(unittest.TestCase):
    """Exercise actual shared native input guards and exact supplied-input process provenance."""

    @classmethod
    def setUpClass(cls):
        """Select actual completed source reports from one explicit shared-executor result.

        @param cls Active input-boundary suite; no latest lookup or old EXE selection occurs.
        @return None; the supplied probe is explicitly built separately from tests.
        """
        path = source.ROOT / "mlm/reviews/rebuild" / RECIPE / RUN_ID / "reports/rebuild.json"
        state = json.loads(path.read_text(encoding="utf-8"))
        # Only complete actual source witnesses may become current native inputs.
        # File existence or an old native success flag cannot supply this boundary.
        if state["complete"] is not True or state["status"] != "verified" or state["run_id"] != RUN_ID:
            raise ValueError("native input checks require the explicit completed source recipe")
        cls.packing = state["nodes"]["m1-derived-inputs"]["outputs"]["manifest"]
        if EXPECTATIONS is not None:
            context = RunContext(source.ROOT, "multilayer-art", "sprite-input-suppliers", RUN_ID)
            bundle = source.checked_expectation_bundle(identity(EXPECTATIONS, source.ROOT), context)
            if bundle["source_result"] != identity(path, source.ROOT):
                raise ValueError("native input expectation bundle has a different source supplier")
            cls.manifests = {family: result["manifest"] for family, result in bundle["families"].items()}
            return
        cls.manifests = {}
        for family in source.load_facts():
            node = state["nodes"][source.recipe_family(family) + "-expected-payload"]
            if node["status"] != "verified":
                raise ValueError("native input source supplier is incomplete")
            cls.manifests[family] = node["outputs"]["manifest"]

    def setUp(self):
        """Create a fresh feature view so each input test owns exact process/report outputs.

        @param self Active contained input-boundary check.
        @return None; the actual source supplier's output bytes are never modified.
        """
        self.context = RunContext(source.ROOT, "multilayer-art", "sprite-input-check-" + uuid.uuid4().hex, RUN_ID)
        # The test owns deliberate data/report fixtures and actual transcripts;
        # immutable RunContext path selection itself creates no output folders.
        for directory in (self.context.data, self.context.reports, self.context.evidence):
            directory.mkdir(parents=True, exist_ok=True)
        self.environment = bind_expected_payload(self.context, "m1", CONFIGURATION, self.manifests["m1"])

    def direct_probe(self, environment, family="m1"):
        """Exercise the actual compiled C++ owner independently of Python preflight.

        @param self Active native/Python parity check.
        @param environment Exact selected child environment, including deliberate refusal fields.
        @param family Native case's expected original protocol family.
        @return Actual bounded probe result; this process links no game or decoder code.
        """
        return subprocess.run([str(APPLICATION), "expected", family], env=environment,
                              capture_output=True, text=True, timeout=60, creationflags=subprocess.CREATE_NO_WINDOW)

    def test_all_families_match_native_owner_and_process_inputs(self):
        """Run actual C++ selection for every family and inspect exact non-save provenance.

        @param self Active real shared-owner acceptance check.
        @return None; source input acceptance grants no actual sprite loading qualification.
        """
        # Each binary protocol has its own original family/recipe. Running the
        # actual shared C++ owner catches disagreement in family naming, path
        # containment, source flags, SHA256 and compiled configuration.
        for family, manifest in self.manifests.items():
            environment = bind_expected_payload(self.context, family, CONFIGURATION, manifest)
            result = runner.run_native(self.context, [str(APPLICATION), "expected", family], env=environment,
                                       capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            observed = observe_expected_payload(self.context, environment, CONFIGURATION)
            self.assertEqual(Path(json.loads(result.stdout)["path"]), source.ROOT / observed["path"])
            report = json.loads((source.ROOT / result.native_report["path"]).read_text(encoding="utf-8"))
            self.assertTrue(report["input_integrity_passed"])
            self.assertEqual(report["bound_inputs"], [observed])
            self.assertIn({key: observed[key] for key in ("path", "sha256")}, report["inputs"])
            self.assertEqual(report["executable"], identity(APPLICATION, source.ROOT))
            self.assertEqual(report["configuration"], CONFIGURATION)

    def test_python_refuses_partial_changed_family_and_configuration_before_launch(self):
        """Reject invalid selected bindings before starting any native process.

        @param self Active Python launch boundary check.
        @return None; original source reports, payloads and probe image stay unchanged.
        """
        changed = dict(self.environment)
        changed.pop(PREFIX + "_MANIFEST")
        bad_hash = dict(self.environment, **{PREFIX + "_SHA256": "0" * 64})
        wrong_family = dict(self.environment, **{PREFIX + "_FAMILY": "space"})
        wrong_configuration = dict(self.environment, **{PREFIX + "_CONFIGURATION": "foreign"})
        # Every malformed current binding must fail before subprocess execution;
        # invoking a child and relying on a later report would leave inputs unbound.
        for environment in (changed, bad_hash, wrong_family, wrong_configuration):
            with patch.object(runner.subprocess, "run", side_effect=AssertionError("invalid input launched a child")):
                with self.assertRaises(ValueError):
                    runner.run_native(self.context, [str(APPLICATION), "expected", "m1"], env=environment)
        with self.assertRaisesRegex(ValueError, "supported configuration"):
            bind_expected_payload(self.context, "m1", "Release", self.manifests["m1"])
        opposite = "Debug" if CONFIGURATION == "RelWithDebInfo" else "RelWithDebInfo"
        with self.assertRaisesRegex(ValueError, "conflicts"):
            bind_expected_payload(self.context, "m1", CONFIGURATION, self.manifests["m1"], {"OTTD_TEST_CONFIGURATION": opposite})
        with self.assertRaisesRegex(ValueError, "actual executable"):
            observe_expected_payload(self.context, self.environment, opposite)

    def test_actual_native_owner_refuses_missing_foreign_family_configuration_and_hash(self):
        """Bypass Python deliberately to prove the compiled owner repeats critical guards.

        @param self Active actual native input refusal check.
        @return None; no native game startup, source publication or world operation occurs.
        """
        missing = dict(self.environment)
        missing.pop(PREFIX + "_MANIFEST")
        foreign = dict(self.environment, **{PREFIX: str(source.ROOT / "media/baseset/openttd/map_layers.png")})
        wrong_family = dict(self.environment, **{PREFIX + "_FAMILY": "space"})
        bad_hash = dict(self.environment, **{PREFIX + "_SHA256": "0" * 64})
        opposite = "Debug" if CONFIGURATION == "RelWithDebInfo" else "RelWithDebInfo"
        wrong_configuration = dict(self.environment, **{PREFIX + "_CONFIGURATION": opposite, "OTTD_TEST_CONFIGURATION": opposite})
        # Direct native callers cannot bypass the Python guard. Keep concrete
        # actual refusal transcripts under this current test's evidence owner.
        for label, environment in (("missing", missing), ("foreign-owner", foreign), ("family", wrong_family),
                                   ("hash", bad_hash), ("configuration", wrong_configuration)):
            result = self.direct_probe(environment)
            self.assertNotEqual(result.returncode, 0, label)
            self.assertTrue(result.stderr.strip(), label)
            self.context.output("evidence", "input-refusal-" + label + ".txt").write_text(result.stderr, encoding="utf-8")

    def test_actual_native_owner_refuses_rehashed_source_report_claims(self):
        """Refuse coherent recipe/status metadata changes even with exact binary bytes.

        @param self Active actual native typed-source boundary check.
        @return None; all deliberate report/data copies are owned disposable test fixtures.
        """
        payload = source.ROOT / observe_expected_payload(self.context, self.environment, CONFIGURATION)["path"]
        copied = self.context.output("data", "source-copy.bin")
        copied.write_bytes(payload.read_bytes())
        original = json.loads((source.ROOT / self.manifests["m1"]["path"]).read_text(encoding="utf-8"))
        # Rehash each metadata fault so native source semantics, rather than a
        # stale file hash, must reject the wrong recipe or claimed native status.
        for key, value in (("recipe", "space-expected-payload"), ("native_loading_verified", True)):
            document = copy.deepcopy(original)
            document.update(_documentation="Owner: r2fc-file-cleanup3.md18B; deliberate current native input fault.",
                            expected_payload=identity(copied, source.ROOT))
            document[key] = value
            report = self.context.output("reports", "fault-" + key + ".json")
            report.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
            environment = dict(self.environment, **{PREFIX: str(copied), PREFIX + "_MANIFEST": str(report),
                PREFIX + "_MANIFEST_SHA256": identity(report, source.ROOT)["sha256"]})
            result = self.direct_probe(environment)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("source report", result.stderr)

    def test_changed_input_during_actual_child_retains_failed_process_evidence(self):
        """Detect a deliberate actual child mutation of its own disposable source payload.

        @param self Active runner closing-integrity check, with no game or decoder invocation.
        @return None; original supplying source outputs stay exact and failed evidence is retained.
        """
        produced = source.produce("m1", self.context, self.packing)
        environment = bind_expected_payload(self.context, "m1", CONFIGURATION, produced["manifest"])
        # This real Python child is an intentional corruption fixture, never a
        # native loading proof. It may alter only the actual source payload just
        # generated under this fresh test scenario; original suppliers are untouched.
        child = Path(__file__).with_name("sprite_input_corruption_child.py")
        with self.assertRaisesRegex(ValueError, "changed during execution"):
            runner.run_native(self.context, [sys.executable, str(child)], env=environment,
                              capture_output=True, text=True, timeout=60)
        reports = list(self.context.reports.glob("native-*.json"))
        self.assertEqual(len(reports), 1)
        record = json.loads(reports[0].read_text(encoding="utf-8"))
        self.assertFalse(record["input_integrity_passed"])
        self.assertIn("input_integrity_error", record)
        self.assertEqual(record["bound_inputs"][0]["sha256"], source.load_facts()["m1"]["sha256"])
        self.assertTrue(record["transcripts"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--application", type=Path, required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--recipe", default="ground-expected-payload")
    parser.add_argument("--expectations", type=Path, help="Exact current expectation bundle continuing the completed source graph.")
    arguments = parser.parse_args()
    APPLICATION = contained(arguments.application, source.ROOT)
    CONFIGURATION, RUN_ID, RECIPE = arguments.configuration, arguments.run_id, arguments.recipe
    EXPECTATIONS = contained(arguments.expectations, source.ROOT) if arguments.expectations is not None else None
    unittest.main(argv=[sys.argv[0]], verbosity=2)
