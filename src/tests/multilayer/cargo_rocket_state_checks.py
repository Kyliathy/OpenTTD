"""Check read-only native-phase source choices before any rocket effect generation.

@file cargo_rocket_state_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62E.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Fresh native producer receipts come from cargo_rocket_native_checks.py --native;
this bounded suite never reloads a paid world, ticks a hull or generates pixels.
"""

import copy
import json
from pathlib import Path
import sys
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
from tools import cargo_rocket_art as art
from tools import cargo_rocket_directions as directions
from tools import cargo_rocket_states as states
import cargo_rocket_native_checks as native


class CargoRocketStateChecks(unittest.TestCase):
    """Require real funded progress, static aliases and untouched source/game owners."""

    @classmethod
    def setUpClass(cls):
        """Verify the actual native/source prerequisites once for this focused burst.

        @param cls Active source-policy suite with immutable shared input.
        @return None.
        """
        cls.policy = art.read_json(states.POLICY)
        states.checked_policy(cls.policy)
        for configuration in native.CONFIGURATIONS:
            native.checked_native(art.read_json(native.OUTPUT/f"native-{configuration}.json"))

    def test_funded_native_progress_drives_only_existing_leg_frame_cadence(self):
        """Frame changes consume the existing countdown and earn no extra work/fuel.

        @param self Active actual paid-leg frame selection check.
        @return None.
        """
        for phase, recovery, expected_state in (("Ascent", False, "ascent"), ("ReturnRecovery", True, "recovery")):
            # Actual canonical elapsed opportunities select two source frames;
            # queries repeat the same result without moving or decrementing data.
            for remaining, frame in ((222, 0), (215, 0), (214, 1), (207, 1), (206, 0), (1, 1)):
                actual = states.source_pose(phase, outbound_burned=True, recovery_burned=recovery,
                                            remaining_ticks=remaining, policy=self.policy)
                expected = dict(state=expected_state, frame=frame, effect="engine/frame"+str(frame))
                self.assertEqual(actual, expected)
                self.assertEqual(states.source_pose(phase, outbound_burned=True, recovery_burned=recovery,
                                                   remaining_ticks=remaining, policy=self.policy), expected)

    def test_lost_target_or_home_does_not_freeze_still_progressing_funded_flight(self):
        """Active native flight outranks lost-reference metadata until terminal Holding.

        @param self Active native controller/block-precedence check.
        @return None.
        """
        for phase, recovery, expected in (("Ascent", False, "ascent"), ("ReturnRecovery", True, "recovery")):
            for block in ("TargetLost", "HomeLost"):
                actual = states.source_pose(phase, block, True, recovery, 222, self.policy)
                self.assertEqual(actual, dict(state=expected, frame=0, effect="engine/frame0"))
        self.assertEqual(states.source_pose("Holding", "HomeLost", True, True, 0, self.policy),
                         dict(state="blocked", frame=0, effect=None))

    def test_missing_paid_receipt_invalid_countdown_or_nonflight_cannot_paint_fire(self):
        """Persistent burn history is not permission for idle/orbit/service engine effects.

        @param self Active unpaid/expired/nonflight source-effect refusal check.
        @return None.
        """
        for phase, outbound, recovery in (("Ascent", False, False), ("Ascent", True, True),
                                          ("ReturnRecovery", True, False), ("ReturnRecovery", False, True)):
            self.assertEqual(states.source_pose(phase, "None", outbound, recovery, 222, self.policy),
                             dict(state="blocked", frame=0, effect=None))
        for remaining in (0, -1, 223):
            self.assertIsNone(states.source_pose("Ascent", "None", True, False, remaining, self.policy)["effect"])
        for phase, state in (("OrbitalDelivery", "orbit"), ("Turnaround", "service")):
            self.assertEqual(states.source_pose(phase, "None", True, True, 74, self.policy),
                             dict(state=state, frame=0, effect=None))

    def test_actual_approach_static_loading_preparation_and_turnaround_alias_exact_body(self):
        """Internal payload/paid handling can retain the unchanged sealed outer hull.

        @param self Active honest static-source and existing movement mapping check.
        @return None.
        """
        self.assertEqual(states.source_pose(None, policy=self.policy)["state"], "ready")
        self.assertEqual(states.source_pose("GroundLoading", policy=self.policy)["state"], "ready")
        self.assertEqual(states.source_pose("Preparing", remaining_ticks=148, policy=self.policy),
                         dict(state="preparing", frame=0, effect=None))
        self.assertEqual(states.source_pose("OrbitalDelivery", "Approach", True, False, 0, self.policy),
                         dict(state="orbit", frame=0, effect=None))
        # Actual freight admission waits stop in the native owner; a moving
        # Approach alone is not relabelled as a blocked physical wait.
        for block in ("TargetLost", "ReceiverBusy", "ReceiverFull"):
            self.assertEqual(states.source_pose("OrbitalDelivery", block, True, False, 0, self.policy),
                             dict(state="blocked", frame=0, effect=None))
        self.assertEqual(self.policy["static_alias_states"], ["ready", "preparing", "orbit", "blocked", "service"])
        self.assertEqual(self.policy["native_budget"]["turnaround_ticks"], 74)

    def test_unknown_native_names_and_changed_immutable_part_policy_are_refused(self):
        """A changed enum, probe job or widened art envelope needs explicit new qualification.

        @param self Active policy/owner/part-geometry drift refusal check.
        @return None.
        """
        with self.assertRaises(ValueError):
            states.source_pose("ProbeService", policy=self.policy)
        with self.assertRaises(ValueError):
            states.source_pose("Ascent", "InventedWait", True, False, 222, self.policy)
        with self.assertRaises(ValueError):
            states.source_pose("Ascent", remaining_ticks=True, policy=self.policy)
        for field, value in (("native_mount", [33, 34]), ("native_envelope", [0, 0, 64, 48]), ("fixed_body", False)):
            policy = copy.deepcopy(self.policy)
            policy["effect_part"][field] = value
            with self.assertRaisesRegex(ValueError, "immutable"):
                states.checked_policy(policy)

    def test_all_ready_pixels_completed_predecessor_and_payload_remain_exact_with_missing_effects(self):
        """A source policy cannot manufacture pixels or publish absent burn frames.

        @param self Active complete-ready/predecessor and incomplete-raster gate check.
        @return None.
        """
        art.checked_predecessor(art.source.read_record(), art.read_json(art.PUBLIC))
        for direction in art.DIRECTIONS:
            directions.direction_view(direction)
        part = self.policy["effect_part"]
        self.assertEqual(part["frames"], [0, 1])
        self.assertEqual(part["native_mount"], [32, 34])
        self.assertEqual(part["native_envelope"], [27, 32, 38, 46])
        self.assertTrue(part["fixed_four_passive_feet"])
        self.assertEqual(part["selected_originals"], {})
        self.assertFalse(part["source_complete"])
        for field in ("source_coverage", "packed", "runtime"):
            self.assertFalse(self.policy[field])


def main():
    """Run only the active phase/source-policy group and save exact evidence.

    @return Zero after native-bound policy and preservation/refusal cases pass.
    """
    start = time.monotonic()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(CargoRocketStateChecks))
    report = dict(_documentation=__doc__, status="passed" if result.wasSuccessful() else "failed", cases=result.testsRun,
                  elapsed_seconds=round(time.monotonic()-start, 3), helper=art.source.identity(Path(states.__file__)),
                  tests=art.source.identity(Path(__file__)), policy=art.source.identity(states.POLICY),
                  fixture=art.source.identity(states.FIXTURE), native_proofs=art.read_json(states.POLICY)["native_proofs"],
                  source_coverage=False, engine_parts_complete=False, packed=False, runtime=False)
    (native.OUTPUT/"verification.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
