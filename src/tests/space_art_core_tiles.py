"""Verify lossless Core source cuts and the original coupling's foreground ownership.

@file space_art_core_tiles.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13E
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group70.2
All native cells remain one fixed Core. Source cuts grant no berth, inventory,
work, utility or native runtime activation.
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

REVIEW = ROOT / "logs/proofs/mlm/native-art/space/core-tile-cuts-v1/tile-source-review.json"


class CoreTileTests(unittest.TestCase):
    """Protect original pixels, nine physical cells and the one real foreground cheek."""

    @classmethod
    def setUpClass(cls):
        """Read exact accepted source images and the actual stored cut atlas.

        @param cls Fixture sharing immutable input/cut pixels only.
        """
        cls.source = art.read_record()
        cls.public = json.loads(core.PUBLIC.read_text(encoding="utf-8"))
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        # Exact report identities bind the stored source pixels, not a filename
        # that a later edited image could inherit without a new source review.
        for artifact in cls.report["artifacts"]:
            assert art.identity(ROOT / artifact["path"]) == artifact
        atlas = Image.open(REVIEW.parent / "native-cuts.png")
        cls.parts = []
        for row in cls.report["sprites"]:
            x, y, width, height = row["box"]
            cls.parts.append((row, atlas.crop((x, y, x + width, y + height))))
        definition = cls.source["families"][core.ASSET]
        reference = definition["base_source_review"]
        assert art.identity(ROOT / reference["path"]) == reference
        base = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        cls.original = Image.open(ROOT / base["artifacts"][0]["path"])
        cls.coupling_pixels = {tuple(point) for point in base["base"]["coupling_pixels"]}

    def setUp(self):
        """Detach source policy before each deliberate refusal or part corruption.

        @param self Independent original Core tile-source fixture.
        """
        self.document = copy.deepcopy(self.source)
        self.family = self.document["families"][core.ASSET]
        self.policy = self.family["tile_policy"]

    def detached_parts(self):
        """Give a test editable metadata and pixel copies without altering accepted inputs.

        @param self Fixture holding the immutable published native cuts.
        @return Independent metadata/image pairs for all18native parts.
        """
        return [(copy.deepcopy(row), image.copy()) for row, image in self.parts]

    def resign(self):
        """Approve deliberately incorrect test metadata to exercise native invariants.

        @param self Detached policy fixture; source files remain untouched.
        """
        self.family["approved_tile_policy"] = tiles.signature(self.policy)

    def test_all18_parts_preserve_nine_real_world_cells_and_anchors(self):
        """Stored source parts retain fixed orientation, tile order and64x64 registration.

        @param self Complete actual native cell/part fixture.
        """
        self.assertEqual(len(self.parts), 18)
        self.assertEqual(self.report["logical_part_count"], 18)
        self.assertEqual(self.report["nonempty_part_count"], 10)
        self.assertEqual(self.report["structurally_absent_part_count"], 8)
        expected = {(tile, part) for tile in range(9) for part in ("back", "front")}
        self.assertEqual({(row["tile_variant"], row["part"]) for row, image in self.parts}, expected)
        # Each sprite names an actual x/y cell; an invented rotation or repeated
        # whole-platform image cannot satisfy the original native cut contract.
        for row, image in self.parts:
            tile = row["tile_variant"]
            self.assertEqual(row["world_cell"], [tile % 3, tile // 3])
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(row["direction"], "none")
            self.assertEqual(image.size, (64, 64))
            if row["part"] == "back":
                self.assertFalse(row["structurally_absent"])

    def test_independent_reassembly_keeps_every_original_pixel_exactly_once(self):
        """Stored pieces independently reproduce all accepted original pixels without overlap.

        @param self Complete source coverage and seam-continuity fixture.
        """
        rebuilt = Image.new("P", (256, 192), 0)
        observed = set()
        # Compute each origin from its actual world coordinate rather than
        # trusting the converter's origin metadata or reconstruction helper.
        for row, image in self.parts:
            wx, wy = row["world_cell"]
            ox, oy = 97 + 32 * (wy - wx), 31 + 16 * (wx + wy)
            for y in range(64):
                for x in range(64):
                    value = image.getpixel((x, y))
                    if value == 0:
                        continue
                    point = (ox + x, oy + y)
                    self.assertNotIn(point, observed)
                    observed.add(point)
                    rebuilt.putpixel(point, value)
        self.assertEqual(rebuilt.tobytes(), self.original.tobytes())
        self.assertEqual(len(observed), self.report["source_opaque_pixels"])

    def test_only_near_cheek_is_foreground_and_eight_fronts_are_absent(self):
        """The receiver cavity and rear cheek remain behind a visiting native hull.

        @param self Actual original coupling and foreground-mask fixture.
        """
        actual = set()
        # Read actual stored foreground pixels through their world-tile origin;
        # declared masks alone cannot prove that a cavity was left unobstructed.
        for row, image in self.parts:
            if row["part"] != "front":
                continue
            if row["structurally_absent"]:
                self.assertFalse(any(image.get_flattened_data()))
                continue
            self.assertEqual(row["world_cell"], [0, 2])
            ox, oy = row["native_origin"]
            for y in range(64):
                for x in range(64):
                    if image.getpixel((x, y)):
                        actual.add((ox + x, oy + y))
        self.assertEqual(len(actual), 20)
        self.assertEqual(actual, {tuple(point) for point in self.policy["foreground_pixels"]})
        self.assertTrue(actual.issubset(self.coupling_pixels))
        self.assertNotIn((209, 118), actual)
        self.assertNotIn((209, 116), actual)

    def test_direct_original_source_api_matches_every_published_cut(self):
        """Fresh original ancestry and conversion reproduce the stored part pixels exactly.

        @param self Direct source-consumer compatibility fixture.
        """
        fresh, evidence = tiles.cut_base(self.document, self.public)
        published = {(row["tile_variant"], row["part"]): image for row, image in self.parts}
        self.assertEqual(evidence["foreground_pixel_count"], 20)
        # Resolve one whole source build rather than replaying earlier complete
        # families; all18real parts must still match independent stored pixels.
        for row, image in fresh:
            self.assertEqual(image.tobytes(), published[row["tile_variant"], row["part"]].tobytes())

    def test_unsigned_policy_change_refuses_before_original_image_access(self):
        """A changed foreground owner cannot inherit the accepted policy signature.

        @param self Unsigned source policy corruption fixture.
        """
        self.policy["foreground_owner"] = "invented berth foreground"
        with patch.object(art, "source_candidate", side_effect=AssertionError("unexpected image source access")):
            with self.assertRaisesRegex(ValueError, "approved Core tile policy changed"):
                tiles.cut_base(self.document)

    def test_new_signature_cannot_shift_or_rotate_world_tile_geometry(self):
        """Native geometry outranks newly signed source-local tile metadata.

        @param self Deliberate fixed-projection source drift fixture.
        """
        mutations = (("anchor", [-32, -32]), ("direction", "N"),
                     ("world_cells", self.policy["world_cells"][:-1]),
                     ("native_cell_origins", [[0, 0]] * 9))
        # Start from the original for each independent geometry defect so one
        # invalid field cannot mask acceptance of another changed native owner.
        for key, value in mutations:
            self.document = copy.deepcopy(self.source)
            self.family = self.document["families"][core.ASSET]
            self.policy = self.family["tile_policy"]
            self.policy[key] = value
            self.resign()
            with self.subTest(field=key):
                with patch.object(art, "source_candidate", side_effect=AssertionError("unexpected source access")):
                    with self.assertRaisesRegex(ValueError, "differs from actual native geometry"):
                        tiles.cut_base(self.document)

    def test_new_signature_cannot_enlarge_foreground_over_receiver_cavity(self):
        """The whole original coupling cannot become an opaque foreground gate.

        @param self Re-signed excessive physical-foreground fixture.
        """
        self.policy["foreground_pixels"] = [list(point) for point in sorted(self.coupling_pixels)]
        self.resign()
        with patch.object(art, "source_candidate", side_effect=AssertionError("unexpected source access")):
            with self.assertRaisesRegex(ValueError, "differs from actual native geometry"):
                tiles.cut_base(self.document)

    def test_missing_or_duplicate_stored_part_refuses(self):
        """A complete tile/source matrix cannot omit a real part or silently select a duplicate.

        @param self Actual stored cut-coverage corruption fixture.
        """
        # Both incomplete and ambiguous matrices must fail before any source
        # can count its duplicated or missing world cell as delivered coverage.
        for pieces in (self.detached_parts()[:-1], self.detached_parts() + [self.detached_parts()[0]]):
            with self.assertRaisesRegex(ValueError, "missing or duplicate Core native tile part"):
                tiles.reassemble(pieces, self.policy)

    def test_moved_stored_tile_refuses_even_with_unchanged_pixels(self):
        """A source-image checksum does not authorize moving its native world contact.

        @param self Shifted stored origin metadata fixture.
        """
        pieces = self.detached_parts()
        pieces[0][0]["native_origin"][0] += 1
        with self.assertRaisesRegex(ValueError, "Core stored tile geometry or palette changed"):
            tiles.reassemble(pieces, self.policy)

    def test_resigned_missing_pixel_is_detected_by_whole_reconstruction(self):
        """Locally plausible metadata cannot hide a one-pixel gap in the original station.

        @param self Missing original pixel with internally consistent stored-part metadata.
        """
        pieces = self.detached_parts()
        row, image = pieces[0]
        offset = next(i for i, value in enumerate(image.get_flattened_data()) if value != 0)
        image.putpixel((offset % 64, offset // 64), 0)
        row["opaque_pixels"] -= 1
        row["native_sha256"] = hashlib.sha256(image.tobytes()).hexdigest()
        with self.assertRaisesRegex(ValueError, "Core cuts differ from accepted whole pixels"):
            tiles.reassemble(pieces, self.policy)

    def test_back_front_swap_cannot_preserve_false_depth_ownership(self):
        """A lossless union alone cannot authorize drawing the platform over a craft.

        @param self Swapped actual coupling-cell part images with consistent local hashes.
        """
        pieces = self.detached_parts()
        back = next(i for i, (row, image) in enumerate(pieces) if row["tile_variant"] == 6 and row["part"] == "back")
        front = next(i for i, (row, image) in enumerate(pieces) if row["tile_variant"] == 6 and row["part"] == "front")
        pieces[back] = (pieces[back][0], pieces[front][1])
        pieces[front] = (pieces[front][0], self.parts[back][1].copy())
        # Preserve accurate local checksums/counts so the refusal specifically
        # tests physical depth ownership rather than a superficial hash error.
        for index in (back, front):
            row, image = pieces[index]
            row["opaque_pixels"] = sum(value != 0 for value in image.get_flattened_data())
            row["native_sha256"] = hashlib.sha256(image.tobytes()).hexdigest()
        with self.assertRaisesRegex(ValueError, "change physical cell or foreground ownership"):
            tiles.reassemble(pieces, self.policy)

    def test_immutable_body_bytes_are_still_checked_by_cut_source_api(self):
        """A cached review cannot conceal a replacement original body file.

        @param self Mocked changed original identity fixture; real images stay untouched.
        """
        candidate = next(c for c in self.document["candidates"] if c["key"] == self.family["base_selection"])
        target = (ROOT / candidate["path"]).resolve()
        identity = art.identity

        def changed_identity(path):
            """Model changed original bytes while preserving real source files.

            @param path Concrete source/receipt path requested by source validation.
            @return Actual identity except for the deliberate body-hash mismatch.
            """
            result = identity(path)
            if path.resolve() == target:
                result["sha256"] = "0" * 64
            return result

        with patch.object(art, "identity", side_effect=changed_identity):
            with self.assertRaisesRegex(ValueError, "immutable Space source ancestry changed"):
                tiles.cut_base(self.document)

    def test_cut_checkpoint_retains_pending_functional_and_runtime_gates(self):
        """One complete-state source cut does not deliver the full144-cell state matrix.

        @param self Immutable bounded-source handoff fixture.
        """
        self.assertEqual({row["state"] for row, image in self.parts}, {"complete"})
        self.assertEqual({row["frame"] for row, image in self.parts}, {0})
        self.assertFalse(self.report["complete_source_coverage"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])


if __name__ == "__main__":
    unittest.main()
