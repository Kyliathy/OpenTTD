"""Protect actual hauler quantities, source ancestry and original foreground.

@file space_art_hauler_load.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group66B.3
Only original cargo cutouts are assembled. Native packets, capacity and service
remain authoritative; this fixture does not claim packing or runtime display.
"""
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_hauler_parts as hauler


class HaulerLoadTests(unittest.TestCase):
    """Verify visible native freight without granting ownership of hull or capacity."""

    def setUp(self):
        """Read detached immutable-source metadata before each focused check.

        @param self Independent quantity/source protection fixture.
        """
        self.record = art.read_record()
        self.family = self.record["families"][hauler.ASSET]

    def test_all_quantities_preserve_outside_hardware_and_original_opacity(self):
        """Actual freight changes only the approved bay in all sixteen source poses.

        @param self Eight-camera original-body and cargo-depth fixture.
        """
        # Both positive quantities must preserve every original near fixture and
        # opaque pixel; camera-specific masking cannot conceal a repainted hull.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.record)
            editable, foreground, _, _ = hauler.bay_geometry(direction, self.record)
            for state in ("partial", "full"):
                pose, evidence = hauler.load_pose(state, direction, self.record)
                self.assertGreaterEqual(evidence["changed_pixels"], 8)
                self.assertEqual(pose.size, (64, 48))
                self.assertEqual(evidence["actual_authored_phase"], 0)
                # Inspect empty air as well as hull pixels. Cargo may add
                # opacity only within its bay and may never remove old opacity.
                for y in range(48):
                    for x in range(64):
                        old, new = base.getpixel((x, y)), pose.getpixel((x, y))
                        if old != 0:
                            self.assertNotEqual(new, 0)
                        if (x, y) not in editable:
                            self.assertEqual(new, old)
                self.assertTrue(all(pose.getpixel(point) == base.getpixel(point) for point in foreground))

    def test_each_original_uses_one_scale_and_the_approved_floor_reference(self):
        """Partial cases keep the same physical floor/height policy as full loads.

        @param self Immutable original registration and camera coverage fixture.
        """
        keys = {key for sources in self.family["load_sources"].values() for key in sources.values()}
        # A generated source keeps one uniform physical scale; per-direction
        # stretching or a changed world contact cannot disguise inconsistent art.
        for key in keys:
            candidate = art.source_candidate(key, self.record)
            scales = {tuple(value["uniform_scale"]) for value in candidate["native_transforms"].values()}
            self.assertEqual(len(scales), 1)
            for direction, transform in candidate["native_transforms"].items():
                self.assertEqual(transform["native_reference"], self.family["cargo_bays"][direction]["floor_reference"])
                self.assertEqual(hauler.load_signature(candidate, direction, self.family),
                                 self.family["approved_load_registration"][candidate["load_state"]][direction])

    def test_partial_original_cannot_supply_full_quantity(self):
        """A missing full load cannot be replaced by relabeling a one-case source.

        @param self Incorrect quantity-bank ownership fixture.
        """
        self.family["load_sources"]["full"]["N"] = self.family["load_sources"]["partial"]["N"]
        with self.assertRaisesRegex(ValueError, "declared original hauler quantity"):
            hauler.load_pose("full", "N", self.record)

    def test_changed_component_registration_is_refused(self):
        """Original cargo may not drift independently of its approved bay contact.

        @param self Post-approval native cargo-transform fixture.
        """
        candidate = art.source_candidate(self.family["load_sources"]["full"]["N"], self.record)
        candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "approved hauler cargo registration changed"):
            hauler.load_pose("full", "N", self.record)

    def test_fresh_approval_cannot_place_cargo_outside_the_physical_bay(self):
        """A newly signed draft cannot authorize cargo over arbitrary hull hardware.

        @param self Physically displaced but metadata-signed cargo fixture.
        """
        candidate = art.source_candidate(self.family["load_sources"]["full"]["N"], self.record)
        transform = candidate["native_transforms"]["N"]
        transform["source_reference"][0] -= 8 * transform["uniform_scale"][1]
        self.family["approved_load_registration"]["full"]["N"] = hauler.load_signature(candidate, "N", self.family)
        with self.assertRaisesRegex(ValueError, "authored hauler cargo leaves its approved bay"):
            hauler.load_pose("full", "N", self.record)

    def test_changed_reference_ancestry_cannot_supply_cargo(self):
        """A replacement reference cannot silently substitute another cassette design.

        @param self Tampered source-reference identity fixture.
        """
        candidate = art.source_candidate(self.family["load_sources"]["partial"]["N"], self.record)
        candidate["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable Space source ancestry changed"):
            hauler.load_pose("partial", "N", self.record)

    def test_unaccepted_tight_e_source_cannot_reenter_by_metadata(self):
        """The earlier three-pixel source margin remains excluded after replacement.

        @param self Explicitly unaccepted original-camera reentry fixture.
        """
        candidate = art.source_candidate("orbital-hauler-full-n-ne-e-se-v1", self.record)
        candidate["directions"].append("E")
        receipt = json.loads((ROOT / candidate["generation_receipt"]["path"]).read_text(encoding="utf-8"))
        window = receipt["layout_review"]["windows"]["E"]
        candidate["native_transforms"]["E"] = dict(crop_box=window["crop_box"],
                                                  measured_subject_bbox_in_crop=window["subject_bbox_in_crop"],
                                                  uniform_scale=[1, 32], source_reference=[384, 836],
                                                  native_reference=self.family["cargo_bays"]["E"]["floor_reference"])
        self.family["load_sources"]["full"]["E"] = candidate["key"]
        self.family["approved_load_registration"]["full"]["E"] = hauler.load_signature(candidate, "E", self.family)
        with self.assertRaisesRegex(ValueError, "unaccepted original Space view"):
            hauler.load_pose("full", "E", self.record)

    def test_complete_load_review_keeps_motion_and_runtime_pending(self):
        """Sixteen reviewed loads preserve obvious quantities without inventing units.

        @param self Source-only load handoff and full/partial native distinction fixture.
        """
        reference = self.family["load_source_review"]
        self.assertEqual(art.identity(ROOT / reference["path"]), reference)
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        self.assertEqual(report["actual_load_pose_count"], 16)
        self.assertEqual(len(report["poses"]), 16)
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime"])
        # Every real camera must keep a clearly larger full source after depth
        # masking; cassette counts are visual categories, never native unit counts.
        for direction in self.family["directions"]:
            result = report["quantity_differences"][direction]
            self.assertGreaterEqual(result["partial_full_differences"], 12)
            self.assertGreaterEqual(result["full_visible_pixels"] * 5, result["partial_visible_pixels"] * 8)


if __name__ == "__main__":
    unittest.main()
