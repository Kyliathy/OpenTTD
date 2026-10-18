"""Protect genuine heavy-miner cutter phases and their exact original chassis.

@file space_art_heavy_work.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65
This focused source fixture checks real original pixels and refusal boundaries;
it grants no missing state coverage, packed sprites or native mining work.
"""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_heavy_parts as heavy


class HeavyMinerCutterTests(unittest.TestCase):
    """Verify actual tooth animation, fixed mounts and protected original pixels."""

    @classmethod
    def setUpClass(cls):
        """Resolve each genuine original phase once for focused pixel checks.

        @param cls Source fixture sharing immutable converted originals only.
        """
        cls.source = art.read_record()
        family = cls.source["families"][heavy.ASSET]
        cls.originals = {}
        cls.poses = {}
        # Cache only this active component family's real originals; every genuine heading and phase still passes the production converter independently.
        for direction in family["directions"]:
            cls.originals[direction], _ = art.native_view(family["selections"]["free/" + direction], direction, cls.source)
            for phase in range(2):
                cls.poses[phase, direction] = heavy.drill_pose(direction, phase, cls.source)

    def setUp(self):
        """Detach metadata before each corruption/refusal case.

        @param self Independent source mutation fixture.
        """
        self.record = copy.deepcopy(self.source)
        self.family = self.record["families"][heavy.ASSET]

    def test_every_genuine_view_has_two_readable_phases(self):
        """Both actual tooth phases survive native palette conversion in all views.

        @param self Cached production-source pixel fixture.
        """
        self.assertEqual(len(self.poses), 16)
        # Every camera needs its own readable phase change; an animated metadata flag cannot turn a still or neighboring view into a real second original.
        for direction in self.family["directions"]:
            first, _ = self.poses[0, direction]
            second, evidence = self.poses[1, direction]
            self.assertEqual(second.size, (64, 48))
            self.assertGreaterEqual(evidence["changed_pixels"], 2)
            self.assertNotEqual(first.tobytes(), second.tobytes())

    def test_idle_cutter_is_exact_original_body(self):
        """Actual phase0 reuses the unchanged original cone without a fake redraw.

        @param self Original pixel ancestry fixture.
        """
        # A phase0 alias must preserve every original pixel, not merely the cutter's apparent shape.
        for direction, original in self.originals.items():
            pose, evidence = self.poses[0, direction]
            self.assertEqual(pose.tobytes(), original.tobytes())
            self.assertEqual(evidence["changed_pixels"], 0)

    def test_tooth_phase_preserves_all_other_hardware_and_opacity(self):
        """Only approved tooth pixels change; original silhouettes and mounts stay exact.

        @param self Whole native-canvas comparison fixture.
        """
        # Compare all pixels, including empty space and gray chassis metal, so a broad rectangle cannot conceal replacement of adjacent collar, gripper or hopper hardware.
        for direction, original in self.originals.items():
            pose, evidence = self.poses[1, direction]
            editable = {tuple(point) for point in evidence["edit_pixels"]}
            self.assertEqual(evidence["protected_pixels"], 64 * 48 - len(editable))
            self.assertEqual(pose.getpixel(tuple(evidence["mount"])), original.getpixel(tuple(evidence["mount"])))
            for y in range(48):
                for x in range(64):
                    old, new = original.getpixel((x, y)), pose.getpixel((x, y))
                    self.assertEqual(old == 0, new == 0)
                    if (x, y) not in editable:
                        self.assertEqual(old, new)

    def test_rotated_faces_have_genuine_new_originals(self):
        """Rotated phases bind different real originals rather than relabeling idle pixels.

        @param self Exact source receipt/ancestry fixture.
        """
        # Both four-view batches must use actual selected original bytes and receipts distinct from the unchanged idle sheets.
        for direction in self.family["directions"]:
            first = art.source_candidate(self.family["mining_sources"]["0"][direction], self.record)
            second = art.source_candidate(self.family["mining_sources"]["1"][direction], self.record)
            self.assertNotEqual(first["sha256"], second["sha256"])
            self.assertNotEqual(first["generation_receipt"], second["generation_receipt"])
            self.assertEqual(second["phase"], 1)

    def test_missing_phase_cannot_be_inferred(self):
        """A missing second original cannot borrow a still or another heading.

        @param self Absent actual phase fixture.
        """
        del self.family["mining_sources"]["1"]["S"]
        with self.assertRaisesRegex(ValueError, "missing genuine heavy cutter original"):
            heavy.drill_pose("S", 1, self.record)

    def test_foreign_role_cannot_supply_heavy_tool(self):
        """Retained construction robot16 cannot be relabeled as actual heavy mining art.

        @param self Incorrect native family fixture.
        """
        candidate = art.source_candidate(self.family["mining_sources"]["1"]["N"], self.record)
        candidate["asset_id"] = "vehicle.model-16"
        with self.assertRaisesRegex(ValueError, "not the declared original heavy cutter"):
            heavy.drill_pose("N", 1, self.record)

    def test_phase_relabel_refuses(self):
        """A phase0 source cannot fill the independent rotated face by metadata alias.

        @param self Relabeled actual original phase fixture.
        """
        self.family["mining_sources"]["1"]["N"] = self.family["mining_sources"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "not the declared original heavy cutter"):
            heavy.drill_pose("N", 1, self.record)

    def test_source_registration_drift_refuses(self):
        """An approved component transform cannot move independently of its original body.

        @param self Shifted original mounting transform fixture.
        """
        candidate = art.source_candidate(self.family["mining_sources"]["1"]["N"], self.record)
        candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "approved heavy cutter registration changed"):
            heavy.drill_pose("N", 1, self.record)

    def test_editable_tooth_mask_drift_refuses(self):
        """Later variants cannot silently change which original steel pixels may rotate.

        @param self Narrowed original ownership mask fixture.
        """
        self.family["mining_edit_pixels"]["N"].pop()
        with self.assertRaisesRegex(ValueError, "approved heavy cutter registration changed"):
            heavy.drill_pose("N", 1, self.record)

    def test_mount_drift_refuses(self):
        """A changed collar reference requires source review even within valid bounds.

        @param self Shifted physical mount fixture.
        """
        self.family["mining_mounts"]["N"][0] += 1
        with self.assertRaisesRegex(ValueError, "approved heavy cutter registration changed"):
            heavy.drill_pose("N", 1, self.record)

    def test_canvas_clipping_refuses(self):
        """An oversized physical part cannot hide clipped source coverage.

        @param self Invalid native ownership bounds fixture.
        """
        self.family["mining_component_bounds"]["S"][3] = 49
        with self.assertRaisesRegex(ValueError, "invalid original heavy cutter pixels"):
            heavy.drill_pose("S", 1, self.record)

    def test_floating_mount_refuses_even_with_new_signature(self):
        """Registration approval alone cannot make an empty original point a real collar.

        @param self Detached physical mount fixture.
        """
        self.family["mining_mounts"]["S"] = [28, 39]
        candidate = art.source_candidate(self.family["mining_sources"]["1"]["S"], self.record)
        self.family["approved_mining_registration"]["1"]["S"] = heavy.drill_signature(candidate, "S", self.family)
        with self.assertRaisesRegex(ValueError, "heavy cutter misses its original mount"):
            heavy.drill_pose("S", 1, self.record)

    def test_rejected_background_trial_cannot_reenter(self):
        """A colored translucent source sheet cannot be rescued with a component mask.

        @param self Explicitly rejected original source fixture.
        """
        self.family["mining_sources"]["1"]["S"] = "heavy-miner-work-b-s-sw-w-nw-v1"
        with self.assertRaisesRegex(ValueError, "rejected Space source"):
            heavy.drill_pose("S", 1, self.record)

    def test_partial_review_keeps_other_states_and_runtime_pending(self):
        """The cutter checkpoint proves16 source poses without claiming the full128 matrix.

        @param self Dated component-source artifact fixture.
        """
        path = ROOT / "logs/proofs/mlm/native-art/space/heavy-miner-cutter-v1/heavy-cutter-source-review.json"
        report = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(report["pose_count"], 16)
        self.assertEqual(report["anchor"], [-32, -32])
        self.assertTrue(report["original_opacity_preserved"])
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime"])


if __name__ == "__main__":
    unittest.main()
