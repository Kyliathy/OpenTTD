"""Verify the distinct original survey model and protected actual scanner phases.

@file space_art_survey.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BC
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63B
Source-only checks never advance scan work, reveal quantities, take mining leases or mutate native gameplay.
"""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_mining_parts as parts
import convert_space_status_parts as status

ASSET = "vehicle.model-19"


class SurveyOriginalTests(unittest.TestCase):
    """Require genuine distinct scanner views and frozen native component registration."""

    def setUp(self):
        """Load independent definitions for each source/refusal case."""
        self.document = art.read_record()
        self.family = self.document["families"][ASSET]
        key = self.family["scanner_sources"]["0"]["N"]
        self.candidate = next(row for row in self.document["candidates"] if row["key"] == key)

    def test_distinct_original_model_has_all_genuine_base_views(self):
        """Every survey heading uses its selected new scanner original, never a recolored mining source."""
        self.assertEqual(self.family["directions"], ["N", "NE", "E", "SE", "S", "SW", "W", "NW"])
        # Each source window/anchor is independently verified; a neighboring or reused mining picture cannot supply an absent survey orientation.
        for direction in self.family["directions"]:
            key = self.family["selections"]["free/" + direction]
            candidate = art.source_candidate(key, self.document)
            self.assertEqual(candidate["asset_id"], ASSET)
            pose, _ = art.native_view(key, direction, self.document)
            self.assertEqual(pose.size, (64, 48))
            self.assertFalse((set(pose.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
            self.assertNotEqual(candidate["sha256"], art.source_candidate(self.document["families"][parts.ASSET]["selections"]["free/" + direction], self.document)["sha256"])
        self.assertEqual(self.family["public_anchor"], [-32, -32])
        self.assertFalse(self.family["runtime_activated"])

    def test_actual_dish_phases_protect_original_bus_and_paddles(self):
        """All16 authored native sweeps retain every original pixel outside their scanner/mounting region."""
        protected = 0
        # Independent per-pixel comparison catches a redraw of the approved hull or an unintended change to the rear nozzle, panel or service hatch.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            left, top, right, bottom = self.family["scanner_component_bounds"][direction]
            phases = []
            for phase in (0, 1):
                pose, _ = parts.component_pose("front_scanner", direction, phase, self.document, ASSET)
                self.assertEqual(pose.getpalette(), base.getpalette())
                for y in range(48):
                    for x in range(64):
                        if not (left <= x < right and top <= y < bottom):
                            self.assertEqual(base.getpixel((x, y)), pose.getpixel((x, y)))
                            protected += 1
                phases.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(phases[0].get_flattened_data(), phases[1].get_flattened_data(), strict=True)), 4)
        self.assertEqual(protected, 47156)

    def test_foreign_mining_hardware_cannot_supply_scanner(self):
        """Distinct roles cannot borrow a drill source to conceal missing actual scanner pixels."""
        self.family["scanner_sources"]["0"]["N"] = self.document["families"][parts.ASSET]["mining_sources"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "declared"):
            parts.component_pose("front_scanner", "N", 0, self.document, ASSET)

    def test_wrong_authored_phase_refuses(self):
        """One physical dish angle cannot fill both actual authored phases."""
        self.family["scanner_sources"]["1"]["N"] = self.candidate["key"]
        with self.assertRaisesRegex(ValueError, "declared"):
            parts.component_pose("front_scanner", "N", 1, self.document, ASSET)

    def test_changed_scanner_contact_refuses(self):
        """The selected dish cannot drift away from its original front mount after review."""
        self.candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            parts.component_pose("front_scanner", "N", 0, self.document, ASSET)

    def test_widened_scanner_region_refuses(self):
        """A widened mask cannot silently authorize later edits to protected original hull pixels."""
        self.family["scanner_component_bounds"]["N"][3] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            parts.component_pose("front_scanner", "N", 0, self.document, ASSET)

    def test_missing_genuine_heading_refuses(self):
        """No rotated flat picture or neighbor view supplies an absent original scanner heading."""
        del self.family["scanner_sources"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "heading"):
            parts.component_pose("front_scanner", "N", 0, self.document, ASSET)

    def test_changed_actual_prompt_receipt_refuses(self):
        """Actual tool/prompt/input ancestry remains required beyond matching visible scanner pixels."""
        self.candidate["generation_receipt"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            parts.component_pose("front_scanner", "N", 0, self.document, ASSET)

    def test_actual_status_parts_preserve_hull_even_inside_panel_region(self):
        """All24 actual cues change only original hatch pixels, preserving every other indexed pixel and all craft opacity."""
        protected = 0
        # The earlier whole-frame trial changed hull shading inside the broad ROI. Compare every actual pixel against the approved physical hatch mask, rather than trusting a rectangle-only counter.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            editable = {tuple(point) for point in self.family["status_edit_pixels"][direction]}
            phases = []
            for state, phase in (("blocked", 0), ("service", 0), ("service", 1)):
                pose, _ = status.status_pose(state, direction, phase, self.document, ASSET)
                for y in range(48):
                    for x in range(64):
                        self.assertEqual(base.getpixel((x, y)) == 0, pose.getpixel((x, y)) == 0)
                        if (x, y) not in editable:
                            self.assertEqual(base.getpixel((x, y)), pose.getpixel((x, y)))
                            protected += 1
                if state == "service":
                    phases.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(phases[0].get_flattened_data(), phases[1].get_flattened_data(), strict=True)), 1)
        self.assertEqual(protected, 73620)

    def test_widened_hatch_mask_refuses(self):
        """An extra hull pixel inside the broad region cannot silently become editable after panel approval."""
        self.family["status_edit_pixels"]["N"].append([30, 28])
        with self.assertRaisesRegex(ValueError, "registration"):
            status.status_pose("service", "N", 0, self.document, ASSET)

    def test_miner_panel_cannot_supply_transverse_survey_hatch(self):
        """Different physical hatch orientation requires actual survey panel originals, beyond matching a logical service label."""
        self.family["status_sources"]["service"]["0"]["N"] = self.document["families"][parts.ASSET]["status_sources"]["service"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "declared"):
            status.status_pose("service", "N", 0, self.document, ASSET)

    def test_missing_status_phase_cannot_be_filled_by_a_still(self):
        """The actual two service originals must remain distinct, rather than relabeling one connector position."""
        self.family["status_sources"]["service"]["1"]["N"] = self.family["status_sources"]["service"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "declared"):
            status.status_pose("service", "N", 1, self.document, ASSET)

    def test_registered_pulses_preserve_complete_craft_and_touch_rear_mount(self):
        """All16 real effect instances preserve every original opaque pixel and touch the approved rear edge in both phases."""
        protected = 0
        # Independently inspect original pixels and added actual gas, catching a pulse attached to empty space despite a valid source filename or overall alpha bbox.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            left, top, right, bottom = self.family["travel_component_bounds"][direction]
            phases = []
            for phase in (0, 1):
                pose, evidence = parts.component_pose("rear_plume", direction, phase, self.document, ASSET)
                self.assertFalse(evidence["source_instance"]["new_generated_artwork"])
                gas = []
                for y in range(48):
                    for x in range(64):
                        # Only transparent space inside the registered physical plume may receive authored pixels; original bus/scanner/paddles/nozzle always occlude gas.
                        if base.getpixel((x, y)) != 0 or not (left <= x < right and top <= y < bottom):
                            self.assertEqual(base.getpixel((x, y)), pose.getpixel((x, y)))
                            protected += 1
                        elif pose.getpixel((x, y)) != 0:
                            gas.append((x, y))
                contacts = self.family["travel_outlet_contacts"][direction]
                self.assertTrue(all(base.getpixel(tuple(point)) != 0 for point in contacts))
                self.assertTrue(gas)
                self.assertEqual(min(max(abs(x - old_x), abs(y - old_y)) for x, y in gas for old_x, old_y in contacts), 1)
                phases.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(phases[0].get_flattened_data(), phases[1].get_flattened_data(), strict=True)), 2)
        self.assertEqual(protected, 47572)

    def test_shared_pulse_cannot_change_actual_source_geometry(self):
        """An effect instance may change its native outlet only, retaining the genuine original crop/root/scale and heading."""
        key = self.family["travel_sources"]["0"]["N"]
        candidate = next(row for row in self.document["candidates"] if row["key"] == key)
        candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "geometry"):
            parts.component_pose("rear_plume", "N", 0, self.document, ASSET)

    def test_unapproved_actual_donor_effect_refuses(self):
        """Raw original gas pixels cannot bypass their original accepted donor registration."""
        del self.document["families"][parts.ASSET]["approved_travel_registration"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "approval"):
            parts.component_pose("rear_plume", "N", 0, self.document, ASSET)

    def test_changed_registered_survey_outlet_refuses(self):
        """Later authoring cannot move the selected actual pulse off its original rear edge."""
        key = self.family["travel_sources"]["0"]["N"]
        candidate = next(row for row in self.document["candidates"] if row["key"] == key)
        candidate["native_transforms"]["N"]["native_reference"][1] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            parts.component_pose("rear_plume", "N", 0, self.document, ASSET)

    def test_shared_instance_cannot_claim_foreign_role_hardware(self):
        """A gas instance cannot quietly authorize copying a miner bus, service hatch or drill into the distinct survey model."""
        key = self.family["travel_sources"]["0"]["N"]
        candidate = next(row for row in self.document["candidates"] if row["key"] == key)
        candidate["component_kind"] = "front_mechanism"
        with self.assertRaisesRegex(ValueError, "unsupported shared"):
            parts.component_pose("rear_plume", "N", 0, self.document, ASSET)

    def test_changed_rear_contact_mask_refuses(self):
        """A false contact cannot conceal a later floating pulse; original rear geometry is part of its frozen approval."""
        self.family["travel_outlet_contacts"]["N"].append([32, 40])
        with self.assertRaisesRegex(ValueError, "registration"):
            parts.component_pose("rear_plume", "N", 0, self.document, ASSET)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
