"""Verify genuine natural geothermal overlays and absence through unsurveyed solid ground.

@file vent_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group54C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed inputs stay in memory; geological capacity, paid terrain and native survey remain read-only.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, patch

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_vent_art as vent


class VentSourceChecks(unittest.TestCase):
    """Require source visibility and exact original custody without acquiring thermal rights."""

    @classmethod
    def setUpClass(cls):
        """Cache actual native source proof and its immutable original/terrain context.

        @param cls Current focused source/check class.
        @return None; retained/public files and all native owners remain unchanged.
        """
        cls.document_bytes = vent.RECORD.read_bytes()
        cls.public_bytes = vent.tube.PUBLIC.read_bytes()
        cls.document = json.loads(cls.document_bytes)
        cls.public = json.loads(cls.public_bytes)
        cls.family, cls.source, cls.spec = vent.source_inputs(cls.document)
        cls.report = json.loads(vent.TARGET.read_text())
        cls.encoded = (ROOT / cls.report["path"]).read_bytes()
        with Image.open(io.BytesIO(cls.encoded)) as image:
            cls.atlas = image.copy()
        with Image.open(ROOT / cls.source["path"]) as image:
            cls.original = image.convert("RGBA")
        hardness = next(row for row in cls.public["assets"] if row["asset_id"] == "terrain.hardness")
        with Image.open(ROOT / hardness["authoring"]["native_atlas"]) as image:
            cls.floors = image.copy()

    def sprite(self, state, frame):
        """Decode one actual native overlay from the complete source atlas.

        @param self Current focused source/check instance.
        @param state Hidden, surveyed or paid-excavated source presentation.
        @param frame Authored visual detail phase0..3.
        @return Indexed64x31 actual source pose with original transparency metadata.
        """
        y = vent.STATES.index(state) * 31
        return self.atlas.crop((frame * 64, y, (frame + 1) * 64, y + 31))

    def test_complete_original_native_state_phase_coverage(self):
        """Require all twelve unique native state/phase identities and exact encoding.

        @param self Current focused source/check instance.
        @return None; source validation does not establish runtime rendering.
        """
        vent.validate(self.report, self.encoded, self.document)
        self.assertEqual(self.report["pose_count"], 12)
        self.assertFalse(self.report["runtime_activated"])

    def test_hidden_original_padding_and_all_frames_have_zero_alpha(self):
        """Require genuine absent pixels instead of an unknown-source badge.

        @param self Current focused source/check instance.
        @return None; unsurveyed solid ground contains no field-specific source information.
        """
        self.assertEqual(self.original.crop(self.source["hidden_crop"]).getchannel("A").getextrema(), (0, 0))
        for frame in range(4):
            self.assertIsNone(self.sprite("hidden", frame).getbbox())
            self.assertEqual(self.sprite("hidden", frame).convert("RGBA").getchannel("A").getextrema(), (0, 0))

    def test_visible_pixels_come_from_actual_original_crops(self):
        """Protect genuine surveyed information and exposed fissures from procedural substitution.

        @param self Current focused source/check instance.
        @return None; every visible native pixel follows its retained original registration.
        """
        # Recreate native sampling directly from the immutable original, not
        # through the converter's build or source-selection implementation.
        for state in ("surveyed", "excavated"):
            for frame in range(4):
                crop = self.original.crop(self.source["native_crop_boxes"][state][frame])
                expected = Image.new("RGBA", (64, 31))
                expected.paste(crop.resize(tuple(self.source["native_sample_size"]), Image.Resampling.BOX), tuple(self.source["native_position"]))
                native = vent.tube.base.quantize(expected, {})
                self.assertEqual(self.sprite(state, frame).tobytes(), native.tobytes())

    def test_all_visible_original_phases_remain_distinct(self):
        """Reject palette reduction losing a genuinely authored thermal detail phase.

        @param self Current focused source/check instance.
        @return None; both visible modes retain four original phase signatures.
        """
        for state in ("surveyed", "excavated"):
            self.assertEqual(len({self.sprite(state, frame).tobytes() for frame in range(4)}), 4)

    def test_hidden_overlay_preserves_every_existing_hardness_floor(self):
        """Ensure hidden information looks exactly like ordinary paid rock in every hardness.

        @param self Current focused source/check instance.
        @return None; original geology appearance cannot reveal membership indirectly.
        """
        # The same absent overlay must preserve each actual existing native
        # floor; a basalt replacement would disclose a field on softer geology.
        for rock in range(4):
            floor = self.floors.crop((192, rock * 31, 256, (rock + 1) * 31)).convert("RGBA")
            for frame in range(4):
                self.assertEqual(Image.alpha_composite(floor, self.sprite("hidden", frame).convert("RGBA")).tobytes(), floor.tobytes())

    def test_visible_detail_preserves_floor_outside_its_original_alpha(self):
        """Keep ordinary hardness under sparse natural detail instead of replacing a whole tile.

        @param self Current focused source/check instance.
        @return None; all four terrain materials retain their original surrounding pixels.
        """
        # Every zero-alpha point stays the original floor pixel. Mineral marks
        # have their independent existing layer and later native draw ordering.
        for state in ("surveyed", "excavated"):
            for frame in range(4):
                overlay = self.sprite(state, frame).convert("RGBA")
                for rock in range(4):
                    floor = self.floors.crop((192, rock * 31, 256, (rock + 1) * 31)).convert("RGBA")
                    composed = Image.alpha_composite(floor, overlay)
                    for original, detail, actual in zip(floor.get_flattened_data(), overlay.get_flattened_data(), composed.get_flattened_data()):
                        if detail[3] == 0:
                            self.assertEqual(actual, original)

    def test_visibility_precedence_does_not_open_unsurveyed_ground(self):
        """Keep actual field membership and paid excavation ahead of source information.

        @param self Current focused source/check instance.
        @return None; preview state choice does not grant terrain, field identity or Heat.
        """
        # Existing owners supply these facts; neither a visual phase nor a
        # downstream plant's demand participates in the source-preview decision.
        for in_field in (False, True):
            for paid in (False, True):
                for survey in (False, True):
                    expected = None if not in_field else "excavated" if paid else "surveyed" if survey else "hidden"
                    self.assertEqual(vent.select_vent_source_state(in_field, paid, survey), expected)

    def test_hidden_source_pixel_leak_refuses(self):
        """Reject one visible hidden-field pixel even with unchanged source metadata.

        @param self Current focused source/check instance.
        @return None; hidden cannot reveal a natural field through solid rock.
        """
        image = self.atlas.copy()
        image.putpixel((31, 15), 4)
        with self.assertRaisesRegex(ValueError, "hidden natural vent reveals"):
            vent.validate(self.report, vent.tube.indexed_png(image), self.document)

    def test_opaque_replacement_ground_refuses(self):
        """Reject a full ground tile replacing the geology owner's ordinary floor.

        @param self Current focused source/check instance.
        @return None; this family supplies only sparse natural information/detail.
        """
        image = self.atlas.copy()
        image.paste(4, (0, 31, 64, 62))
        with self.assertRaisesRegex(ValueError, "replacement terrain"):
            vent.validate(self.report, vent.tube.indexed_png(image), self.document)

    def test_missing_phase_cannot_be_padded_with_hidden(self):
        """Reject a repeated hidden pose replacing one genuine visible source phase.

        @param self Current focused source/check instance.
        @return None; unique native coverage is required independently of pose count.
        """
        report = copy.deepcopy(self.report)
        report["sprites"][-1] = copy.deepcopy(report["sprites"][0])
        with self.assertRaisesRegex(ValueError, "state/phase coverage mismatch"):
            vent.validate(report, self.encoded, self.document)

    def test_moved_native_anchor_refuses(self):
        """Keep the source ground anchor separate from the existing20x20 mineral marker offset.

        @param self Current focused source/check instance.
        @return None; native tile detail cannot drift into another world cell.
        """
        report = copy.deepcopy(self.report)
        report["sprites"][4]["anchor"] = [-10, 6]
        with self.assertRaisesRegex(ValueError, "native pose geometry mismatch"):
            vent.validate(report, self.encoded, self.document)

    def test_changed_original_registration_refuses(self):
        """Reject an unchanged raw image sampled through a moved source crop.

        @param self Current focused source/check instance.
        @return None; original SHA alone cannot authorize a different native presentation.
        """
        document = copy.deepcopy(self.document)
        source = next(row for row in document["candidates"] if row["key"] == self.source["key"])
        source["native_crop_boxes"]["surveyed"][0][1] += 1
        with self.assertRaisesRegex(ValueError, "registration differs"):
            vent.build(document)

    def test_even_approved_hidden_crop_cannot_contain_visible_original(self):
        """Require opacity absence independently of a mistakenly accepted registration hash.

        @param self Current focused source/check instance.
        @return None; source approval cannot waive hidden-field visibility rules.
        """
        document = copy.deepcopy(self.document)
        source = next(row for row in document["candidates"] if row["key"] == self.source["key"])
        source["hidden_crop"] = source["native_crop_boxes"]["excavated"][0]
        public = copy.deepcopy(self.public)
        spec = next(row for row in public["assets"] if row["asset_id"] == "geothermal.vent")
        spec["geometry_notes"]["approved_natural_vent_source_sha256"] = vent.registration_signature(document)
        with patch.object(vent.tube, "PUBLIC", Mock(read_text=Mock(return_value=json.dumps(public)))):
            with self.assertRaisesRegex(ValueError, "hidden natural vent reveals original pixels"):
                vent.build(document)

    def test_copied_heat_consumer_state_refuses(self):
        """Reject natural geology becoming a downstream Heat consumer or tapped machine.

        @param self Current focused source/check instance.
        @return None; the source's immutable capacity keeps its single native owner.
        """
        document = copy.deepcopy(self.document)
        next(row for row in document["families"] if row["asset_id"] == "geothermal.vent")["states"].append("heat_starved")
        with self.assertRaisesRegex(ValueError, "native visibility/field geometry mismatch"):
            vent.build(document)

    def test_false_source_provenance_and_changed_pixels_refuse(self):
        """Reject a valid-looking overlay with false original custody or edited detail.

        @param self Current focused source/check instance.
        @return None; metadata and actual encoded pixels must both match the original.
        """
        report = copy.deepcopy(self.report)
        report["sprites"][4]["source_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "false original/visibility provenance"):
            vent.validate(report, self.encoded, self.document)
        image = self.atlas.copy()
        x, y = next((x, y) for y in range(31, 62) for x in range(64) if image.getpixel((x, y)) != 0)
        value = image.getpixel((x, y))
        image.putpixel((x, y), 4 if value != 4 else 5)
        with self.assertRaisesRegex(ValueError, "changes original source pixels"):
            vent.validate(self.report, vent.tube.indexed_png(image), self.document)

    def test_exact_regeneration_does_not_mutate_source_public_or_runtime(self):
        """Verify atlas/mapping/review regeneration while every native owner remains untouched.

        @param self Current focused source/check instance.
        @return None; source validation is read-only and does not establish native rendering.
        """
        vent.publish(check=True)
        self.assertEqual(vent.RECORD.read_bytes(), self.document_bytes)
        self.assertEqual(vent.tube.PUBLIC.read_bytes(), self.public_bytes)
        self.assertFalse(self.report["runtime_activated"])


if __name__ == "__main__":
    unittest.main()
