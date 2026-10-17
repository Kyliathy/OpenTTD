"""Focused genuine Space original, registration and crop refusal checks.

@file space_art_source.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63
Malformed detached receipts test real source refusal; they never replace original pixels or install gameplay state.
"""
import copy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art

KEY = "small-miner-free-n-ne-e-se-v1"


class SpaceOriginalTests(unittest.TestCase):
    """Protect eight genuine base headings while later actual state/phase coverage remains explicitly incomplete."""

    def setUp(self):
        """Read a fresh detached provenance owner so every refusal leaves the actual retained record unchanged."""
        self.document = art.read_record()
        self.candidate = next(row for row in self.document["candidates"] if row["key"] == KEY)

    def test_genuine_views(self):
        """Actual eight source rectangles yield nonempty permitted-palette native pixels with the same fixed world reference."""
        definitions = self.document["families"]["vehicle.model-15"]
        # Every heading resolves to its own genuine retained source window; another view cannot substitute for an absent physical direction.
        for direction in definitions["directions"]:
            selected = definitions["selections"]["free/" + direction]
            pixels, evidence = art.native_view(selected, direction, self.document)
            self.assertEqual(pixels.size, (64, 48))
            self.assertTrue(any(pixels.get_flattened_data()))
            self.assertEqual(evidence["direction"], direction)
            self.assertTrue(set(pixels.get_flattened_data()).isdisjoint(set(art.base.FORBIDDEN) - {0}))
        self.assertFalse(self.document["families"]["vehicle.model-15"]["runtime_activated"])

    def test_original_hash_refuses(self):
        """A changed original cannot silently replace approved body pixels."""
        self.candidate["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            art.native_view(KEY, "N", self.document)

    def test_receipt_hash_refuses(self):
        """An altered executed-prompt receipt cannot authorize another source history."""
        self.candidate["generation_receipt"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            art.native_view(KEY, "N", self.document)

    def test_missing_genuine_heading_refuses(self):
        """A converter cannot rotate the present north view into an unauthored south view."""
        with self.assertRaisesRegex(ValueError, "genuine"):
            art.native_view(KEY, "S", self.document)

    def test_approved_registration_refuses(self):
        """Changing the frozen physical reference cannot shift a later work-state hull."""
        self.candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            art.native_view(KEY, "N", self.document)

    def test_candidate_crop_refuses_clipped_appendage(self):
        """Even an unapproved candidate must reject a new crop that removes a genuine solar paddle."""
        self.document["families"]["vehicle.model-15"].pop("approved_base_registration")
        self.candidate["native_transforms"]["N"]["crop_box"][0] = 250
        with self.assertRaisesRegex(ValueError, "measured"):
            art.native_view(KEY, "N", self.document)

    def test_candidate_scale_refuses(self):
        """A nonpositive physical scale cannot manufacture an empty native pose."""
        self.document["families"]["vehicle.model-15"].pop("approved_base_registration")
        self.candidate["native_transforms"]["N"]["uniform_scale"][0] = 0
        with self.assertRaisesRegex(ValueError, "scale"):
            art.native_view(KEY, "N", self.document)

    def test_native_clipping_refuses(self):
        """Canvas pasting must refuse visible hardware outside its declared native envelope."""
        self.document["families"]["vehicle.model-15"].pop("approved_base_registration")
        self.candidate["native_transforms"]["N"]["source_reference"][0] += 1000
        with self.assertRaisesRegex(ValueError, "envelope"):
            art.native_view(KEY, "N", self.document)

    def test_duplicate_candidate_refuses(self):
        """Ambiguous original selection cannot pick a convenient duplicate key."""
        self.document["candidates"].append(copy.deepcopy(self.candidate))
        with self.assertRaisesRegex(ValueError, "duplicate"):
            art.native_view(KEY, "N", self.document)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
