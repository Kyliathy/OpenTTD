"""Focused native-origin, relative-child and source-preservation checks for facility packing.

@file ground_station_offset_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCCB.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Expected parent geometry comes from the actual retained native module run,
rather than duplicating the source adapter's native anchor calculation.
"""

import copy
import unittest

from ground_pack_check_support import ROOT, ground, handoff, run_checks
from tools.ground_payload_binding import checked_reference
import pack_m1_art


class NativeFacilityOffsetChecks(unittest.TestCase):
    """Check actual native parent origins and preserve the relative roof/glass source reference."""

    @classmethod
    def setUpClass(cls):
        """Resolve immutable accepted sources and actual command-built module geometry.

        @param cls This focused facility offset suite.
        """
        cls.scope, cls.before, public = handoff.read_scope()
        cls.families = {row["asset_id"]: row for row in public["assets"]}
        cls.sources = {}
        cls.diagnostic = handoff.read_json("logs/proofs/mlm/native-art/ground/ground-consumers-v1/station-offset-diagnostic/diagnostic.json")
        execution = handoff.read_json(cls.diagnostic["execution"]["path"])
        for row in execution["observation"]["artifacts"]:
            checked_reference(row)
        metadata = next(row for row in execution["observation"]["artifacts"] if row["path"].endswith("/station-modules.json"))
        scene = handoff.read_json(metadata["path"])
        # Current actual native geometry is unchanged by source selection.
        # One ready opaque row per native graphics value supplies its real
        # parent/child origins, including absent roofs and through-stop parts.
        cls.geometry = {(row["asset_id"], row["graphics"]): row["parts"] for row in scene["frames"]
                        if row["state"] == "ready" and row["appearance"] == 0}
        for asset in ("facility.rail-station", "facility.road-terminal"):
            cls.sources[asset] = handoff.source_poses(cls.families[asset], cls.scope, cls.before, {}, {})

    def expected_geometry(self, asset, key):
        """Resolve actual native part geometry independently of the source's declared compensation.

        @param self Current focused case.
        @param asset Actual accepted rail-station or road-terminal source family.
        @param key Exact source pose identity.
        @return Actual part geometry from the retained native observer.
        """
        family = self.families[asset]
        graphics = family["directions"].index(key[2]) + (key[3] * 2 if asset == "facility.rail-station" else 0)
        return self.geometry[asset, graphics][family["parts"].index(key[4])]

    def test_all_parent_offsets_match_actual_native_geometry(self):
        """@param self Current case. Every installed parent cancels its actual native origin/contact displacement."""
        for asset, (poses, atlas) in self.sources.items():
            for key, pose in poses.items():
                # Relative glass and structural absence have distinct native
                # ownership; neither can be treated as an ordinary parent.
                if pose["structurally_absent"] or key[4] == "glass":
                    continue
                native = self.expected_geometry(asset, key)
                self.assertTrue(native["parent"])
                x, y, z = [a + b for a, b in zip(native["origin"], native["offset"])]
                expected = [-31 - 2 * (y - x), -33 - (x + y - z)]
                left, top, width, height = pose["box"]
                crop = atlas.crop((left, top, left + width, top + height)).getbbox()
                if key[4] != "roof":
                    expected = [expected[0] + crop[0], expected[1] + crop[1]]
                sprite = ground.native_pose(self.families[asset], key, poses, atlas)
                self.assertEqual(list(sprite[3:]), expected, (asset, key))

    def test_relative_glass_keeps_the_full_roof_reference(self):
        """@param self Current case. Cropped glass remains aligned to the untrimmed parent canvas in every canopy pose."""
        asset = "facility.rail-station"
        poses, atlas = self.sources[asset]
        for key, pose in poses.items():
            # Only installed glass can reference a roof; bare/building modules
            # intentionally publish absent parts without native parents.
            if key[4] != "glass" or pose["structurally_absent"]:
                continue
            roof_key = (*key[:4], "roof")
            roof = ground.native_pose(self.families[asset], roof_key, poses, atlas)
            glass = ground.native_pose(self.families[asset], key, poses, atlas)
            self.assertEqual(roof[1:3], (64, 64))
            native = self.expected_geometry(asset, roof_key)
            x, y, z = [a + b for a, b in zip(native["origin"], native["offset"])]
            self.assertEqual([2 * (y - x) + roof[3], x + y - z + roof[4]], [-31, -33])
            left, top, width, height = pose["box"]
            crop = atlas.crop((left, top, left + width, top + height)).getbbox()
            self.assertEqual(list(glass[3:]), list(crop[:2]))
            self.assertFalse(self.expected_geometry(asset, key)["parent"])

    def test_all_facility_source_indices_remain_exact(self):
        """@param self Current case. Parent/child correction changes references while preserving all authored palette indices."""
        for asset, (poses, atlas) in self.sources.items():
            for key, pose in poses.items():
                if pose["structurally_absent"]:
                    continue
                left, top, width, height = pose["box"]
                source = atlas.crop((left, top, left + width, top + height))
                crop = source.getbbox()
                pixels, packed_width, packed_height, x, y = ground.native_pose(self.families[asset], key, poses, atlas)
                expected = source if key[4] == "roof" else source.crop(crop)
                self.assertEqual(bytes(pixels), expected.tobytes(), (asset, key))
                self.assertEqual((packed_width, packed_height), expected.size)

    def test_absent_parts_do_not_gain_a_native_parent(self):
        """@param self Current case. Bare roofs/glass and through-stop third parents keep their existing empty publication slots."""
        for asset, (poses, atlas) in self.sources.items():
            for key, pose in poses.items():
                if pose["structurally_absent"]:
                    self.assertEqual(ground.native_pose(self.families[asset], key, poses, atlas), pack_m1_art.cropped_pose(atlas, pose))

    def test_unrelated_native_sources_keep_original_offsets(self):
        """@param self Current case. Representative machine/vehicle sources retain their original crop, pixels and anchors."""
        for asset in ("mine.COAL", "facility.battery-works", "vehicle.model-05"):
            poses, atlas = handoff.source_poses(self.families[asset], self.scope, self.before, {}, {})
            for key in (next(iter(poses)), next(reversed(poses))):
                self.assertEqual(ground.native_pose(self.families[asset], key, poses, atlas), pack_m1_art.cropped_pose(atlas, poses[key]))

    def test_missing_installed_native_anchor_refuses(self):
        """@param self Current case. An installed part cannot silently fall back to the common source canvas anchor."""
        asset = "facility.rail-station"
        original, atlas = self.sources[asset]
        poses = copy.deepcopy(original)
        key = ("ready", 0, "NE", 2, "front")
        del poses[key]["native_packed_anchor"]
        with self.assertRaisesRegex(ValueError, "exact signed native anchor"):
            ground.native_pose(self.families[asset], key, poses, atlas)

    def test_invalid_installed_native_anchor_refuses(self):
        """@param self Current case. Malformed or overflowing native offsets cannot authorize a new placement."""
        asset = "facility.rail-station"
        original, atlas = self.sources[asset]
        key = ("ready", 0, "NE", 2, "front")
        for anchor in ([True, 0], [0, 40000], [1], "0,0"):
            poses = copy.deepcopy(original)
            poses[key]["native_packed_anchor"] = anchor
            with self.assertRaisesRegex(ValueError, "exact signed native anchor"):
                ground.native_pose(self.families[asset], key, poses, atlas)

    def test_missing_roof_child_contract_refuses(self):
        """@param self Current case. A live roof cannot lose the child contract needed to preserve its reference canvas."""
        asset = "facility.rail-station"
        original, atlas = self.sources[asset]
        poses = copy.deepcopy(original)
        key = ("ready", 0, "NE", 2, "roof")
        del poses[(*key[:4], "glass")]
        with self.assertRaisesRegex(ValueError, "relative glass contract"):
            ground.native_pose(self.families[asset], key, poses, atlas)


if __name__ == "__main__":
    run_checks(NativeFacilityOffsetChecks)
