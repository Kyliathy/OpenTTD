"""Focused genuine-view, original reuse and access proof for native mine terminals.

@file conventional_terminal_view_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task25.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Six real views retain independently sorted parents; source comparisons do not
establish packed pixels, native vehicle sorting or ordinary-game acceptance.
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
import convert_terminal_views as views


class ConventionalTerminalViewSourceChecks(unittest.TestCase):
    """Require six genuine native views, exact approved ancestry and open actual access."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the active six-view set once from immutable original components.

        @param cls Test class retaining native geometry and independent indexed parents.
        @return None.
        """
        cls.record = json.loads(views.terminal.RECORD.read_text(encoding="utf-8"))
        cls.report, cls.files = views.build(cls.record)
        cls.geometry = views.terminal.load_geometry()
        cls.atlas = Image.open(io.BytesIO(cls.files[views.OUTPUT / f"{views.PREFIX}-atlas.png"]))
        cls.parts = {}
        # Preserve absence explicitly: a blank sprite cannot impersonate a
        # missing bay rail or change the native through layout's parent count.
        for layout in cls.geometry["layouts"]:
            graphics = layout["graphics"]
            cls.parts[graphics] = [None if parent is None else cls.atlas.crop((index * 64, graphics * 64, (index + 1) * 64, (graphics + 1) * 64))
                                   for index, parent in enumerate(layout["parents"])]

    def test_exact_six_view_artifacts_and_native_parent_coverage(self):
        """Retain all16 actual parents and exactly two structural through absences.

        @param self Active native-view source proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        self.assertEqual(self.atlas.size, (192, 384))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertEqual(len(self.report["source_rows"]), 18)
        self.assertEqual(self.report["nonempty_source_parts"], 16)
        self.assertEqual(self.report["structurally_absent_parts"], 2)
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertFalse(set(self.atlas.get_flattened_data()) & (views.terminal.base.FORBIDDEN - {0}))
        self.assertEqual([(row["native_graphics"], row["part"]) for row in self.report["source_rows"] if row["structurally_absent"]],
                         [(4, "parent-2"), (5, "parent-2")])

    def test_se_base_is_exact_and_four_directed_housings_are_distinct(self):
        """Keep the approved SE pixels while genuinely directed facades remain different originals.

        @param self Active native-view source proof.
        @return None.
        """
        self.assertEqual(self.files[views.OUTPUT / f"{views.PREFIX}-gfx1-whole.png"],
                         (views.OUTPUT / "terminal-se-base-whole.png").read_bytes())
        self.assertEqual(len({self.parts[gfx][1].tobytes() for gfx in range(4)}), 4)
        self.assertEqual(self.parts[4][0].tobytes(), self.parts[1][1].tobytes())
        self.assertEqual(self.parts[5][0].tobytes(), self.parts[0][1].tobytes())

    def test_six_bounded_reuses_preserve_every_whole_original_pixel(self):
        """Actual world translations preserve exact original material without crop, stretch or rotation.

        @param self Active native-view source proof.
        @return None.
        """
        reused = 0
        # Compare all4096 native pixels independently, including empty space.
        # Component ancestry alone cannot prove the converter preserved them.
        for selection in self.record["families"]["facility.road-terminal"]["ready_parts"]:
            component = views.component_record(self.record, selection)
            reuse = component.get("reuse_of")
            if reuse is None:
                continue
            reused += 1
            dx, dy = reuse["screen_translation"]
            source = self.parts[reuse["graphics"]][int(reuse["part"][-1])]
            target = self.parts[selection["graphics"]][int(selection["part"][-1])]
            for y in range(64):
                for x in range(64):
                    value = source.getpixel((x - dx, y - dy)) if 0 <= x - dx < 64 and 0 <= y - dy < 64 else 0
                    self.assertEqual(target.getpixel((x, y)), value, (selection["component"], x, y))
            self.assertEqual(sum(bool(value) for value in target.get_flattened_data()),
                             sum(bool(value) for value in source.get_flattened_data()))
        self.assertEqual(reused, 6)

    def test_original_windows_are_clear_and_wrong_axis_panel_is_excluded(self):
        """Every unique selected original retains transparent margins and no opaque frame paint.

        @param self Active native-view source proof.
        @return None.
        """
        windows = set()
        # Several native parents intentionally reuse one genuine component;
        # inspect each original rectangle once rather than treating clones as
        # independent generated views or multiplying their source identities.
        for selection in self.record["families"]["facility.road-terminal"]["ready_parts"]:
            source = next(row for row in self.record["candidates"] if row["id"] == selection["source"])
            component = views.component_record(self.record, selection)
            key = (source["id"], tuple(component["source_window"]))
            if key in windows:
                continue
            windows.add(key)
            crop = Image.open(ROOT / source["path"]).convert("RGBA").crop(component["source_window"])
            bounds = crop.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
            self.assertEqual(list(bounds), component["measured_alpha16_bbox"])
            left, top, right, bottom = bounds
            self.assertGreaterEqual(min(left, top, crop.width - right, crop.height - bottom), 33)
            self.assertFalse(any(alpha >= 16 and red > 220 and green < 60 and blue > 220 for red, green, blue, alpha in crop.get_flattened_data()))
        self.assertEqual(len(windows), 10)
        with self.assertRaisesRegex(ValueError, "excluded"):
            views.terminal.native_component(self.record, {"source": "terminal-equipment-v1", "component": "rail-x13", "part": "parent-0"}, self.geometry["layouts"][0])

    def test_missing_duplicate_and_wrong_native_view_refuse(self):
        """Missing whole views and relabelled parents cannot inherit genuine through absence.

        @param self Active native-view source proof.
        @return None.
        """
        for change in ("missing", "duplicate", "wrong-view"):
            record = copy.deepcopy(self.record)
            rows = record["families"]["facility.road-terminal"]["ready_parts"]
            if change == "missing":
                rows.pop()
            elif change == "duplicate":
                rows[-1] = copy.deepcopy(rows[0])
            else:
                rows[-1]["graphics"] = 4
            with self.subTest(change=change), self.assertRaisesRegex(ValueError, "matrix"):
                views.build(record)

    def test_changed_registration_original_reference_and_rejected_source_refuse(self):
        """Immutable geometry and provenance reject quiet source replacement before publication.

        @param self Active native-view source proof.
        @return None.
        """
        for field in ("native_box", "sha256", "reference", "rejected"):
            record = copy.deepcopy(self.record)
            source = next(row for row in record["candidates"] if row["id"] == "terminal-equipment-v1")
            if field == "native_box":
                source["components"][0]["native_box"][0] += 1
            elif field == "sha256":
                source["sha256"] = "0" * 64
            elif field == "reference":
                source["references"][0]["sha256"] = "0" * 64
            else:
                source["selection"]["status"] = "rejected"
            with self.subTest(field=field), self.assertRaises(ValueError):
                views.build(record)
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload)

    def test_orientation_and_false_whole_pixel_reuse_refuse(self):
        """Reusing another axis or stretching an original cannot masquerade as a native translation.

        @param self Active native-view source proof.
        @return None.
        """
        selected = next(row for row in self.record["families"]["facility.road-terminal"]["ready_parts"] if row["component"] == "rail-x13-sw")
        for change in ("axis", "translation", "stretch", "crop"):
            record = copy.deepcopy(self.record)
            component = views.component_record(record, selected)
            if change == "axis":
                component["reuse_of"] = {"source": "terminal-se-base-v1", "component": "rail-near-se", "graphics": 1, "part": "parent-0", "screen_translation": [0, 0]}
            elif change == "translation":
                component["reuse_of"]["screen_translation"][0] += 1
            elif change == "stretch":
                component["native_box"][2] += 1
            else:
                component["source_window"][0] += 1
            with self.subTest(change=change), self.assertRaisesRegex(ValueError, "reuse"):
                views.validate_reuse(record, selected, self.geometry["layouts"][2], self.geometry)

    def test_absent_parent_detached_contact_and_blocked_bay_mouth_refuse(self):
        """Every physical owner and both actual truck entrance lanes remain protected.

        @param self Active native-view source proof.
        @return None.
        """
        for change in ("absent", "detached", "entrance"):
            parts = [sprite.copy() for sprite in self.parts[1]]
            if change == "absent":
                parts[0] = None
            elif change == "detached":
                parts[0].putdata([0] * 4096)
                parts[0].putpixel((32, 32), 15)
            else:
                parts[0].putpixel((31 + 2 * (15 - 5), 33 + 5 + 15), 15)
            with self.subTest(change=change), self.assertRaises(ValueError):
                views.validate_view(parts, self.geometry["layouts"][1])
        for graphics in (4, 5):
            parts = [sprite.copy() if sprite is not None else None for sprite in self.parts[graphics]]
            parts[1].putpixel((31, 43), 15)
            with self.assertRaisesRegex(ValueError, "vehicle centres"):
                views.validate_view(parts, self.geometry["layouts"][graphics])


if __name__ == "__main__":
    unittest.main()
