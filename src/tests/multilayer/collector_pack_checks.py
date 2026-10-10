"""Protect shipped native ranges and the collector's verified single-atlas handoff.

@file collector_pack_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group79C
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
These focused refusals do not claim actual GRF loading or game rendering.
"""

import copy
import json
from pathlib import Path
import sys
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_m1_art as art


class CollectorPackingChecks(unittest.TestCase):
    """Exercise compatibility and producer refusals against actual source contracts."""

    @classmethod
    def setUpClass(cls):
        """Load the selected contract and actual completed collector source report.

        @param cls Test class retaining immutable inputs for independent corruptions.
        @return None; fail immediately if the real source handoff is absent.
        """
        cls.public = json.loads(art.PUBLIC.read_text(encoding="utf-8"))
        cls.scope = json.loads(art.SCOPE.read_text(encoding="utf-8"))
        cls.collector = next(row for row in cls.public["assets"] if row["asset_id"] == "facility.vent-collector")
        cls.report_path = cls.collector["authoring"]["atlas_manifest"]
        cls.report = json.loads(art.base.repository_path(cls.report_path).read_text(encoding="utf-8"))

    def test_public_reordering_preserves_shipped_enum_order_and_ranges(self):
        """Keep old enum identities/offsets when new art occurs earlier in the public list.

        @return None; require the frozen shipped sequence and collector append.
        """
        public = copy.deepcopy(self.public)
        public["assets"].reverse()
        rows, bound = art.selected_ranges(public, self.scope)
        expected = self.scope["preserved_ranges"]
        self.assertEqual([{key: row[key] for key in ("asset_id", "native_offset", "sprite_count")} for row in rows[:-1]], expected)
        self.assertEqual((rows[-1]["asset_id"], rows[-1]["native_offset"], rows[-1]["sprite_count"], bound),
                         ("facility.vent-collector", 2436, 1152, 3588))

    def test_removing_a_shipped_selection_refuses(self):
        """Prevent a selection change from reclaiming a previously shipped family's slots.

        @return None; require a preserved-family refusal.
        """
        scope = copy.deepcopy(self.scope)
        scope["asset_ids"].remove("vehicle.model-00")
        with self.assertRaisesRegex(ValueError, "missing/duplicate preserved"):
            art.selected_ranges(self.public, scope)

    def test_changed_shipped_extent_refuses(self):
        """Refuse enlarging an existing source matrix into the next family's IDs.

        @return None; require a published-range refusal before source conversion.
        """
        public = copy.deepcopy(self.public)
        family = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-00")
        family["frames"] += 1
        with self.assertRaisesRegex(ValueError, "published native range changed"):
            art.selected_ranges(public, self.scope)

    def test_changed_shipped_offset_refuses(self):
        """Prevent stale public metadata from relocating a valid old source family.

        @return None; require a published-range refusal.
        """
        public = copy.deepcopy(self.public)
        family = next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-00")
        family["packed"]["native_offset"] += 1
        with self.assertRaisesRegex(ValueError, "published native range changed"):
            art.selected_ranges(public, self.scope)

    def test_overlapping_published_ranges_refuse(self):
        """Reject overlap even when a corrupted frozen/public pair agrees internally.

        @return None; require independently occupied native slots to remain exclusive.
        """
        public, scope = copy.deepcopy(self.public), copy.deepcopy(self.scope)
        family = next(row for row in public["assets"] if row["asset_id"] == "terrain.hardness-edges")
        family["packed"]["native_offset"] = 2404
        frozen = next(row for row in scope["preserved_ranges"] if row["asset_id"] == family["asset_id"])
        frozen["native_offset"] = 2404
        with self.assertRaisesRegex(ValueError, "overlapping selected native range"):
            art.selected_ranges(public, scope)

    def test_reserved_core_slots_refuse(self):
        """Keep reserved offsets unavailable even for a seemingly new core family.

        @return None; require the planned-core boundary to reject the collector.
        """
        public = copy.deepcopy(self.public)
        family = next(row for row in public["assets"] if row["asset_id"] == self.collector["asset_id"])
        family["packed"] = {"namespace": "native_map_layer_extension", "offset": 66}
        with self.assertRaisesRegex(ValueError, "invalid selected core range"):
            art.selected_ranges(public, self.scope)

    def test_unselected_native_binding_refuses(self):
        """Do not recycle a native owner that was omitted from the selection contract.

        @return None; require explicit inclusion of every already bound family.
        """
        public = copy.deepcopy(self.public)
        family = next(row for row in public["assets"] if row["asset_id"] not in self.scope["asset_ids"])
        family["packed"].update(route=self.scope["native_route"], native_offset=3588, sprite_count=1)
        with self.assertRaisesRegex(ValueError, "published native family omitted"):
            art.selected_ranges(public, self.scope)

    def test_repeated_collector_binding_cannot_move(self):
        """A second conversion must keep the first published collector append intact.

        @return None; refuse a later metadata edit that would shift its native IDs.
        """
        public = copy.deepcopy(self.public)
        family = next(row for row in public["assets"] if row["asset_id"] == self.collector["asset_id"])
        family["packed"].update(route=self.scope["native_route"], native_offset=2437, sprite_count=1152)
        with self.assertRaisesRegex(ValueError, "selected native binding changed"):
            art.selected_ranges(public, self.scope)

    def test_native_count_limit_refuses(self):
        """Stop a new family before its range exceeds Action5's count encoding.

        @return None; require the explicit native block bound to reject overflow.
        """
        public = copy.deepcopy(self.public)
        family = next(row for row in public["assets"] if row["asset_id"] == self.collector["asset_id"])
        family["packed"] = {"namespace": "MLVI", "offset": None, "sha256": None}
        family["frames"] = 1000
        with self.assertRaisesRegex(ValueError, "exceeds Action 5 count range"):
            art.selected_ranges(public, self.scope)

    def test_verified_single_atlas_has_exact_collector_coverage(self):
        """Consume all real collector cells/states/views/phases and aligned native parts.

        @return None; require every one of the 1,152 declared source poses.
        """
        poses, image = art.source_poses(self.collector, {self.report_path: self.report}, {})
        self.assertEqual(len(poses), 1152)
        self.assertEqual(set(poses), set(art.required_keys(self.collector)))
        self.assertEqual(image.size, tuple(self.report["size"]))

    def test_unverified_single_atlas_refuses(self):
        """Never interpret an arbitrary report as a verified ground producer handoff.

        @return None; reject incomplete, wrong-owner and unknown-schema reports.
        """
        # Each field binds a different producer obligation. A valid atlas hash
        # cannot replace explicit completion, owner identity or schema agreement.
        for field, value in (("source_coverage_complete", False), ("asset_id", "facility.vent-powerhouse"), ("schema_version", 2)):
            with self.subTest(field=field):
                document = copy.deepcopy(self.report)
                document[field] = value
                with self.assertRaisesRegex(ValueError, "invalid verified single-atlas"):
                    art.source_poses(self.collector, {self.report_path: document}, {})

    def test_single_atlas_missing_physical_part_refuses(self):
        """A complete flag cannot conceal a missing collector tile/front/state pose.

        @return None; require exact Cartesian coverage against the genuine atlas.
        """
        document = copy.deepcopy(self.report)
        document["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "incomplete source coverage"):
            art.source_poses(self.collector, {self.report_path: document}, {})

    def test_single_atlas_stale_hash_refuses(self):
        """Keep selected source bytes bound to their producer and public identities.

        @return None; reject report metadata that fingerprints another atlas.
        """
        document = copy.deepcopy(self.report)
        document["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "stale source atlas"):
            art.source_poses(self.collector, {self.report_path: document}, {})

    def test_single_atlas_changed_dimensions_refuse(self):
        """Do not accept a source mapping that describes a different whole atlas.

        @return None; reject dimensions before inspecting per-pose source boxes.
        """
        document = copy.deepcopy(self.report)
        document["size"][0] += 1
        with self.assertRaisesRegex(ValueError, "source atlas dimensions changed"):
            art.source_poses(self.collector, {self.report_path: document}, {})


if __name__ == "__main__":
    unittest.main()
