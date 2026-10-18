"""Verify fixed Power hardware without changing the original paid panel or utility account.

@file space_art_power.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13H
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group73.1
Native paid activation, original panel availability and the shared utility remain authoritative.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_power_art as power
from tools import orbital_site_art as site

REVIEW = ROOT / "logs/proofs/mlm/native-art/space/power-base-v1/base-source-review.json"


class PowerBaseTests(unittest.TestCase):
    """Protect paid Power geometry and its single passive construction contact."""

    @classmethod
    def setUpClass(cls):
        """Load actual immutable base proof and original source registrations.

        @param cls Fixture sharing accepted original source/proof pixels.
        """
        cls.source = art.read_record()
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        for artifact in cls.report["artifacts"]:
            assert art.identity(ROOT / artifact["path"]) == artifact
        cls.native = Image.open(REVIEW.parent / "native.png")
        cls.coupling = Image.open(REVIEW.parent / "coupling.png")

    def setUp(self):
        """Detach mutable metadata so refusal tests cannot alter actual originals.

        @param self Current isolated original/geometry corruption fixture.
        """
        self.document = copy.deepcopy(self.source)
        self.family = self.document["families"][power.ASSET]

    def candidate(self, key):
        """Locate one exact detached original or explicit reused-hardware record.

        @param self Current isolated fixture.
        @param key Explicit source identity, never filename/latest-run discovery.
        @return Detached candidate used by the focused refusal case.
        """
        return next(row for row in self.document["candidates"] if row["key"] == key)

    def test_public_contract_retains_actual_fixed_four_world_cells(self):
        """Power remains2x2 with original64x64 cells, fixed anchor and no invented rotation.

        @param self Actual source/public geometry fixture.
        """
        row = power.public_contract()
        self.assertEqual(row["footprint"], [2, 2])
        self.assertEqual(row["directions"], ["none"])
        self.assertEqual(row["tile_variants"], 4)
        self.assertEqual(row["anchor"], [-31, -33])
        self.assertEqual(self.family["native_world_cells"], [[0, 0], [1, 0], [0, 1], [1, 1]])

    def test_every_real_original_pixel_fits_a_native_world_cell(self):
        """No source pixel relies on clipping outside the real sorting-cell union.

        @param self Stored whole-model base pixels.
        """
        coverage = [0] * 4
        # Derive real canvases independently from world-cell coordinates;
        # source-local coverage labels cannot conceal a clipped floor corner.
        for y in range(160):
            for x in range(192):
                if not self.native.getpixel((x, y)):
                    continue
                owners = []
                for tile in range(4):
                    wx, wy = tile % 2, tile // 2
                    ox, oy = 65 + 32 * (wy - wx), 15 + 16 * (wx + wy)
                    if 0 <= x - ox < 64 and 0 <= y - oy < 64:
                        owners.append(tile)
                        coverage[tile] += 1
                self.assertTrue(owners, (x, y))
        self.assertTrue(all(coverage))

    def test_fresh_ancestry_checked_conversion_matches_every_published_pixel(self):
        """Whole original conversion independently reproduces accepted body/coupling composition.

        @param self Published base and current explicit original registrations.
        """
        image, evidence = power.native_base(self.document)
        self.assertEqual(image.tobytes(), self.native.tobytes())
        self.assertEqual(image.getpalette(), self.native.getpalette())
        self.assertEqual(evidence, self.report["base"])
        self.assertEqual(hashlib.sha256(image.tobytes()).hexdigest(), evidence["native_sha256"])

    def test_passive_coupling_changes_only_its_actual_original_pixels(self):
        """One genuine construction attachment leaves all unrelated PV/control hardware exact.

        @param self Body original and single actual stored coupling.
        """
        body, evidence = power.native_body(document=self.document)
        points = {tuple(point) for point in self.report["base"]["coupling"]["original_pixels"]}
        self.assertEqual(len(points), 19)
        # Compare actual source pixels, rather than trusting a reported count,
        # so a second coupling or painted cargo cannot hide in the composition.
        for y in range(160):
            for x in range(192):
                point = (x, y)
                expected = self.coupling.getpixel(point) if point in points else body.getpixel(point)
                self.assertEqual(self.native.getpixel(point), expected)
        self.assertEqual(self.report["base"]["coupling"]["body_overlap_pixels"], 19)
        self.assertEqual(self.report["base"]["protected_body_canvas_pixels"], 30701)
        self.assertEqual(self.native.getpixel((160, 96)), 0)

    def test_reused_metal_retains_its_original_ancestry_without_dock_service(self):
        """Explicit shared hardware registration never claims another generation or Dock berth.

        @param self Original generated hardware and Power-only passive registration.
        """
        candidate = self.candidate(self.family["construction_source"])
        shared = self.candidate(power.SHARED_COUPLING)
        self.assertEqual(candidate["reused_original_key"], shared["key"])
        for key in ("path", "sha256", "image_size"):
            self.assertEqual(candidate[key], shared[key])
        self.assertEqual(self.report["base"]["coupling"]["original_generation_receipt"], shared["generation_receipt"])
        self.assertEqual(self.family["native_approach_slots"], 1)
        self.assertEqual(self.family["native_service_slots"], 0)
        self.assertEqual(self.family["native_stock_cap_per_label"], 0)
        self.assertEqual(self.family["native_producer_count"], 1)
        self.assertEqual(self.family["native_panel_rate_w"], 60000)

    def test_metadata_cannot_add_berths_cargo_or_multiply_panel_output_before_raster_access(self):
        """Source dictionaries cannot multiply original watts or add cargo, service and work.

        @param self Detached service/stock/utility corruption fixture.
        """
        # Each independent false capability must be refused even when all
        # other fields retain the accepted physical geometry and signatures.
        for field, value in (("native_approach_slots", 2), ("native_service_slots", 1),
                             ("native_stock_cap_per_label", 1024), ("native_power_producer", False),
                             ("native_producer_count", 2), ("native_panel_rate_w", 120000), ("native_work_consumer", True)):
            document = copy.deepcopy(self.source)
            document["families"][power.ASSET][field] = value
            with patch.object(site, "original_image", side_effect=AssertionError("unexpected raster access")):
                with self.assertRaisesRegex(ValueError, "single-producer ownership"):
                    power.native_base(document)

    def test_invented_energy_policy_refuses_before_raster_access(self):
        """Array appearance cannot replace the original utility ledger or credit free energy.

        @param self Detached shared-utility ownership corruption.
        """
        self.family["utility_policy"] = "Each PV bank adds another60000W producer and a local free-energy account"
        with patch.object(site, "original_image", side_effect=AssertionError("unexpected raster access")):
            with self.assertRaisesRegex(ValueError, "single-producer ownership"):
                power.native_base(self.document)

    def test_changed_public_footprint_or_rotation_is_refused(self):
        """A new source review cannot reinterpret the original site as3x3 or a rotating layout.

        @param self Corrupted detached public native contract fixture.
        """
        public = json.loads(power.PUBLIC.read_text(encoding="utf-8"))
        row = next(row for row in public["assets"] if row["asset_id"] == power.ASSET)
        row["directions"] = ["NE", "SE", "SW", "NW"]
        with self.assertRaises(ValueError):
            power.native_body(document=self.document, public=public)

    def test_unsigned_original_transform_change_is_refused(self):
        """Changed source placement cannot inherit a previously approved original registration.

        @param self Unsigned original geometry corruption fixture.
        """
        self.candidate(self.family["body_sources"]["complete"])["native_transform"]["source_reference"][0] += 1
        with patch.object(site, "original_image", side_effect=AssertionError("unexpected raster access")):
            with self.assertRaisesRegex(ValueError, "stage registration changed"):
                power.native_body(document=self.document)

    def test_new_signature_cannot_relocate_native_floor(self):
        """Re-signing a source dictionary never moves the saved world footprint.

        @param self Re-signed native origin corruption fixture.
        """
        candidate = self.candidate(self.family["body_sources"]["complete"])
        candidate["native_transform"]["native_reference"][0] += 1
        self.family["approved_body_registration"]["complete"] = site.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "whole-model native floor contract changed"):
            power.native_body(document=self.document)

    def test_new_signature_cannot_authorize_a_steeper_source_projection(self):
        """Measured original floor corners must match actual native geometry under uniform reduction.

        @param self Re-signed measured source-floor corruption fixture.
        """
        candidate = self.candidate(self.family["body_sources"]["complete"])
        candidate["native_transform"]["source_floor_corners"][2][1] += 200
        self.family["approved_body_registration"]["complete"] = site.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "does not match native projection"):
            power.native_body(document=self.document)

    def test_new_signature_cannot_reuse_a_dock_body_as_power(self):
        """Genuine originals retain family and physical component identity after re-signing.

        @param self Re-signed foreign original-role fixture.
        """
        candidate = self.candidate(self.family["body_sources"]["complete"])
        candidate["asset_id"] = "module.dock"
        self.family["approved_body_registration"]["complete"] = site.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "stage registration changed"):
            power.native_body(document=self.document)

    def test_new_signature_cannot_clip_or_move_a_whole_coupling(self):
        """A crop or large translation cannot conceal lost passive hardware before assembly.

        @param self Re-signed source crop and canvas-overflow corruption fixture.
        """
        candidate = self.candidate(self.family["construction_source"])
        candidate["native_transform"]["crop_box"][0] += 1
        self.family["approved_construction_registration"] = site.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "whole-source crop changed"):
            power.construction_component(self.document)
        candidate["native_transform"]["crop_box"][0] -= 1
        candidate["native_transform"]["source_reference"][0] -= 8000
        self.family["approved_construction_registration"] = site.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "leaves native canvas before assembly"):
            power.construction_component(self.document)

    def test_base_proof_keeps_missing_stages_packing_and_runtime_open(self):
        """A complete physical base is not full64-cell source coverage or native game delivery.

        @param self Immutable bounded base proof and public native allocation.
        """
        self.assertFalse(self.report["complete_source_coverage"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(power.public_contract()["packed"], dict(namespace="MLVI", offset=None, sha256=None))
        self.assertEqual(self.family["required_pose_count"], 64)
        self.assertEqual(self.family["native_service_slots"], 0)


if __name__ == "__main__":
    unittest.main()
