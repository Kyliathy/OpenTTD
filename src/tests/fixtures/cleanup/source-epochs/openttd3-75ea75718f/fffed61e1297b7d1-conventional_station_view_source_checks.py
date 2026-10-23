"""Focused proof for four genuine native bare/loading-building source views.

@file conventional_station_view_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task24.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Canopy/state coverage and native runtime activation remain separate obligations.
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
import convert_station_views as views
from tools.ground_station_contract import load_geometry


class ConventionalStationViewSourceChecks(unittest.TestCase):
    """Preserve native roles/axes, original part ownership and actual floor/train clearance."""

    @classmethod
    def setUpClass(cls):
        """Regenerate only the active source view checkpoint once.

        @param cls Check class retaining exact source/native artifacts.
        @return None.
        """
        cls.record = json.loads(views.loader.RECORD.read_text(encoding="utf-8"))
        cls.report, cls.files = views.build(cls.record)
        cls.geometry = load_geometry()
        cls.atlas = Image.open(io.BytesIO(cls.files[views.OUTPUT / f"{views.PREFIX}-atlas.png"]))

    def test_exact_regeneration_and_original_x_base_preservation(self):
        """All partial artifacts regenerate exactly and the approved X loader remains byte-identical.

        @param self Active focused source view check.
        @return None.
        """
        # Exact bytes bind measured source registration, atlas layout and
        # review to immutable originals rather than a completeness annotation.
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        self.assertEqual(self.files[views.OUTPUT / f"{views.PREFIX}-gfx2-whole.png"],
                         (views.OUTPUT / "station-loader-x-base-whole.png").read_bytes())
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])

    def test_four_native_cells_and_structural_absences_are_explicit(self):
        """Cover all low roles/axes with eight actual parents and eight genuine roof/glass absences.

        @param self Active focused source view check.
        @return None.
        """
        rows = self.report["source_rows"]
        self.assertEqual(len(rows), 16)
        self.assertEqual({row["native_graphics"] for row in rows}, {0, 1, 2, 3})
        self.assertEqual(sum(row["structurally_absent"] for row in rows), 8)
        # The native descriptor, not a blank atlas cell, owns roof/glass
        # absence. Nonempty source parts must really contain original pixels.
        for row in rows:
            sprite = self.atlas.crop(row["box"])
            self.assertEqual(sprite.getbbox() is None, row["structurally_absent"])
            if row["part"] in ("roof", "glass"):
                self.assertTrue(row["structurally_absent"])
            self.assertEqual(row["axis"], self.geometry["layouts"][row["native_graphics"]]["axis"])

    def test_actual_native_ledge_contacts_and_train_centres_remain_clear(self):
        """Inspect source pixels at actual native parent edges and projected vehicle centres.

        @param self Active focused source view check.
        @return None.
        """
        # Compute contacts from compiled5/6/5 native boxes independently of
        # source captions. Both ledges must meet both through-station mouths.
        for graphics in range(4):
            layout = self.geometry["layouts"][graphics]
            whole = Image.open(io.BytesIO(self.files[views.OUTPUT / f"{views.PREFIX}-gfx{graphics}-whole.png"]))
            front = self.atlas.crop(next(row["box"] for row in self.report["source_rows"]
                                         if row["native_graphics"] == graphics and row["part"] == "front"))
            rear_box, front_box, _ = layout["parts"]
            if layout["axis"] == "X":
                edge_values = (rear_box["extent"][1], front_box["origin"][1])
                contacts = [(longitudinal, across) for across in edge_values for longitudinal in (0, rear_box["extent"][0])]
            else:
                edge_values = (rear_box["extent"][0], front_box["origin"][0])
                contacts = [(across, longitudinal) for across in edge_values for longitudinal in (0, rear_box["extent"][1])]
            for world_x, world_y in contacts:
                px, py = 31 + 2 * (world_y - world_x), 33 + world_x + world_y
                self.assertTrue(any(whole.getpixel((x, y)) for x in range(px - 2, px + 3)
                                    for y in range(py - 2, min(64, py + 3))), (graphics, world_x, world_y))
            across = sum(edge_values) // 2
            # Low original parents may not paint a rail-floor bridge or cover
            # the actual checked train-centre heights in the foreground.
            for longitudinal in range(16):
                wx, wy = (longitudinal, across) if layout["axis"] == "X" else (across, longitudinal)
                px, py = 31 + 2 * (wy - wx), 33 + wx + wy
                self.assertEqual(whole.getpixel((px, py)), 0)
                for height in range(9):
                    self.assertEqual(front.getpixel((px, py - height)), 0)

    def test_missing_duplicate_or_relabelled_native_parents_refuse(self):
        """No copied axis or repeated row can fill another required native source slot.

        @param self Active focused source view check.
        @return None.
        """
        for duplicate in (False, True):
            record = copy.deepcopy(self.record)
            rows = record["families"]["facility.rail-station"]["ready_base_parts"]
            if duplicate:
                rows.append(copy.deepcopy(rows[0]))
            else:
                rows.pop()
            with self.assertRaisesRegex(ValueError, "matrix"):
                views.build(record)
        record = copy.deepcopy(self.record)
        source = next(row for row in record["candidates"] if row["id"] == "station-independent-parents-v2")
        next(row for row in source["components"] if row["id"] == "back-y-bare")["axis"] = "X"
        with self.assertRaisesRegex(ValueError, "actual native parent"):
            views.build(record)

    def test_original_reference_and_measured_rectangle_changes_refuse(self):
        """A changed original or frame/crop registration cannot inherit source approval.

        @param self Active focused source view check.
        @return None.
        """
        # Native source claims remain bound to immutable images/references and
        # measured ordinary rectangles; source frames cannot enter the atlas.
        for field in ("sha256", "reference", "source_window"):
            record = copy.deepcopy(self.record)
            source = next(row for row in record["candidates"] if row["id"] == "station-independent-parents-v2")
            if field == "sha256":
                source["sha256"] = "0" * 64
            elif field == "reference":
                source["references"][0]["sha256"] = "0" * 64
            else:
                source["components"][0]["source_window"] = [800, 32, 1504, 480]
            with self.assertRaises(ValueError):
                views.build(record)

    def test_bare_cells_cannot_use_the_loader_housing(self):
        """The same axis/ledge contacts do not turn a building into a required bare platform view.

        @param self Active focused source view check.
        @return None.
        """
        record = copy.deepcopy(self.record)
        rows = record["families"]["facility.rail-station"]["ready_base_parts"]
        row = next(row for row in rows if row["graphics"] == 0 and row["part"] == "back")
        row.clear()
        row.update(graphics=0, part="back", retained_base="station-loader-x-base-v4")
        with self.assertRaisesRegex(ValueError, "bare platform source contains a building"):
            views.build(record)

    def test_one_pixel_front_shift_cannot_paint_the_native_rail_channel(self):
        """Refuse the measured unsafe Y front placement without masking away its original pixels.

        @param self Active focused source view check.
        @return None.
        """
        record = copy.deepcopy(self.record)
        source = next(row for row in record["candidates"] if row["id"] == "station-independent-parents-v2")
        next(row for row in source["components"] if row["id"] == "front-y")["native_box"] = [0, 41, 43, 64]
        with self.assertRaisesRegex(ValueError, "native rail centre"):
            views.build(record)

    def test_rejected_whole_pose_and_excluded_loader_frame_refuse(self):
        """Known failed whole poses and the short sheet loader cannot supply approved parent pixels.

        @param self Active focused source view check.
        @return None.
        """
        for whole_pose in (False, True):
            record = copy.deepcopy(self.record)
            row = next(row for row in record["families"]["facility.rail-station"]["ready_base_parts"]
                       if row["graphics"] == 3 and row["part"] == "back")
            row.update(source="station-loader-y-base-v2" if whole_pose else "station-independent-parents-v2", component="loader-y-back")
            with self.assertRaises(ValueError):
                views.build(record)


if __name__ == "__main__":
    unittest.main()
