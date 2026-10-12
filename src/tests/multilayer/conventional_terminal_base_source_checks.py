"""Focused original/source/native-contact proof for the first SE mine-terminal bay.

@file conventional_terminal_base_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task25.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only one base is selected here. Five other views, all source states and native
packing/game rendering remain separate uncompleted delivery obligations.
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
import convert_terminal_art as terminal


class ConventionalTerminalBaseSourceChecks(unittest.TestCase):
    """Preserve all three actual SE native parents, original pixels and truck-mouth clearance."""

    @classmethod
    def setUpClass(cls):
        """Regenerate only the active first terminal base once.

        @param cls Test class retaining exact original/native/base evidence.
        @return None.
        """
        cls.record = json.loads(terminal.RECORD.read_text(encoding="utf-8"))
        cls.report, cls.files = terminal.build(cls.record)
        cls.atlas = Image.open(io.BytesIO(cls.files[terminal.OUTPUT / f"{terminal.PREFIX}-atlas.png"]))
        cls.whole = Image.open(io.BytesIO(cls.files[terminal.OUTPUT / f"{terminal.PREFIX}-whole.png"]))
        cls.layout = terminal.load_geometry()["layouts"][1]

    def test_exact_regeneration_and_three_nonempty_native_parents(self):
        """Bind all partial artifacts to exact original source bytes without claiming complete coverage.

        @param self Active first-base source proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        self.assertEqual(self.atlas.size, (192, 64))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertEqual({row["part"] for row in self.report["source_parts"]}, {"parent-0", "parent-1", "parent-2"})
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])
        # Each source slot really contains independent original pixels; the
        # native bay cannot inherit the third-parent absence of a through stop.
        for index in range(3):
            self.assertIsNotNone(self.atlas.crop((index * 64, 0, (index + 1) * 64, 64)).getbbox())

    def test_original_clearance_magenta_and_uniform_aspect_registration(self):
        """All selected ordinary crops retain generous alpha margins and complete unpainted originals.

        @param self Active first-base source proof.
        @return None.
        """
        # Independently inspect original source rectangles, not final palette
        # output where unwanted separator colors could already be disguised.
        for selected in self.record["families"]["facility.road-terminal"]["base_parts"]:
            source = next(row for row in self.record["candidates"] if row["id"] == selected["source"])
            component = next(row for row in source["components"] if row["id"] == selected["component"])
            crop = Image.open(ROOT / source["path"]).convert("RGBA").crop(component["source_window"])
            bounds = crop.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
            self.assertEqual(list(bounds), component["measured_alpha16_bbox"])
            left, top, right, bottom = bounds
            self.assertGreaterEqual(min(left, top, crop.width - right, crop.height - bottom), 78)
            self.assertFalse(any(alpha >= 16 and red > 220 and green < 60 and blue > 220 for red, green, blue, alpha in crop.get_flattened_data()))
            x0, y0, x1, y1 = component["native_box"]
            scale_x, scale_y = (x1 - x0) / (right - left), (y1 - y0) / (bottom - top)
            self.assertLess(abs(scale_x / scale_y - 1), .02)

    def test_actual_native_strip_contacts_and_both_entry_lanes(self):
        """Three native mounting strips and real SE entrance positions remain correctly registered.

        @param self Active first-base source proof.
        @return None.
        """
        # Contacts come independently from compiled world boxes and actual
        # native SE bay lanes, not from labels on the generated source sheet.
        for index, parent in enumerate(self.layout["parents"]):
            sprite = self.atlas.crop((index * 64, 0, (index + 1) * 64, 64))
            x, y, _ = parent["origin"]
            width, depth, _ = parent["extent"]
            points = [(x, y), (x, y + depth - 1)] if index != 1 else [(x, y + depth - 1), (x + width - 1, y + depth - 1)]
            for wx, wy in points:
                px, py = 31 + 2 * (wy - wx), 33 + wx + wy
                self.assertTrue(any(sprite.getpixel((sx, sy)) for sx in range(max(0, px - 2), min(64, px + 3))
                                    for sy in range(max(0, py - 2), min(64, py + 3))), (index, wx, wy))
        for wx in (5, 9):
            for wy in range(12, 16):
                self.assertEqual(self.whole.getpixel((31 + 2 * (wy - wx), 33 + wx + wy)), 0)
        self.assertFalse(set(self.atlas.get_flattened_data()) & (terminal.base.FORBIDDEN - {0}))

    def test_missing_duplicate_or_relabelled_base_parent_refuses(self):
        """No missing parent, repeated slot or different native view can inherit base selection.

        @param self Active first-base source proof.
        @return None.
        """
        for duplicate in (False, True):
            record = copy.deepcopy(self.record)
            rows = record["families"]["facility.road-terminal"]["base_parts"]
            if duplicate:
                rows[-1] = copy.deepcopy(rows[0])
            else:
                rows.pop()
            with self.assertRaisesRegex(ValueError, "matrix"):
                terminal.build(record)
        record = copy.deepcopy(self.record)
        source = next(row for row in record["candidates"] if row["id"] == "terminal-se-loader-v1")
        source["components"][0]["direction"] = "NE"
        with self.assertRaisesRegex(ValueError, "native parent/view"):
            terminal.build(record)

    def test_changed_original_reference_crop_or_registration_refuses(self):
        """Immutable source identities and approved native placement prevent quiet model drift.

        @param self Active first-base source proof.
        @return None.
        """
        # Every malformed input must refuse before publication, even when a
        # one-pixel shift could still appear close to a contact tolerance.
        for field in ("sha256", "reference", "crop", "native_box"):
            record = copy.deepcopy(self.record)
            source = next(row for row in record["candidates"] if row["id"] == "terminal-se-loader-v1")
            if field == "sha256":
                source["sha256"] = "0" * 64
            elif field == "reference":
                source["references"][0]["sha256"] = "0" * 64
            elif field == "crop":
                source["components"][0]["source_window"] = [0, 0, 800, 800]
            else:
                source["components"][0]["native_box"][0] += 1
            with self.subTest(field=field), self.assertRaises(ValueError):
                terminal.build(record)

    def test_excluded_two_louver_loader_and_failed_sheet_refuse(self):
        """Safe rails do not approve the failed two-louver model or contaminated correction sheet.

        @param self Active first-base source proof.
        @return None.
        """
        selected = {"source": "terminal-se-base-v1", "component": "parent-1-loader", "part": "parent-1"}
        with self.assertRaisesRegex(ValueError, "excluded"):
            terminal.native_component(self.record, selected, self.layout)
        selected["source"] = "terminal-se-base-v2"
        with self.assertRaisesRegex(ValueError, "rejected"):
            terminal.native_component(self.record, selected, self.layout)
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload)


if __name__ == "__main__":
    unittest.main()
