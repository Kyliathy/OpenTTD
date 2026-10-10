"""Focused native X/Y crossing source coverage and route-clearance checks.

@file tube_crossing_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47F.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed source reports/pixels exist only in memory; originals remain immutable.
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
import convert_tube_crossings as crossings


class TubeCrossingSourceChecks(unittest.TestCase):
    """Prove six actual native crossing parts and reject fictitious connections.

    @see Plan13 task47F.3; gameplay topology/reservations retain native ownership.
    """

    def setUp(self):
        """Decode isolated source metadata for each refusal case.

        @param self Current test instance.
        @return None; source hashes/public files stay unchanged.
        """
        self.document = json.loads(crossings.tube.RECORD.read_text(encoding="utf-8"))
        self.family = next(row for row in self.document["families"] if row["asset_id"] == "tube.crossing")

    def test_exact_source_regeneration(self):
        """Verify all six actual state/part poses and exact read-only proof.

        @param self Current test instance.
        @return None; actual original mappings/pixels and contacts must agree.
        """
        report, encoded = crossings.build(self.document)
        self.assertEqual(report["pose_count"], 6)
        crossings.validate(report, encoded, self.document)
        crossings.publish(check=True)

    def test_missing_construction_original_refuses(self):
        """Reject incomplete straight donor lifecycle rather than inventing paint.

        @param self Current test instance.
        @return None; every installed casing supplies its actual authored state.
        """
        family = next(row for row in self.document["families"] if row["asset_id"] == "tube.straight")
        family["selections"][:] = [row for row in family["selections"] if (row["state"], row["direction"]) != ("construction", "NE")]
        with self.assertRaisesRegex(ValueError, "state/axis coverage"):
            crossings.build(self.document)

    def test_extra_installed_corner_refuses(self):
        """Reject a corner donor that would turn the crossing into a junction.

        @param self Current test instance.
        @return None; a generic four-door hub cannot replace native X/Y.
        """
        self.family["native_tracks"].append("Upper")
        with self.assertRaisesRegex(ValueError, "source topology/state/part mismatch"):
            crossings.build(self.document)

    def test_wrong_native_mask_refuses(self):
        """Reject a shared-edge X/Upper mask substituted for actual X/Y.

        @param self Current test instance.
        @return None; native mask3 owns the only crossing layout.
        """
        self.family["trackbits"] = 5
        with self.assertRaisesRegex(ValueError, "source topology/state/part mismatch"):
            crossings.build(self.document)

    def test_changed_original_refuses(self):
        """Reject modified powered Y donor identity before composition.

        @param self Current test instance.
        @return None; mutate only the expected digest, never the source image.
        """
        source = next(row for row in self.document["candidates"] if row["key"] == "tube-straight-y-base-v1")
        source["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable tube source/reference changed"):
            crossings.build(self.document)

    def test_missing_front_refuses(self):
        """Reject count claims missing an actual source foreground state.

        @param self Current test instance.
        @return None; all real capsule approaches require aligned occlusion.
        """
        report, encoded = crossings.build(self.document)
        report["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "state/part coverage"):
            crossings.validate(report, encoded, self.document)

    def test_bad_anchor_refuses(self):
        """Reject a source report that drifts off the shared native ground anchor.

        @param self Current test instance.
        @return None; neighbour contacts require identical64x31 placement.
        """
        report, encoded = crossings.build(self.document)
        report["sprites"][0]["anchor"] = [-32, 0]
        with self.assertRaisesRegex(ValueError, "anchor/box mismatch"):
            crossings.validate(report, encoded, self.document)

    def test_false_donor_provenance_refuses(self):
        """Reject a fictitious opposite-view source identity in an intact atlas.

        @param self Current test instance.
        @return None; correct-looking pixels cannot certify invented provenance.
        """
        report, encoded = crossings.build(self.document)
        report["sprites"][0]["contributors"][1]["source_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "false contributor provenance"):
            crossings.validate(report, encoded, self.document)

    def test_actual_native_route_covered_refuses(self):
        """Reject repainting both layers over the actual crossing centreline.

        @param self Current test instance.
        @return None; native x31/y16 route clearance is independent of subset checks.
        """
        report, encoded = crossings.build(self.document)
        with Image.open(io.BytesIO(encoded)) as atlas:
            broken = atlas.copy()
        colour = next(value for value in broken.get_flattened_data() if value != 0)
        # Matching both layers defeats a simple source-subset comparison;
        # the independent actual native centreline must still reject the wall.
        for row in report["sprites"]:
            if row["state"] == "powered":
                x, y = row["box"][:2]
                broken.putpixel((x + 31, y + 16), colour)
        with self.assertRaisesRegex(ValueError, "foreground covers actual route"):
            crossings.validate(report, crossings.tube.indexed_png(broken), self.document)

    def test_painted_absent_arm_refuses(self):
        """Reject an added pixel absent from either original at that coordinate.

        @param self Current test instance.
        @return None; native crossing source cannot manufacture a floor/arm.
        """
        report, encoded = crossings.build(self.document)
        with Image.open(io.BytesIO(encoded)) as atlas:
            broken = atlas.copy()
        broken.putpixel((1, 1), next(value for value in broken.get_flattened_data() if value != 0))
        with self.assertRaisesRegex(ValueError, "changes original source pixels"):
            crossings.validate(report, crossings.tube.indexed_png(broken), self.document)

    def test_forbidden_palette_refuses(self):
        """Reject a reserved animated index in an otherwise valid atlas.

        @param self Current test instance.
        @return None; state art remains static authored DOS colour.
        """
        report, encoded = crossings.build(self.document)
        with Image.open(io.BytesIO(encoded)) as atlas:
            broken = atlas.copy()
        broken.putpixel((0, 0), min(crossings.tube.base.FORBIDDEN - {0}))
        with self.assertRaisesRegex(ValueError, "forbidden native crossing palette"):
            crossings.validate(report, crossings.tube.indexed_png(broken), self.document)


if __name__ == "__main__":
    unittest.main()
