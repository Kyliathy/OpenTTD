"""Verify explicit native asteroid size/source geometry before authoring natural bodies.

@file space_art_asteroid_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BF
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group67
Native catalogue tests supply saved diameter/size authority; this fixture checks actual projection containment and source-contract refusals without generating resource/art state.
"""
import json
import math
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import multilayer_sprite_contract as contract


class AsteroidSourceContractTests(unittest.TestCase):
    """Require saved size variants, sufficient native canvas and honest source-frame coverage."""

    def setUp(self):
        """Read detached current public metadata for each independent refusal case."""
        self.document = json.loads(contract.MANIFEST.read_text(encoding="utf-8"))
        self.row = next(row for row in self.document["assets"] if row["asset_id"] == "asteroid.nickel")

    def test_saved_sizes_fit_actual_isometric_projection(self):
        """The2tile circular visual diameter projects wider than the old64pixel draft and fits the explicit96pixel source."""
        self.assertEqual(self.row["size_variants"], ["small", "medium", "large"])
        self.assertEqual(self.row["visual_diameters_q16"], [32768, 65536, 131072])
        # Native RemapCoords has screenX=2*(worldY-worldX). Its diagonal projection of a circular tile diameter needs sqrt(2)*2*TILE_SIZE pixels, plus declared clear canvas padding.
        widths = [math.sqrt(2) * 2 * 16 * diameter / 65536 for diameter in self.row["visual_diameters_q16"]]
        self.assertGreater(widths[-1], 64)
        self.assertLessEqual(math.ceil(widths[-1]) + 4, self.row["native_size"][0])
        self.assertAlmostEqual(widths[-1] / widths[0], 4)
        self.assertEqual(self.row["anchor"], [-48, -48])

    def test_actual_public_counts_include_every_saved_size(self):
        """The three natural-body rows require1152poses in addition to416existing craft/pod poses."""
        assets = ["vehicle.model-15", "vehicle.model-19", "vehicle.model-20", "vehicle.model-18",
                  "asteroid.nickel", "asteroid.ice", "asteroid.rare-earth"]
        self.assertEqual(contract.validate_manifest(self.document, assets), 1568)

    def test_missing_medium_variant_refuses(self):
        """A source matrix cannot hide an undelivered middle size behind the common body row."""
        self.row["size_variants"].pop(1)
        with self.assertRaisesRegex(ValueError, "size"):
            contract.validate_asteroid_contract(self.row)

    def test_generic_vehicle_canvas_cannot_clip_the_large_body(self):
        """Restoring the old planned64x48rectangle remains invalid even if its bounds are changed to agree."""
        self.row["native_size"] = [64, 48]
        self.row["bounds"] = [0, 0, 64, 48]
        with self.assertRaisesRegex(ValueError, "canvas"):
            contract.validate_asteroid_contract(self.row)

    def test_shifted_native_reference_refuses(self):
        """A different world anchor cannot conceal body clipping or move its original lease position."""
        self.row["anchor"][0] += 1
        with self.assertRaisesRegex(ValueError, "contract"):
            contract.validate_asteroid_contract(self.row)

    def test_invented_angular_spin_refuses(self):
        """Static body source cannot claim four real spin phases when the native field owns no angular motion."""
        self.row["source_frame_contract"]["free"] = [0, 1, 2, 3]
        with self.assertRaisesRegex(ValueError, "alias"):
            contract.validate_asteroid_contract(self.row)

    def test_natural_body_cannot_gain_company_recoloring(self):
        """Company-independent composition remains natural source art, without a fabricated owner color mask."""
        self.row["company_colors"] = "explicit_mask"
        with self.assertRaisesRegex(ValueError, "contract"):
            contract.validate_asteroid_contract(self.row)

    def test_default_craft_source_bytes_remain_identical(self):
        """Widening only natural-body support preserves an actual existing miner's indexed native publication."""
        document = art.read_record()
        definition = document["families"]["vehicle.model-15"]
        pose, _ = art.native_view(definition["selections"]["free/N"], "N", document)
        report = json.loads((art.ROOT / definition["complete_source_review"]["path"]).read_text())
        row = next(r for r in report["atlases"][0]["sprites"] if (r["state"], r["frame"], r["direction"]) == ("free", 0, "N"))
        atlas = art.Image.open(art.ROOT / report["atlases"][0]["path"])
        x, y, width, height = row["box"]
        self.assertEqual(pose.tobytes(), atlas.crop((x, y, x + width, y + height)).tobytes())
        self.assertEqual(pose.size, (64, 48))

    def test_overbroad_visible_envelope_cannot_bypass_native_clipping(self):
        """A caller cannot authorize out-of-canvas artwork by widening only its visible-envelope metadata."""
        document = art.read_record()
        definition = document["families"]["vehicle.model-15"]
        candidate = art.source_candidate(definition["selections"]["free/N"], document)
        with self.assertRaisesRegex(ValueError, "envelope"):
            art.source_pixels(candidate, "N", [-1, 0, 65, 48])


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
