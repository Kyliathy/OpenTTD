"""Verify original hauler bay depth and immutable foreground ownership.

@file space_art_hauler_bay.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group66B.2
These focused source checks grant no native freight, service or runtime art.
"""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_hauler_parts as hauler


class HaulerBayTests(unittest.TestCase):
    """Refuse cargo geometry that moves the original body or covers near hardware."""

    def setUp(self):
        """Read detached source definitions for each geometry refusal.

        @param self Independent original-hauler bay fixture.
        """
        self.record = art.read_record()
        self.family = self.record["families"][hauler.ASSET]

    def test_all_eight_bays_have_real_floor_contact_and_protected_foreground(self):
        """Cargo rises from each real floor while near rails/head/nozzle remain exact.

        @param self Genuine eight-camera depth and source-ownership fixture.
        """
        # Each camera owns a separately reviewed floor and depth order. A shared
        # rectangle or opposite-view transform cannot silently repaint hardware.
        for direction in self.family["directions"]:
            editable, foreground, footprint, evidence = hauler.bay_geometry(direction, self.record)
            self.assertFalse(editable & foreground)
            self.assertGreaterEqual(len(editable), 120)
            self.assertIn(tuple(evidence["geometry"]["floor_reference"]), footprint)
            self.assertEqual(evidence["geometry"]["cargo_height"], 4)
            self.assertEqual(evidence["base_registration"], self.family["approved_base_registration"][direction])
            names = evidence["geometry"]["foreground_polygons"]
            self.assertIn("head_display", names)
            self.assertTrue(any("nozzle" in name for name in names))

    def test_changed_approved_volume_is_refused_before_source_assembly(self):
        """Widening cargo ownership cannot gain permission to cover the original hull.

        @param self Tampered per-view projected-volume fixture.
        """
        self.family["cargo_bays"]["E"]["cargo_footprint"][0][0] -= 1
        with self.assertRaisesRegex(ValueError, "approved hauler bay geometry changed"):
            hauler.bay_geometry("E", self.record)

    def test_moved_original_cannot_inherit_the_old_bay(self):
        """Moving a source world contact invalidates the body and all cargo placement.

        @param self Original registration-drift fixture.
        """
        key = self.family["selections"]["free/N"]
        candidate = art.source_candidate(key, self.record)
        candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "hauler bay original registration changed"):
            hauler.bay_geometry("N", self.record)

    def test_foreign_role_cannot_supply_hauler_floor_permissions(self):
        """A construction carrier's deck cannot acquire general-freight source rights.

        @param self Cross-role body replacement fixture.
        """
        self.family["selections"]["free/N"] = self.record["families"]["vehicle.model-20"]["selections"]["free/N"]
        with self.assertRaisesRegex(ValueError, "hauler bay original registration changed"):
            hauler.bay_geometry("N", self.record)

    def test_signed_but_floating_or_taller_geometry_is_refused(self):
        """Fresh metadata approval cannot invent a taller hold or lose floor contact.

        @param self Invalid physical contact and cargo-height fixture.
        """
        # These independent structural failures remain invalid even if a draft
        # recalculates its approval; the native floor and one height are fixed.
        for field, value in (("floor_reference", [0, 0]), ("cargo_height", 5)):
            record = copy.deepcopy(self.record)
            family = record["families"][hauler.ASSET]
            family["cargo_bays"]["N"][field] = value
            family["approved_cargo_bay_registration"]["N"] = hauler.bay_signature(family, "N")
            with self.assertRaisesRegex(ValueError, "original floor contact"):
                hauler.bay_geometry("N", record)

    def test_bay_review_binds_actual_pixels_without_claiming_authored_loads(self):
        """The diagnostic review verifies floor depth and keeps load artwork pending.

        @param self Hash-bound eight-camera geometry review fixture.
        """
        reference = self.family["cargo_bay_review"]
        self.assertEqual(art.identity(ROOT / reference["path"]), reference)
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        self.assertFalse(report["cargo_originals_authored"])
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["runtime"])
        self.assertEqual([item["direction"] for item in report["views"]], self.family["directions"])
        # The report must describe each actual immutable native body, including
        # every originally transparent pixel where future cargo may appear.
        for item in report["views"]:
            direction = item["direction"]
            editable, _, _, evidence = hauler.bay_geometry(direction, self.record)
            base, original = art.native_view(self.family["selections"]["free/" + direction], direction, self.record)
            self.assertEqual(item["base"], original)
            self.assertEqual(item["signature"], evidence["signature"])
            self.assertEqual(item["editable_pixels"], evidence["editable_pixels"])
            self.assertEqual(item["occluded_original_pixels"], sum(base.getpixel(point) != 0 for point in editable))
            self.assertEqual(item["protected_outside_pixels"], 3072 - len(editable))


if __name__ == "__main__":
    unittest.main()
