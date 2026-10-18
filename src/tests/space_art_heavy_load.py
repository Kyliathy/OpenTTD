"""Protect original full-ore artwork and the heavy miner's real hopper geometry.

@file space_art_heavy_load.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65.3c
Only actual original ore within the reviewed hopper may change native pixels;
the source cue grants no cargo, capacity, material type or packed/runtime art.
"""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_load_parts as loads

ASSET = "vehicle.model-21"


class HeavyMinerLoadTests(unittest.TestCase):
    """Preserve native hopper rims, foreground arms, chassis and static load identity."""

    @classmethod
    def setUpClass(cls):
        """Convert each actual original heavy load once for native pixel checks.

        @param cls Fixture sharing immutable original and loaded source pixels.
        """
        cls.source = art.read_record()
        family = cls.source["families"][ASSET]
        cls.originals, cls.poses = {}, {}
        # All genuine camera angles resolve independently through the actual production source converter, with no neighboring-view inference.
        for direction in family["directions"]:
            cls.originals[direction], _ = art.native_view(family["selections"]["free/" + direction], direction, cls.source)
            cls.poses[direction] = loads.load_pose("ore", direction, cls.source, ASSET)

    def setUp(self):
        """Detach source metadata before every independent refusal case.

        @param self Actual original cargo-volume corruption fixture.
        """
        self.record = copy.deepcopy(self.source)
        self.family = self.record["families"][ASSET]

    def test_all_eight_genuine_load_views_are_readable(self):
        """Original ore remains distinguishable from the real empty floor in every view.

        @param self Actual selected/native original source fixture.
        """
        self.assertEqual(len(self.poses), 8)
        # Genuine full sources must contain their own visible ore pixels and immutable receipts distinct from the approved empty original.
        for direction, (pose, evidence) in self.poses.items():
            self.assertEqual(pose.size, (64, 48))
            self.assertGreaterEqual(evidence["changed_pixels"], 4)
            self.assertNotEqual(evidence["component"]["original"], evidence["base"]["original"])
            self.assertNotEqual(pose.tobytes(), self.originals[direction].tobytes())

    def test_exact_rim_foreground_hardware_and_opacity_survive(self):
        """Only frozen interior pixels change; no original hardware or empty space moves.

        @param self Full native-canvas comparison fixture.
        """
        # Include the original gray frame, yellow rims, grippers, hopper walls and transparent surroundings, rather than checking only a broad cargo bounding box.
        for direction, (pose, evidence) in self.poses.items():
            original = self.originals[direction]
            editable = {tuple(point) for point in evidence["edit_pixels"]}
            self.assertEqual(evidence["protected_pixels"], 3072 - len(editable))
            self.assertEqual(evidence["added_cargo_opacity"], 0)
            for y in range(48):
                for x in range(64):
                    old, new = original.getpixel((x, y)), pose.getpixel((x, y))
                    self.assertEqual(old == 0, new == 0)
                    if (x, y) not in editable:
                        self.assertEqual(old, new)

    def test_full_ore_has_only_one_actual_static_phase(self):
        """A second load animation cannot be invented by relabeling the same original.

        @param self Static original cargo cue fixture.
        """
        with self.assertRaisesRegex(ValueError, "unsupported"):
            loads.load_pose("ore-1", "N", self.record, ASSET)

    def test_foreign_material_original_cannot_fill_heavy_hopper(self):
        """Carrier Steel artwork cannot become heavy-miner ore through a source alias.

        @param self Cross-role cargo original fixture.
        """
        self.family["load_sources"]["ore"]["N"] = self.record["families"][loads.ASSET]["load_sources"]["steel"]["N"]
        with self.assertRaisesRegex(ValueError, "not the declared original cargo load"):
            loads.load_pose("ore", "N", self.record, ASSET)

    def test_existing_carrier_cannot_borrow_heavy_ore(self):
        """The default carrier retains its actual original Steel/Goods admission profile.

        @param self Existing load-consumer compatibility fixture.
        """
        with self.assertRaisesRegex(ValueError, "unsupported"):
            loads.load_pose("ore", "N", self.record)

    def test_missing_original_heading_refuses(self):
        """A missing full-ore original cannot be filled by another camera.

        @param self Absent actual load view fixture.
        """
        del self.family["load_sources"]["ore"]["W"]
        with self.assertRaisesRegex(ValueError, "heading"):
            loads.load_pose("ore", "W", self.record, ASSET)

    def test_changed_original_ancestry_refuses(self):
        """Real original bytes and the exact ImageGen receipt remain mandatory.

        @param self Changed retained source identity fixture.
        """
        candidate = art.source_candidate(self.family["load_sources"]["ore"]["N"], self.record)
        candidate["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            loads.load_pose("ore", "N", self.record, ASSET)

    def test_hopper_registration_drift_refuses(self):
        """Loaded source registration cannot move independently of the original hopper.

        @param self Shifted original cargo source fixture.
        """
        candidate = art.source_candidate(self.family["load_sources"]["ore"]["N"], self.record)
        candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "registration changed"):
            loads.load_pose("ore", "N", self.record, ASSET)

    def test_foreground_rim_cannot_become_editable(self):
        """A cargo-volume mask cannot cover its original front lip or arm hardware.

        @param self Overlapping physical foreground fixture.
        """
        self.family["load_foreground_regions"]["N"].append([26, 29, 27, 30])
        with self.assertRaisesRegex(ValueError, "foreground"):
            loads.load_pose("ore", "N", self.record, ASSET)

    def test_colored_hardware_refuses_even_with_new_approval(self):
        """A reviewed load mask cannot turn the original blue electronics housing into ore.

        @param self Invalid physical cargo-volume ownership fixture.
        """
        self.family["load_edit_pixels"]["N"].append([32, 22])
        self.family["load_component_bounds"]["N"][1] = 22
        candidate = art.source_candidate(self.family["load_sources"]["ore"]["N"], self.record)
        self.family["approved_load_registration"]["ore"]["N"] = loads.load_signature(
            candidate, "N", self.family["load_component_bounds"]["N"], self.family["approved_base_registration"]["N"],
            self.family["load_edit_pixels"]["N"], self.family["load_foreground_regions"]["N"])
        with self.assertRaisesRegex(ValueError, "frame hardware"):
            loads.load_pose("ore", "N", self.record, ASSET)

    def test_partial_source_review_retains_full_matrix_and_runtime_pending(self):
        """Eight static load views do not imply all128 source poses or runtime cargo.

        @param self Dated original-source evidence fixture.
        """
        path = ROOT / "logs/proofs/mlm/native-art/space/heavy-miner-ore-v1/load-source-review.json"
        report = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(report["asset_id"], ASSET)
        self.assertEqual(len(report["poses"]), 8)
        self.assertEqual(report["actual_phases_per_load"], 1)
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime"])


if __name__ == "__main__":
    unittest.main()
