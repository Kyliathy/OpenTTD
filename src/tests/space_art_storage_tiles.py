"""Verify lossless Storage tile cuts and original near-jaw foreground ownership.

@file space_art_storage_tiles.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13G
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group72.2
The source matrix remains separate from native packing, service and game review.
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
import convert_space_storage_art as storage
import convert_space_storage_tiles as tiles
from tools import orbital_tile_art as cuts

REVIEW = ROOT / "logs/proofs/mlm/native-art/space/storage-tile-cuts-v1/tile-source-review.json"


class StorageTileTests(unittest.TestCase):
    """Protect actual world cells, original pixel continuity and physical depth ownership."""

    @classmethod
    def setUpClass(cls):
        """Load concrete immutable tile/source images once for independent comparisons.

        @param cls Fixture sharing reviewed source pixels without modifying them.
        """
        cls.source = art.read_record()
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        for artifact in cls.report["artifacts"]:
            assert art.identity(ROOT / artifact["path"]) == artifact
        atlas = Image.open(REVIEW.parent / "native-cuts.png")
        cls.parts = []
        for row in cls.report["sprites"]:
            x, y, width, height = row["box"]
            cls.parts.append((row, atlas.crop((x, y, x + width, y + height))))
        cls.original = Image.open(ROOT / "logs/proofs/mlm/native-art/space/storage-base-v1/native.png")

    def setUp(self):
        """Detach the approved policy before deliberately corrupting source metadata.

        @param self Independent source-policy refusal fixture.
        """
        self.document = copy.deepcopy(self.source)
        self.family = self.document["families"][storage.ASSET]
        self.policy = self.family["tile_policy"]

    def detached_parts(self):
        """Give a refusal test private copies of stored part metadata and pixels.

        @param self Fixture holding the immutable accepted part images.
        @return Editable independent metadata/image pairs.
        """
        return [(copy.deepcopy(row), image.copy()) for row, image in self.parts]

    def rebuild(self, pieces, foreground=None):
        """Exercise the stored-part reconstruction boundary with the accepted native geometry.

        @param self Current detached source policy fixture.
        @param pieces Stored part pairs, including deliberate corruption where requested.
        @param foreground Optional deliberately changed physical foreground mask.
        @return Independently reconstructed whole source image if the boundary accepts it.
        """
        mask = self.policy["foreground_pixels"] if foreground is None else foreground
        return cuts.reassemble(pieces, self.policy["geometry"], mask, self.policy["base_native_sha256"], "complete")

    def test_all_eight_parts_retain_four_actual_world_cells_and_three_absent_fronts(self):
        """Every required native part is explicit without inventing missing foreground hardware.

        @param self Complete stored native Storage cut fixture.
        """
        self.assertEqual(len(self.parts), 8)
        self.assertEqual(self.report["nonempty_part_count"], 5)
        self.assertEqual(self.report["structurally_absent_part_count"], 3)
        self.assertEqual({(row["tile_variant"], row["part"]) for row, image in self.parts},
                         {(tile, part) for tile in range(4) for part in ("back", "front")})
        for row, image in self.parts:
            self.assertEqual(row["world_cell"], [row["tile_variant"] % 2, row["tile_variant"] // 2])
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(image.size, (64, 64))
            if row["part"] == "back":
                self.assertFalse(row["structurally_absent"])

    def test_independent_reassembly_preserves_every_original_pixel_once(self):
        """Actual stored cuts independently reconstruct the accepted model without any seam.

        @param self Whole-source pixel and world-cell continuity fixture.
        """
        image = Image.new("P", storage.CANVAS, 0)
        seen = set()
        # Derive origins from real world coordinates rather than the converter
        # helper or stored origin metadata, so moved/cropped pieces cannot hide.
        for row, source in self.parts:
            wx, wy = row["world_cell"]
            ox, oy = 65 + 32 * (wy - wx), 15 + 16 * (wx + wy)
            for y in range(64):
                for x in range(64):
                    value = source.getpixel((x, y))
                    if not value:
                        continue
                    point = (ox + x, oy + y)
                    self.assertNotIn(point, seen)
                    seen.add(point)
                    image.putpixel(point, value)
        self.assertEqual(image.tobytes(), self.original.tobytes())

    def test_only_original_near_jaws_are_foreground_and_receivers_remain_back(self):
        """The original passive receiver cavity and far jaw remain behind a visiting hull.

        @param self Stored source pixels and original clamp-component evidence.
        """
        actual = set()
        # Inspect physical front images, including their native world origins;
        # a promised mask cannot prove that an opaque cavity was left behind.
        for row, image in self.parts:
            if row["part"] != "front":
                continue
            ox, oy = row["native_origin"]
            for y in range(64):
                for x in range(64):
                    if image.getpixel((x, y)):
                        actual.add((ox + x, oy + y))
        self.assertEqual(len(actual), 9)
        self.assertEqual(actual, {tuple(point) for point in self.policy["foreground_pixels"]})
        points = {tuple(point) for point in self.report["source"]["coupling"]["original_pixels"]}
        self.assertEqual(len(actual & points), 9)
        cx, cy = storage.CONTACT
        self.assertNotIn((cx - 4, cy - 4), actual)
        self.assertNotIn((cx - 2, cy - 5), actual)

    def test_fresh_original_cut_source_matches_every_published_part(self):
        """Original source ancestry and direct conversion reproduce all eight actual cuts.

        @param self Direct source consumer/producer compatibility fixture.
        """
        fresh, evidence = tiles.cut_base(self.document)
        previous = {(row["tile_variant"], row["part"]): image for row, image in self.parts}
        self.assertEqual(evidence["foreground_pixel_count"], 9)
        for row, image in fresh:
            self.assertEqual(image.tobytes(), previous[row["tile_variant"], row["part"]].tobytes())

    def test_unsigned_policy_change_refuses_before_original_source_access(self):
        """Changed foreground ownership cannot inherit the existing cut signature.

        @param self Unsigned source-policy corruption fixture.
        """
        self.policy["foreground_owner"] = "entire storage over craft"
        with patch.object(storage, "native_base", side_effect=AssertionError("unexpected original access")):
            with self.assertRaisesRegex(ValueError, "approved Storage tile policy changed"):
                tiles.cut_base(self.document)

    def test_new_signature_cannot_shift_native_origins_or_invent_rotation(self):
        """Fixed native world geometry outranks newly signed authoring metadata.

        @param self Deliberate re-signed geometry/pose drift fixture.
        """
        # Reset between defects so the first rejected mutation cannot conceal
        # accidental acceptance of a second changed anchor or native direction.
        for key in ("anchor", "native_cell_origins", "direction"):
            self.document = copy.deepcopy(self.source)
            self.family = self.document["families"][storage.ASSET]
            self.policy = self.family["tile_policy"]
            if key == "direction":
                self.policy[key] = "N"
            elif key == "anchor":
                self.policy["geometry"][key][0] += 1
            else:
                self.policy["geometry"][key][0][0] += 1
            self.family["approved_tile_policy"] = cuts.signature(self.policy)
            with patch.object(storage, "native_base", side_effect=AssertionError("unexpected original access")):
                with self.assertRaisesRegex(ValueError, "differs from native geometry"):
                    tiles.cut_base(self.document)

    def test_new_signature_cannot_promote_entire_passive_coupling_to_foreground(self):
        """Receiver and rear hardware cannot become an opaque gate over visiting craft.

        @param self Re-signed excessive original-foreground fixture.
        """
        self.policy["foreground_pixels"] = copy.deepcopy(self.report["source"]["coupling"]["original_pixels"])
        self.family["approved_tile_policy"] = cuts.signature(self.policy)
        with self.assertRaisesRegex(ValueError, "differs from native geometry"):
            tiles.checked_policy(self.document)

    def test_missing_or_duplicate_stored_part_is_refused(self):
        """Coverage needs one unambiguous source for every real cell/part.

        @param self Deliberately incomplete or duplicate stored matrix fixture.
        """
        for pieces in (self.detached_parts()[:-1], self.detached_parts() + [self.detached_parts()[0]]):
            with self.assertRaisesRegex(ValueError, "missing or duplicate orbital native tile part"):
                self.rebuild(pieces)

    def test_stored_origin_or_physical_pose_cannot_drift_with_unchanged_pixels(self):
        """A valid local image checksum does not authorize another position or state.

        @param self Actual stored part metadata drift fixture.
        """
        # Each corruption keeps identical pixels and hashes, forcing refusal
        # to protect real native pose ownership instead of superficial content.
        for field in ("native_origin", "state", "direction"):
            pieces = self.detached_parts()
            if field == "native_origin":
                pieces[0][0][field][0] += 1
            else:
                pieces[0][0][field] = "invented"
            with self.assertRaisesRegex(ValueError, "stored tile geometry, pose or palette changed"):
                self.rebuild(pieces)

    def test_resigned_one_pixel_loss_is_detected_by_whole_original_hash(self):
        """Internally consistent local metadata cannot conceal a hole in the accepted deck.

        @param self Deliberately removed original pixel with accurate new local checksum.
        """
        pieces = self.detached_parts()
        row, image = pieces[0]
        offset = next(i for i, value in enumerate(image.get_flattened_data()) if value)
        image.putpixel((offset % 64, offset // 64), 0)
        row["opaque_pixels"] -= 1
        row["native_sha256"] = hashlib.sha256(image.tobytes()).hexdigest()
        with self.assertRaisesRegex(ValueError, "differ from accepted whole pixels"):
            self.rebuild(pieces)

    def test_swapped_back_front_pixels_cannot_invent_an_opaque_receiver_gate(self):
        """Lossless union alone cannot justify changing the physical drawing depth.

        @param self Re-signed swapped stored-depth images fixture.
        """
        pieces = self.detached_parts()
        back = next(i for i, (row, image) in enumerate(pieces) if row["tile_variant"] == 2 and row["part"] == "back")
        front = next(i for i, (row, image) in enumerate(pieces) if row["tile_variant"] == 2 and row["part"] == "front")
        pieces[back] = (pieces[back][0], pieces[front][1])
        pieces[front] = (pieces[front][0], self.parts[back][1].copy())
        for index in (back, front):
            row, image = pieces[index]
            row["opaque_pixels"] = sum(value != 0 for value in image.get_flattened_data())
            row["native_sha256"] = hashlib.sha256(image.tobytes()).hexdigest()
        with self.assertRaisesRegex(ValueError, "physical cell or foreground ownership"):
            self.rebuild(pieces)

    def test_foreground_mask_cannot_claim_an_absent_original_pixel(self):
        """Transparent storage is never evidence of additional physical foreground.

        @param self False foreground declaration over an unchanged accepted model.
        """
        mask = [*self.policy["foreground_pixels"], [0, 0]]
        with self.assertRaisesRegex(ValueError, "foreground policy claims absent original hardware"):
            self.rebuild(self.detached_parts(), mask)

    def test_changed_palette_cannot_preserve_a_false_native_image_identity(self):
        """Identical colour indexes cannot excuse a changed canonical DOS palette.

        @param self Changed actual stored image palette with unchanged indexed pixels.
        """
        pieces = self.detached_parts()
        palette = pieces[0][1].getpalette()
        palette[3] ^= 1
        pieces[0][1].putpalette(palette)
        with self.assertRaisesRegex(ValueError, "geometry, pose or palette changed"):
            self.rebuild(pieces)

    def test_bounded_cut_review_keeps_functional_and_runtime_coverage_pending(self):
        """Complete frame0 tile cutting cannot close the full64-cell state matrix.

        @param self Immutable bounded cut source-review fixture.
        """
        self.assertEqual({row["state"] for row, image in self.parts}, {"complete"})
        self.assertEqual({row["frame"] for row, image in self.parts}, {0})
        self.assertFalse(self.report["complete_source_coverage"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])


if __name__ == "__main__":
    unittest.main()
