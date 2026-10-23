"""Focused genuine completed refinery base, exact reuse and source-only publication proof.

@file processor_base_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This runner proves32 completed source parts only. Full512 poses, packed ranges
and actual ordinary-game display require their separately dispatched owners.
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
import convert_processor_base as plant


class ProcessorBaseSourceChecks(unittest.TestCase):
    """Require complete native model/original ancestry without claiming future states or game delivery."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the executing sixteen-cell completed base from actual retained originals once.

        @param cls Active source proof retaining exact completed model and public handoff.
        @return None.
        """
        cls.record = json.loads(plant.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(plant.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == plant.ASSET)
        cls.family = cls.record["families"][plant.ASSET]
        cls.before = json.loads((ROOT / cls.family["verification_baseline"]).read_text(encoding="utf-8"))
        cls.cells = plant.completed_cells(cls.record, cls.public)
        cls.report, cls.files = plant.build(cls.record, cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[plant.OUTPUT / "metal-refinery-base-atlas.png"]))

    def sprite(self, variant, part=0):
        """Read an independently owned actual native completed source part.

        @param self Active completed refinery source proof.
        @param variant Actual row-major machinery cell0..15.
        @param part Original underlay0 or independent original upper-front1.
        @return Indexed64x64 source at the fixed native anchor.
        """
        x = (variant * 2 + part) * 64
        return self.atlas.crop((x, 0, x + 64, 64))

    def test_exact_regeneration_of_all_32_actual_completed_native_source_parts(self):
        """Every retained completed source artifact regenerates exactly with all sixteen native owners.

        @param self Active completed refinery source proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        expected = {(variant, part, variant % 4, variant // 4) for variant in range(16) for part in ("back", "front")}
        actual = {(row["tile_variant"], row["part"], *row["world_cell"]) for row in self.report["source_parts"]}
        self.assertEqual(actual, expected)
        self.assertEqual(len(self.report["source_parts"]), 32)
        self.assertTrue(all(row["nonempty_pixels"] > 0 and not row["structurally_absent"] for row in self.report["source_parts"]))
        self.assertEqual(self.atlas.size, (2048, 64))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertIn("Group39C", self.atlas.info["Owner"])
        self.assertEqual(len({self.sprite(variant).tobytes() for variant in range(16)}), 14)

    def test_genuine_completed_empty_originals_have_safe_native_registration(self):
        """Twelve new genuine complete apparatus cells retain exact raw/reference/crop/native ancestry.

        @param self Active completed refinery source proof.
        @return None.
        """
        self.assertEqual({row["variant"] for row in self.family["base_cells"]}, set(plant.NEW_VARIANTS))
        # Each selected rectangle belongs to a completed empty model, including
        # partial sheets whose explicitly excluded neighbours cannot supply art.
        for selection in self.family["base_cells"]:
            source, component = plant.mine.component_record(self.record, selection)
            plant.mine.facilities.verify_original(source)
            self.assertEqual(source["processor_asset_id"], plant.ASSET)
            self.assertEqual(source["construction_stage"], 3)
            self.assertIs(source["completed_empty"], True)
            self.assertNotIn(selection["variant"], source.get("excluded_variants", []))
            measurement = next(row for row in source["measurements"] if row["variant"] == selection["variant"])
            self.assertGreaterEqual(measurement["minimum_alpha16_clearance"], 16)
            self.assertEqual(measurement["opaque_magenta_pixels"], 0)
            bounds = component["measured_alpha16_bbox"]
            height = round((bounds[3] - bounds[1]) * 62 / (bounds[2] - bounds[0]))
            self.assertEqual(component["native_box"], [1, 64 - height, 63, 64])
            self.assertEqual(component["world_cell"], self.public["native_cells"][selection["variant"]])

    def test_reuse_is_exact_individual_empty_fuel_apparatus_m1_hut_and_noncargo_filter(self):
        """Only four explicitly compatible cells reuse exact pixels; no whole industry or foreign stock is copied.

        @param self Active completed refinery source proof.
        @return None.
        """
        self.assertEqual([row["variant"] for row in self.family["reuse_graph"]], [4, 5, 11, 12])
        self.assertEqual(self.cells[4].tobytes(), self.cells[0].tobytes())
        self.assertEqual(self.cells[5].tobytes(), self.cells[1].tobytes())
        reuse = self.family["reuse_graph"][2]["donor_control_reuse"]
        with Image.open(ROOT / reuse["source_native_atlas"]) as donor:
            self.assertEqual(self.cells[11].tobytes(), donor.crop((640, 256, 704, 320)).tobytes())
        with Image.open(plant.OUTPUT / "coal-mine-base-atlas.png") as donor:
            self.assertEqual(self.cells[12].tobytes(), donor.crop((1024, 0, 1088, 64)).tobytes())

    def test_native_contacts_palette_height_and_original_foreground_reconstruct(self):
        """Whole originals retain the native volume and intentional repeated underlay/front pixel policy.

        @param self Active completed refinery source proof.
        @return None.
        """
        # Native placement and original overlapping parents are checked for
        # every actual cell, including exact source reuse at new world offsets.
        for variant, original in self.cells.items():
            plant.mine.validate_cell(original, variant)
            self.assertEqual(self.sprite(variant).tobytes(), original.tobytes())
            back, front = self.sprite(variant), self.sprite(variant, 1)
            self.assertIsNotNone(front.getbbox())
            for source, upper in zip(back.get_flattened_data(), front.get_flattened_data()):
                if upper:
                    self.assertEqual(source, upper)
            mask = Image.new("L", (64, 64))
            mask.putdata([255 if value else 0 for value in front.get_flattened_data()])
            back.paste(front, (0, 0), mask)
            self.assertEqual(back.tobytes(), original.tobytes())
        with Image.open(plant.OUTPUT / "metal-refinery-base-campus.png") as scene:
            self.assertEqual(scene.size, (320, 192))

    def test_missing_duplicate_relabelled_unfinished_and_changed_raw_or_reference_refuse(self):
        """Plausible base counts or captions cannot replace required genuine completed apparatus.

        @param self Active completed refinery source proof.
        @return None.
        """
        candidates = []
        missing = copy.deepcopy(self.record)
        missing["families"][plant.ASSET]["base_cells"].pop()
        candidates.append(missing)
        duplicate = copy.deepcopy(self.record)
        duplicate["families"][plant.ASSET]["base_cells"][-1] = duplicate["families"][plant.ASSET]["base_cells"][0]
        candidates.append(duplicate)
        # These independent original metadata changes must refuse before any
        # missing/foreign/unfinished pixels can enter the completed native base.
        for key, value in (("processor_asset_id", "facility.chemical-works"), ("construction_stage", 2),
                           ("completed_empty", False), ("sha256", "0" * 64)):
            changed = copy.deepcopy(self.record)
            source, _component = plant.mine.component_record(changed, changed["families"][plant.ASSET]["base_cells"][0])
            source[key] = value
            candidates.append(changed)
        changed_reference = copy.deepcopy(self.record)
        source, _component = plant.mine.component_record(changed_reference, changed_reference["families"][plant.ASSET]["base_cells"][0])
        source["references"][0]["sha256"] = "0" * 64
        candidates.append(changed_reference)
        for changed in candidates:
            with self.assertRaises(ValueError):
                plant.completed_cells(changed, self.public)

    def test_changed_reuse_registration_native_geometry_and_model_refuse(self):
        """Recomputed captions/hashes cannot authorize a foreign role, shifted floor or changed physical model.

        @param self Active completed refinery source proof.
        @return None.
        """
        changed = copy.deepcopy(self.record)
        family = changed["families"][plant.ASSET]
        family["reuse_graph"][0]["source_variant"] = 2
        family["approved_reuse_sha256"] = plant.mine.signature(family["reuse_graph"])
        with self.assertRaises(ValueError):
            plant.completed_cells(changed, self.public)
        changed = copy.deepcopy(self.record)
        family = changed["families"][plant.ASSET]
        _source, component = plant.mine.component_record(changed, family["base_cells"][0])
        component["native_box"][0] = 2
        family["approved_base_component_geometry_sha256"] = plant.mine.signature({str(row["variant"]): plant.mine.component_record(changed, row)[1] for row in family["base_cells"]})
        with self.assertRaises(ValueError):
            plant.completed_cells(changed, self.public)
        for key, value in (("native_size", [96, 64]), ("directions", ["NE", "SE", "SW", "NW"]), ("source_height_limit", 48)):
            public = copy.deepcopy(self.public)
            public[key] = value
            with self.assertRaises(ValueError):
                plant.completed_cells(self.record, public)
        changed = copy.deepcopy(self.record)
        changed["families"][plant.ASSET]["model"] = "different equipment count"
        with self.assertRaises(ValueError):
            plant.completed_cells(changed, self.public)

    def test_public_base_requires_live_payload_without_full_source_or_runtime_claim(self):
        """The exact32-part base accepts a separate live full handoff and refuses forged coverage/activation.

        @param self Active completed refinery source proof.
        @return None.
        """
        plant.validate_public_base(self.public, self.report)
        # Later full source must prove its own live512 slots. The completed
        # base and full source retain separate actual artifacts and evidence.
        for key, value in (("selected_base_nonempty_parts", 31), ("base_native_atlas", "foreign.png"),
                           ("coverage_complete", None), ("runtime_activated", True)):
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
        if self.public["authoring"]["coverage_complete"]:
            # Complete source must bind its actual live matrix, rather than
            # obtaining authority from the historical completed-base receipt.
            for key, value in (("staged_pose_count", 511), ("native_atlas", "foreign.png")):
                public = copy.deepcopy(self.public)
                public["authoring"][key] = value
                with self.assertRaises(ValueError):
                    plant.validate_public_base(public, self.report)
        else:
            public = copy.deepcopy(self.public)
            public["authoring"]["coverage_complete"] = True
            with self.assertRaises(ValueError):
                plant.validate_public_base(public, self.report)
        self.assertFalse(self.public["authoring"]["runtime_activated"])
        self.assertEqual(self.report["base_source_parts"], 32)
        self.assertEqual(self.report["source_slots_required"], 512)

    def test_prior_art_mine_originals_native_binaries_and_other_public_rows_stay_exact(self):
        """All448 previous source/native identities and every other family retain their exact current bytes.

        @param self Active completed refinery source proof.
        @return None.
        """
        self.assertEqual(plant.mine.base.digest(plant.mine.RECORD), self.before["mine_record_sha256"])
        for row in self.before["preserved_identities"]:
            self.assertEqual(plant.mine.base.digest(ROOT / row["path"]), row["sha256"], row["path"])
        for row in self.document["assets"]:
            if row["asset_id"] != plant.ASSET:
                self.assertEqual(plant.mine.signature(row), self.before["public_family_sha256"][row["asset_id"]], row["asset_id"])
