"""Verify original Core base geometry without granting native module facilities.

@file space_art_core.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13E
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group70.1
Source fixtures protect one fixed3x3 model and its original reserved approach.
Tile cuts, functional states, native packing and ordinary review remain separate.
"""
import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_module_art as core


class CoreBaseTests(unittest.TestCase):
    """Protect fixed native tile geometry, one approach and immutable original pixels."""

    @classmethod
    def setUpClass(cls):
        """Resolve the selected original Core once for focused base-source checks.

        @param cls Fixture sharing only the accepted original's source pixels.
        """
        cls.source = art.read_record()
        cls.public = json.loads(core.PUBLIC.read_text(encoding="utf-8"))
        cls.key = cls.source["families"][core.ASSET]["base_selection"]
        cls.native, cls.evidence = core.native_base(cls.key, cls.source, cls.public)

    def setUp(self):
        """Detach source and public metadata before each deliberate refusal.

        @param self Independent original-Core geometry fixture.
        """
        self.document = copy.deepcopy(self.source)
        self.manifest = copy.deepcopy(self.public)
        self.definition = self.document["families"][core.ASSET]
        self.candidate = next(row for row in self.document["candidates"] if row["key"] == self.key)

    def resign(self):
        """Sign deliberately invalid test geometry to exercise native invariants.

        @param self Detached source fixture; immutable source files remain untouched.
        """
        self.definition["approved_base_registration"] = core.registration_signature(self.candidate)

    def test_actual_native_core_footprint_and_approach_owners(self):
        """Live native definitions retain3x3 occupancy and the sole south approach.

        @param self Source-only native contract consumer; no runtime acceptance claim.
        """
        definitions = (ROOT / "src/multilayer_space_content.cpp").read_text(encoding="utf-8")
        self.assertRegex(definitions, r"SpaceBuildKind::Core, UINT16_MAX, core,[^\n]+, 3, 3\}")
        structures = (ROOT / "src/space_structure.cpp").read_text(encoding="utf-8")
        start = structures.index("SpacePoint GetSpaceStructureApproach(")
        body = structures[start:structures.index("/** @copydoc IsSpaceStructureSiteValid", start)]
        self.assertIn("int32_t{s.x} * SPACE_Q16", body)
        self.assertIn("int32_t{s.y} + (d == nullptr ? 0 : d->height)", body)
        station = (ROOT / "src/orbital_station.cpp").read_text(encoding="utf-8")
        self.assertIn("if (kind == SpaceBuildKind::Core) return 1;", station)

    def test_whole_model_fits_all_nine_unchanged_native_cell_canvases(self):
        """Every occupied native pixel has a real cell owner at the public anchor.

        @param self Complete original footprint/anchor coverage fixture.
        """
        self.assertEqual(self.native.size, (256, 192))
        self.assertEqual(core.CELLS, [(x, y) for y in range(3) for x in range(3)])
        self.assertEqual(len(self.evidence["covered_pixels_per_cell"]), 9)
        self.assertTrue(all(count > 0 for count in self.evidence["covered_pixels_per_cell"]))
        # Check actual converted opaque pixels independently of the converter's
        # reported coverage totals; the later cuts cannot repair a clipped base.
        for y in range(self.native.height):
            for x in range(self.native.width):
                if self.native.getpixel((x, y)) != 0:
                    self.assertTrue(any(0 <= x - ox < 64 and 0 <= y - oy < 64 for ox, oy in core.ORIGINS))
        self.assertEqual(core.public_contract(self.manifest)["anchor"], [-31, -33])

    def test_reserved_approach_stays_outside_the_original_model(self):
        """Source geometry preserves the actual single approach centre without a painted hull.

        @param self Native approach projection and original transparency fixture.
        """
        # The native position is the centre of(x,y+3): adding half a tile in
        # both axes leaves screenX unchanged and adds16native pixels to screenY.
        expected = (128 + 32 * 3, 64 + 16 * 3 + 16)
        self.assertEqual(core.APPROACH, expected)
        self.assertEqual(self.native.getpixel(expected), 0)
        self.assertEqual(self.definition["native_approach_slots"], 1)

    def test_changed_public_geometry_or_extra_rotation_refuses(self):
        """A valid original cannot authorize a shrunken footprint or invented orientation.

        @param self Detached public geometry corruption fixture.
        """
        changes = (("footprint", [2, 2]), ("directions", ["N", "E"]),
                   ("tile_variants", 4), ("anchor", [-32, -32]))
        # Test each invariant independently so an earlier invalid field cannot
        # mask acceptance of another altered native source contract.
        for key, value in changes:
            document = copy.deepcopy(self.public)
            row = next(row for row in document["assets"] if row["asset_id"] == core.ASSET)
            row[key] = value
            with self.subTest(field=key):
                with self.assertRaises(ValueError):
                    core.public_contract(document)

    def test_unapproved_transform_change_refuses_before_image_access(self):
        """An old source approval cannot authorize a different whole-model registration.

        @param self Changed native reference fixture with its original old signature.
        """
        self.candidate["native_transform"]["source_reference"][0] += 1
        with patch.object(core.Image, "open", side_effect=AssertionError("unexpected image IO")):
            with self.assertRaisesRegex(ValueError, "approved Core base registration changed"):
                core.native_base(self.key, self.document, self.manifest)

    def test_fresh_signature_cannot_move_native_floor_or_approach(self):
        """Source approval cannot override the original footprint or reserved berth centre.

        @param self Newly signed invalid floor/approach metadata fixture.
        """
        # Both projection and berth belong to native geometry; separately
        # re-signed artwork must refuse changes to either original anchor.
        for field in ("native_reference", "native_approach"):
            self.document = copy.deepcopy(self.source)
            self.definition = self.document["families"][core.ASSET]
            self.candidate = next(row for row in self.document["candidates"] if row["key"] == self.key)
            self.candidate["native_transform"][field][0] += 1
            self.resign()
            with self.assertRaisesRegex(ValueError, "Core whole-model floor/approach contract changed"):
                core.native_base(self.key, self.document, self.manifest)

    def test_new_signature_cannot_stretch_measured_original_floor(self):
        """Measured source vertices must agree with the native diamond under one scale.

        @param self Re-signed incorrect source-floor geometry fixture.
        """
        self.candidate["native_transform"]["source_floor_corners"][1][0] += 100
        self.resign()
        with self.assertRaisesRegex(ValueError, "Core original floor does not match native3x3 projection"):
            core.native_base(self.key, self.document, self.manifest)

    def test_outside_crop_cannot_conceal_missing_source_pixels(self):
        """An enlarged crop cannot pad past its original to invent safe source clearance.

        @param self Incorrect freshly signed whole-source rectangle fixture.
        """
        width, height = self.candidate["image_size"]
        self.candidate["native_transform"]["crop_box"] = [0, 0, width + 1, height]
        self.resign()
        with self.assertRaisesRegex(ValueError, "Core crop leaves original"):
            core.native_base(self.key, self.document, self.manifest)

    def test_new_signature_cannot_move_coupling_away_from_native_approach(self):
        """An attractive dock elsewhere on the floor edge cannot substitute for the native berth.

        @param self Re-signed incorrect physical coupling-source contact fixture.
        """
        candidate = next(row for row in self.document["candidates"] if row["key"] == self.definition["coupling_source"])
        candidate["native_transform"]["native_reference"][0] -= 5
        self.definition["approved_coupling_registration"] = core.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "Core original coupling misses the native approach edge"):
            core.native_base(self.key, self.document, self.manifest)

    def test_one_original_coupling_preserves_every_other_body_pixel(self):
        """Component assembly adds only approved original hardware at the one native contact.

        @param self Actual separate-source assembly and outside-pixel protection fixture.
        """
        body, _ = core.native_body(self.key, self.source, self.public)
        allowed = {tuple(point) for point in self.evidence["coupling_pixels"]}
        self.assertGreaterEqual(self.evidence["original_mount_overlap_pixels"], 3)
        self.assertEqual(self.evidence["coupling"]["native_contact"], [208, 120])
        # Check stored native pixels independently of the converter's protection
        # count, so emptying the original deck cannot pass as adding a coupling.
        for y in range(self.native.height):
            for x in range(self.native.width):
                if (x, y) not in allowed:
                    self.assertEqual(self.native.getpixel((x, y)), body.getpixel((x, y)))

    def test_foreign_original_cannot_supply_core_hardware(self):
        """Another module's source receipt does not grant the Core's physical identity.

        @param self Foreign valid-original metadata fixture.
        """
        self.candidate["asset_id"] = "module.dock"
        with self.assertRaisesRegex(ValueError, "Core original belongs to another family"):
            core.native_base(self.key, self.document, self.manifest)

    def test_immutable_ancestry_is_checked_before_conversion(self):
        """Raw images and prompt receipts retain their exact original byte identities.

        @param self Mocked changed-source identity fixture; no real source is overwritten.
        """
        with patch.object(art, "identity", return_value={"path": self.candidate["path"], "sha256": "0" * 64}):
            with self.assertRaisesRegex(ValueError, "immutable Space source ancestry changed"):
                core.native_base(self.key, self.document, self.manifest)

    def test_base_checkpoint_does_not_claim_functional_or_runtime_coverage(self):
        """One completed base remains a bounded checkpoint before tile/state authoring.

        @param self Original staged-base source ownership fixture.
        """
        reference = self.definition["base_source_review"]
        self.assertEqual(art.identity(ROOT / reference["path"]), reference)
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        self.assertFalse(report["complete_source_coverage"])
        self.assertFalse(report["packed"])
        self.assertFalse(report["runtime_activated"])


if __name__ == "__main__":
    unittest.main()
