"""Verify genuine heavy-miner gas instances and original physical nozzle contacts.

@file space_art_heavy_travel.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65.3b
This source fixture preserves the original heavy chassis and explicit effect
reuse; it grants no movement, work, packed sprites or stopped-craft plume.
"""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_heavy_travel as travel


class HeavyMinerTravelTests(unittest.TestCase):
    """Protect actual twin-nozzle geometry, original gas ancestry and native pixels."""

    @classmethod
    def setUpClass(cls):
        """Resolve the active family's real original travel sources once.

        @param cls Fixture sharing only immutable converted source pixels.
        """
        cls.source = art.read_record()
        family = cls.source["families"][travel.ASSET]
        cls.originals = {}
        cls.poses = {}
        # Resolve every actual source heading and phase independently, without reopening completed native executable or all-family source graphs.
        for direction in family["directions"]:
            cls.originals[direction], _ = art.native_view(family["selections"]["free/" + direction], direction, cls.source)
            for phase in range(2):
                cls.poses[phase, direction] = travel.travel_pose(direction, phase, cls.source)

    def setUp(self):
        """Detach receiving and donor metadata before each refusal case.

        @param self Independent original-source corruption fixture.
        """
        self.record = copy.deepcopy(self.source)
        self.family = self.record["families"][travel.ASSET]

    def test_all_native_headings_have_distinct_actual_pulses(self):
        """Both real gas phases remain readable after the original hull occludes them.

        @param self Actual native source pixel fixture.
        """
        self.assertEqual(len(self.poses), 16)
        # Each genuine heading must retain a visible actual pulse change; a metadata animation flag cannot rescue duplicate native pixels.
        for direction in self.family["directions"]:
            first, _ = self.poses[0, direction]
            second, _ = self.poses[1, direction]
            self.assertEqual(first.size, (64, 48))
            self.assertNotEqual(first.tobytes(), second.tobytes())

    def test_every_visible_outlet_has_its_own_original_contact(self):
        """Two physical nozzles remain original, with natural front-on S occlusion.

        @param self Actual source/contact evidence fixture.
        """
        # Check each mouth in both phases, rather than accepting one nearby jet for the whole two-nozzle body.
        for (phase, direction), (_, evidence) in self.poses.items():
            self.assertEqual(evidence["physical_outlet_count"], 2)
            self.assertEqual(evidence["visible_outlet_count"], 1 if direction == "S" else 2)
            for outlet in evidence["outlets"]:
                self.assertEqual(outlet["contact_distance"], 1)
                self.assertGreater(outlet["added_gas_pixels"], 0)
                self.assertFalse(outlet["new_generated_artwork"])
                self.assertFalse(outlet["source_pixels_changed"])

    def test_every_original_opaque_hardware_pixel_is_exact(self):
        """Gas adds opacity only behind the original body within approved outlet regions.

        @param self Whole native-canvas comparison fixture.
        """
        # Compare the complete original hull, tool, grippers, hopper, electronics and every empty pixel outside the effect volumes.
        for (_, direction), (pose, evidence) in self.poses.items():
            original = self.originals[direction]
            for y in range(48):
                for x in range(64):
                    old, new = original.getpixel((x, y)), pose.getpixel((x, y))
                    inside = any(o["bounds"][0] <= x < o["bounds"][2] and o["bounds"][1] <= y < o["bounds"][3] for o in evidence["outlets"])
                    # Existing hardware always occludes the plume; empty space is editable only inside an explicitly registered physical outlet volume.
                    if old != 0 or not inside:
                        self.assertEqual(old, new)

    def test_original_effect_reuse_is_explicit(self):
        """Registered instances retain the exact genuine original source and receipt.

        @param self Shared original ancestry fixture.
        """
        # Every outlet resolves the actual approved donor, with unchanged source geometry; registration does not claim another generated heavy model.
        for phase in range(2):
            for direction in self.family["directions"]:
                for candidate, _ in travel.outlet_sources(direction, phase, self.record):
                    donor = art.source_candidate(candidate["instance_of"]["candidate"], self.record)
                    self.assertEqual(candidate["sha256"], donor["sha256"])
                    self.assertEqual(candidate["generation_receipt"], donor["generation_receipt"])
                    self.assertEqual(candidate["phase"], donor["phase"])

    def test_missing_secondary_source_refuses(self):
        """One genuine jet cannot silently fill both heavy nozzles.

        @param self Missing second original instance fixture.
        """
        del self.family["travel_secondary_sources"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "missing genuine heavy outlet source"):
            travel.travel_pose("N", 0, self.record)

    def test_changed_source_scale_refuses(self):
        """Gas reuse cannot enlarge or distort the original authored effect.

        @param self Altered genuine original geometry fixture.
        """
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.record)
        candidate["native_transforms"]["N"]["uniform_scale"] = [1, 10]
        with self.assertRaisesRegex(ValueError, "shared effect changes original source geometry"):
            travel.travel_pose("N", 0, self.record)

    def test_changed_donor_approval_refuses(self):
        """Unchanged raw bytes cannot authorize an unapproved original donor assembly.

        @param self Stale original donor registration fixture.
        """
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.record)
        candidate["donor_registration"]["N"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "missing or changed original shared effect approval"):
            travel.travel_pose("N", 0, self.record)

    def test_wrong_original_role_refuses(self):
        """A heavy hardware original cannot masquerade as the approved generic gas.

        @param self Incorrect original source-role fixture.
        """
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.record)
        candidate["instance_of"]["source_asset_id"] = "vehicle.model-20"
        with self.assertRaisesRegex(ValueError, "unsupported shared original Space effect"):
            travel.travel_pose("N", 0, self.record)

    def test_receiving_reference_drift_refuses(self):
        """A registered gas instance cannot move independently of its frozen nozzle.

        @param self Shifted receiving native reference fixture.
        """
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.record)
        candidate["native_transforms"]["N"]["native_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "misses its original outlet reference"):
            travel.travel_pose("N", 0, self.record)

    def test_original_nozzle_count_cannot_shrink(self):
        """Source coverage cannot be achieved by removing a real heavy thruster.

        @param self Reduced original physical inventory fixture.
        """
        self.family["physical_outlet_count"] = 1
        with self.assertRaisesRegex(ValueError, "changes original nozzle visibility/count"):
            travel.travel_pose("N", 0, self.record)

    def test_occluded_original_nozzle_cannot_be_invented(self):
        """The actual S hopper occlusion cannot be replaced by an extra visible mouth.

        @param self Fabricated second front-on visible outlet fixture.
        """
        self.family["travel_outlets"]["S"].append(copy.deepcopy(self.family["travel_outlets"]["S"][0]))
        with self.assertRaisesRegex(ValueError, "changes original nozzle visibility/count"):
            travel.travel_pose("S", 0, self.record)

    def test_empty_original_contact_refuses_even_if_reapproved(self):
        """Registration metadata cannot turn transparent background into a nozzle edge.

        @param self Invalid physical outlet contact fixture.
        """
        self.family["travel_outlets"]["N"][0]["contacts"] = [[0, 0]]
        self.family["approved_travel_registration"]["0"]["N"] = travel.travel_signature("N", 0, self.record)
        with self.assertRaisesRegex(ValueError, "no actual original outlet contact"):
            travel.travel_pose("N", 0, self.record)

    def test_floating_gas_refuses_even_if_reapproved(self):
        """A visible genuine pulse still needs to touch its own original physical mouth.

        @param self Detached but in-canvas receiving registration fixture.
        """
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.record)
        candidate["native_transforms"]["N"]["native_reference"][0] -= 8
        outlet = self.family["travel_outlets"]["N"][0]
        outlet["native_reference"][0] -= 8
        outlet["bounds"][0] -= 8
        outlet["bounds"][2] -= 8
        self.family["approved_travel_registration"]["0"]["N"] = travel.travel_signature("N", 0, self.record)
        with self.assertRaisesRegex(ValueError, "does not touch its original outlet"):
            travel.travel_pose("N", 0, self.record)

    def test_partial_review_keeps_complete_source_and_runtime_pending(self):
        """Sixteen source poses do not imply the full128 matrix or native movement.

        @param self Dated actual source review fixture.
        """
        path = ROOT / "logs/proofs/mlm/native-art/space/heavy-miner-travel-v1/heavy-travel-source-review.json"
        report = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(report["pose_count"], 16)
        self.assertEqual(report["anchor"], [-32, -32])
        self.assertFalse(report["new_generated_artwork"])
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime"])


if __name__ == "__main__":
    unittest.main()
