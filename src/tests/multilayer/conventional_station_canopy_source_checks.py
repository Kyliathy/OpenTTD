"""Focused native-scale proof for genuine conventional station canopy source views.

@file conventional_station_canopy_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task24.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original supports, complementary roofs and glass are source-only; native packing
and ordinary-game rendering remain the separate master10.4D obligation.
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
import convert_station_canopies as canopy


class ConventionalStationCanopySourceChecks(unittest.TestCase):
    """Check actual support/roof contacts, transparent-child ownership and immutable pixels."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the active canopy source checkpoint once for focused checks.

        @param cls Test class retaining actual original, native and output evidence.
        @return None.
        """
        cls.record = json.loads(canopy.loader.RECORD.read_text(encoding="utf-8"))
        cls.report, cls.files = canopy.build(cls.record)
        cls.geometry = canopy.views.load_geometry()
        cls.atlas = Image.open(io.BytesIO(cls.files[canopy.OUTPUT / f"{canopy.PREFIX}-atlas.png"]))
        _, cls.base_files = canopy.views.build(cls.record)
        cls.base_atlas = Image.open(io.BytesIO(cls.base_files[canopy.OUTPUT / f"{canopy.views.PREFIX}-atlas.png"]))

    def source_part(self, graphics, part):
        """Read one source owner from the deterministic four-cell canopy atlas.

        @param self Active canopy source proof.
        @param graphics Actual native graphics4..7.
        @param part Back/front/roof/glass native source owner.
        @return Independent indexed64x64 source image.
        """
        row = next(row for row in self.report["source_rows"] if row["native_graphics"] == graphics and row["part"] == part)
        return self.atlas.crop(row["box"])

    def test_exact_regeneration_and_complete_ready_view_matrix(self):
        """Prove all eight native ready0 cells while leaving full state/runtime coverage pending.

        @param self Active canopy source proof.
        @return None.
        """
        # Exact published bytes bind native-scale comparisons to original
        # inputs, rather than a source-complete label or a plausible screenshot.
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        rows = self.report["source_rows"]
        self.assertEqual(len(rows), 16)
        self.assertEqual({(row["native_graphics"], row["part"]) for row in rows},
                         {(gfx, part) for gfx in range(4, 8) for part in ("back", "front", "roof", "glass")})
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])
        # Refactoring shared original extraction must preserve every previously
        # approved low-view pixel/artifact and the exact selected X base.
        for path, payload in self.base_files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)

    def test_original_supports_reach_real_deck_and_elevated_roof(self):
        """Derive physical contacts independently from actual native supporting strips.

        @param self Active canopy source proof.
        @return None.
        """
        # Source posts reach roofz16 even where native sorting metadata remains
        # deliberately2/7 high. Neither a short post nor a box rewrite is valid.
        for graphics in range(4, 8):
            layout = self.geometry["layouts"][graphics]
            owner = 0 if layout["role"] == 2 else 1
            part = "back" if owner == 0 else "front"
            strip = layout["parts"][owner]
            across = strip["origin"][1 if layout["axis"] == "X" else 0] + (3 if owner == 0 else 2)
            selected = next(row for row in self.report["source_selections"] if row["graphics"] == graphics)
            for support in selected["supports"]:
                longitudinal = support["longitudinal"]
                sprite = canopy.views.native_component(self.record, {**support, "part": part}, layout)
                wx, wy = (longitudinal, across) if layout["axis"] == "X" else (across, longitudinal)
                for height in (2, layout["parts"][2]["origin"][2]):
                    px, py = 31 + 2 * (wy - wx), 33 + wx + wy - height
                    self.assertTrue(any(sprite.getpixel((x, y)) for x in range(px - 2, px + 3)
                                        for y in range(py - 2, py + 3)), (graphics, longitudinal, height))
            self.assertEqual(layout["parts"][2]["origin"], [0, 0, 16])
        self.assertEqual(self.geometry["layouts"][4]["parts"][0]["extent"][2], 7)
        self.assertEqual(self.geometry["layouts"][5]["parts"][0]["extent"][2], 2)

    def test_front_train_clearance_changes_only_at_two_physical_posts(self):
        """Near-side posts may occlude their own narrow columns; every other front pixel stays exact.

        @param self Active canopy source proof.
        @return None.
        """
        # Independently measured changed pixels must stay in the two six-pixel
        # support columns. This catches a new wall or a source-shadow rail bridge.
        for graphics in range(4, 8):
            layout = self.geometry["layouts"][graphics]
            axis_index = graphics % 2
            for index, part in enumerate(("back", "front")):
                bare = self.base_atlas.crop((index * 64, axis_index * 64, (index + 1) * 64, (axis_index + 1) * 64))
                current = self.source_part(graphics, part)
                supporting = (layout["role"] == 2 and part == "back") or (layout["role"] == 3 and part == "front")
                if not supporting:
                    self.assertEqual(current.tobytes(), bare.tobytes())
                    continue
                centres = (33, 9) if graphics == 4 else (29, 53) if graphics == 5 else (53, 29) if graphics == 6 else (9, 33)
                differences = [(i % 64, i // 64) for i, (a, b) in enumerate(zip(current.get_flattened_data(), bare.get_flattened_data())) if a != b]
                self.assertTrue(differences)
                self.assertTrue(all(any(centre - 3 <= x < centre + 3 for centre in centres) for x, _ in differences))
                self.assertLessEqual(len(differences), 2 * 6 * 17)

    def test_four_original_panes_are_disjoint_from_structural_roof(self):
        """Separate native child pixels reconstruct the exact original roof without stock glass.

        @param self Active canopy source proof.
        @return None.
        """
        # Compare against independently extracted original pixels, not the
        # whole-preview rendering order, which intentionally hides transparency.
        for graphics in range(4, 8):
            layout = self.geometry["layouts"][graphics]
            selected = next(row for row in self.report["source_selections"] if row["graphics"] == graphics)
            original = canopy.views.native_component(self.record, {**selected["roof"], "part": "roof"}, layout)
            roof, glass = self.source_part(graphics, "roof"), self.source_part(graphics, "glass")
            self.assertEqual(len(canopy.pane_components(glass)), 4)
            self.assertTrue(all(len(component) >= 12 for component in canopy.pane_components(glass)))
            self.assertFalse(any(a and b for a, b in zip(roof.get_flattened_data(), glass.get_flattened_data())))
            self.assertEqual(canopy.overlay_original(roof, glass).tobytes(), original.tobytes())
            self.assertEqual(layout["roof_glass"], {"screen_offset": [0, 0], "transparent": True, "palette": "PALETTE_TO_TRANSPARENT"})
            self.assertEqual(canopy.views.native_packed_anchor(layout, "roof"), [-31, -17])
            self.assertEqual(canopy.views.native_packed_anchor(layout, "glass"), [0, 0])

    def test_missing_duplicate_and_relabelled_canopy_cells_refuse(self):
        """A repeated cell or another axis cannot fill a missing genuine native source view.

        @param self Active canopy source proof.
        @return None.
        """
        # Matrix failures must occur before any partial output publication.
        for duplicate in (False, True):
            record = copy.deepcopy(self.record)
            rows = record["families"]["facility.rail-station"]["canopy_sources"]
            if duplicate:
                rows[-1] = copy.deepcopy(rows[0])
            else:
                rows.pop()
            with self.assertRaisesRegex(ValueError, "matrix"):
                canopy.build(record)
        record = copy.deepcopy(self.record)
        source = next(row for row in record["candidates"] if row["id"] == "station-canopy-supports-v1")
        source["components"][0]["axis"] = "Y"
        with self.assertRaisesRegex(ValueError, "actual native parent"):
            canopy.build(record)

    def test_shifted_or_duplicated_supports_refuse(self):
        """Real roof/deck contacts cannot be replaced by moved or duplicated original posts.

        @param self Active canopy source proof.
        @return None.
        """
        record = copy.deepcopy(self.record)
        source = next(row for row in record["candidates"] if row["id"] == "station-canopy-supports-v1")
        source["components"][0]["native_box"][1] += 1
        with self.assertRaisesRegex(ValueError, "physical contact"):
            canopy.build(record)
        record = copy.deepcopy(self.record)
        selected = record["families"]["facility.rail-station"]["canopy_sources"][0]
        selected["supports"][1]["component"] = selected["supports"][0]["component"]
        with self.assertRaisesRegex(ValueError, "contacts"):
            canopy.build(record)

    def test_roof_contact_drift_and_missing_original_pane_refuse(self):
        """An elevated but displaced roof or incomplete pane set cannot inherit source approval.

        @param self Active canopy source proof.
        @return None.
        """
        record = copy.deepcopy(self.record)
        source = next(row for row in record["candidates"] if row["id"] == "station-canopy-roofs-v1")
        source["components"][0]["native_box"] = [0, 20, 64, 45]
        with self.assertRaisesRegex(ValueError, "modular corner"):
            canopy.build(record)
        layout = self.geometry["layouts"][4]
        selected = self.report["source_selections"][0]["roof"]
        original = canopy.views.native_component_rgba(self.record, {**selected, "part": "roof"}, layout)
        missing_pane = original.copy()
        # This synthetic negative removes one measured original pane, verifying
        # that three panes cannot pass merely because the steel silhouette fits.
        for pixel in canopy.pane_components(self.source_part(4, "glass"))[0]:
            missing_pane.putpixel(pixel, (0, 0, 0, 0))
        with self.assertRaisesRegex(ValueError, "four separate"):
            canopy.split_roof(missing_pane)

    def test_rejected_original_and_changed_frame_window_cannot_publish(self):
        """Source rejection and opaque source-frame contamination refuse before changing output.

        @param self Active canopy source proof.
        @return None.
        """
        # Existing published evidence must remain exact after both refusal
        # paths; neither source approval nor a source crop is trusted blindly.
        for field in ("selection", "frame"):
            record = copy.deepcopy(self.record)
            source = next(row for row in record["candidates"] if row["id"] == "station-canopy-roofs-v1")
            if field == "selection":
                source["selection"]["status"] = "rejected_native_geometry"
            else:
                source["components"][0]["source_window"] = [0, 0, 744, 488]
            with self.assertRaises(ValueError):
                canopy.build(record)
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload)


if __name__ == "__main__":
    unittest.main()
