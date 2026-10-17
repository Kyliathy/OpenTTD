"""Verify genuine passive-pod views, fixed native registration and source ancestry.

@file space_art_pod.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BE
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group64
Source admission preserves the original separate SpacePod/SpaceDescentJob custody owner and grants no packets, work or recovered shell.
"""
from pathlib import Path
import sys
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art

ASSET = "vehicle.model-18"


class PassivePodOriginalTests(unittest.TestCase):
    """Require eight actual shell views, correct front/back visibility and immutable original provenance."""

    def setUp(self):
        """Load detached source definitions for each original or refusal case."""
        self.document = art.read_record()
        self.family = self.document["families"][ASSET]
        self.candidate = art.source_candidate(self.family["selections"]["free/N"], self.document)

    def test_all_eight_original_shell_views_keep_native_palette_and_anchor(self):
        """Every actual heading retains its own selected source registration, without inventing a Vehicle direction owner."""
        self.assertEqual(self.family["directions"], ["N", "NE", "E", "SE", "S", "SW", "W", "NW"])
        self.assertEqual(self.family["public_anchor"], [-32, -32])
        poses = []
        # Resolve each actual physical view independently. Neither rotational symmetry nor a neighboring picture can stand in for the one forward hatch and two rear tabs.
        for direction in self.family["directions"]:
            key = self.family["selections"]["free/" + direction]
            candidate = art.source_candidate(key, self.document)
            self.assertEqual(candidate["asset_id"], ASSET)
            self.assertNotIn(direction, candidate.get("unaccepted_directions", {}))
            pose, evidence = art.native_view(key, direction, self.document)
            self.assertEqual(pose.size, (64, 48))
            self.assertEqual(pose.getpalette(), art.base.PALETTE_BYTES)
            self.assertFalse((set(pose.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
            self.assertEqual(evidence["transform"]["uniform_scale"], [3, 50])
            poses.append(pose.tobytes())
        self.assertEqual(len(set(poses)), 8)
        self.assertFalse(self.family["runtime_activated"])

    def test_complete_original_models_stay_inside_measured_source_windows(self):
        """Every visible nonseparator pixel stays inside one complete original view, including unused control views."""
        keys = set(self.family["selections"].values())
        policy = dict(red_min=100, blue_min=100, green_max=128, chroma_min=64)
        # Complete-image containment prevents a clipped or stray shell fragment outside the declared source windows from disappearing during extraction.
        for key in keys:
            candidate = art.source_candidate(key, self.document)
            original = Image.open(art.ROOT / candidate["path"]).convert("RGBA")
            outside = original.getchannel("A").point([0] * 16 + [255] * 240)
            for transform in candidate["native_transforms"].values():
                outside.paste(0, transform["crop_box"])
            visible = [(pixel, flag) for pixel, flag in zip(original.get_flattened_data(), outside.get_flattened_data(), strict=True) if flag]
            self.assertTrue(all(art.source_separator(pixel, policy) for pixel, flag in visible))

    def test_superseded_direct_front_back_views_cannot_reenter_conversion(self):
        """The retained N hatch-on-near-face and S missing-hatch attempts cannot bypass model-continuity rejection."""
        # These originals retain other valid selected views. Reject only the explicitly failed heading, without discarding provenance or silently relabeling a source picture.
        for key, direction in (("return-pod-packed-n-ne-e-se-v1", "N"), ("return-pod-packed-s-sw-w-nw-v1", "S")):
            candidate = art.source_candidate(key, self.document)
            with self.assertRaisesRegex(ValueError, "unaccepted"):
                art.source_pixels(candidate, direction, self.family["native_art_envelope"])

    def test_changed_body_reference_refuses(self):
        """One source-coordinate change cannot shift the selected shell relative to its native custody position."""
        self.candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            art.native_view(self.candidate["key"], "N", self.document)

    def test_missing_true_heading_refuses(self):
        """The corrected direct-view original cannot provide an unauthored diagonal view."""
        with self.assertRaisesRegex(ValueError, "heading"):
            art.native_view(self.candidate["key"], "NE", self.document)

    def test_changed_original_bytes_refuse(self):
        """A plausible replacement file cannot inherit the actual selected shell's source identity."""
        self.candidate["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            art.native_view(self.candidate["key"], "N", self.document)

    def test_changed_actual_prompt_receipt_refuses(self):
        """The actual executed correction and its two real input images remain immutable ancestry."""
        self.candidate["generation_receipt"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            art.native_view(self.candidate["key"], "N", self.document)

    def test_changed_world_anchor_refuses(self):
        """The passive shell source cannot move its public world anchor to conceal a placement error."""
        self.family["public_anchor"][1] += 1
        with self.assertRaisesRegex(ValueError, "anchor"):
            art.native_view(self.candidate["key"], "N", self.document)

    def test_assumed_canonical_crop_refuses_shifted_separator_frames(self):
        """Actual measured deeper windows preserve the original source; assumed frame geometry cannot become a shell pixel."""
        transform = self.candidate["native_transforms"]["N"]
        transform["crop_box"] = [32, 32, 736, 480]
        transform["measured_subject_bbox_in_crop"] = [0, 0, 704, 448]
        with self.assertRaisesRegex(ValueError, "clearance"):
            art.source_pixels(self.candidate, "N", self.family["native_art_envelope"])


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
