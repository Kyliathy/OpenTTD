"""Verify complete model08 originals, closed load cues and native road geometry.

@file sealed_road_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group22
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
These checks activate no art, cargo transport, calendar exception or road route.
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
import convert_sealed_road_states as carrier
from tools.ground_fleet_contract import native_cargo_forms, native_cargo_labels, variant_contract


class SealedRoadSourceChecks(unittest.TestCase):
    """Protect own-road identity, genuine views and original material-only reuse.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks22.1-22.3
    """

    @classmethod
    def setUpClass(cls):
        """Regenerate complete selected sources once for this focused group.

        @param cls Check class retaining native base/state/purchase artifacts.
        @return None.
        """
        cls.record = json.loads(carrier.fleet.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(carrier.fleet.base.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.files = carrier.build(cls.record, cls.public)
        cls.family = cls.record["families"][carrier.ASSET]
        cls.geometry, cls.components, cls.donor_geometry, cls.donor_components = carrier.family_spec(cls.record, cls.public)
        cls.gears, cls.lenses = carrier.road.component_sources(cls.record, cls.geometry, cls.components, cls.donor_geometry, cls.donor_components)
        cls.meter = carrier.sealed.load_meter_source(cls.components["meter_source"])
        cls.world, cls.purchase = cls.report["atlases"]
        cls.atlas = Image.open(io.BytesIO(cls.files[ROOT / cls.world["path"]]))
        cls.rows = {(row["state"], row["frame"], row["direction"]): row for row in cls.world["sprites"]}
        cls.bodies = {}
        # Each later source comparison begins with the actual own-model heading,
        # never a copied rail shell, open truck or stale cache image.
        for direction in carrier.fleet.DIRECTIONS:
            cls.bodies[direction], _evidence = carrier.fleet.native_view(cls.family["selections"]["empty/" + direction], direction, cls.record, cls.public)

    def pose(self, state, frame, direction):
        """Read one exact own-road native body state from the regenerated atlas.

        @param self Current focused source check instance.
        @param state Actual load or empty service/traffic presentation.
        @param frame Native owner-selected zero/one visual phase.
        @param direction Genuine source heading.
        @return Indexed native64x48 own08 source pose.
        """
        x, y, width, height = self.rows[state, frame, direction]["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def region_points(self, regions):
        """Expand exact original attachment rectangles into native coordinates.

        @param self Current focused source check instance.
        @param regions Inspected own08 rectangles, including empty hidden masks.
        @return Set of native target points, never generated artwork.
        """
        return {(px, py) for x, y, width, height in regions for py in range(y, y + height) for px in range(x, x + width)}

    def test_eight_genuine_views_and_complete_eighty_plus_two_exact_artifacts(self):
        """Require the complete actual closed exterior without missing-frame padding.

        @param self Current focused source check instance.
        @return None.
        """
        expected = {(state, frame, direction) for state in carrier.ordinary.STATES for frame in range(2) for direction in carrier.fleet.DIRECTIONS}
        self.assertEqual(set(self.rows), expected)
        self.assertEqual(self.report["body_pose_count"], 80)
        self.assertEqual(self.report["purchase_pose_count"], 2)
        self.assertTrue(self.report["source_coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(len({body.tobytes() for body in self.bodies.values()}), 8)
        self.assertEqual(len(self.files), 12)
        for path, encoded in self.files.items():
            self.assertEqual(path.read_bytes(), encoded, path.name)

    def test_actual_sealed_cargo_date_capacity_and_road_state_contract(self):
        """Keep protected cargo meanings and1980 admission with the native model.

        @param self Current focused source check instance.
        @return None.
        """
        self.assertEqual(native_cargo_labels(8), ["URAN", "RARE"])
        self.assertEqual(native_cargo_forms(8), {"URAN": "sealed_drums", "RARE": "covered_granules"})
        self.assertEqual(self.family["capacity"], 24)
        self.assertEqual(self.family["introduction_year"], 1980)
        self.assertEqual((self.family["physical_native_length"], self.family["shorten_factor"]), (8, 0))
        self.assertEqual(self.family["commissioning"], "RoadDepot")
        spec = carrier.fleet.public_family(carrier.ASSET, self.public)
        for key, value in variant_contract(8).items():
            self.assertEqual(spec[key], value)
        self.assertEqual(spec["tile_variants"], 1)
        policy = spec["native_state_policy"]
        self.assertIn("RoadVehicle::cargo", policy["cargo_owner"])
        self.assertIn("model08", policy["cargo_label_owner"])
        self.assertIn("IsRoadDepotTile", policy["service_empty"])
        self.assertIn("blocked_ctr>0", policy["blocked_empty"])
        self.assertNotIn("Train", json.dumps(policy))
        for row in self.world["sprites"]:
            self.assertEqual(row["cargo_labels"], ["URAN", "RARE"])

    def test_source_clearance_and_excluded_roof_cells_are_truthful(self):
        """Retain corrected end roofs and refuse unselected continuity repeats.

        @param self Current focused source check instance.
        @return None.
        """
        report = json.loads(self.files[carrier.fleet.OUTPUT / "vehicle-model-08-base-source-manifest.json"])
        self.assertTrue(report["base_geometry_complete"])
        self.assertEqual(report["body_source_pose_count"], 8)
        # Requested frames alone do not prove safe extraction. Every genuine
        # source has measured alpha16 clearance and the same native contact.
        for row in report["sprites"]:
            self.assertGreaterEqual(row["transparent_clearance"], 43)
            self.assertEqual(row["native_opaque_bounds"][3], 40)
            self.assertEqual(row["anchor"], [-32, -32])
        for key, direction in (("sealed-road-empty-n-ne-se-s-v1", "N"), ("sealed-road-empty-n-ne-se-s-v1", "S"),
                               ("sealed-road-empty-n-ne-se-s-v2", "NE"), ("sealed-road-empty-sw-w-nw-e-v1", "E")):
            # Wrong-joint end cells and repeated continuity views remain outside
            # the selected source matrix even when their crops would be safe.
            with self.assertRaisesRegex(ValueError, "no measured requested heading"):
                carrier.fleet.native_view(key, direction, self.record, self.public)

    def test_foreign_rail_and_open_road_silhouettes_are_refused(self):
        """Protect all selected model identities through the shared camera tool.

        @param self Current focused source check instance.
        @return None.
        """
        # Both transfer directions matter: closed-road selection must not
        # corrupt completed ordinary/sealed wagon or open-road source families.
        for target, donor in ((carrier.ASSET, "vehicle.model-05"), (carrier.ASSET, "vehicle.model-06"),
                              (carrier.ASSET, "vehicle.model-07"), ("vehicle.model-07", carrier.ASSET)):
            record = copy.deepcopy(self.record)
            record["families"][target]["selections"]["empty/E"] = record["families"][donor]["selections"]["empty/E"]
            with self.assertRaisesRegex(ValueError, "another native model"):
                carrier.fleet.build_base(target, record, self.public)

    def test_fixed_closed_body_alpha_tyres_and_contact_survive_all_states(self):
        """Confine every change to actual hub, door-plaque or roof-module pixels.

        @param self Current focused source check instance.
        @return None.
        """
        # Test all80 actual poses; a single attractive preview can conceal an
        # opened hatch, rear-door cut or one camera's misplaced road hub cue.
        for identity, row in self.rows.items():
            state, frame, direction = identity
            image, body = self.pose(*identity), self.bodies[direction]
            allowed = self.region_points([self.geometry["company_regions"][direction]])
            if state in ("partial", "full"):
                allowed.update(tuple(point) for point in self.geometry["sealed_meter_points"][direction])
            if state in ("empty", "partial", "full") and frame == 1:
                # Only movement-selected phases may change visible road hubs.
                allowed.update(self.region_points(self.geometry["gear_regions"][direction]))
            if state in ("service", "blocked"):
                allowed.update(self.region_points([self.geometry["status_regions"][direction]]))
            self.assertEqual(image.getbbox(), body.getbbox())
            self.assertEqual(image.getbbox()[3], 40)
            self.assertEqual(bytes(bool(p) for p in image.get_flattened_data()), bytes(bool(p) for p in body.get_flattened_data()))
            for index, (before, after) in enumerate(zip(body.get_flattened_data(), image.get_flattened_data())):
                if (index % 64, index // 64) not in allowed:
                    self.assertEqual(before, after)

    def test_partial_full_add_only_genuine_roof_glass_and_keep_enclosure_closed(self):
        """Distinguish packet load categories without exposing protected contents.

        @param self Current focused source check instance.
        @return None.
        """
        index, evidence = self.meter
        self.assertEqual(index, 192)
        self.assertEqual(evidence["frame"], 1)
        self.assertEqual(carrier.fleet.base.digest(ROOT / evidence["source"]), evidence["source_sha256"])
        # Full adds only the second actual pane; it preserves the partial pane,
        # shell and packet custody rather than manufacturing visible granules.
        for direction in carrier.fleet.DIRECTIONS:
            first, second = [tuple(point) for point in self.geometry["sealed_meter_points"][direction]]
            partial, full = self.pose("partial", 0, direction), self.pose("full", 0, direction)
            empty = self.pose("empty", 0, direction)
            self.assertEqual(partial.getpixel(first), index)
            self.assertEqual(full.getpixel(first), index)
            self.assertEqual(full.getpixel(second), index)
            self.assertEqual(partial.getpixel(second), empty.getpixel(second))
            self.assertNotEqual(partial.tobytes(), empty.tobytes())
            self.assertNotEqual(full.tobytes(), partial.tobytes())
            for frame in range(2):
                for state in ("partial", "full"):
                    self.assertIsNone(self.rows[state, frame, direction]["components"]["status"])

    def test_three_visible_road_hubs_and_end_on_occlusion_remain_explicit(self):
        """Retain real road-wheel interiors and truthfully alias hidden hub faces.

        @param self Current focused source check instance.
        @return None.
        """
        # Every packet-backed category keeps its glass during road movement;
        # physical end-view occlusion never authorizes a missing side source.
        for state in ("empty", "partial", "full"):
            for direction in carrier.fleet.DIRECTIONS:
                first, second = self.pose(state, 0, direction), self.pose(state, 1, direction)
                if direction in ("N", "S"):
                    self.assertEqual(first.tobytes(), second.tobytes())
                    self.assertIn("occlusion", self.rows[state, 1, direction]["components"]["gear"])
                    self.assertEqual(self.geometry["gear_regions"][direction], [])
                else:
                    self.assertNotEqual(first.tobytes(), second.tobytes())
                    points = self.region_points(self.geometry["gear_regions"][direction])
                    self.assertEqual(len(points), 3)
                    donor, evidence = self.gears[direction]
                    self.assertTrue(evidence["material_only"])
                    for point in points:
                        self.assertEqual(second.getpixel(point), donor.getpixel(point))

    def test_empty_service_hold_use_four_original_glasses_and_static_tyres(self):
        """Keep actual empty road status separate from motion and loaded packets.

        @param self Current focused source check instance.
        @return None.
        """
        self.assertEqual(len({sample[0] for sample in self.lenses.values()}), 4)
        for direction in carrier.fleet.DIRECTIONS:
            # Both actual service/traffic phases keep the stopped tyres and
            # second original roof pane; only the first glass communicates status.
            second = tuple(self.geometry["sealed_meter_points"][direction][1])
            for state in ("service", "blocked"):
                for frame in range(2):
                    image, row = self.pose(state, frame, direction), self.rows[state, frame, direction]
                    self.assertIsNone(row["components"]["gear"])
                    x, y, _width, _height = self.geometry["status_regions"][direction]
                    self.assertEqual(image.getpixel((x, y)), self.lenses[state + "/" + str(frame)][0])
                    self.assertEqual(image.getpixel(second), self.bodies[direction].getpixel(second))
                    for point in self.region_points(self.geometry["gear_regions"][direction]):
                        self.assertEqual(image.getpixel(point), self.bodies[direction].getpixel(point))

    def test_palette_and_explicit_company_mask_do_not_tint_the_shell(self):
        """Restrict company remap to actual own08 cab-door plaque footprints.

        @param self Current focused source check instance.
        @return None.
        """
        self.assertEqual(self.atlas.getpalette(), carrier.fleet.base.PALETTE_BYTES)
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertFalse(set(self.atlas.get_flattened_data()) & (carrier.fleet.base.FORBIDDEN - {0} - set(range(198, 206))))
        for identity in self.rows:
            direction = identity[2]
            image = self.pose(*identity)
            expected = self.region_points([self.geometry["company_regions"][direction]])
            actual = {(i % 64, i // 64) for i, p in enumerate(image.get_flattened_data()) if 198 <= p <= 205}
            self.assertEqual(actual, expected)

    def test_own_purchase_and_original_lock_preserve_every_available_pixel(self):
        """Keep the complete own08 preview under its separate genuine UI badge.

        @param self Current focused source check instance.
        @return None.
        """
        image = Image.open(io.BytesIO(self.files[ROOT / self.purchase["path"]]))
        available, locked = image.crop((0, 0, 20, 20)), image.crop((20, 0, 40, 20))
        original = self.bodies["E"].convert("RGBA")
        expected = carrier.fleet.base.quantize(carrier.fleet.base.fit(original.crop(original.getchannel("A").getbbox()), (20, 20), (18, 6)), {})
        self.assertEqual(available.tobytes(), expected.tobytes())
        self.assertNotEqual(available.tobytes(), locked.tobytes())
        for before, after in zip(available.get_flattened_data(), locked.get_flattened_data()):
            if before:
                self.assertEqual(before, after)
        for row in self.purchase["sprites"]:
            self.assertEqual(row["body_source"]["source"], self.family["selections"]["empty/E"])
        lock = self.purchase["sprites"][1]["lock_component"]
        self.assertEqual(lock["family"], "control.future-lock")
        self.assertEqual(carrier.fleet.base.digest(ROOT / lock["source"]), lock["source_sha256"])

    def test_changed_original_hash_or_native_anchor_is_refused(self):
        """Refuse source ancestry and placement drift before converting state art.

        @param self Current focused source check instance.
        @return None.
        """
        for mutation in ("hash", "anchor"):
            record, public = copy.deepcopy(self.record), copy.deepcopy(self.public)
            if mutation == "hash":
                next(row for row in record["candidates"] if row["key"] == "sealed-road-empty-e-v1")["sha256"] = "0" * 64
            else:
                next(row for row in public["assets"] if row["asset_id"] == carrier.ASSET)["anchor"] = [0, 0]
            with self.assertRaises(ValueError):
                carrier.fleet.build_base(carrier.ASSET, record, public)

    def test_foreign_owner_and_art_driven_state_contract_are_refused(self):
        """Reject copied vehicle owners and invented traffic/load presentation facts.

        @param self Current focused source check instance.
        @return None.
        """
        # Signing private metadata cannot change the native model/cargo owner
        # or make another vehicle's component family a shared original material.
        for mutation in ("owner", "state", "cargo"):
            record = copy.deepcopy(self.record)
            family = record["families"][carrier.ASSET]
            if mutation == "owner":
                family["state_components"]["gear_source"]["source_family"] = "vehicle.model-07"
                family["approved_state_components_sha256"] = carrier.ordinary.signature(family["state_components"])
            elif mutation == "state":
                family["state_component_geometry"]["state_owner"]["blocked_empty"] = "art timer or rail reservation"
            else:
                family["native_cargo_labels"] = ["COAL"]
            with self.assertRaises(ValueError):
                carrier.family_spec(record, self.public)

    def test_missing_heading_false_occlusion_and_duplicate_meter_are_refused(self):
        """Keep all real attachments and two independent physical roof panes.

        @param self Current focused source check instance.
        @return None.
        """
        for mutation in ("heading", "occlusion", "meter"):
            record = copy.deepcopy(self.record)
            family = record["families"][carrier.ASSET]
            geometry = family["state_component_geometry"]
            if mutation == "heading":
                del geometry["status_regions"]["NW"]
            elif mutation == "occlusion":
                geometry["gear_visibility"]["E"] = "hidden_end_elevation"
            else:
                geometry["sealed_meter_points"]["E"][1] = geometry["sealed_meter_points"]["E"][0]
            family["approved_state_component_geometry_sha256"] = carrier.ordinary.signature(geometry)
            family["state_components"]["geometry_sha256"] = carrier.ordinary.signature(geometry)
            family["approved_state_components_sha256"] = carrier.ordinary.signature(family["state_components"])
            with self.assertRaises(ValueError):
                carrier.family_spec(record, self.public)

    def test_foreign_meter_frame_and_roof_as_hub_donor_are_refused(self):
        """Require known original amber glass and approved neutral wheel interiors.

        @param self Current focused source check instance.
        @return None.
        """
        meter = copy.deepcopy(self.components["meter_source"])
        meter["frame"] = 4
        with self.assertRaisesRegex(ValueError, "meter material"):
            carrier.sealed.load_meter_source(meter)
        components = copy.deepcopy(self.components)
        components["gear_source"]["source_points"]["E"][0] = [31, 20]
        with self.assertRaisesRegex(ValueError, "leaves approved region"):
            carrier.road.component_sources(self.record, self.geometry, components, self.donor_geometry, self.donor_components)

    def test_stale_artifact_cannot_publish_shared_source_coverage(self):
        """Refuse changed proof pixels before any canonical source metadata write.

        @param self Current focused source check instance.
        @return None.
        """
        files = dict(self.files)
        files[ROOT / self.world["path"]] = b"stale proof"
        with mock.patch.object(Path, "write_text") as write:
            with self.assertRaisesRegex(ValueError, "exact artifacts"):
                carrier.publish_source(self.record, self.public, self.report, files)
            write.assert_not_called()

    def test_metadata_substitution_cannot_publish_another_vehicle(self):
        """Bind complete counts to verified own08 body and purchase consumers.

        @param self Current focused source check instance.
        @return None.
        """
        report = copy.deepcopy(self.report)
        report["atlases"][0]["asset_id"] = "vehicle.model-06"
        with mock.patch.object(Path, "write_text") as write:
            with self.assertRaisesRegex(ValueError, "metadata differs"):
                carrier.publish_source(self.record, self.public, report, self.files)
            write.assert_not_called()


if __name__ == "__main__":
    unittest.main()
