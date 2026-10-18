"""Protect the selected heavy-miner base without claiming the remaining sources.

@file space_art_heavy.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups65.1-65.2
Only measured original rectangles, existing registration and the established
palette converter are exercised; no missing heading or work state is generated.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art

KEY = "heavy-miner-free-n-ne-e-se-v2"
ASSET = "vehicle.model-21"


class HeavyMinerBaseTests(unittest.TestCase):
    """Reject changed ancestry, guessed views, shifted anchors and silent clipping."""

    def setUp(self):
        """Load one detached selected-source record for isolated corruption checks.

        @param self Independent original-source fixture.
        """
        self.record = art.read_record()
        self.candidate = art.source_candidate(KEY, self.record)

    def test_four_distinct_genuine_original_views(self):
        """Every reviewed view comes from its own original measured source window.

        @param self Source identity and native direction fixture.
        """
        self.assertEqual(self.candidate["directions"], ["N", "NE", "E", "SE"])
        hashes = set()
        # Distinct original cameras must produce distinct native pixels; copying another heading cannot fill the missing directional coverage.
        for direction in self.candidate["directions"]:
            native, evidence = art.native_view(KEY, direction, self.record)
            hashes.add(hashlib.sha256(native.tobytes()).hexdigest())
            self.assertEqual(evidence["original"]["sha256"], self.candidate["sha256"])
        self.assertEqual(len(hashes), 4)

    def test_missing_heading_cannot_be_inferred(self):
        """A selected first-quarter source cannot supply an opposite camera view.

        @param self Missing original heading fixture.
        """
        with self.assertRaisesRegex(ValueError, "missing genuine Space heading"):
            art.native_view(KEY, "S", self.record)

    def test_changed_original_hash_refuses(self):
        """Source provenance binds the actual retained original rather than its name.

        @param self Incorrect original identity fixture.
        """
        self.candidate["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            art.source_candidate(KEY, self.record)

    def test_changed_input_reference_refuses(self):
        """The original model edit target remains part of the exact source ancestry.

        @param self Incorrect referenced-original identity fixture.
        """
        self.candidate["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            art.source_candidate(KEY, self.record)

    def test_approved_world_registration_cannot_drift(self):
        """Later variants cannot quietly shift the selected original body anchor.

        @param self Altered original-to-world registration fixture.
        """
        self.candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "approved Space registration changed"):
            art.native_view(KEY, "N", self.record)

    def test_large_native_source_cannot_be_silently_clipped(self):
        """An invalid scale must refuse before canvas pasting can erase hardware.

        @param self Oversized source-to-native registration fixture.
        """
        candidate = copy.deepcopy(self.candidate)
        candidate["native_transforms"]["E"]["uniform_scale"] = [1, 2]
        with self.assertRaisesRegex(ValueError, "approved native envelope"):
            art.source_pixels(candidate, "E", self.record["families"][ASSET]["native_art_envelope"])

    def test_rejected_original_cannot_reenter(self):
        """The first sheet's inadequate clearance cannot be rescued as another state.

        @param self Explicitly rejected generation fixture.
        """
        with self.assertRaisesRegex(ValueError, "rejected Space source"):
            art.source_candidate("heavy-miner-free-n-ne-e-se-v1", self.record)

    def test_partial_review_preserves_native_canvas_and_claims(self):
        """Baseline review proves exact64x48 cells while keeping full source/runtime separate.

        @param self Actual source artifact and canvas fixture.
        """
        report = json.loads((ROOT / "logs/proofs/mlm/native-art/space/heavy-miner-free-v2/source-review.json").read_text(encoding="utf-8"))
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime"])
        # All actual native cells keep the same palette/canvas and complete original bounds; a direction cannot pass by losing its body during quantization.
        for direction in self.candidate["directions"]:
            native, evidence = art.native_view(KEY, direction, self.record)
            self.assertEqual(native.mode, "P")
            self.assertEqual(native.size, (64, 48))
            self.assertTrue(any(native.tobytes()))
            x0, y0, x1, y1 = evidence["native_subject_bounds"]
            self.assertTrue(0 <= x0 < x1 <= 64)
            self.assertTrue(0 <= y0 < y1 <= 48)


class HeavyMinerEightViewTests(unittest.TestCase):
    """Protect the complete genuine base cameras before source state variants."""

    def setUp(self):
        """Load only current selected heavy-family camera ownership.

        @param self Detached eight-view source fixture.
        """
        self.record = art.read_record()
        self.family = self.record["families"][ASSET]

    def test_all_eight_cameras_are_distinct_original_views(self):
        """Opposite headings must have actual retained source pixels, not inferred copies.

        @param self Full genuine-camera coverage fixture.
        """
        hashes = set()
        originals = set()
        # Each engine view resolves its own explicit selected source and immutable registration; no missing orientation can fall back to a neighbouring view.
        for direction in self.family["directions"]:
            key = self.family["selections"]["free/" + direction]
            native, evidence = art.native_view(key, direction, self.record)
            hashes.add(hashlib.sha256(native.tobytes()).hexdigest())
            originals.add(evidence["original"]["sha256"])
        self.assertEqual(len(hashes), 8)
        self.assertEqual(len(originals), 2)

    def test_opposed_chassis_extents_remain_consistent(self):
        """Normalized original batches preserve the same physical machinery across180degree yaw.

        @param self Native model-scale continuity fixture.
        """
        extents = {}
        for direction in self.family["directions"]:
            key = self.family["selections"]["free/" + direction]
            _, evidence = art.native_view(key, direction, self.record)
            left, top, right, bottom = evidence["native_subject_bounds"]
            extents[direction] = (right - left, bottom - top)
        # Natural occlusion and pixel quantization can alter the outer silhouette slightly; a different-sized robot cannot hide behind a heading label.
        for first, opposite in (("N", "S"), ("NE", "SW"), ("E", "W"), ("SE", "NW")):
            self.assertLessEqual(abs(extents[first][0] - extents[opposite][0]), 4)
            self.assertLessEqual(abs(extents[first][1] - extents[opposite][1]), 4)

    def test_each_original_uses_one_uniform_physical_scale(self):
        """Camera normalization cannot squash one axis or independently resize a direction.

        @param self Original-scale and allowed-transform fixture.
        """
        selected = set(self.family["selections"].values())
        # Separate generated sheets may have different raw pixel framing, but all real views within each sheet retain one physical scale and the same world anchor.
        for key in selected:
            candidate = art.source_candidate(key, self.record)
            scales = {tuple(transform["uniform_scale"]) for transform in candidate["native_transforms"].values()}
            self.assertEqual(len(scales), 1)
            for transform in candidate["native_transforms"].values():
                self.assertFalse({"rotation", "flip", "mirror", "scale_x", "scale_y"} & transform.keys())

    def test_opposite_registration_cannot_drift(self):
        """The newly selected opposite cameras retain the same protection as the first four.

        @param self Altered opposite-view world reference fixture.
        """
        key = self.family["selections"]["free/S"]
        art.source_candidate(key, self.record)["native_transforms"]["S"]["source_reference"][1] += 1
        with self.assertRaisesRegex(ValueError, "approved Space registration changed"):
            art.native_view(key, "S", self.record)

    def test_opposite_actual_model_reference_remains_immutable(self):
        """A plausible new model cannot replace the actual image input ancestry.

        @param self Corrupt selected-model image reference fixture.
        """
        key = self.family["selections"]["free/S"]
        candidate = art.source_candidate(key, self.record)
        candidate["references"][1]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            art.source_candidate(key, self.record)

    def test_clipped_opposite_trial_cannot_reenter(self):
        """The discarded S drill crop cannot supply a work component or a hidden heading.

        @param self Explicit rejected opposite-trial fixture.
        """
        with self.assertRaisesRegex(ValueError, "rejected Space source"):
            art.source_candidate("heavy-miner-free-s-sw-w-nw-v1", self.record)

    def test_full_base_review_keeps_state_coverage_separate(self):
        """Eight base views cannot masquerade as all128 logical state/frame sources.

        @param self Complete base artifact with explicitly partial state coverage.
        """
        review = json.loads((ROOT / "logs/proofs/mlm/native-art/space/heavy-miner-free-eight-v1/source-review.json").read_text(encoding="utf-8"))
        self.assertEqual([view["direction"] for view in review["views"]], self.family["directions"])
        self.assertFalse(review["complete_source_coverage"])
        self.assertFalse(review["packed"])
        self.assertFalse(review["runtime"])


if __name__ == "__main__":
    unittest.main()
