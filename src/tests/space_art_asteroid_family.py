"""Verify the published metallic native source matrix and exact original provenance.

@file space_art_asteroid_family.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BF
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group67
Source coverage remains distinct from packed sprites, native field selection and ordinary-game review.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest
from unittest import mock

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_asteroids as asteroids
import multilayer_sprite_contract as contract


class MetallicPublicationTests(unittest.TestCase):
    """Decode concrete native poses and retain immutable own-body prompt/input ancestry."""

    def setUp(self):
        """Read exact public source evidence without filename/recency inference."""
        self.document = art.read_record()
        self.family = self.document["families"]["asteroid.nickel"]
        self.proof = self.family["complete_source_review"]
        self.report = json.loads((art.ROOT / self.proof["path"]).read_text(encoding="utf-8"))
        self.atlas_record = next(row for row in self.report["artifacts"] if Path(row["path"]).name == "native.png")
        with Image.open(art.ROOT / self.atlas_record["path"]) as atlas:
            self.atlas = atlas.copy()

    def test_public_contract_binds_source_and_keeps_native_runtime_pending(self):
        """Publish384 actual logical poses without claiming packed or in-game delivery."""
        manifest = json.loads((art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text())
        row = next(row for row in manifest["assets"] if row["asset_id"] == "asteroid.nickel")
        self.assertEqual(art.identity(art.ROOT / self.proof["path"]), self.proof)
        self.assertEqual(row["authoring"]["atlas_manifest_sha256"], self.proof["sha256"])
        self.assertEqual(row["authoring"]["native_sha256"], self.atlas_record["sha256"])
        self.assertEqual(row["authoring"]["staged_pose_count"], 384)
        self.assertTrue(row["authoring"]["coverage_complete"])
        self.assertFalse(row["authoring"]["runtime_activated"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime"])
        self.assertIsNone(row["packed"]["offset"])
        self.assertIsNone(row["packed"]["sha256"])
        self.assertEqual(row["runtime"]["consumer"]["source"], "src/asteroid.h")
        selected = ["vehicle.model-15", "vehicle.model-19", "vehicle.model-20", "vehicle.model-18", *art.ASTEROID_ASSETS]
        self.assertEqual(contract.validate_manifest(manifest, selected), 1568)

    def test_decode_all_original_size_direction_state_frame_rectangles(self):
        """Check exact decoded pixel hashes, native anchors and full384-pose coverage."""
        self.assertEqual(self.atlas.size, (768, 3840))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.getpalette(), art.base.PALETTE_BYTES)
        self.assertFalse((set(self.atlas.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
        expected = {(size, direction, state, frame) for size in asteroids.SIZES
                    for direction in asteroids.DIRECTIONS for state in asteroids.STATES for frame in range(4)}
        seen = set()
        # Decode every concrete source rectangle; explicit saved-size/phase labels cannot substitute for real native pixels or hide duplicated coverage.
        for row in self.report["entries"]:
            key = row["size"], row["direction"], row["state"], row["frame"]
            self.assertNotIn(key, seen)
            seen.add(key)
            self.assertEqual(row["variant"], asteroids.SIZES.index(row["size"]))
            self.assertEqual(row["anchor"], [-48, -48])
            pose = self.atlas.crop(row["box"])
            self.assertEqual(pose.size, (96, 80))
            self.assertTrue(any(pose.get_flattened_data()))
            self.assertEqual(hashlib.sha256(pose.tobytes()).hexdigest(), row["pixel_sha256"])
        self.assertEqual(seen, expected)

    def test_honest_static_and_repeated_mining_aliases(self):
        """Decode static bodies and two real mining phases without fabricating spin/hardware."""
        poses = {(row["size"], row["direction"], row["state"], row["frame"]): self.atlas.crop(row["box"]).tobytes()
                 for row in self.report["entries"]}
        actual_components = 0
        # Each actual body/size has four real native outputs: original, two mining phases and worked ore. All remaining logical poses follow declared aliases.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                values = {poses[size, direction, state, frame] for state in asteroids.STATES for frame in range(4)}
                self.assertEqual(len(values), 4)
                actual_components += len(values)
                for frame in range(4):
                    self.assertEqual(poses[size, direction, "free", frame], poses[size, direction, "free", 0])
                    self.assertEqual(poses[size, direction, "attached", frame], poses[size, direction, "free", 0])
                    self.assertEqual(poses[size, direction, "depleted", frame], poses[size, direction, "depleted", 0])
                    self.assertEqual(poses[size, direction, "mining", frame], poses[size, direction, "mining", frame % 2])
        self.assertEqual(actual_components, 96)
        self.assertEqual(self.report["explicit_source_alias_count"], 384 - actual_components)

    def test_all_eight_own_originals_keep_exact_executed_prompts_and_input_roles(self):
        """Bind all selected body/effect/ore originals to actual immutable generation receipts."""
        self.assertEqual(self.report["new_original_source_count"], 8)
        self.assertEqual(self.report["registered_source_instance_count"], 0)
        # Every body part belongs to this exact physical model; no miner/probe effect or recolored unrelated asteroid fills a missing authored state.
        for snapshot in self.report["source_candidates"]:
            actual = art.source_candidate(snapshot["key"], self.document)
            self.assertEqual(actual, snapshot)
            self.assertEqual(actual["asset_id"], "asteroid.nickel")
            receipt = json.loads((art.ROOT / actual["generation_receipt"]["path"]).read_text())
            self.assertEqual(receipt["prompt"], actual["prompt"])
            self.assertEqual(receipt["sha256"], actual["sha256"])
            self.assertEqual(receipt["tool"], "built-in image_gen")
            self.assertTrue(receipt["transparent_background"])
            self.assertTrue(all(reference["role"] for reference in receipt["reference_inputs"]))

    def test_final_images_reproduce_the_reviewed_contact_set(self):
        """The final selected-source snapshot preserves every reviewed native/contact PNG byte."""
        # Source snapshot/report metadata was finalized after visual review; all16 image artifacts must remain exactly those independently reviewed pixels.
        for artifact in self.report["artifacts"]:
            self.assertEqual(art.identity(art.ROOT / artifact["path"]), artifact)
            prior = art.ROOT / "logs/proofs/mlm/native-art/space/metallic-complete-v2" / Path(artifact["path"]).name
            self.assertEqual(art.identity(prior)["sha256"], artifact["sha256"])

    def test_complete_original_layout_has_no_hidden_model_pixels_or_alpha_repairs(self):
        """Separate whole-original layout evidence from local-part extraction and source coverage."""
        audit = json.loads((art.ROOT / "logs/space-metallic-source-layout-verification.json").read_text())
        self.assertEqual(audit["source_review"], self.proof)
        self.assertEqual(len(audit["originals"]), 8)
        # Every full source view retains measured transparent clearance; neither source-part rectangles nor native masks may conceal stray model pixels or separator frames.
        for original in audit["originals"]:
            self.assertEqual(original["visible_nonseparator_pixels_outside"], 0)
            self.assertFalse(original["source_alpha_modified"])
            self.assertEqual(art.identity(art.ROOT / original["original"]["path"]), original["original"])
            for view in original["views"]:
                self.assertGreaterEqual(view["minimum_alpha16_clearance"], 16)
                self.assertEqual(view["visible_separator_pixels_inside"], 0)

    def test_small_mining_and_large_worked_originals_rebuild_exactly(self):
        """Rebuild opposite and formerly vulnerable Small poses from frozen original transforms."""
        sources = asteroids.AsteroidSources("asteroid.nickel", self.document)
        # These independent samples cross original batches, source states and size policies; a convenient large preview cannot satisfy the Small native source boundary.
        for size, direction, state, frame in (("small", "SE", "mining", 1), ("large", "NW", "depleted", 0)):
            row = next(row for row in self.report["entries"] if (row["size"], row["direction"], row["state"], row["frame"]) == (size, direction, state, frame))
            pixels, _evidence = sources.pose(direction, size, state, frame)
            self.assertEqual(pixels.tobytes(), self.atlas.crop(row["box"]).tobytes())

    def test_whole_original_audit_refuses_overlapping_view_windows(self):
        """A complete-layout audit cannot count one original rectangle as two views."""
        damaged = copy.deepcopy(self.report)
        views = damaged["source_candidates"][0]["native_transforms"]
        directions = list(views)
        views[directions[1]]["crop_box"] = views[directions[0]]["crop_box"][:]
        output = art.ROOT / "logs/proofs/mlm/native-art/space/metallic-audit-refusal-never-written.json"
        with mock.patch.object(asteroids.json, "loads", return_value=damaged):
            with self.assertRaisesRegex(ValueError, "rectangles overlap"):
                asteroids.audit_layout(art.ROOT / self.proof["path"], output)
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
