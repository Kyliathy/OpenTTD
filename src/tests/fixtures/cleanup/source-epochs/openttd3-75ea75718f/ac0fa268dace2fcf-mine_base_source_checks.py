"""Focused original/source/native-placement proof for the fixed nine-cell Coal mine.

@file mine_base_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md tasks27.1/27.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
One actual native layout supplies eighteen independently owned base parts.
Source comparisons neither activate sprites nor complete construction/state art.
"""

import copy
import hashlib
import io
import json
from pathlib import Path
import statistics
import sys
import unittest

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests"))
import convert_mine_art as mine
import convert_material_mine_art as material
import multilayer_sprite_contract as shared


class MineBaseSourceChecks(unittest.TestCase):
    """Require genuine native cells, exact compatible reuse and unchanged prior delivery identities."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the active completed Coal base once from its retained original ancestry.

        @param cls Test class retaining actual native/public/source evidence.
        @return None.
        """
        cls.record = json.loads(mine.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(mine.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == "mine.COAL")
        cls.report, cls.files = mine.build(cls.record, cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[mine.OUTPUT / f"{mine.PREFIX}-atlas.png"]))
        cls.before = json.loads((ROOT / "logs/graphics/coal-mine/20261012-base-01/before.json").read_text(encoding="utf-8"))

    def sprite(self, variant, part=0):
        """Read one actual independent native owner from the base atlas.

        @param self Active completed base proof.
        @param variant Real row-major machinery cell0..8.
        @param part Back0 or independently sorted upper-front1.
        @return Indexed64x64 original native source parent.
        """
        x = (variant * 2 + part) * 64
        return self.atlas.crop((x, 0, x + 64, 64))

    def test_exact_regeneration_and_all_eighteen_real_native_source_parts(self):
        """One actual fixed layout retains every distinct role and complete independent part coverage.

        @param self Active completed base proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        self.assertEqual(self.atlas.size, (1152, 64))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertEqual(len(self.report["source_parts"]), 18)
        self.assertEqual(self.report["directions"], ["none"])
        self.assertEqual(self.report["native_layout"], 0)
        self.assertEqual(self.report["footprint"], [3, 3])
        self.assertEqual(len({self.sprite(variant).tobytes() for variant in range(9)}), 9)
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertIsNone(self.public["packed"]["offset"])
        self.assertEqual(shared.validate_manifest(self.document), 48893)
        # Actual row-major cells own all parts. A duplicated pose or renamed
        # four-direction sheet cannot replace the real native machinery matrix.
        expected = {(variant, part, variant % 3, variant // 3) for variant in range(9) for part in ("back", "front")}
        actual = {(row["tile_variant"], row["part"], *row["world_cell"]) for row in self.report["source_parts"]}
        self.assertEqual(actual, expected)

    def test_selected_originals_have_safe_complete_rectangles_and_uniform_native_scale(self):
        """All eight new original cells retain their complete clean crop and one native registration.

        @param self Active completed base proof.
        @return None.
        """
        # Inspect original alpha and frame colors before palette conversion can
        # disguise contamination. Whole component scaling has no painted/cut
        # rescue; all native model pixels come from the retained raw original.
        for selection in self.record["families"]["mine.COAL"]["base_cells"]:
            source, component = mine.component_record(self.record, selection)
            crop = Image.open(ROOT / source["path"]).convert("RGBA").crop(component["source_window"])
            bounds = crop.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
            self.assertEqual(list(bounds), component["measured_alpha16_bbox"])
            left, top, right, bottom = bounds
            self.assertGreaterEqual(min(left, top, crop.width - right, crop.height - bottom), 25)
            self.assertFalse(any(alpha >= 16 and red > 220 and green < 60 and blue > 220 for red, green, blue, alpha in crop.get_flattened_data()))
            x0, y0, x1, y1 = component["native_box"]
            self.assertEqual((x0, x1, y1), (1, 63, 64))
            self.assertLess(abs(((x1 - x0) / (right - left)) / ((y1 - y0) / (bottom - top)) - 1), .02)

    def test_actual_native_floor_contacts_south_tip_palette_and_twenty_high_envelope(self):
        """Every whole source meets its real paid floor without shifted contacts or excessive height.

        @param self Active completed base proof.
        @return None.
        """
        self.assertFalse(set(self.atlas.get_flattened_data()) & (mine.base.FORBIDDEN - {0}))
        # Check actual native source pixels independently of guide labels.
        # A skewed south tip can overlap a tolerant corner region while still
        # shifting the whole plant; the last opaque row must remain centered.
        for variant in range(9):
            sprite = self.sprite(variant)
            for px, py in ((31, 33), (1, 48), (31, 63), (61, 48)):
                self.assertTrue(any(sprite.getpixel((x, y)) for x in range(max(0, px - 2), min(64, px + 3))
                                    for y in range(max(0, py - 2), min(64, py + 3))), (variant, px, py))
            left, top, right, bottom = sprite.getbbox()
            south = statistics.mean(x for x in range(left, right) if sprite.getpixel((x, bottom - 1)))
            self.assertLessEqual(abs(south - 31), 2, variant)
            for y in range(64):
                for x in range(64):
                    if sprite.getpixel((x, y)):
                        self.assertGreaterEqual(y, 13 + abs(x - 31) / 2 - 2, (variant, x, y))
                        self.assertLessEqual(y, 64 - abs(x - 31) / 2 + 2, (variant, x, y))

    def test_native_upper_parts_preserve_exact_original_pixels_and_reconstruction(self):
        """Independent native sorting keeps the existing M1 underlay/upper-subset convention exact.

        @param self Active completed base proof.
        @return None.
        """
        # Upper parts may repeat original underlay pixels for native sorting,
        # but cannot paint new material or lose a required machinery owner.
        for variant in range(9):
            back, front = self.sprite(variant), self.sprite(variant, 1)
            self.assertIsNotNone(back.getbbox())
            self.assertIsNotNone(front.getbbox())
            reconstructed = [new if new else old for old, new in zip(back.get_flattened_data(), front.get_flattened_data())]
            self.assertEqual(bytes(reconstructed), back.tobytes())
            self.assertTrue(all(new == 0 or new == old for old, new in zip(back.get_flattened_data(), front.get_flattened_data())))
        campus = Image.open(io.BytesIO(self.files[mine.OUTPUT / f"{mine.PREFIX}-campus.png"]))
        self.assertEqual(campus.size, (256, 160))
        self.assertEqual(campus.info["transparency"], 0)

    def test_control_hut_reuse_preserves_actual_native_original_and_refuses_foreign_geometry(self):
        """One compatible retained component reproduces its exact native M1 control-cell pixels.

        @param self Active completed base proof.
        @return None.
        """
        reuse = self.record["families"]["mine.COAL"]["control_reuse"]
        self.assertEqual(mine.base.digest(ROOT / reuse["source_path"]), reuse["source_sha256"])
        self.assertEqual(mine.base.digest(ROOT / reuse["source_native_atlas"]), reuse["source_native_sha256"])
        with Image.open(ROOT / reuse["source_native_atlas"]) as atlas:
            self.assertEqual(self.sprite(7).tobytes(), atlas.crop((640, 256, 704, 320)).tobytes())
        for field, value in (("source_variant", 3), ("target_variant", 6), ("source_native_canvas", [128, 64]),
                             ("source_sha256", "0" * 64), ("source_native_sha256", "0" * 64)):
            record = copy.deepcopy(self.record)
            record["families"]["mine.COAL"]["control_reuse"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                mine.control_cell(record)

    def test_missing_duplicate_changed_original_and_shifted_registration_refuse(self):
        """Source matrices and immutable geometry prevent quiet model or placement substitution.

        @param self Active completed base proof.
        @return None.
        """
        for change in ("missing", "duplicate", "registration", "original", "reference"):
            record = copy.deepcopy(self.record)
            selections = record["families"]["mine.COAL"]["base_cells"]
            source, component = mine.component_record(record, selections[0])
            if change == "missing":
                selections.pop()
            elif change == "duplicate":
                selections[-1] = copy.deepcopy(selections[0])
            elif change == "registration":
                component["native_box"][0] += 1
            elif change == "original":
                source["sha256"] = "0" * 64
            else:
                source["references"][0]["sha256"] = "0" * 64
            with self.subTest(change=change), self.assertRaises(ValueError):
                mine.build(record, self.public)
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload)

    def test_excluded_tall_cell_skewed_floor_and_bad_material_refuse(self):
        """Unsafe originals cannot inherit approval from neighboring independent source components.

        @param self Active completed base proof.
        @return None.
        """
        selection = {"variant": 5, "source": "coal-machinery-b-v1", "component": "cell-4"}
        with self.assertRaisesRegex(ValueError, "excluded"):
            mine.original_cell(self.record, selection, mine.mine_contract("COAL"))
        # Corrupt actual native pixels rather than merely a metadata caption:
        # moved south contact, a tall tower and forbidden palette each refuse.
        for change in ("south", "height", "palette"):
            sprite = self.sprite(0).copy()
            if change == "south":
                for x in range(64):
                    sprite.putpixel((x, 63), 0)
                sprite.putpixel((40, 63), 15)
            elif change == "height":
                sprite.putpixel((31, 0), 15)
            else:
                sprite.putpixel((31, 33), next(iter(mine.base.FORBIDDEN - {0})))
            with self.subTest(change=change), self.assertRaises(ValueError):
                mine.validate_cell(sprite, 0)

    def test_all_prior_source_payload_binary_and_other_family_identities_stay_exact(self):
        """Base authoring changes only the selected Coal row and new Coal artifacts.

        @param self Active completed base proof.
        @return None.
        """
        for row in self.before["preserved_identities"]:
            self.assertEqual(mine.base.digest(ROOT / row["path"]), row["sha256"], row["path"])
        for row in self.document["assets"]:
            if row["asset_id"] == "mine.COAL":
                continue
            # Later selected material sources may replace their own planned
            # metadata only through a complete exact live native source handoff.
            if material.validate_following_source(row):
                continue
            self.assertEqual(hashlib.sha256(json.dumps(row, sort_keys=True).encode()).hexdigest(),
                             self.before["public_family_sha256"][row["asset_id"]], row["asset_id"])


if __name__ == "__main__":
    unittest.main()
