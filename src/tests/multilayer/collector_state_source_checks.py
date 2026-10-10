"""Verify complete collector source custody across native stages and equipment modes.

@file collector_state_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group54BC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All malformed inputs stay in memory; native work, cargo, Heat and packed assets are untouched.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_geothermal_art as collector


class CollectorStateSourceChecks(unittest.TestCase):
    """Require complete original hardware/glass custody without acquiring gameplay state."""

    @classmethod
    def setUpClass(cls):
        """Cache actual encoded source, its immutable models and independent native assembly.

        @param cls Current focused source/check class.
        @return None; all original files and public contracts remain read-only.
        """
        cls.public_bytes = collector.tube.PUBLIC.read_bytes()
        cls.document = json.loads(collector.RECORD.read_text())
        cls.report = json.loads(collector.COLLECTOR_REPORT.read_text())
        cls.encoded = (ROOT / cls.report["path"]).read_bytes()
        with Image.open(io.BytesIO(cls.encoded)) as atlas:
            cls.atlas = atlas.copy()
        cls.family, cls.spec, cls.component, cls.phases, cls.models = collector.collector_source_inputs(cls.document)
        cls.poses = {}
        # Decode the real atlas once. Every mode/stage/view/part is checked
        # independently of the converter's assembly and partition helpers.
        for state in collector.facilities.STATES:
            for frame in range(4):
                for direction in collector.DIRECTIONS:
                    for part in ("back", "front"):
                        cls.poses[state, frame, direction, part] = cls.assemble_native(state, frame, direction, part)

    @classmethod
    def assemble_native(cls, state, frame, direction, part):
        """Decode native world-cell ownership directly from actual stored PNG sections.

        @param cls Current focused source/check class with a decoded source atlas.
        @param state Actual collector construction or completed presentation mode.
        @param frame Native paid-work quarter or original glass phase.
        @param direction Genuine NE/SE/SW/NW original model view.
        @param part Native aligned back or sortable solid front.
        @return Reconstructed indexed original pose with duplicate ownership refused.
        """
        result = Image.new("P", (256, 192), 0)
        result.putpalette(collector.tube.base.PALETTE_BYTES)
        result.info["transparency"] = 0
        occupied = set()
        count = 0
        # Read actual sprite boxes, then apply native RemapCoords and the
        # published(-31,-33) anchor. No converter owner-selection code is used.
        for row in cls.report["sprites"]:
            if (row["state"], row["frame"], row["direction"], row["part"]) != (state, frame, direction, part):
                continue
            count += 1
            x, y, width, height = row["box"]
            piece = cls.atlas.crop((x, y, x + width, y + height))
            tile_x, tile_y = row["world_cell"]
            ox, oy = 128 + 32 * (tile_y - tile_x) - 31, 64 + 16 * (tile_x + tile_y) - 33
            for ly in range(64):
                for lx in range(64):
                    value = piece.getpixel((lx, ly))
                    if value == 0:
                        continue
                    point = (ox + lx, oy + ly)
                    if point in occupied:
                        raise AssertionError("two native cells duplicate an original pixel")
                    occupied.add(point)
                    result.putpixel(point, value)
        if count != 9:
            raise AssertionError("native collector pose omits a physical industry cell")
        return result

    def test_complete_native_coverage_and_source_encoding(self):
        """Require every real state/stage/view/cell/part and canonical native encoding.

        @param self Current focused source/check instance.
        @return None;1152 unique sprites and exact native source are required.
        """
        self.assertEqual(self.report["pose_count"], 1152)
        collector.validate_collector(self.report, self.encoded, self.document)

    def test_every_non_glass_pixel_and_alpha_remains_original(self):
        """Protect complete hardware/floor material outside reviewed glass interiors.

        @param self Current focused source/check instance.
        @return None; all128 assembled back/front poses preserve their actual original.
        """
        # Restoring only the tiny fitting must recover the complete original
        # byte-for-byte. This catches changed steel, pipes, floor contacts and
        # transparent space even when the altered atlas reports valid hashes.
        for (state, frame, direction, part), pose in self.poses.items():
            key = self.family["construction_frame_sources"][str(frame)][direction] if state == "construction" else self.family["whole_view_selections"][direction]
            original, unused = self.models[key]
            x, y, width, height = self.family["indicator_targets"][key]
            restored = bytearray(pose.tobytes())
            pixels = original.tobytes()
            for dy in range(height):
                for dx in range(width):
                    index = (y + dy) * 256 + x + dx
                    restored[index] = pixels[index]
            self.assertEqual(bytes(restored), pixels, (state, frame, direction, part))
            self.assertEqual(pose.convert("RGBA").getchannel("A").tobytes(), original.convert("RGBA").getchannel("A").tobytes())

    def test_glass_pixels_come_from_the_actual_authored_component(self):
        """Require original amber/green/cyan/red glass rather than procedural status colours.

        @param self Current focused source/check instance.
        @return None; actual sampled component phases occupy only reviewed physical fittings.
        """
        # Compare the encoded result directly to the retained M1 component
        # artwork; construction stage never substitutes for component phase.
        for (state, frame, direction, part), pose in self.poses.items():
            key = self.family["construction_frame_sources"][str(frame)][direction] if state == "construction" else self.family["whole_view_selections"][direction]
            x, y, width, height = self.family["indicator_targets"][key]
            phase = 0 if state == "construction" else frame
            raw = self.phases[collector.facilities.STATES.index(state)][phase].resize((width, height), Image.Resampling.BOX)
            expected = collector.tube.base.quantize(raw, {})
            self.assertEqual(pose.crop((x, y, x + width, y + height)).tobytes(), expected.tobytes(), (state, frame, direction, part))

    def test_completed_phases_and_modes_are_distinct_at_native_scale(self):
        """Reject lost equipment phase/state information after tiny fitting conversion.

        @param self Current focused source/check instance.
        @return None; all completed modes retain four real phases in every view.
        """
        # Static floor cells may repeat; the whole collector must still retain
        # all actual equipment phases and distinguish ready/working/blocked.
        for direction in collector.DIRECTIONS:
            for state in ("ready", "working", "blocked"):
                self.assertEqual(len({self.poses[state, phase, direction, "back"].tobytes() for phase in range(4)}), 4)
            for phase in range(4):
                self.assertEqual(len({self.poses[state, phase, direction, "back"].tobytes() for state in ("ready", "working", "blocked")}), 3)

    def test_solid_fronts_keep_aligned_native_original_sections(self):
        """Preserve sortable collector coverage without inventing a vehicle entrance.

        @param self Current focused source/check instance.
        @return None; every front retains the solid industry's original native pose.
        """
        self.assertEqual(self.report["foreground_policy"], "solid_native_industry_cell_full_original_sections_no_vehicle_aperture")
        for state in collector.facilities.STATES:
            for frame in range(4):
                for direction in collector.DIRECTIONS:
                    self.assertEqual(self.poses[state, frame, direction, "front"].tobytes(), self.poses[state, frame, direction, "back"].tobytes())

    def test_duplicate_pose_cannot_replace_missing_native_coverage(self):
        """Reject padded sprite counts which omit a real physical source pose.

        @param self Current focused source/check instance.
        @return None; unique native identity is required independently of total count.
        """
        report = copy.deepcopy(self.report)
        report["sprites"][-1] = copy.deepcopy(report["sprites"][0])
        with self.assertRaisesRegex(ValueError, "native state/stage/view/cell/part coverage"):
            collector.validate_collector(report, self.encoded, self.document)

    def test_construction_frame_cannot_select_a_blink_phase(self):
        """Keep native paid progress separate from completed equipment phase selection.

        @param self Current focused source/check instance.
        @return None; stage3 still uses genuine construction amber phase0.
        """
        report = copy.deepcopy(self.report)
        row = next(row for row in report["sprites"] if row["state"] == "construction" and row["frame"] == 3)
        row["indicator"]["frame"] = 3
        with self.assertRaisesRegex(ValueError, "false original/frame/equipment provenance"):
            collector.validate_collector(report, self.encoded, self.document)

    def test_false_original_provenance_refuses(self):
        """Reject an unchanged bitmap claiming a different retained original.

        @param self Current focused source/check instance.
        @return None; the actual component/model ancestry is authoritative.
        """
        report = copy.deepcopy(self.report)
        report["sprites"][0]["source_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "false original/frame/equipment provenance"):
            collector.validate_collector(report, self.encoded, self.document)

    def test_report_cannot_publish_another_asset_identity(self):
        """Reject a valid atlas relabelled to bind another family's public source.

        @param self Current focused source/check instance.
        @return None; source publication remains with the actual collector family.
        """
        report = copy.deepcopy(self.report)
        report["asset_id"] = "facility.tube-terminal"
        with self.assertRaisesRegex(ValueError, "family/native canvas identity mismatch"):
            collector.validate_collector(report, self.encoded, self.document)

    def test_custom_input_cannot_hide_changed_construction_registration(self):
        """Require explicit input records to preserve their actual approved original model.

        @param self Current focused source/check instance.
        @return None; disk re-reading cannot conceal malformed isolated source metadata.
        """
        document = copy.deepcopy(self.document)
        key = self.family["construction_frame_sources"]["2"]["SE"]
        next(row for row in document["candidates"] if row["key"] == key)["native_position"][1] += 1
        with self.assertRaisesRegex(ValueError, "construction registration differs"):
            collector.build_collector(document)

    def test_one_changed_floor_pixel_refuses(self):
        """Reject source-payload corruption outside equipment while all provenance still matches.

        @param self Current focused source/check instance.
        @return None; valid metadata cannot legitimize painted original material.
        """
        altered = self.atlas.copy()
        row = self.report["sprites"][0]
        x, y, unused_width, unused_height = row["box"]
        piece = altered.crop((x, y, x + 64, y + 64))
        px, py = next((px, py) for py in range(64) for px in range(64) if piece.getpixel((px, py)) != 0)
        value = altered.getpixel((x + px, y + py))
        altered.putpixel((x + px, y + py), 4 if value != 4 else 5)
        with self.assertRaisesRegex(ValueError, "changes original model/component pixels"):
            collector.validate_collector(self.report, collector.tube.indexed_png(altered), self.document)

    def test_moved_glass_cannot_recolour_adjacent_machinery(self):
        """Reject a valid opaque target moved out of its approved physical glass interior.

        @param self Current focused source/check instance.
        @return None; source approval preserves frame and pipe pixels as well as raw hashes.
        """
        document = copy.deepcopy(self.document)
        key = self.family["whole_view_selections"]["NE"]
        document["families"][0]["indicator_targets"][key][0] += 4
        with self.assertRaisesRegex(ValueError, "equipment registration differs"):
            collector.collector_source_inputs(document)

    def test_glass_cannot_create_opaque_pixels_in_empty_space(self):
        """Reject an insert on transparent space before any native source is partitioned.

        @param self Current focused source/check instance.
        @return None; equipment cannot enlarge the original silhouette or floor footprint.
        """
        original, unused = self.models[self.family["whole_view_selections"]["SE"]]
        with self.assertRaisesRegex(ValueError, "fitting is not opaque"):
            collector.insert_collector_glass(original, self.phases[1][0], [0, 0, 1, 2])

    def test_transparent_component_refuses(self):
        """Reject an incomplete donor instead of cutting a hole in a real glass fitting.

        @param self Current focused source/check instance.
        @return None; original source alpha remains unchanged in all modes.
        """
        key = self.family["whole_view_selections"]["SE"]
        original, unused = self.models[key]
        with self.assertRaisesRegex(ValueError, "indicator glass is transparent"):
            collector.insert_collector_glass(original, Image.new("RGBA", (3, 3)), self.family["indicator_targets"][key])

    def test_source_checks_preserve_public_metadata_and_runtime_authority(self):
        """Keep read-only source validation separate from later packing and actual native use.

        @param self Current focused source/check instance.
        @return None; an authored source report does not activate an operating plant.
        """
        self.assertEqual(collector.tube.PUBLIC.read_bytes(), self.public_bytes)
        self.assertTrue(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])


if __name__ == "__main__":
    unittest.main()
