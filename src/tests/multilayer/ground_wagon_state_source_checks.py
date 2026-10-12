"""Check complete wagon sources against independent fixed body/load pixels.

@file ground_wagon_state_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20E
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
These focused source checks grant no packed range or ordinary-game acceptance.
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
import convert_ground_wagon_states as states


class GroundWagonStateSourceChecks(unittest.TestCase):
    """Protect real source coverage, fixed hardware and native state ownership.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks20E.1-20E.3
    """

    @classmethod
    def setUpClass(cls):
        """Regenerate one complete source handoff for this bounded focused burst.

        @param cls Focused check class caching immutable pixels and registrations.
        @return None.
        """
        cls.record = json.loads(states.fleet.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(states.fleet.base.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.files = states.build(cls.record, cls.public)
        cls.family = cls.record["families"]["vehicle.model-05"]
        cls.geometry, cls.components = states.checked_geometry(cls.record, cls.public)
        cls.gears, cls.lenses = states.state_sources(cls.record, cls.geometry, cls.components)
        cls.body_report, cls.purchase_report = cls.report["atlases"]
        cls.atlas = Image.open(io.BytesIO(cls.files[ROOT / cls.body_report["path"]]))
        cls.rows = {(r["cargo_label"], r["state"], r["frame"], r["direction"]): r for r in cls.body_report["sprites"]}
        cls.baselines = {}
        # Earlier immutable base/bulk/object atlases are independent references
        # for the new state assembler; a state report cannot redefine its body.
        for name in ("base", "bulk", "objects"):
            path = states.fleet.OUTPUT / ("vehicle-model-05-" + name + "-source-manifest.json")
            report = json.loads(cls.files[path])
            atlas = Image.open(io.BytesIO(cls.files[ROOT / report["path"]]))
            for row in report["sprites"]:
                x, y, width, height = row["box"]
                cls.baselines[row.get("cargo_label"), row["state"], row["direction"]] = atlas.crop((x, y, x + width, y + height))

    def pose(self, label, state, frame, direction):
        """Decode one actual selected body pose without reapplying its assembler.

        @param self Current focused source check instance.
        @param label Existing native cargo label, preserving separate variants.
        @param state Native packet-backed load or actual empty status category.
        @param frame Authored zero/one visual phase.
        @param direction Actual engine heading with its own original chassis.
        @return Indexed64x48 selected atlas crop.
        """
        x, y, width, height = self.rows[label, state, frame, direction]["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def points(self, boxes):
        """Expand declared opaque native component rectangles for delta checks.

        @param self Current focused source check instance.
        @param boxes Reviewed native attachment boxes, each x/y/width/height.
        @return Exact affected coordinate set, never a source-pixel erasing mask.
        """
        return {(px, py) for x, y, width, height in boxes for py in range(y, y + height) for px in range(x, x + width)}

    def test_complete_real_identity_matrix_and_exact_artifacts(self):
        """Require every actual cargo/state/frame/view plus both purchase states.

        @param self Current focused source check instance.
        @return None.
        """
        labels = self.family["native_cargo_labels"]
        expected = {(label, state, frame, direction) for label in labels for state in states.STATES for frame in range(2) for direction in states.fleet.DIRECTIONS}
        self.assertEqual(len(expected), 1360)
        self.assertEqual(set(self.rows), expected)
        self.assertEqual(len(self.body_report["sprites"]), 1360)
        self.assertEqual(self.report["purchase_pose_count"], 2)
        self.assertTrue(self.report["source_coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(len(self.files), 34)
        # Every explicit artifact must regenerate exactly, including all five
        # native phase contacts and the actual two-state purchase preview.
        for path, encoded in self.files.items():
            self.assertEqual(path.read_bytes(), encoded, path.name)

    def test_only_reviewed_components_change_fixed_body_or_load(self):
        """Preserve load pixels, steel, couplers, contacts and body opacity.

        @param self Current focused source check instance.
        @return None.
        """
        # All1,360 outputs must preserve their independent earlier source pose
        # except the explicit livery, rolling axle or empty status attachment.
        for label, state, frame, direction in self.rows:
            source_state = state if state in ("partial", "full") else "empty"
            baseline = self.baselines[label if source_state != "empty" else None, source_state, direction]
            image = self.pose(label, state, frame, direction)
            allowed = self.points([self.geometry["company_regions"][direction]])
            if state in ("empty", "partial", "full") and frame == 1:
                # Actual rolling phases may change axle interiors, preserving
                # their native contacts and the complete packet-backed load.
                allowed |= self.points(self.geometry["gear_regions"][direction])
            if state in ("service", "blocked"):
                allowed |= self.points([self.geometry["status_regions"][direction]])
            original, actual = baseline.tobytes(), image.tobytes()
            self.assertEqual(bytes(value != 0 for value in original), bytes(value != 0 for value in actual))
            for index, (before, after) in enumerate(zip(original, actual)):
                if before != after:
                    self.assertIn((index % 64, index // 64), allowed)
            self.assertEqual(image.crop((0, 39, 64, 48)).tobytes(), baseline.crop((0, 39, 64, 48)).tobytes())

    def test_every_rolling_phase_changes_only_real_axles(self):
        """Require visible authored wheel differences without cargo/status drift.

        @param self Current focused source check instance.
        @return None.
        """
        # Every refit must retain its load through both rolling phases; testing
        # only the empty body would miss cargo overwritten by a wheel donor.
        for label in self.family["native_cargo_labels"]:
            for direction in states.fleet.DIRECTIONS:
                allowed = self.points(self.geometry["gear_regions"][direction])
                for state in ("empty", "partial", "full"):
                    first, second = self.pose(label, state, 0, direction), self.pose(label, state, 1, direction)
                    differences = {(index % 64, index // 64) for index, (a, b) in enumerate(zip(first.tobytes(), second.tobytes())) if a != b}
                    self.assertTrue(differences)
                    self.assertTrue(differences <= allowed)
                    self.assertIsNone(self.rows[label, state, 1, direction]["components"]["status"])

    def test_service_and_reservation_hold_keep_static_gear(self):
        """Require distinct real lamp phases over the same stopped empty body.

        @param self Current focused source check instance.
        @return None.
        """
        # Both real empty status categories need distinct authored phases in
        # every camera while the same original stopped wheel pixels persist.
        for direction in states.fleet.DIRECTIONS:
            allowed = self.points([self.geometry["status_regions"][direction]])
            presentations = []
            for state in ("service", "blocked"):
                first, second = self.pose("COAL", state, 0, direction), self.pose("COAL", state, 1, direction)
                differences = {(index % 64, index // 64) for index, (a, b) in enumerate(zip(first.tobytes(), second.tobytes())) if a != b}
                self.assertEqual(differences, allowed)
                for frame in range(2):
                    row = self.rows["COAL", state, frame, direction]
                    self.assertIsNone(row["components"]["gear"])
                    self.assertIsNone(row["load_source"])
                    presentations.append(self.pose("COAL", state, frame, direction).tobytes())
            self.assertEqual(len(set(presentations)), 4)

    def test_company_colours_are_confined_to_explicit_painted_fitting(self):
        """Keep reserved livery indices out of steel, cargo and authored lamps.

        @param self Current focused source check instance.
        @return None.
        """
        forbidden = states.fleet.base.FORBIDDEN - set(range(198, 206))
        # Palette ownership applies to every load and status, so a blue cargo
        # or cyan lamp cannot silently become company-remapped material.
        for identity, row in self.rows.items():
            image = self.pose(*identity)
            points = self.points([self.geometry["company_regions"][row["direction"]]])
            remapped = {(index % 64, index // 64) for index, value in enumerate(image.tobytes()) if 198 <= value <= 205}
            self.assertEqual(remapped, points)
            self.assertFalse((set(image.tobytes()) - {0}) & forbidden)
            self.assertEqual(image.getpalette(), states.fleet.base.PALETTE_BYTES)

    def test_declared_empty_aliases_keep_separate_cargo_variants(self):
        """Keep every native label despite shared empty/service/blocked exteriors.

        @param self Current focused source check instance.
        @return None.
        """
        # Shared empty exteriors never merge cargo identities or remove their
        # separate actual native variant slots from the complete source matrix.
        for variant, label in enumerate(self.family["native_cargo_labels"]):
            for state in ("empty", "service", "blocked"):
                for frame in range(2):
                    for direction in states.fleet.DIRECTIONS:
                        row = self.rows[label, state, frame, direction]
                        self.assertEqual(row["tile_variant"], variant)
                        self.assertIsNotNone(row["visual_alias"])
                        self.assertEqual(self.pose(label, state, frame, direction).tobytes(), self.pose("COAL", state, frame, direction).tobytes())

    def test_purchase_lock_preserves_complete_own_model_preview(self):
        """Retain all available wagon pixels above the genuine calendar badge.

        @param self Current focused source check instance.
        @return None.
        """
        atlas = Image.open(io.BytesIO(self.files[ROOT / self.purchase_report["path"]]))
        available, locked = atlas.crop((0, 0, 20, 20)), atlas.crop((20, 0, 40, 20))
        self.assertEqual(available.getbbox(), (1, 7, 19, 13))
        self.assertEqual(available.crop((0, 0, 20, 13)).tobytes(), locked.crop((0, 0, 20, 13)).tobytes())
        self.assertNotEqual(available.tobytes(), locked.tobytes())
        # The availability badge must preserve every opaque own-model preview
        # pixel, including its narrow wheel/chassis silhouette at native scale.
        for before, after in zip(available.tobytes(), locked.tobytes()):
            if before != 0:
                self.assertEqual(before, after)
        lock = self.purchase_report["sprites"][1]["lock_component"]
        self.assertEqual(lock["family"], "control.future-lock")
        self.assertEqual(lock["target"], [12, 13, 7, 7])
        self.assertEqual(states.fleet.base.digest(ROOT / lock["source"]), lock["source_sha256"])

    def test_changed_geometry_or_native_state_owner_is_refused(self):
        """Prevent source-only cues from changing attachment or gameplay ownership.

        @param self Current focused source check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        record["families"]["vehicle.model-05"]["state_component_geometry"]["status_regions"]["E"][0] += 1
        with self.assertRaisesRegex(ValueError, "geometry changed"):
            states.checked_geometry(record, self.public)
        public = copy.deepcopy(self.public)
        row = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-05")
        row["native_state_policy"]["empty"] = "invented local stock count"
        with self.assertRaisesRegex(ValueError, "ownership changed"):
            states.checked_geometry(self.record, public)
        record = copy.deepcopy(self.record)
        record["families"]["vehicle.model-05"]["native_cargo_labels"].pop()
        with self.assertRaisesRegex(ValueError, "label mapping"):
            states.checked_geometry(record, self.public)

    def test_missing_heading_or_status_phase_is_refused_after_isolated_approval(self):
        """Require real originals even when malformed fixture signatures agree.

        @param self Current focused source check instance.
        @return None.
        """
        # Re-signing isolated malformed metadata exercises actual coverage
        # guards beyond approval hashes; original repository inputs stay exact.
        for category, identity in (("gear", "NW"), ("lenses", "service/1")):
            record = copy.deepcopy(self.record)
            family = record["families"]["vehicle.model-05"]
            del family["state_components"][category][identity]
            family["approved_state_components_sha256"] = states.signature(family["state_components"])
            with self.assertRaisesRegex(ValueError, "lacks"):
                states.checked_geometry(record, self.public)

    def test_wrong_heading_rejected_original_and_ancestry_are_refused(self):
        """Keep genuine same-model selected source ancestry before cropping pixels.

        @param self Current focused source check instance.
        @return None.
        """
        selection = self.components["gear"]["N"]
        with self.assertRaisesRegex(ValueError, "wrong source heading"):
            states.original_component(selection, "running_gear", "NE", self.record, {})
        # Selected status, immutable ancestry and own-model identity each guard
        # independently against a plausible substitute component original.
        for mutation in ("status", "sha256", "asset_id"):
            record = copy.deepcopy(self.record)
            candidate = next(row for row in record["candidates"] if row["key"] == selection["key"])
            candidate[mutation] = {"status": "rejected_model_drift", "sha256": "0" * 64, "asset_id": "vehicle.model-06"}[mutation]
            with self.assertRaises(ValueError):
                states.original_component(selection, "running_gear", "N", record, {})

    def test_duplicate_native_wheel_phase_and_contact_extension_are_refused(self):
        """Prevent duplicate padding or wheel animation that changes contact alpha.

        @param self Current focused source check instance.
        @return None.
        """
        body = self.baselines[None, "empty", "E"]
        gears = dict(self.gears)
        gears["E"] = body, self.gears["E"][1]
        with self.assertRaisesRegex(ValueError, "duplicate native frame"):
            states.apply_components(body, "E", "empty", 1, self.geometry, gears, self.lenses)
        geometry = copy.deepcopy(self.geometry)
        geometry["gear_regions"]["E"] = [[0, 39, 1, 1]]
        with self.assertRaisesRegex(ValueError, "alpha or ground contact"):
            states.apply_components(body, "E", "empty", 1, geometry, self.gears, self.lenses)

    def test_purchase_cannot_borrow_another_lock_or_canvas(self):
        """Refuse invented availability art and world-size UI substitutions.

        @param self Current focused source check instance.
        @return None.
        """
        components = copy.deepcopy(self.components)
        components["purchase"]["lock_family"] = "private.calendar"
        with self.assertRaisesRegex(ValueError, "purchase geometry"):
            states.purchase_source(self.baselines[None, "empty", "E"], components, self.public)
        public = copy.deepcopy(self.public)
        next(row for row in public["assets"] if row["asset_id"] == "purchase.model-05")["native_size"] = [64, 48]
        with self.assertRaisesRegex(ValueError, "purchase geometry"):
            states.purchase_source(self.baselines[None, "empty", "E"], self.components, public)

    def test_whole_component_clearance_and_opaque_lens_interior_are_required(self):
        """Reject clipped source borders and transparent status sampling regions.

        @param self Current focused source check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        selection = copy.deepcopy(self.components["gear"]["N"])
        candidate = next(row for row in record["candidates"] if row["key"] == selection["key"])
        selection["source_window"] = [288, 151, 479, 444]
        image = Image.open(ROOT / candidate["path"]).convert("RGBA").crop(selection["source_window"])
        selection["whole_subject_bounds_in_window"] = list(image.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox())
        candidate["component_views"]["N"] = {key: value for key, value in selection.items() if key != "key"}
        with self.assertRaisesRegex(ValueError, "clearance"):
            states.original_component(selection, "running_gear", "N", record, {})
        record, components = copy.deepcopy(self.record), copy.deepcopy(self.components)
        selection = components["lenses"]["service/0"]
        selection["interior_crop"] = [40, 40, 48, 48]
        candidate = next(row for row in record["candidates"] if row["key"] == selection["key"])
        candidate["component_views"]["service/0"] = {key: value for key, value in selection.items() if key != "key"}
        with self.assertRaisesRegex(ValueError, "transparent padding"):
            states.state_sources(record, self.geometry, components)

    def test_duplicate_original_lens_phase_cannot_pad_native_coverage(self):
        """Require four distinct actual glass phases after native quantization.

        @param self Current focused source check instance.
        @return None.
        """
        record, components = copy.deepcopy(self.record), copy.deepcopy(self.components)
        components["lenses"]["service/1"] = copy.deepcopy(components["lenses"]["service/0"])
        selection = components["lenses"]["service/1"]
        candidate = next(row for row in record["candidates"] if row["key"] == selection["key"])
        candidate["component_views"]["service/1"] = {key: value for key, value in selection.items() if key != "key"}
        with self.assertRaisesRegex(ValueError, "collapse at native palette"):
            states.state_sources(record, self.geometry, components)

    def test_stale_source_artifact_cannot_publish_shared_metadata(self):
        """Refuse stale source proof before any shared manifest mutation occurs.

        @param self Current focused source check instance.
        @return None.
        """
        files = dict(self.files)
        path = ROOT / self.purchase_report["path"]
        files[path] += b"stale proof"
        previous = states.fleet.base.PUBLIC.read_bytes()
        with self.assertRaisesRegex(ValueError, "requires exact artifacts"):
            states.publish_source(self.record, copy.deepcopy(self.public), self.report, files)
        self.assertEqual(states.fleet.base.PUBLIC.read_bytes(), previous)


if __name__ == "__main__":
    unittest.main()
