"""Focused physical tube end source and provenance refusal checks.

@file tube_end_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47E.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed inputs remain in memory; immutable source originals are never changed.
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
import convert_tube_ends as ends


class TubeEndSourceChecks(unittest.TestCase):
    """Verify actual cap geometry and refuse fabricated coverage/provenance.

    @see Plan13 task47E.3; runtime native endpoint selection has its own fixture.
    """

    def setUp(self):
        """Decode isolated retained metadata for each refusal case.

        @param self Current test instance.
        @return None; no repository source or public metadata is modified.
        """
        self.document = json.loads(ends.tube.RECORD.read_text(encoding="utf-8"))
        self.family = next(row for row in self.document["families"] if row["asset_id"] == "tube.end")

    def test_complete_reproducible_source(self):
        """Check twelve authored whole caps and their24 aligned parts.

        @param self Current test instance.
        @return None; exact PNG/contact/provenance must regenerate read-only.
        """
        report, encoded = ends.build(self.document)
        self.assertEqual(report["pose_count"], 24)
        ends.validate(report, encoded, self.document)
        ends.publish(check=True)

    def test_missing_physical_facing_refuses(self):
        """Reject a family missing the actual unpowered NW cap.

        @param self Current test instance.
        @return None; another directed view cannot fill the gap.
        """
        self.family["selections"][:] = [row for row in self.family["selections"] if (row["state"], row["direction"]) != ("unpowered", "NW")]
        with self.assertRaisesRegex(ValueError, "end state coverage"):
            ends.build(self.document)

    def test_state_substitution_refuses(self):
        """Reject a powered original labeled as unpowered source.

        @param self Current test instance.
        @return None; source identity must agree with the public lifecycle.
        """
        row = next(row for row in self.family["selections"] if (row["state"], row["direction"]) == ("unpowered", "NE"))
        row.pop("source_frame")
        row["source"] = "tube-end-base-v1"
        with self.assertRaisesRegex(ValueError, "state/view substitution"):
            ends.build(self.document)

    def test_changed_original_refuses(self):
        """Reject a changed retained-original identity before quantization.

        @param self Current test instance.
        @return None; only the expected in-memory digest is modified.
        """
        candidate = next(row for row in self.document["candidates"] if row["key"] == "tube-end-base-v1")
        candidate["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable tube source/reference changed"):
            ends.build(self.document)

    def test_rejected_repeated_facing_refuses(self):
        """Keep the known repeated-NE sheet excluded from actual SE art.

        @param self Current test instance.
        @return None; a requested caption cannot rehabilitate wrong geometry.
        """
        row = next(row for row in self.family["selections"] if (row["state"], row["direction"]) == ("powered", "SE"))
        row["source"] = "tube-end-views-v1"
        with self.assertRaisesRegex(ValueError, "rejected tube end original"):
            ends.build(self.document)

    def test_wrong_actual_plane_at_correct_port_refuses(self):
        """Reject the NE face even when translated exactly onto the SE mouth.

        @param self Current test instance.
        @return None; independent physical plane inspection catches false labels.
        """
        candidate = ends.tube.retained_candidate("tube-end-base-v1", self.document)
        ne = ends.native_end(candidate)
        wrong = Image.new("P", ne.size, 0)
        wrong.putpalette(ne.getpalette())
        wrong.paste(ne, (0, 16))
        with self.assertRaisesRegex(ValueError, "wrong physical face plane"):
            ends.validate_cap_geometry(wrong, "SE")

    def test_oversized_stub_refuses(self):
        """Reject a long guideway masquerading as a small local pressure cap.

        @param self Current test instance.
        @return None; malformed pixels exist only in this in-memory image.
        """
        candidate = ends.tube.retained_candidate("tube-end-base-v1", self.document)
        wrong = ends.native_end(candidate)
        wrong.putpixel((10, 8), next(value for value in wrong.get_flattened_data() if value != 0))
        with self.assertRaisesRegex(ValueError, "local mouth footprint"):
            ends.validate_cap_geometry(wrong, "NE")

    def test_bad_anchor_refuses(self):
        """Reject source registration that moves the cap off its native tile.

        @param self Current test instance.
        @return None; source parts must retain anchor(-31,0).
        """
        candidate = next(row for row in self.document["candidates"] if row["key"] == "tube-end-base-v1")
        candidate["native_transform"]["anchor"] = [-32, 0]
        with self.assertRaisesRegex(ValueError, "unsupported native tube end transform"):
            ends.build(self.document)

    def test_missing_front_refuses(self):
        """Reject apparent coverage missing one exact low-facing source part.

        @param self Current test instance.
        @return None; all four mouths/states need aligned back/front coverage.
        """
        report, encoded = ends.build(self.document)
        report["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "end part coverage"):
            ends.validate(report, encoded, self.document)

    def test_false_provenance_refuses(self):
        """Reject a successful-looking atlas report with a fabricated source hash.

        @param self Current test instance.
        @return None; pixels alone do not prove retained source identity.
        """
        report, encoded = ends.build(self.document)
        report["sprites"][0]["source_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "false source provenance"):
            ends.validate(report, encoded, self.document)

    def test_painted_foreground_refuses(self):
        """Reject a painted pixel extending an otherwise correctly placed cap.

        @param self Current test instance.
        @return None; every output part must remain the exact original subset.
        """
        report, encoded = ends.build(self.document)
        with Image.open(io.BytesIO(encoded)) as atlas:
            broken = atlas.copy()
        front = next(row for row in report["sprites"] if row["part"] == "front")
        x, y = front["box"][:2]
        broken.putpixel((x, y), next(value for value in broken.get_flattened_data() if value != 0))
        with self.assertRaisesRegex(ValueError, "changes original source pixels"):
            ends.validate(report, ends.tube.indexed_png(broken), self.document)

    def test_forbidden_palette_refuses(self):
        """Reject an animated DOS index in the static source atlas.

        @param self Current test instance.
        @return None; mutate encoded proof pixels only.
        """
        report, encoded = ends.build(self.document)
        with Image.open(io.BytesIO(encoded)) as atlas:
            broken = atlas.copy()
        broken.putpixel((0, 0), min(ends.tube.base.FORBIDDEN - {0}))
        with self.assertRaisesRegex(ValueError, "forbidden native tube end palette"):
            ends.validate(report, ends.tube.indexed_png(broken), self.document)


if __name__ == "__main__":
    unittest.main()
