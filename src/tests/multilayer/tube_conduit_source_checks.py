"""Focused actual floor-cable branch, state and provenance boundary checks.

@file tube_conduit_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47G.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed reports/pixels stay in memory; source art owns no power or physical path.
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
import convert_tube_conduits as conduits


class TubeConduitSourceChecks(unittest.TestCase):
    """Reject missing/painted cable branches and false authored lifecycle claims.

    @see Plan13 task47G.3; TubePowerConnection remains the physical corridor owner.
    """

    def setUp(self):
        """Decode isolated retained provenance for each refusal case.

        @param self Current test instance.
        @return None; no repository input or public source metadata is modified.
        """
        self.document = json.loads(conduits.tube.RECORD.read_text(encoding="utf-8"))
        self.family = next(row for row in self.document["families"] if row["asset_id"] == "tube.conduit")

    def test_complete_exact_source_regeneration(self):
        """Verify all twelve true state/physical-branch bodies and exact proof.

        @param self Current test instance.
        @return None; exact pixels, original mappings and contacts regenerate read-only.
        """
        report, encoded = conduits.build(self.document)
        self.assertEqual(report["pose_count"], 12)
        conduits.validate(report, encoded, self.document)
        conduits.publish(check=True)

    def test_opposite_halves_retain_whole_original(self):
        """Verify complementary path halves restore every source pixel unchanged.

        @param self Current test instance.
        @return None; no absent opposite arm may survive outside the central sleeve.
        """
        originals = conduits.sources(self.document)
        # Both independent axes and every lifecycle must conserve their exact
        # source silhouette/palette. The selector cannot paint a missing arm.
        for state in conduits.tube.STATES:
            for axis, forward, reverse in (("X", "NE", "SW"), ("Y", "SE", "NW")):
                whole = originals[state, axis][2]
                first, second = conduits.branch(whole, forward), conduits.branch(whole, reverse)
                joined = [a if a != 0 else b for a, b in zip(first.get_flattened_data(), second.get_flattened_data())]
                self.assertEqual(joined, list(whole.get_flattened_data()))
                self.assertFalse(any(first.getpixel((x, y)) for x in range(29) for y in range(31)))
                self.assertFalse(any(second.getpixel((x, y)) for x in range(34, 64) for y in range(31)))

    def test_missing_unpowered_axis_refuses(self):
        """Reject a missing authored unpowered Y source rather than recolouring X.

        @param self Current test instance.
        @return None; six true state/axis originals precede branch selection.
        """
        self.family["axis_selections"][:] = [row for row in self.family["axis_selections"] if (row["state"], row["axis"]) != ("unpowered", "Y")]
        with self.assertRaisesRegex(ValueError, "state/axis coverage"):
            conduits.build(self.document)

    def test_missing_physical_branch_refuses(self):
        """Reject a report missing one actual NW floor connection state.

        @param self Current test instance.
        @return None; a complete count cannot replace missing physical geometry.
        """
        report, encoded = conduits.build(self.document)
        report["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "branch/state coverage"):
            conduits.validate(report, encoded, self.document)

    def test_state_substitution_refuses(self):
        """Reject the powered source labeled as unpowered cable art.

        @param self Current test instance.
        @return None; exact authored lifecycle selection is part of source authority.
        """
        row = next(row for row in self.family["axis_selections"] if (row["state"], row["axis"]) == ("unpowered", "X"))
        row.pop("source_frame")
        row["source"] = "tube-conduit-x-base-v1"
        with self.assertRaisesRegex(ValueError, "state/axis substitution"):
            conduits.build(self.document)

    def test_wrong_actual_axis_refuses(self):
        """Reject true Y pixels falsely labeled as X despite identical bounds.

        @param self Current test instance.
        @return None; independent native line geometry detects the wrong slope.
        """
        whole = conduits.sources(self.document)["powered", "Y"][2]
        with self.assertRaisesRegex(ValueError, "wrong axis or a tall floor obstruction"):
            conduits.validate_axis(whole, "X")

    def test_tall_floor_obstruction_refuses(self):
        """Reject a wall/slab pixel away from the actual thin floor cable.

        @param self Current test instance.
        @return None; malformed art is kept only in memory.
        """
        whole = conduits.sources(self.document)["powered", "X"][2].copy()
        whole.putpixel((31, 7), next(value for value in whole.get_flattened_data() if value != 0))
        with self.assertRaisesRegex(ValueError, "wrong axis or a tall floor obstruction"):
            conduits.validate_axis(whole, "X")

    def test_changed_original_refuses(self):
        """Reject a changed retained Y original before cropping or quantization.

        @param self Current test instance.
        @return None; change only the expected hash, never original image bytes.
        """
        row = next(row for row in self.document["candidates"] if row["key"] == "tube-conduit-y-base-v1")
        row["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable tube source/reference changed"):
            conduits.build(self.document)

    def test_false_foreground_contract_refuses(self):
        """Reject a foreground wall invented for the actual floor service path.

        @param self Current test instance.
        @return None; cable draw order remains below vehicles and facilities.
        """
        self.family["parts"] = ["back", "front"]
        with self.assertRaisesRegex(ValueError, "branch/ground contract mismatch"):
            conduits.build(self.document)

    def test_false_provenance_refuses(self):
        """Reject a fabricated source identity behind an otherwise intact atlas.

        @param self Current test instance.
        @return None; correct-looking proof pixels cannot bless changed inputs.
        """
        report, encoded = conduits.build(self.document)
        report["sprites"][0]["source_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "false original provenance"):
            conduits.validate(report, encoded, self.document)

    def test_bad_anchor_refuses(self):
        """Reject a branch offset that loses the shared native floor contact.

        @param self Current test instance.
        @return None; all12 source bodies retain anchor(-31,0).
        """
        report, encoded = conduits.build(self.document)
        report["sprites"][0]["anchor"] = [-32, 0]
        with self.assertRaisesRegex(ValueError, "anchor/box mismatch"):
            conduits.validate(report, encoded, self.document)

    def test_painted_absent_arm_refuses(self):
        """Reject an opposite half painted into a directed NE source branch.

        @param self Current test instance.
        @return None; output may contain only selected original branch pixels.
        """
        report, encoded = conduits.build(self.document)
        with Image.open(io.BytesIO(encoded)) as atlas:
            broken = atlas.copy()
        row = next(row for row in report["sprites"] if (row["state"], row["direction"]) == ("powered", "NE"))
        x, y = row["box"][:2]
        broken.putpixel((x + 15, y + 24), next(value for value in broken.get_flattened_data() if value != 0))
        with self.assertRaisesRegex(ValueError, "changes original branch pixels"):
            conduits.validate(report, conduits.tube.indexed_png(broken), self.document)

    def test_forbidden_palette_refuses(self):
        """Reject a reserved animated index in static ground cable source.

        @param self Current test instance.
        @return None; alter only an in-memory encoded proof.
        """
        report, encoded = conduits.build(self.document)
        with Image.open(io.BytesIO(encoded)) as atlas:
            broken = atlas.copy()
        broken.putpixel((0, 0), min(conduits.tube.base.FORBIDDEN - {0}))
        with self.assertRaisesRegex(ValueError, "forbidden native conduit palette"):
            conduits.validate(report, conduits.tube.indexed_png(broken), self.document)


if __name__ == "__main__":
    unittest.main()
