"""Focused retained-original and native-stage proof for Coal construction source.

@file mine_construction_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group27C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Source-only coverage preserves all previously delivered native/packed identities.
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
import convert_mine_construction as construction
import convert_material_mine_art as material


class MineConstructionSourceChecks(unittest.TestCase):
    """Verify actual stages/cells, genuine ancestry, native geometry and false-source refusals."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the bounded construction comparison once from immutable inputs.

        @param cls Active proof class retaining source artifacts and prior delivery identities.
        @return None.
        """
        cls.record = json.loads(construction.mine.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(construction.mine.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == "mine.COAL")
        cls.report, cls.files = construction.build(cls.record, cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[construction.OUTPUT / "coal-mine-construction-atlas.png"]))
        cls.before = json.loads((ROOT / "logs/graphics/coal-mine/20261012-construction-01/before.json").read_text(encoding="utf-8"))

    def sprite(self, stage, variant, part=0):
        """Read one actual independent native owner without flattening another cell.

        @param self Active construction proof.
        @param stage Actual native construction phase0..3.
        @param variant Actual row-major machinery cell0..8.
        @param part Original underlay0 or independent upper-front1.
        @return Indexed64x64 native sprite.
        """
        x, y = (variant * 2 + part) * 64, stage * 64
        return self.atlas.crop((x, y, x + 64, y + 64))

    def test_exact_regeneration_and_complete_actual_stage_cell_part_matrix(self):
        """All72 explicit native owners regenerate exactly with full-family/runtime pending.

        @param self Active construction proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        self.assertEqual(self.atlas.size, (1152, 256))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        expected = {(stage, variant, part, variant % 3, variant // 3) for stage in range(4) for variant in range(9) for part in ("back", "front")}
        actual = {(row["frame"], row["tile_variant"], row["part"], *row["world_cell"]) for row in self.report["source_parts"]}
        self.assertEqual(actual, expected)
        self.assertEqual(len(self.report["source_parts"]), 72)
        self.assertTrue(self.report["construction_coverage_complete"])
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertIsNone(self.public["packed"]["offset"])
        self.assertEqual(self.report["loading_cells"], [[0, 3], [1, 3], [0, 4], [1, 4]])

    def test_completed_stage_preserves_every_approved_base_part_exactly(self):
        """Per-cell stage3 is the retained completed model even before whole-industry completion.

        @param self Active construction proof.
        @return None.
        """
        with Image.open(construction.OUTPUT / "coal-mine-base-atlas.png") as base:
            self.assertEqual(self.atlas.crop((0, 192, 1152, 256)).tobytes(), base.tobytes())
        # Genuine unfinished/finishing roles differ from each other and from
        # their final model; a repeated ready cell cannot fill missing stages.
        for variant in range(9):
            self.assertNotEqual(self.sprite(1, variant).tobytes(), self.sprite(2, variant).tobytes(), variant)
            self.assertNotEqual(self.sprite(2, variant).tobytes(), self.sprite(3, variant).tobytes(), variant)

    def test_exact_compatible_foundation_and_hut_native_ancestry(self):
        """Only the original compatible yard/hut are reused, with original finishing glass.

        @param self Active construction proof.
        @return None.
        """
        reuse = self.record["families"]["mine.COAL"]["construction_reuse"]
        with Image.open(ROOT / reuse["source_native_atlas"]) as donor:
            foundation = donor.crop((512, 0, 576, 64))
            hut = donor.crop((640, 0, 704, 64))
        for variant in range(9):
            self.assertEqual(self.sprite(0, variant).tobytes(), foundation.tobytes())
        self.assertEqual(self.sprite(1, 7).tobytes(), hut.tobytes())
        finishing, ready = self.sprite(2, 7), self.sprite(3, 7)
        changed = {(x, y) for y in range(64) for x in range(64) if finishing.getpixel((x, y)) != ready.getpixel((x, y))}
        self.assertTrue(changed)
        self.assertLessEqual(changed, {(x, y) for y in range(43, 46) for x in range(30, 33)})
        self.assertEqual([bool(v) for v in finishing.get_flattened_data()], [bool(v) for v in ready.get_flattened_data()])

    def test_all_stages_meet_native_contact_height_palette_and_source_part_ownership(self):
        """Every original fits its actual paid cell and independent parts retain original pixels.

        @param self Active construction proof.
        @return None.
        """
        # Every phase needs the same physical geometry checks. Native empty
        # floor/upper parts still remain actual nonempty source owners.
        for stage in range(4):
            for variant in range(9):
                back, front = self.sprite(stage, variant), self.sprite(stage, variant, 1)
                construction.mine.validate_cell(back, variant)
                self.assertIsNotNone(front.getbbox())
                self.assertTrue(all(new == 0 or new == old for old, new in zip(back.get_flattened_data(), front.get_flattened_data())))
                self.assertEqual(bytes(new if new else old for old, new in zip(back.get_flattened_data(), front.get_flattened_data())), back.tobytes())

    def test_sixteen_original_components_have_clean_whole_rectangles_and_uniform_registration(self):
        """Selected stages retain their actual original crop and fixed native floor registration.

        @param self Active construction proof.
        @return None.
        """
        family = self.record["families"]["mine.COAL"]
        for stage, selections in family["construction_cells"].items():
            self.assertEqual(len(selections), 8)
            for selection in selections:
                source, component = construction.mine.component_record(self.record, selection)
                self.assertEqual(source["construction_stage"], int(stage))
                self.assertEqual(component["construction_stage"], int(stage))
                measurement = next(row for row in source["measurements"] if row["variant"] == selection["variant"])
                self.assertGreaterEqual(measurement["minimum_alpha16_clearance"], 16)
                self.assertEqual(measurement["opaque_magenta_pixels"], 0)
                self.assertEqual(component["source_window"], measurement["source_window"])
                x0, y0, x1, y1 = component["native_box"]
                left, top, right, bottom = component["measured_alpha16_bbox"]
                self.assertEqual((x0, x1, y1), (1, 63, 64))
                self.assertLess(abs(((x1 - x0) / (right - left)) / ((y1 - y0) / (bottom - top)) - 1), .02)

    def test_missing_duplicate_and_changed_native_stage_matrices_refuse(self):
        """Actual source coverage cannot be completed by dropping/relabeling machinery owners.

        @param self Active construction proof.
        @return None.
        """
        for change in ("missing_stage", "missing_cell", "duplicate_cell", "registration"):
            record = copy.deepcopy(self.record)
            family = record["families"]["mine.COAL"]
            if change == "missing_stage":
                family["construction_cells"].pop("2")
            elif change == "missing_cell":
                family["construction_cells"]["1"].pop()
            elif change == "duplicate_cell":
                family["construction_cells"]["1"][-1] = copy.deepcopy(family["construction_cells"]["1"][0])
            else:
                _source, component = construction.mine.component_record(record, family["construction_cells"]["1"][0])
                component["native_box"][0] += 1
            with self.subTest(change=change), self.assertRaises(ValueError):
                construction.stage_cells(record, self.public)

    def test_wrong_stage_changed_original_reference_and_rejected_selection_refuse(self):
        """Even a valid-shaped component requires exact approved original and native stage ancestry.

        @param self Active construction proof.
        @return None.
        """
        for change in ("source_stage", "component_stage", "original", "reference", "rejected", "excluded"):
            record = copy.deepcopy(self.record)
            selection = record["families"]["mine.COAL"]["construction_cells"]["1"][0]
            source, component = construction.mine.component_record(record, selection)
            if change == "source_stage":
                source["construction_stage"] = 2
            elif change == "component_stage":
                component["construction_stage"] = 2
            elif change == "original":
                source["sha256"] = "0" * 64
            elif change == "reference":
                source["references"][0]["sha256"] = "0" * 64
            elif change == "rejected":
                source["selection"]["status"] = "rejected_native_placement"
            else:
                source["excluded_variants"] = [selection["variant"]]
            with self.subTest(change=change), self.assertRaises(ValueError):
                construction.stage_original(record, selection, 1, construction.mine.mine_contract("COAL"))

    def test_foreign_foundation_hut_phase_and_native_reuse_refuse(self):
        """Recomputing a caption hash cannot turn incompatible donor apparatus into Coal construction.

        @param self Active construction proof.
        @return None.
        """
        for field, value in (("foundation_source_variant", 1), ("hut_source_variant", 3), ("hut_target", 8),
                             ("source_native_sha256", "0" * 64), ("phase_component_sha256", "0" * 64),
                             ("finishing_hut_glass_frame", 0), ("cue_target", [29, 43, 3, 3])):
            record = copy.deepcopy(self.record)
            family = record["families"]["mine.COAL"]
            family["construction_reuse"][field] = value
            family["approved_construction_reuse_sha256"] = construction.mine.signature(family["construction_reuse"])
            with self.subTest(field=field), self.assertRaises(ValueError):
                construction.reused_cells(record)

    def test_unsafe_crop_and_nonuniform_or_shifted_registration_refuse(self):
        """Whole-original extraction cannot hide clipping or move a missing stage onto paid floor.

        @param self Active construction proof.
        @return None.
        """
        for change in ("crop", "aspect", "anchor", "world_cell"):
            record = copy.deepcopy(self.record)
            selection = record["families"]["mine.COAL"]["construction_cells"]["1"][0]
            _source, component = construction.mine.component_record(record, selection)
            if change == "crop":
                component["source_window"][0] += component["measured_alpha16_bbox"][0]
                component["measured_alpha16_bbox"][2] -= component["measured_alpha16_bbox"][0]
                component["measured_alpha16_bbox"][0] = 0
            elif change == "aspect":
                component["native_box"][1] += 3
            elif change == "anchor":
                component["anchor"][0] += 1
            else:
                component["world_cell"] = [2, 2]
            with self.subTest(change=change), self.assertRaises(ValueError):
                construction.stage_original(record, selection, 1, construction.mine.mine_contract("COAL"))

    def test_all_prior_delivery_identities_and_public_rows_are_preserved(self):
        """Construction source cannot overwrite completed bases, packed ranges or native binaries.

        @param self Active construction proof.
        @return None.
        """
        for row in self.before["preserved_identities"]:
            self.assertEqual(construction.mine.base.digest(ROOT / row["path"]), row["sha256"], row["path"])
        for row in self.document["assets"]:
            # The subsequent Coal operating-state group may publish its own
            # complete source; all other native/source families remain exact.
            if row["asset_id"] == "mine.COAL":
                construction.mine.validate_mine_contract(row)
                continue
            # A subsequent selected material source owns its new authored
            # metadata only after exact native source/payload validation.
            if material.validate_following_source(row):
                continue
            self.assertEqual(construction.mine.signature(row), self.before["public_family_sha256"][row["asset_id"]], row["asset_id"])
        family = self.record["families"]["mine.COAL"]
        for key in ("approved_base_component_geometry_sha256", "approved_control_reuse_sha256"):
            self.assertEqual(family[key], self.before[key])


if __name__ == "__main__":
    unittest.main()
