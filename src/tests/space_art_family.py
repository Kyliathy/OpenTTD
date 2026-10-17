"""Check the published small-miner matrix and truthful original/alias coverage.

@file space_art_family.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63
Checks consume actual retained indexed atlas pixels; detached malformed inputs cannot mutate original art or native gameplay.
"""
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_family_art as family


class FamilySourceTests(unittest.TestCase):
    """Require complete public coverage, real animation and exact original source ancestry."""

    def setUp(self):
        """Load independent source definitions and the explicitly selected complete proof."""
        self.document = art.read_record()
        self.definition = self.document["families"][family.parts.ASSET]
        self.proof = self.definition["complete_source_review"]
        self.report = json.loads((art.ROOT / self.proof["path"]).read_text())
        self.entry = self.report["atlases"][0]

    def test_publication_matches_actual_complete_atlas(self):
        """Public source acceptance binds actual atlas bytes without claiming packed/runtime delivery."""
        self.assertEqual(art.identity(art.ROOT / self.proof["path"]), self.proof)
        self.assertEqual(art.identity(art.ROOT / self.entry["path"])["sha256"], self.entry["sha256"])
        manifest = json.loads((art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text())
        row = next(r for r in manifest["assets"] if r["asset_id"] == family.parts.ASSET)
        self.assertEqual(row["directions"], family.DIRECTIONS)
        self.assertEqual(row["states"], family.STATES)
        self.assertEqual(row["authoring"]["atlas_manifest_sha256"], self.proof["sha256"])
        self.assertEqual(row["authoring"]["native_sha256"], self.entry["sha256"])
        self.assertEqual(row["authoring"]["staged_pose_count"], 128)
        self.assertEqual(row["company_colors"], "none")
        self.assertIsNone(row["packed"]["sha256"])
        self.assertFalse(row["authoring"]["runtime_activated"])
        self.assertTrue(row["authoring"]["coverage_complete"])

    def test_native_pixels_cover_every_public_pose_and_truthful_alias(self):
        """Decode all128 boxes independently: each heading exists and only declared static poses repeat."""
        atlas = Image.open(art.ROOT / self.entry["path"])
        self.assertEqual(atlas.mode, "P")
        self.assertEqual(atlas.size, (512, 768))
        self.assertEqual(atlas.getpalette(), art.base.PALETTE_BYTES)
        self.assertFalse((set(atlas.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
        poses, aliases = {}, 0
        # Actual indexed crop bytes, rather than reported counts, prove complete and nonoverlapping logical coverage with the original anchor.
        for row in self.entry["sprites"]:
            key = row["state"], row["frame"], row["direction"]
            self.assertNotIn(key, poses)
            x, y, width, height = row["box"]
            self.assertEqual((width, height), (64, 48))
            self.assertEqual(row["anchor"], [-32, -32])
            image = atlas.crop((x, y, x + width, y + height))
            self.assertTrue(any(image.get_flattened_data()))
            poses[key] = image.tobytes()
            aliases += row["explicit_static_alias"]
        self.assertEqual(len(poses), 128)
        self.assertEqual(aliases, 32)
        # Animation must change real native pixels in every view; free/attached/full/blocked explicitly preserve one static physical pose.
        for state in family.STATES:
            for direction in family.DIRECTIONS:
                self.assertEqual(poses[state, 0, direction] != poses[state, 1, direction], state in ("rendezvous", "mining", "return", "service"))
                self.assertEqual(poses["return", 0, direction], poses["rendezvous", 0, direction])
                self.assertEqual(poses["attached", 0, direction], poses["mining", 0, direction])
        self.assertEqual(self.report["actual_component_pose_count"], 72)

    def test_all_retained_originals_have_exact_actual_receipts(self):
        """All18 selected originals and every actual input hash remain reproducible; excluded alpha failures never supply source pixels."""
        self.assertEqual(len(self.report["source_candidates"]), 18)
        excluded = {row["key"] for row in self.document["excluded_sources"]}
        # Direct source and prompt/input identities prevent a plausible later replacement from entering this dated accepted source boundary.
        for candidate in self.report["source_candidates"]:
            self.assertNotIn(candidate["key"], excluded)
            self.assertEqual(art.source_candidate(candidate["key"], self.document)["sha256"], candidate["sha256"])

    def test_one_exported_mechanical_pose_rebuilds_exactly(self):
        """Rebuild an actual phase from originals and compare it with the published indexed crop."""
        pose, _ = family.state_pose("mining", "NW", 1, self.document)
        row = next(r for r in self.entry["sprites"] if (r["state"], r["direction"], r["frame"]) == ("mining", "NW", 1))
        x, y, width, height = row["box"]
        atlas = Image.open(art.ROOT / self.entry["path"])
        self.assertEqual(pose.tobytes(), atlas.crop((x, y, x + width, y + height)).tobytes())

    def test_changed_static_animation_claim_refuses(self):
        """A still source cannot silently become an animated claim after approval."""
        self.definition["state_policy"]["free"]["animated"] = True
        with self.assertRaisesRegex(ValueError, "policy"):
            family.state_pose("free", "N", 0, self.document)

    def test_missing_actual_phase_approval_refuses(self):
        """A complete export cannot borrow an unapproved component just because a raw bitmap exists."""
        del self.definition["approved_travel_registration"]["1"]["N"]
        with self.assertRaisesRegex(ValueError, "approval"):
            family.state_pose("return", "N", 1, self.document)

    def test_missing_public_state_refuses(self):
        """Removing a required state cannot reduce the matrix to make incomplete source appear accepted."""
        self.definition["states"].remove("blocked")
        with self.assertRaisesRegex(ValueError, "public"):
            family.state_pose("free", "N", 0, self.document)

    def test_unauthored_public_frame_refuses(self):
        """A third logical frame cannot invent a missing original animation phase."""
        with self.assertRaisesRegex(ValueError, "public"):
            family.state_pose("mining", "N", 2, self.document)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
