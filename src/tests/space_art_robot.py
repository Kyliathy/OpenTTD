"""Verify the retained cargo-less robot's selected original native source cameras.

@file space_art_robot.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13CB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65B.1/65B.2
Source geometry/provenance checks grant no cargo, construction/mining work or
runtime art. Actual working imagery remains the native contract's idle alias.
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
from tools.orbital_art_contract import validate_orbital_contract

ASSET = "vehicle.model-16"
KEY = "construction-robot-idle-s-sw-e-se-v1"


class RetainedRobotBaseTests(unittest.TestCase):
    """Protect genuine idle cameras, original world registration and retained role."""

    def setUp(self):
        """Load a detached original source and exact retained native contract.

        @param self Independent selected-base source fixture.
        """
        self.record = art.read_record()
        self.family = self.record["families"][ASSET]
        self.candidate = art.source_candidate(KEY, self.record)

    def test_four_actual_cameras_are_genuine_and_distinct(self):
        """Select the rendered physical S/SW/E/SE views without inventing requested N/NE.

        @param self Actual original-camera and source-selection fixture.
        """
        self.assertEqual(self.candidate["directions"], ["S", "SW", "E", "SE"])
        receipt = json.loads((ROOT / self.candidate["generation_receipt"]["path"]).read_text(encoding="utf-8"))
        self.assertEqual([row["actual"] for row in receipt["actual_camera_mapping"]], self.candidate["directions"])
        hashes = set()
        # Each accepted camera owns a different real source rectangle and independent native pixels. Neither requested labels nor another family's body can supply absent geometry.
        for direction in self.candidate["directions"]:
            self.assertEqual(self.family["selections"]["free/" + direction], KEY)
            native, evidence = art.native_view(KEY, direction, self.record)
            self.assertEqual(self.candidate["asset_id"], ASSET)
            self.assertEqual(evidence["original"]["sha256"], self.candidate["sha256"])
            hashes.add(hashlib.sha256(native.tobytes()).hexdigest())
        self.assertEqual(len(hashes), 4)

    def test_missing_original_heading_cannot_be_inferred(self):
        """The generator's requested N label cannot override its actual front-down view.

        @param self Missing genuine camera fixture.
        """
        with self.assertRaisesRegex(ValueError, "missing genuine Space heading"):
            art.native_view(KEY, "N", self.record)

    def test_all_eight_selected_cameras_have_independent_original_geometry(self):
        """Complete the compass from genuine originals rather than copied neighboring views.

        @param self Eight-heading source continuity and registration fixture.
        """
        signatures = set()
        # Each runtime heading needs its own observed physical camera; a complete key list cannot hide repeated front views or another hull's source.
        for direction in self.family["directions"]:
            key = self.family["selections"]["free/" + direction]
            candidate = art.source_candidate(key, self.record)
            self.assertEqual(candidate["asset_id"], ASSET)
            self.assertIn(direction, candidate["directions"])
            native, evidence = art.native_view(key, direction, self.record)
            self.assertEqual(native.size, (64, 48))
            self.assertEqual(self.family["public_anchor"], [-32, -32])
            self.assertEqual(art.registration_signature(candidate, direction), self.family["approved_base_registration"][direction])
            self.assertEqual(evidence["original"], art.identity(ROOT / candidate["path"]))
            signatures.add(hashlib.sha256(native.tobytes()).hexdigest())
        self.assertEqual(len(signatures), 8)

    def test_failed_opposite_sheet_cannot_supply_any_camera(self):
        """Preserve the explicit changed-model rejection even after missing cameras arrive.

        @param self Rejected two-nozzle/roof-placement trial fixture.
        """
        key = "construction-robot-idle-n-ne-w-nw-v1"
        self.assertNotIn(key, self.family["selections"].values())
        with self.assertRaisesRegex(ValueError, "rejected Space source"):
            art.native_view(key, "W", self.record)

    def test_rejected_high_camera_cannot_reenter_selected_north(self):
        """Keep the oversized rear projection out of the native body after its correction.

        @param self Rejected steep-camera and compact corrected-view fixture.
        """
        self.assertEqual(self.family["selections"]["free/N"], "construction-robot-idle-n-v2")
        with self.assertRaisesRegex(ValueError, "rejected Space source"):
            art.native_view("construction-robot-idle-n-v1", "N", self.record)

    def test_opposing_cameras_keep_one_physical_body_scale(self):
        """Opposite real cameras fit comparable extents around the original world anchor.

        @param self Opposing body-size continuity fixture; asymmetric coupler/nozzle occlusion may change visible extents.
        """
        # A genuine camera orbit can reveal the longer rear nozzle, but cannot silently shrink the complete opposite body to conceal a high/incorrect projection.
        for first, opposite in (("N", "S"), ("NE", "SW"), ("E", "W"), ("SE", "NW")):
            a, _ = art.native_view(self.family["selections"]["free/" + first], first, self.record)
            b, _ = art.native_view(self.family["selections"]["free/" + opposite], opposite, self.record)
            ab, bb = a.getbbox(), b.getbbox()
            self.assertLessEqual(abs((ab[2] - ab[0]) - (bb[2] - bb[0])), 6)
            self.assertLessEqual(abs((ab[3] - ab[1]) - (bb[3] - bb[1])), 8)

    def test_first_four_approved_world_registrations_remain_exact(self):
        """New physical cameras cannot reposition the original accepted four-view body.

        @param self Frozen first-source registration fixture.
        """
        expected = {
            "S": "f361cee9c761c9c8bc93a5cf0a52fc4a6aa202c64b38937f60ef5d3b40f346d9",
            "SW": "65ea5ba9c4f1e11489653106a425d3283bdcbbc1e5920e10a92e37e7e81f684a",
            "E": "080ac64217a50b98617d223e86ac98916dc4d77aaf8df8c7b2b856dd67056683",
            "SE": "a7d583a53723d6c853b9eb4b26dad429f96216a9932ecf2f6590228e157bda40",
        }
        # These accepted world contacts protect animation continuity across later source phases; camera completion must fit around them.
        for direction, signature in expected.items():
            self.assertEqual(art.registration_signature(self.candidate, direction), signature)
            self.assertEqual(self.family["approved_base_registration"][direction], signature)

    def test_single_view_reference_ancestry_cannot_be_substituted(self):
        """The separate W fallback remains bound to its untouched approved E source guide.

        @param self Changed camera-reference ancestry fixture.
        """
        key = "construction-robot-idle-w-v1"
        candidate = art.source_candidate(key, self.record)
        self.assertEqual(candidate["directions"], ["W"])
        candidate["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            art.source_candidate(key, self.record)

    def test_changed_retained_original_hash_refuses(self):
        """The selected model binds actual original bytes rather than a filename.

        @param self Corrupted immutable original fixture.
        """
        self.candidate["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            art.source_candidate(KEY, self.record)

    def test_changed_exact_prompt_receipt_refuses(self):
        """The complete actual source request and truthful camera mapping remain immutable.

        @param self Corrupted original generation receipt fixture.
        """
        self.candidate["generation_receipt"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            art.source_candidate(KEY, self.record)

    def test_approved_world_reference_cannot_drift(self):
        """Later source phases cannot silently shift the selected original chassis.

        @param self Shifted fixed world-registration fixture.
        """
        self.candidate["native_transforms"]["S"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "approved Space registration changed"):
            art.native_view(KEY, "S", self.record)

    def test_oversized_original_cannot_be_clipped_to_native_canvas(self):
        """A false physical scale refuses before native pasting can erase real hardware.

        @param self Out-of-canvas actual source registration fixture.
        """
        candidate = copy.deepcopy(self.candidate)
        candidate["native_transforms"]["E"]["uniform_scale"] = [1, 2]
        with self.assertRaisesRegex(ValueError, "approved native envelope"):
            art.source_pixels(candidate, "E", self.family["native_art_envelope"])

    def test_native_work_contract_cannot_become_mining_or_active_building(self):
        """Retained16 keeps its actual idle-working alias and distinct original native role.

        @param self Actual published role/capability source fixture.
        """
        manifest = json.loads((ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
        row = next(row for row in manifest["assets"] if row["asset_id"] == ASSET)
        validate_orbital_contract(row)
        self.assertEqual(self.family["native_model"], 16)
        self.assertEqual(self.family["states"], row["states"])
        self.assertEqual(row["source_frame_contract"]["working"], [0, 0])
        self.assertEqual(len(row["states"]) * len(row["directions"]) * row["frames"], 80)
        row["source_frame_contract"]["working"] = [0, 1]
        with self.assertRaisesRegex(ValueError, "native orbital geometry/role contract changed"):
            validate_orbital_contract(row)

    def test_partial_review_preserves_geometry_without_claiming_full_source(self):
        """Four native views retain exact geometry while the80-pose handoff stays pending.

        @param self Dated native source review fixture.
        """
        path = ROOT / "logs/proofs/mlm/native-art/space/retained-robot-idle-v1/source-review.json"
        report = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(len(report["views"]), 4)
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime"])
        # Every real original view fits the unchanged cell and world anchor with visible model pixels; no reserved template slot counts as source coverage.
        for direction in self.candidate["directions"]:
            native, _ = art.native_view(KEY, direction, self.record)
            self.assertEqual(native.size, (64, 48))
            self.assertEqual(self.family["public_anchor"], [-32, -32])
            self.assertTrue(any(native.get_flattened_data()))


if __name__ == "__main__":
    unittest.main()
