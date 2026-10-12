"""Verify genuine sealed rail wagon originals without claiming complete states.

@file sealed_wagon_base_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20B
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
The actual model06 catalogue remains the cargo/date/layer authority.
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


class SealedWagonBaseChecks(unittest.TestCase):
    """Protect selected model identity, genuine headings and native registration.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks20B.1-20B.2
    """

    @classmethod
    def setUpClass(cls):
        """Regenerate the actual eight-view base slice once per focused burst.

        @param cls Focused check class holding immutable expected source pixels.
        @return None.
        """
        cls.record = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.files = fleet.build_base("vehicle.model-06", cls.record, cls.public)

    def test_all_actual_sealed_headings_and_exact_artifacts(self):
        """Require eight distinct actual originals while exposing missing states.

        @param self Current focused source check instance.
        @return None.
        """
        self.assertTrue(self.report["base_geometry_complete"])
        self.assertEqual(self.report["body_source_pose_count"], 8)
        self.assertEqual(self.report["required_family_pose_count"], 80)
        self.assertEqual(self.report["missing_base_directions"], [])
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertFalse(self.report["packed"])
        self.assertEqual({r["direction"] for r in self.report["sprites"]}, set(fleet.DIRECTIONS))
        # All three artifacts derive from explicit selected originals; a stale
        # atlas or contact cannot be substituted for a missing actual heading.
        for path, encoded in self.files.items():
            self.assertEqual(path.read_bytes(), encoded, path.name)
        atlas = Image.open(io.BytesIO(self.files[ROOT / self.report["path"]]))
        poses = []
        for row in self.report["sprites"]:
            x, y, width, height = row["box"]
            poses.append(atlas.crop((x, y, x + width, y + height)).tobytes())
        self.assertEqual(len(set(poses)), 8)

    def test_native_palette_contacts_and_measured_source_clearance(self):
        """Preserve train canvas, anchor, whole-source alpha and ground baseline.

        @param self Current focused source check instance.
        @return None.
        """
        atlas = Image.open(io.BytesIO(self.files[ROOT / self.report["path"]]))
        self.assertEqual(atlas.mode, "P")
        self.assertEqual(atlas.info["transparency"], 0)
        self.assertEqual(atlas.getpalette(), fleet.base.PALETTE_BYTES)
        self.assertFalse(set(atlas.get_flattened_data()) & (fleet.base.FORBIDDEN - {0}))
        # Genuine source extraction precedes native fitting; safe source margins
        # never authorize moving couplers or using another camera's silhouette.
        for row in self.report["sprites"]:
            self.assertEqual(row["anchor"], [-32, -32])
            self.assertEqual(row["native_opaque_bounds"][3], 40)
            self.assertGreaterEqual(row["transparent_clearance"], 17)
            self.assertEqual(row["frame"], 0)
            self.assertEqual(row["state"], "empty")

    def test_actual_sealed_cargo_meaning_and_closed_exterior_policy(self):
        """Retain both existing covered cargos under one physical closed body.

        @param self Current focused source check instance.
        @return None.
        """
        self.assertEqual(native_cargo_labels(6), ["URAN", "RARE"])
        self.assertEqual(native_cargo_forms(6), {"URAN": "sealed_drums", "RARE": "covered_granules"})
        spec = fleet.public_family("vehicle.model-06", self.public)
        self.assertEqual(spec["tile_variants"], 1)
        # A shared sealed exterior must still describe the complete actual
        # native cargo/state contract, including uranium's sealed-drum meaning.
        for key, value in variant_contract(6).items():
            self.assertEqual(spec[key], value)
        public = copy.deepcopy(self.public)
        row = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-06")
        row["native_cargo_labels"] = ["COAL"]
        with self.assertRaisesRegex(ValueError, "ownership changed"):
            fleet.build_base("vehicle.model-06", self.record, public)

    def test_missing_w_and_excluded_crowded_cell_never_inherit_a_view(self):
        """Keep unsafe W/repeated E source cells outside actual selected coverage.

        @param self Current focused source check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        del record["families"]["vehicle.model-06"]["selections"]["empty/W"]
        report, _files = fleet.build_base("vehicle.model-06", record, self.public)
        self.assertEqual(report["missing_base_directions"], ["W"])
        self.assertEqual(report["body_source_pose_count"], 7)
        self.assertFalse(report["base_geometry_complete"])
        with self.assertRaisesRegex(ValueError, "no measured requested heading"):
            fleet.native_view("sealed-wagon-empty-sw-w-nw-e-v1", "W", self.record, self.public)
        with self.assertRaisesRegex(ValueError, "no measured requested heading"):
            fleet.native_view("sealed-wagon-empty-sw-w-nw-e-v1", "E", self.record, self.public)

    def test_shared_camera_cannot_borrow_another_wagon_model(self):
        """Refuse matching-size foreign silhouettes in either selected family.

        @param self Current focused source check instance.
        @return None.
        """
        # Both directions of the shared-tool boundary matter: preserving the
        # completed ordinary wagon is as important as refusing a fake sealed one.
        for target, donor in (("vehicle.model-06", "vehicle.model-05"), ("vehicle.model-05", "vehicle.model-06")):
            record = copy.deepcopy(self.record)
            record["families"][target]["selections"]["empty/E"] = record["families"][donor]["selections"]["empty/E"]
            with self.assertRaisesRegex(ValueError, "another native model"):
                fleet.build_base(target, record, self.public)

    def test_source_hash_registration_and_world_anchor_cannot_drift(self):
        """Refuse changed original ancestry or placement before publishing pixels.

        @param self Current focused source check instance.
        @return None.
        """
        # Each isolated corruption exercises its own actual source/placement
        # guard without writing a negative fixture over retained originals.
        for mutation in ("hash", "registration", "anchor"):
            record, public = copy.deepcopy(self.record), copy.deepcopy(self.public)
            candidate = next(row for row in record["candidates"] if row["key"] == "sealed-wagon-empty-e-v1")
            if mutation == "hash":
                candidate["sha256"] = "0" * 64
            elif mutation == "registration":
                candidate["native_transforms"]["E"]["crop_box"][0] += 1
            else:
                next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-06")["anchor"] = [0, 0]
            with self.assertRaises(ValueError):
                fleet.build_base("vehicle.model-06", record, public)


if __name__ == "__main__":
    unittest.main()
