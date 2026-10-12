"""Focused complete Coal source and native state-owner checks.

@file mine_state_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group27D/task27.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Complete source coverage leaves packed/native game acceptance with master10.4D.
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
sys.path.insert(0, str(ROOT / "src/tests"))
import convert_mine_states as states
import convert_material_mine_art as material
import multilayer_sprite_contract as shared


class MineStateSourceChecks(unittest.TestCase):
    """Require full actual stage/state coverage, exact retained material and source-only publication."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the full active Coal source once using current original/native owners.

        @param cls Active class retaining the288-slot source and previous delivery identities.
        @return None.
        """
        cls.record = json.loads(states.mine.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(states.mine.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == "mine.COAL")
        cls.report, cls.files = states.build(cls.record, cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[states.OUTPUT / "coal-mine-source-authored.png"]))
        cls.before = json.loads((ROOT / "logs/graphics/coal-mine/20261012-states-01/before.json").read_text(encoding="utf-8"))

    def sprite(self, state, frame, variant, part=0):
        """Read one exact native state/phase/cell/source-part owner.

        @param self Active full-source proof.
        @param state Actual mine construction/working/blocked/depleted state.
        @param frame Native construction or existing synchronized presentation phase0..3.
        @param variant Actual machinery cell0..8, never a loading-floor cell.
        @param part Back0 or independently sorted original upper-front1.
        @return Indexed native64x64 source sprite.
        """
        x, y = (variant * 2 + part) * 64, (states.STATES.index(state) * 4 + frame) * 64
        return self.atlas.crop((x, y, x + 64, y + 64))

    def test_exact_regeneration_and_all_288_actual_native_source_owners(self):
        """Complete native source coverage is exact, independently owned and source-only.

        @param self Active full-source proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        self.assertEqual(self.atlas.size, (1152, 1024))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertIn("r2mlvs14", self.atlas.info["Owner"])
        expected = {(state, frame, variant, part, variant % 3, variant // 3) for state in states.STATES for frame in range(4) for variant in range(9) for part in ("back", "front")}
        actual = {(row["state"], row["frame"], row["tile_variant"], row["part"], *row["world_cell"]) for row in self.report["source_parts"]}
        self.assertEqual(actual, expected)
        self.assertEqual(len(self.report["source_parts"]), 288)
        self.assertTrue(all(row["nonempty_pixels"] > 0 and not row["structurally_absent"] for row in self.report["source_parts"]))
        self.assertEqual(shared.validate_manifest(self.document), 48893)
        self.assertTrue(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])

    def test_every_construction_part_is_the_approved_stage_source_exactly(self):
        """The full source handoff cannot substitute generic machinery for any actual construction phase.

        @param self Active full-source proof.
        @return None.
        """
        with Image.open(states.OUTPUT / "coal-mine-construction-atlas.png") as approved:
            self.assertEqual(self.atlas.crop((0, 0, 1152, 256)).tobytes(), approved.tobytes())

    def test_operating_paused_and_depleted_preserve_all_apparatus_floor_and_coal(self):
        """Only original control-hut glass changes; depletion retains final physical output identity.

        @param self Active full-source proof.
        @return None.
        """
        allowed = {y * 64 + x for y in range(43, 46) for x in range(30, 33)}
        # Inspect every independent native part in every completed state, so a
        # hidden cargo erasure cannot be concealed by the assembled contact sheet.
        for state in ("working", "blocked", "depleted"):
            for frame in range(4):
                for variant in range(9):
                    for part in range(2):
                        ready = self.sprite("construction", 3, variant, part)
                        current = self.sprite(state, frame, variant, part)
                        changed = {i for i, (old, new) in enumerate(zip(ready.get_flattened_data(), current.get_flattened_data())) if old != new}
                        if variant == 7:
                            self.assertLessEqual(changed, allowed)
                        else:
                            self.assertFalse(changed, (state, frame, variant, part))
                        self.assertEqual([bool(v) for v in ready.get_flattened_data()], [bool(v) for v in current.get_flattened_data()])

    def test_working_has_four_original_phases_and_pause_depletion_are_static(self):
        """Native state policy keeps real construction phases and synchronized working presentation distinct.

        @param self Active full-source proof.
        @return None.
        """
        self.assertEqual(len({self.sprite("working", frame, 7).tobytes() for frame in range(4)}), 4)
        for state in ("blocked", "depleted"):
            self.assertEqual(len({self.sprite(state, frame, 7).tobytes() for frame in range(4)}), 1)
            self.assertEqual(self.public["native_state_policy"]["frame_aliases"][state], [0, 0, 0, 0])
        self.assertNotEqual(self.sprite("blocked", 0, 7).tobytes(), self.sprite("depleted", 0, 7).tobytes())
        lenses = states.original_lenses(self.record)
        self.assertTrue(all(max(pixel[:3]) - min(pixel[:3]) <= 12 for pixel in lenses["depleted"][0].get_flattened_data()))
        self.assertNotEqual(lenses["depleted"][0].tobytes(), lenses["blocked"][0].tobytes())

    def test_all_native_cells_parts_palette_and_source_comparison_frames(self):
        """All state poses retain their physical sorting geometry and reproducible four-phase comparison.

        @param self Active full-source proof.
        @return None.
        """
        for state in states.STATES:
            for frame in range(4):
                for variant in range(9):
                    states.mine.validate_cell(self.sprite(state, frame, variant), variant)
                    back, front = self.sprite(state, frame, variant), self.sprite(state, frame, variant, 1)
                    self.assertTrue(all(new == 0 or new == old for old, new in zip(back.get_flattened_data(), front.get_flattened_data())))
        with Image.open(io.BytesIO(self.files[states.OUTPUT / "coal-mine-source-working.gif"])) as gif:
            self.assertEqual(gif.n_frames, 4)
            self.assertEqual(gif.size, (768, 480))
            self.assertIn(b"r2mlvs14", gif.info["comment"])

    def test_complete_public_source_binds_exact_atlas_without_runtime_or_packed_range(self):
        """The public family exposes usable full source while native rendering stays with its later owner.

        @param self Active full-source proof.
        @return None.
        """
        states.validate_public_source(self.public, self.report)
        self.assertTrue(self.public["authoring"]["coverage_complete"])
        self.assertEqual(self.public["authoring"]["staged_pose_count"], 288)
        self.assertIsNone(self.public["packed"]["offset"])
        self.assertIsNone(self.public["packed"]["sha256"])
        self.assertFalse(self.public["authoring"]["runtime_activated"])
        self.assertEqual(states.mine.base.digest(ROOT / self.report["source"]), self.report["source_sha256"])

    def test_changed_off_original_reference_fitting_and_state_ancestry_refuse(self):
        """A missing/changed genuine off component cannot be replaced with a relabelled working lamp.

        @param self Active full-source proof.
        @return None.
        """
        for change in ("original", "reference", "fitting", "state", "glass_window", "m1_original", "rejected"):
            record = copy.deepcopy(self.record)
            family = record["families"]["mine.COAL"]
            geometry = family["operating_component_geometry"]
            source, component = states.mine.component_record(record, geometry["depleted_selection"])
            if change == "original":
                source["sha256"] = "0" * 64
            elif change == "reference":
                source["references"][0]["sha256"] = "0" * 64
            elif change == "fitting":
                geometry["cue_target"] = [29, 43, 3, 3]
            elif change == "state":
                component["state"] = "working"
            elif change == "glass_window":
                component["glass_window"] = [0, 0, 1254, 1254]
            elif change == "m1_original":
                geometry["m1_component_sha256"] = "0" * 64
            else:
                source["selection"]["status"] = "rejected"
            family["approved_operating_component_geometry_sha256"] = states.mine.signature({"geometry": geometry, "off_component": component})
            with self.subTest(change=change), self.assertRaises(ValueError):
                states.original_lenses(record)

    def test_false_complete_public_claims_missing_native_stages_and_packed_activation_refuse(self):
        """Source acceptance requires exact ancestry/geometry/counts and cannot allocate native sprites.

        @param self Active full-source proof.
        @return None.
        """
        for field, value in (("coverage_complete", False), ("native_sha256", "0" * 64), ("staged_pose_count", 72),
                             ("nonempty_source_parts", 18), ("runtime_activated", True), ("atlas_manifest", "wrong.json")):
            public = copy.deepcopy(self.public)
            public["authoring"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                states.validate_public_source(public, self.report)
        for change in ("packed", "native_geometry", "frame_owner"):
            public = copy.deepcopy(self.public)
            if change == "packed":
                public["packed"]["offset"] = 3588
            elif change == "native_geometry":
                public["tile_variants"] = 4
            else:
                public["native_state_policy"]["frame_aliases"]["depleted"] = [0, 1, 2, 3]
            with self.subTest(change=change), self.assertRaises(ValueError):
                states.validate_public_source(public, self.report)
        # Matching forged metadata cannot replace the actual generated atlas:
        # validate its live payload as well as the expected family/slot contract.
        for field, value in (("asset_id", "mine.IORE"), ("source_slots", 72), ("source_sha256", "0" * 64)):
            report = copy.deepcopy(self.report)
            public = copy.deepcopy(self.public)
            report[field] = value
            if field == "source_sha256":
                public["authoring"]["native_sha256"] = value
            with self.subTest(report_field=field), self.assertRaises(ValueError):
                states.validate_public_source(public, report)

    def test_every_prior_base_construction_packed_binary_and_other_public_identity_stays_exact(self):
        """Full Coal source alters only its own public delivery metadata and new source artifacts.

        @param self Active full-source proof.
        @return None.
        """
        for row in self.before["preserved_identities"]:
            self.assertEqual(states.mine.base.digest(ROOT / row["path"]), row["sha256"], row["path"])
        # Coal publication and subsequent validated selected material sources
        # own their metadata. Every other family remains byte-exact to baseline.
        for row in self.document["assets"]:
            if row["asset_id"] == "mine.COAL":
                continue
            if material.validate_following_source(row):
                continue
            self.assertEqual(states.mine.signature(row), self.before["public_family_sha256"][row["asset_id"]], row["asset_id"])
        family = self.record["families"]["mine.COAL"]
        self.assertEqual(family["approved_construction_component_geometry_sha256"], self.before["construction_component_geometry_sha256"])
        self.assertEqual(family["approved_construction_reuse_sha256"], self.before["construction_reuse_sha256"])


if __name__ == "__main__":
    unittest.main()
