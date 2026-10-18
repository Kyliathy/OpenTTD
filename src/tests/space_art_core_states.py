"""Check original Core construction stages, static aliases and unchanged paid geometry.

@file space_art_core_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13E
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group70.3
Source coverage remains separate from packing, runtime selection and game review.
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
import convert_space_module_art as core
import convert_space_module_tiles as tiles
import convert_space_module_states as states

REVIEW = ROOT / "logs/proofs/mlm/native-art/space/core-complete-v1/source-manifest.json"


class CoreStateTests(unittest.TestCase):
    """Protect genuine physical stages and native ownership against plausible source drift."""

    @classmethod
    def setUpClass(cls):
        """Load concrete stored source artifacts once, checking each exact identity.

        @param cls Fixture sharing accepted original/state images without editing them.
        """
        cls.document = art.read_record()
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        for artifact in cls.report["artifacts"]:
            assert art.identity(ROOT / artifact["path"]) == artifact
        atlas = Image.open(REVIEW.parent / "module-nucleus-authored.png")
        cls.parts = {}
        # Read actual stored rectangles rather than trusting reported source
        # counts; later assertions independently reconstruct all four models.
        for row in cls.report["sprites"]:
            x, y, width, height = row["box"]
            key = (row["state"], row["frame"], row["tile_variant"], row["part"])
            assert key not in cls.parts
            cls.parts[key] = (row, atlas.crop((x, y, x + width, y + height)))
        cls.whole = {state: Image.open(REVIEW.parent / (state + ".png")) for state in states.STATES}

    def setUp(self):
        """Detach metadata for each deliberate source refusal.

        @param self Fixture whose mutations cannot alter accepted catalogue files.
        """
        self.source = copy.deepcopy(self.document)
        self.family = self.source["families"][core.ASSET]

    def candidate(self, key):
        """Find the exact detached original targeted by a refusal case.

        @param self Current detached Core source fixture.
        @param key Explicit original source identity.
        @return Editable detached candidate metadata only.
        """
        return next(row for row in self.source["candidates"] if row["key"] == key)

    def test_complete144_logical_cells_have_actual_native_geometry(self):
        """Coverage contains each required real world cell/part/state/frame exactly once.

        @param self Published native matrix fixture.
        """
        expected = {(state, frame, tile, part) for state in states.STATES for frame in range(2)
                    for tile in range(9) for part in ("back", "front")}
        self.assertEqual(set(self.parts), expected)
        self.assertEqual(self.report["logical_pose_count"], 144)
        for key, (row, image) in self.parts.items():
            self.assertEqual(row["direction"], "none")
            self.assertEqual(row["world_cell"], [row["tile_variant"] % 3, row["tile_variant"] // 3])
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(image.size, (64, 64))
            self.assertEqual(hashlib.sha256(image.tobytes()).hexdigest(), row["native_sha256"])
            self.assertEqual(row["structurally_absent"], not any(image.get_flattened_data()))

    def test_independent_original_reconstruction_has_no_seams_or_duplicates(self):
        """All stored native stages independently reconstruct their exact whole pixels.

        @param self All144actual stored source rectangles.
        """
        # Origins come from actual world coordinates, independently of stored
        # native origins and the converter's own reconstruction implementation.
        for state in states.STATES:
            for frame in range(2):
                image = Image.new("P", core.CANVAS, 0)
                seen = set()
                for tile in range(9):
                    wx, wy = tile % 3, tile // 3
                    ox, oy = 97 + 32 * (wy - wx), 31 + 16 * (wx + wy)
                    for part in ("back", "front"):
                        row, source = self.parts[state, frame, tile, part]
                        for y in range(64):
                            for x in range(64):
                                value = source.getpixel((x, y))
                                if not value:
                                    continue
                                point = (ox + x, oy + y)
                                self.assertNotIn(point, seen)
                                seen.add(point)
                                image.putpixel(point, value)
                self.assertEqual(image.tobytes(), self.whole[state].tobytes())

    def test_all72_second_frames_are_explicit_exact_static_aliases(self):
        """No blueprint, stopped construction or completed Core gains an art-local clock.

        @param self Declared two-frame physical stage fixture.
        """
        aliases = 0
        for (state, frame, tile, part), (row, image) in self.parts.items():
            if frame == 0:
                self.assertIsNone(row["alias_of"])
                continue
            self.assertEqual(row["alias_of"], dict(state=state, frame=0, direction="none", tile_variant=tile, part=part))
            self.assertEqual(image.tobytes(), self.parts[state, 0, tile, part][1].tobytes())
            aliases += 1
        self.assertEqual(aliases, 72)

    def test_complete_original_and_all_previously_accepted_cuts_remain_exact(self):
        """Construction authoring cannot replace the accepted completed body or coupling.

        @param self Current source matrix and immutable70.1/70.2 artifacts.
        """
        previous = ROOT / "logs/proofs/mlm/native-art/space/core-tile-cuts-v1"
        old = json.loads((previous / "tile-source-review.json").read_text(encoding="utf-8"))
        atlas = Image.open(previous / "native-cuts.png")
        self.assertEqual(self.whole["complete"].tobytes(), Image.open(previous / "reconstructed.png").tobytes())
        for row in old["sprites"]:
            x, y, width, height = row["box"]
            stored = atlas.crop((x, y, x + width, y + height))
            self.assertEqual(stored.tobytes(), self.parts["complete", 0, row["tile_variant"], row["part"]][1].tobytes())

    def test_blueprint_is_transparent_planning_and_partial_is_distinct_physical_work(self):
        """The genuine unbuilt plan cannot silently substitute the finished physical station.

        @param self Original state-body and physical source-cue fixture.
        """
        counts = {state: sum(value != 0 for value in image.get_flattened_data()) for state, image in self.whole.items()}
        self.assertLess(counts["blueprint"], counts["complete"] * 0.8)
        self.assertNotEqual(self.whole["partial"].tobytes(), self.whole["complete"].tobytes())
        self.assertNotEqual(self.whole["partial"].tobytes(), self.whole["blueprint"].tobytes())
        for state in ("blueprint", "partial"):
            self.assertEqual(self.report["sources"][state]["candidate"], self.family["construction_sources"][state])
            for tile in range(9):
                self.assertFalse(self.parts[state, 0, tile, "back"][0]["structurally_absent"])
                self.assertTrue(self.parts[state, 0, tile, "front"][0]["structurally_absent"])

    def test_blocked_retains_complete_body_coupling_and_silhouette_outside_window(self):
        """An unavailable completed Core keeps its paid hardware and one actual berth geometry.

        @param self Actual stored complete/blocked original-pixel fixture.
        """
        complete, blocked = self.whole["complete"], self.whole["blocked"]
        changed, amber = 0, 0
        x0, y0, x1, y1 = states.STATUS_BOUNDS
        # Inspect actual images; a source-policy promise alone cannot conceal
        # erased PV banks, a changed mouth or a warning expanded into the floor.
        for y in range(core.CANVAS[1]):
            for x in range(core.CANVAS[0]):
                a, b = complete.getpixel((x, y)), blocked.getpixel((x, y))
                self.assertEqual(a == 0, b == 0)
                if a != b:
                    self.assertTrue(x0 <= x < x1 and y0 <= y < y1)
                    changed += 1
                    red, green, blue = blocked.getpalette()[b * 3:b * 3 + 3]
                    amber += red > 160 and green > 70 and red > green * 1.1 and blue < 90
        self.assertGreater(changed, 0)
        self.assertGreater(amber, 0)
        self.assertEqual(changed, self.report["sources"]["blocked"]["changed_pixels"])
        for tile in range(9):
            self.assertEqual(self.parts["complete", 0, tile, "front"][1].tobytes(),
                             self.parts["blocked", 0, tile, "front"][1].tobytes())

    def test_fresh_original_conversion_matches_all144_stored_parts(self):
        """Full independent source conversion retains original ancestry and published pixels.

        @param self Direct producer/consumer matrix compatibility fixture.
        """
        fresh, whole, evidence = states.matrix(self.source)
        self.assertEqual(len(fresh), 144)
        for row, image in fresh:
            key = (row["state"], row["frame"], row["tile_variant"], row["part"])
            self.assertEqual(image.tobytes(), self.parts[key][1].tobytes())

    def test_unsigned_state_policy_change_refuses_before_original_access(self):
        """Changed state descriptions cannot inherit prior source approval.

        @param self Unsigned detached construction policy fixture.
        """
        self.family["state_policy"]["frame_policy"] = "animate from GUI wall time"
        with patch.object(art, "source_candidate", side_effect=AssertionError("unexpected image access")):
            with self.assertRaisesRegex(ValueError, "approved Core state policy changed"):
                states.native_states(self.source)

    def test_new_signature_cannot_invent_work_motion_or_material_completion(self):
        """Native state meanings outrank freshly signed authoring metadata.

        @param self Re-signed false physical/phase ownership fixture.
        """
        self.family["state_policy"]["state_owners"]["partial"] = "Complete as soon as inputs arrive"
        self.family["approved_state_policy_sha256"] = tiles.signature(self.family["state_policy"])
        with patch.object(art, "source_candidate", side_effect=AssertionError("unexpected image access")):
            with self.assertRaisesRegex(ValueError, "differs from native construction"):
                states.native_states(self.source)

    def test_foreign_partial_source_cannot_borrow_core_registration(self):
        """A valid raw identity does not authorize a different family as Core construction.

        @param self Deliberately foreign source-role metadata fixture.
        """
        candidate = self.candidate(self.family["construction_sources"]["partial"])
        candidate["asset_id"] = "module.dock"
        self.family["approved_construction_registration"]["partial"] = core.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "Core construction original or stage registration changed"):
            states.construction_body("partial", self.source)

    def test_signed_construction_floor_shift_still_refuses_native_geometry(self):
        """A new stage approval cannot move the fixed public/world footprint.

        @param self Re-signed original transform drift fixture.
        """
        candidate = self.candidate(self.family["construction_sources"]["blueprint"])
        candidate["native_transform"]["native_reference"][0] += 1
        self.family["approved_construction_registration"]["blueprint"] = core.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "floor/approach contract changed"):
            states.construction_body("blueprint", self.source)

    def test_signed_status_contact_cannot_leave_existing_hub_window(self):
        """Status approval cannot create a second indicator elsewhere on the paid body.

        @param self Re-signed native status contact drift fixture.
        """
        candidate = self.candidate(self.family["blocked_source"])
        candidate["native_transform"]["native_contact"][0] += 1
        self.family["approved_blocked_registration"] = core.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "leaves its native window"):
            states.status_component(self.source)

    def test_missing_actual_state_cannot_count_as_full_source_coverage(self):
        """Removing a physical construction original cannot be hidden with a new policy hash.

        @param self Incomplete source-selection fixture.
        """
        del self.family["construction_sources"]["partial"]
        self.family["state_policy"] = states.expected_policy(self.source)
        self.family["approved_state_policy_sha256"] = tiles.signature(self.family["state_policy"])
        with self.assertRaisesRegex(ValueError, "differs from native construction"):
            states.checked_policy(self.source)

    def test_changed_original_status_bytes_still_refuse_immutable_ancestry(self):
        """A cached source report cannot authorize replacement glass pixels.

        @param self Mocked original identity failure with unchanged real files.
        """
        target = (ROOT / self.candidate(self.family["blocked_source"])["path"]).resolve()
        original_identity = art.identity

        def changed_identity(path):
            """Simulate changed original bytes without editing the retained image.

            @param path Actual requested source/receipt/reference file.
            @return Real identity except the deliberate selected-original hash defect.
            """
            result = original_identity(path)
            if path.resolve() == target:
                result["sha256"] = "0" * 64
            return result

        with patch.object(art, "identity", side_effect=changed_identity):
            with self.assertRaisesRegex(ValueError, "immutable Space source ancestry changed"):
                states.status_component(self.source)

    def test_full_source_does_not_claim_packed_or_runtime_activation(self):
        """Original state coverage does not bypass the later native packing/game review owners.

        @param self Explicit source-only public handoff fixture.
        """
        self.assertTrue(self.report["coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(self.family["native_approach_slots"], 1)


if __name__ == "__main__":
    unittest.main()
