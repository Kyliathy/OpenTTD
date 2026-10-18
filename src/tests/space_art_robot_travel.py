"""Verify original single-nozzle gas registered on the retained cargo-less robot.

@file space_art_robot_travel.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13CB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65B.3a
These source checks grant no movement, cargo, construction work or packed art.
"""
import copy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_mining_parts as parts

ASSET = "vehicle.model-16"


class RetainedRobotTravelTests(unittest.TestCase):
    """Protect the one original nozzle, exact body pixels and real gas ancestry."""

    @classmethod
    def setUpClass(cls):
        """Resolve each actual retained-body heading and both approved gas phases once.

        @param cls Fixture sharing only immutable converted source pixels.
        """
        cls.source = art.read_record()
        family = cls.source["families"][ASSET]
        cls.originals, cls.poses = {}, {}
        # Every physical outlet must show two actual source phases after the original nozzle occludes them; a neighboring heading cannot fill a missing pulse.
        for direction in family["directions"]:
            cls.originals[direction], _ = art.native_view(family["selections"]["free/" + direction], direction, cls.source)
            for phase in range(2):
                cls.poses[phase, direction] = parts.component_pose("rear_plume", direction, phase, cls.source, ASSET)

    def setUp(self):
        """Detach source metadata before testing a refused authoring change.

        @param self Independent receiving/donor source fixture.
        """
        self.record = copy.deepcopy(self.source)
        self.family = self.record["families"][ASSET]
        self.candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.record)

    def test_all_sixteen_actual_pulses_are_readable(self):
        """Both authored gas phases survive native conversion in every genuine camera.

        @param self Registered phase/readability fixture.
        """
        self.assertEqual(len(self.poses), 16)
        # An animation declaration requires visible original source differences at native size, including the rear-facing N/NE/NW mouths.
        for direction in self.family["directions"]:
            a, _ = self.poses[0, direction]
            b, _ = self.poses[1, direction]
            self.assertEqual(a.size, (64, 48))
            self.assertGreaterEqual(sum(x != y for x, y in zip(a.get_flattened_data(), b.get_flattened_data(), strict=True)), 2)

    def test_whole_original_hardware_and_outside_pixels_remain_exact(self):
        """Only empty space behind the one original nozzle may gain gas pixels.

        @param self Complete native body/opacity protection fixture.
        """
        # Check the complete canvas: folded arms, passive coupling, display, nozzle and chassis always occlude the plume unchanged.
        for (_, direction), (pose, evidence) in self.poses.items():
            base = self.originals[direction]
            left, top, right, bottom = evidence["bounds"]
            for y in range(48):
                for x in range(64):
                    old, new = base.getpixel((x, y)), pose.getpixel((x, y))
                    inside = left <= x < right and top <= y < bottom
                    # Existing opaque machinery and space outside the approved outlet volume retain their exact original palette index.
                    if old != 0 or not inside:
                        self.assertEqual(new, old)

    def test_one_real_outlet_contact_in_both_phases(self):
        """Gas touches the original single nozzle rim without floating or extra engines.

        @param self Physical outlet and actual native contact fixture.
        """
        self.assertEqual(self.family["physical_rear_outlet_count"], 1)
        self.assertNotIn("travel_secondary_sources", self.family)
        # Each visible pulse must adjoin its own frozen opaque rim; another nearby body edge cannot silently replace a missing original outlet.
        for (_, direction), (pose, evidence) in self.poses.items():
            base = self.originals[direction]
            contacts = self.family["travel_outlet_contacts"][direction]
            self.assertEqual(len(contacts), 1)
            self.assertNotEqual(base.getpixel(tuple(contacts[0])), 0)
            left, top, right, bottom = evidence["bounds"]
            gas = [(x, y) for y in range(top, bottom) for x in range(left, right)
                   if base.getpixel((x, y)) == 0 and pose.getpixel((x, y)) != 0]
            self.assertTrue(gas)
            self.assertEqual(min(max(abs(x - contacts[0][0]), abs(y - contacts[0][1])) for x, y in gas), 1)

    def test_registered_instances_keep_actual_original_ancestry(self):
        """Source reuse retains unchanged original pixels, prompts, receipts and geometry.

        @param self Genuine shared gas source fixture.
        """
        keys = set()
        # Four real original images supply both phases and all eight headings; sixteen registrations are not sixteen newly generated robot pictures.
        for phase in ("0", "1"):
            for direction, key in self.family["travel_sources"][phase].items():
                candidate = parts.registered_component_source(key, direction, self.record)
                donor = art.source_candidate(candidate["instance_of"]["candidate"], self.record)
                self.assertEqual(candidate["instance_of"]["source_asset_id"], parts.ASSET)
                self.assertEqual(candidate["path"], donor["path"])
                self.assertEqual(candidate["sha256"], donor["sha256"])
                self.assertEqual(candidate["generation_receipt"], donor["generation_receipt"])
                keys.add(candidate["instance_of"]["candidate"])
        self.assertEqual(len(keys), 4)

    def test_changed_original_geometry_refuses(self):
        """A receiver cannot enlarge or distort the real gas to rescue its registration.

        @param self Altered immutable source-transform fixture.
        """
        self.candidate["native_transforms"]["N"]["uniform_scale"] = [1, 10]
        with self.assertRaisesRegex(ValueError, "shared effect changes original source geometry"):
            parts.component_pose("rear_plume", "N", 0, self.record, ASSET)

    def test_shifted_receiving_reference_refuses(self):
        """Later source changes cannot move gas independently of the approved body mouth.

        @param self Shifted original outlet fixture.
        """
        self.candidate["native_transforms"]["N"]["native_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "approved mining mechanism registration changed"):
            parts.component_pose("rear_plume", "N", 0, self.record, ASSET)

    def test_stale_donor_approval_refuses(self):
        """Unchanged raw bytes cannot borrow an unapproved donor assembly.

        @param self Stale original gas approval fixture.
        """
        self.candidate["donor_registration"]["N"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "missing or changed original shared effect approval"):
            parts.component_pose("rear_plume", "N", 0, self.record, ASSET)

    def test_foreign_hardware_cannot_masquerade_as_gas(self):
        """A work mechanism cannot reuse the gas registration to modify retained16.

        @param self Incorrect authored component fixture.
        """
        self.candidate["component_kind"] = "front_mechanism"
        with self.assertRaisesRegex(ValueError, "unsupported shared original Space effect"):
            parts.component_pose("rear_plume", "N", 0, self.record, ASSET)

    def test_unreviewed_receiver_role_refuses(self):
        """Adding retained16 does not authorize arbitrary other receiving models.

        @param self Unsupported new receiver fixture.
        """
        self.candidate["asset_id"] = "vehicle.model-17"
        with self.assertRaisesRegex(ValueError, "unsupported shared original Space effect"):
            parts.component_pose("rear_plume", "N", 0, self.record, ASSET)

    def test_missing_original_phase_heading_refuses(self):
        """A declared movement phase cannot substitute a neighboring camera.

        @param self Missing genuine actual gas source fixture.
        """
        del self.family["travel_sources"]["1"]["NW"]
        with self.assertRaisesRegex(ValueError, "missing authored mining source heading"):
            parts.component_pose("rear_plume", "NW", 1, self.record, ASSET)

    def test_empty_contact_cannot_be_resigned_as_approval(self):
        """A signed source record still needs an actual original opaque nozzle contact.

        @param self Invalid freshly signed empty-contact fixture.
        """
        self.family["travel_outlet_contacts"]["N"] = []
        self.family["approved_travel_registration"]["0"]["N"] = parts.part_signature(
            self.candidate, "N", self.family["travel_component_bounds"]["N"],
            self.family["approved_base_registration"]["N"], [])
        with self.assertRaisesRegex(ValueError, "invalid original rear outlet contact"):
            parts.component_pose("rear_plume", "N", 0, self.record, ASSET)

    def test_original_generation_receipt_cannot_change(self):
        """Registered gas remains tied to the original executed request.

        @param self Corrupted exact prompt receipt fixture.
        """
        self.candidate["generation_receipt"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            parts.component_pose("rear_plume", "N", 0, self.record, ASSET)


if __name__ == "__main__":
    unittest.main()
