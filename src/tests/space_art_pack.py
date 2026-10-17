"""Verify final Space append, exact crop anchors and source corruption refusal.

@file space_art_pack.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG
Read-only source fixtures; actual decoded GRF loading has a separate native gate.
"""
import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import pack_space_art as art


class SpacePackingTests(unittest.TestCase):
    """Check appended actual source pixels without mutating earlier native art.

    @see r2mlvs10-asteroids-and-mining-probes.md Group14BG.1
    """

    @classmethod
    def setUpClass(cls):
        """Derive both independent clean input conversions once for this file.

        @param cls Test class retaining bounded exact source/packing observations.
        """
        cls.public = json.loads(art.PUBLIC.read_text(encoding="utf-8"))
        cls.report, cls.files = art.derive(cls.public)
        cls.second_report, cls.second_files = art.derive(cls.public)
        cls.families = {row["asset_id"]: row for row in cls.public["assets"]}
        family = cls.families["asteroid.nickel"]
        cls.asteroid_report = json.loads((art.ROOT / family["authoring"]["atlas_manifest"]).read_text())

    def test_clean_conversions_and_protected_outputs_are_identical(self):
        """Keep exact old inputs while independently deriving the new append.

        @param self Test instance comparing actual output byte mappings.
        """
        self.assertEqual(self.report, self.second_report)
        self.assertEqual(self.files, self.second_files)
        art.read_scope()

    def test_exact_native_ranges_follow_all_original_slots(self):
        """Allocate seven complete families without overlapping the preview prefix.

        @param self Test instance checking all1568 actual native references.
        """
        self.assertEqual(self.report["pose_count"], 1568)
        self.assertEqual(self.report["native_start"], 3599)
        self.assertEqual(self.report["native_sprite_bound"], 5167)
        self.assertEqual([row["native_offset"] for row in self.report["sprites"]], list(range(3599, 5167)))
        self.assertEqual([row["native_offset"] for row in self.report["families"]], [3599, 3727, 3823, 3951, 4015, 4399, 4783])

    def test_every_cropped_sprite_restores_exact_source_pixels(self):
        """Round-trip actual indexed body pixels and every original world anchor.

        @param self Test instance comparing the shared packer's cropped output.
        """
        scope = art.read_scope()
        packed = Image.open(io.BytesIO(self.files[art.ROOT / scope["png"]]))
        originals = {}
        # Transparent margin removal may not shift a craft hook, natural-body center or internal transparent hole; reconstruct every declared logical pose on its original canvas.
        for row in self.report["sprites"]:
            family = self.families[row["asset_id"]]
            path = row["source_atlas"]
            if path not in originals:
                originals[path] = Image.open(art.ROOT / path)
            x, y, width, height = row["box"]
            cropped = packed.crop((x, y, x + width, y + height))
            restored = Image.new("P", family["native_size"], 0)
            rx, ry = row["anchor"]
            restored.paste(cropped, (rx - family["anchor"][0], ry - family["anchor"][1]))
            x, y, width, height = row["source_box"]
            expected = originals[path].crop((x, y, x + width, y + height))
            self.assertEqual(restored.tobytes(), expected.tobytes(), row["native_offset"])

    def test_changed_source_review_cannot_enter_existing_range(self):
        """Refuse a modified handoff before any plausible native atlas is packed.

        @param self Test instance changing only a detached public manifest.
        """
        public = copy.deepcopy(self.public)
        next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-15")["authoring"]["atlas_manifest_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "source handoff"):
            art.derive(public)

    def test_missing_asteroid_view_cannot_hide_behind_count(self):
        """Require exact saved-size/view/state/frame identities in source coverage.

        @param self Test instance removing one isolated report row.
        """
        report = copy.deepcopy(self.asteroid_report)
        report["entries"].pop()
        with self.assertRaisesRegex(ValueError, "identity matrix"):
            art.asteroid_poses(self.families["asteroid.nickel"], report, {})

    def test_changed_asteroid_bounds_or_anchor_refuse(self):
        """Reject source cell or world-anchor drift independently of atlas counts.

        @param self Test instance corrupting detached source geometry only.
        """
        # Both a moved source box and a moved world anchor can make plausible pixels appear in the wrong physical location; each must fail before native packing.
        for field in ("box", "anchor"):
            report = copy.deepcopy(self.asteroid_report)
            report["entries"][0][field][0] += 1
            with self.assertRaisesRegex(ValueError, "source cell"):
                art.asteroid_poses(self.families["asteroid.nickel"], report, {})

    def test_reserved_palette_color_refuses(self):
        """Keep material pixels out of reserved company/animation color ranges.

        @param self Test instance altering a detached tiny indexed corruption input.
        """
        image = Image.new("P", (2, 2), 0)
        image.putpalette(art.packing.base.PALETTE_BYTES)
        image.info["transparency"] = 0
        image.putpixel((0, 0), max(art.packing.base.FORBIDDEN))
        with self.assertRaisesRegex(ValueError, "reserved source palette"):
            art.packing.validate_source_image(image, "asteroid.nickel")

    def test_existing_append_binding_cannot_move_into_preview(self):
        """Preserve staged native IDs independently of definition-local offsets.

        @param self Test instance attempting an isolated native range relocation.
        """
        public = copy.deepcopy(self.public)
        next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-15")["packed"]["native_offset"] = 3598
        with self.assertRaisesRegex(ValueError, "native Space binding"):
            art.derive(public)


if __name__ == "__main__":
    unittest.main()
