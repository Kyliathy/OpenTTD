"""Focused native tube corner source/foreground boundary checks.

@file tube_curve_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed cases change in-memory metadata/art only; retained originals remain immutable.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_tube_curves as curves


class TubeCurveSourceChecks(unittest.TestCase):
    """Reject incomplete source bindings and covered actual capsule paths.

    @see Plan13 task47C.3; source completeness is separate from runtime acceptance.
    """

    def setUp(self):
        """Prepare isolated decoded provenance for each malformed input.

        @param self Current test case.
        @return None; no original or published metadata is modified.
        """
        self.document = json.loads(curves.tube.RECORD.read_text(encoding="utf-8"))
        self.family = next(row for row in self.document["families"] if row["asset_id"] == "tube.bend45")

    def test_complete_reproducible_source(self):
        """Verify24 actual parts and exact source/contact regeneration.

        @param self Current test case.
        @return None; read-only regeneration must match the published proof.
        """
        report, encoded = curves.build(self.document)
        self.assertEqual(report["pose_count"], 24)
        curves.validate(report, encoded)
        curves.publish(check=True)

    def test_missing_corner_state_refuses(self):
        """Reject a seemingly complete family missing unpowered East.

        @param self Current test case.
        @return None; another view cannot substitute for the absent original.
        """
        self.family["selections"][:] = [row for row in self.family["selections"] if (row["state"], row["direction"]) != ("unpowered", "E")]
        with self.assertRaisesRegex(ValueError, "corner state coverage"):
            curves.build(self.document)

    def test_wrong_view_binding_refuses(self):
        """Reject reliance on the wrong row caption or an opposite native corner.

        @param self Current test case.
        @return None; actual retained geometry owns view identity.
        """
        row = next(row for row in self.family["selections"] if (row["state"], row["direction"]) == ("unpowered", "E"))
        row["source_frame"] = ["unpowered", "W"]
        with self.assertRaisesRegex(ValueError, "state/view substitution"):
            curves.build(self.document)

    def test_excluded_panel_change_refuses(self):
        """Keep the known model-changing North unpowered candidate excluded.

        @param self Current test case.
        @return None; fabricated transform labels cannot rehabilitate rejected pixels.
        """
        row = next(row for row in self.family["selections"] if (row["state"], row["direction"]) == ("unpowered", "N"))
        row.update(source="tube-curve-states-v1", source_frame=["unpowered", "N"])
        candidate = next(row for row in self.document["candidates"] if row["key"] == "tube-curve-states-v1")
        frame = copy.deepcopy(next(row for row in candidate["frames"] if (row["state"], row["corner"]) == ("unpowered", "S")))
        frame["corner"] = "N"
        candidate["frames"].append(frame)
        with self.assertRaisesRegex(ValueError, "excluded native tube corner frame"):
            curves.build(self.document)

    def test_changed_source_identity_refuses(self):
        """Reject changed selected-original bytes before source conversion.

        @param self Current test case.
        @return None; change the expected hash in memory, never the retained image.
        """
        candidate = next(row for row in self.document["candidates"] if row["key"] == "tube-curve-base-v1")
        candidate["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable tube source/reference changed"):
            curves.build(self.document)

    def test_bad_native_anchor_refuses(self):
        """Reject a corner offset that differs from its actual native ground tile.

        @param self Current test case.
        @return None; native contact continuity requires the same anchor for every part.
        """
        candidate = next(row for row in self.document["candidates"] if row["key"] == "tube-curve-base-v1")
        candidate["native_transform"]["anchor"] = [-32, 0]
        with self.assertRaisesRegex(ValueError, "unsupported native tube corner transform"):
            curves.build(self.document)

    def test_missing_foreground_refuses(self):
        """Reject a complete-count claim without one actual foreground part.

        @param self Current test case.
        @return None; every lifecycle/corner requires its own aligned near rim.
        """
        report, encoded = curves.build(self.document)
        report["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "corner part coverage"):
            curves.validate(report, encoded)

    def test_forbidden_palette_refuses(self):
        """Reject a reserved animated index in an otherwise valid source atlas.

        @param self Current test case.
        @return None; mutate only the encoded in-memory proof.
        """
        report, encoded = curves.build(self.document)
        with Image.open(io.BytesIO(encoded)) as image:
            broken = image.copy()
        broken.putpixel((0, 0), min(curves.tube.base.FORBIDDEN - {0}))
        with self.assertRaisesRegex(ValueError, "forbidden native tube corner palette"):
            curves.validate(report, curves.tube.indexed_png(broken))

    def test_covered_actual_native_chord_refuses(self):
        """Reject a foreground covering the actual native Left capsule path.

        @param self Current test case.
        @return None; independent x15/y16 vehicle coordinates detect a bad aperture.
        """
        report, encoded = curves.build(self.document)
        with Image.open(io.BytesIO(encoded)) as image:
            broken = image.copy()
        colour = next(value for value in broken.get_flattened_data() if value != 0)
        # Mutate both layers equally so ordinary subset/recomposition checks
        # still pass; only the independent actual vehicle-path check refuses.
        for sprite in report["sprites"]:
            if (sprite["state"], sprite["direction"]) == ("powered", "W"):
                x, y = sprite["box"][:2]
                broken.putpixel((x + 15, y + 16), colour)
        with self.assertRaisesRegex(ValueError, "foreground covers actual capsule path"):
            curves.validate(report, curves.tube.indexed_png(broken))


if __name__ == "__main__":
    unittest.main()
