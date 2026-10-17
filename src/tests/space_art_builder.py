"""Verify original construction-carrier views and protected actual Steel/Goods loads.

@file space_art_builder.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BD
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63C
Source tests grant no native cargo, recipe inputs, work, service or runtime sprite acceptance.
"""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_load_parts as loads

ASSET = loads.ASSET


class BuilderLoadSourceTests(unittest.TestCase):
    """Require genuine carrier originals, actual material cues and immutable cargo-volume protection."""

    def setUp(self):
        """Read detached metadata for each independent source/refusal case."""
        self.document = art.read_record()
        self.family = self.document["families"][ASSET]
        key = self.family["load_sources"]["goods"]["N"]
        self.candidate = next(c for c in self.document["candidates"] if c["key"] == key)

    def test_all_genuine_views_preserve_the_observed_heading_correction(self):
        """Eight own originals use honest actual headings and the selected fixed native anchor."""
        self.assertEqual(self.family["directions"], ["N", "NE", "E", "SE", "S", "SW", "W", "NW"])
        self.assertEqual(self.family["public_anchor"], [-32, -32])
        # Every heading is checked against its own original registration; the generator's first S/SW views cannot be mislabeled as missing N/NE pictures.
        for direction in self.family["directions"]:
            key = self.family["selections"]["free/" + direction]
            candidate = art.source_candidate(key, self.document)
            self.assertEqual(candidate["asset_id"], ASSET)
            self.assertIn(direction, candidate["directions"])
            pose, _ = art.native_view(key, direction, self.document)
            self.assertEqual(pose.size, (64, 48))
            self.assertFalse((set(pose.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
        first = art.source_candidate(self.family["selections"]["free/S"], self.document)
        self.assertEqual(first["directions"], ["S", "SW", "E", "SE"])
        self.assertEqual(first["requested_directions"], ["N", "NE", "E", "SE"])
        self.assertFalse(self.family["runtime_activated"])

    def test_both_actual_loads_preserve_foreground_and_all_outside_pixels(self):
        """All16 load cues preserve original hardware, every old opaque pixel and exact original foreground arms."""
        # Independently compare all actual native pixels, including transparent air around the cradle. A generated whole-chassis load edit cannot pass by changing a convenient bounding rectangle.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            editable = {tuple(p) for p in self.family["load_edit_pixels"][direction]}
            poses = []
            for state in loads.STATES:
                pose, evidence = loads.load_pose(state, direction, self.document)
                self.assertGreaterEqual(evidence["changed_pixels"], 4)
                self.assertEqual(pose.getpalette(), base.getpalette())
                for y in range(48):
                    for x in range(64):
                        old, new = base.getpixel((x, y)), pose.getpixel((x, y))
                        if old != 0:
                            self.assertNotEqual(new, 0)
                        foreground = any(l <= x < r and t <= y < b for l, t, r, b in self.family["load_foreground_regions"][direction])
                        # Original manipulators stay in front of the cargo even where their projection meets the swept cargo volume.
                        if (x, y) not in editable or foreground:
                            self.assertEqual(old, new)
                poses.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(poses[0].get_flattened_data(), poses[1].get_flattened_data(), strict=True)), 4)

    def test_original_goods_frame_intrusion_is_not_misclassified_as_bad_alpha(self):
        """The untouched Goods original passes measured complete windows, while its initial assumed crop is refused."""
        art.source_pixels(self.candidate, "N", self.family["native_art_envelope"])
        transform = self.candidate["native_transforms"]["N"]
        transform["crop_box"] = [32, 32, 736, 480]
        transform["measured_subject_bbox_in_crop"] = [0, 0, 704, 448]
        with self.assertRaisesRegex(ValueError, "clearance"):
            art.source_pixels(self.candidate, "N", self.family["native_art_envelope"])

    def test_wrong_material_source_refuses(self):
        """A Steel sheet cannot silently supply Goods just because its source shape fits the same cradle."""
        self.family["load_sources"]["goods"]["N"] = self.family["load_sources"]["steel"]["N"]
        with self.assertRaisesRegex(ValueError, "declared"):
            loads.load_pose("goods", "N", self.document)

    def test_missing_heading_refuses(self):
        """An absent original cargo view cannot be supplied by rotation or a neighboring direction."""
        del self.family["load_sources"]["goods"]["N"]
        with self.assertRaisesRegex(ValueError, "heading"):
            loads.load_pose("goods", "N", self.document)

    def test_changed_prompt_receipt_refuses(self):
        """Actual executed source ancestry remains required beyond plausible load pixels."""
        self.candidate["generation_receipt"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            loads.load_pose("goods", "N", self.document)

    def test_widened_cargo_volume_refuses(self):
        """A later source change cannot add editable pixels beside the selected original load volume."""
        points = self.family["load_edit_pixels"]["N"]
        bounds = self.family["load_component_bounds"]["N"]
        extra = next([x, y] for y in range(bounds[1], bounds[3]) for x in range(bounds[0], bounds[2]) if [x, y] not in points)
        points.append(extra)
        with self.assertRaisesRegex(ValueError, "registration|foreground"):
            loads.load_pose("goods", "N", self.document)

    def test_foreground_arm_cannot_become_cargo(self):
        """An overlapping foreground region cannot authorize covering an original manipulator with a material cue."""
        x, y = self.family["load_edit_pixels"]["N"][0]
        self.family["load_foreground_regions"]["N"].append([x, y, x + 1, y + 1])
        with self.assertRaisesRegex(ValueError, "foreground"):
            loads.load_pose("steel", "N", self.document)

    def test_native_goods_count_cannot_become_an_invented_source_state(self):
        """Symbolic type art accepts only real Steel/Goods definitions; it never manufactures unit-count pictures."""
        with self.assertRaisesRegex(ValueError, "unsupported"):
            loads.load_pose("goods-20", "N", self.document)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
