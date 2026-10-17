"""Verify published rich Rare-Earth source coverage, actual native pixels and original ancestry.

@file space_art_rich_family.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BFC
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group69
Publication certifies source coverage only; native packing, field selection and ordinary-game review stay separate.
"""
import hashlib
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_asteroids as asteroids
import multilayer_sprite_contract as contract


class RichPublicationTests(unittest.TestCase):
    """Require the exact three-size rich matrix and its own unaltered source receipts.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md Group69.3
    """

    def setUp(self):
        """Read the explicit original source record and its exact published evidence.

        @param self Test instance checking this exact published rich-source boundary.
        """
        self.document = art.read_record()
        self.family = self.document["families"]["asteroid.rare-earth"]
        self.proof = self.family["complete_source_review"]
        self.report = json.loads((art.ROOT / self.proof["path"]).read_text(encoding="utf-8"))
        self.atlas_record = next(row for row in self.report["artifacts"] if Path(row["path"]).name == "native.png")
        with Image.open(art.ROOT / self.atlas_record["path"]) as image:
            self.atlas = image.copy()

    def test_public_coverage_preserves_separate_field_rendering_gate(self):
        """Bind all384 source poses without granting packed range or runtime acceptance.

        @param self Test instance checking this exact published rich-source boundary.
        """
        manifest = json.loads((art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text())
        row = next(row for row in manifest["assets"] if row["asset_id"] == "asteroid.rare-earth")
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

    def test_decode_all384_saved_size_view_state_frame_rectangles(self):
        """Verify every actual indexed source rectangle and its fixed native anchor.

        @param self Test instance checking this exact published rich-source boundary.
        """
        self.assertEqual(self.atlas.size, (768, 3840))
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.getpalette(), art.base.PALETTE_BYTES)
        self.assertFalse((set(self.atlas.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
        expected = {(size, direction, state, frame) for size in asteroids.SIZES
                    for direction in asteroids.DIRECTIONS for state in asteroids.STATES for frame in range(4)}
        actual = set()
        # Native decoded pixels, rather than metadata counts alone, must supply every original saved size and state before a later packer can consume the source.
        for row in self.report["entries"]:
            key = row["size"], row["direction"], row["state"], row["frame"]
            self.assertNotIn(key, actual)
            actual.add(key)
            self.assertEqual(row["variant"], asteroids.SIZES.index(row["size"]))
            self.assertEqual(row["anchor"], [-48, -48])
            pose = self.atlas.crop(row["box"])
            self.assertEqual(pose.size, (96, 80))
            self.assertTrue(any(pose.get_flattened_data()))
            self.assertEqual(hashlib.sha256(pose.tobytes()).hexdigest(), row["pixel_sha256"])
        self.assertEqual(actual, expected)

    def test_logical_aliases_retain96_actual_component_poses(self):
        """Count original/free, two real mining phases and worked mineral without invented spin.

        @param self Test instance checking this exact published rich-source boundary.
        """
        poses = {(row["size"], row["direction"], row["state"], row["frame"]): self.atlas.crop(row["box"]).tobytes()
                 for row in self.report["entries"]}
        count = 0
        # A separately drawn native probe leaves attachment identical to the free body; static/repeated frames refer to actual originals rather than additional authored work.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                values = {poses[size, direction, state, frame] for state in asteroids.STATES for frame in range(4)}
                self.assertEqual(len(values), 4)
                count += len(values)
                for frame in range(4):
                    self.assertEqual(poses[size, direction, "free", frame], poses[size, direction, "free", 0])
                    self.assertEqual(poses[size, direction, "attached", frame], poses[size, direction, "free", 0])
                    self.assertEqual(poses[size, direction, "mining", frame], poses[size, direction, "mining", frame % 2])
                    self.assertEqual(poses[size, direction, "depleted", frame], poses[size, direction, "depleted", 0])
        self.assertEqual(count, self.report["actual_component_pose_count"])
        self.assertEqual(self.report["explicit_source_alias_count"], 384 - count)

    def test_nine_own_originals_retain_executed_prompt_and_body_input_roles(self):
        """Retain independent rich originals and their exact own model-edit ancestry.

        @param self Test instance checking this exact published rich-source boundary.
        """
        self.assertEqual(self.report["new_original_source_count"], 9)
        self.assertEqual(self.report["registered_source_instance_count"], 0)
        # Every selected body/effect/worked source keeps its exact immutable original and executed prompt receipt; later source additions do not rewrite this body's dated ancestry.
        for snapshot in self.report["source_candidates"]:
            candidate = art.source_candidate(snapshot["key"], self.document)
            self.assertEqual(candidate, snapshot)
            self.assertEqual(candidate["asset_id"], "asteroid.rare-earth")
            receipt = json.loads((art.ROOT / candidate["generation_receipt"]["path"]).read_text())
            self.assertEqual(receipt["prompt"], candidate["prompt"])
            self.assertEqual(receipt["sha256"], candidate["sha256"])
            self.assertEqual(receipt["tool"], "built-in image_gen")
            self.assertTrue(receipt["transparent_background"])
            self.assertTrue(all(reference["role"] for reference in receipt["reference_inputs"]))
            self.assertTrue(all("rich-asteroid" in reference["path"] for reference in candidate["references"]))

    def test_complete_original_layout_preserves_alpha_and_separated_views(self):
        """Bind whole-original separation independently of local mineral masks.

        @param self Test instance checking this exact published rich-source boundary.
        """
        audit = json.loads((art.ROOT / "logs/space-rich-source-layout-verification.json").read_text())
        self.assertEqual(audit["source_review"], self.proof)
        self.assertEqual(len(audit["originals"]), 9)
        # No local part mask can hide a missing full subject, intrusive frame or unregistered model pixel in a source gutter.
        for original in audit["originals"]:
            self.assertEqual(original["visible_nonseparator_pixels_outside"], 0)
            self.assertFalse(original["source_alpha_modified"])
            self.assertEqual(art.identity(art.ROOT / original["original"]["path"]), original["original"])
            for view in original["views"]:
                self.assertGreaterEqual(view["minimum_alpha16_clearance"], 16)
                self.assertEqual(view["visible_separator_pixels_inside"], 0)

    def test_final_images_equal_all16_reviewed_native_and_contact_artifacts(self):
        """Freezing selected source snapshots must preserve every reviewed image byte.

        @param self Test instance checking this exact published rich-source boundary.
        """
        # Publication metadata is finalized after native/4x review; no later atlas or contact may replace the accepted pixels.
        for artifact in self.report["artifacts"]:
            self.assertEqual(art.identity(art.ROOT / artifact["path"]), artifact)
            prior = art.ROOT / "logs/proofs/mlm/native-art/space/rich-complete-v3" / Path(artifact["path"]).name
            self.assertEqual(art.identity(prior)["sha256"], artifact["sha256"])

    def test_corrected_small_ne_and_large_w_glints_rebuild_exactly(self):
        """Cross actual corrected Small NE and Large W contact-glint source boundaries.

        @param self Test instance checking this exact published rich-source boundary.
        """
        sources = asteroids.AsteroidSources("asteroid.rare-earth", self.document)
        # These actual corrected glints exercise two earlier native readability failures without requalifying unrelated completed families.
        for size, direction, state, frame in (("small", "NE", "mining", 0), ("large", "W", "mining", 1)):
            row = next(row for row in self.report["entries"] if (row["size"], row["direction"], row["state"], row["frame"]) == (size, direction, state, frame))
            pixels, _evidence = sources.pose(direction, size, state, frame)
            self.assertEqual(pixels.tobytes(), self.atlas.crop(row["box"]).tobytes())


if __name__ == "__main__":
    unittest.main()
