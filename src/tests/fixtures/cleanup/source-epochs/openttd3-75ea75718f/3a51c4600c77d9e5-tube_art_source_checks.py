"""Focused straight-tube source completeness and conversion boundary checks.

@file tube_art_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Tests operate on in-memory malformed metadata/art; retained originals stay immutable.
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
import convert_tube_art as tube


class TubeArtSourceChecks(unittest.TestCase):
    """Exercise incomplete/changed-source refusals before a family can be packed.

    @see Plan13 task47B.3: each actual state/axis/part needs a checked source.
    """

    def setUp(self):
        """Load a fresh metadata copy so malformed cases cannot affect each other.

        @param self Current isolated unittest case.
        @return None; retain decoded original provenance in memory only.
        """
        self.document = json.loads(tube.RECORD.read_text(encoding="utf-8"))

    def test_complete_reproducible_source(self):
        """Check all twelve poses against retained originals and the published cache.

        @param self Current test case.
        @return None; the read-only proof also checks exact PNG/provenance regeneration.
        """
        report, encoded = tube.build_straight(self.document)
        self.assertEqual(report["pose_count"], 12)
        tube.validate_straight(report, encoded)
        tube.publish_straight(check=True)

    def test_missing_state_axis_refuses(self):
        """Reject a family lacking its independently authored unpowered Y view.

        @param self Current test case.
        @return None; an authored flag cannot fill a missing physical pose.
        """
        selections = self.document["families"][0]["selections"]
        selections[:] = [row for row in selections if (row["state"], row["direction"]) != ("unpowered", "SE")]
        with self.assertRaisesRegex(ValueError, "state/axis coverage"):
            tube.build_straight(self.document)

    def test_duplicate_candidate_refuses(self):
        """Reject ambiguous source selection even when both copies have valid hashes.

        @param self Current test case.
        @return None; cache/discovery order cannot choose an original implicitly.
        """
        self.document["candidates"].append(copy.deepcopy(self.document["candidates"][0]))
        with self.assertRaisesRegex(ValueError, "duplicate retained tube candidate"):
            tube.build_straight(self.document)

    def test_changed_original_identity_refuses(self):
        """Reject a selected original whose retained identity no longer matches.

        @param self Current test case.
        @return None; mutate the expected digest rather than writing source pixels.
        """
        self.document["candidates"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable tube source/reference changed"):
            tube.build_straight(self.document)

    def test_cross_state_substitution_refuses(self):
        """Prevent a powered original from standing in for an unpowered state.

        @param self Current test case.
        @return None; a complete pose count alone cannot establish actual authored coverage.
        """
        selection = next(row for row in self.document["families"][0]["selections"]
                         if (row["state"], row["direction"]) == ("unpowered", "NE"))
        selection["source"] = "tube-straight-base-v1"
        selection.pop("source_frame")
        with self.assertRaisesRegex(ValueError, "substituted across a state or axis"):
            tube.build_straight(self.document)

    def test_bad_anchor_refuses(self):
        """Reject an original transform that drifts from the native ground anchor.

        @param self Current test case.
        @return None; malformed placement never reaches deterministic packing.
        """
        self.document["candidates"][0]["native_transform"]["anchor"] = [-32, 0]
        with self.assertRaisesRegex(ValueError, "unsupported native straight transform"):
            tube.build_straight(self.document)

    def test_missing_front_refuses(self):
        """Reject an atlas mapping without its actual near rim.

        @param self Current test case.
        @return None; moving capsules must retain the intended foreground occlusion.
        """
        report, encoded = tube.build_straight(self.document)
        report["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "incomplete part coverage"):
            tube.validate_straight(report, encoded)

    def test_forbidden_palette_pixel_refuses(self):
        """Reject a reserved animated colour in otherwise valid indexed source art.

        @param self Current test case.
        @return None; corrupt only an in-memory atlas, preserving immutable originals.
        """
        report, encoded = tube.build_straight(self.document)
        with Image.open(io.BytesIO(encoded)) as atlas:
            broken = atlas.copy()
        broken.putpixel((0, 0), min(tube.base.FORBIDDEN - {0}))
        with self.assertRaisesRegex(ValueError, "forbidden palette indices"):
            tube.validate_straight(report, tube.indexed_png(broken))

    def test_actual_native_capsule_centreline_is_clear(self):
        """Check an actual capsule's native path through each authored lifecycle pose.

        @param self Current test case.
        @return None; independent native edge-contact coordinates must remain transparent.
        """
        points = {"NE": [(25, 19), (27, 18), (29, 17), (31, 16), (33, 15), (35, 14), (37, 13), (39, 12)],
                  "SE": [(25, 13), (27, 14), (29, 15), (31, 16), (33, 17), (35, 18), (37, 19), (39, 20)]}
        # These samples lie on the engine paths between NE(47,8)/SW(15,24)
        # and NW(15,8)/SE(47,24). A correctly tiled but shifted casing must not
        # hide a moving capsule or put its foreground wall over the path.
        for (state, direction), (_, _, image) in tube.straight_sources(self.document).items():
            with self.subTest(state=state, direction=direction):
                self.assertTrue(all(image.getpixel(point) == 0 for point in points[direction]))


if __name__ == "__main__":
    unittest.main()
