"""Verify the retained robot's complete authored source without native activation.

@file space_art_robot_family.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13CB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65B.3c
Working is the exact idle alias required by ReadyProbe's model20-only admission.
Source validation cannot grant work, cargo, movement, packing or runtime art.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_family_art as family
from tools.orbital_art_contract import retained_robot_contract

ASSET = "vehicle.model-16"
REVIEW = ROOT / "logs/proofs/mlm/native-art/space/retained-robot-complete-v1/space-source-manifest.json"


class RetainedRobotFamilyTests(unittest.TestCase):
    """Protect all logical source poses, unreachable-work aliases and receiving roles."""

    @classmethod
    def setUpClass(cls):
        """Load the real source matrix and independently resolve both endpoint cameras.

        @param cls Fixture sharing only original converted source pixels.
        """
        cls.source = art.read_record()
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        cls.entry = cls.report["atlases"][0]
        cls.atlas = Image.open(ROOT / cls.entry["path"])
        cls.sprites = {(row["state"], row["frame"], row["direction"]): row for row in cls.entry["sprites"]}
        cls.fresh = {}
        definitions = cls.source["families"][ASSET]
        # Independently exercise every actual source branch in N/NW. Cache only real shared components; the full primary conversion already resolves all48physical components.
        for direction in ("N", "NW"):
            for state in definitions["states"]:
                for frame in range(2):
                    policy = definitions["state_policy"][state]
                    key = policy["source"], policy["phase_alias"][frame], direction
                    if key not in cls.fresh:
                        cls.fresh[key] = family.state_pose(state, direction, frame, cls.source, ASSET)[0]

    def setUp(self):
        """Detach source policy before testing a refused authoring input.

        @param self Independent source/receiving-body fixture.
        """
        self.record = copy.deepcopy(self.source)
        self.definition = self.record["families"][ASSET]

    def pose(self, state, frame, direction):
        """Read an actual indexed native sprite from the published source atlas.

        @param self Complete source-matrix fixture.
        @param state Public retained robot source state.
        @param frame Logical source frame0/1.
        @param direction Genuine physical camera heading.
        @return Original indexed64x48 cell at the public fixed anchor.
        """
        x, y, width, height = self.sprites[state, frame, direction]["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def test_complete_eighty_pose_matrix_preserves_geometry(self):
        """Every public state/frame/heading has a nonempty original native cell.

        @param self Exact published source geometry fixture.
        """
        expected = {(state, frame, direction) for state in family.source_states(ASSET)
                    for frame in range(2) for direction in self.definition["directions"]}
        self.assertEqual(set(self.sprites), expected)
        self.assertEqual(self.report["pose_count"], 80)
        self.assertEqual(self.entry["missing_pose_count"], 0)
        self.assertEqual(self.atlas.size, (512, 480))
        self.assertEqual(hashlib.sha256((ROOT / self.entry["path"]).read_bytes()).hexdigest(), self.entry["sha256"])
        # A reserved or empty cell cannot masquerade as a delivered original pose; no state may shrink its native bounds or move the world anchor.
        for key, row in self.sprites.items():
            self.assertEqual(row["box"][2:], [64, 48])
            self.assertEqual(row["anchor"], [-32, -32])
            self.assertTrue(any(self.pose(*key).get_flattened_data()))

    def test_every_working_pose_is_exactly_idle(self):
        """Retained16 gains no work imagery from builder20 or heavy miner21.

        @param self Native model20-only work-admission alias fixture.
        """
        # Both logical frames in every physical camera must be exact idle pixels, including the folded arms and unlit display.
        for direction in self.definition["directions"]:
            for frame in range(2):
                self.assertEqual(self.pose("working", frame, direction).tobytes(), self.pose("idle", frame, direction).tobytes())
        self.assertEqual(self.definition["state_policy"]["working"]["source"], "base")

    def test_static_and_animated_phases_follow_the_native_contract(self):
        """Only moving/service animate; idle/working/blocked keep exact still aliases.

        @param self Complete canonical phase-contract fixture.
        """
        self.assertEqual(self.report["actual_component_pose_count"], 48)
        self.assertEqual(self.report["explicit_static_alias_count"], 24)
        # Test every real camera rather than trusting animation flags; service must remain readable even on the tiny E/W display faces.
        for state, phases in retained_robot_contract()["source_frame_contract"].items():
            self.assertEqual(self.definition["state_policy"][state]["phase_alias"], phases)
            for direction in self.definition["directions"]:
                equal = self.pose(state, 0, direction).tobytes() == self.pose(state, 1, direction).tobytes()
                self.assertEqual(equal, phases == [0, 0])

    def test_independent_endpoint_sources_match_published_cells(self):
        """Fresh real source assembly equals the atlas across both endpoint cameras.

        @param self Independently converted original-component fixture.
        """
        # Source-bank reuse is explicit; each logical idle-working/static alias must match the exact independently derived actual component.
        for direction in ("N", "NW"):
            for state in self.definition["states"]:
                for frame in range(2):
                    policy = self.definition["state_policy"][state]
                    key = policy["source"], policy["phase_alias"][frame], direction
                    self.assertEqual(self.pose(state, frame, direction).tobytes(), self.fresh[key].tobytes())

    def test_source_counts_and_runtime_boundaries_remain_truthful(self):
        """Shared gas and repeated component registrations do not become new artwork.

        @param self Source ancestry/count and runtime-gate fixture.
        """
        self.assertEqual(self.report["new_original_source_count"], 8)
        self.assertEqual(self.report["registered_source_instance_count"], 4)
        self.assertTrue(self.report["coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertTrue(all(row["asset_id"] == ASSET for row in self.report["source_candidates"]))

    def test_unsigned_source_policy_drift_refuses(self):
        """A later source change cannot silently rewrite the accepted phase ownership.

        @param self Corrupted approved policy fixture.
        """
        self.definition["state_policy"]["idle"]["owner"] = "another work owner"
        with self.assertRaisesRegex(ValueError, "approved native source state policy changed"):
            family.state_pose("idle", "N", 0, self.record, ASSET)

    def test_resigned_work_animation_cannot_grant_work(self):
        """Even a newly signed source policy must keep the unreachable work alias still.

        @param self Incorrect newly signed work-phase fixture.
        """
        self.definition["state_policy"]["working"].update(animated=True, phase_alias=[0, 1])
        self.definition["approved_state_policy_sha256"] = family.policy_signature(self.definition["state_policy"])
        with self.assertRaisesRegex(ValueError, "retained source state differs from public native contract"):
            family.state_pose("working", "N", 1, self.record, ASSET)

    def test_resigned_work_kind_cannot_replace_idle_pixels(self):
        """A blocked still cannot masquerade as working even when its phase list is static.

        @param self Incorrect newly signed source-kind fixture.
        """
        self.definition["state_policy"]["working"]["source"] = "blocked"
        self.definition["approved_state_policy_sha256"] = family.policy_signature(self.definition["state_policy"])
        with self.assertRaisesRegex(ValueError, "retained source state differs from public native contract"):
            family.state_pose("working", "N", 0, self.record, ASSET)

    def test_resigned_movement_still_cannot_hide_a_real_phase(self):
        """Actual native travel keeps both authored phases rather than a false still alias.

        @param self Missing animated native source-phase fixture.
        """
        self.definition["state_policy"]["moving"].update(animated=False, phase_alias=[0, 0])
        self.definition["approved_state_policy_sha256"] = family.policy_signature(self.definition["state_policy"])
        with self.assertRaisesRegex(ValueError, "retained source state differs from public native contract"):
            family.state_pose("moving", "N", 1, self.record, ASSET)

    def test_foreign_body_refuses_before_every_source_branch_and_image_io(self):
        """Another approved family's body cannot supply retained16 base/travel/status.

        @param self Foreign valid heavy21 body with its own original approval fixture.
        """
        self.definition["selections"]["free/N"] = self.record["families"]["vehicle.model-21"]["selections"]["free/N"]
        # This preflight must stay metadata-only in all branches; eagerly opening a foreign raw original is already too late and unnecessarily repeats large image IO.
        with patch.object(art, "source_candidate", side_effect=AssertionError("foreign source IO before receiving-body validation")):
            for state in self.definition["states"]:
                with self.assertRaisesRegex(ValueError, "receiving original body belongs to another family"):
                    family.state_pose(state, "N", 0, self.record, ASSET)

    def test_duplicate_receiving_body_key_refuses(self):
        """An ambiguous selected source cannot choose whichever duplicate happens to occur first.

        @param self Duplicate real original key fixture.
        """
        candidate = art.source_candidate(self.definition["selections"]["free/N"], self.record)
        self.record["candidates"].append(copy.deepcopy(candidate))
        with self.assertRaisesRegex(ValueError, "missing or duplicate receiving original body"):
            family.state_pose("service", "N", 0, self.record, ASSET)

    def test_changed_receiving_body_registration_refuses(self):
        """An accepted part cannot shift the frozen body under an old receiving approval.

        @param self Shifted actual source-body world reference fixture.
        """
        candidate = art.source_candidate(self.definition["selections"]["free/N"], self.record)
        candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "receiving original body differs from its approved registration"):
            family.state_pose("moving", "N", 0, self.record, ASSET)

    def test_missing_body_camera_cannot_borrow_another_transform(self):
        """The selected original must actually contain the requested physical camera.

        @param self Missing genuine measured body heading fixture.
        """
        candidate = art.source_candidate(self.definition["selections"]["free/N"], self.record)
        del candidate["native_transforms"]["N"]
        with self.assertRaisesRegex(ValueError, "missing genuine heading"):
            family.state_pose("blocked", "N", 0, self.record, ASSET)

    def test_prior_carrier_and_heavy_source_boundaries_still_resolve(self):
        """The shared receiving-body preflight preserves two existing direct role consumers.

        @param self Bounded existing-role compatibility fixture; no old full matrix replay.
        """
        # Resolve one actual old carrier and one actual heavy body through the updated adapter; both must remain byte-identical to their own unchanged original base.
        for asset, state, direction in (("vehicle.model-20", "empty", "E"), ("vehicle.model-21", "free", "N")):
            pose, _ = family.state_pose(state, direction, 0, self.record, asset)
            definition = self.record["families"][asset]
            original, _ = art.native_view(definition["selections"]["free/" + direction], direction, self.record)
            self.assertEqual(pose.tobytes(), original.tobytes())


if __name__ == "__main__":
    unittest.main()
