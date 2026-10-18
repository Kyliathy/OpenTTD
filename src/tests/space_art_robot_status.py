"""Verify authored service/blocked cues on the retained robot's original display.

@file space_art_robot_status.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13CB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group65B.3b
Only the frozen native navy face may change. Source cues grant no service,
work, cargo, movement, extra hardware or runtime sprite activation.
"""
import copy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_status_parts as status

ASSET = "vehicle.model-16"
PHASES = (("blocked", 0), ("service", 0), ("service", 1))


class RetainedRobotStatusTests(unittest.TestCase):
    """Protect actual display projections, distinct light phases and exact body pixels."""

    @classmethod
    def setUpClass(cls):
        """Resolve original bodies and all actual display variants once.

        @param cls Fixture sharing only immutable native source pixels.
        """
        cls.source = art.read_record()
        family = cls.source["families"][ASSET]
        cls.originals, cls.poses = {}, {}
        # Every actual cue must survive its own physical camera and native mask; repeated headings or a display borrowed from another role cannot complete this matrix.
        for direction in family["directions"]:
            cls.originals[direction], _ = art.native_view(family["selections"]["free/" + direction], direction, cls.source)
            for state, phase in PHASES:
                cls.poses[state, phase, direction] = status.status_pose(state, direction, phase, cls.source, ASSET)

    def setUp(self):
        """Detach receiving display/source metadata for each refused change.

        @param self Independent actual-status approval fixture.
        """
        self.record = copy.deepcopy(self.source)
        self.family = self.record["families"][ASSET]
        self.candidate = art.source_candidate(self.family["status_sources"]["blocked"]["0"]["N"], self.record)

    def test_all_twenty_four_actual_cues_keep_their_native_colors(self):
        """Amber blocking and cyan service remain visibly distinct on every real face.

        @param self Actual quantized native status/color fixture.
        """
        self.assertEqual(len(self.poses), 24)
        # A dark still, ivory bezel or metadata flag cannot substitute for an actual authored colored cue on the original navy screen.
        for (state, _, direction), (pose, _) in self.poses.items():
            colors = [pose.convert("RGB").getpixel(tuple(point)) for point in self.family["status_edit_pixels"][direction]]
            if state == "service":
                self.assertTrue(any(g >= r + 25 and b >= r + 25 for r, g, b in colors), direction)
            else:
                self.assertTrue(any(r >= b + 50 and g >= b + 25 for r, g, b in colors), direction)

    def test_actual_service_phases_differ_in_all_eight_cameras(self):
        """Opposite-end illumination survives the original tiny native screen masks.

        @param self Actual source-phase readability fixture.
        """
        # Check even the three-pixel E screen and five-pixel W screen rather than accepting a difference only in the larger front display.
        for direction in self.family["directions"]:
            first, _ = self.poses["service", 0, direction]
            second, _ = self.poses["service", 1, direction]
            self.assertNotEqual(first.tobytes(), second.tobytes(), direction)

    def test_opacity_bezels_and_all_other_original_body_pixels_are_exact(self):
        """Status changes only original navy-face pixels and never any physical geometry.

        @param self Whole native body, face-mask and opacity fixture.
        """
        # Compare every native pixel independently of converter evidence; ivory edges, folded arms, coupler, engine and every transparent point retain their exact original values.
        for (_, _, direction), (pose, _) in self.poses.items():
            base = self.originals[direction]
            allowed = {tuple(point) for point in self.family["status_edit_pixels"][direction]}
            for y in range(48):
                for x in range(64):
                    old, new = base.getpixel((x, y)), pose.getpixel((x, y))
                    self.assertEqual(old == 0, new == 0)
                    if (x, y) not in allowed:
                        self.assertEqual(old, new)

    def test_original_face_masks_keep_their_reviewed_extent(self):
        """No camera completion or phase readability fix may widen the original display.

        @param self Frozen navy-face geometry and body approval fixture.
        """
        counts = dict(N=14, NE=11, E=3, SE=11, S=10, SW=10, W=5, NW=9)
        self.assertEqual(self.family["status_geometry_base_review"], self.family["base_heading_review"])
        # All editable points already belong to the original opaque display; no floating light or new opacity can enter the source contract.
        for direction, points in self.family["status_edit_pixels"].items():
            self.assertEqual(len(points), counts[direction])
            self.assertTrue(all(self.originals[direction].getpixel(tuple(point)) != 0 for point in points))

    def test_nine_component_records_retain_three_real_originals(self):
        """Three actual source sheets are counted once each across their status variants.

        @param self Immutable original source/receipt ancestry fixture.
        """
        keys, originals = set(), set()
        # Each state/phase/heading retains its own approved original part. Repeated component records from one actual sheet are not extra generated artwork.
        for state, phase in PHASES:
            for direction in self.family["directions"]:
                key = self.family["status_sources"][state][str(phase)][direction]
                candidate = art.source_candidate(key, self.record)
                keys.add(key)
                originals.add((candidate["path"], candidate["sha256"]))
                self.assertEqual(candidate["asset_id"], ASSET)
                self.assertEqual(candidate["generation_receipt"], art.identity(ROOT / candidate["generation_receipt"]["path"]))
        self.assertEqual(len(keys), 9)
        self.assertEqual(len(originals), 3)

    def test_rejected_opposite_display_projections_cannot_reenter(self):
        """Wrong W/NW slopes remain unaccepted even if their transform fields are added.

        @param self Explicit rejected physical-camera fixture.
        """
        candidate = art.source_candidate("construction-robot-status-n-ne-w-nw-v1-service-0", self.record)
        # A valid N/NE part from this sheet does not approve its wrongly rendered W/NW geometry or authorize fabricating a missing physical source transform.
        for direction in ("W", "NW"):
            candidate["native_transforms"][direction] = copy.deepcopy(candidate["native_transforms"]["N"])
            with self.assertRaisesRegex(ValueError, "unaccepted original Space view"):
                art.source_pixels(candidate, direction, self.family["status_component_bounds"][direction])

    def test_widened_face_mask_refuses(self):
        """A later source edit cannot recolor the pale hull under an old face approval.

        @param self Altered native display-mask fixture.
        """
        self.family["status_edit_pixels"]["N"].append([26, 10])
        with self.assertRaisesRegex(ValueError, "approved status registration changed"):
            status.status_pose("blocked", "N", 0, self.record, ASSET)

    def test_shifted_source_reference_refuses(self):
        """The original display pattern cannot drift independently of its body contact.

        @param self Shifted authored component fixture.
        """
        self.candidate["native_transforms"]["N"]["native_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "approved status registration changed"):
            status.status_pose("blocked", "N", 0, self.record, ASSET)

    def test_foreign_role_status_source_refuses(self):
        """A builder/miner status panel cannot borrow retained16's original screen.

        @param self Incorrect physical role fixture.
        """
        self.candidate["asset_id"] = "vehicle.model-20"
        with self.assertRaisesRegex(ValueError, "source is not the declared authored status panel"):
            status.status_pose("blocked", "N", 0, self.record, ASSET)

    def test_wrong_authored_state_refuses(self):
        """A service bar cannot masquerade as the approved static blocked cue.

        @param self Incorrect authored state fixture.
        """
        self.candidate["panel_state"] = "service"
        with self.assertRaisesRegex(ValueError, "source is not the declared authored status panel"):
            status.status_pose("blocked", "N", 0, self.record, ASSET)

    def test_changed_exact_generation_receipt_refuses(self):
        """Actual authored light patterns remain tied to the executed source request.

        @param self Corrupted prompt/provenance fixture.
        """
        self.candidate["generation_receipt"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry"):
            status.status_pose("blocked", "N", 0, self.record, ASSET)

    def test_missing_real_service_heading_refuses(self):
        """The public service phase cannot fall back to a neighboring display view.

        @param self Missing actual phase/heading source fixture.
        """
        del self.family["status_sources"]["service"]["1"]["W"]
        with self.assertRaisesRegex(ValueError, "missing genuine status heading"):
            status.status_pose("service", "W", 1, self.record, ASSET)

    def test_static_blocked_cannot_claim_an_extra_source_phase(self):
        """The interlock remains one original still, without a fabricated animation.

        @param self Unsupported static-source phase fixture.
        """
        with self.assertRaisesRegex(ValueError, "missing actual authored status phase"):
            status.status_pose("blocked", "N", 1, self.record, ASSET)


if __name__ == "__main__":
    unittest.main()
