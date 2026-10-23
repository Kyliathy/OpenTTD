"""Verify contained wagon refits without claiming missing animation or runtime.

@file ground_fleet_bulk_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20C
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Published native pixels, complete public coverage and original ancestry are
checked independently; actual packet simulation remains with the native train.
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
import convert_ground_fleet_art as fleet


class GroundFleetBulkChecks(unittest.TestCase):
    """Check genuine material sources and source-only load geometry for model05.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks20C.1-20C.5
    """

    @classmethod
    def setUpClass(cls):
        """Regenerate the focused source slice once for the entire bounded burst.

        @param cls Focused check class holding immutable expected source artifacts.
        @return None.
        """
        cls.record = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.files = fleet.build_bulk(document=cls.record, public=cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[ROOT / cls.report["path"]]))
        base_report = json.loads(cls.files[fleet.OUTPUT / "vehicle-model-05-base-source-manifest.json"])
        base_atlas = Image.open(io.BytesIO(cls.files[ROOT / base_report["path"]]))
        cls.bodies = {}
        # Preserve the independently regenerated original view bytes as the
        # reference for all cargo variants; load output cannot define its own body.
        for pose in base_report["sprites"]:
            x, y, width, height = pose["box"]
            cls.bodies[pose["direction"]] = base_atlas.crop((x, y, x + width, y + height))

    def pose_bytes(self, row):
        """Read one actual decoded source load pose for independent comparisons.

        @param self Current focused check instance.
        @param row Explicit selected sprite rectangle in the published native atlas.
        @return Exact indexed64x48 pixels for that physical heading and fill level.
        """
        x, y, width, height = row["box"]
        return self.atlas.crop((x, y, x + width, y + height)).tobytes()

    def test_exact_artifacts_and_honest_remaining_matrix(self):
        """Require genuine coverage while keeping all missing sources explicit.

        @param self Current focused check instance.
        @return None.
        """
        self.assertEqual(self.report["bulk_frame0_pose_count"], 128)
        self.assertEqual(self.report["empty_frame0_alias_count"], 136)
        self.assertEqual(self.report["staged_body_matrix_pose_count"], 264)
        self.assertEqual(self.report["required_body_matrix_pose_count"], 1360)
        self.assertEqual(self.report["missing_pose_count"], 1096)
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertTrue(all(row["frame"] == 0 for row in self.report["sprites"] + self.report["empty_aliases"]))
        # Reproducible material contacts and source manifests must describe the
        # same bytes; a source-only report cannot stand in for an absent atlas.
        for path, encoded in self.files.items():
            self.assertEqual(path.read_bytes(), encoded, path.name)

    def test_loaded_pixels_preserve_body_alpha_contacts_and_front_rims(self):
        """Keep actual chassis pixels outside the public load region unchanged.

        @param self Current focused check instance.
        @return None.
        """
        spec = next(row for row in self.public["assets"] if row["asset_id"] == "vehicle.model-05")
        allowed, rims = {}, {}
        # Construct independent masks from the shared public contract, rather
        # than asking the composition routine what pixels it happened to change.
        for direction in fleet.DIRECTIONS:
            mask = Image.new("L", (64, 48), 0)
            ImageDraw.Draw(mask).polygon([tuple(point) for point in spec["cargo_geometry"]["bulk_full_polygons"][direction]], fill=1)
            allowed[direction] = mask.tobytes()
            rim = Image.new("L", (64, 48), 0)
            draw = ImageDraw.Draw(rim)
            # Original near steel must occlude every load; its original colors
            # and alpha remain exact even where it borders a full hold.
            for line in spec["cargo_geometry"]["foreground_rim_lines"][direction]:
                draw.line([tuple(point) for point in line], fill=1, width=1)
            rims[direction] = rim.tobytes()
        for row in self.report["sprites"]:
            direction = row["direction"]
            before, after = self.bodies[direction].tobytes(), self.pose_bytes(row)
            self.assertEqual([value == 0 for value in before], [value == 0 for value in after])
            self.assertEqual(bytes(value for index, value in enumerate(before) if not allowed[direction][index]),
                             bytes(value for index, value in enumerate(after) if not allowed[direction][index]))
            self.assertEqual(bytes(value for index, value in enumerate(before) if rims[direction][index]),
                             bytes(value for index, value in enumerate(after) if rims[direction][index]))
            self.assertEqual(row["anchor"], [-32, -32])

    def test_fill_levels_are_distinct_and_existing_grains_stay_fixed(self):
        """Add cargo to the approved bank without moving the body or existing grains.

        @param self Current focused check instance.
        @return None.
        """
        selected = {(row["cargo_label"], row["direction"], row["state"]): row for row in self.report["sprites"]}
        # Every material/heading independently exposes empty/partial/full. Native
        # fill classification must not collapse into a copied or invisible pose.
        for material in self.report["materials"]:
            for direction in fleet.DIRECTIONS:
                body = self.bodies[direction].tobytes()
                partial = self.pose_bytes(selected[material["material"], direction, "partial"])
                full = self.pose_bytes(selected[material["material"], direction, "full"])
                self.assertEqual(len({body, partial, full}), 3)
                self.assertTrue(all(complete == occupied for empty, occupied, complete in zip(body, partial, full) if empty != occupied))

    def test_all_eight_materials_survive_native_palette_conversion(self):
        """Prevent authored resource identities from collapsing into one generic ore.

        @param self Current focused check instance.
        @return None.
        """
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertEqual(self.atlas.getpalette(), fleet.base.PALETTE_BYTES)
        self.assertFalse(set(self.atlas.get_flattened_data()) & (fleet.base.FORBIDDEN - {0}))
        # Side-on holds are only two pixels tall, so each heading must retain
        # eight distinct original material presentations at the final palette.
        for direction in fleet.DIRECTIONS:
            poses = [self.pose_bytes(row) for row in self.report["sprites"] if row["state"] == "full" and row["direction"] == direction]
            self.assertEqual(len(set(poses)), 8, direction)

    def test_missing_material_remains_missing_without_fallback(self):
        """Keep absent cargo sources visible instead of borrowing another ore.

        @param self Current focused check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        del record["families"]["vehicle.model-05"]["approved_bulk_materials"]["COAL"]
        report, _files = fleet.build_bulk(document=record, public=self.public)
        self.assertEqual(report["bulk_frame0_pose_count"], 112)
        self.assertEqual(report["missing_pose_count"], 1112)
        self.assertIn(["partial", 0, "E", 0], report["missing"])
        self.assertFalse(report["source_coverage_complete"])

    def test_changed_material_bytes_and_crop_are_refused(self):
        """Protect actual source ancestry and measured original material selection.

        @param self Current focused check instance.
        @return None.
        """
        # Both file identity and interior registration matter; neither can be
        # replaced by a plausible filename or another area of the same sheet.
        for changed in ("bytes", "crop"):
            record = copy.deepcopy(self.record)
            source = next(row for row in record["candidates"] if row["key"] == "wagon-bulk-coal-iore-core-baux-v1")
            if changed == "bytes":
                source["sha256"] = "0" * 64
            else:
                source["component_materials"]["COAL"]["material_crop"][0] += 1
            with self.assertRaises(ValueError):
                fleet.bulk_material("COAL", record)

    def test_granules_cannot_replace_oriented_protected_cargo(self):
        """Require genuine object views for crates, ingots and protected concentrate.

        @param self Current focused check instance.
        @return None.
        """
        with self.assertRaisesRegex(ValueError, "actual bulk refit"):
            fleet.bulk_material("RARE", self.record)
        record = copy.deepcopy(self.record)
        source = next(row for row in record["candidates"] if row["key"] == "wagon-bulk-coal-iore-core-baux-v1")
        source["component_kind"] = "oriented_packages"
        with self.assertRaisesRegex(ValueError, "directional"):
            fleet.bulk_material("COAL", record)

    def test_fill_source_pointer_cannot_borrow_another_material(self):
        """Keep both fill-level source references aligned to the selected original.

        @param self Current focused check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        row = next(item for item in record["families"]["vehicle.model-05"]["cargo_source_matrix"] if item["cargo_label"] == "COAL")
        row["full_source"]["material"] = "IORE"
        with self.assertRaisesRegex(ValueError, "fill source differs"):
            fleet.bulk_material("COAL", record)

    def test_transparent_donor_or_open_body_hole_cannot_supply_bulk(self):
        """Refuse transparency shortcuts that could repaint or extend the wagon.

        @param self Current focused check instance.
        @return None.
        """
        with self.assertRaisesRegex(ValueError, "transparent gaps"):
            fleet.compose_bulk(self.bodies["N"], Image.new("RGBA", (16, 16)), "N", "full")
        body = self.bodies["N"].copy()
        body.putpixel((30, 24), 0)
        material, _evidence = fleet.bulk_material("COAL", self.record)
        with self.assertRaisesRegex(ValueError, "outside its body"):
            fleet.compose_bulk(body, material, "N", "full")

    def test_reordered_or_incomplete_source_refits_are_refused(self):
        """Keep source variants aligned to the actual native cargo catalogue.

        @param self Current focused check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        record["families"]["vehicle.model-05"]["cargo_source_matrix"].pop()
        with self.assertRaisesRegex(ValueError, "native catalogue"):
            fleet.build_bulk(document=record, public=self.public)

    def test_native_owner_or_aperture_drift_is_refused(self):
        """Prevent new presentation metadata from replacing real packets or geometry.

        @param self Current focused check instance.
        @return None.
        """
        # Independent public corruptions must fail before any material is read;
        # the art pipeline may not change quantity authority or body clearance.
        for changed in ("owner", "aperture"):
            public = copy.deepcopy(self.public)
            spec = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-05")
            if changed == "owner":
                spec["native_state_policy"]["cargo_owner"] = "art-local inventory"
            else:
                spec["cargo_geometry"]["bulk_full_polygons"]["E"][0][1] += 1
            with self.assertRaises(ValueError):
                fleet.build_bulk(document=self.record, public=public)


if __name__ == "__main__":
    unittest.main()
