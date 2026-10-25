"""Check separate actual-candidate and expected-pixel process bindings.

@file candidate_baseset_check.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Uses the explicit52/53source handoff and actual path-owner probe. No game,
canonical publication, saved-world tick or recursive fixture cleanup occurs.
"""
import argparse
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
import uuid

from artifact_paths import RunContext
from artifact_rebuild import identity
from sprite_expected_payload import checked_expectation_bundle
from sprite_native_inputs import bind_candidate_baseset, bind_expected_payload, observe_candidate_baseset
import runner

ROOT = Path(__file__).resolve().parents[4]
BUNDLE = PROBE = None


class CandidateBasesetChecks(unittest.TestCase):
    """Refuse foreign/changed candidates independently of valid source pixel expectations."""

    @classmethod
    def setUpClass(cls):
        """Validate the explicit source phase bundle once for this focused transport burst.

        @param cls Active source/candidate binding suite.
        """
        raw = json.loads(BUNDLE.read_text(encoding="utf-8"))
        cls.run_id = raw["run_id"]
        cls.bundle = checked_expectation_bundle(identity(BUNDLE, ROOT), RunContext(ROOT, "multilayer-art", "candidate-check", cls.run_id))

    def setUp(self):
        """Reserve a unique feature/process under the already explicit source invocation.

        @param self Active persistent isolated candidate fixture.
        """
        scenario = "candidate-check-" + uuid.uuid4().hex
        self.context = RunContext(ROOT, "multilayer-art", scenario, self.run_id, "packed-art", "probe").for_feature("multilayer-art", scenario)

    def prepare(self):
        """Prepare actual candidate bytes and an independent original M1expectation binding.

        @param self Current process owner; all writes remain inside its generated runtime.
        @return Complete dual-input environment for the independently compiled Debug routing probe.
        """
        self.binding = self.context.prepare_candidate_baseset(self.bundle["actual_candidate"], self.bundle["actual_checksum"], "Debug")
        environment = bind_expected_payload(self.context, "m1", "Debug", self.bundle["families"]["m1"]["manifest"])
        return bind_candidate_baseset(self.context, self.binding, "Debug", environment)

    def test_actual_process_records_candidate_and_expected_as_distinct_inputs(self):
        """The actual shared native input owner sees expected pixels while transport records both inputs.

        @param self Active real routing-probe check; it grants no native sprite decoding approval.
        """
        before = identity(ROOT / "media/baseset/openttd.grf", ROOT)
        environment = self.prepare()
        result = runner.run_native(self.context, [PROBE, "expected", "m1"], env=environment,
                                   capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stderr)
        process = json.loads((ROOT / result.native_report["path"]).read_text())
        self.assertEqual([row["role"] for row in process["bound_inputs"]], ["expected_sprites", "candidate_grf"])
        self.assertIn(self.bundle["actual_candidate"], process["inputs"])
        self.assertIn(self.bundle["actual_checksum"], process["inputs"])
        self.assertTrue(process["input_integrity_passed"])
        self.assertEqual(identity(ROOT / "media/baseset/openttd.grf", ROOT), before)
        self.assertFalse(json.loads(self.binding.read_text())["native_loading_verified"])

    def test_wrong_candidate_and_missing_companion_refuse_before_runtime_install(self):
        """A valid expectation cannot supply wrong actual GRFbytes or an absent checksum companion.

        @param self Active actual candidate source admission refusal check.
        """
        wrong = dict(self.bundle["actual_candidate"], sha256="0" * 64)
        with self.assertRaisesRegex(ValueError, "source/run/hash"):
            self.context.prepare_candidate_baseset(wrong, self.bundle["actual_checksum"], "Debug")
        missing = dict(self.bundle["actual_checksum"], path=self.bundle["actual_checksum"]["path"] + ".missing")
        with self.assertRaisesRegex(ValueError, "source/run/hash"):
            self.context.prepare_candidate_baseset(self.bundle["actual_candidate"], missing, "Debug")

    def test_changed_runtime_candidate_refuses_before_child_launch(self):
        """Exact expected source pixels cannot excuse a changed copied GRF in the process home.

        @param self Active runtime-container integrity refusal check.
        """
        environment = self.prepare()
        target = self.context.runtime_dir / "baseset/openttd.grf"
        payload = bytearray(target.read_bytes())
        payload[-1] ^= 1
        target.write_bytes(payload)
        with patch.object(runner, "launch_owned_process", side_effect=AssertionError("changed candidate launched a child")):
            with self.assertRaisesRegex(ValueError, "candidate source/runtime/companion"):
                runner.run_native(self.context, [PROBE, "expected", "m1"], env=environment, capture_output=True, timeout=20)

    def test_missing_descriptor_and_wrong_process_or_configuration_refuse(self):
        """Candidate companions, process homes and independent configurations remain required roles.

        @param self Active descriptor/process/configuration boundary refusal check.
        """
        environment = self.prepare()
        with self.assertRaisesRegex(ValueError, "run/process/configuration"):
            observe_candidate_baseset(self.context, environment, "RelWithDebInfo")
        foreign = self.context.for_process("foreign")
        with self.assertRaisesRegex(ValueError, "run/process/configuration"):
            observe_candidate_baseset(foreign, environment, "Debug")
        descriptor = self.context.runtime_dir / "baseset/orig_dos.obg"
        descriptor.rename(descriptor.with_suffix(".absent"))
        with self.assertRaises(OSError):
            observe_candidate_baseset(self.context, environment, "Debug")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--expectations", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    arguments = parser.parse_args()
    BUNDLE, PROBE = arguments.expectations.resolve(), arguments.probe.resolve()
    unittest.main(argv=[sys.argv[0]], verbosity=2)
