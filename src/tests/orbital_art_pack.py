"""Verify protected orbital packing, exact crop anchors and honest absent parts.

@file orbital_art_pack.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K.1
These source fixtures grant no native decoded loading, gameplay state or runtime acceptance.
"""
import copy
import hashlib
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import pack_orbital_art as art


class OrbitalPackingTests(unittest.TestCase):
    """Protect actual source pixels and previous ranges across the ten-family append."""

    @classmethod
    def setUpClass(cls):
        """Derive two clean outputs and retain actual accepted source manifests.

        @param cls Fixture sharing read-only source and deterministic packing observations.
        """
        cls.public = json.loads(art.PUBLIC.read_text(encoding="utf-8"))
        cls.scope = art.read_scope()
        cls.report, cls.files = art.derive(cls.public)
        cls.second_report, cls.second_files = art.derive(cls.public)
        cls.families = {row["asset_id"]: row for row in cls.public["assets"]}
        cls.module = cls.families["module.processor"]
        cls.native = cls.scope["source_atlases"]["module.processor"]
        cls.module_report = json.loads((art.ROOT / cls.scope["source_reviews"]["module.processor"]["path"]).read_text())

    def setUp(self):
        """Detach metadata before each focused deliberate corruption.

        @param self Current independent source-contract refusal fixture.
        """
        self.document = copy.deepcopy(self.public)
        self.family = next(row for row in self.document["assets"] if row["asset_id"] == "module.processor")
        self.detached = copy.deepcopy(self.module_report)

    def test_two_clean_conversions_and_staged_bytes_are_exact(self):
        """Independent conversions reproduce every report, sheet, NFO and range byte.

        @param self Actual clean-derived and staged source output fixture.
        """
        self.assertEqual(self.report, self.second_report)
        self.assertEqual(self.files, self.second_files)
        for path, data in self.files.items():
            self.assertEqual(path.read_bytes(), data)
        art.read_scope()

    def test_all976_native_ids_follow_the_frozen_previous_prefix(self):
        """All ten actual families append without reclaiming or moving earlier IDs.

        @param self Complete deterministic native range fixture.
        """
        self.assertEqual(self.report["pose_count"], 976)
        self.assertEqual((self.report["native_start"], self.report["native_sprite_bound"]), (5167, 6143))
        self.assertEqual([row["native_offset"] for row in self.report["sprites"]], list(range(5167, 6143)))
        self.assertEqual([row["native_offset"] for row in self.report["families"]],
                         [5167, 5295, 5375, 5535, 5679, 5743, 5807, 5871, 5935, 5999])
        self.assertEqual([row["asset_id"] for row in self.report["families"]], list(art.ASSETS))
        self.assertIsNone(self.report["packed_grf"])
        self.assertFalse(self.report["runtime_activated"])

    def test_every_packed_crop_reconstructs_original_pixels_and_world_anchor(self):
        """All976 actual poses round-trip without shifted hooks, holes or foregrounds.

        @param self Packed sheet and exact immutable native source images.
        """
        packed = Image.open(io.BytesIO(self.files[art.ROOT / self.scope["png"]]))
        originals = {}
        # Reconstruct each declared original cell using only packed crop and
        # independent anchor arithmetic. Internal holes and empty parts remain exact.
        for row in self.report["sprites"]:
            if row["source_atlas"] not in originals:
                originals[row["source_atlas"]] = Image.open(art.ROOT / row["source_atlas"])
            x, y, width, height = row["box"]
            crop = packed.crop((x, y, x + width, y + height))
            self.assertEqual(hashlib.sha256(crop.tobytes()).hexdigest(), row["pixel_sha256"])
            family = self.families[row["asset_id"]]
            restored = Image.new("P", tuple(family["native_size"]), 0)
            shift = tuple(a - b for a, b in zip(row["anchor"], row["source_anchor"]))
            restored.paste(crop, shift)
            sx, sy, sw, sh = row["source_box"]
            expected = originals[row["source_atlas"]].crop((sx, sy, sx + sw, sy + sh))
            self.assertEqual(restored.tobytes(), expected.tobytes(), row["native_offset"])

    def test_absent_fronts_are_exact_transparent_parts_with_original_anchor(self):
        """Honest absence never becomes a fabricated clamp or an omitted native ID.

        @param self Staged empty-front source and crop geometry fixture.
        """
        absent = [row for row in self.report["sprites"] if row["structurally_absent"]]
        self.assertGreater(len(absent), 100)
        # The shared cropper deliberately retains the original empty cell;
        # its blank pixels and world anchor still have a complete native slot.
        for row in absent:
            self.assertEqual(row["part"], "front")
            self.assertEqual(row["box"][2:], [64, 64])
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(row["pixel_sha256"], hashlib.sha256(bytes(4096)).hexdigest())

    def test_native_nfo_and_header_use_only_the_new_append(self):
        """Generated declarations preserve explicit domains and point to the own new sheet.

        @param self Exact NFO and generated C++ range source fixture.
        """
        nfo = self.files[art.ROOT / self.scope["nfo"]].decode()
        header = self.files[art.ROOT / self.scope["header"]].decode()
        self.assertEqual(nfo.count("sprites/orbital_final.png"), 976)
        self.assertNotIn("sprites/map_layers_m1.png", nfo)
        self.assertIn("ORBITAL_SPRITE_START = 5167", header)
        self.assertIn("ORBITAL_SPRITE_END = 6143", header)
        self.assertIn("uint8_t parts;", header)

    def test_missing_or_duplicate_public_family_refuses_before_source_reads(self):
        """A dictionary overwrite cannot conceal a duplicated or absent original role.

        @param self Detached public family-identity corruption fixture.
        """
        # Each public defect is independent; a failed missing-family case must
        # not mask accidental acceptance of a duplicate row on the next attempt.
        for missing in (True, False):
            document = copy.deepcopy(self.public)
            if missing:
                document["assets"].remove(next(row for row in document["assets"] if row["asset_id"] == "module.processor"))
            else:
                document["assets"].append(copy.deepcopy(self.module))
            with self.assertRaisesRegex(ValueError, "missing or duplicate orbital"):
                art.derive(document)

    def test_rotated_or_shrunken_native_footprint_is_refused(self):
        """New packing approval cannot reinterpret a fixed3x3 saved processor.

        @param self Detached actual-native geometry corruption fixture.
        """
        self.family["directions"] = ["NE", "SE", "SW", "NW"]
        with self.assertRaisesRegex(ValueError, "native orbital geometry"):
            art.derive(self.document)

    def test_changed_native_offset_cannot_move_a_staged_family(self):
        """Repeated packing preserves the exact native slot and definition-local MLVI owner.

        @param self Detached staged native-offset corruption fixture.
        """
        self.family["packed"]["native_offset"] += 1
        with self.assertRaisesRegex(ValueError, "native binding moved"):
            art.derive(self.document)

    def test_missing_source_part_cannot_claim_full_coverage(self):
        """A true completion flag cannot supply an omitted source rectangle.

        @param self Detached144part source manifest missing one actual part.
        """
        self.detached["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "incomplete module source identity"):
            art.module_poses(self.module, self.detached, self.native, {})

    def test_definition_local_binding_and_native_route_remain_owned(self):
        """Staging cannot rewrite a content-owned MLVI binding or claim a foreign payload.

        @param self Detached content-binding and native-route corruption fixture.
        """
        # Test each ownership defect independently: rejecting a moved local
        # offset must not conceal acceptance of a foreign route or payload next.
        for key, value in (("offset", 123), ("sha256", "0" * 64),
                           ("route", "foreign_payload"), ("source_payload", "foreign.json")):
            document = copy.deepcopy(self.public)
            family = next(row for row in document["assets"] if row["asset_id"] == "module.processor")
            family["packed"][key] = value
            with self.assertRaisesRegex(ValueError, "native binding moved"):
                art.derive(document)

    def test_changed_world_origin_or_anchor_refuses_identical_source_pixels(self):
        """A local pixel checksum cannot move a paid footprint or physical sorting cell.

        @param self Detached original world-contact corruption fixture.
        """
        self.detached["sprites"][0]["native_origin"][0] += 1
        with self.assertRaisesRegex(ValueError, "cell geometry"):
            art.module_poses(self.module, self.detached, self.native, {})

    def test_false_physical_absence_cannot_hide_real_hardware(self):
        """An empty-front declaration must agree with actual indexed original pixels.

        @param self Deliberately false absence metadata over an unchanged real back.
        """
        self.detached["sprites"][0]["structurally_absent"] = True
        with self.assertRaisesRegex(ValueError, "false physical absence"):
            art.module_poses(self.module, self.detached, self.native, {})

    def test_changed_cell_hash_is_refused_before_cropping(self):
        """A plausible rectangle must still identify its exact approved original bytes.

        @param self Detached source-cell identity corruption fixture.
        """
        self.detached["sprites"][0]["native_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "module source pixels"):
            art.module_poses(self.module, self.detached, self.native, {})

    def test_false_alias_cannot_replace_a_distinct_physical_stage(self):
        """An explicit still alias must equal its referenced original part exactly.

        @param self Detached blueprint-to-complete false alias fixture.
        """
        row = self.detached["sprites"][0]
        row["alias_of"] = dict(state="complete", frame=0, direction="none", tile_variant=0, part="back")
        with self.assertRaisesRegex(ValueError, "false module source alias"):
            art.module_poses(self.module, self.detached, self.native, {})

    def test_changed_palette_is_refused_without_touching_actual_art(self):
        """Identical indexes cannot inherit another DOS palette's colour meanings.

        @param self Mocked altered source-atlas palette fixture.
        """
        image = Image.open(art.ROOT / self.native["path"]).copy()
        palette = image.getpalette()
        palette[3] ^= 1
        image.putpalette(palette)
        with patch.object(art.Image, "open", return_value=image):
            with self.assertRaisesRegex(ValueError, "palette/transparency mismatch"):
                art.module_poses(self.module, self.detached, self.native, {})

    def test_changed_original_prompt_receipt_refuses_native_packing(self):
        """Immutable selected ancestry remains an entry gate after source approval.

        @param self Mocked receipt mismatch preserving every real project file.
        """
        target = (art.ROOT / self.scope["source_candidates"]["vehicle.model-21"][0]["generation_receipt"]["path"]).resolve()
        original_identity = art.source.identity

        def changed_identity(path):
            """Simulate a changed exact prompt receipt without modifying the actual source.

            @param path Original requested source, receipt, reference or protected-output path.
            @return Actual identity except for the intentionally corrupted receipt hash.
            """
            result = original_identity(path)
            if path.resolve() == target:
                result["sha256"] = "0" * 64
            return result

        with patch.object(art.source, "identity", side_effect=changed_identity):
            with self.assertRaisesRegex(ValueError, "immutable Space source ancestry changed"):
                art.derive(self.document)


if __name__ == "__main__":
    unittest.main()
