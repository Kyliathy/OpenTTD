"""Check model07 original cargo reuse and fixed road-body foreground cuts.

@file mine_truck_bulk_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group21B
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
These source checks neither activate art nor authorize native cargo transfers.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_mine_truck_art as truck


class MineTruckBulkChecks(unittest.TestCase):
    """Keep all native bulk refits genuine and every fixed road pixel intact.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks21B.1-21B.3
    """

    @classmethod
    def setUpClass(cls):
        """Build the actual staged source once for the focused owner burst.

        @param cls Check class retaining regenerated artifacts and native bodies.
        @return None.
        """
        cls.record = json.loads(truck.fleet.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(truck.fleet.base.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.files = truck.build(cls.record, cls.public)
        cls.family = cls.record["families"][truck.ASSET]
        cls.atlas = Image.open(io.BytesIO(cls.files[ROOT / cls.report["path"]]))
        cls.bodies = {}
        for direction in truck.fleet.DIRECTIONS:
            cls.bodies[direction], _evidence = truck.fleet.native_view(cls.family["selections"]["empty/" + direction], direction, cls.record, cls.public)

    def pose(self, row):
        """Read one exact regenerated loaded pose for geometry comparisons.

        @param self Current check instance holding the actual source atlas.
        @param row Explicit cargo/heading/load source row, never a filename guess.
        @return Indexed native64x48 pose.
        """
        x, y, width, height = row["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def test_exact_ten_refit_matrix_and_seventeen_artifacts(self):
        """Require160 real loads and80 empty aliases while exposing560 missing.

        @param self Current focused check instance.
        @return None.
        """
        expected = {(label, state, direction) for label in self.family["native_cargo_labels"]
                    for state in ("partial", "full") for direction in truck.fleet.DIRECTIONS}
        actual = {(row["cargo_label"], row["state"], row["direction"]) for row in self.report["sprites"]}
        self.assertEqual(actual, expected)
        self.assertEqual(len(actual), 160)
        self.assertEqual(self.report["empty_frame0_alias_count"], 80)
        self.assertEqual(self.report["staged_body_matrix_pose_count"], 240)
        self.assertEqual(self.report["required_body_matrix_pose_count"], 800)
        self.assertEqual(self.report["missing_pose_count"], 560)
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(len(self.files), 17)
        for path, encoded in self.files.items():
            self.assertEqual(path.read_bytes(), encoded, path.name)

    def test_all_loads_preserve_cab_steel_tyres_alpha_anchor_and_contact(self):
        """Confine every changed cargo pixel to its genuine own-road aperture.

        @param self Current focused check instance.
        @return None.
        """
        self.assertEqual(self.atlas.getpalette(), truck.fleet.base.PALETTE_BYTES)
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertFalse(set(self.atlas.get_flattened_data()) & (truck.fleet.base.FORBIDDEN - {0}))
        # Test every actual refit/heading/fill pair: a representative image can
        # miss a cargo-specific palette collapse or a single exposed tyre cut.
        for row in self.report["sprites"]:
            image, body = self.pose(row), self.bodies[row["direction"]]
            mask = truck.cargo_mask(row["direction"], row["state"])
            self.assertNotEqual(image.tobytes(), body.tobytes())
            self.assertEqual(bytes(bool(p) for p in image.get_flattened_data()), bytes(bool(p) for p in body.get_flattened_data()))
            self.assertEqual(image.getbbox()[3], 40)
            self.assertEqual(row["anchor"], [-32, -32])
            for before, after, allowed in zip(body.get_flattened_data(), image.get_flattened_data(), mask.get_flattened_data()):
                if not allowed:
                    self.assertEqual(before, after)

    def test_partial_is_rear_bank_and_samples_same_full_material_coordinates(self):
        """Preserve the existing rear bank when actual cargo reaches full.

        @param self Current focused check instance.
        @return None.
        """
        rows = {(row["cargo_label"], row["state"], row["direction"]): row for row in self.report["sprites"]}
        # The same occupied pixels must keep the same original material; full
        # adds cargo rather than repainting partial or growing the truck body.
        for label in self.family["native_cargo_labels"]:
            for direction in truck.fleet.DIRECTIONS:
                partial, full = truck.cargo_mask(direction, "partial"), truck.cargo_mask(direction, "full")
                occupied = [i for i, value in enumerate(partial.get_flattened_data()) if value]
                full_pixels = list(full.get_flattened_data())
                self.assertGreater(len(occupied), 0)
                self.assertLess(len(occupied), sum(bool(p) for p in full_pixels))
                self.assertTrue(all(full_pixels[i] for i in occupied))
                small = list(self.pose(rows[label, "partial", direction]).get_flattened_data())
                large = list(self.pose(rows[label, "full", direction]).get_flattened_data())
                self.assertTrue(all(small[i] == large[i] for i in occupied))
        # Actual cab-front position reverses which end of the bed is rear-B.
        self.assertLess(truck.cargo_mask("E", "partial").getbbox()[2], 38)
        self.assertGreater(truck.cargo_mask("W", "partial").getbbox()[0], 26)
        self.assertGreater(truck.cargo_mask("N", "partial").getbbox()[1], 26)
        self.assertLess(truck.cargo_mask("S", "partial").getbbox()[3], 25)

    def test_foreground_rims_retain_exact_original_pixels(self):
        """Keep near steel visible ahead of every actual cargo material.

        @param self Current focused check instance.
        @return None.
        """
        # Explicit rim lines describe original steel, independent of cargo color;
        # their retained pixels must survive all160 actual loaded source poses.
        for row in self.report["sprites"]:
            rim = Image.new("L", (64, 48))
            draw = ImageDraw.Draw(rim)
            for line in self.family["cargo_geometry"]["foreground_rim_lines"][row["direction"]]:
                draw.line([tuple(point) for point in line], fill=255, width=1)
            original, loaded = self.bodies[row["direction"]], self.pose(row)
            for old, new, preserve in zip(original.get_flattened_data(), loaded.get_flattened_data(), rim.get_flattened_data()):
                if preserve:
                    self.assertEqual(old, new)

    def test_earth_rock_and_eight_shared_minerals_retain_actual_originals(self):
        """Use only opaque genuine interiors without heap outlines or recoloring.

        @param self Current focused check instance.
        @return None.
        """
        ordinary = self.record["families"]["vehicle.model-05"]["approved_bulk_materials"]
        # Independent original hashes retain physical cargo meanings across the
        # small material windows; provenance is never inferred from color alone.
        for label in self.family["native_cargo_labels"]:
            material, evidence = truck.material_source(label, self.record)
            self.assertGreaterEqual(material.getchannel("A").getextrema()[0], 128)
            self.assertGreaterEqual(evidence["whole_patch_clearance"], 16)
            self.assertEqual(evidence["material"], label)
            if label in ("ERTH", "ROCK"):
                name = "earth" if label == "ERTH" else "rock"
                self.assertEqual(evidence["source_path"], "media/multilayer/source/m1/raw/cargo-" + name + "-v1.png")
                self.assertEqual(evidence["whole_patch_clearance"], 69)
            else:
                self.assertEqual(self.family["approved_bulk_materials"][label], ordinary[label])

    def test_foreign_masks_missing_views_and_changed_native_order_are_refused(self):
        """Reject rail apertures and incomplete actual road cargo/body contracts.

        @param self Current focused check instance.
        @return None.
        """
        with self.assertRaisesRegex(ValueError, "another cargo aperture"):
            truck.cargo_mask("E", "full", truck.fleet.cargo_geometry())
        # Each isolated mutation exercises the actual owning guard; reapproval
        # of a changed private copy cannot change the public geometry contract.
        for mutation in ("geometry", "heading", "order", "material"):
            record = copy.deepcopy(self.record)
            family = record["families"][truck.ASSET]
            if mutation == "geometry":
                family["cargo_geometry"]["bulk_full_polygons"]["E"][0][1] = 36
                family["approved_cargo_geometry_sha256"] = truck.geometry_signature(family)
            elif mutation == "heading":
                del family["selections"]["empty/NW"]
            elif mutation == "order":
                family["cargo_source_matrix"].reverse()
            else:
                del family["approved_bulk_materials"]["ROCK"]
            with self.assertRaises(ValueError):
                truck.build(record, self.public)

    def test_original_ancestry_crop_and_stale_fill_pointer_are_refused(self):
        """Keep material hashes, measured crops and both fill references exact.

        @param self Current focused check instance.
        @return None.
        """
        # These changes keep plausible material colors but break independent
        # original ancestry or same-material fill registration and must fail.
        for mutation in ("hash", "crop", "fill", "m1_owner", "form"):
            record = copy.deepcopy(self.record)
            family = record["families"][truck.ASSET]
            candidate = next(row for row in record["candidates"] if row["key"] == family["approved_bulk_materials"]["ERTH"]["source"])
            if mutation == "hash":
                candidate["sha256"] = "0" * 64
            elif mutation == "crop":
                candidate["component_materials"]["ERTH"]["material_crop"][0] -= 1
            elif mutation == "fill":
                family["cargo_source_matrix"][-2]["partial_source"]["material"] = "ROCK"
            elif mutation == "m1_owner":
                candidate["original_record"]["key"] = "cargo_rock"
            else:
                family["cargo_source_matrix"][-2]["form"] = "packaged"
            with self.assertRaises(ValueError):
                truck.material_source("ERTH", record)

    def test_transparent_donor_and_unrelated_alpha_cannot_be_repaired(self):
        """Refuse missing original cargo pixels or an aperture outside the body.

        @param self Current focused check instance.
        @return None.
        """
        material, _evidence = truck.material_source("ERTH", self.record)
        missing = material.copy()
        missing.putpixel((0, 0), (0, 0, 0, 0))
        with self.assertRaisesRegex(ValueError, "transparent gaps"):
            truck.compose_bulk(self.bodies["N"], missing, "N", "full")
        body = self.bodies["N"].copy()
        body.putpixel((28, 26), 0)
        with self.assertRaisesRegex(ValueError, "leaves its original body"):
            truck.compose_bulk(body, material, "N", "full")

    def test_empty_aliases_keep_all_actual_refit_slots_and_exact_headings(self):
        """Expose genuine cargo-independent empty reuse without state padding.

        @param self Current focused check instance.
        @return None.
        """
        aliases = self.report["empty_aliases"]
        expected = {(label, direction) for label in self.family["native_cargo_labels"] for direction in truck.fleet.DIRECTIONS}
        self.assertEqual({(row["cargo_label"], row["direction"]) for row in aliases}, expected)
        # Explicit source boxes must resolve to the matching genuine body;
        # aliasing cannot hide a missing direction or invent another phase.
        for row in aliases:
            atlas = Image.open(io.BytesIO(self.files[ROOT / row["source_atlas"]]))
            x, y, width, height = row["source_box"]
            self.assertEqual(atlas.crop((x, y, x + width, y + height)).tobytes(), self.bodies[row["direction"]].tobytes())
            self.assertEqual((row["state"], row["frame"]), ("empty", 0))
            self.assertEqual(self.family["native_cargo_labels"][row["tile_variant"]], row["cargo_label"])

    def test_public_road_aperture_and_controller_cannot_change_privately(self):
        """Refuse changed shared floor geometry and invented native state facts.

        @param self Current focused check instance.
        @return None.
        """
        # Source geometry and controller policy have independent public owners;
        # a modified private spec cannot authorize a different load or hold cue.
        for mutation in ("aperture", "controller"):
            public = copy.deepcopy(self.public)
            row = next(row for row in public["assets"] if row["asset_id"] == truck.ASSET)
            if mutation == "aperture":
                row["cargo_geometry"]["bulk_full_polygons"]["E"][0][1] += 1
            else:
                row["native_state_policy"]["blocked_empty"] = "Train reservation or art-local timer"
            with self.assertRaises(ValueError):
                truck.build(self.record, public)


if __name__ == "__main__":
    unittest.main()
