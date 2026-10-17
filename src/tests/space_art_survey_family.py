"""Verify the published survey matrix without inheriting mining/cargo state or claiming reused gas as new artwork.

@file space_art_survey_family.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BC
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63B.3
Actual retained indexed pixels and immutable source ancestry govern source acceptance; native packing and gameplay review remain separate gates.
"""
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_family_art as family

ASSET = "vehicle.model-19"


class SurveyFamilySourceTests(unittest.TestCase):
    """Require complete original scanner/service coverage and explicit shared-effect provenance."""

    def setUp(self):
        """Load independent family metadata and its exact selected source proof."""
        self.document = art.read_record()
        self.definition = self.document["families"][ASSET]
        self.proof = self.definition["complete_source_review"]
        self.report = json.loads((art.ROOT / self.proof["path"]).read_text())
        self.entry = self.report["atlases"][0]

    def test_public_source_descriptor_matches_actual_reviewed_atlas(self):
        """The public cargo-less survey descriptor binds real96-pose source bytes while packing/runtime remain pending."""
        self.assertEqual(art.identity(art.ROOT / self.proof["path"]), self.proof)
        self.assertEqual(art.identity(art.ROOT / self.entry["path"])["sha256"], self.entry["sha256"])
        manifest = json.loads((art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text())
        row = next(r for r in manifest["assets"] if r["asset_id"] == ASSET)
        self.assertEqual(row["states"], family.SURVEY_STATES)
        self.assertEqual(row["directions"], family.DIRECTIONS)
        self.assertEqual(row["authoring"]["atlas_manifest_sha256"], self.proof["sha256"])
        self.assertEqual(row["authoring"]["native_sha256"], self.entry["sha256"])
        self.assertEqual(row["authoring"]["staged_pose_count"], 96)
        self.assertTrue(row["authoring"]["coverage_complete"])
        self.assertFalse(row["authoring"]["runtime_activated"])
        self.assertIsNone(row["packed"]["sha256"])
        self.assertEqual(row["company_colors"], "none")

    def test_decode_all_actual_poses_and_explicit_static_aliases(self):
        """Decode every public box: only free/blocked are static, and return shares the real approach pulses."""
        atlas = Image.open(art.ROOT / self.entry["path"])
        self.assertEqual(atlas.size, (512, 576))
        self.assertEqual(atlas.mode, "P")
        self.assertEqual(atlas.getpalette(), art.base.PALETTE_BYTES)
        self.assertFalse((set(atlas.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
        poses, aliases = {}, 0
        # Crop actual indexed pixels independently of producer counts, refusing duplicate logical entries or a source matrix that silently omits a required physical heading.
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
        self.assertEqual(len(poses), 96)
        self.assertEqual(aliases, 16)
        # Both real work states and both travelling states must change actual native pixels; identical stills cannot supply their authored animation.
        for state in family.SURVEY_STATES:
            for direction in family.DIRECTIONS:
                self.assertEqual(poses[state, 0, direction] != poses[state, 1, direction], state in ("approach", "scanning", "return", "service"))
                self.assertEqual(poses["approach", 0, direction], poses["return", 0, direction])
                self.assertEqual(poses["approach", 1, direction], poses["return", 1, direction])
        self.assertEqual(self.report["actual_component_pose_count"], 64)

    def test_shared_gas_is_honestly_registered_original_art(self):
        """Twelve actual survey originals and four gas instances retain their real receipts and approved donor geometry."""
        self.assertEqual(self.report["new_original_source_count"], 12)
        self.assertEqual(self.report["registered_source_instance_count"], 4)
        self.assertEqual(len(self.report["source_candidates"]), 16)
        self.assertEqual(len(self.report["shared_original_sources"]), 4)
        # Resolve every real original/input hash and each actual instance heading. Reused gas is not new generated spacecraft hardware or a fabricated directional view.
        for candidate in self.report["source_candidates"]:
            actual = art.source_candidate(candidate["key"], self.document)
            self.assertEqual(actual["sha256"], candidate["sha256"])
            if candidate.get("instance_of") is not None:
                for direction in candidate["native_transforms"]:
                    family.parts.registered_component_source(candidate["key"], direction, self.document)

    def test_one_exported_scanner_pose_rebuilds_from_actual_original(self):
        """Rebuild the actual opposite-view sweep and compare its indexed pixels with the published atlas."""
        pose, _ = family.state_pose("scanning", "SW", 1, self.document, ASSET)
        row = next(r for r in self.entry["sprites"] if (r["state"], r["direction"], r["frame"]) == ("scanning", "SW", 1))
        x, y, width, height = row["box"]
        atlas = Image.open(art.ROOT / self.entry["path"])
        self.assertEqual(pose.tobytes(), atlas.crop((x, y, x + width, y + height)).tobytes())

    def test_source_policy_cannot_claim_scan_for_idle_still(self):
        """A later label change cannot present free art as real authored scan animation."""
        self.definition["state_policy"]["free"]["animated"] = True
        with self.assertRaisesRegex(ValueError, "policy"):
            family.state_pose("free", "N", 0, self.document, ASSET)

    def test_missing_actual_shared_phase_approval_refuses(self):
        """A complete matrix cannot borrow an unapproved effect solely because its original PNG exists."""
        del self.definition["approved_travel_registration"]["1"]["SW"]
        with self.assertRaisesRegex(ValueError, "approval"):
            family.state_pose("return", "SW", 1, self.document, ASSET)

    def test_survey_cannot_inherit_a_mining_or_full_hold_state(self):
        """Cargo-less scanning gains neither mining graphics authority nor a fictitious full-hold status."""
        # Explicitly check both foreign native meanings instead of accepting any state available in the older mining profile.
        for state in ("mining", "full"):
            with self.assertRaisesRegex(ValueError, "public"):
                family.state_pose(state, "N", 0, self.document, ASSET)

    def test_unauthored_public_frame_refuses(self):
        """A third logical frame cannot invent an absent original dish or service phase."""
        with self.assertRaisesRegex(ValueError, "public"):
            family.state_pose("scanning", "N", 2, self.document, ASSET)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
