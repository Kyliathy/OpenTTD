"""Check the selected station base against compiled native geometry and immutable originals.

@file conventional_station_base_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task24.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This focused source burst grants no source-family, packing or runtime closure.
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
import convert_station_art as station
from tools.ground_station_contract import load_geometry


class ConventionalStationBaseSourceChecks(unittest.TestCase):
    """Protect genuine base selection, native strip contacts and parent/vehicle clearance."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the one selected base pose once for this focused source burst.

        @param cls Check class retaining immutable source/proof inputs.
        @return None.
        """
        cls.record = json.loads(station.RECORD.read_text(encoding="utf-8"))
        cls.identity = "station-loader-x-base-v4"
        cls.source = station.original_candidate(cls.record, cls.identity)
        cls.report, cls.files = station.build(cls.record, cls.identity)
        cls.whole = Image.open(io.BytesIO(cls.files[station.OUTPUT / f"{station.PREFIX}-whole.png"]))
        cls.parts = {name: Image.open(io.BytesIO(cls.files[station.OUTPUT / f"{station.PREFIX}-{name}.png"]))
                     for name in ("back", "front")}
        cls.layout = load_geometry()["layouts"][2]

    def test_deterministic_base_and_original_selection_are_exact(self):
        """Every selected source artifact must rebuild exactly without claiming missing coverage.

        @param self Active focused source check instance.
        @return None.
        """
        self.assertEqual(self.report["graphics"], self.layout["graphics"])
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertIsNone(self.report["roof"])
        self.assertIsNone(self.report["glass"])
        # Pixel/JSON equality identifies the exact original/native placement,
        # independently of a friendly description or a source-complete flag.
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        self.assertEqual(station.base.digest(ROOT / self.source["path"]), self.source["sha256"])

    def test_platform_ends_follow_compiled_native_strip_contacts(self):
        """Check both ends of each ledge from actual native parent boxes, not candidate captions.

        @param self Active focused source check instance.
        @return None.
        """
        back, front, roof = self.layout["parts"]
        self.assertIsNone(roof)
        far_y = back["origin"][1] + back["extent"][1]
        near_y = front["origin"][1]
        # Both actual platform strips must meet both through-station tile
        # boundaries, allowing only low-deck shading at native pixel scale.
        for world_x, world_y in ((0, far_y), (back["extent"][0], far_y),
                                 (0, near_y), (front["extent"][0], near_y)):
            px, py = 31 + 2 * (world_y - world_x), 33 + world_x + world_y
            self.assertTrue(any(self.whole.getpixel((x, y)) != 0 for x in range(px - 2, px + 3)
                                for y in range(py - 2, min(64, py + 3))), (world_x, world_y))

    def test_native_front_preserves_actual_rail_and_train_centre_clearance(self):
        """Ground and projected train centres stay clear of the actual front source parent.

        @param self Active focused source check instance.
        @return None.
        """
        back, front, _ = self.layout["parts"]
        track_y = (back["extent"][1] + front["origin"][1]) // 2
        # The train centre follows actual native X geometry. Ground contains
        # no baked track/floor and the foreground leaves all checked vehicle
        # heights clear; tall rear art remains in its separate behind-train owner.
        for world_x in range(back["extent"][0]):
            px, py = 31 + 2 * (track_y - world_x), 33 + world_x + track_y
            self.assertEqual(self.whole.getpixel((px, py)), 0)
            for height in range(9):
                self.assertEqual(self.parts["front"].getpixel((px, py - height)), 0)

    def test_parents_recombine_without_lost_or_duplicated_original_pixels(self):
        """Independent native parents preserve the whole original exactly and supply no fake roof.

        @param self Active focused source check instance.
        @return None.
        """
        # Every visible original pixel belongs to one physical component; a
        # copied whole sprite in both parents would double-draw the loader.
        for whole, back, front in zip(self.whole.get_flattened_data(), self.parts["back"].get_flattened_data(), self.parts["front"].get_flattened_data()):
            self.assertFalse(back != 0 and front != 0)
            self.assertEqual(back or front, whole)

    def test_rejected_first_models_cannot_supply_any_native_pose(self):
        """Unsafe framing or unverified geometry cannot be bypassed through direct conversion.

        @param self Active focused source check instance.
        @return None.
        """
        for version in (1, 2, 3):
            with self.assertRaisesRegex(ValueError, "rejected station original"):
                station.build(self.record, f"station-loader-x-base-v{version}")

    def test_changed_original_reference_or_view_registration_refuses(self):
        """Refuse lost provenance, a fake second axis or silent anchor changes.

        @param self Active focused source check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        next(row for row in record["candidates"] if row["id"] == self.identity)["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "original changed"):
            station.build(record, self.identity)
        record = copy.deepcopy(self.record)
        next(row for row in record["candidates"] if row["id"] == self.identity)["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "reference changed"):
            station.build(record, self.identity)
        # Neither the other native axis nor a shifted floor contact can be
        # supplied by relabeling the approved X image without genuine source.
        for key, value in (("axis", "Y"), ("native_box", [0, 15, 64, 64]), ("anchor", [-32, -32])):
            source = copy.deepcopy(self.source)
            source["native_transform"][key] = value
            with self.assertRaises(ValueError):
                station.native_base(source)

    def test_missing_platform_and_painted_rail_corridor_refuse(self):
        """A plausible loader cannot hide an absent front ledge or source pixels on real track.

        @param self Active focused source check instance.
        @return None.
        """
        without_front = self.whole.copy()
        without_front.putdata([0 if front else whole for whole, front in
                               zip(self.whole.get_flattened_data(), self.parts["front"].get_flattened_data())])
        with self.assertRaisesRegex(ValueError, "ledge contact"):
            station.validate_base(without_front)
        obstructed = self.whole.copy()
        opaque = next(value for value in self.whole.get_flattened_data() if value)
        obstructed.putpixel((31, 49), opaque)
        with self.assertRaisesRegex(ValueError, "rail corridor"):
            station.validate_base(obstructed)


if __name__ == "__main__":
    unittest.main()
