"""Protect complete sealed wagon sources, original parts and closed cargo custody.

@file sealed_wagon_state_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20B.3
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Source checks grant no packed range or actual game-rendering acceptance.
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
import convert_sealed_wagon_states as sealed


class SealedWagonStateChecks(unittest.TestCase):
    """Keep own06 shells/contacts while reusing only approved original materials.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md task20B.3
    """

    @classmethod
    def setUpClass(cls):
        """Regenerate the complete bounded source once for this focused burst.

        @param cls Focused check class holding immutable original expectations.
        @return None.
        """
        cls.record = json.loads(sealed.fleet.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(sealed.fleet.base.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.files = sealed.build(cls.record, cls.public)
        cls.geometry, cls.components, cls.g5, cls.c5 = sealed.family_spec(cls.record, cls.public)
        cls.gears, cls.lenses, cls.meter = sealed.component_sources(cls.record, cls.geometry, cls.components, cls.g5, cls.c5)
        cls.body_report, cls.purchase_report = cls.report["atlases"]
        cls.atlas = Image.open(io.BytesIO(cls.files[ROOT / cls.body_report["path"]]))
        cls.rows = {(r["state"], r["frame"], r["direction"]): r for r in cls.body_report["sprites"]}
        base_report = json.loads(cls.files[sealed.fleet.OUTPUT / "vehicle-model-06-base-source-manifest.json"])
        base_atlas = Image.open(io.BytesIO(cls.files[ROOT / base_report["path"]]))
        cls.bodies = {}
        # Independent earlier base pixels define every fixed shell and contact;
        # the new assembled source cannot approve its own geometry differences.
        for row in base_report["sprites"]:
            x, y, width, height = row["box"]
            cls.bodies[row["direction"]] = base_atlas.crop((x, y, x + width, y + height))

    def pose(self, state, frame, direction):
        """Decode one selected source pose independently of its component assembler.

        @param self Current focused source check instance.
        @param state Actual native packet or empty service/reservation category.
        @param frame Independently authored zero/one visual phase.
        @param direction Genuine native heading of the own06 original body.
        @return Indexed64x48 source pose with canonical palette/transparency.
        """
        x, y, width, height = self.rows[state, frame, direction]["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def points(self, rectangles):
        """Expand approved physical component regions for independent delta checks.

        @param self Current focused source check instance.
        @param rectangles Inspected own06 x/y/width/height attachment boxes.
        @return Exact coordinate set; this never erases an original source pixel.
        """
        return {(px, py) for x, y, width, height in rectangles for py in range(y, y + height) for px in range(x, x + width)}

    def test_complete_eighty_body_two_purchase_and_twelve_exact_artifacts(self):
        """Require all native identities and genuine same-exterior cargo aliases.

        @param self Current focused source check instance.
        @return None.
        """
        expected = {(state, frame, direction) for state in sealed.ordinary.STATES for frame in range(2) for direction in sealed.fleet.DIRECTIONS}
        self.assertEqual(set(self.rows), expected)
        self.assertEqual(len(self.body_report["sprites"]), 80)
        self.assertEqual(len(self.purchase_report["sprites"]), 2)
        self.assertEqual(self.report["native_cargo_forms"], {"URAN": "sealed_drums", "RARE": "covered_granules"})
        self.assertTrue(self.report["source_coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(len(self.files), 12)
        # Complete coverage includes exact original source/phase contacts, not
        # just plausible identity rows over stale atlas or preview files.
        for path, encoded in self.files.items():
            self.assertEqual(path.read_bytes(), encoded, path.name)
        for row in self.body_report["sprites"]:
            self.assertEqual(row["cargo_labels"], ["URAN", "RARE"])
            self.assertEqual(row["tile_variant"], 0)

    def test_closed_shell_alpha_couplers_and_contacts_change_only_at_fittings(self):
        """Keep the actual own-model body outside explicit original material inserts.

        @param self Current focused source check instance.
        @return None.
        """
        # Every output must retain the same opaque shell, closed roof/hatches
        # and contact row. Material sharing cannot transfer a foreign body.
        for identity in self.rows:
            state, frame, direction = identity
            body, image = self.bodies[direction], self.pose(*identity)
            allowed = self.points([self.geometry["company_regions"][direction]])
            if state in ("partial", "full"):
                allowed |= {tuple(p) for p in self.geometry["sealed_meter_points"][direction]}
            if state in ("empty", "partial", "full") and frame == 1:
                # Only actual rolling phases may use wheel interiors; loaded
                # packet categories retain their independent meter presentation.
                allowed |= self.points(self.geometry["gear_regions"][direction])
            if state in ("service", "blocked"):
                allowed |= self.points([self.geometry["status_regions"][direction]])
            self.assertEqual(bytes(v != 0 for v in body.tobytes()), bytes(v != 0 for v in image.tobytes()))
            self.assertEqual(image.crop((0, 39, 64, 48)).tobytes(), body.crop((0, 39, 64, 48)).tobytes())
            for index, (before, after) in enumerate(zip(body.tobytes(), image.tobytes())):
                if before != after:
                    self.assertIn((index % 64, index // 64), allowed)

    def test_partial_full_preserve_existing_meter_and_never_open_cargo(self):
        """Keep one/two original amber fitting pixels as packet-backed categories.

        @param self Current focused source check instance.
        @return None.
        """
        for direction in sealed.fleet.DIRECTIONS:
            first, second = map(tuple, self.geometry["sealed_meter_points"][direction])
            for frame in range(2):
                empty, partial, full = (self.pose(state, frame, direction) for state in ("empty", "partial", "full"))
                self.assertEqual(partial.getpixel(first), self.meter[0])
                self.assertEqual(full.getpixel(first), self.meter[0])
                self.assertEqual(full.getpixel(second), self.meter[0])
                self.assertEqual(partial.getpixel(second), empty.getpixel(second))
                differences = {(index % 64, index // 64) for index, (a, b) in enumerate(zip(partial.tobytes(), full.tobytes())) if a != b}
                self.assertEqual(differences, {second})
                self.assertNotEqual(empty.tobytes(), partial.tobytes())

    def test_rolling_uses_only_exact_same_heading_original_axle_samples(self):
        """Retarget genuine wheel pixels without copying another wagon's chassis.

        @param self Current focused source check instance.
        @return None.
        """
        donor_gears, _lenses = sealed.ordinary.state_sources(self.record, self.g5, self.c5)
        # Each output sample must equal its explicit original same-camera axle
        # pixel; shifts, recolouring or a convenient foreign body cannot pass.
        for direction in sealed.fleet.DIRECTIONS:
            targets = self.geometry["gear_regions"][direction]
            sources = self.components["gear_source"]["source_points"][direction]
            allowed = self.points(targets)
            for state in ("empty", "partial", "full"):
                first, second = self.pose(state, 0, direction), self.pose(state, 1, direction)
                differences = {(index % 64, index // 64) for index, (a, b) in enumerate(zip(first.tobytes(), second.tobytes())) if a != b}
                self.assertTrue(differences)
                self.assertTrue(differences <= allowed)
                for (x, y, _width, _height), point in zip(targets, sources):
                    self.assertEqual(second.getpixel((x, y)), donor_gears[direction][0].getpixel(tuple(point)))

    def test_empty_status_phases_keep_static_gear_and_distinct_real_glass(self):
        """Preserve stopped axles and all four authored cyan/red presentations.

        @param self Current focused source check instance.
        @return None.
        """
        for direction in sealed.fleet.DIRECTIONS:
            allowed = self.points([self.geometry["status_regions"][direction]])
            presentations = []
            for state in ("service", "blocked"):
                first, second = self.pose(state, 0, direction), self.pose(state, 1, direction)
                differences = {(index % 64, index // 64) for index, (a, b) in enumerate(zip(first.tobytes(), second.tobytes())) if a != b}
                self.assertEqual(differences, allowed)
                for frame in range(2):
                    self.assertIsNone(self.rows[state, frame, direction]["components"]["gear"])
                    self.assertIsNone(self.rows[state, frame, direction]["load_source"])
                    presentations.append(self.pose(state, frame, direction).tobytes())
            self.assertEqual(len(set(presentations)), 4)

    def test_company_mask_palette_and_hidden_end_fittings_remain_explicit(self):
        """Keep reserved livery solely on actual visible original blue plaques.

        @param self Current focused source check instance.
        @return None.
        """
        forbidden = sealed.fleet.base.FORBIDDEN - set(range(198, 206))
        for identity in self.rows:
            direction = identity[2]
            image = self.pose(*identity)
            expected = self.points([self.geometry["company_regions"][direction]])
            remapped = {(index % 64, index // 64) for index, v in enumerate(image.tobytes()) if 198 <= v <= 205}
            self.assertEqual(remapped, expected)
            self.assertFalse((set(image.tobytes()) - {0}) & forbidden)
            self.assertEqual(image.getpalette(), sealed.fleet.base.PALETTE_BYTES)
            if direction in ("N", "S"):
                self.assertFalse(remapped)

    def test_own_model_purchase_preserves_all_preview_pixels_and_original_lock(self):
        """Keep the real sealed-body preview above its separate genuine UI badge.

        @param self Current focused source check instance.
        @return None.
        """
        atlas = Image.open(io.BytesIO(self.files[ROOT / self.purchase_report["path"]]))
        available, locked = atlas.crop((0, 0, 20, 20)), atlas.crop((20, 0, 40, 20))
        # The18x6 uniform fit retains17 opaque columns after the native alpha
        # threshold; source edge transparency must not be padded into artwork.
        self.assertEqual(available.getbbox(), (1, 7, 18, 13))
        for before, after in zip(available.tobytes(), locked.tobytes()):
            if before != 0:
                self.assertEqual(before, after)
        self.assertNotEqual(available.tobytes(), locked.tobytes())
        lock = self.purchase_report["sprites"][1]["lock_component"]
        self.assertEqual(lock["family"], "control.future-lock")
        self.assertEqual(lock["target"], [12, 13, 7, 7])
        self.assertEqual(sealed.fleet.base.digest(ROOT / lock["source"]), lock["source_sha256"])
        self.assertEqual(self.purchase_report["sprites"][0]["body_source"]["source"], "sealed-wagon-empty-e-v1")

    def test_foreign_shared_owner_and_changed_own_attachment_are_refused(self):
        """Reject plausible substitutes even after isolated malformed approval.

        @param self Current focused source check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        family = record["families"]["vehicle.model-06"]
        family["state_components"]["gear_source"]["source_family"] = "vehicle.model-07"
        family["approved_state_components_sha256"] = sealed.ordinary.signature(family["state_components"])
        with self.assertRaisesRegex(ValueError, "foreign sealed wagon shared"):
            sealed.family_spec(record, self.public)
        record = copy.deepcopy(self.record)
        record["families"]["vehicle.model-06"]["state_component_geometry"]["status_regions"]["E"][0] += 1
        with self.assertRaisesRegex(ValueError, "geometry or ownership changed"):
            sealed.family_spec(record, self.public)

    def test_roof_pixel_cannot_impersonate_shared_wheel_material(self):
        """Require actual original axle interior before retargeting its colour.

        @param self Current focused source check instance.
        @return None.
        """
        components = copy.deepcopy(self.components)
        components["gear_source"]["source_points"]["E"][0] = [32, 30]
        with self.assertRaisesRegex(ValueError, "leaves approved source/target"):
            sealed.component_sources(self.record, self.geometry, components, self.g5, self.c5)

    def test_missing_heading_duplicate_meter_and_foreign_glass_are_refused(self):
        """Keep complete genuine coverage and independent packet-category fittings.

        @param self Current focused source check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        family = record["families"]["vehicle.model-06"]
        del family["state_components"]["gear_source"]["source_points"]["NW"]
        family["approved_state_components_sha256"] = sealed.ordinary.signature(family["state_components"])
        with self.assertRaisesRegex(ValueError, "lacks a genuine"):
            sealed.family_spec(record, self.public)
        geometry = copy.deepcopy(self.geometry)
        geometry["sealed_meter_points"]["E"][1] = geometry["sealed_meter_points"]["E"][0]
        with self.assertRaisesRegex(ValueError, "absent or ambiguous"):
            sealed.closed_load(self.bodies["E"], "E", "full", geometry, self.meter)
        components = copy.deepcopy(self.components)
        components["meter_source"]["source_key"] = "private.random-glass"
        with self.assertRaisesRegex(ValueError, "foreign sealed wagon load meter"):
            sealed.component_sources(self.record, self.geometry, components, self.g5, self.c5)

    def test_stale_artifact_cannot_publish_shared_source_metadata(self):
        """Refuse stale original proof before changing either public source family.

        @param self Current focused source check instance.
        @return None.
        """
        files = dict(self.files)
        files[ROOT / self.purchase_report["path"]] += b"stale proof"
        previous = sealed.fleet.base.PUBLIC.read_bytes()
        with self.assertRaisesRegex(ValueError, "requires exact artifacts"):
            sealed.publish_source(self.record, copy.deepcopy(self.public), self.report, files)
        self.assertEqual(sealed.fleet.base.PUBLIC.read_bytes(), previous)

    def test_metadata_only_family_substitution_cannot_publish_verified_pixels(self):
        """Bind exact verified source metadata to its actual own-model consumers.

        @param self Current focused source check instance.
        @return None.
        """
        report = copy.deepcopy(self.report)
        report["atlases"][0]["asset_id"] = "vehicle.model-05"
        previous = sealed.fleet.base.PUBLIC.read_bytes()
        with self.assertRaisesRegex(ValueError, "metadata differs from verified proof"):
            sealed.publish_source(self.record, copy.deepcopy(self.public), report, self.files)
        self.assertEqual(sealed.fleet.base.PUBLIC.read_bytes(), previous)


if __name__ == "__main__":
    unittest.main()
