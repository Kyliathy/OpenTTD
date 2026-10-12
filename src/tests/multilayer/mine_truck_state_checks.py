"""Verify complete original road truck sources without native activation.

@file mine_truck_state_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group21C
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Native cargo packets, road traffic and calendar admission retain their owners.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest
from unittest import mock

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_mine_truck_states as truck


class MineTruckStateChecks(unittest.TestCase):
    """Protect actual road parts, physical occlusion and complete public sources.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks21C.1-21C.3
    """

    @classmethod
    def setUpClass(cls):
        """Regenerate complete originals once for the focused owner burst.

        @param cls Check class retaining exact body/cargo/state/purchase artifacts.
        @return None.
        """
        cls.record = json.loads(truck.fleet.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(truck.fleet.base.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.files = truck.build(cls.record, cls.public)
        cls.family = cls.record["families"][truck.bulk.ASSET]
        cls.geometry, cls.components, cls.donor_geometry, cls.donor_components = truck.family_spec(cls.record, cls.public)
        cls.gears, cls.lenses = truck.component_sources(cls.record, cls.geometry, cls.components, cls.donor_geometry, cls.donor_components)
        cls.world, cls.purchase = cls.report["atlases"]
        cls.atlas = Image.open(io.BytesIO(cls.files[ROOT / cls.world["path"]]))
        cls.rows = {(row["cargo_label"], row["state"], row["frame"], row["direction"]): row for row in cls.world["sprites"]}
        cls.bodies = {}
        for direction in truck.fleet.DIRECTIONS:
            cls.bodies[direction], _evidence = truck.fleet.native_view(cls.family["selections"]["empty/" + direction], direction, cls.record, cls.public)

    def pose(self, label, state, frame, direction):
        """Read one exact world pose without using an implicit material alias.

        @param self Current check instance holding the regenerated native atlas.
        @param label Actual native cargo meaning owning its separate variant slot.
        @param state Actual load or empty service/traffic presentation.
        @param frame Explicit zero/one visual phase.
        @param direction Genuine native camera heading.
        @return Indexed own07 native64x48 pose.
        """
        x, y, width, height = self.rows[label, state, frame, direction]["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def region_points(self, regions):
        """Expand inspected attachment rectangles into native pixel coordinates.

        @param self Current focused check instance.
        @param regions Explicit own-road rectangles, including hidden empty masks.
        @return Set of native target coordinates; never synthetic artwork.
        """
        return {(px, py) for x, y, width, height in regions for py in range(y, y + height) for px in range(x, x + width)}

    def test_complete_eight_hundred_body_two_purchase_and_exact_artifacts(self):
        """Require every actual cargo/state/frame/view and both own purchase states.

        @param self Current focused check instance.
        @return None.
        """
        expected = {(label, state, frame, direction) for label in self.family["native_cargo_labels"]
                    for state in truck.ordinary.STATES for frame in range(2) for direction in truck.fleet.DIRECTIONS}
        self.assertEqual(set(self.rows), expected)
        self.assertEqual(len(self.rows), 800)
        self.assertEqual(self.report["body_pose_count"], 800)
        self.assertEqual(self.report["purchase_pose_count"], 2)
        self.assertTrue(self.report["source_coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(len(self.files), 26)
        for path, encoded in self.files.items():
            self.assertEqual(path.read_bytes(), encoded, path.name)

    def test_all_fixed_body_cargo_alpha_and_contacts_survive_component_placement(self):
        """Confine every state change to declared road parts or actual cargo floors.

        @param self Current focused check instance.
        @return None.
        """
        # Every actual cargo/state/frame/camera combination is checked. A single
        # preview can hide a load-specific contact cut or an accidental cab tint.
        for identity, row in self.rows.items():
            label, state, frame, direction = identity
            image, body = self.pose(*identity), self.bodies[direction]
            allowed = self.region_points([self.geometry["company_regions"][direction]])
            if state in ("partial", "full"):
                mask = truck.bulk.cargo_mask(direction, state)
                allowed.update((x, y) for y in range(48) for x in range(64) if mask.getpixel((x, y)))
                self.assertIsNotNone(row["load_source"])
            if state in ("empty", "partial", "full") and frame == 1:
                # Only genuine rolling presentations may touch visible hubs.
                allowed.update(self.region_points(self.geometry["gear_regions"][direction]))
            if state in ("service", "blocked"):
                allowed.update(self.region_points([self.geometry["status_regions"][direction]]))
            self.assertEqual(image.getbbox(), body.getbbox())
            self.assertEqual(image.getbbox()[3], 40)
            self.assertEqual(bytes(bool(p) for p in image.get_flattened_data()), bytes(bool(p) for p in body.get_flattened_data()))
            for index, (before, after) in enumerate(zip(body.get_flattened_data(), image.get_flattened_data())):
                if (index % 64, index // 64) not in allowed:
                    self.assertEqual(before, after)

    def test_six_visible_hub_phases_and_two_explicit_end_occlusions(self):
        """Animate only original road hubs and truthfully alias hidden end faces.

        @param self Current focused check instance.
        @return None.
        """
        # Each actual load remains intact during motion; end elevations alias
        # their observed hidden hub faces rather than invent side-facing wheels.
        for label in self.family["native_cargo_labels"]:
            for state in ("empty", "partial", "full"):
                for direction in truck.fleet.DIRECTIONS:
                    first, second = self.pose(label, state, 0, direction), self.pose(label, state, 1, direction)
                    row = self.rows[label, state, 1, direction]
                    if direction in ("N", "S"):
                        self.assertEqual(first.tobytes(), second.tobytes())
                        self.assertIn("occlusion", row["components"]["gear"])
                        self.assertEqual(self.geometry["gear_regions"][direction], [])
                    else:
                        self.assertNotEqual(first.tobytes(), second.tobytes())
                        donor, _evidence = self.gears[direction]
                        for point in self.region_points(self.geometry["gear_regions"][direction]):
                            self.assertEqual(second.getpixel(point), donor.getpixel(point))

    def test_empty_status_glass_has_four_phases_and_static_tyres(self):
        """Keep real empty service/traffic cues separate from moving cargo states.

        @param self Current focused check instance.
        @return None.
        """
        self.assertEqual(len({sample[0] for sample in self.lenses.values()}), 4)
        # Every roof fitting is genuinely visible. Both status phases preserve
        # the stopped tyres and cannot replace an admitted loaded presentation.
        for direction in truck.fleet.DIRECTIONS:
            values = []
            for state in ("service", "blocked"):
                for frame in range(2):
                    row = self.rows["COAL", state, frame, direction]
                    self.assertIsNone(row["components"]["gear"])
                    image = self.pose("COAL", state, frame, direction)
                    expected = self.lenses[state + "/" + str(frame)][0]
                    x, y, _width, _height = self.geometry["status_regions"][direction]
                    self.assertEqual(image.getpixel((x, y)), expected)
                    values.append(expected)
                    for point in self.region_points(self.geometry["gear_regions"][direction]):
                        self.assertEqual(image.getpixel(point), self.bodies[direction].getpixel(point))
            self.assertEqual(len(set(values)), 4)
        for row in self.world["sprites"]:
            if row["state"] in ("partial", "full"):
                self.assertIsNone(row["components"]["status"])

    def test_company_colours_exist_only_in_explicit_original_door_footprints(self):
        """Protect cargo, windows, bed, steel and tyres from company recoloring.

        @param self Current focused check instance.
        @return None.
        """
        self.assertEqual(self.atlas.getpalette(), truck.fleet.base.PALETTE_BYTES)
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertFalse(set(self.atlas.get_flattened_data()) & (truck.fleet.base.FORBIDDEN - {0} - set(range(198, 206))))
        # Original blue plaques may blend at tiny native scale. Their explicit
        # physical footprint, not an unrestricted color matcher, owns the mask.
        for identity, row in self.rows.items():
            direction = identity[3]
            image = self.pose(*identity)
            expected = self.region_points([self.geometry["company_regions"][direction]])
            actual = {(index % 64, index // 64) for index, value in enumerate(image.get_flattened_data()) if 198 <= value <= 205}
            self.assertEqual(actual, expected)
            self.assertEqual({tuple(value["target"]) for value in row["components"]["company_mask"]}, expected)
        self.assertEqual(self.region_points([self.geometry["company_regions"]["N"]]), set())
        self.assertEqual(self.region_points([self.geometry["company_regions"]["S"]]), set())

    def test_own_purchase_preserves_whole_available_preview_and_genuine_lock(self):
        """Use only own07 E art and retain every available pixel under the lock.

        @param self Current focused check instance.
        @return None.
        """
        image = Image.open(io.BytesIO(self.files[ROOT / self.purchase["path"]]))
        available, locked = image.crop((0, 0, 20, 20)), image.crop((20, 0, 40, 20))
        original = self.bodies["E"].convert("RGBA")
        expected = truck.fleet.base.quantize(truck.fleet.base.fit(original.crop(original.getchannel("A").getbbox()), (20, 20), (18, 6)), {})
        self.assertEqual(available.tobytes(), expected.tobytes())
        self.assertNotEqual(available.tobytes(), locked.tobytes())
        # The separate badge sits below the complete preview; a locked state
        # must never erase a tyre, cab or any other available model pixel.
        for before, after in zip(available.get_flattened_data(), locked.get_flattened_data()):
            if before:
                self.assertEqual(before, after)
        for row in self.purchase["sprites"]:
            self.assertEqual(row["body_source"]["source"], self.family["selections"]["empty/E"])
        lock = self.purchase["sprites"][1]["lock_component"]
        self.assertEqual(lock["family"], "control.future-lock")
        self.assertEqual(truck.fleet.base.digest(ROOT / lock["source"]), lock["source_sha256"])

    def test_foreign_owner_and_shortened_native_matrix_are_refused(self):
        """Keep cargo/state ownership with the real road model and original donor.

        @param self Current focused check instance.
        @return None.
        """
        # Plausible rail facts and narrowed private cargo tables must fail even
        # when their component metadata is independently signed in the fixture.
        for mutation in ("owner", "cargo", "state"):
            record = copy.deepcopy(self.record)
            family = record["families"][truck.bulk.ASSET]
            if mutation == "owner":
                family["state_components"]["gear_source"]["source_family"] = "vehicle.model-06"
                family["approved_state_components_sha256"] = truck.ordinary.signature(family["state_components"])
            elif mutation == "cargo":
                family["native_cargo_labels"].pop()
            else:
                family["state_component_geometry"]["state_owner"]["blocked_empty"] = "Train reservation"
                family["approved_state_component_geometry_sha256"] = truck.ordinary.signature(family["state_component_geometry"])
            with self.assertRaises(ValueError):
                truck.family_spec(record, self.public)

    def test_missing_heading_or_false_hidden_hub_cannot_pad_coverage(self):
        """Require physical occlusion only in the genuine end-on road cameras.

        @param self Current focused check instance.
        @return None.
        """
        for mutation in ("heading", "visibility"):
            record = copy.deepcopy(self.record)
            family = record["families"][truck.bulk.ASSET]
            geometry = family["state_component_geometry"]
            if mutation == "heading":
                del geometry["status_regions"]["NW"]
            else:
                geometry["gear_visibility"]["E"] = "hidden_end_elevation"
            family["approved_state_component_geometry_sha256"] = truck.ordinary.signature(geometry)
            family["state_components"]["geometry_sha256"] = truck.ordinary.signature(geometry)
            family["approved_state_components_sha256"] = truck.ordinary.signature(family["state_components"])
            with self.assertRaises(ValueError):
                truck.family_spec(record, self.public)

    def test_roof_pixel_cannot_impersonate_original_hub_material(self):
        """Reject a plausible donor outside its approved original axle interior.

        @param self Current focused check instance.
        @return None.
        """
        components = copy.deepcopy(self.components)
        components["gear_source"]["source_points"]["E"][0] = [31, 20]
        with self.assertRaisesRegex(ValueError, "leaves approved region"):
            truck.component_sources(self.record, self.geometry, components, self.donor_geometry, self.donor_components)

    def test_changed_geometry_and_foreign_purchase_lock_are_refused(self):
        """Keep physical attachments and the genuine shared calendar badge exact.

        @param self Current focused check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        record["families"][truck.bulk.ASSET]["state_component_geometry"]["gear_regions"]["E"][0][1] += 1
        with self.assertRaises(ValueError):
            truck.family_spec(record, self.public)
        components = copy.deepcopy(self.components)
        components["purchase"]["lock_family"] = "control.portal"
        with self.assertRaisesRegex(ValueError, "lock changed"):
            truck.ordinary.purchase_source(self.bodies["E"], components, self.public, "purchase.model-07")

    def test_stale_artifact_cannot_publish_shared_source(self):
        """Refuse changed proof pixels before any public manifest write.

        @param self Current focused check instance.
        @return None.
        """
        files = dict(self.files)
        files[ROOT / self.world["path"]] = b"stale proof"
        with mock.patch.object(Path, "write_text") as write:
            with self.assertRaisesRegex(ValueError, "exact artifacts"):
                truck.publish_source(self.record, self.public, self.report, files)
            write.assert_not_called()

    def test_metadata_substitution_cannot_publish_another_model(self):
        """Bind complete counts to the exact verified own-road consumer metadata.

        @param self Current focused check instance.
        @return None.
        """
        report = copy.deepcopy(self.report)
        report["atlases"][0]["asset_id"] = "vehicle.model-05"
        with mock.patch.object(Path, "write_text") as write:
            with self.assertRaisesRegex(ValueError, "metadata differs"):
                truck.publish_source(self.record, self.public, report, self.files)
            write.assert_not_called()


if __name__ == "__main__":
    unittest.main()
