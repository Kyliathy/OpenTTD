"""Focused native interchange reuse and ownership boundary checks.

@file tube_interchange_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed records remain in memory; native cargo/track owners and originals are untouched.
"""

import copy
import json
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_tube_interchanges as interchange


class TubeInterchangeSourceChecks(unittest.TestCase):
    """Verify compatible native source reuse without inventing transfer machinery or inventory."""

    @classmethod
    def setUpClass(cls):
        """Build one unchanged baseline for independent refusal checks.

        @param cls Current class retaining source metadata and original atlas bytes.
        @return None; source originals/public contracts remain unchanged.
        """
        cls.baseline, cls.encoded = interchange.build()

    def setUp(self):
        """Isolate provenance for each malformed ownership or source handoff.

        @param self Current test instance.
        @return None; mutations cannot escape this case.
        """
        self.document = json.loads(interchange.tube.RECORD.read_text(encoding="utf-8"))
        self.family = next(row for row in self.document["families"] if row["asset_id"] == interchange.ASSET)
        self.report = copy.deepcopy(self.baseline)

    def test_complete_exact_original_reuse(self):
        """Verify complete native poses and exact existing platform pixels.

        @param self Current test instance.
        @return None; all source states/phases/parts and proof regenerate.
        """
        parent, pixels = interchange.terminals.build(self.document)
        self.assertEqual(self.encoded, pixels)
        self.assertEqual(self.report["sprites"], parent["sprites"])
        self.assertEqual(self.report["pose_count"], 256)
        interchange.validate(self.report, self.encoded, self.document)
        interchange.publish(check=True)

    def test_fabricated_geometry_and_ownership_refuse(self):
        """Reject a fixed campus, shared track, cross-layer identity or private stock.

        @param self Current test instance.
        @return None; changing one claimed owner cannot inherit original source acceptance.
        """
        changes = {"footprint": [3, 2], "tile_variants": 6,
                   "station_identity": "same_xy_across_layers", "physical_geometry": "mixed_tube_rail_track",
                   "inventory_owner": "private_transfer_buffer", "partner_families": ["facility.tube-service"]}
        # Each forbidden change tests a distinct native authority boundary;
        # later malformed cases must start from the unchanged valid contract.
        for key, value in changes.items():
            with self.subTest(field=key):
                document = copy.deepcopy(self.document)
                next(row for row in document["families"] if row["asset_id"] == interchange.ASSET)[key] = value
                with self.assertRaisesRegex(ValueError, "geometry/ownership"):
                    interchange.build(document)

    def test_incompatible_original_family_refuses(self):
        """Reject depot source relabelled as a through loading platform.

        @param self Current test instance.
        @return None; matching filenames do not supply real platform geometry.
        """
        self.family["reuse_family"] = "facility.tube-service"
        with self.assertRaisesRegex(ValueError, "geometry/ownership"):
            interchange.build(self.document)

    def test_missing_real_front_refuses(self):
        """Reject a missing actual Y canopy front part.

        @param self Current test instance.
        @return None; reuse cannot skip the canonical platform coverage check.
        """
        self.report["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "coverage"):
            interchange.validate(self.report, self.encoded, self.document)

    def test_changed_original_hash_refuses(self):
        """Reject stale original provenance through the alias as through its parent.

        @param self Current test instance.
        @return None; no original image is modified.
        """
        source = self.report["sprites"][0]["source"]
        next(row for row in self.document["candidates"] if row["path"] == source)["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "hash|identity|changed"):
            interchange.build(self.document)

    def test_false_transfer_runtime_claim_refuses(self):
        """Keep source acceptance separate from actual loading/transfer operation.

        @param self Current test instance.
        @return None; complete original pixels cannot activate unverified simulation.
        """
        self.report["runtime_activated"] = True
        with self.assertRaisesRegex(ValueError, "source/runtime"):
            interchange.validate(self.report, self.encoded, self.document)

    def test_false_alias_identity_refuses(self):
        """Reject a source report that silently removes the canonical station owner.

        @param self Current test instance.
        @return None; source ancestry includes geometry and ownership, not just pixels.
        """
        self.report["source_reuse"]["inventory_owner"] = "private_transfer_buffer"
        with self.assertRaisesRegex(ValueError, "source/runtime"):
            interchange.validate(self.report, self.encoded, self.document)


if __name__ == "__main__":
    unittest.main()
