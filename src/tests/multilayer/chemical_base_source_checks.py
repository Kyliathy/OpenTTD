"""Focused completed chemical source, exact individual reuse and source-only publication proof.

@file chemical_base_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group40B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
These checks prove32 completed base parts; genuine full512 poses and native
packing/game display retain their separate40C/master10.4D requirements.
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
import convert_chemical_base as plant


class ChemicalBaseSourceChecks(unittest.TestCase):
    """Require genuine completed apparatus and actual native ancestry while preserving earlier delivery."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the executing completed base from the retained actual source once.

        @param cls Active chemical source proof with immutable model/public/native bindings.
        @return None.
        """
        cls.record = json.loads(plant.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(plant.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == plant.ASSET)
        cls.family = cls.record["families"][plant.ASSET]
        cls.before = json.loads((ROOT / cls.family["verification_baseline"]).read_text(encoding="utf-8"))
        cls.report, cls.files = plant.build(cls.record, cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[plant.OUTPUT / f"{plant.PREFIX}-atlas.png"]))

    def sprite(self, variant, part=0):
        """Read an independently owned actual native completed source part.

        @param self Active completed chemical source proof.
        @param variant Actual row-major machinery owner0..15.
        @param part Whole original underlay0 or intentional original upper subset1.
        @return Indexed64x64 native source at unchanged anchor.
        """
        x = (variant * 2 + part) * 64
        return self.atlas.crop((x, 0, x + 64, 64))

    def test_exact_regeneration_and_all_32_actual_completed_source_slots(self):
        """All retained base artifacts and ordered native cells reproduce from genuine selected originals.

        @param self Active completed chemical source proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        expected = {(variant, part, variant % 4, variant // 4) for variant in range(16) for part in plant.mine.PARTS}
        actual = {(row["tile_variant"], row["part"], *row["world_cell"]) for row in self.report["source_parts"]}
        self.assertEqual(actual, expected)
        self.assertEqual(len(self.report["source_parts"]), 32)
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.size, (2048, 64))
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertIn("Group40B", self.atlas.info["Owner"])
        self.assertEqual(len({self.sprite(variant).tobytes() for variant in range(16)}), 14)
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertEqual(self.report["source_slots_required"], 512)

    def test_seven_new_complete_empty_or_closed_originals_keep_safe_registration(self):
        """Every selected rectangle is genuine completed apparatus with frozen references and zero source separators.

        @param self Active completed chemical source proof.
        @return None.
        """
        self.assertEqual({row["variant"] for row in self.family["base_cells"]}, set(plant.NEW_VARIANTS))
        # Independently selected components cannot inherit model or extraction
        # approval from neighbouring rejected floors, tall fittings or source haze.
        for selection in self.family["base_cells"]:
            source, component = plant.mine.component_record(self.record, selection)
            plant.mine.facilities.verify_original(source)
            self.assertEqual(source["tool"], "image_gen.imagegen")
            self.assertEqual(source["processor_asset_id"], plant.ASSET)
            self.assertEqual(source["construction_stage"], 3)
            self.assertTrue(source["completed_empty"])
            self.assertNotIn(selection["variant"], source["excluded_variants"])
            measure = next(row for row in source["measurements"] if row["variant"] == selection["variant"])
            self.assertGreaterEqual(measure["minimum_alpha16_clearance"], 16)
            self.assertEqual(measure["opaque_magenta_pixels"], 0)
            bounds = component["measured_alpha16_bbox"]
            height = round((bounds[3] - bounds[1]) * 62 / (bounds[2] - bounds[0]))
            self.assertEqual(component["native_box"], [1, 64 - height, 63, 64])
            self.assertEqual(component["world_cell"], self.public["native_cells"][selection["variant"]])

    def test_nine_reused_cells_keep_exact_individual_original_native_pixels(self):
        """Only compatible physical apparatus, independent local duplicates and exact M1 control pixels are reused.

        @param self Active completed chemical source proof.
        @return None.
        """
        self.assertEqual(self.family["reuse_graph"], plant.expected_reuse(self.record))
        self.assertEqual([row["variant"] for row in self.family["reuse_graph"]], [0, 1, 8, 9, 10, 11, 13, 14, 15])
        # Matching individual cells preserve exact completed donor pixels at
        # new world offsets; this never copies an industry or its physical stock.
        with Image.open(plant.OUTPUT / "metal-refinery-base-atlas.png") as donor:
            for target, source_variant in (*plant.DONORS, (11, 11)):
                x = source_variant * 128
                self.assertEqual(self.sprite(target).tobytes(), donor.crop((x, 0, x + 64, 64)).tobytes())
        self.assertEqual(self.sprite(8).tobytes(), self.sprite(5).tobytes())
        self.assertEqual(self.sprite(10).tobytes(), self.sprite(3).tobytes())

    def test_native_contacts_volume_palette_and_original_foreground_reconstruct(self):
        """Every actual completed cell fits the native paid volume and preserves its whole original source parents.

        @param self Active completed chemical source proof.
        @return None.
        """
        # Native corners, actual south tip and20-high sorting bound every whole
        # cell, including reuse. Upper foreground pixels intentionally overlap.
        for variant in range(16):
            original, front = self.sprite(variant), self.sprite(variant, 1)
            plant.mine.validate_cell(original, variant)
            self.assertIsNotNone(front.getbbox())
            for source, upper in zip(original.get_flattened_data(), front.get_flattened_data()):
                if upper:
                    self.assertEqual(source, upper)
            back = original.copy()
            mask = Image.new("L", (64, 64))
            mask.putdata([255 if value else 0 for value in front.get_flattened_data()])
            back.paste(front, (0, 0), mask)
            self.assertEqual(back.tobytes(), original.tobytes())
        with Image.open(plant.OUTPUT / f"{plant.PREFIX}-campus.png") as scene:
            self.assertEqual(scene.size, (320, 192))

    def test_missing_duplicate_foreign_unfinished_or_changed_originals_refuse(self):
        """A plausible matrix or raw rectangle cannot conceal missing genuine completed chemical apparatus.

        @param self Active completed chemical source proof.
        @return None.
        """
        candidates = []
        missing = copy.deepcopy(self.record)
        missing["families"][plant.ASSET]["base_cells"].pop()
        candidates.append(missing)
        duplicate = copy.deepcopy(self.record)
        duplicate["families"][plant.ASSET]["base_cells"][-1] = duplicate["families"][plant.ASSET]["base_cells"][0]
        candidates.append(duplicate)
        # Each original metadata mutation independently changes a required
        # source owner/stage or immutable raw/reference identity before publication.
        for key, value in (("processor_asset_id", plant.shared.ASSET), ("construction_stage", 2),
                           ("completed_empty", False), ("sha256", "0" * 64)):
            changed = copy.deepcopy(self.record)
            source, _component = plant.mine.component_record(changed, changed["families"][plant.ASSET]["base_cells"][0])
            source[key] = value
            candidates.append(changed)
        changed = copy.deepcopy(self.record)
        source, _component = plant.mine.component_record(changed, changed["families"][plant.ASSET]["base_cells"][0])
        source["references"][0]["sha256"] = "0" * 64
        candidates.append(changed)
        for changed in candidates:
            with self.assertRaises(ValueError):
                plant.completed_cells(changed, self.public)

    def test_forged_reuse_registration_native_geometry_or_model_inventory_refuse(self):
        """Recomputed captions cannot authorize another physical role, shifted floor, rotation or changed apparatus.

        @param self Active completed chemical source proof.
        @return None.
        """
        changed = copy.deepcopy(self.record)
        family = changed["families"][plant.ASSET]
        family["reuse_graph"][0]["source_selection"] = family["reuse_graph"][1]["source_selection"]
        family["approved_reuse_sha256"] = plant.mine.signature(family["reuse_graph"])
        with self.assertRaises(ValueError):
            plant.completed_cells(changed, self.public)
        changed = copy.deepcopy(self.record)
        family = changed["families"][plant.ASSET]
        _source, component = plant.mine.component_record(changed, family["base_cells"][0])
        component["native_box"][0] = 2
        family["approved_base_component_geometry_sha256"] = plant.mine.signature(
            {str(row["variant"]): plant.mine.component_record(changed, row)[1] for row in family["base_cells"]})
        with self.assertRaises(ValueError):
            plant.completed_cells(changed, self.public)
        for key, value in (("directions", ["NE", "SE", "SW", "NW"]), ("native_size", [96, 64]), ("source_height_limit", 48)):
            public = copy.deepcopy(self.public)
            public[key] = value
            with self.assertRaises(ValueError):
                plant.completed_cells(self.record, public)
        changed = copy.deepcopy(self.record)
        changed["families"][plant.ASSET]["selected_model_inventory"]["7"] = "tall incomplete filter pack"
        with self.assertRaises(ValueError):
            plant.completed_cells(changed, self.public)

    def test_public_base_requires_exact_live_payload_without_full_source_or_runtime_claim(self):
        """Only the live32-part handoff can complete this base; full512 source and packed activation still refuse.

        @param self Active completed chemical source proof.
        @return None.
        """
        plant.validate_public_base(self.public, self.report)
        for key, value in (("selected_base_nonempty_parts", 31), ("base_native_atlas", "foreign.png"),
                           ("coverage_complete", True), ("runtime_activated", True)):
            public = copy.deepcopy(self.public)
            public["authoring"][key] = value
            with self.assertRaises(ValueError):
                plant.validate_public_base(public, self.report)
        public, report = copy.deepcopy(self.public), copy.deepcopy(self.report)
        public["authoring"]["base_native_sha256"] = report["base_native_sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            plant.validate_public_base(public, report)
        public = copy.deepcopy(self.public)
        public["packed"]["offset"] = 10000
        with self.assertRaises(ValueError):
            plant.validate_public_base(public, self.report)
        self.assertTrue(self.family["base_coverage_complete"])
        self.assertFalse(self.family["source_coverage_complete"])
        self.assertFalse(self.public["authoring"]["coverage_complete"])
        self.assertFalse(self.public["authoring"]["runtime_activated"])

    def test_all_prior_metal_mine_raw_native_code_and_public_owners_stay_exact(self):
        """All487 earlier identities and the complete Metal source/candidate/public documents remain exact.

        @param self Active completed chemical source proof.
        @return None.
        """
        self.assertEqual(plant.mine.base.digest(plant.mine.RECORD), self.before["mine_record_sha256"])
        for row in self.before["preserved_identities"]:
            self.assertEqual(plant.mine.base.digest(ROOT / row["path"]), row["sha256"], row["path"])
        for name, signature in self.before["processor_family_sha256"].items():
            self.assertEqual(plant.mine.signature(self.record["families"][name]), signature, name)
        for source in self.record["candidates"]:
            if source["id"] in self.before["processor_candidate_sha256"]:
                self.assertEqual(plant.mine.signature(source), self.before["processor_candidate_sha256"][source["id"]])
        for row in self.document["assets"]:
            if row["asset_id"] != plant.ASSET:
                self.assertEqual(plant.mine.signature(row), self.before["public_family_sha256"][row["asset_id"]], row["asset_id"])
