"""Refuse altered or incomplete genuine-bootstrap claims through the existing public evidence owner.

@file normal_ground_bootstrap_checks.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DEC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Negative metadata copies consume the real complete proof;no native world is fabricated or changed.
"""

import copy
import importlib.util
import json
from pathlib import Path
import sys
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
from normal_ground_bootstrap_proof import verify_proof_payload

spec = importlib.util.spec_from_file_location("genuine_ground_acceptance_owner", ROOT / "src/tests/multilayer/acceptance/native_ground.py")
owner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(owner)


class GenuineBootstrapBoundaries(unittest.TestCase):
    """Actual proof/lineage/cash completeness and explicit scenario classification cannot be bypassed."""

    @classmethod
    def setUpClass(cls):
        """Load the actual complete original-world prerequisite artifact for independent corruption tests."""
        cls.path = owner.NORMAL_START_PROOF["path"]
        cls.proof = json.loads((ROOT / cls.path).read_text(encoding="utf-8"))

    def test_older_partial_proof_refuses(self):
        """A genuine newgame foundation alone cannot stand in for completed physical service/recovery."""
        with self.assertRaisesRegex(owner.EvidenceError, "exact completed genuine predecessor graph"):
            owner.run_scenario("S1", bootstrap_claim=True,
                               normal_start_proof="logs/acceptance/bootstrap/20261019-native-01/normal-start-verification.json")

    def test_unselected_complete_candidate_refuses(self):
        """An earlier prerequisite draft cannot silently replace the exact fully checked selected artifact."""
        with self.assertRaisesRegex(owner.EvidenceError, "exact completed genuine predecessor graph"):
            owner.run_scenario("S1", bootstrap_claim=True,
                               normal_start_proof="logs/acceptance/bootstrap/20261019-native-01/normal-start-prerequisites.json")

    def test_false_fixture_metadata_refuses(self):
        """An explicit proof does not authorize caller-supplied fixture relabelling or missing binding."""
        with self.assertRaisesRegex(owner.EvidenceError, "fixture metadata differs"):
            owner.run_scenario("S2", fixture={"classification": "real_bootstrap", "shortcuts": []},
                               bootstrap_claim=True, normal_start_proof=self.path)

    def test_unproven_scenario_claim_and_presentation_refuse(self):
        """Headless S1/S2 verification cannot become another scenario,implicit claim or GUI presentation."""
        for scenario, claim, presentation, message in (
                ("S8", True, "headless", "requires S1/S2"),
                ("S1", False, "headless", "explicit bootstrap claim"),
                ("S1", True, "hidden", "headless presentation")):
            with self.subTest(scenario=scenario, claim=claim, presentation=presentation), self.assertRaisesRegex(owner.EvidenceError, message):
                owner.run_scenario(scenario, bootstrap_claim=claim, presentation=presentation, normal_start_proof=self.path)

    def test_actual_payload_scope_lineage_and_cost_corruption_refuse(self):
        """Real incomplete dependencies,wrong original world and one-unit cash discrepancies must all refuse."""
        for fault in ("relabel", "missing_recovery", "other_original", "changed_cash"):
            payload = copy.deepcopy(self.proof)
            if fault == "relabel":
                payload["classification"], payload["bootstrap_claim"] = "real_bootstrap", True
            elif fault == "missing_recovery":
                del payload["receipts"]["original-recovery"]
            elif fault == "other_original":
                payload["records"]["Debug"]["final_input"] = payload["records"]["RelWithDebInfo"]["final_input"]
            else:
                payload["records"]["Debug"]["steps"][-1]["cash_after"] += 1
            with self.subTest(fault=fault), self.assertRaisesRegex(AssertionError, "complete exact predecessor graph"):
                verify_proof_payload(payload)

    def test_complete_current_owners_and_wrong_original_refusal(self):
        """Decode real current nested vehicle/utility tables and reject a different actual original."""
        first = self.proof["records"]["Debug"]["final_input"]["path"]
        other = self.proof["records"]["RelWithDebInfo"]["final_input"]["path"]
        ledger = owner.capture_ledger([first], complete_native=True)[0]
        self.assertEqual(len(ledger["owners"]), 126)
        self.assertEqual(set(ledger["tables"]), {"VEHS", "CAPA", "PLYR", "TBMP", "TBXA", "SPRR", "LPWK", "XJOB", "TUPD", "TUVD", "UPLD"})
        with self.assertRaisesRegex(owner.EvidenceError, "owner/clock mismatch"):
            owner.compare_state_digest(first, other, complete_native=True)

    def test_legacy_scoped_ledger_remains_explicit(self):
        """Actual older native transport remains readable but cannot claim the complete434 contract."""
        legacy = "src/tests/fixtures/haulage/source-loop/source-native-deep-stopped.sav"
        ledger = owner.capture_ledger([legacy])[0]
        self.assertLess(ledger["save_version"], 434)
        self.assertEqual(len(ledger["owners"]), 46)
        with self.assertRaisesRegex(owner.EvidenceError, "every unique native434 chunk"):
            owner.capture_ledger([legacy], complete_native=True)


if __name__ == "__main__":
    unittest.main()
