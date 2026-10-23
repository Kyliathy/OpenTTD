"""Check honest conventional wagon base coverage and immutable source boundaries.

@file ground_fleet_base_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
These checks cover empty frame-zero sources; cargo, purchase and runtime remain pending.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_ground_fleet_art as fleet


class GroundFleetBaseChecks(unittest.TestCase):
    """Protect real directional sources without implying complete fleet delivery.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks20.1-20.2
    """

    def setUp(self):
        """Load isolated contracts so negative checks never edit curated originals.

        @param self Current focused check instance.
        @return None.
        """
        self.record = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
        self.public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))

    def test_exact_base_artifacts_do_not_claim_complete_states(self):
        """Require reproducible eight-view geometry with honest remaining coverage.

        @param self Current focused check instance.
        @return None.
        """
        report, files = fleet.build_base(document=self.record, public=self.public)
        self.assertTrue(report["base_geometry_complete"])
        self.assertEqual(report["body_source_pose_count"], 8)
        self.assertEqual(report["required_family_pose_count"], 1360)
        self.assertEqual(report["missing_base_directions"], [])
        self.assertFalse(report["source_coverage_complete"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime_activated"])
        # Only actual retained source bytes may reproduce a completed view;
        # the source report and both visual artifacts must agree exactly.
        for path, expected in files.items():
            self.assertEqual(path.read_bytes(), expected, path.name)

    def test_missing_heading_stays_missing(self):
        """Prevent seven actual views from being padded into an eight-view claim.

        @param self Current focused check instance.
        @return None.
        """
        del self.record["families"]["vehicle.model-05"]["selections"]["empty/W"]
        report, _files = fleet.build_base(document=self.record, public=self.public)
        self.assertFalse(report["base_geometry_complete"])
        self.assertEqual(report["missing_base_directions"], ["W"])
        self.assertEqual(report["body_source_pose_count"], 7)

    def test_native_poses_keep_palette_anchor_and_ground_contact(self):
        """Keep all real camera views inside their registered ordinary-train canvas.

        @param self Current focused check instance.
        @return None.
        """
        report, files = fleet.build_base(document=self.record, public=self.public)
        atlas = Image.open(io.BytesIO(files[ROOT / report["path"]]))
        self.assertEqual(atlas.mode, "P")
        self.assertEqual(atlas.info["transparency"], 0)
        self.assertEqual(atlas.getpalette(), fleet.base.PALETTE_BYTES)
        self.assertFalse(set(atlas.get_flattened_data()) & (fleet.base.FORBIDDEN - {0}))
        # Each original heading keeps its own wheel/coupler body rather than
        # borrowing a mirrored source to fill an otherwise missing camera view.
        for row in report["sprites"]:
            self.assertEqual(row["anchor"], [-32, -32])
            self.assertGreaterEqual(row["transparent_clearance"], 16)
            self.assertEqual(row["native_opaque_bounds"][3], 40)
            self.assertEqual(row["state"], "empty")
            self.assertEqual(row["frame"], 0)

    def test_public_anchor_cannot_be_reinterpreted(self):
        """Refuse a world-anchor change before any selected original is converted.

        @param self Current focused check instance.
        @return None.
        """
        family = next(row for row in self.public["assets"] if row["asset_id"] == "vehicle.model-05")
        family["anchor"] = [0, 0]
        with self.assertRaisesRegex(ValueError, "public geometry"):
            fleet.build_base(document=self.record, public=self.public)

    def test_original_and_reference_hashes_are_required(self):
        """Reject altered body or ancestry bytes rather than trusting filenames.

        @param self Current focused check instance.
        @return None.
        """
        # Each independent provenance corruption must fail before resampling;
        # reference identity matters as much as the selected output's own hash.
        for target in ("original", "reference"):
            record = copy.deepcopy(self.record)
            source = next(row for row in record["candidates"] if row["key"] == "mineral-wagon-empty-e-v2")
            identity = source if target == "original" else source["references"][0]
            identity["sha256"] = "0" * 64
            with self.assertRaisesRegex(ValueError, "immutable"):
                fleet.native_view(source["key"], "E", record, self.public)

    def test_registration_drift_is_refused(self):
        """Prevent a later crop adjustment from shifting the accepted wagon body.

        @param self Current focused check instance.
        @return None.
        """
        source = next(row for row in self.record["candidates"] if row["key"] == "mineral-wagon-empty-e-v2")
        source["native_transforms"]["E"]["crop_box"][0] += 1
        with self.assertRaisesRegex(ValueError, "registration changed"):
            fleet.native_view(source["key"], "E", self.record, self.public)

    def test_separator_pixels_cannot_enter_the_model(self):
        """Refuse opaque source magenta even when the recorded crop still fits.

        @param self Current focused check instance.
        @return None.
        """
        source = next(row for row in self.record["candidates"] if row["key"] == "mineral-wagon-empty-e-v2")
        altered = Image.open(ROOT / source["path"]).convert("RGBA")
        altered.putpixel((700, 550), (255, 0, 255, 255))
        # Corrupt only the in-memory decoded subject; immutable files and their
        # hashes stay untouched while the independent pixel gate is exercised.
        with patch.object(fleet.Image, "open", return_value=altered):
            with self.assertRaisesRegex(ValueError, "opaque source separator"):
                fleet.native_view(source["key"], "E", self.record, self.public)

    def test_consistency_view_cannot_replace_selected_base(self):
        """Keep the original approved E body when a later sheet repeats that view.

        @param self Current focused check instance.
        @return None.
        """
        self.record["families"]["vehicle.model-05"]["selections"]["empty/E"] = "mineral-wagon-empty-sw-w-nw-e-v1"
        with self.assertRaisesRegex(ValueError, "registration changed"):
            fleet.build_base(document=self.record, public=self.public)


if __name__ == "__main__":
    unittest.main()
