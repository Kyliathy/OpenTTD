"""Verify the complete hauler source matrix and conversion-local component reuse.

@file space_art_hauler_family.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group66D
Actual packets retain quantity in every movement/blocking branch. Unsupported
service aliases empty; source approval grants no native work or runtime sprites.
"""
import copy
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
import convert_space_hauler_parts as hauler
from tools.orbital_art_contract import hauler_contract

ASSET = hauler.ASSET
REVIEW = ROOT / "logs/proofs/mlm/native-art/space/hauler-complete-v1/space-source-manifest.json"


class HaulerFamilyTests(unittest.TestCase):
    """Protect every logical pose, immutable component ancestry and native aliases."""

    @classmethod
    def setUpClass(cls):
        """Read actual accepted atlases and resolve one fresh source-bank camera.

        @param cls Fixture sharing verified original inputs, never gameplay state.
        """
        cls.source = art.read_record()
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        cls.entry = cls.report["atlases"][0]
        assert art.identity(ROOT / cls.entry["path"])["sha256"] == cls.entry["sha256"]
        cls.atlas = Image.open(ROOT / cls.entry["path"])
        cls.sprites = {(row["state"], row["frame"], row["direction"]): row for row in cls.entry["sprites"]}
        definition = cls.source["families"][ASSET]
        cls.producers = {}
        # Compare against the already reviewed direct producers, not a second
        # copy of the family adapter's source-kind dispatch logic.
        for field in ("load_source_review", "travel_source_review", "blocked_source_review"):
            reference = definition[field]
            assert art.identity(ROOT / reference["path"]) == reference
            report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
            for artifact in report["artifacts"]:
                assert art.identity(ROOT / artifact["path"]) == artifact
            cls.producers[field] = Image.open((ROOT / reference["path"]).parent / "native.png")
        cls.bank = hauler.HaulerSourceBank(cls.source, ["N"])

    def setUp(self):
        """Detach metadata before each deliberate policy or ancestry refusal.

        @param self Independent hauler-family test fixture.
        """
        self.document = copy.deepcopy(self.source)
        self.definition = self.document["families"][ASSET]

    def pose(self, state, frame, direction):
        """Read one logical indexed cell from the published native atlas.

        @param self Complete source-matrix fixture.
        @param state Public hauler state, including the unsupported service alias.
        @param frame Logical frame0/1.
        @param direction Genuine viewport camera heading.
        @return Exact64x48 native pixels without any bitmap transformation.
        """
        x, y, width, height = self.sprites[state, frame, direction]["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def resign_policy(self):
        """Sign intentionally changed test metadata to exercise native invariants.

        @param self Detached policy fixture; no source file is rewritten.
        """
        self.definition["approved_state_policy_sha256"] = family.policy_signature(self.definition["state_policy"])

    def test_complete160_pose_matrix_preserves_geometry(self):
        """Every original heading and logical state/frame has a nonempty fixed cell.

        @param self Complete native geometry and source-identity fixture.
        """
        expected = {(state, frame, direction) for state in hauler_contract()["states"]
                    for frame in range(2) for direction in family.DIRECTIONS}
        self.assertEqual(set(self.sprites), expected)
        self.assertEqual(len(self.entry["sprites"]), 160)
        self.assertEqual(self.report["pose_count"], 160)
        self.assertEqual(self.entry["missing_pose_count"], 0)
        self.assertEqual(self.atlas.size, (512, 960))
        # Empty or shifted cells cannot count as delivered source coverage.
        for key, row in self.sprites.items():
            self.assertEqual(row["box"][2:], [64, 48])
            self.assertEqual(row["anchor"], [-32, -32])
            self.assertTrue(any(self.pose(*key).get_flattened_data()))

    def test_all_phases_follow_native_motion_and_still_ownership(self):
        """Only actual moving quantities animate; seven still states remain exact.

        @param self Complete native phase and explicit still-alias fixture.
        """
        self.assertEqual(self.report["actual_component_pose_count"], 96)
        self.assertEqual(self.report["explicit_static_alias_count"], 56)
        # Inspect actual bytes for all80state/camera pairs: declared animation
        # flags cannot hide an absent pulse or animate a stopped loaded hull.
        for state, phases in hauler_contract()["source_frame_contract"].items():
            self.assertEqual(self.definition["state_policy"][state]["phase_alias"], phases)
            for direction in family.DIRECTIONS:
                equal = self.pose(state, 0, direction).tobytes() == self.pose(state, 1, direction).tobytes()
                self.assertEqual(equal, phases == [0, 0])

    def test_service_is_explicit_empty_alias_in_every_camera(self):
        """No source phase invents the service producer excluded by native model17.

        @param self Unsupported-service source and evidence fixture.
        """
        self.assertEqual(self.report["state_policy"]["service"]["source_state_alias"], "empty")
        # Both service frames reuse the actual empty hull, including the dark
        # original display and the complete absence of travel gas.
        for direction in family.DIRECTIONS:
            for frame in range(2):
                self.assertEqual(self.pose("service", frame, direction).tobytes(), self.pose("empty", 0, direction).tobytes())
        aliases = [row for row in self.report["poses"] if row["source_state_alias"] == "empty"]
        self.assertEqual(len(aliases), 16)
        self.assertTrue(all(row["state"] == "service" for row in aliases))

    def test_all_quantities_match_the_accepted_direct_producers(self):
        """Assembly keeps exact cargo, one-nozzle motion and quantity-preserving blocking.

        @param self Independently accepted component-atlas comparison fixture.
        """
        # Each original direct producer has a fixed quantity-row layout. Compare
        # every corresponding logical cell, including static frame1 aliases.
        for column, direction in enumerate(family.DIRECTIONS):
            for quantity_index, quantity in enumerate(("empty", "partial", "full")):
                for frame in range(2):
                    load = self.producers["load_source_review"]
                    y = quantity_index * 48
                    expected = load.crop((column * 64, y, column * 64 + 64, y + 48))
                    self.assertEqual(self.pose(quantity, frame, direction).tobytes(), expected.tobytes())
                    moving = self.producers["travel_source_review"]
                    y = (quantity_index * 2 + frame) * 48
                    expected = moving.crop((column * 64, y, column * 64 + 64, y + 48))
                    self.assertEqual(self.pose("moving-" + quantity, frame, direction).tobytes(), expected.tobytes())
                    blocked = self.producers["blocked_source_review"]
                    y = quantity_index * 48
                    expected = blocked.crop((column * 64, y, column * 64 + 64, y + 48))
                    state = "blocked" if quantity == "empty" else "blocked-" + quantity
                    self.assertEqual(self.pose(state, frame, direction).tobytes(), expected.tobytes())

    def test_source_counts_and_activation_boundary_are_truthful(self):
        """Actual generated originals and registered donor gas remain distinct.

        @param self Full source ancestry and deferred-runtime fixture.
        """
        self.assertEqual(self.report["new_original_source_count"], 12)
        self.assertEqual(self.report["registered_source_instance_count"], 4)
        self.assertEqual(len(self.report["shared_original_sources"]), 4)
        self.assertTrue(self.report["coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertTrue(all(row["asset_id"] == ASSET for row in self.report["source_candidates"]))

    def test_fresh_bank_and_direct_apis_match_the_full_matrix(self):
        """Local reuse and uncached endpoint branches reproduce accepted native bytes.

        @param self Fresh N-bank and direct NW branch compatibility fixture.
        """
        self.assertEqual(self.bank.component_pose_count, 12)
        # All logical aliases in one fresh bank must reproduce the full output;
        # four direct endpoint branches additionally exercise uncached callers.
        for state in hauler_contract()["states"]:
            for frame in range(2):
                pose, _ = family.state_pose(state, "N", frame, self.document, ASSET, self.bank)
                self.assertEqual(pose.tobytes(), self.pose(state, frame, "N").tobytes())
        for state, frame in (("empty", 0), ("full", 1), ("moving-partial", 1), ("blocked-full", 1)):
            pose, _ = family.state_pose(state, "NW", frame, self.document, ASSET)
            self.assertEqual(pose.tobytes(), self.pose(state, frame, "NW").tobytes())

    def test_unsigned_policy_change_refuses_before_source_io(self):
        """A changed state owner cannot inherit the previous source-policy approval.

        @param self Unsigned native ownership corruption fixture.
        """
        self.definition["state_policy"]["empty"]["owner"] = "invented cargo owner"
        with patch.object(art, "source_candidate", side_effect=AssertionError("unexpected source IO")):
            with self.assertRaisesRegex(ValueError, "approved native source state policy changed"):
                family.state_pose("empty", "N", 0, self.document, ASSET, self.bank)

    def test_new_signature_cannot_grant_service_or_erase_quantity(self):
        """Native invariants outrank a newly signed but incorrect authoring policy.

        @param self Signed service/quantity/stopped-motion corruption fixture.
        """
        cases = (("service", dict(animated=True, phase_alias=[0, 1])),
                 ("service", dict(source_state_alias=None)),
                 ("blocked-full", dict(source="base")),
                 ("partial", dict(source="hauler-full")),
                 ("full", dict(source="hauler-moving-full", animated=True, phase_alias=[0, 1])),
                 ("moving-empty", dict(animated=False, phase_alias=[0, 0])))
        # Source signatures approve artwork, never gameplay semantics. Reject
        # every substituted branch before decoding any original image.
        for state, change in cases:
            with self.subTest(state=state, change=change):
                self.document = copy.deepcopy(self.source)
                self.definition = self.document["families"][ASSET]
                self.definition["state_policy"][state].update(change)
                self.resign_policy()
                with patch.object(art, "source_candidate", side_effect=AssertionError("unexpected source IO")):
                    with self.assertRaisesRegex(ValueError, "hauler source state differs from public native contract"):
                        family.state_pose(state, "N", 1, self.document, ASSET)

    def test_foreign_body_refuses_before_every_branch(self):
        """Another approved hull cannot supply hauler cargo, movement or status.

        @param self Foreign actual retained16-body metadata fixture.
        """
        self.definition["selections"]["free/N"] = self.document["families"]["vehicle.model-16"]["selections"]["free/N"]
        with patch.object(art, "source_candidate", side_effect=AssertionError("unexpected foreign source IO")):
            # All ten logical states share the same receiving-body boundary;
            # a valid part approval cannot excuse a foreign physical receiver.
            for state in hauler_contract()["states"]:
                with self.assertRaisesRegex(ValueError, "receiving original body belongs to another family"):
                    family.state_pose(state, "N", 0, self.document, ASSET)

    def test_ambiguous_missing_or_changed_receiving_camera_refuses(self):
        """Duplicate source keys and altered camera registration cannot reuse approvals.

        @param self Original receiving-camera corruption fixture.
        """
        # Each corruption starts from the actual approved source rather than
        # allowing an earlier error to mask the specific receiving-body defect.
        for change in ("duplicate", "missing-camera", "shift"):
            with self.subTest(change=change):
                self.document = copy.deepcopy(self.source)
                definition = self.document["families"][ASSET]
                candidate = art.source_candidate(definition["selections"]["free/N"], self.document)
                if change == "duplicate":
                    self.document["candidates"].append(copy.deepcopy(candidate))
                    message = "missing or duplicate receiving original body"
                elif change == "missing-camera":
                    del candidate["native_transforms"]["N"]
                    message = "missing genuine heading"
                else:
                    candidate["native_transforms"]["N"]["source_reference"][0] += 1
                    message = "receiving original body differs from its approved registration"
                with patch.object(art, "source_candidate", side_effect=AssertionError("unexpected source IO")):
                    with self.assertRaisesRegex(ValueError, message):
                        family.state_pose("blocked-full", "N", 0, self.document, ASSET)

    def test_cache_cannot_hide_changed_approval_or_source_metadata(self):
        """A bank belongs to exactly one approved catalogue, even after re-signing.

        @param self Cached-source metadata invalidation fixture.
        """
        self.definition["state_policy"]["full"]["owner"] += " changed"
        self.resign_policy()
        with self.assertRaisesRegex(ValueError, "hauler source bank metadata changed"):
            family.state_pose("full", "N", 0, self.document, ASSET, self.bank)

    def test_cache_rechecks_original_and_receipt_bytes(self):
        """Reusing decoded images cannot conceal changed cargo, body, gas or display bytes.

        @param self Cached-source immutable ancestry fixture; real files stay untouched.
        """
        definition = self.source["families"][ASSET]
        selections = (("base", definition["selections"]["free/N"]),
                      ("hauler-full", definition["load_sources"]["full"]["N"]),
                      ("hauler-moving-empty", definition["travel_sources"]["0"]["N"]),
                      ("hauler-blocked-empty", definition["status_sources"]["blocked"]["0"]["N"]))
        identity = art.identity
        # Corrupt only the observed hash, never an immutable raw file. Both
        # raw pixels and prompt receipts must be checked again at cache lookup.
        for kind, key in selections:
            candidate = art.source_candidate(key, self.source)
            for reference in (candidate, candidate["generation_receipt"]):
                target = (ROOT / reference["path"]).resolve()

                def changed_identity(path):
                    """Model replacement bytes without modifying retained originals.

                    @param path Exact source/receipt path requested by ancestry validation.
                    @return Actual identity except for the one deliberately changed hash.
                    """
                    result = identity(path)
                    if path.resolve() == target:
                        result["sha256"] = "0" * 64
                    return result

                with self.subTest(kind=kind, target=reference["path"]):
                    with patch.object(art, "identity", side_effect=changed_identity):
                        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry changed"):
                            self.bank.pose(kind, 0, "N", self.source)

    def test_cache_copies_cannot_mutate_later_poses(self):
        """A caller's editable image/evidence copies cannot poison subsequent logical aliases.

        @param self Conversion-local image and nested evidence isolation fixture.
        """
        pose, evidence = self.bank.pose("hauler-full", 0, "N", self.source)
        before = pose.tobytes()
        pose.putpixel((32, 20), 0 if pose.getpixel((32, 20)) else 1)
        evidence["base"]["original"]["sha256"] = "changed"
        fresh, proof = self.bank.pose("hauler-full", 0, "N", self.source)
        self.assertEqual(fresh.tobytes(), before)
        self.assertNotEqual(proof["base"]["original"]["sha256"], "changed")

    def test_missing_cached_camera_cannot_alias_an_existing_view(self):
        """An N-only preview bank refuses to claim an unassembled NW camera.

        @param self Deliberately bounded source-bank coverage fixture.
        """
        with self.assertRaisesRegex(ValueError, "missing verified hauler source-bank pose"):
            family.state_pose("empty", "NW", 0, self.document, ASSET, self.bank)

    def test_direct_previous_role_boundaries_remain_exact(self):
        """Shared metadata preflight preserves one actual retained and heavy source consumer.

        @param self Two direct original-role compatibility samples; no full prior regression.
        """
        # Resolve only the direct changed adapter boundary for two prior roles;
        # previously accepted component/source matrices remain their evidence.
        for asset, state in (("vehicle.model-16", "working"), ("vehicle.model-21", "free")):
            pose, _ = family.state_pose(state, "N", 0, self.document, asset)
            definition = self.document["families"][asset]
            original, _ = art.native_view(definition["selections"]["free/N"], "N", self.document)
            self.assertEqual(pose.tobytes(), original.tobytes())


if __name__ == "__main__":
    unittest.main()
