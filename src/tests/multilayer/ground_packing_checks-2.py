"""Independently compare clean conversions and reconstruct every packed ground source.

@file ground_packing_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Two separate-process outputs are retained under test-cache/ground-pack/conversion-a and -b.
"""

import hashlib
import json
import unittest

from PIL import Image

from ground_pack_check_support import ROOT, handoff, run_checks


class GroundPackingChecks(unittest.TestCase):
    """Qualify exact indices, anchors, masks, aliases, IDs and predecessor preservation."""

    @classmethod
    def setUpClass(cls):
        """Load the two independently emitted conversions and current immutable baseline.

        @param cls Active append-input qualification suite.
        @return None.
        """
        cls.scope, cls.before, public = handoff.read_scope()
        cls.families = {row["asset_id"]: row for row in public["assets"]}
        cls.first = ROOT / "test-cache/ground-pack/conversion-a"
        cls.second = ROOT / "test-cache/ground-pack/conversion-b"
        cls.report = json.loads((cls.first / "packed-ground-manifest.json").read_text(encoding="utf-8"))
        cls.sheet = Image.open(cls.first / "ground_final.png")

    def test_clean_process_outputs_match_and_bind_exact_current_sources(self):
        """Both clean conversions must match bytes, scope, source audit and all three output hashes.

        @param self Active determinism/provenance check.
        @return None.
        """
        for name in ("ground_final.png", "ground_final.nfo", "ground_sprite_assets.h", "packed-ground-manifest.json"):
            self.assertEqual((self.first / name).read_bytes(), (self.second / name).read_bytes(), name)
        self.assertEqual(self.report["scope"]["sha256"], handoff.digest(handoff.SCOPE))
        self.assertEqual(self.report["source_audit"], self.scope["source_audit"])
        for row in self.report["outputs"]:
            self.assertEqual(handoff.digest(self.first / handoff.bound(row["path"]).name), row["sha256"])
        self.assertIsNone(self.report["packed_grf"])
        self.assertFalse(self.report["runtime_activated"])

    def test_ranges_are_complete_contiguous_and_follow_every_published_family(self):
        """All14,254 canonical poses follow the62-family prefix without omitted aliases or parents.

        @param self Active native-identity check.
        @return None.
        """
        self.assertEqual([row["asset_id"] for row in self.report["families"]], self.scope["asset_ids"])
        self.assertEqual([row["native_offset"] for row in self.report["sprites"]], list(range(7791, 22045)))
        self.assertEqual(self.report["preserved_family_count"], 62)
        # Canonical tuple ordering is the selector ABI. Exact equality catches
        # missing static aliases, transposed cell/part axes or phase reordering.
        for family in self.report["families"]:
            rows = [row for row in self.report["sprites"] if row["asset_id"] == family["asset_id"]]
            keys = [(row["state"], row["frame"], row["direction"], row["tile_variant"], row["part"]) for row in rows]
            self.assertEqual(keys, handoff.packing.required_keys(self.families[family["asset_id"]]))
            self.assertEqual(len(rows), family["sprite_count"])

    def test_every_crop_reconstructs_the_original_native_pixels_and_company_mask(self):
        """All14,254 cropped sprites preserve original registration, internal holes and remap pixels.

        @param self Active independent packed-sheet decoder.
        @return None.
        """
        originals = {}
        # Reconstruct full native cells using only output rectangles and the
        # adjusted world anchors. No packer crop helper participates in this proof.
        for row in self.report["sprites"]:
            path = row["source_atlas"]
            if path not in originals:
                originals[path] = Image.open(handoff.bound(path))
            x, y, width, height = row["box"]
            actual = self.sheet.crop((x, y, x + width, y + height))
            self.assertEqual(hashlib.sha256(actual.tobytes()).hexdigest(), row["pixel_sha256"])
            restored = Image.new("P", tuple(self.families[row["asset_id"]]["native_size"]), 0)
            restored.paste(actual, (row["anchor"][0] - row["source_anchor"][0], row["anchor"][1] - row["source_anchor"][1]))
            sx, sy, sw, sh = row["source_box"]
            expected = originals[path].crop((sx, sy, sx + sw, sy + sh))
            self.assertEqual(restored.tobytes(), expected.tobytes(), row["native_offset"])
            self.assertEqual(hashlib.sha256(expected.tobytes()).hexdigest(), row["source_pixels_sha256"])
            mask = bytes(int(value in handoff.COMPANY_INDICES) for value in restored.get_flattened_data())
            self.assertEqual(hashlib.sha256(mask).hexdigest(), row["company_color_mask_sha256"])
            self.assertEqual(sum(mask), row["company_color_pixels"])

    def test_palette_and_approved_transparent_slots_keep_exact_native_canvases(self):
        """No palette conversion or one-pixel empty sentinel may change the physical source contract.

        @param self Active whole-sheet and explicit-absence check.
        @return None.
        """
        self.assertEqual(self.sheet.mode, "P")
        self.assertEqual(self.sheet.info["transparency"], 0)
        self.assertEqual(self.sheet.getpalette(), handoff.packing.base.PALETTE_BYTES)
        self.assertFalse(set(self.sheet.get_flattened_data()) & (handoff.packing.base.FORBIDDEN - {0} - handoff.COMPANY_INDICES))
        absent = [row for row in self.report["sprites"] if row["structurally_absent"]]
        self.assertEqual(len(absent), 228)
        self.assertEqual(sum(row["company_color_pixels"] for row in self.report["sprites"]), 4082)
        for row in absent:
            self.assertEqual(row["box"][2:], self.families[row["asset_id"]]["native_size"])
            self.assertEqual(row["anchor"], row["source_anchor"])

    def test_nfo_matches_every_rectangle_anchor_and_action5_range(self):
        """The actual GRF input uses the same native IDs and crop registration as its source receipt.

        @param self Active independent NFO text decoder.
        @return None.
        """
        lines = (self.first / "ground_final.nfo").read_text(encoding="utf-8").splitlines()
        blocks = [line for line in lines if "* 0 05 " in line]
        self.assertEqual(len(blocks), 1)
        self.assertIn("05 9C FF AE 37 FF 6F 1E", blocks[0])
        sprites = [line.split() for line in lines if "sprites/ground_final.png 8bpp" in line]
        self.assertEqual(len(sprites), 14254)
        for words, row in zip(sprites, self.report["sprites"]):
            self.assertEqual(list(map(int, words[3:9])), row["box"] + row["anchor"])

    def test_all2145_frozen_files_and_actual_predecessor_payload_remain_exact(self):
        """New conversion inputs cannot rewrite any existing source, Space/launch asset or application.

        @param self Active current complete-prefix preservation check.
        @return None.
        """
        self.assertEqual(len(self.before["files"]), 2145)
        handoff.validate_preserved_files(self.scope, self.before)


if __name__ == "__main__":
    run_checks(GroundPackingChecks)
