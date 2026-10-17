"""Exercise actual Water-Ice originals, physical fissure edits and saved-size registration.

@file space_art_ice.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BFB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group68
Only retained own carbonaceous originals supply pixels; these tests grant no native state or runtime art acceptance.
"""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_asteroids as asteroids


class WaterIceOriginalTests(unittest.TestCase):
    """Check the distinct flat carbonaceous body and its own original ice-surface states."""

    @classmethod
    def setUpClass(cls):
        """Convert the active family's actual original matrix once within this focused file.

        @param cls Test class retaining exact source inputs and bounded conversion caches.
        """
        cls.document = art.read_record()
        cls.asset = "asteroid.ice"
        cls.sources = asteroids.AsteroidSources(cls.asset, cls.document)
        cls.bodies, cls.surfaces = {}, {}
        # Reuse this file's concrete source outputs to inspect all actual sizes/states, without repeatedly converting completed metallic or craft families.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                cls.bodies[size, direction] = cls.sources.body(direction, size)[0]
                for state, phase in (("mining", 0), ("mining", 1), ("depleted", 0)):
                    cls.surfaces[size, direction, state, phase] = cls.sources.surface(direction, size, state, phase)

    def test_genuine_large_views_match_reviewed_base_pixels(self):
        """Retain exact original slab/ice/pit pixels while adding sizes and local work states."""
        with art.Image.open(art.ROOT / "logs/proofs/mlm/native-art/space/carbonaceous-eight-free-v1/native.png") as atlas:
            # Eight genuine body views stay equal to the accepted base contact, rather than becoming rotations or recolors of a different asteroid.
            for index, direction in enumerate(asteroids.DIRECTIONS):
                expected = atlas.crop((index * 96, 0, (index + 1) * 96, 80))
                self.assertEqual(expected.tobytes(), self.bodies["large", direction].tobytes())

    def test_three_saved_sizes_preserve_slab_proportions(self):
        """Keep native half/quarter diameters despite the carbonaceous source's own3/20 scale."""
        # Integer raster rounding may alter an edge slightly; neither axis can stretch independently or retain an identical Large body in the smaller native size slots.
        for direction in asteroids.DIRECTIONS:
            large = self.bodies["large", direction].getbbox()
            for size, divisor in (("small", 4), ("medium", 2)):
                small = self.bodies[size, direction].getbbox()
                for low, high in ((0, 2), (1, 3)):
                    self.assertLessEqual(abs((small[high] - small[low]) * divisor - (large[high] - large[low])), divisor * 2)

    def test_all_72_surfaces_preserve_original_stone_and_opacity(self):
        """Compare actual outside-fissure bytes and silhouette instead of protection counters."""
        # A worked ice lens must not redraw the natural slab, move the pit, cut a new contour or erase the original body through transparent part margins.
        for (size, direction, state, phase), (result, evidence) in self.surfaces.items():
            before = self.bodies[size, direction].tobytes()
            after = result.tobytes()
            mask = {tuple(point) for point in evidence["physical_mask"]}
            self.assertEqual([value != 0 for value in before], [value != 0 for value in after])
            self.assertTrue(all(old == new for index, (old, new) in enumerate(zip(before, after))
                                if (index % 96, index // 96) not in mask), (size, direction, state, phase))
            self.assertFalse((set(after) - {0}) & art.base.FORBIDDEN)

    def test_every_size_retains_two_work_glints_and_fully_worked_bright_ice(self):
        """Require real phase differences and replace all original bright ice, including Small."""
        # Front/rear light phases remain different at every actual saved size; aliases cannot hide a missing native cue or preserve pristine ice after authorized exhaustion.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                body = self.bodies[size, direction]
                a = self.surfaces[size, direction, "mining", 0][0]
                b = self.surfaces[size, direction, "mining", 1][0]
                worked = self.surfaces[size, direction, "depleted", 0][0]
                self.assertEqual(len({body.tobytes(), a.tobytes(), b.tobytes(), worked.tobytes()}), 4)
                original_rgb, worked_rgb = body.convert("RGB"), worked.convert("RGB")
                for index, value in enumerate(body.tobytes()):
                    point = (index % 96, index // 96)
                    if value != 0 and min(original_rgb.getpixel(point)) >= 205:
                        self.assertLess(min(worked_rgb.getpixel(point)), 205, (size, direction, point))

    def test_static_body_attachment_and_two_actual_phase_aliases(self):
        """The body gains no extra native probe, angular motion or independent work clock."""
        # Original attachment leaves the body unchanged. Four logical mining frames reference two real originals while every depleted frame holds the same worked surface.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                for frame in range(4):
                    self.assertEqual(self.sources.pose(direction, size, "attached", frame)[0].tobytes(),
                                     self.bodies[size, direction].tobytes())
                    self.assertEqual(self.sources.pose(direction, size, "mining", frame)[0].tobytes(),
                                     self.surfaces[size, direction, "mining", frame % 2][0].tobytes())
                    self.assertEqual(self.sources.pose(direction, size, "depleted", frame)[0].tobytes(),
                                     self.surfaces[size, direction, "depleted", 0][0].tobytes())

    def test_small_west_worked_surface_keeps_complete_original_sampling_grid(self):
        """Prevent a separately rounded part crop from leaving the identified Small ice corner."""
        family = self.document["families"][self.asset]
        candidate = art.source_candidate(family["surface_sources"]["depleted"]["0"]["W"], self.document)
        self.assertNotIn("source_part_box", candidate["native_transforms"]["W"])
        self.assertGreaterEqual(min(self.bodies["small", "W"].convert("RGB").getpixel((41, 45))), 205)
        self.assertLess(min(self.surfaces["small", "W", "depleted", 0][0].convert("RGB").getpixel((41, 45))), 205)

    def test_all_eight_sources_are_own_carbonaceous_originals(self):
        """Reuse geometry tooling without borrowing metallic source pixels or work effects."""
        family = self.document["families"][self.asset]
        keys = set(family["selections"].values())
        keys.update(key for states in family["surface_sources"].values() for views in states.values() for key in views.values())
        self.assertEqual(len(keys), 8)
        # Both free source batches and all six actual surface originals retain this body's exact input ancestry; no relabelled metallic original can complete a Water-Ice state.
        for key in keys:
            candidate = art.source_candidate(key, self.document)
            self.assertEqual(candidate["asset_id"], self.asset)
            self.assertTrue(all("carbonaceous-asteroid" in reference["path"] for reference in candidate["references"]))

    def test_changed_large_registration_cannot_supply_small_slab(self):
        """Validate frozen Large ancestry before deriving a believable Small uniform scale."""
        document = copy.deepcopy(self.document)
        family = document["families"][self.asset]
        candidate = next(row for row in document["candidates"] if row["key"] == family["selections"]["free/N"])
        candidate["native_transforms"]["N"]["source_reference"][1] += 1
        with self.assertRaisesRegex(ValueError, "Large original registration"):
            asteroids.AsteroidSources(self.asset, document).body("N", "small")

    def test_second_phase_cannot_borrow_first_phase_original(self):
        """A distinct phase requires its own real source, beyond a different logical frame label."""
        document = copy.deepcopy(self.document)
        family = document["families"][self.asset]
        family["surface_sources"]["mining"]["1"]["SE"] = family["surface_sources"]["mining"]["0"]["SE"]
        with self.assertRaisesRegex(ValueError, "phase ancestry"):
            asteroids.AsteroidSources(self.asset, document).surface("SE", "small", "mining", 1)


if __name__ == "__main__":
    unittest.main()
