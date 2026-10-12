"""Focused source proof for the currently executing selected material-mine group.

@file material_mine_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups28/29/31/32/34/35.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
MLM_MINE_SOURCE_RESOURCE selects one active original/native family; this runner
does not regress all mines or imply native packing/game acceptance.
"""

import copy
import io
import json
import os
from pathlib import Path
import sys
import unittest

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests"))
import convert_material_mine_art as material
import multilayer_sprite_contract as shared


class MaterialMineSourceChecks(unittest.TestCase):
    """Prove genuine current-mineral originals, compatible native reuse and source-only publication."""

    @classmethod
    def setUpClass(cls):
        """Regenerate only the executing mineral family from exact retained original ancestry.

        @param cls Active source class retaining one complete288-slot native source handoff.
        @return None.
        """
        cls.resource = os.environ.get("MLM_MINE_SOURCE_RESOURCE", "IORE")
        cls.record = json.loads(material.mine.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(material.mine.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == "mine." + cls.resource)
        cls.family = cls.record["families"][cls.public["asset_id"]]
        cls.name = material.prefix(cls.public)
        cls.report, cls.files = material.build(cls.record, cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[material.OUTPUT / f"{cls.name}-source-authored.png"]))
        cls.before = json.loads((ROOT / cls.family["verification_baseline"]).read_text(encoding="utf-8"))

    def sprite(self, state, frame, variant, part=0):
        """Read one exact actual native source owner without flattening neighboring roles.

        @param self Active selected material-mine source proof.
        @param state Native construction/working/blocked/depleted presentation state.
        @param frame Actual native construction or synchronized working phase0..3.
        @param variant Actual native row-major machinery cell0..8.
        @param part Full original underlay0 or independent upper-front1.
        @return Indexed64x64 native source sprite at the common anchor.
        """
        x, y = (variant * 2 + part) * 64, (material.STATES.index(state) * 4 + frame) * 64
        return self.atlas.crop((x, y, x + 64, y + 64))

    def test_exact_regeneration_and_all_288_real_native_source_owners(self):
        """Actual native cell/state coverage regenerates exactly with all18 completed base parts.

        @param self Active selected material-mine source proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        expected = {(state, frame, variant, part, variant % 3, variant // 3) for state in material.STATES for frame in range(4) for variant in range(9) for part in ("back", "front")}
        actual = {(row["state"], row["frame"], row["tile_variant"], row["part"], *row["world_cell"]) for row in self.report["source_parts"]}
        self.assertEqual(actual, expected)
        self.assertEqual(len(self.report["source_parts"]), 288)
        self.assertTrue(all(row["nonempty_pixels"] > 0 and not row["structurally_absent"] for row in self.report["source_parts"]))
        self.assertEqual(self.atlas.size, (1152, 1024))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertIn("r2mlvs14", self.atlas.info["Owner"])
        with Image.open(material.OUTPUT / f"{self.name}-base-atlas.png") as base:
            self.assertEqual(self.atlas.crop((0, 192, 1152, 256)).tobytes(), base.tobytes())
        self.assertEqual(len({self.sprite("construction", 3, variant).tobytes() for variant in range(9)}), 9)

    def test_genuine_current_mineral_originals_and_safe_whole_native_registration(self):
        """Cargo-bearing completed apparatus retains current-mineral ancestry and complete clean crops.

        @param self Active selected material-mine source proof.
        @return None.
        """
        for selection in self.family["base_cells"]:
            source, component = material.mine.component_record(self.record, selection)
            if selection["variant"] != 8:
                self.assertEqual(source["mineral_resource"], self.resource)
            self.assertEqual(material.mine.base.digest(ROOT / source["path"]), source["sha256"])
            with Image.open(ROOT / source["path"]) as raw:
                crop = raw.convert("RGBA").crop(component["source_window"])
            bounds = crop.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
            self.assertEqual(list(bounds), component["measured_alpha16_bbox"])
            left, top, right, bottom = bounds
            self.assertGreaterEqual(min(left, top, crop.width - right, crop.height - bottom), 16)
            self.assertFalse(any(alpha >= 16 and red > 220 and green < 60 and blue > 220 for red, green, blue, alpha in crop.get_flattened_data()))
            x0, y0, x1, y1 = component["native_box"]
            self.assertEqual((x0, x1, y1), (1, 63, 64))
            self.assertLess(abs(((x1 - x0) / (right - left)) / ((y1 - y0) / (bottom - top)) - 1), .02)

    def test_compatible_empty_construction_and_no_cargo_components_keep_exact_ancestry(self):
        """Each reused physical role comes from its genuine component, while completed cargo is mineral-specific.

        @param self Active selected material-mine source proof.
        @return None.
        """
        material.validate_reuse(self.record, self.family, self.public)
        with Image.open(material.OUTPUT / "coal-mine-construction-atlas.png") as donor:
            self.assertEqual(self.atlas.crop((0, 0, 1152, 192)).tobytes(), donor.crop((0, 0, 1152, 192)).tobytes())
        with Image.open(material.OUTPUT / "coal-mine-base-atlas.png") as donor:
            for variant in (7, 8):
                x = variant * 128
                self.assertEqual(self.sprite("construction", 3, variant).tobytes(), donor.crop((x, 0, x + 64, 64)).tobytes())
            # New mineral cargo cannot be a renamed Coal completed original.
            # Every actual cargo-bearing role differs, preserving genuine identity.
            for variant in (0, 1, 2, 3, 4, 5, 6):
                x = variant * 128
                self.assertNotEqual(self.sprite("construction", 3, variant).tobytes(), donor.crop((x, 0, x + 64, 64)).tobytes(), variant)

    def test_all_cells_keep_native_contacts_height_palette_and_original_upper_parts(self):
        """Every state stays inside paid machinery/sorting geometry and independently owned source parts.

        @param self Active selected material-mine source proof.
        @return None.
        """
        for state in material.STATES:
            for frame in range(4):
                for variant in range(9):
                    back, front = self.sprite(state, frame, variant), self.sprite(state, frame, variant, 1)
                    material.mine.validate_cell(back, variant)
                    self.assertIsNotNone(front.getbbox())
                    self.assertTrue(all(new == 0 or new == old for old, new in zip(back.get_flattened_data(), front.get_flattened_data())))
        self.assertEqual(self.report["loading_cells"], [[0, 3], [1, 3], [0, 4], [1, 4]])

    def test_operating_pause_depletion_keep_all_apparatus_ground_and_final_output(self):
        """Only the genuine control fitting changes; depletion never erases final stored mineral cargo.

        @param self Active selected material-mine source proof.
        @return None.
        """
        allowed = {y * 64 + x for y in range(43, 46) for x in range(30, 33)}
        # Check independent parts across every completed source pose, so a
        # hidden cargo/alpha change cannot be concealed by the review campus.
        for state in ("working", "blocked", "depleted"):
            for frame in range(4):
                for variant in range(9):
                    for part in range(2):
                        before, after = self.sprite("construction", 3, variant, part), self.sprite(state, frame, variant, part)
                        changed = {i for i, (old, new) in enumerate(zip(before.get_flattened_data(), after.get_flattened_data())) if old != new}
                        self.assertLessEqual(changed, allowed if variant == 7 else set())
                        self.assertEqual([bool(v) for v in before.get_flattened_data()], [bool(v) for v in after.get_flattened_data()])
        self.assertEqual(len({self.sprite("working", frame, 7).tobytes() for frame in range(4)}), 4)
        for state in ("blocked", "depleted"):
            self.assertEqual(len({self.sprite(state, frame, 7).tobytes() for frame in range(4)}), 1)
        with Image.open(io.BytesIO(self.files[material.OUTPUT / f"{self.name}-source-working.gif"])) as gif:
            self.assertEqual(gif.n_frames, 4)
            self.assertEqual(gif.size, (768, 480))
            self.assertIn(b"r2mlvs14", gif.info["comment"])

    def test_full_public_source_binds_live_atlas_without_packed_or_runtime_claim(self):
        """The complete existing public handoff identifies genuine source while later game work stays pending.

        @param self Active selected material-mine source proof.
        @return None.
        """
        material.validate_public_source(self.public, self.report)
        self.assertTrue(material.validate_following_source(self.public))
        self.assertEqual(shared.validate_manifest(self.document), 48893)
        self.assertTrue(self.public["authoring"]["coverage_complete"])
        self.assertFalse(self.public["authoring"]["runtime_activated"])
        self.assertIsNone(self.public["packed"]["offset"])
        self.assertIsNone(self.public["packed"]["sha256"])

    def test_missing_duplicate_changed_registration_and_relabelled_mineral_refuse(self):
        """A valid-looking native sprite cannot replace a missing or wrong-mineral completed original.

        @param self Active selected material-mine source proof.
        @return None.
        """
        for change in ("missing", "duplicate", "registration", "mineral", "original", "reference"):
            record = copy.deepcopy(self.record)
            family = record["families"][self.public["asset_id"]]
            source, component = material.mine.component_record(record, family["base_cells"][0])
            if change == "missing":
                family["base_cells"].pop()
            elif change == "duplicate":
                family["base_cells"][-1] = copy.deepcopy(family["base_cells"][0])
            elif change == "registration":
                component["native_box"][0] += 1
            elif change == "mineral":
                source["mineral_resource"] = "COAL"
            elif change == "original":
                source["sha256"] = "0" * 64
            else:
                source["references"][0]["sha256"] = "0" * 64
            with self.subTest(change=change), self.assertRaises(ValueError):
                material.base_cells(record, family, self.public)

    def test_changed_foreign_stage_and_wrong_physical_reuse_refuse_even_with_new_caption_hash(self):
        """Reuse needs identical actual physical roles, original stages and fixed native contacts.

        @param self Active selected material-mine source proof.
        @return None.
        """
        for change in ("foreign_family", "roles", "stage", "geometry", "hut", "loading"):
            record = copy.deepcopy(self.record)
            family = record["families"][self.public["asset_id"]]
            reuse = family["compatible_components"]
            if change == "foreign_family":
                reuse["source_family"] = "facility.earth-receiver"
            elif change == "roles":
                reuse["physical_roles"] = []
            elif change == "stage":
                reuse["empty_construction_cells"]["1"].pop()
            elif change == "geometry":
                reuse["anchor"][0] += 1
            elif change == "hut":
                family["control_reuse"]["source_variant"] = 3
            else:
                family["loading_cells"][0] = [2, 2]
            family["approved_compatible_components_sha256"] = material.mine.signature(reuse)
            with self.subTest(change=change), self.assertRaises(ValueError):
                material.validate_reuse(record, family, self.public)

    def test_false_complete_foreign_atlas_and_runtime_or_packed_claims_refuse(self):
        """Exact live payload evidence prevents missing coverage, forged reports and premature activation.

        @param self Active selected material-mine source proof.
        @return None.
        """
        for field, value in (("coverage_complete", False), ("native_sha256", "0" * 64), ("staged_pose_count", 72),
                             ("nonempty_source_parts", 18), ("runtime_activated", True)):
            public = copy.deepcopy(self.public)
            public["authoring"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                material.validate_public_source(public, self.report)
        for change in ("foreign_report", "forged_live_sha", "packed", "native_geometry"):
            public, report = copy.deepcopy(self.public), copy.deepcopy(self.report)
            if change == "foreign_report":
                report["asset_id"] = "mine.COAL"
            elif change == "forged_live_sha":
                report["source_sha256"] = public["authoring"]["native_sha256"] = "0" * 64
            elif change == "packed":
                public["packed"]["offset"] = 3588
            else:
                public["native_cells"][-1] = [0, 0]
            with self.subTest(change=change), self.assertRaises(ValueError):
                material.validate_public_source(public, report)

    def test_older_fixture_handoff_accepts_only_genuine_complete_selected_sources(self):
        """Older preservation checks keep pending/foreign rows exact and refuse false later delivery.

        @param self Active selected material-mine source proof.
        @return None.
        """
        pending = copy.deepcopy(self.public)
        pending["authoring"]["coverage_complete"] = False
        self.assertFalse(material.validate_following_source(pending))
        foreign = copy.deepcopy(self.public)
        foreign["asset_id"] = "mine.URAN"
        self.assertFalse(material.validate_following_source(foreign))
        invalid = copy.deepcopy(self.public)
        invalid["authoring"]["native_sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            material.validate_following_source(invalid)

    def test_prior_coal_native_packed_other_public_and_original_identities_stay_exact(self):
        """New material source changes only its own metadata plus explicit focused-test handoff updates.

        @param self Active selected material-mine source proof.
        @return None.
        """
        approved = {row["path"]: row for row in self.before.get("approved_boundary_updates", [])}
        # Preserve every historical identity, recording any necessary focused
        # boundary-code update separately rather than silently replacing baseline.
        for row in self.before["preserved_identities"]:
            expected = approved.get(row["path"], row)
            self.assertEqual(material.mine.base.digest(ROOT / row["path"]), expected["sha256"], row["path"])
        self.assertEqual(material.mine.signature(self.record["families"]["mine.COAL"]), self.before["coal_family_sha256"])
        # Already delivered predecessor families stay exact during the current
        # source group; later publication belongs only to its own family owner.
        for asset_id, expected in self.before.get("previous_family_sha256", {}).items():
            self.assertEqual(material.mine.signature(self.record["families"][asset_id]), expected, asset_id)
        for source in self.record["candidates"]:
            if source["id"] in self.before["coal_candidate_sha256"]:
                self.assertEqual(material.mine.signature(source), self.before["coal_candidate_sha256"][source["id"]], source["id"])
        for row in self.document["assets"]:
            if row["asset_id"] == self.public["asset_id"]:
                continue
            actual = material.mine.signature(row)
            expected = self.before["public_family_sha256"][row["asset_id"]]
            # An older resource fixture may observe a subsequent selected
            # family's completed source. Only exact live native/payload proof
            # can supersede that family's formerly planned metadata.
            if actual != expected and material.validate_following_source(row):
                continue
            self.assertEqual(actual, expected, row["asset_id"])


if __name__ == "__main__":
    unittest.main()
