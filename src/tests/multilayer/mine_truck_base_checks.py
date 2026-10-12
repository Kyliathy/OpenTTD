"""Verify selected road truck originals before cargo and state authoring.

@file mine_truck_base_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group21
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Native model07 owns cargo, road length and controller state; art grants none.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_ground_fleet_art as fleet
from tools.ground_fleet_contract import native_cargo_forms, native_cargo_labels, variant_contract


class MineTruckBaseChecks(unittest.TestCase):
    """Protect genuine road headings, original ancestry and partial coverage.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks21.1-21.2
    """

    @classmethod
    def setUpClass(cls):
        """Regenerate the actual eight-view base once for this focused burst.

        @param cls Check class retaining immutable inputs and generated artifacts.
        @return None.
        """
        cls.record = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.files = fleet.build_base("vehicle.model-07", cls.record, cls.public)

    def test_eight_genuine_views_and_exact_base_artifacts(self):
        """Require all distinct native headings without padding later states.

        @param self Current focused check instance.
        @return None.
        """
        self.assertTrue(self.report["base_geometry_complete"])
        self.assertEqual(self.report["body_source_pose_count"], 8)
        self.assertEqual(self.report["required_family_pose_count"], 800)
        self.assertEqual(self.report["missing_base_directions"], [])
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        atlas = Image.open(io.BytesIO(self.files[ROOT / self.report["path"]]))
        poses = []
        # An eight-slot atlas must contain eight actual original views, rather
        # than rotated, repeated or missing-source substitutes.
        for row in self.report["sprites"]:
            x, y, width, height = row["box"]
            poses.append(atlas.crop((x, y, x + width, y + height)).tobytes())
        self.assertEqual(len(set(poses)), 8)
        for path, encoded in self.files.items():
            self.assertEqual(path.read_bytes(), encoded, path.name)

    def test_palette_whole_source_clearance_and_fixed_road_contact(self):
        """Keep the complete truck inside its native canvas and contact row.

        @param self Current focused check instance.
        @return None.
        """
        atlas = Image.open(io.BytesIO(self.files[ROOT / self.report["path"]]))
        self.assertEqual(atlas.mode, "P")
        self.assertEqual(atlas.info["transparency"], 0)
        self.assertEqual(atlas.getpalette(), fleet.base.PALETTE_BYTES)
        self.assertFalse(set(atlas.get_flattened_data()) & (fleet.base.FORBIDDEN - {0}))
        # Source clearance protects mirrors, bumper and tyres before the same
        # fixed native placement is consumed by ordinary road drawing.
        for row in self.report["sprites"]:
            self.assertEqual(row["anchor"], [-32, -32])
            self.assertEqual(row["native_opaque_bounds"][3], 40)
            self.assertGreaterEqual(row["transparent_clearance"], 16)
            self.assertEqual((row["state"], row["frame"]), ("empty", 0))

    def test_ten_actual_bulk_cargos_and_road_owned_state_policy(self):
        """Keep Earth/Rock meanings and road controller facts with native owners.

        @param self Current focused check instance.
        @return None.
        """
        labels = ["COAL", "IORE", "CORE", "BAUX", "SALT", "LITH", "NORE", "COBT", "ERTH", "ROCK"]
        self.assertEqual(native_cargo_labels(7), labels)
        self.assertEqual(native_cargo_forms(7), dict.fromkeys(labels, "dry_bulk"))
        spec = fleet.public_family("vehicle.model-07", self.public)
        for key, value in variant_contract(7).items():
            self.assertEqual(spec[key], value)
        policy = spec["native_state_policy"]
        self.assertIn("RoadVehicle::cargo", policy["cargo_owner"])
        self.assertIn("IsRoadDepotTile", policy["service_empty"])
        self.assertIn("head->blocked_ctr>0", policy["blocked_empty"])
        self.assertIn("head->cur_speed==0", policy["blocked_empty"])
        self.assertNotIn("Train", json.dumps(policy))
        self.assertNotIn("RoadVehFindCloseTo", json.dumps(policy))
        family = self.record["families"]["vehicle.model-07"]
        self.assertEqual((family["capacity"], family["physical_native_length"], family["shorten_factor"]), (32, 8, 0))
        self.assertEqual((family["native_speed_units"], family["native_power_units"], family["native_weight_units"]), (140, 35, 80))
        self.assertEqual(family["introduction_year"], 1950)
        broken = copy.deepcopy(self.public)
        next(row for row in broken["assets"] if row["asset_id"] == "vehicle.model-07")["native_cargo_labels"].pop()
        with self.assertRaisesRegex(ValueError, "ownership changed"):
            fleet.build_base("vehicle.model-07", self.record, broken)

    def test_missing_heading_and_excluded_sheet_cells_remain_missing(self):
        """Refuse angled N and repeated E instead of manufacturing coverage.

        @param self Current focused check instance.
        @return None.
        """
        broken = copy.deepcopy(self.record)
        del broken["families"]["vehicle.model-07"]["selections"]["empty/N"]
        report, _files = fleet.build_base("vehicle.model-07", broken, self.public)
        self.assertEqual(report["missing_base_directions"], ["N"])
        self.assertFalse(report["base_geometry_complete"])
        # These two cells exist in the delivered sheets but were never approved
        # as native views; source proximity cannot authorize their selection.
        for key, direction in (("mine-truck-empty-n-ne-se-s-v1", "N"), ("mine-truck-empty-sw-w-nw-e-v1", "E")):
            with self.assertRaisesRegex(ValueError, "no measured requested heading"):
                fleet.native_view(key, direction, self.record, self.public)

    def test_rejected_shallow_camera_cannot_supply_se(self):
        """Preserve the rejected prototype as design provenance only.

        @param self Current focused check instance.
        @return None.
        """
        with self.assertRaisesRegex(ValueError, "rejected"):
            fleet.native_view("mine-truck-empty-se-v1", "SE", self.record, self.public)

    def test_shared_camera_refuses_foreign_rail_and_road_bodies(self):
        """Keep same-size native silhouettes attached to their actual model.

        @param self Current focused check instance.
        @return None.
        """
        # Both transfer directions protect delivered wagon sources as well as
        # the new road truck from a plausible same-canvas body substitution.
        for target, donor in (("vehicle.model-07", "vehicle.model-05"), ("vehicle.model-07", "vehicle.model-06"),
                              ("vehicle.model-05", "vehicle.model-07"), ("vehicle.model-06", "vehicle.model-07")):
            broken = copy.deepcopy(self.record)
            broken["families"][target]["selections"]["empty/E"] = broken["families"][donor]["selections"]["empty/E"]
            with self.assertRaisesRegex(ValueError, "another native model"):
                fleet.build_base(target, broken, self.public)

    def test_changed_ancestry_registration_and_native_anchor_are_refused(self):
        """Reject changed original bytes or placement before publishing pixels.

        @param self Current focused check instance.
        @return None.
        """
        # Each corruption remains isolated in memory; negative fixtures never
        # overwrite immutable raw art or the public delivered source contract.
        for mutation in ("hash", "registration", "anchor"):
            record, public = copy.deepcopy(self.record), copy.deepcopy(self.public)
            candidate = next(row for row in record["candidates"] if row["key"] == "mine-truck-empty-e-v1")
            if mutation == "hash":
                candidate["sha256"] = "0" * 64
            elif mutation == "registration":
                candidate["native_transforms"]["E"]["crop_box"][0] += 1
            else:
                next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-07")["anchor"] = [0, 0]
            with self.assertRaises(ValueError):
                fleet.build_base("vehicle.model-07", record, public)


if __name__ == "__main__":
    unittest.main()
