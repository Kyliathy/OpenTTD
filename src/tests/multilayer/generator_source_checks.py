"""Focused original/native geometry checks for selected dated generator views.

@file generator_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md tasks55.1/55.2.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group8B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
These are source-conversion checks, not operating-generator or transported-bootstrap acceptance.
"""

import copy
import json
import sys
import unittest
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_generator_art as art


class GeneratorSourceChecks(unittest.TestCase):
    """Protect genuine selected views and exact whole-original cell ownership.

    @see r2mlvs13-graphics-and-asset-pipeline.md Group55.
    """

    def setUp(self):
        """Load independent metadata copies for safe isolated mutation cases.

        @return None; immutable source files are never modified by these checks.
        """
        self.record = json.loads(art.RECORD.read_text())
        self.public = json.loads(art.tube.PUBLIC.read_text())
        self.spec = next(row for row in self.public["assets"] if row["asset_id"] == "facility.vent-powerhouse")
        self.family = next(row for row in self.record["families"] if row["asset_id"] == self.spec["asset_id"])
        self.key = self.family["whole_view_selections"]["SE"]
        self.source = next(row for row in self.record["candidates"] if row["key"] == self.key)

    def test_all_native_views_reconstruct_complete_originals(self):
        """Independently composite every stored cell and check original conservation.

        @return None; each native pixel appears exactly once in the sixteen cells.
        """
        models = art.ready_models(self.spec["asset_id"], self.record, self.public)
        self.assertEqual(set(models), {"NE", "SE", "SW", "NW"})
        # Alpha compositing uses independently calculated native offsets, while
        # pixel counts detect duplicate ownership hidden by an identical overlay.
        for direction, (campus, pieces, source) in models.items():
            self.assertEqual(len(pieces), 16)
            rebuilt = Image.new("RGBA", (320, 224))
            opaque = 0
            for index, piece in enumerate(pieces):
                x, y = index % 4, index // 4
                rebuilt.alpha_composite(piece.convert("RGBA"), (129 + 32 * (y - x), 31 + 16 * (x + y)))
                opaque += sum(value != 0 for value in piece.get_flattened_data())
                self.assertIsNotNone(piece.getbbox(), (direction, x, y))
            original = campus.convert("RGBA")
            self.assertEqual(rebuilt.getchannel("A").tobytes(), original.getchannel("A").tobytes(), direction)
            # DOS index zero stores blue beneath alpha zero; an empty RGBA
            # canvas stores black there. Rendered colors and exact alpha must
            # match, while those invisible RGB values carry no source pixels.
            background = Image.new("RGBA", (320, 224), (220, 220, 220, 255))
            self.assertEqual(Image.alpha_composite(background, rebuilt).tobytes(),
                             Image.alpha_composite(background, original).tobytes(), direction)
            self.assertEqual(opaque, sum(value != 0 for value in campus.get_flattened_data()))
            self.assertEqual(source["cell_order"], [[x, y] for y in range(4) for x in range(4)])

    def test_original_conversion_is_repeatable(self):
        """Require exact native bytes from repeated conversion of retained originals.

        @return None; no procedural or random artwork is allowed into source conversion.
        """
        first, unused = art.native_base(self.key, self.record, self.spec)
        second, unused = art.native_base(self.key, self.record, self.spec)
        self.assertEqual(art.tube.indexed_png(first), art.tube.indexed_png(second))

    def test_reviewed_registration_cannot_shift(self):
        """Refuse a legal-canvas translation after visual approval.

        @return None; valid raw hashes alone do not authorize new physical placement.
        """
        self.source["native_position"][1] -= 1
        with self.assertRaisesRegex(ValueError, "reviewed original"):
            art.native_base(self.key, self.record, self.spec)

    def test_changed_original_identity_refused(self):
        """Refuse a changed original identity before any native artwork is sampled.

        @return None; tests change metadata only, preserving the actual immutable bytes.
        """
        self.source["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable"):
            art.native_base(self.key, self.record, self.spec)

    def test_changed_reference_identity_refused(self):
        """Refuse changed labelled geometry/style inputs as well as changed originals.

        @return None; retained prompt provenance must still match its real references.
        """
        self.source["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable"):
            art.native_base(self.key, self.record, self.spec)

    def test_native_geometry_mutations_refused(self):
        """Refuse copied source metadata that changes native ownership or cell geometry.

        @return None; source registration cannot substitute a different physical contract.
        """
        mutations = [("consumer", 16), ("footprint", [3, 3]), ("tile_variants", 15),
                     ("anchor", [-31, 0]), ("native_size", [64, 31]), ("geometry", "whole")]
        # Each mutation targets an independent producer boundary, rather than
        # merely comparing a receipt with a regenerated copy of itself.
        for field, value in mutations:
            with self.subTest(field=field):
                spec = copy.deepcopy(self.spec)
                spec[field] = value
                with self.assertRaisesRegex(ValueError, "verified native geometry"):
                    art.native_base(self.key, self.record, spec)

    def test_dated_native_layout_cannot_be_reinterpreted(self):
        """Refuse a changed model date or cell order behind unchanged sprite dimensions.

        @return None; the passive native producer remains the dated layout authority.
        """
        for field, value in [("introduction_year", 2000), ("world_cells", list(reversed(art.CELLS)))]:
            with self.subTest(field=field):
                spec = copy.deepcopy(self.spec)
                spec["geometry_notes"]["native_generator_geometry"][field] = value
                with self.assertRaisesRegex(ValueError, "verified native geometry"):
                    art.native_base(self.key, self.record, spec)

    def test_unapproved_crop_outside_real_original_refused(self):
        """Exercise the source bounds guard independently of the approval guard.

        @return None; transparent padding cannot conceal an invalid source window.
        """
        self.spec["geometry_notes"]["approved_generator_registrations"] = {}
        self.source["native_crop"][0] = -1
        with self.assertRaisesRegex(ValueError, "crop leaves"):
            art.native_base(self.key, self.record, self.spec)

    def test_missing_and_duplicate_selected_views_refused(self):
        """Refuse absent directions and repeated source keys before claiming four views.

        @return None; a completed SE original cannot fill another orientation slot.
        """
        self.family["whole_view_selections"].pop("NW")
        with self.assertRaisesRegex(ValueError, "four distinct"):
            art.ready_models(self.spec["asset_id"], self.record, self.public)
        self.family["whole_view_selections"]["NW"] = self.key
        with self.assertRaisesRegex(ValueError, "four distinct"):
            art.ready_models(self.spec["asset_id"], self.record, self.public)

    def test_rejected_drafts_remain_excluded(self):
        """Keep the duplicated-view sheet and wrong-axis SW draft outside selected source.

        @return None; retaining rejected provenance must never reactivate those pixels.
        """
        for key in ("venttap-views-draft-v1", "venttap-campus-sw-v1"):
            with self.subTest(key=key):
                with self.assertRaises(ValueError):
                    art.native_base(key, self.record, self.spec)

    def test_foreign_collector_original_cannot_fill_generator_view(self):
        """Refuse a genuine collector original substituted into a generator selection.

        @return None; source provenance alone does not establish correct family ownership.
        """
        self.family["whole_view_selections"]["NE"] = "collector-campus-ne-v1"
        with self.assertRaisesRegex(ValueError, "another source family"):
            art.ready_models(self.spec["asset_id"], self.record, self.public)

    def test_pixel_outside_native_cells_cannot_be_clipped(self):
        """Refuse an original pixel that no native cell can own.

        @return None; conversion must not silently discard unsupported silhouette extents.
        """
        campus = art.indexed_canvas((320, 224))
        campus.putpixel((0, 0), 1)
        with self.assertRaisesRegex(ValueError, "leaves native cells"):
            art.partition_model(campus, self.source)

    def test_absent_native_world_cells_cannot_be_hidden(self):
        """Refuse a small substitute drawing that leaves actual floor cells absent.

        @return None; sixteen real sections are required even when one pixel fits.
        """
        campus = art.indexed_canvas((320, 224))
        campus.putpixel((160, 80), 1)
        with self.assertRaisesRegex(ValueError, "native cells"):
            art.partition_model(campus, self.source)


if __name__ == "__main__":
    unittest.main()
