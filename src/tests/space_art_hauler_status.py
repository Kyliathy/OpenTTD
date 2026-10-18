"""Verify actual hauler blocking preserves cargo and the original display geometry.

@file space_art_hauler_status.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups66C.2/66C.3
Source-only display checks grant no cargo, blocking, service or runtime authority.
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
import convert_space_hauler_parts as hauler
import convert_space_status_parts as status


class HaulerStatusTests(unittest.TestCase):
    """Protect the actual navy cores, all cargo quantities and unsupported service."""

    @classmethod
    def setUpClass(cls):
        """Read the verified blocked atlas and independently resolve actual static loads.

        @param cls Fixture sharing immutable original pixels only.
        """
        cls.record = art.read_record()
        family = cls.record["families"][hauler.ASSET]
        reference = family["blocked_source_review"]
        assert art.identity(ROOT / reference["path"]) == reference
        cls.report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        # The report and actual atlas bytes must agree; a later edited image
        # cannot inherit an old source review merely by keeping its filename.
        for artifact in cls.report["artifacts"]:
            assert art.identity(ROOT / artifact["path"]) == artifact
        cls.atlas = Image.open((ROOT / reference["path"]).parent / "native.png")
        cls.bodies = {}
        # Resolve each actual quantity independently of the blocked producer so
        # a visually empty replacement cannot pass as preserved positive freight.
        for direction in family["directions"]:
            cls.bodies["empty", direction] = art.native_view(family["selections"]["free/" + direction], direction, cls.record)
            for quantity in ("partial", "full"):
                cls.bodies[quantity, direction] = hauler.load_pose(quantity, direction, cls.record)

    def setUp(self):
        """Detach metadata before each source approval or geometry refusal.

        @param self Independent original-hauler display fixture.
        """
        self.document = copy.deepcopy(self.record)
        self.family = self.document["families"][hauler.ASSET]

    def test_all24_blocked_poses_keep_actual_cargo_hardware_and_opacity(self):
        """Only the original display cores differ from independently resolved bodies.

        @param self Complete actual blocked/quantity pixel fixture.
        """
        self.assertEqual(self.report["actual_blocked_poses"], 24)
        self.assertEqual(self.report["actual_display_components"], 8)
        # Every opaque and transparent pixel outside the original screen remains
        # exact, including cargo, foreground rails, nozzle and absence of gas.
        for item in self.report["poses"]:
            quantity, direction = item["quantity"], item["direction"]
            column = self.family["directions"].index(direction)
            row = ("empty", "partial", "full").index(quantity)
            pose = self.atlas.crop((column * 64, row * 48, column * 64 + 64, row * 48 + 48))
            body, _ = self.bodies[quantity, direction]
            editable = {tuple(point) for point in self.family["status_edit_pixels"][direction]}
            self.assertEqual(item["quantity_body_sha256"], hashlib.sha256(body.tobytes()).hexdigest())
            actual = set()
            for y in range(48):
                for x in range(64):
                    old, new = body.getpixel((x, y)), pose.getpixel((x, y))
                    self.assertEqual(old == 0, new == 0)
                    if old != new:
                        actual.add((x, y))
                    if (x, y) not in editable:
                        self.assertEqual(new, old)
            self.assertTrue(actual)
            self.assertTrue(actual.issubset(editable))
            self.assertEqual(actual, {tuple(point) for point in item["changed_display_pixels"]})
        self.assertFalse(self.report["complete_source_coverage"])
        self.assertFalse(self.report["runtime"])

    def test_direct_loaded_api_matches_the_reviewed_source(self):
        """The blocked selector cannot return an empty body for a full loaded craft.

        @param self Actual direct source-consumer fixture.
        """
        pose, evidence = hauler.blocked_pose("full", "N", self.document)
        self.assertEqual(pose.tobytes(), self.atlas.crop((0, 96, 64, 144)).tobytes())
        self.assertEqual(evidence["body"]["state"], "full")

    def test_tiny_side_display_is_not_enlarged(self):
        """Native side visibility uses the actual dark core and preserves shaded edges.

        @param self Exact side-camera screen ownership fixture.
        """
        self.assertEqual(self.family["status_edit_pixels"]["W"], [[11, 23]])
        self.assertEqual(self.family["status_edit_pixels"]["E"], [[51, 23], [51, 24]])
        # The same physical screen supplies every quantity; cargo cannot move
        # the indicator or authorize larger source pixels on a thin side view.
        for direction in self.family["directions"]:
            rows = [item for item in self.report["poses"] if item["direction"] == direction]
            self.assertEqual(len(rows), 3)
            self.assertTrue(all(item["changed_display_pixels"] == rows[0]["changed_display_pixels"] for item in rows))

    def test_widened_display_mask_is_refused(self):
        """A new authored picture cannot broaden already inspected screen ownership.

        @param self Changed original display geometry fixture.
        """
        self.family["status_edit_pixels"]["N"].append([32, 12])
        with self.assertRaisesRegex(ValueError, "approved hauler display geometry changed"):
            hauler.blocked_pose("empty", "N", self.document)

    def test_fresh_signatures_cannot_authorize_a_bezel_pixel(self):
        """Even signed metadata cannot turn the original ivory surround into a lamp.

        @param self Invalid newly approved display ownership fixture.
        """
        self.family["status_edit_pixels"]["N"].append([32, 12])
        self.family["approved_display_geometry"]["N"] = hauler.display_geometry_signature(self.family, "N")
        candidate = art.source_candidate(self.family["status_sources"]["blocked"]["0"]["N"], self.document)
        self.family["approved_status_registration"]["blocked"]["0"]["N"] = status.status_signature(
            candidate, "N", self.family["status_component_bounds"]["N"],
            self.family["approved_base_registration"]["N"], self.family["status_edit_pixels"]["N"])
        with self.assertRaisesRegex(ValueError, "not the original navy face"):
            hauler.blocked_pose("empty", "N", self.document)

    def test_changed_status_registration_and_reference_ancestry_refuse(self):
        """A moved or substituted original cannot inherit the selected display approval.

        @param self Independent source placement and reference identity failures.
        """
        # Check each independent tampering mode against a fresh source catalogue;
        # a previous failure must not mask the actual rejection under test.
        for field in ("registration", "ancestry"):
            document = copy.deepcopy(self.record)
            family = document["families"][hauler.ASSET]
            candidate = art.source_candidate(family["status_sources"]["blocked"]["0"]["N"], document)
            if field == "registration":
                candidate["native_transforms"]["N"]["source_reference"][0] += 1
                message = "approved status registration changed"
            else:
                candidate["references"][0]["sha256"] = "0" * 64
                message = "immutable Space source ancestry changed"
            with self.assertRaisesRegex(ValueError, message):
                hauler.blocked_pose("empty", "N", document)

    def test_wrong_original_se_projection_cannot_reenter(self):
        """The first sheet's incorrect SE diagonal remains excluded despite a new draft.

        @param self Explicit unaccepted original camera reentry fixture.
        """
        candidate = art.source_candidate("orbital-hauler-blocked-n-ne-e-se-v1", self.document)
        candidate["directions"].append("SE")
        candidate["native_transforms"]["SE"] = copy.deepcopy(candidate["native_transforms"]["NE"])
        self.family["status_sources"]["blocked"]["0"]["SE"] = candidate["key"]
        self.family["approved_status_registration"]["blocked"]["0"]["SE"] = status.status_signature(
            candidate, "SE", self.family["status_component_bounds"]["SE"],
            self.family["approved_base_registration"]["SE"], self.family["status_edit_pixels"]["SE"])
        with self.assertRaisesRegex(ValueError, "unaccepted original Space view"):
            hauler.blocked_pose("empty", "SE", self.document)

    def test_service_stays_an_explicit_unreachable_empty_alias(self):
        """Blocked art supplies no repair lamps, paid work or invented animation phase.

        @param self Original native17service and static-source policy fixture.
        """
        self.assertEqual(self.family["source_state_aliases"], {"service": "empty"})
        self.assertEqual(set(self.family["status_sources"]), {"blocked"})
        self.assertEqual(set(self.family["status_sources"]["blocked"]), {"0"})
        with self.assertRaisesRegex(ValueError, "unsupported hauler blocked quantity"):
            hauler.blocked_pose("service", "N", self.document)
        with self.assertRaisesRegex(ValueError, "missing actual authored status phase"):
            status.status_pose("blocked", "N", 1, self.document, hauler.ASSET)


if __name__ == "__main__":
    unittest.main()
