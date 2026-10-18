"""Protect genuine heavy-miner indicator originals and original display ownership.

@file space_art_heavy_status.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65.3d
Actual original lamp pixels change only frozen display cells; no source grants
paid service work, opens hardware, changes cargo or provides packed/runtime art.
"""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_status_parts as status

ASSET = "vehicle.model-21"
CUES = (("blocked", 0), ("service", 0), ("service", 1))


class HeavyMinerStatusTests(unittest.TestCase):
    """Verify original lamp animation, rejected camera refusal and protected hardware."""

    @classmethod
    def setUpClass(cls):
        """Resolve each actual source view and cue once for focused pixel checks.

        @param cls Fixture sharing immutable converted originals only.
        """
        cls.source = art.read_record()
        family = cls.source["families"][ASSET]
        cls.originals, cls.poses = {}, {}
        # Every genuine camera and actual indicator phase passes the production converter independently; copying a neighboring face cannot fill a missing direction.
        for direction in family["directions"]:
            cls.originals[direction], _ = art.native_view(family["selections"]["free/" + direction], direction, cls.source)
            for state, phase in CUES:
                cls.poses[state, phase, direction] = status.status_pose(state, direction, phase, cls.source, ASSET)

    def setUp(self):
        """Detach source definitions before each independent refusal case.

        @param self Original source registration and display ownership fixture.
        """
        self.record = copy.deepcopy(self.source)
        self.family = self.record["families"][ASSET]

    def test_all_genuine_views_have_distinct_actual_service_phases(self):
        """Both authored diagnostic positions survive native palette conversion.

        @param self Actual source pixel and cue identity fixture.
        """
        self.assertEqual(len(self.poses), 24)
        # Native illumination phases and the actual blocked cue must remain independently readable for every real camera.
        for direction in self.family["directions"]:
            first, _ = self.poses["service", 0, direction]
            second, _ = self.poses["service", 1, direction]
            blocked, _ = self.poses["blocked", 0, direction]
            self.assertEqual(first.size, (64, 48))
            self.assertNotEqual(first.tobytes(), second.tobytes())
            self.assertNotEqual(blocked.tobytes(), first.tobytes())
            self.assertNotEqual(blocked.tobytes(), second.tobytes())

    def test_every_other_original_pixel_and_all_opacity_are_exact(self):
        """The original housing, mounts, chassis and empty space never flicker with lamps.

        @param self Whole native-canvas comparison fixture.
        """
        # Compare pixels inside the wider component bounds as well as outside them; only actual frozen indicator cells may change, including where neutral-colored original housing metal surrounds the lens.
        for (_, _, direction), (pose, evidence) in self.poses.items():
            original = self.originals[direction]
            editable = {tuple(point) for point in evidence["edit_pixels"]}
            self.assertEqual(evidence["protected_pixels"], 3072 - len(editable))
            self.assertGreater(evidence["changed_pixels"], 0)
            for y in range(48):
                for x in range(64):
                    old, new = original.getpixel((x, y)), pose.getpixel((x, y))
                    self.assertEqual(old == 0, new == 0)
                    if (x, y) not in editable:
                        self.assertEqual(old, new)

    def test_actual_components_retain_three_exact_originals(self):
        """Two part matrices plus the genuine NW correction supply original lamp pixels.

        @param self Original receipt and physical camera source fixture.
        """
        originals = set()
        # Each actual state resolves its own measured original subrectangle, even when independent components share a single immutable generated sheet.
        for direction in self.family["directions"]:
            for state, phase in CUES:
                candidate = art.source_candidate(self.family["status_sources"][state][str(phase)][direction], self.record)
                originals.add((candidate["path"], candidate["sha256"]))
                self.assertEqual(candidate["panel_state"], state)
                self.assertEqual(candidate["phase"], phase)
        self.assertEqual(len(originals), 3)

    def test_rejected_nw_matrix_camera_cannot_reenter(self):
        """Safe extraction cannot authorize the old positive-diagonal NW housing.

        @param self Explicitly unaccepted original camera fixture.
        """
        candidate = art.source_candidate("heavy-miner-status-s-sw-w-nw-v1-service-0", self.record)
        with self.assertRaisesRegex(ValueError, "unaccepted original Space view"):
            art.source_pixels(candidate, "NW", self.family["status_component_bounds"]["NW"])

    def test_genuine_nw_uses_its_separate_corrected_original(self):
        """All NW indicators bind the actual negative-diagonal camera correction.

        @param self Original NW source and reference-role fixture.
        """
        # Neither a neighboring genuine heading nor the unaccepted old NW panel may provide the corrected original's three actual cue states.
        for state, phase in CUES:
            candidate = art.source_candidate(self.family["status_sources"][state][str(phase)]["NW"], self.record)
            self.assertEqual(candidate["path"], "media/multilayer/source/space/raw/heavy-miner-status-nw-v1.png")
            self.assertEqual(candidate["directions"], ["NW"])
            self.assertEqual(candidate["native_transforms"]["NW"]["uniform_scale"], [1, 20])

    def test_missing_phase_view_refuses(self):
        """An absent original service phase cannot be inferred from the first pose.

        @param self Missing actual status source fixture.
        """
        del self.family["status_sources"]["service"]["1"]["NW"]
        with self.assertRaisesRegex(ValueError, "missing genuine status heading"):
            status.status_pose("service", "NW", 1, self.record, ASSET)

    def test_blocked_original_cannot_be_relabelled_service(self):
        """An amber interlock still cannot become a service phase by metadata alias.

        @param self Incorrect authored indicator cue fixture.
        """
        self.family["status_sources"]["service"]["0"]["N"] = self.family["status_sources"]["blocked"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "not the declared authored status panel"):
            status.status_pose("service", "N", 0, self.record, ASSET)

    def test_changed_original_ancestry_refuses(self):
        """The selected original and exact generation receipt remain immutable.

        @param self Corrupted retained original identity fixture.
        """
        candidate = art.source_candidate(self.family["status_sources"]["service"]["0"]["N"], self.record)
        candidate["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            status.status_pose("service", "N", 0, self.record, ASSET)

    def test_source_reference_drift_refuses(self):
        """Indicator registration cannot move independently of the original housing.

        @param self Shifted actual display mounting fixture.
        """
        candidate = art.source_candidate(self.family["status_sources"]["service"]["0"]["N"], self.record)
        candidate["native_transforms"]["N"]["native_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "approved status registration changed"):
            status.status_pose("service", "N", 0, self.record, ASSET)

    def test_display_mask_cannot_expand_after_review(self):
        """A lamp cannot silently gain ownership of another original housing pixel.

        @param self Broadened original indicator surface fixture.
        """
        self.family["status_edit_pixels"]["N"].append([32, 21])
        with self.assertRaisesRegex(ValueError, "approved status registration changed"):
            status.status_pose("service", "N", 0, self.record, ASSET)

    def test_native_canvas_cannot_expand(self):
        """An oversized component region cannot hide missing source clearance or clipping.

        @param self Invalid native status ownership fixture.
        """
        self.family["status_component_bounds"]["N"][2] = 65
        with self.assertRaisesRegex(ValueError, "status panel leaves native canvas"):
            status.status_pose("service", "N", 0, self.record, ASSET)

    def test_partial_review_keeps_full_source_and_runtime_pending(self):
        """Twenty-four status sources do not imply all128 states or actual service work.

        @param self Dated source-only status review fixture.
        """
        path = ROOT / "logs/proofs/mlm/native-art/space/heavy-miner-status-v1/status-source-review.json"
        report = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(report["asset_id"], ASSET)
        self.assertEqual(len(report["poses"]), 24)
        self.assertTrue(all(value > 0 for value in report["service_phase_differences"].values()))
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime"])


if __name__ == "__main__":
    unittest.main()
