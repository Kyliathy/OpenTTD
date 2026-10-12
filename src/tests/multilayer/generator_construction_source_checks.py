"""Protect reviewed incomplete Powerhouse originals and native cell ownership.

@file generator_construction_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group55B
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Complete stage0..2 source and final hardware aliasing do not activate operating machinery.
"""

import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_generator_art as art


class GeneratorConstructionSourceChecks(unittest.TestCase):
    """Verify actual unfinished views independently of ready-model or runtime completion.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks55B.1-55B.2.
    """

    def setUp(self):
        """Load isolated candidate records while retaining original source bytes.

        @param self Current focused source test instance.
        @return None; mutations apply only to independent dictionaries.
        """
        self.record = json.loads(art.RECORD.read_text())
        self.public = json.loads(art.tube.PUBLIC.read_text())
        self.asset = "facility.vent-powerhouse"
        self.family = next(row for row in self.record["families"] if row["asset_id"] == self.asset)
        self.spec = next(row for row in self.public["assets"] if row["asset_id"] == self.asset)
        self.source = next(row for row in self.record["candidates"] if row["key"] == "venttap-construction-1-ne-v1")

    def test_twelve_incomplete_originals_preserve_all_sixteen_cells(self):
        """Rebuild actual whole originals from their unique per-cell pixels.

        @param self Current focused source test instance.
        @return None; all192 nonempty cells reconstruct the twelve full originals exactly.
        """
        models = art.construction_models(self.asset, (0, 1, 2), self.record, self.public)
        self.assertEqual(len(models), 12)
        # Recompose the stored cell pixels independently. Duplicate ownership,
        # clipped equipment and missing floor cells cannot pass conservation.
        for (stage, direction), (campus, pieces, source) in models.items():
            self.assertEqual(source["construction_stage"], stage)
            self.assertEqual(source["direction"], direction)
            rebuilt = art.indexed_canvas(art.CANVAS)
            occupied = set()
            for (ox, oy), piece in zip(art.ORIGINS, pieces):
                self.assertIsNotNone(piece.getbbox())
                for y in range(64):
                    for x in range(64):
                        value = piece.getpixel((x, y))
                        if value == 0:
                            continue
                        point = (ox + x, oy + y)
                        self.assertNotIn(point, occupied)
                        occupied.add(point)
                        rebuilt.putpixel(point, value)
            self.assertEqual(rebuilt.tobytes(), campus.tobytes())

    def test_removed_stage2_cannot_borrow_final_hardware(self):
        """Refuse whole construction coverage if the selected stage2 originals are removed.

        @param self Current focused source test instance.
        @return None; source-only readiness never fabricates later construction.
        """
        self.assertTrue(self.family["construction_hardware_coverage_complete"])
        del self.family["construction_frame_sources"]["2"]
        with self.assertRaisesRegex(ValueError, "four distinct reviewed views"):
            art.construction_models(self.asset, (2,), self.record, self.public)
        self.assertFalse(self.family["runtime_activated"])

    def test_final_hardware_alias_retains_paid_work_and_amber_policy(self):
        """Keep native stage3 distinct from actual construction completion.

        @param self Current focused source test instance.
        @return None; ready hardware retains the explicit592-tick construction policy.
        """
        self.assertEqual(self.family["construction_frame_sources"]["3"], self.family["whole_view_selections"])
        self.assertEqual(self.family["construction_indicator_phase"], 0)
        self.assertEqual(self.family["construction_ticks"], {"total": 592, "stage_length": 148,
                         "stages": [0, 1, 2, 3], "completion_separate": True})

    def test_isolated_stage2_crops_keep_all_opaque_original_pixels(self):
        """Keep every real model corner and exclude the two rejected NE attempts.

        @param self Current focused source test instance.
        @return None; four isolated crops enclose complete opaque source bounds.
        """
        from PIL import Image
        keys = set(self.family["construction_frame_sources"]["2"].values())
        self.assertNotIn("venttap-construction-2-ne-v1", keys)
        self.assertNotIn("venttap-construction-2-ne-v2", keys)
        # Immutable alpha is inspected independently of the registered native
        # image, so a cropped-off floor corner cannot hide in a passing partition.
        for key in keys:
            source = art.tube.retained_candidate(key, self.record)
            with Image.open(ROOT / source["path"]) as original:
                bounds = original.getchannel("A").point(lambda value: 255 if value >= 128 else 0).getbbox()
                left, top, right, bottom = source["native_crop"]
                self.assertLessEqual(left, bounds[0])
                self.assertLessEqual(top, bounds[1])
                self.assertGreaterEqual(right, bounds[2])
                self.assertGreaterEqual(bottom, bounds[3])
                self.assertGreater(bounds[0], 0)
                self.assertGreater(bounds[1], 0)
                self.assertLess(bounds[2], original.width)
                self.assertLess(bounds[3], original.height)

    def test_measured_ne_registration_cannot_drift(self):
        """Preserve the reviewed correction that keeps every original pixel within cells.

        @param self Current focused source test instance.
        @return None; neither the former leaking registration nor another shift is accepted.
        """
        self.assertEqual(self.source["native_position"], [33, 50])
        self.source["native_position"][1] += 1
        with self.assertRaisesRegex(ValueError, "reviewed original"):
            art.construction_models(self.asset, (1,), self.record, self.public)

    def test_ready_hardware_cannot_impersonate_incomplete_work(self):
        """Refuse a valid ready original substituted for unfinished construction.

        @param self Current focused source test instance.
        @return None; a matching family alone cannot finish a paid work stage.
        """
        self.family["construction_frame_sources"]["0"]["NE"] = self.family["whole_view_selections"]["NE"]
        with self.assertRaisesRegex(ValueError, "wrong native stage or view"):
            art.construction_models(self.asset, (0,), self.record, self.public)

    def test_missing_or_duplicate_views_refuse(self):
        """Refuse missing orientations and one physical view copied into another slot.

        @param self Current focused source test instance.
        @return None; all four independently registered views remain necessary.
        """
        original = copy.deepcopy(self.family["construction_frame_sources"]["0"])
        # Both incomplete matrices fail before source conversion; neither can
        # borrow final hardware or silently suppress the missing physical view.
        for mode in ("missing", "duplicate"):
            selections = copy.deepcopy(original)
            if mode == "missing":
                del selections["NW"]
            else:
                selections["NW"] = selections["NE"]
            self.family["construction_frame_sources"]["0"] = selections
            with self.assertRaisesRegex(ValueError, "four distinct reviewed views"):
                art.construction_models(self.asset, (0,), self.record, self.public)

    def test_changed_original_refuses(self):
        """Refuse an altered source identity before any registration can approve it.

        @param self Current focused source test instance.
        @return None; original hashes remain exact and immutable.
        """
        self.source["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable"):
            art.construction_models(self.asset, (1,), self.record, self.public)

    def test_construction_stage_is_frozen_with_original(self):
        """Bind the native work stage into an incomplete original's signature.

        @param self Current focused source test instance.
        @return None; stage labels cannot reinterpret already reviewed original pixels.
        """
        before = art.registration_signature(self.source)
        self.source["construction_stage"] = 0
        self.assertNotEqual(art.registration_signature(self.source), before)
        with self.assertRaisesRegex(ValueError, "wrong native stage or view"):
            art.construction_models(self.asset, (1,), self.record, self.public)

    def test_final_or_repeated_incomplete_stage_refuses(self):
        """Keep final hardware aliasing separate from independently authored incomplete stages.

        @param self Current focused source test instance.
        @return None; empty, repeated and final-stage requests cannot hide missing stage2.
        """
        for stages in ((), (0, 0), (3,)):
            with self.assertRaisesRegex(ValueError, "distinct incomplete"):
                art.construction_models(self.asset, stages, self.record, self.public)


if __name__ == "__main__":
    unittest.main()
