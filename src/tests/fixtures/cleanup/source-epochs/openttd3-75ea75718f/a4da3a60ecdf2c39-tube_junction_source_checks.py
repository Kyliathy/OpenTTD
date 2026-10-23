"""Focused original-pixel and exact native topology checks for tube junction art.

@file tube_junction_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only in-memory malformed art/metadata is changed; retained originals stay immutable.
"""

import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_tube_junctions as junction


class TubeJunctionSourceChecks(unittest.TestCase):
    """Require exact installed contributors and source pixels for every junction.

    @see Plan13 task47D.3: source proof does not certify later packed/runtime acceptance.
    """

    def setUp(self):
        """Load isolated source metadata for each malformed case.

        @param self Current test case.
        @return None; no original/public record is written.
        """
        self.document = json.loads(junction.tube.RECORD.read_text(encoding="utf-8"))

    def test_complete_reproducible_source(self):
        """Check all54 native topologies,324 parts and exact contact regeneration.

        @param self Current test case.
        @return None; every source pixel and actual foreground path is inspected.
        """
        report, encoded = junction.build(self.document)
        self.assertEqual(report["pose_count"], 324)
        junction.validate(report, encoded)
        junction.publish(check=True)

    def test_nonjunction_and_unknown_masks_refuse(self):
        """Keep crossing, independent curves, singles and invalid bits out of junctions.

        @param self Current test case.
        @return None; no alternate render case acquires a fictitious shared arm.
        """
        # These are native empty/single/crossing/parallel cases plus unknown
        # depot/wormhole bits. Their refusal prevents an overbroad selector.
        for mask in (0, 1, 2, 3, 12, 48, 64, 127):
            with self.subTest(mask=mask), self.assertRaisesRegex(ValueError, "not a native shared-edge"):
                junction.track_indices(mask)

    def test_actual_three_mouth_topologies(self):
        """Check independent known native edge pairs rather than four-door symmetry.

        @param self Current test case.
        @return None; each three-mouth layout excludes its actual absent edge.
        """
        expected = {5: {"NE", "SW", "NW"}, 9: {"NE", "SW", "SE"},
                    18: {"NW", "SE", "SW"}, 36: {"NW", "NE", "SE"}}
        # Native X+Upper, X+Lower, Y+Left and Upper+Right exercise each missing
        # mouth from independently frozen TrackdirToExitdir table identities.
        for mask, ports in expected.items():
            self.assertEqual(junction.installed_ports(mask), ports)

    def test_missing_authored_state_refuses(self):
        """Reject a family that lost one required original lifecycle state.

        @param self Current test case.
        @return None; conversion cannot expand an incomplete declared source contract silently.
        """
        family = next(row for row in self.document["families"] if row["asset_id"] == "tube.junction")
        family["states"].pop()
        with self.assertRaisesRegex(ValueError, "topology/source contract mismatch"):
            junction.build(self.document)

    def test_changed_original_identity_refuses(self):
        """Reject changed original identities before composing their source pixels.

        @param self Current test case.
        @return None; mutate the expected hash in memory, preserving the actual original.
        """
        self.document["candidates"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable tube source/reference changed"):
            junction.build(self.document)

    def test_false_primitive_provenance_refuses(self):
        """Require the proof metadata to identify the actual immutable contributors.

        @param self Current test case.
        @return None; matching pixels cannot authorize a forged source digest.
        """
        report, encoded = junction.build(self.document)
        report["primitive_frames"]["powered:X"]["source_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "primitive provenance mismatch"):
            junction.validate(report, encoded)

    def test_absent_track_contributor_refuses(self):
        """Prevent a generic extra arm from entering an exact X/Upper junction.

        @param self Current test case.
        @return None; actual TrackBits control the entire contributing set.
        """
        report, encoded = junction.build(self.document)
        row = report["sprites"][0]
        self.assertEqual(row["native_track_bits"], 5)
        row["source_frames"].append("construction:Y")
        with self.assertRaisesRegex(ValueError, "absent native track or arm"):
            junction.validate(report, encoded)

    def test_missing_part_refuses(self):
        """Reject incomplete topology/state/foreground coverage.

        @param self Current test case.
        @return None; a family-level authored status cannot replace a missing part.
        """
        report, encoded = junction.build(self.document)
        report["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "junction pose coverage"):
            junction.validate(report, encoded)

    def test_bad_anchor_refuses(self):
        """Reject a foreground/whole crop that drifts from its physical native tile.

        @param self Current test case.
        @return None; all topology variants share the same native ground anchor.
        """
        report, encoded = junction.build(self.document)
        report["sprites"][0]["anchor"] = [-32, 0]
        with self.assertRaisesRegex(ValueError, "anchor/variant box mismatch"):
            junction.validate(report, encoded)

    def test_painted_or_shifted_pixel_refuses(self):
        """Reject art absent at that native coordinate from every installed original.

        @param self Current test case.
        @return None; deterministic composition selects pixels instead of drawing replacements.
        """
        report, encoded = junction.build(self.document)
        with Image.open(io.BytesIO(encoded)) as image:
            broken = image.copy()
        colour = next(value for value in broken.get_flattened_data() if value != 0)
        broken.putpixel((63, 30), colour)
        with self.assertRaisesRegex(ValueError, "nonoriginal or uninstalled pixel"):
            junction.validate(report, junction.tube.indexed_png(broken))

    def test_covered_actual_left_chord_refuses(self):
        """Reject an original casing pixel placed over an actual native capsule path.

        @param self Current test case.
        @return None; x15/y18 comes directly from native Left movement, independently of the compositor.
        """
        report, encoded = junction.build(self.document)
        sources = junction.primitive_sources(self.document)
        colour = sources["powered", 4]["whole"][18 * 64 + 15]
        self.assertNotEqual(colour, 0)
        with Image.open(io.BytesIO(encoded)) as image:
            broken = image.copy()
        # The pixel is authentic and both parts retain the same colour. Only
        # the independent actual-path aperture check must refuse this cover.
        for row in report["sprites"]:
            if (row["state"], row["native_track_bits"]) == ("powered", 17):
                x, y = row["box"][:2]
                broken.putpixel((x + 15, y + 18), colour)
        with self.assertRaisesRegex(ValueError, "foreground covers an installed native path"):
            junction.validate(report, junction.tube.indexed_png(broken))

    def test_forbidden_palette_refuses(self):
        """Reject reserved animated pixels in an otherwise valid indexed atlas.

        @param self Current test case.
        @return None; no source or published artifact is changed by the malformed case.
        """
        report, encoded = junction.build(self.document)
        with Image.open(io.BytesIO(encoded)) as image:
            broken = image.copy()
        broken.putpixel((0, 0), min(junction.tube.base.FORBIDDEN - {0}))
        with self.assertRaisesRegex(ValueError, "forbidden palette index"):
            junction.validate(report, junction.tube.indexed_png(broken))


if __name__ == "__main__":
    unittest.main()
