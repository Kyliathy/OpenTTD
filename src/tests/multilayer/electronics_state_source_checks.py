"""Focused electronics works genuine stages, native phase aliases and full source preservation proof.

@file electronics_state_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group42C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
These checks establish512 original source parts. Packed pixels and ordinary-game
rendering remain separately dispatched through master10.4D.
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
import convert_electronics_states as plant


class ElectronicsStateSourceChecks(unittest.TestCase):
    """Prove actual-stage ancestry and original-only state cues without granting runtime authority."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the executing complete native source once from retained originals.

        @param cls Active source proof with original model, compiled geometry and preserved handoff.
        @return None.
        """
        cls.record = json.loads(plant.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(plant.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == plant.ASSET)
        cls.family = cls.record["families"][plant.ASSET]
        cls.before = json.loads((ROOT / cls.family["states_verification_baseline"]).read_text(encoding="utf-8"))
        cls.report, cls.files = plant.build(cls.record, cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[plant.OUTPUT / f"{plant.PREFIX}-authored.png"]))

    def sprite(self, state, frame, variant, part=0):
        """Read one independent actual native source part.

        @param self Active electronics works proof with regenerated original source atlas.
        @param state Native construction/ready/working/blocked state.
        @param frame Actual construction stage or native presentation phase0..3.
        @param variant Actual row-major machinery owner0..15.
        @param part Whole original underlay0 or intentional original upper subset1.
        @return Indexed64x64 native source at unchanged anchor.
        """
        x, y = (variant * 2 + part) * 64, (plant.STATES.index(state) * 4 + frame) * 64
        return self.atlas.crop((x, y, x + 64, y + 64))

    def test_full_source_regenerates_exactly_and_covers_all_actual_512_slots(self):
        """Every live artifact and ordered native source slot reproduces from actual retained originals.

        @param self Active complete electronics works source proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        expected = {(state, frame, variant, part) for state in plant.STATES for frame in range(4)
                    for variant in range(16) for part in plant.mine.PARTS}
        actual = {(row["state"], row["frame"], row["tile_variant"], row["part"]) for row in self.report["source_parts"]}
        self.assertEqual(actual, expected)
        self.assertEqual(len(self.report["source_parts"]), 512)
        self.assertTrue(all(row["nonempty_pixels"] > 0 and not row["structurally_absent"] for row in self.report["source_parts"]))
        self.assertEqual(self.atlas.size, (2048, 1024))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertIn("Group42C", self.atlas.info["Owner"])

    def test_frame_antialias_exception_cannot_hide_gutter_or_model_pixels(self):
        """Only real canonical frame edges may retain their red220 antialiased magenta fringe.

        @param self Active Electronics source proof with measured immutable source windows.
        @return None; synthetic reader faults never change source files or selected art.
        """
        source, _component = plant.mine.component_record(self.record, self.family["construction_cells"]["1"][0])
        plant.validate_construction_layout(source)
        # The same faint magenta belongs to a frame at x27 but is unsafe in
        # the inter-panel gutter. Non-magenta apparatus is unsafe even on that
        # frame edge; geometry recognition cannot discard a real model pixel.
        for point, color, refuses in (((27, 250), (220, 4, 232, 20), False),
                                      ((760, 256), (220, 4, 232, 20), True),
                                      ((27, 250), (100, 120, 100, 255), True)):
            image = Image.new("RGBA", (1536, 1024))
            image.putpixel(point, color)
            with mock.patch.object(plant.Image, "open", return_value=image):
                if refuses:
                    with self.assertRaisesRegex(ValueError, "leave complete source windows"):
                        plant.validate_construction_layout(source)
                else:
                    plant.validate_construction_layout(source)
        changed = copy.deepcopy(source)
        changed["components"][1]["source_window"] = changed["components"][0]["source_window"]
        with self.assertRaisesRegex(ValueError, "overlap"):
            plant.validate_construction_layout(changed)

    def test_all_64_construction_cells_keep_contacts_foreground_and_exact_completed_base(self):
        """Each original stage keeps whole native floors and intentional overlapping source-parent pixels.

        @param self Active complete electronics works source proof.
        @return None.
        """
        with Image.open(plant.OUTPUT / "electronics-works-base-atlas.png") as base:
            self.assertEqual(self.atlas.crop((0, 192, 2048, 256)).tobytes(), base.tobytes())
        # Every per-tile stage is independently placeable. Intentional upper
        # source overlap preserves the whole original when parented in native order.
        for stage in range(4):
            for variant in range(16):
                back, front = self.sprite("construction", stage, variant), self.sprite("construction", stage, variant, 1)
                plant.mine.validate_cell(back, variant)
                self.assertIsNotNone(front.getbbox())
                for source, upper in zip(back.get_flattened_data(), front.get_flattened_data()):
                    if upper:
                        self.assertEqual(source, upper)
        self.assertEqual(len({self.sprite("construction", 0, variant).tobytes() for variant in range(16)}), 1)
        for variant in range(16):
            self.assertEqual(len({self.sprite("construction", stage, variant).tobytes() for stage in range(4)}), 4, variant)

    def test_eight_new_unfinished_cells_have_genuine_stage_and_safe_immutable_registration(self):
        """Selected original rectangles have genuine unfinished owners, intact alpha clearance and zero separator pixels.

        @param self Active complete electronics works source proof.
        @return None.
        """
        # Separate partial sheets remain independent: rejected neighbours and
        # originals with opaque haze never inherit approval from selected cells.
        for stage, rows in self.family["construction_cells"].items():
            self.assertEqual({row["variant"] for row in rows}, set(plant.NEW_VARIANTS))
            for selection in rows:
                source, component = plant.mine.component_record(self.record, selection)
                plant.mine.facilities.verify_original(source)
                self.assertIn(selection["variant"], source["selection"]["approved_variants"])
                self.assertTrue(source["selection"]["model_review"][str(selection["variant"])])
                self.assertEqual(source["tool"], "image_gen.imagegen")
                self.assertEqual(source["construction_stage"], int(stage))
                self.assertEqual(component["construction_stage"], int(stage))
                self.assertEqual(source["processor_asset_id"], plant.ASSET)
                self.assertTrue(source["unfinished_empty"])
                self.assertNotIn(selection["variant"], source["excluded_variants"])
                measure = next(row for row in source["measurements"] if row["variant"] == selection["variant"])
                self.assertGreaterEqual(measure["minimum_alpha16_clearance"], 16)
                self.assertEqual(measure["opaque_magenta_pixels"], 0)
                bounds = component["measured_alpha16_bbox"]
                height = round((bounds[3] - bounds[1]) * 62 / (bounds[2] - bounds[0]))
                self.assertEqual(component["native_box"], [1, 64 - height, 63, 64])

    def test_compatible_stage_reuse_matches_exact_individual_donor_native_pixels(self):
        """Nine apparatus targets per stage and the paid foundation/hut keep exact individual original ancestry.

        @param self Active complete electronics works source proof.
        @return None.
        """
        reuse = self.family["construction_reuse"]
        self.assertEqual(reuse, plant.expected_construction_reuse(self.record))
        self.assertEqual(len(reuse["apparatus"]), 18)
        self.assertEqual(reuse["local_aliases"], {"2": 0, "3": 1})
        # Each compatible original is compared with its own actual same-stage
        # donor pixels; the composed campus cannot hide a wrong physical role.
        for row in reuse["apparatus"]:
            prefix = "coal-mine" if row["source_family"] == "mine.COAL" else row["source_family"].split(".", 1)[1]
            with Image.open(plant.OUTPUT / f"{prefix}-source-authored.png") as atlas:
                x, y = row["source_selection"]["variant"] * 128, row["stage"] * 64
                self.assertEqual(self.sprite("construction", row["stage"], row["target_variant"]).tobytes(),
                                 atlas.crop((x, y, x + 64, y + 64)).tobytes())
        for stage in range(4):
            self.assertEqual(self.sprite("construction", stage, 2).tobytes(), self.sprite("construction", stage, 0).tobytes())
            self.assertEqual(self.sprite("construction", stage, 3).tobytes(), self.sprite("construction", stage, 1).tobytes())
        row = copy.deepcopy(next(row for row in reuse["apparatus"] if row["source_family"] == "mine.COAL"))
        row["source_selection"]["variant"] = 2
        with self.assertRaisesRegex(ValueError, "outside exact hopper/belt"):
            plant.compatible_stage_contract(row)

    def test_native_operating_aliases_change_only_original_control_glass(self):
        """Industry stock/work and all apparatus/floors remain exact outside the existing3x3 fitting.

        @param self Active complete electronics works source proof.
        @return None.
        """
        self.assertEqual(self.public["native_state_policy"]["frame_aliases"],
                         {"construction": [0, 1, 2, 3], "ready": [0, 0, 0, 0], "working": [0, 1, 2, 3], "blocked": [0, 0, 0, 0]})
        # Actual operating states may communicate the native result only at
        # the already opaque original fitting, never by painting/removing stock.
        for state in ("ready", "working", "blocked"):
            for frame in range(4):
                for variant in range(16):
                    original, pose = self.sprite("construction", 3, variant), self.sprite(state, frame, variant)
                    for offset, (old, new) in enumerate(zip(original.get_flattened_data(), pose.get_flattened_data())):
                        self.assertEqual(bool(old), bool(new))
                        if not (variant == 11 and 30 <= offset % 64 < 33 and 43 <= offset // 64 < 46):
                            self.assertEqual(old, new)
        for state in ("ready", "blocked"):
            self.assertEqual(len({self.sprite(state, frame, 11).tobytes() for frame in range(4)}), 1)
        self.assertEqual(self.sprite("ready", 0, 11).tobytes(), self.sprite("construction", 3, 11).tobytes())
        self.assertEqual(len({self.sprite("working", frame, 11).tobytes() for frame in range(4)}), 4)
        with Image.open(plant.OUTPUT / f"{plant.PREFIX}-working.gif") as gif:
            self.assertEqual(gif.n_frames, 4)
            self.assertEqual(gif.size, (960, 576))

    def test_missing_duplicate_relabelled_and_foreign_construction_stages_refuse(self):
        """Plausible stage counts or recomputed hashes cannot replace genuine unfinished source ownership.

        @param self Active complete electronics works source proof.
        @return None.
        """
        changed = copy.deepcopy(self.record)
        changed["families"][plant.ASSET]["construction_cells"].pop("2")
        with self.assertRaises(ValueError):
            plant.stage_cells(changed, self.public)
        # Each mutation would otherwise hide a missing real stage behind a
        # valid raw rectangle or seemingly complete native component matrix.
        for key, value in (("construction_stage", 3), ("processor_asset_id", "facility.metal-refinery"), ("unfinished_empty", False)):
            changed = copy.deepcopy(self.record)
            family = changed["families"][plant.ASSET]
            source, _component = plant.mine.component_record(changed, family["construction_cells"]["1"][0])
            source[key] = value
            with self.assertRaises(ValueError):
                plant.stage_cells(changed, self.public)
        for mutation in ("missing", "duplicate", "wrong_component_stage"):
            changed = copy.deepcopy(self.record)
            family = changed["families"][plant.ASSET]
            rows = family["construction_cells"]["1"]
            if mutation == "missing":
                rows.pop()
            elif mutation == "duplicate":
                rows[-1] = rows[0]
            else:
                _source, component = plant.mine.component_record(changed, rows[0])
                component["construction_stage"] = 2
            family["approved_construction_component_geometry_sha256"] = plant.mine.signature(
                {stage: {str(row["variant"]): plant.mine.component_record(changed, row)[1] for row in selections}
                 for stage, selections in family["construction_cells"].items()})
            with self.assertRaises(ValueError):
                plant.stage_cells(changed, self.public)

    def test_changed_original_reference_reuse_or_operating_ownership_refuse(self):
        """Raw/reference mutation and forged compatible roles or enlarged state cues refuse before publication.

        @param self Active complete electronics works source proof.
        @return None.
        """
        for reference in (False, True):
            changed = copy.deepcopy(self.record)
            selection = changed["families"][plant.ASSET]["construction_cells"]["1"][0]
            source, _component = plant.mine.component_record(changed, selection)
            if reference:
                source["references"][0]["sha256"] = "0" * 64
            else:
                source["sha256"] = "0" * 64
            with self.assertRaises(ValueError):
                plant.construction.stage_original(changed, selection, 1, plant.plant.shared.processor_contract(plant.ASSET))
        changed = copy.deepcopy(self.record)
        family = changed["families"][plant.ASSET]
        family["construction_reuse"]["apparatus"][0]["target_variant"] = 6
        family["approved_construction_reuse_sha256"] = plant.mine.signature(family["construction_reuse"])
        with self.assertRaises(ValueError):
            plant.stage_cells(changed, self.public)
        for key, value in (("cue_target", [29, 42, 5, 5]), ("hut_variant", 10), ("component_sha256", "0" * 64)):
            changed = copy.deepcopy(self.record)
            family = changed["families"][plant.ASSET]
            family["operating_component_geometry"][key] = value
            family["approved_operating_component_geometry_sha256"] = plant.mine.signature(family["operating_component_geometry"])
            with self.assertRaises(ValueError):
                plant.operating_lenses(changed, self.public)

    def test_full_public_handoff_refuses_counts_foreign_atlas_missing_rows_and_early_activation(self):
        """Complete source requires its exact live matrix and payload, independently of the32-part base claim.

        @param self Active complete electronics works source proof.
        @return None.
        """
        plant.validate_public_source(self.public, self.report)
        for key, value in (("staged_pose_count", 511), ("nonempty_source_parts", 0), ("native_atlas", "foreign.png"),
                           ("native_sha256", "0" * 64), ("runtime_activated", True)):
            public = copy.deepcopy(self.public)
            public["authoring"][key] = value
            with self.assertRaises(ValueError):
                plant.validate_public_source(public, self.report)
        report = copy.deepcopy(self.report)
        report["source_parts"].pop()
        with self.assertRaises(ValueError):
            plant.validate_public_source(self.public, report)
        public = copy.deepcopy(self.public)
        public["packed"]["offset"] = 10000
        with self.assertRaises(ValueError):
            plant.validate_public_source(public, self.report)
        self.assertTrue(self.family["source_coverage_complete"])
        self.assertFalse(self.public["authoring"]["runtime_activated"])
        self.assertEqual(self.public["authoring"]["selected_base_nonempty_parts"], 32)

    def test_all_prior_raw_native_base_and_other_public_owners_stay_exact(self):
        """All2012 protected files, frozen completed Electronics model and147 other public rows retain exact identities.

        @param self Active complete electronics works source proof.
        @return None.
        """
        allowed = {plant.RECORD.relative_to(ROOT).as_posix(), plant.PUBLIC.relative_to(ROOT).as_posix()}
        # The shared record gains only this family's stages and the public row
        # gains source coverage. Earlier raw/base/payload/app bytes stay exact.
        for row in self.before["files"]:
            if row["path"] not in allowed:
                self.assertEqual(plant.mine.base.digest(ROOT / row["path"]), row["sha256"], row["path"])
        for name, family in self.before["processor_families"].items():
            if name != plant.ASSET:
                self.assertEqual(self.record["families"][name], family, name)
        self.assertEqual(self.record["candidates"][:len(self.before["processor_candidates"])], self.before["processor_candidates"])
        changed_fields = {"status", "candidates", "source_coverage_complete", "remaining"}
        for key, value in self.before["processor_families"][plant.ASSET].items():
            if key not in changed_fields:
                self.assertEqual(self.family[key], value, key)
        protected = {row["asset_id"]: row for row in self.before["public"]["assets"] if row["asset_id"] != plant.ASSET}
        for row in self.document["assets"]:
            if row["asset_id"] != plant.ASSET:
                self.assertEqual(row, protected[row["asset_id"]], row["asset_id"])
        self.assertEqual({key: value for key, value in self.document.items() if key != "assets"},
                         {key: value for key, value in self.before["public"].items() if key != "assets"})


if __name__ == "__main__":
    unittest.main(verbosity=2)
