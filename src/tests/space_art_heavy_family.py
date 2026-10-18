"""Validate the complete heavy-miner source handoff without native activation.

@file space_art_heavy_family.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65.3e
The actual128-pose atlas preserves original geometry, source ancestry and static
aliases. Native selectors, packed sprites and gameplay acceptance stay separate.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_family_art as family
from tools.orbital_art_contract import heavy_miner_contract

ASSET = "vehicle.model-21"
REVIEW = ROOT / "logs/proofs/mlm/native-art/space/heavy-miner-complete-v1/space-source-manifest.json"


class HeavyMinerFamilyTests(unittest.TestCase):
    """Protect the actual heavy role's complete source matrix and refusal boundaries."""

    @classmethod
    def setUpClass(cls):
        """Load the published atlas and independently resolve both endpoint cameras.

        @param cls Source fixture sharing immutable native pixels only.
        """
        cls.source = art.read_record()
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        cls.entry = cls.report["atlases"][0]
        cls.atlas = Image.open(ROOT / cls.entry["path"])
        cls.sprites = {(row["state"], row["frame"], row["direction"]): row for row in cls.entry["sprites"]}
        cls.fresh = {}
        definitions = cls.source["families"][ASSET]
        # Resolve actual production source branches independently for the first and opposite final camera. Cache only genuinely shared source banks, keeping explicit static aliases rather than counting repeated imagery as new art.
        for direction in ("N", "NW"):
            for state in definitions["states"]:
                for frame in range(2):
                    policy = definitions["state_policy"][state]
                    key = policy["source"], policy["phase_alias"][frame], direction
                    if key not in cls.fresh:
                        cls.fresh[key] = family.state_pose(state, direction, frame, cls.source, ASSET)[0]

    def setUp(self):
        """Detach policy metadata for independent source refusal inputs.

        @param self Original source/phase policy mutation fixture.
        """
        self.record = copy.deepcopy(self.source)
        self.definition = self.record["families"][ASSET]

    def pose(self, state, frame, direction):
        """Read one actual native cell from the published atlas.

        @param self Published source matrix fixture.
        @param state Exact public source state.
        @param frame Logical frame0/1.
        @param direction Genuine source camera heading.
        @return Indexed64x48 source cell at the original public anchor.
        """
        row = self.sprites[state, frame, direction]
        x, y, width, height = row["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def test_complete_native_matrix_matches_actual_public_contract(self):
        """All128 genuine state/frame/view entries retain exact64x48 geometry and anchors.

        @param self Actual published sprite contract fixture.
        """
        contract = heavy_miner_contract()
        # Enumerate the native public state/frame/camera cross-product so reserved or missing cells cannot silently disappear from the coverage count.
        expected = {(state, frame, direction) for state in contract["states"] for frame in range(2) for direction in contract["directions"]}
        self.assertEqual(set(self.sprites), expected)
        self.assertEqual(self.report["pose_count"], 128)
        self.assertEqual(self.entry["missing_pose_count"], 0)
        self.assertEqual(self.atlas.size, (512, 768))
        self.assertEqual(hashlib.sha256((ROOT / self.entry["path"]).read_bytes()).hexdigest(), self.entry["sha256"])
        # Every actual cell preserves native geometry and contains original model pixels; an empty reserved slot cannot masquerade as a covered logical pose.
        for key, row in self.sprites.items():
            self.assertEqual(row["box"][2:], [64, 48])
            self.assertEqual(row["anchor"], [-32, -32])
            self.assertTrue(any(self.pose(*key).get_flattened_data()))

    def test_static_aliases_and_real_animations_are_explicit(self):
        """Native stopped/full/blocked poses stay static while real moving/work cues differ.

        @param self Public actual phase contract fixture.
        """
        expected = heavy_miner_contract()["source_frame_contract"]
        self.assertEqual(self.report["explicit_static_alias_count"], 32)
        # Check every genuine view, including tiny side-on indicators; metadata alone cannot turn duplicated pixels into actual source animation.
        for state, phases in expected.items():
            for direction in self.definition["directions"]:
                first = self.pose(state, 0, direction).tobytes()
                second = self.pose(state, 1, direction).tobytes()
                self.assertEqual(first == second, phases == [0, 0])

    def test_stopped_originals_have_no_travel_or_fabricated_attachment(self):
        """Free and stopped attached source poses retain the unchanged original body.

        @param self Exact original idle/attachment source fixture.
        """
        # The attachment lease remains native state. Its stopped source is the original cone, without a painted attachment transform, new gripping pose or frozen travel plume.
        for direction in self.definition["directions"]:
            original, _ = art.native_view(self.definition["selections"]["free/" + direction], direction, self.record)
            for state in ("free", "attached"):
                for frame in range(2):
                    self.assertEqual(self.pose(state, frame, direction).tobytes(), original.tobytes())

    def test_return_reuses_actual_approach_gas_bank(self):
        """Return source reuse stays explicit instead of claiming another authored effect.

        @param self Genuine shared original travel phase fixture.
        """
        # Both logical travel states use the same physically registered original pulses; neither state creates another motion opportunity or a different heavy nozzle.
        for direction in self.definition["directions"]:
            for frame in range(2):
                self.assertEqual(self.pose("return", frame, direction).tobytes(), self.pose("rendezvous", frame, direction).tobytes())

    def test_published_pixels_match_fresh_actual_source_consumers(self):
        """The actual endpoint-camera atlas cells match the production source getters.

        @param self Independent freshly converted pixel fixture.
        """
        # Exercise each actual source branch through both endpoint headings, including true static aliases and independently authored animation phases.
        for direction in ("N", "NW"):
            for state in self.definition["states"]:
                for frame in range(2):
                    policy = self.definition["state_policy"][state]
                    key = policy["source"], policy["phase_alias"][frame], direction
                    self.assertEqual(self.pose(state, frame, direction).tobytes(), self.fresh[key].tobytes())

    def test_reused_component_records_do_not_inflate_original_art_counts(self):
        """The128-pose source truthfully counts9new original images and8gas registrations.

        @param self Actual immutable source ancestry fixture.
        """
        self.assertEqual(self.report["actual_component_pose_count"], 72)
        self.assertEqual(self.report["new_original_source_count"], 9)
        self.assertEqual(self.report["registered_source_instance_count"], 8)
        parents = {(row["path"], row["sha256"]) for row in self.report["shared_original_sources"]}
        self.assertEqual(len(parents), 4)
        self.assertTrue(self.report["coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])

    def test_changed_approved_state_policy_refuses(self):
        """A changed source selector cannot silently retain the reviewed policy identity.

        @param self Corrupted complete source policy fixture.
        """
        self.definition["state_policy"]["mining"]["source"] = "base"
        with self.assertRaisesRegex(ValueError, "approved native source state policy changed"):
            family.state_pose("mining", "N", 0, self.record, ASSET)

    def test_reapproved_static_mining_still_violates_native_public_contract(self):
        """A new local signature cannot suppress the native heavy miner's two work phases.

        @param self Incorrect freshly approved static work policy fixture.
        """
        self.definition["state_policy"]["mining"].update(animated=False, phase_alias=[0, 0])
        self.definition["approved_state_policy_sha256"] = family.policy_signature(self.definition["state_policy"])
        with self.assertRaisesRegex(ValueError, "heavy source phase policy differs from public native contract"):
            family.state_pose("mining", "N", 1, self.record, ASSET)

    def test_static_state_cannot_claim_an_independent_second_phase(self):
        """Idle has one actual original even when malformed authoring metadata is signed.

        @param self Forged static source phase fixture.
        """
        self.definition["state_policy"]["free"]["phase_alias"] = [0, 1]
        self.definition["approved_state_policy_sha256"] = family.policy_signature(self.definition["state_policy"])
        with self.assertRaisesRegex(ValueError, "actual animation/static ownership"):
            family.state_pose("free", "N", 1, self.record, ASSET)

    def test_carrier_cannot_borrow_heavy_cutter(self):
        """Model20 construction cannot acquire heavy extraction art by state relabeling.

        @param self Cross-role original work source fixture.
        """
        carrier = self.record["families"]["vehicle.model-20"]
        carrier["state_policy"]["constructing"]["source"] = "heavy-cutter"
        carrier["approved_state_policy_sha256"] = family.policy_signature(carrier["state_policy"])
        with self.assertRaisesRegex(ValueError, "missing actual heavy cutter approval"):
            family.state_pose("constructing", "N", 0, self.record, "vehicle.model-20")

    def test_heavy_hopper_cannot_borrow_carrier_material_cue(self):
        """A static full hold still requires the heavy role's actual original ore source.

        @param self Incorrect original load role fixture.
        """
        self.definition["state_policy"]["full"]["source"] = "steel"
        self.definition["approved_state_policy_sha256"] = family.policy_signature(self.definition["state_policy"])
        with self.assertRaisesRegex(ValueError, "missing actual original load approval"):
            family.state_pose("full", "N", 0, self.record, ASSET)

    def test_missing_actual_heavy_source_approval_refuses(self):
        """Full source coverage cannot fall back to an unapproved travel registration.

        @param self Missing actual component review fixture.
        """
        del self.definition["approved_travel_registration"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "missing actual heavy travel approval"):
            family.state_pose("rendezvous", "N", 0, self.record, ASSET)


if __name__ == "__main__":
    unittest.main()
