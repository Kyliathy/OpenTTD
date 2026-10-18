"""Verify genuine Dock construction stages, static frames and two retained physical clamps.

@file space_art_dock_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13F
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group71.3
Source appearance never supplies native work, stock, utility or service admission.
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
import convert_space_dock_art as dock
import convert_space_dock_states as states
from tools import orbital_tile_art as cuts

REVIEW = ROOT / "logs/proofs/mlm/native-art/space/dock-complete-v1/source-manifest.json"


class DockStateTests(unittest.TestCase):
    """Protect original physical states without inventing a second two-berth controller."""

    @classmethod
    def setUpClass(cls):
        """Read concrete stored native source rectangles and exact whole-stage images.

        @param cls Fixture sharing immutable review/source pixels.
        """
        cls.source = art.read_record()
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        for artifact in cls.report["artifacts"]:
            assert art.identity(ROOT / artifact["path"]) == artifact
        atlas = Image.open(REVIEW.parent / "module-dock-authored.png")
        cls.parts = {}
        # Every stored rectangle must have one real physical pose; dictionary
        # overwrite cannot hide duplicate source rows or missing matrix entries.
        for row in cls.report["sprites"]:
            x, y, width, height = row["box"]
            key = (row["state"], row["frame"], row["tile_variant"], row["part"])
            assert key not in cls.parts
            cls.parts[key] = (row, atlas.crop((x, y, x + width, y + height)))
        cls.whole = {state: Image.open(REVIEW.parent / (state + ".png")) for state in states.STATES}

    def setUp(self):
        """Detach metadata before each deliberate source/ownership corruption.

        @param self Fixture whose mutations cannot alter actual source files.
        """
        self.document = copy.deepcopy(self.source)
        self.family = self.document["families"][dock.ASSET]

    def candidate(self, key):
        """Find the exact editable original targeted by a focused refusal case.

        @param self Current detached source fixture.
        @param key Explicit selected or shared original identity.
        @return Detached candidate metadata only.
        """
        return next(row for row in self.document["candidates"] if row["key"] == key)

    def test_all64_cells_retain_actual_world_geometry_and_complete_pose_coverage(self):
        """Every required real world-cell/part/state/frame exists once at its original anchor.

        @param self Actual stored full native source matrix.
        """
        expected = {(state, frame, tile, part) for state in states.STATES for frame in range(2)
                    for tile in range(4) for part in ("back", "front")}
        self.assertEqual(set(self.parts), expected)
        self.assertEqual(self.report["logical_pose_count"], 64)
        for key, (row, image) in self.parts.items():
            self.assertEqual(row["direction"], "none")
            self.assertEqual(row["world_cell"], [row["tile_variant"] % 2, row["tile_variant"] // 2])
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(image.size, (64, 64))
            self.assertEqual(row["native_sha256"], hashlib.sha256(image.tobytes()).hexdigest())
            self.assertEqual(row["structurally_absent"], not any(image.get_flattened_data()))

    def test_independent_stored_pixel_reconstruction_has_no_seams_or_duplicates(self):
        """Every physical stage/frame independently reconstructs its exact original whole model.

        @param self Complete stored source matrix and whole-stage artifacts.
        """
        # Derive projection from real world coordinates rather than trusting
        # stored native origins or calling the converter's reconstruction helper.
        for state in states.STATES:
            for frame in range(2):
                image = Image.new("P", dock.CANVAS, 0)
                seen = set()
                for tile in range(4):
                    wx, wy = tile % 2, tile // 2
                    ox, oy = 65 + 32 * (wy - wx), 15 + 16 * (wx + wy)
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

    def test_all32_second_frames_are_explicit_exact_static_aliases(self):
        """A fixed clamp gains no work animation from a query, pause or elapsed wall time.

        @param self Declared two-frame physical source fixture.
        """
        aliases = 0
        for (state, frame, tile, part), (row, image) in self.parts.items():
            if frame == 0:
                self.assertIsNone(row["alias_of"])
                continue
            self.assertEqual(row["alias_of"], dict(state=state, frame=0, direction="none", tile_variant=tile, part=part))
            self.assertEqual(image.tobytes(), self.parts[state, 0, tile, part][1].tobytes())
            aliases += 1
        self.assertEqual(aliases, 32)

    def test_accepted_complete_base_and_all_original_tile_cuts_remain_exact(self):
        """New stages cannot replace the accepted Dock, its two clamps or physical depth cuts.

        @param self Current full source and immutable71.1/71.2 artifacts.
        """
        previous = ROOT / "logs/proofs/mlm/native-art/space/dock-tile-cuts-v1"
        old = json.loads((previous / "tile-source-review.json").read_text(encoding="utf-8"))
        atlas = Image.open(previous / "native-cuts.png")
        self.assertEqual(self.whole["complete"].tobytes(), Image.open(previous / "reconstructed.png").tobytes())
        for row in old["sprites"]:
            x, y, width, height = row["box"]
            image = atlas.crop((x, y, x + width, y + height))
            self.assertEqual(image.tobytes(), self.parts["complete", 0, row["tile_variant"], row["part"]][1].tobytes())

    def test_unbuilt_plan_and_missing_partial_front_panel_are_visibly_distinct(self):
        """Real unfinished stages cannot silently alias the ready physical service platform.

        @param self Original planning/physical-construction source fixture.
        """
        counts = {state: sum(value != 0 for value in image.get_flattened_data()) for state, image in self.whole.items()}
        self.assertLess(counts["blueprint"], counts["complete"] * 0.8)
        # A planning outline must retain its actual interior tile junction at
        # native size; a visible outer rim alone conceals the2x2 build layout.
        junction = [(x, y) for y in range(77, 84) for x in range(93, 100)]
        self.assertGreaterEqual(sum(bool(self.whole["blueprint"].getpixel(point)) for point in junction), 12)
        self.assertNotEqual(self.whole["partial"].tobytes(), self.whole["complete"].tobytes())
        # Inspect a real interior region of the front tile: a missing plate must
        # expose genuine transparency around its authored support cross-braces.
        visible = [(x, y) for y in range(90, 102) for x in range(84, 108)
                   if self.whole["complete"].getpixel((x, y))]
        self.assertGreater(sum(not self.whole["partial"].getpixel(point) for point in visible), 30)
        for state in ("blueprint", "partial"):
            for tile in range(4):
                self.assertFalse(self.parts[state, 0, tile, "back"][0]["structurally_absent"])
                self.assertTrue(self.parts[state, 0, tile, "front"][0]["structurally_absent"])

    def test_blocked_retains_both_clamps_and_exact_pixels_outside_existing_cabinet(self):
        """Facility unavailability preserves paid hardware and cannot erase a second ready berth.

        @param self Actual completed/blocked original pixel fixture.
        """
        complete, blocked = self.whole["complete"], self.whole["blocked"]
        changed, amber = 0, 0
        x0, y0, x1, y1 = states.STATUS_BOUNDS
        # Verify actual whole images; source labels cannot excuse changed deck
        # panels, a missing clamp, expanded silhouette or a warning on the floor.
        for y in range(dock.CANVAS[1]):
            for x in range(dock.CANVAS[0]):
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
        for tile in range(4):
            self.assertEqual(self.parts["complete", 0, tile, "front"][1].tobytes(),
                             self.parts["blocked", 0, tile, "front"][1].tobytes())

    def test_shared_amber_material_retains_ancestry_and_unusable_trials_are_rejected(self):
        """Registered material reuse cannot masquerade as another generation or finished work.

        @param self Explicit original/material provenance fixture.
        """
        candidate = self.candidate(self.family["status_source"])
        shared = self.candidate(states.SHARED_GLASS)
        self.assertEqual(candidate["reused_original_key"], shared["key"])
        self.assertEqual(candidate["path"], shared["path"])
        self.assertEqual(candidate["sha256"], shared["sha256"])
        self.assertEqual(self.report["sources"]["blocked"]["original_generation_receipt"], shared["generation_receipt"])
        # Clipped corners, inadequate clearance and unreadable/steep plans
        # cannot re-enter completed coverage through a retained original key.
        for key in ("orbital-dock-partial-v1", "orbital-dock-partial-v2", "orbital-dock-partial-v3",
                    "orbital-dock-partial-v4", "orbital-dock-blueprint-v1", "orbital-dock-blueprint-v2"):
            with self.assertRaisesRegex(ValueError, "rejected Space source"):
                art.source_candidate(key, self.document)

    def test_fresh_source_conversion_matches_every_published_native_cell(self):
        """Full original ancestry and conversion independently reproduce all64stored parts.

        @param self Direct current producer/source-consumer compatibility fixture.
        """
        fresh, images, evidence = states.matrix(self.document)
        self.assertEqual(len(fresh), 64)
        for row, image in fresh:
            key = (row["state"], row["frame"], row["tile_variant"], row["part"])
            self.assertEqual(image.tobytes(), self.parts[key][1].tobytes())

    def test_unsigned_state_policy_change_refuses_before_original_image_access(self):
        """Changed phase control cannot inherit the accepted static source signature.

        @param self Unsigned physical-state policy corruption fixture.
        """
        self.family["state_policy"]["frame_policy"] = "Use GUI wall time"
        with patch.object(dock, "original_image", side_effect=AssertionError("unexpected image access")):
            with self.assertRaisesRegex(ValueError, "approved Dock state policy changed"):
                states.native_states(self.document)

    def test_new_signature_cannot_merge_two_jobs_or_invent_an_art_clock(self):
        """Native independent service receipts outrank freshly signed source-local semantics.

        @param self Re-signed whole-facility and clock corruption fixture.
        """
        # Reset between independent defects so one rejected state description
        # cannot conceal acceptance of a wall-clock frame or merged berth owner.
        for field in ("service_policy", "frame_policy"):
            self.document = copy.deepcopy(self.source)
            self.family = self.document["families"][dock.ASSET]
            self.family["state_policy"][field] = "One paused berth blocks both; animate from wall time"
            self.family["approved_state_policy_sha256"] = cuts.signature(self.family["state_policy"])
            with patch.object(dock, "original_image", side_effect=AssertionError("unexpected image access")):
                with self.assertRaisesRegex(ValueError, "differs from native construction or service owners"):
                    states.native_states(self.document)

    def test_signed_partial_original_cannot_move_the_native_floor(self):
        """A new body registration cannot relocate the saved fixed world footprint.

        @param self Re-signed construction-stage geometry drift fixture.
        """
        candidate = self.candidate(self.family["body_sources"]["partial"])
        candidate["native_transform"]["native_reference"][0] += 1
        self.family["approved_body_registration"]["partial"] = dock.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "whole-model native floor contract changed"):
            dock.native_body("partial", self.document)

    def test_signed_status_registration_cannot_leave_the_original_cabinet(self):
        """Shared glass cannot create another warning beacon or a new service facility.

        @param self Re-signed material mounting contact drift fixture.
        """
        candidate = self.candidate(self.family["status_source"])
        candidate["native_transform"]["native_contact"][0] += 1
        self.family["approved_status_registration"] = dock.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "leaves its existing native cabinet"):
            states.status_component(self.document)

    def test_missing_original_stage_cannot_count_as_full_coverage_with_a_new_hash(self):
        """One unbuilt body cannot silently fill the missing physical-construction state.

        @param self Re-signed incomplete source-stage selection fixture.
        """
        del self.family["body_sources"]["partial"]
        self.family["state_policy"] = states.expected_policy(self.document)
        self.family["approved_state_policy_sha256"] = cuts.signature(self.family["state_policy"])
        with self.assertRaisesRegex(ValueError, "differs from native construction or service owners"):
            states.checked_policy(self.document)

    def test_replaced_shared_original_bytes_are_refused_without_changing_real_files(self):
        """A retained status review never authorizes replacement material pixels.

        @param self Mocked genuine shared-source identity failure fixture.
        """
        target = (ROOT / self.candidate(states.SHARED_GLASS)["path"]).resolve()
        original_identity = art.identity

        def changed_identity(path):
            """Simulate changed original bytes without touching the preserved source.

            @param path Exact requested source/receipt/reference file.
            @return Real identity except the intentional shared-original hash mismatch.
            """
            result = original_identity(path)
            if path.resolve() == target:
                result["sha256"] = "0" * 64
            return result

        with patch.object(art, "identity", side_effect=changed_identity):
            with self.assertRaisesRegex(ValueError, "immutable Space source ancestry changed"):
                states.status_component(self.document)

    def test_full_source_keeps_packing_runtime_and_actual_service_admission_separate(self):
        """Original stage coverage grants no native sprite allocation, berth activation or work.

        @param self Explicit source-only full matrix handoff fixture.
        """
        self.assertTrue(self.report["coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(self.family["native_inert_approach_slots"], 1)
        self.assertEqual(self.family["native_approach_slots"], 2)


if __name__ == "__main__":
    unittest.main()
