"""Verify genuine collector views and lossless nine-cell native source ownership.

@file collector_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups54B.2/54BB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed originals/registrations remain in memory; native industry and Heat are untouched.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_geothermal_art as collector


class CollectorSourceChecks(unittest.TestCase):
    """Require original visual custody across one industry's nine actual native cells."""

    @classmethod
    def setUpClass(cls):
        """Cache approved ready/construction originals and their actual native pieces.

        @param cls Current focused source/check class.
        @return None; source/public/runtime files remain unchanged.
        """
        cls.public_bytes = collector.tube.PUBLIC.read_bytes()
        cls.document = json.loads(collector.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(cls.public_bytes)
        cls.family = next(row for row in cls.document["families"] if row["asset_id"] == "facility.vent-collector")
        cls.views = {}
        # Four genuine originals supply the complete ready view set. Cache
        # their real pieces once, keeping focused malformed cases independent.
        for direction, key in cls.family["whole_view_selections"].items():
            campus, original = collector.collector_base(key)
            pieces, source, unused = collector.partition_collector(key)
            cls.views[direction] = (campus, pieces, source)
        cls.construction_views = {}
        # Real unfinished quarters retain their own twelve originals. Stage3
        # explicitly reuses the approved ready hardware, whose native pieces
        # are already cached; no duplicate source work is needed for that alias.
        for stage in range(4):
            for direction, key in cls.family["construction_frame_sources"][str(stage)].items():
                if stage == 3:
                    cls.construction_views[stage, direction] = cls.views[direction]
                else:
                    campus, original = collector.collector_base(key)
                    pieces, source, unused = collector.partition_collector(key)
                    cls.construction_views[stage, direction] = (campus, pieces, source)

    def assert_original_reconstruction(self, label, original, pieces, source):
        """Independently rebuild an original through decoded native PNG sections.

        @param self Current focused source/check instance.
        @param label Human-readable actual view/stage for an assertion failure.
        @param original Approved full native indexed original model.
        @param pieces Nine actual native cell bitmaps to encode and decode.
        @param source Original provenance and per-cell unique-pixel measurements.
        @return None; exact colours, alpha, ownership and complete source are asserted.
        """
        rebuilt = Image.new("P", original.size, 0)
        rebuilt.putpalette(collector.tube.base.PALETTE_BYTES)
        occupied = set()
        self.assertEqual(len(pieces), 9)
        # Decode the actual output format through the native world projection,
        # independently of the partitioner's nearest-contact ownership rule.
        for index, piece in enumerate(pieces):
            x, y = index % 3, index // 3
            ox, oy = 128 + 32 * (y - x) - 31, 64 + 16 * (x + y) - 33
            with Image.open(io.BytesIO(collector.tube.indexed_png(piece))) as decoded:
                self.assertEqual(decoded.size, (64, 64))
                self.assertIsNotNone(decoded.getbbox())
                self.assertEqual(decoded.info.get("transparency"), 0)
                for ly in range(64):
                    for lx in range(64):
                        value = decoded.getpixel((lx, ly))
                        if value == 0:
                            continue
                        point = (ox + lx, oy + ly)
                        self.assertNotIn(point, occupied, label)
                        occupied.add(point)
                        rebuilt.putpixel(point, value)
        self.assertEqual(rebuilt.tobytes(), original.tobytes(), label)
        self.assertEqual(sum(row["opaque_pixels"] for row in source["resolved_cell_sources"]), len(occupied))

    def test_all_four_views_reconstruct_exact_original_through_png(self):
        """Rebuild each original from decoded native PNG pieces with unique pixels.

        @param self Current focused source/check instance.
        @return None; original colours/alpha and all nine native cells remain exact.
        """
        self.assertEqual(set(self.views), {"NE", "SE", "SW", "NW"})
        for direction, (original, pieces, source) in self.views.items():
            self.assert_original_reconstruction(direction, original, pieces, source)

    def test_native_construction_progress_reconstructs_all_four_stages(self):
        """Keep all actual paid quarters distinct and lossless in every physical view.

        @param self Current focused source/check instance.
        @return None; twelve incomplete originals and four ready aliases preserve144 cells.
        """
        self.assertEqual(set(self.construction_views), {(stage, view) for stage in range(4) for view in self.views})
        originals = set()
        # Actual model progression must survive native palette/section encoding,
        # not just metadata labels attached to one repeated finished bitmap.
        for (stage, direction), (original, pieces, source) in self.construction_views.items():
            self.assert_original_reconstruction(f"{direction} stage{stage}", original, pieces, source)
            originals.add(source["sha256"])
            if stage == 3:
                self.assertEqual(source["key"], self.family["whole_view_selections"][direction])
            else:
                self.assertEqual(source["construction_stage"], stage)
                self.assertEqual(source["model_state"], "construction")
        self.assertEqual(len(originals), 16)

    def test_finished_hardware_cannot_replace_an_incomplete_stage(self):
        """Reject a recoloured final model standing in for a missing paid-work quarter.

        @param self Current focused source/check instance.
        @return None; actual unfinished source remains mandatory before stage3.
        """
        document = copy.deepcopy(self.document)
        document["families"][0]["construction_frame_sources"]["1"]["SE"] = self.family["whole_view_selections"]["SE"]
        with self.assertRaisesRegex(ValueError, "actual construction original"):
            collector.collector_construction_signature(document)

    def test_final_stage_keeps_the_approved_ready_hardware(self):
        """Reject replacing final-looking unfinished stage3 with an earlier model.

        @param self Current focused source/check instance.
        @return None; native stage3 and completed hardware retain their intentional alias.
        """
        document = copy.deepcopy(self.document)
        document["families"][0]["construction_frame_sources"]["3"]["SE"] = self.family["construction_frame_sources"]["2"]["SE"]
        with self.assertRaisesRegex(ValueError, "retain approved hardware"):
            collector.collector_construction_signature(document)

    def test_changed_construction_registration_refuses(self):
        """Reject moving a frozen construction model while its raw image still hashes.

        @param self Current focused source/check instance.
        @return None; construction cannot drift away from the approved native footprint.
        """
        document = copy.deepcopy(self.document)
        key = self.family["construction_frame_sources"]["2"]["SE"]
        next(row for row in document["candidates"] if row["key"] == key)["native_position"][1] -= 1
        with patch.object(collector.json, "loads", side_effect=[document, self.public]):
            with self.assertRaisesRegex(ValueError, "construction registration differs"):
                collector.collector_base(key)

    def test_construction_hardware_review_does_not_claim_composed_source(self):
        """Keep equipment-glass composition, packing and runtime pending after model review.

        @param self Current focused source/check instance.
        @return None; sixteen approved hardware views cannot activate a Heat producer.
        """
        report = json.loads((collector.tube.REVIEW / "collector-construction-views-manifest.json").read_text())
        self.assertEqual(report["native_cells"], 144)
        self.assertEqual(len(report["views"]), 16)
        self.assertTrue(report["construction_hardware_coverage_complete"])
        self.assertFalse(report["source_coverage_complete"])
        self.assertFalse(report["runtime_activated"])
        self.assertTrue(all(not row["final_glass_composed"] for row in report["views"]))

    def test_zero_is_transparent_before_and_after_png_encoding(self):
        """Prevent opaque palette-zero canvases from hiding another native cell.

        @param self Current focused source/check instance.
        @return None; in-memory assembly and actual PNG decoding agree on alpha.
        """
        # Lossless indexed pixels are insufficient if palette-zero becomes an
        # opaque blue rectangle during actual composition or PNG consumption.
        for direction, (unused, pieces, source) in self.views.items():
            for piece in pieces:
                self.assertEqual(piece.info.get("transparency"), 0)
                rgba = piece.convert("RGBA")
                for value, pixel in zip(piece.get_flattened_data(), rgba.get_flattened_data()):
                    self.assertEqual(pixel[3], 0 if value == 0 else 255, direction)
                with Image.open(io.BytesIO(collector.tube.indexed_png(piece))) as decoded:
                    self.assertEqual(decoded.convert("RGBA").getchannel("A").tobytes(), rgba.getchannel("A").tobytes())

    def test_native_world_cells_keep_actual_anchor_and_projection(self):
        """Bind per-cell original provenance to actual RemapCoords and sprite anchor.

        @param self Current focused source/check instance.
        @return None; no reversed X/Y projection or fictitious single-tile campus passes.
        """
        expected = [[x, y] for y in range(3) for x in range(3)]
        for direction, (unused, pieces, source) in self.views.items():
            self.assertEqual(source["native_geometry"]["tile_cells"], expected)
            self.assertEqual(source["native_geometry"]["anchor"], [-31, -33])
            # Actual native X runs SW/down-left and Y SE/down-right. Each
            # sprite's source window follows that real world cell exactly.
            for row in source["resolved_cell_sources"]:
                x, y = row["world_cell"]
                self.assertEqual(row["native_source_box"], [97 + 32 * (y - x), 31 + 16 * (x + y), 64, 64])
                self.assertEqual(row["source_sha256"], source["sha256"])

    def test_original_pixel_outside_native_canvases_refuses(self):
        """Reject out-of-floor artwork without clipping away the offending pixel.

        @param self Current focused source/check instance.
        @return None; all original visible pixels need a real native owner.
        """
        campus, unused, source = self.views["SE"]
        malformed = campus.copy()
        malformed.putpixel((0, 0), 4)
        with patch.object(collector, "collector_base", return_value=(malformed, source)):
            with self.assertRaisesRegex(ValueError, "leaves native cell canvases"):
                collector.partition_collector(self.family["whole_view_selections"]["SE"])

    def test_missing_actual_cell_refuses(self):
        """Reject a one-pixel model masquerading as all nine physical native cells.

        @param self Current focused source/check instance.
        @return None; every native TIDS cell retains actual original artwork.
        """
        campus, unused, source = self.views["SE"]
        malformed = Image.new("P", campus.size, 0)
        malformed.putpixel((128, 64), 4)
        with patch.object(collector, "collector_base", return_value=(malformed, source)):
            with self.assertRaisesRegex(ValueError, "loses original pixels or a native cell"):
                collector.partition_collector(self.family["whole_view_selections"]["SE"])

    def test_known_wrong_side_originals_refuse(self):
        """Keep retained wrong-side drafts excluded from accepted whole-view source.

        @param self Current focused source/check instance.
        @return None; an original with an ignored target side is never fallback art.
        """
        for key in ("collector-campus-sw-draft-v1", "collector-campus-nw-draft-v1"):
            with self.assertRaisesRegex(ValueError, "known excluded"):
                collector.collector_base(key)

    def test_changed_ready_registration_refuses(self):
        """Reject native sampling drift while the unchanged original still hashes.

        @param self Current focused source/check instance.
        @return None; source identity alone cannot authorize a different physical view.
        """
        document = copy.deepcopy(self.document)
        key = self.family["whole_view_selections"]["SE"]
        next(row for row in document["candidates"] if row["key"] == key)["native_position"][1] += 1
        with patch.object(collector.json, "loads", side_effect=[document, self.public]):
            with self.assertRaisesRegex(ValueError, "differs from approved views"):
                collector.collector_base(key)

    def test_duplicate_original_cannot_supply_another_heading(self):
        """Reject one unchanged image relabelled as two physical whole-model views.

        @param self Current focused source/check instance.
        @return None; genuine source images remain distinct across all four headings.
        """
        document = copy.deepcopy(self.document)
        se = next(row for row in document["candidates"] if row["key"] == self.family["whole_view_selections"]["SE"])
        ne = next(row for row in document["candidates"] if row["key"] == self.family["whole_view_selections"]["NE"])
        ne["path"], ne["sha256"] = se["path"], se["sha256"]
        with self.assertRaisesRegex(ValueError, "four genuine original images"):
            collector.collector_view_signature(document)

    def test_changed_native_public_anchor_refuses(self):
        """Reject a public anchor inconsistent with the actual nine-cell producer.

        @param self Current focused source/check instance.
        @return None; artwork never redefines the native industry's physical geometry.
        """
        public = copy.deepcopy(self.public)
        next(row for row in public["assets"] if row["asset_id"] == "facility.vent-collector")["anchor"] = [-32, -32]
        with patch.object(collector.json, "loads", side_effect=[self.document, public]):
            with self.assertRaisesRegex(ValueError, "actual nine-cell native geometry"):
                collector.collector_base(self.family["whole_view_selections"]["SE"])

    def test_changed_original_hash_refuses(self):
        """Reject altered original provenance without modifying repository art.

        @param self Current focused source/check instance.
        @return None; every actual original and declared input retains its exact bytes.
        """
        document = copy.deepcopy(self.document)
        key = self.family["whole_view_selections"]["SE"]
        next(row for row in document["candidates"] if row["key"] == key)["sha256"] = "0" * 64
        with patch.object(collector.json, "loads", return_value=document):
            with self.assertRaisesRegex(ValueError, "original|reference"):
                collector.collector_base(key)

    def test_ready_review_does_not_claim_complete_source_or_runtime(self):
        """Keep missing construction/state/phase work explicit in each native review.

        @param self Current focused source/check instance.
        @return None; ready pictures cannot activate a plant or finish the whole family.
        """
        for key in self.family["whole_view_selections"].values():
            report = json.loads((collector.tube.REVIEW / (key + "-cells-review.json")).read_text(encoding="utf-8"))
            self.assertEqual(report["native_cells"], 9)
            self.assertFalse(report["source_coverage_complete"])
            self.assertFalse(report["runtime_activated"])

    def test_conversion_preserves_public_source_and_packed_metadata(self):
        """Require read-only original/cell conversion across every accepted heading.

        @param self Current focused source/check instance.
        @return None; neither content activation nor packed metadata changes during sampling.
        """
        self.assertEqual(collector.tube.PUBLIC.read_bytes(), self.public_bytes)


if __name__ == "__main__":
    unittest.main()
