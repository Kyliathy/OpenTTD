"""Verify the actual orbital cargo hauler's selected empty-body source cameras.

@file space_art_hauler.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups66.1/66.2/66B.1
Only actual original geometry is selected here. Native packets, capacity,
navigation and service retain ownership; full source/packing/runtime are later.
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
from tools.orbital_art_contract import hauler_contract, validate_orbital_contract

ASSET = "vehicle.model-17"
KEY = "orbital-hauler-empty-n-ne-e-se-v1"


class OrbitalHaulerBaseTests(unittest.TestCase):
    """Protect genuine empty-deck views, fixed body registration and native freight role."""

    def setUp(self):
        """Load detached source metadata before an actual-source or refusal check.

        @param self Independent original-hauler source fixture.
        """
        self.record = art.read_record()
        self.family = self.record["families"][ASSET]
        self.candidate = art.source_candidate(KEY, self.record)

    def test_four_actual_empty_cameras_have_distinct_original_pixels(self):
        """The generator supplied real N/NE/E/SE cameras rather than inferred headings.

        @param self Actual first-source camera/ancestry fixture.
        """
        self.assertEqual(self.candidate["directions"], ["N", "NE", "E", "SE"])
        receipt = json.loads((ROOT / self.candidate["generation_receipt"]["path"]).read_text(encoding="utf-8"))
        self.assertEqual([row["actual"] for row in receipt["actual_camera_mapping"]], self.candidate["directions"])
        self.assertEqual(receipt["reference_inputs"], [])
        native_hashes = set()
        # Every observed camera owns a real separate source rectangle. A duplicate body picture, neighboring direction or another model cannot fill missing physical geometry.
        for direction in self.candidate["directions"]:
            self.assertEqual(self.family["selections"]["free/" + direction], KEY)
            native, evidence = art.native_view(KEY, direction, self.record)
            self.assertEqual(evidence["original"]["sha256"], self.candidate["sha256"])
            native_hashes.add(hashlib.sha256(native.tobytes()).hexdigest())
        self.assertEqual(len(native_hashes), 4)

    def test_selected_physical_scale_and_world_references_remain_fixed(self):
        """Later payload/travel parts cannot shift the accepted original barge.

        @param self Frozen first-body physical geometry fixture.
        """
        references = dict(N=[384, 344], NE=[1150, 350], E=[386, 832], SE=[1140, 845])
        # One scale preserves the physical model's long deck and true foreshortening; each original chassis contact remains at native32,32 without axis stretching.
        for direction, point in references.items():
            transform = self.candidate["native_transforms"][direction]
            self.assertEqual(transform["uniform_scale"], [1, 13])
            self.assertEqual(transform["source_reference"], point)
            self.assertEqual(art.registration_signature(self.candidate, direction), self.family["approved_base_registration"][direction])

    def test_missing_opposite_camera_cannot_be_inferred(self):
        """A flat rotation or requested future compass slot supplies no original S view.

        @param self Missing genuine opposite-camera fixture.
        """
        with self.assertRaisesRegex(ValueError, "missing genuine Space heading"):
            art.native_view(KEY, "S", self.record)

    def test_all_eight_selected_body_cameras_are_genuine_and_distinct(self):
        """Every compass slot comes from a safely measured original of this barge.

        @param self Complete original empty-deck camera fixture.
        """
        hashes = set()
        # A populated compass cannot hide repeated pictures, a foreign body or missing physical transforms; each real source retains this family's fixed registration.
        for direction in self.family["directions"]:
            key = self.family["selections"]["free/" + direction]
            candidate = art.source_candidate(key, self.record)
            self.assertEqual(candidate["asset_id"], ASSET)
            self.assertIn(direction, candidate["directions"])
            native, evidence = art.native_view(key, direction, self.record)
            self.assertEqual(native.size, (64, 48))
            self.assertEqual(evidence["original"], art.identity(ROOT / candidate["path"]))
            hashes.add(hashlib.sha256(native.tobytes()).hexdigest())
        self.assertEqual(len(hashes), 8)

    def test_opposing_views_retain_one_physical_body_scale(self):
        """A camera orbit cannot shrink the complete barge to conceal a bad projection.

        @param self Opposing native visible-extent continuity fixture.
        """
        # The head/nozzle have different visible height, but opposing widths/deck mass must stay comparable at the same native world anchor.
        for first, opposite in (("N", "S"), ("NE", "SW"), ("E", "W"), ("SE", "NW")):
            a, _ = art.native_view(self.family["selections"]["free/" + first], first, self.record)
            b, _ = art.native_view(self.family["selections"]["free/" + opposite], opposite, self.record)
            ab, bb = a.getbbox(), b.getbbox()
            self.assertLessEqual(abs((ab[2] - ab[0]) - (bb[2] - bb[0])), 6)
            self.assertLessEqual(abs((ab[3] - ab[1]) - (bb[3] - bb[1])), 8)

    def test_unaccepted_opposite_parts_cannot_be_added_back_by_transform(self):
        """Unsafe SW margins and W's extra display remain excluded from their original.

        @param self Explicit failed original-camera/layout fixture.
        """
        candidate = art.source_candidate("orbital-hauler-empty-s-sw-w-nw-v1", self.record)
        # Accepting S/NW from this sheet does not approve its other physical parts, even if a caller invents matching transform fields for them.
        for direction in ("SW", "W"):
            candidate["native_transforms"][direction] = copy.deepcopy(candidate["native_transforms"]["S"])
            with self.assertRaisesRegex(ValueError, "unaccepted original Space view"):
                art.source_pixels(candidate, direction, self.family["native_art_envelope"])

    def test_separate_view_reference_ancestry_cannot_change(self):
        """The genuine W fallback remains bound to the untouched approved E reference.

        @param self Corrupted original camera-reference fixture.
        """
        candidate = art.source_candidate(self.family["selections"]["free/W"], self.record)
        self.assertEqual(candidate["directions"], ["W"])
        candidate["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            art.source_candidate(candidate["key"], self.record)

    def test_complete_empty_compass_is_separate_from_all_state_coverage(self):
        """Eight approved body views do not claim the complete load/travel/status handoff.

        @param self Dated eight-camera native source-review fixture.
        """
        path = ROOT / "logs/proofs/mlm/native-art/space/hauler-empty-eight-view-v1/source-review.json"
        report = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(len(report["views"]), 8)
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime"])

    def test_changed_original_bytes_refuse(self):
        """The approved cargo barge remains tied to its actual generated original.

        @param self Corrupted raw original identity fixture.
        """
        self.candidate["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            art.source_candidate(KEY, self.record)

    def test_changed_exact_request_receipt_refuses(self):
        """A new prompt or false camera claim cannot replace the executed request.

        @param self Corrupted exact generation provenance fixture.
        """
        self.candidate["generation_receipt"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            art.source_candidate(KEY, self.record)

    def test_shifted_approved_body_reference_refuses(self):
        """Load/service authoring cannot move the original nozzle, rails or command head.

        @param self Shifted source-body world-contact fixture.
        """
        self.candidate["native_transforms"]["N"]["source_reference"][1] += 1
        with self.assertRaisesRegex(ValueError, "approved Space registration changed"):
            art.native_view(KEY, "N", self.record)

    def test_oversized_original_cannot_be_silently_clipped(self):
        """A false scale cannot discard a physical rail/head/engine at the native edge.

        @param self Invalid full-model source scale fixture.
        """
        candidate = copy.deepcopy(self.candidate)
        candidate["native_transforms"]["E"]["uniform_scale"] = [1, 2]
        with self.assertRaisesRegex(ValueError, "approved native envelope"):
            art.source_pixels(candidate, "E", self.family["native_art_envelope"])

    def test_general_freight_contract_retains_static_and_moving_load_states(self):
        """Native17 load cues cannot borrow another role or lose stopped cargo semantics.

        @param self Public role/geometry and original native source-frame fixture.
        """
        manifest = json.loads((ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
        row = next(row for row in manifest["assets"] if row["asset_id"] == ASSET)
        validate_orbital_contract(row)
        self.assertEqual(self.family["native_model"], 17)
        self.assertEqual(self.family["states"], hauler_contract()["states"])
        self.assertEqual(len(row["states"]) * len(row["directions"]) * row["frames"], 160)
        self.assertEqual(row["source_frame_contract"]["partial"], [0, 0])
        self.assertEqual(row["source_frame_contract"]["moving-partial"], [0, 1])
        row["source_frame_contract"]["partial"] = [0, 1]
        with self.assertRaisesRegex(ValueError, "native orbital geometry/role contract changed"):
            validate_orbital_contract(row)

    def test_first_source_review_preserves_native_geometry_without_full_claim(self):
        """A selected four-camera empty deck is separate from all authored state poses.

        @param self Dated original native source-review fixture.
        """
        review = json.loads((ROOT / "logs/proofs/mlm/native-art/space/hauler-empty-first-v2/source-review.json").read_text(encoding="utf-8"))
        self.assertEqual(len(review["views"]), 4)
        self.assertFalse(review["complete_source_coverage"])
        self.assertFalse(review["packed"])
        self.assertFalse(review["runtime"])
        # Each accepted view remains a nonempty original barge inside the public cell at the fixed anchor; no empty slot or wider canvas conceals clipped equipment.
        for direction in self.candidate["directions"]:
            native, _ = art.native_view(KEY, direction, self.record)
            self.assertEqual(native.size, (64, 48))
            self.assertEqual(self.family["public_anchor"], [-32, -32])
            self.assertTrue(any(native.get_flattened_data()))


if __name__ == "__main__":
    unittest.main()
