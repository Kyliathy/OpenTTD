"""Verify the published passive-pod source matrix and its truthful mechanical/static aliases.

@file space_art_pod_family.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BE
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group64.3
Legacy recovered source means recovery-ready live-job art; native successful expenditure still deletes that shell. Packing/runtime stay separate.
"""
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_family_art as family

ASSET = "vehicle.model-18"


class PodFamilySourceTests(unittest.TestCase):
    """Require all original shell views and real tab phases without fabricating additional physical components."""

    def setUp(self):
        """Read the selected actual publication and independent definitions for each refusal check."""
        self.document = art.read_record()
        self.definition = self.document["families"][ASSET]
        self.proof = self.definition["complete_source_review"]
        self.report = json.loads((art.ROOT / self.proof["path"]).read_text())
        self.entry = self.report["atlases"][0]

    def test_public_source_identity_is_complete_while_passive_rendering_is_pending(self):
        """The public contract binds64 actual source poses and retains its separate passive-pool runtime gate."""
        self.assertEqual(art.identity(art.ROOT / self.proof["path"]), self.proof)
        self.assertEqual(art.identity(art.ROOT / self.entry["path"])["sha256"], self.entry["sha256"])
        manifest = json.loads((art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text())
        row = next(r for r in manifest["assets"] if r["asset_id"] == ASSET)
        self.assertEqual(row["states"], family.POD_STATES)
        self.assertEqual(row["authoring"]["atlas_manifest_sha256"], self.proof["sha256"])
        self.assertEqual(row["authoring"]["native_sha256"], self.entry["sha256"])
        self.assertEqual(row["authoring"]["staged_pose_count"], 64)
        self.assertTrue(row["authoring"]["coverage_complete"])
        self.assertFalse(row["authoring"]["runtime_activated"])
        self.assertIsNone(row["packed"]["sha256"])
        self.assertIsNone(row["packed"]["offset"])
        self.assertEqual(row["runtime"]["consumer"]["source"], "src/space_pod.h")

    def test_decode_actual_poses_and_honest_recovery_blocked_aliases(self):
        """Only descent animates; recovery-ready holds actual phase1 and blocked explicitly reuses the folded shell."""
        atlas = Image.open(art.ROOT / self.entry["path"])
        self.assertEqual(atlas.size, (512, 384))
        self.assertEqual(atlas.mode, "P")
        self.assertEqual(atlas.getpalette(), art.base.PALETTE_BYTES)
        self.assertFalse((set(atlas.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
        poses, aliases = {}, 0
        # Decode actual indexed boxes rather than trusting producer counters; source duplication may be accepted only by the explicit static-alias contract.
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
        self.assertEqual(len(poses), 64)
        self.assertEqual(aliases, 24)
        self.assertEqual(self.report["actual_component_pose_count"], 24)
        # Static recovery-ready is the same real open-tab source, not a newly created recovered object or a new animation/work cycle. Blocking keeps the same physical shell.
        for direction in family.DIRECTIONS:
            for state in family.POD_STATES:
                self.assertEqual(poses[state, 0, direction] != poses[state, 1, direction], state == "descending")
            self.assertEqual(poses["recovered", 0, direction], poses["descending", 1, direction])
            self.assertEqual(poses["blocked", 0, direction], poses["packed", 0, direction])

    def test_actual_originals_retain_direct_prompt_input_ancestry(self):
        """Eight own accepted originals retain real source/receipt identities; rejected haze is never counted as coverage."""
        self.assertEqual(self.report["new_original_source_count"], 8)
        self.assertEqual(self.report["registered_source_instance_count"], 0)
        self.assertEqual(len(self.report["source_candidates"]), 8)
        self.assertEqual(self.report["shared_original_sources"], [])
        # Resolve exact original files/receipts/references for every selected body/tab source. No historical cache or borrowed role artwork can fill a missing pod component.
        for candidate in self.report["source_candidates"]:
            actual = art.source_candidate(candidate["key"], self.document)
            self.assertEqual(actual["sha256"], candidate["sha256"])
            self.assertNotEqual(candidate["key"], "return-pod-tabs-b-ne-se-sw-nw-v2")

    def test_opposite_actual_descent_pose_rebuilds_identically(self):
        """An original W phase rebuild reproduces the published native bytes at the fixed world anchor."""
        pose, _ = family.state_pose("descending", "W", 1, self.document, ASSET)
        row = next(r for r in self.entry["sprites"] if (r["state"], r["frame"], r["direction"]) == ("descending", 1, "W"))
        x, y, width, height = row["box"]
        atlas = Image.open(art.ROOT / self.entry["path"])
        self.assertEqual(pose.tobytes(), atlas.crop((x, y, x + width, y + height)).tobytes())

    def test_missing_tab_approval_refuses_recovery_ready_alias(self):
        """A static final pose still requires its actual live-job phase source, beyond a convenient alias label."""
        del self.definition["approved_descent_registration"]["1"]["S"]
        with self.assertRaisesRegex(ValueError, "approval"):
            family.state_pose("recovered", "S", 0, self.document, ASSET)

    def test_recovery_ready_cannot_become_a_new_fabricated_animation(self):
        """A later metadata change cannot claim additional original frames or an art-owned completion cycle."""
        self.definition["state_policy"]["recovered"]["animated"] = True
        with self.assertRaisesRegex(ValueError, "policy"):
            family.state_pose("recovered", "S", 0, self.document, ASSET)

    def test_probe_work_state_cannot_enter_passive_pod(self):
        """A passive payload gains no mining, scanner or manipulator source behavior."""
        # Each incompatible native role is checked independently; the shared canvas conveys no work/cargo authority.
        for state in ("mining", "scanning", "constructing", "steel", "service"):
            with self.assertRaisesRegex(ValueError, "public"):
                family.state_pose(state, "N", 0, self.document, ASSET)

    def test_unauthored_third_frame_refuses(self):
        """A third logical frame cannot invent another mechanical tab phase."""
        with self.assertRaisesRegex(ValueError, "public"):
            family.state_pose("descending", "N", 2, self.document, ASSET)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
