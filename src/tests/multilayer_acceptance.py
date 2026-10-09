"""Focused evidence-boundary refusal checks for the real ground acceptance runner.

@file multilayer_acceptance.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-integrated-acceptance-and-balance.md Group5
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint11.1
"""

import importlib.util
from pathlib import Path
import sys
import unittest
import uuid

ROOT = Path(__file__).resolve().parents[2]
specification = importlib.util.spec_from_file_location("ground_acceptance_owner",
    ROOT / "src/tests/multilayer/acceptance/native_ground.py")
owner = importlib.util.module_from_spec(specification)
specification.loader.exec_module(owner)


class GroundEvidenceBoundary(unittest.TestCase):
    """Real failure and provenance checks; passing synthetic logs cannot replace executable scenario acceptance."""

    def test_failed_command_keeps_failure(self):
        """Verify a real child failure stays a refusal with retained exact process output.

        @param self Test instance.
        @return None; a nonzero child cannot produce a passing command record.
        """
        log = owner.CONTEXT.logs / ("r2mlvs3mp-z-a-runner-failed-command-" + uuid.uuid4().hex + ".log")
        with self.assertRaisesRegex(owner.EvidenceError, "status 7"):
            owner.execute_command([sys.executable, "-c", "print('actual child refusal'); raise SystemExit(7)"],
                                  ROOT, log)
        self.assertIn("actual child refusal", log.read_text(encoding="utf-8"))

    def test_missing_artifact_refuses(self):
        """Require an actual source artifact instead of an expected or stale-output placeholder.

        @param self Test instance.
        @return None; the absent native save cannot become a resource ledger.
        """
        with self.assertRaisesRegex(owner.EvidenceError, "required artifact missing"):
            owner.capture_ledger(["src/tests/fixtures/haulage/missing-" + uuid.uuid4().hex + ".sav"])

    def test_seeded_bootstrap_refuses(self):
        """Reject bootstrap claims for the known declared development setup.

        @param self Test instance.
        @return None; actual boundary fixtures remain usable without being relabelled bootstrap.
        """
        with self.assertRaisesRegex(owner.EvidenceError, "bootstrap claim refused"):
            owner.validate_fixture("S1", None, True)
        with self.assertRaisesRegex(owner.EvidenceError, "metadata differs"):
            owner.validate_fixture("S2", {"classification": "real_bootstrap", "shortcuts": []}, True)
        self.assertEqual(owner.validate_fixture("S1", None, False)["classification"], "isolated_boundary")

    def test_unimplemented_scenario_refuses(self):
        """Keep the skipped tube producer absence visible to direct runner consumers.

        @param self Test instance.
        @return None; a missing operational capsule scenario never returns passed.
        """
        with self.assertRaisesRegex(owner.EvidenceError, "no delivered owner"):
            owner.validate_fixture("S11", None, False)

    def test_repository_escape_refuses(self):
        """Prevent a scenario artifact from redirecting the runner into a user-wide store.

        @param self Test instance.
        @return None; read/write ownership remains inside this authorized repository.
        """
        with self.assertRaisesRegex(owner.EvidenceError, "escapes repository"):
            owner.repository_path("../outside-acceptance.sav")

    def test_different_clocks_are_not_matching_state(self):
        """Compare two real native phases and require their saved owner/clock difference to refuse equality.

        @param self Test instance.
        @return None; mid-action and completed worlds cannot masquerade as matching-frame replay.
        """
        with self.assertRaisesRegex(owner.EvidenceError, "owner/clock mismatch"):
            owner.compare_state_digest("src/tests/fixtures/haulage/source-loop/source-native-deep-stopped.sav",
                                       "src/tests/fixtures/haulage/source-loop/source-native-deep-view-headless-complete-loop.sav")


if __name__ == "__main__":
    unittest.main()
