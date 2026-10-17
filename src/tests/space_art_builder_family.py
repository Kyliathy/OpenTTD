"""Verify the complete128-pose construction source handoff and its native-owner policy.

@file space_art_builder_family.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BD
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63C.3
Actual retained indexed pixels govern this source-only handoff; packing/runtime remain separate tasks.
"""
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_family_art as family

ASSET = "vehicle.model-20"


class BuilderFamilySourceTests(unittest.TestCase):
    """Require real source coverage, honest static aliases and independent material/work/service ancestry."""

    def setUp(self):
        """Load the exact selected source publication and detached definitions for refusal checks."""
        self.document = art.read_record()
        self.definition = self.document["families"][ASSET]
        self.proof = self.definition["complete_source_review"]
        self.report = json.loads((art.ROOT / self.proof["path"]).read_text())
        self.entry = self.report["atlases"][0]

    def test_public_contract_binds_the_actual_atlas_without_claiming_runtime(self):
        """The public source descriptor binds128 real native poses while packed offsets and runtime stay pending."""
        self.assertEqual(art.identity(art.ROOT / self.proof["path"]), self.proof)
        self.assertEqual(art.identity(art.ROOT / self.entry["path"])["sha256"], self.entry["sha256"])
        manifest = json.loads((art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text())
        row = next(r for r in manifest["assets"] if r["asset_id"] == ASSET)
        self.assertEqual(row["states"], family.BUILDER_STATES)
        self.assertEqual(row["directions"], family.DIRECTIONS)
        self.assertEqual(row["authoring"]["atlas_manifest_sha256"], self.proof["sha256"])
        self.assertEqual(row["authoring"]["native_sha256"], self.entry["sha256"])
        self.assertEqual(row["authoring"]["staged_pose_count"], 128)
        self.assertTrue(row["authoring"]["coverage_complete"])
        self.assertFalse(row["authoring"]["runtime_activated"])
        self.assertIsNone(row["packed"]["sha256"])
        self.assertIsNone(row["packed"]["offset"])

    def test_all_exported_pixels_have_correct_static_and_animated_coverage(self):
        """Decode every logical box, checking actual material distinctions and only the declared real animations."""
        atlas = Image.open(art.ROOT / self.entry["path"])
        self.assertEqual(atlas.size, (512, 768))
        self.assertEqual(atlas.mode, "P")
        self.assertEqual(atlas.getpalette(), art.base.PALETTE_BYTES)
        self.assertFalse((set(atlas.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
        poses, aliases = {}, 0
        # Decode each actual public box independently, so duplicate entries or omitted headings cannot be concealed by producer counters.
        for row in self.entry["sprites"]:
            key = row["state"], row["frame"], row["direction"]
            self.assertNotIn(key, poses)
            x, y, width, height = row["box"]
            self.assertEqual((width, height), (64, 48))
            self.assertEqual(row["anchor"], [-32, -32])
            pose = atlas.crop((x, y, x + width, y + height))
            self.assertTrue(any(pose.get_flattened_data()))
            poses[key] = pose.tobytes()
            aliases += row["explicit_static_alias"]
        self.assertEqual(len(poses), 128)
        self.assertEqual(aliases, 32)
        self.assertEqual(self.report["actual_component_pose_count"], 80)
        # Loads remain honest static type cues, while the real arm/service/gas phases must differ. Return deliberately shares the same actual navigation effect as approach.
        for direction in family.DIRECTIONS:
            for state in family.BUILDER_STATES:
                self.assertEqual(poses[state, 0, direction] != poses[state, 1, direction], state in ("approach", "constructing", "return", "service"))
            self.assertNotEqual(poses["steel", 0, direction], poses["goods", 0, direction])
            self.assertEqual(poses["approach", 0, direction], poses["return", 0, direction])
            self.assertEqual(poses["approach", 1, direction], poses["return", 1, direction])

    def test_actual_original_ancestry_and_reused_gas_are_distinct(self):
        """Sixteen own originals and four real gas instances retain exact original receipt/input identities."""
        self.assertEqual(self.report["new_original_source_count"], 16)
        self.assertEqual(self.report["registered_source_instance_count"], 4)
        self.assertEqual(len(self.report["source_candidates"]), 20)
        self.assertEqual(len(self.report["shared_original_sources"]), 4)
        # Every actual source and every reused instance resolves against its immutable ancestry; no reused gas is counted as new carrier hardware.
        for candidate in self.report["source_candidates"]:
            actual = art.source_candidate(candidate["key"], self.document)
            self.assertEqual(actual["sha256"], candidate["sha256"])
            if candidate.get("instance_of") is not None:
                self.assertEqual(candidate["component_kind"], "rear_plume")
                for direction in candidate["native_transforms"]:
                    family.parts.registered_component_source(candidate["key"], direction, self.document)

    def test_actual_opposite_work_pose_rebuilds_identically(self):
        """Rebuild a genuine opposite-view fastening pose and compare actual indexed publication bytes."""
        pose, _ = family.state_pose("constructing", "NW", 1, self.document, ASSET)
        row = next(r for r in self.entry["sprites"] if (r["state"], r["direction"], r["frame"]) == ("constructing", "NW", 1))
        x, y, width, height = row["box"]
        atlas = Image.open(art.ROOT / self.entry["path"])
        self.assertEqual(pose.tobytes(), atlas.crop((x, y, x + width, y + height)).tobytes())

    def test_missing_cargo_approval_refuses(self):
        """An existing material PNG alone cannot supply an unapproved cargo-volume registration."""
        del self.definition["approved_load_registration"]["goods"]["W"]
        with self.assertRaisesRegex(ValueError, "approval"):
            family.state_pose("goods", "W", 0, self.document, ASSET)

    def test_missing_work_approval_refuses(self):
        """A complete atlas cannot hide a missing actual fastening-phase approval."""
        del self.definition["approved_builder_registration"]["1"]["NW"]
        with self.assertRaisesRegex(ValueError, "approval"):
            family.state_pose("constructing", "NW", 1, self.document, ASSET)

    def test_static_load_cannot_become_fabricated_animation(self):
        """A later metadata edit cannot claim a second actual cargo pose from the one authored still."""
        self.definition["state_policy"]["goods"]["animated"] = True
        with self.assertRaisesRegex(ValueError, "policy"):
            family.state_pose("goods", "N", 0, self.document, ASSET)

    def test_foreign_work_state_and_unauthored_frame_refuse(self):
        """The carrier cannot inherit mining/scanning behavior or invent a third physical frame."""
        # Each foreign native meaning is refused independently; source coverage never authorizes role, cargo or work mutations.
        for state in ("mining", "scanning", "full"):
            with self.assertRaisesRegex(ValueError, "public"):
                family.state_pose(state, "N", 0, self.document, ASSET)
        with self.assertRaisesRegex(ValueError, "public"):
            family.state_pose("constructing", "N", 2, self.document, ASSET)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
