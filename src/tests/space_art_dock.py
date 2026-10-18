"""Verify original Dock geometry, two native clamp instances and immutable base pixels.

@file space_art_dock.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13F
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group71.1
These source checks grant no native activation, work, cargo or service admission.
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

REVIEW = ROOT / "logs/proofs/mlm/native-art/space/dock-base-v1/base-source-review.json"


class DockBaseTests(unittest.TestCase):
    """Protect paid fixed geometry and genuine hardware against plausible source drift."""

    @classmethod
    def setUpClass(cls):
        """Load exact accepted artifacts and one fresh original body for comparison.

        @param cls Fixture sharing immutable original/source-review pixels.
        """
        cls.source = art.read_record()
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        for artifact in cls.report["artifacts"]:
            assert art.identity(ROOT / artifact["path"]) == artifact
        cls.native = Image.open(REVIEW.parent / "native.png")
        cls.clamps = [Image.open(REVIEW.parent / f"clamp{slot}.png") for slot in range(2)]
        cls.body, cls.body_evidence = dock.native_body(document=cls.source)

    def setUp(self):
        """Detach source metadata before every intended refusal.

        @param self Fixture whose edits never touch real original files.
        """
        self.document = copy.deepcopy(self.source)
        self.family = self.document["families"][dock.ASSET]

    def candidate(self, key):
        """Find one exact editable source candidate in the detached catalogue.

        @param self Current detached source fixture.
        @param key Explicit retained original identity.
        @return Detached candidate metadata only.
        """
        return next(row for row in self.document["candidates"] if row["key"] == key)

    def test_four_fixed_cells_and_half_tile_berths_retain_original_native_owners(self):
        """Public geometry and actual source projections agree with existing native exports.

        @param self Current fixed Dock source/native declaration fixture.
        """
        row = dock.public_contract()
        self.assertEqual(row["footprint"], [2, 2])
        self.assertEqual(row["directions"], ["none"])
        self.assertEqual(row["anchor"], [-31, -33])
        self.assertEqual(dock.CELLS, ((0, 0), (1, 0), (0, 1), (1, 1)))
        self.assertEqual(tuple(b - a for a, b in zip(*dock.APPROACHES)), (-16, 8))
        self.assertEqual(self.family["native_inert_approach_slots"], 1)
        self.assertEqual(self.family["native_approach_slots"], 2)
        native = (ROOT / "src/space_approach.cpp").read_text(encoding="utf-8")
        self.assertIn("point.x += int32_t{slot} * (SPACE_Q16 / 2);", native)
        self.assertIn("site.module->activation_ticks == 0 ? GetOrbitalModuleServiceCapacity(site.state.kind) : 1", native)
        self.assertRegex((ROOT / "src/multilayer_space_content.cpp").read_text(encoding="utf-8"),
                         r"SpaceBuildKind::Dock[^\n]+, 2, 2\}")

    def test_two_instances_keep_one_original_clamp_without_rotation_or_extra_pixels(self):
        """The second real clamp is the exact native half-tile displacement of the first.

        @param self Stored physical clamp source fixture.
        """
        first, second = [], []
        # Observe actual pixels rather than trust instance labels; a rotated,
        # duplicated whole Dock or extra clamp cannot satisfy this comparison.
        for slot, image in enumerate(self.clamps):
            points = first if slot == 0 else second
            for y in range(dock.CANVAS[1]):
                for x in range(dock.CANVAS[0]):
                    value = image.getpixel((x, y))
                    if value:
                        points.append((x, y, value))
        self.assertTrue(first)
        self.assertEqual([(x - 16, y + 8, value) for x, y, value in first], second)

    def test_all_body_pixels_outside_actual_hardware_stay_exact(self):
        """Original deck and cabinet are protected outside the two genuine source instances.

        @param self Actual native body, clamp and assembled-base pixel fixture.
        """
        protected = 0
        # Independent pixel inspection checks both attachment and preservation;
        # claimed source coverage cannot excuse a hidden repaint or lost tile.
        for y in range(dock.CANVAS[1]):
            for x in range(dock.CANVAS[0]):
                values = [image.getpixel((x, y)) for image in self.clamps]
                present = [value for value in values if value]
                self.assertLessEqual(len(present), 1)
                expected = present[0] if present else self.body.getpixel((x, y))
                self.assertEqual(self.native.getpixel((x, y)), expected)
                protected += not present
        self.assertEqual(protected, self.report["base"]["protected_body_canvas_pixels"])
        for clamp in self.report["base"]["clamps"]:
            self.assertGreaterEqual(clamp["body_overlap_pixels"], 3)

    def test_original_pixels_fit_all_four_canvases_and_leave_both_approaches_clear(self):
        """No cropped geometry or source furniture occupies the native service points.

        @param self Complete original native source canvas fixture.
        """
        self.assertTrue(all(self.body_evidence["covered_pixels_per_cell"]))
        for point in dock.APPROACHES:
            self.assertEqual(self.native.getpixel(point), 0)
        # Every opaque source pixel needs a real sorting cell; a whole-image
        # checksum alone cannot reveal a corner silently cropped during cutting.
        for y in range(dock.CANVAS[1]):
            for x in range(dock.CANVAS[0]):
                if self.native.getpixel((x, y)):
                    self.assertTrue(dock.eligible_tiles(x, y))

    def test_fresh_original_assembly_reproduces_the_published_complete_base(self):
        """Current original ancestry and conversion match the concrete stored review pixels.

        @param self Direct source-producer compatibility fixture.
        """
        image, evidence = dock.native_base(self.document)
        self.assertEqual(image.tobytes(), self.native.tobytes())
        self.assertEqual(image.getpalette(), self.native.getpalette())
        self.assertEqual(evidence["native_sha256"], self.report["base"]["native_sha256"])

    def test_unsigned_body_change_refuses_before_decoding_any_original(self):
        """A changed transform cannot inherit the selected body's prior approval.

        @param self Unsigned original registration corruption fixture.
        """
        candidate = self.candidate(self.family["body_sources"]["complete"])
        candidate["native_transform"]["source_reference"][0] += 1
        with patch.object(dock, "original_image", side_effect=AssertionError("unexpected image access")):
            with self.assertRaisesRegex(ValueError, "body or stage registration changed"):
                dock.native_body(document=self.document)

    def test_new_body_signature_cannot_shift_or_steepen_the_native_floor(self):
        """Fresh source approval cannot redefine the real2x2 floor projection.

        @param self Signed native origin and measured-floor corruption fixture.
        """
        # Each independent defect starts from the accepted metadata, so an
        # earlier refusal cannot conceal acceptance of the other geometry error.
        for field in ("native_reference", "source_floor_corners"):
            self.document = copy.deepcopy(self.source)
            self.family = self.document["families"][dock.ASSET]
            candidate = self.candidate(self.family["body_sources"]["complete"])
            if field == "native_reference":
                candidate["native_transform"][field][0] += 1
            else:
                candidate["native_transform"][field][2][1] += 100
            self.family["approved_body_registration"]["complete"] = dock.registration_signature(candidate)
            with patch.object(dock, "original_image", side_effect=AssertionError("unexpected image access")):
                with self.assertRaisesRegex(ValueError, "native floor contract|native2x2 projection"):
                    dock.native_body(document=self.document)

    def test_foreign_family_cannot_borrow_a_valid_original_receipt(self):
        """An original's identity does not authorize a Core as Dock equipment.

        @param self Re-signed foreign physical-role metadata fixture.
        """
        candidate = self.candidate(self.family["body_sources"]["complete"])
        candidate["asset_id"] = "module.nucleus"
        self.family["approved_body_registration"]["complete"] = dock.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "body or stage registration changed"):
            dock.native_body(document=self.document)

    def test_family_metadata_cannot_create_another_native_service_slot(self):
        """Extra authoring capacity cannot activate a new physical berth or approach cell.

        @param self Deliberately changed native approach ownership fixture.
        """
        self.family["native_approach_slots"] = 3
        with patch.object(dock, "original_image", side_effect=AssertionError("unexpected image access")):
            with self.assertRaisesRegex(ValueError, "actual fixed native geometry or berth ownership"):
                dock.native_base(self.document)
        self.family["native_approach_slots"] = 2
        with self.assertRaisesRegex(ValueError, "cannot add a native berth"):
            dock.clamp_source(2, self.document)

    def test_signed_clamp_envelope_cannot_expand_into_a_reserved_approach(self):
        """A new registration signature cannot enlarge the original mounting contract.

        @param self Signed physical-clamp envelope drift fixture.
        """
        candidate = self.candidate(self.family["clamp_source"])
        candidate["native_transform"]["native_relative_bounds"][2] = 1
        self.family["approved_clamp_registration"] = dock.registration_signature(candidate)
        with patch.object(dock, "original_image", side_effect=AssertionError("unexpected image access")):
            with self.assertRaisesRegex(ValueError, "mounting envelope changed"):
                dock.clamp_source(0, self.document)

    def test_signed_oversized_clamp_is_refused_instead_of_clipped(self):
        """A small destination canvas cannot hide a physically oversized source original.

        @param self Re-signed enlarged original transform fixture.
        """
        candidate = self.candidate(self.family["clamp_source"])
        candidate["native_transform"]["uniform_scale"][0] *= 4
        self.family["approved_clamp_registration"] = dock.registration_signature(candidate)
        with self.assertRaises(ValueError):
            dock.clamp_source(1, self.document)

    def test_changed_original_bytes_cannot_hide_behind_a_cached_source_review(self):
        """Every direct source call revalidates the retained clamp original's ancestry.

        @param self Mocked source replacement without changing a real image.
        """
        target = (ROOT / self.candidate(self.family["clamp_source"])["path"]).resolve()
        original_identity = art.identity

        def changed_identity(path):
            """Simulate a replaced selected image while preserving every real file.

            @param path Exact source/receipt/reference requested by ancestry validation.
            @return Real identity except the intentional selected-source hash mismatch.
            """
            result = original_identity(path)
            if path.resolve() == target:
                result["sha256"] = "0" * 64
            return result

        with patch.object(art, "identity", side_effect=changed_identity):
            with self.assertRaisesRegex(ValueError, "immutable Space source ancestry changed"):
                dock.clamp_source(0, self.document)

    def test_duplicate_selected_original_cannot_silently_choose_one_record(self):
        """One source key must identify exactly one original and registration.

        @param self Duplicate selected-source catalogue corruption fixture.
        """
        self.document["candidates"].append(copy.deepcopy(self.candidate(self.family["clamp_source"])))
        with self.assertRaisesRegex(ValueError, "missing or duplicate Space candidate"):
            dock.clamp_source(0, self.document)

    def test_base_checkpoint_keeps_tile_functional_packing_and_runtime_gates_pending(self):
        """One selected whole base does not deliver the complete64-cell functional matrix.

        @param self Immutable bounded base-review fixture.
        """
        self.assertFalse(self.report["complete_source_coverage"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(len(self.report["base"]["clamps"]), 2)


if __name__ == "__main__":
    unittest.main()
