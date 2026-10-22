"""Focused occupied-view scope and malformed native-observation refusal checks.

@file launch_viewport_review_check.py Detached negative metadata; no synthetic capture qualification.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19B.
"""

import copy
import unittest
from unittest.mock import patch

import launch_viewport_review as review


class LaunchViewportChecks(unittest.TestCase):
    """Refuse wrong scopes, repeated frames and traversal before a paid fixture is loaded."""

    def test_malformed_observations_cannot_qualify_a_complete_scene_matrix(self):
        """Reject detached metadata that substitutes scenes, simulation work or foreign frames.

        @param self Active original occupied-matrix boundary check.
        @return None; metadata alone is never published as native rendering proof.
        """
        scenes = ("empty", "pad-ready", "pad-preparing", "pad-ascent0", "pad-ascent1",
                  "recovery-turnaround", "workshop-working", "workshop-blocked", "stale-pad")
        base = dict(component_pose_inputs=True, simulation_ticks=0, ordinary_game_reviewed=False,
                    observations=[dict(rotation=view, scene=scene, image="view-" + str(view) + "-" + scene + ".ppm")
                                  for view in range(4) for scene in scenes])
        # Each altered scope must independently refuse. A plausible scene
        # count cannot hide a duplicate, paid tick or substituted file owner.
        for mutation in ("missing", "duplicate", "paid-tick", "ordinary-claim", "foreign-frame"):
            document = copy.deepcopy(base)
            if mutation == "missing":
                document["observations"].pop()
            elif mutation == "duplicate":
                document["observations"].append(document["observations"][0])
            elif mutation == "paid-tick":
                document["simulation_ticks"] = 1
            elif mutation == "ordinary-claim":
                document["ordinary_game_reviewed"] = True
            else:
                document["observations"][0]["image"] = "../another-view.ppm"
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                review.renderer.qualified_observations(document)

    def test_unsafe_capture_scope_refuses_before_source_or_build_selection(self):
        """Reject paid/publication/unsupported action scopes before touching a retained world.

        @param self Active capture authorization metadata check.
        @return None; no native process, source input or output invocation is selected.
        """
        base = dict(stage="verify", configuration="Debug", paid_work_authorized=False, publishes_source=False)
        # Every changed action boundary must fail before even a registered
        # producer lookup; current receipts cannot authorize paid gameplay.
        for key, value in (("stage", "earn"), ("configuration", "Release"),
                           ("paid_work_authorized", True), ("publishes_source", True)):
            with patch.object(review, "selected_recipe", side_effect=AssertionError("unsafe capture selected sources")):
                with self.subTest(field=key), self.assertRaisesRegex(ValueError, "read-only current"):
                    review.checked_request(dict(base, **{key: value}), review.ROOT)

    def test_capture_output_identifier_refuses_before_current_selector_reads(self):
        """Keep an explicit fresh output run from traversing existing proof/runtime owners.

        @param self Active configuration-specific output namespace check.
        @return None; native frame names cannot escape into another invocation.
        """
        with patch.object(review, "selected_recipe", side_effect=AssertionError("unsafe output selected producer")):
            with self.assertRaises(ValueError):
                review.execute_prepared("Debug", {}, {}, {}, "../existing")


if __name__ == "__main__":
    unittest.main()
