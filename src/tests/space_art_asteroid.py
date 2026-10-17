"""Exercise actual metallic original pixels, native size reduction and refused ancestry.

@file space_art_asteroid.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BF
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group67
No fixture creates gameplay resources or edits the retained source originals.
"""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_asteroids as asteroids


class MetallicOriginalTests(unittest.TestCase):
    """Verify real selected body/part pixels and frozen original registration boundaries."""

    @classmethod
    def setUpClass(cls):
        """Convert each actual state/size once for this bounded focused file.

        @param cls Test class retaining exact detached input and conversion caches.
        """
        cls.document = art.read_record()
        cls.asset = "asteroid.nickel"
        cls.sources = asteroids.AsteroidSources(cls.asset, cls.document)
        cls.bodies = {}
        cls.surfaces = {}
        # Actual source conversion is deliberately shared within this file: all invariants examine the same original bytes, without repeatedly resampling unrelated accepted craft families.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                cls.bodies[size, direction] = cls.sources.body(direction, size)[0]
                for state, phase in (("mining", 0), ("mining", 1), ("depleted", 0)):
                    cls.surfaces[size, direction, state, phase] = cls.sources.surface(direction, size, state, phase)

    def test_original_large_matches_accepted_eight_view_atlas(self):
        """Keep every original Large pixel equal to the already reviewed eight-view atlas."""
        atlas = art.Image.open(art.ROOT / "logs/proofs/mlm/native-art/space/metallic-eight-free-v1/native.png")
        # Genuine directional originals stay immutable when local effects and additional saved sizes are introduced.
        for index, direction in enumerate(asteroids.DIRECTIONS):
            expected = atlas.crop((index * 96, 0, (index + 1) * 96, 80))
            self.assertEqual(expected.tobytes(), self.bodies["large", direction].tobytes())

    def test_saved_sizes_preserve_actual_body_proportions(self):
        """Check real raster extents against native quarter/half/full diameter ratios."""
        # Pixel rounding may move an edge by one pixel; it cannot turn the three original saved sizes into identical silhouettes or stretch one axis independently.
        for direction in asteroids.DIRECTIONS:
            large = self.bodies["large", direction].getbbox()
            for size, divisor in (("small", 4), ("medium", 2)):
                small = self.bodies[size, direction].getbbox()
                for low, high in ((0, 2), (1, 3)):
                    self.assertLessEqual(abs((small[high] - small[low]) * divisor - (large[high] - large[low])), divisor * 2)

    def test_all_surface_edits_preserve_outside_pixels_and_silhouette(self):
        """Compare actual output bytes instead of trusting the converter's protection report."""
        # The real asteroid body remains visible after mining/depletion; no changed ore texture may replace the beak, depression, contour or unrelated facets.
        for (size, direction, state, phase), (result, evidence) in self.surfaces.items():
            body = self.bodies[size, direction]
            mask = {tuple(point) for point in evidence["physical_mask"]}
            before, after = body.tobytes(), result.tobytes()
            self.assertEqual([value != 0 for value in before], [value != 0 for value in after])
            self.assertTrue(all(old == new for index, (old, new) in enumerate(zip(before, after))
                                if (index % 96, index // 96) not in mask), (size, direction, state, phase))

    def test_every_size_has_distinct_authored_mining_and_worked_cues(self):
        """Require actual phase and depletion differences even on Small bodies."""
        # A large-source sparkle disappearing after reduction is missing Small source coverage; logical animation aliases cannot hide that failure.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                normal = self.bodies[size, direction].tobytes()
                phase_a = self.surfaces[size, direction, "mining", 0][0].tobytes()
                phase_b = self.surfaces[size, direction, "mining", 1][0].tobytes()
                depleted = self.surfaces[size, direction, "depleted", 0][0].tobytes()
                self.assertEqual(len({normal, phase_a, phase_b, depleted}), 4, (size, direction))
                original_rgb = self.bodies[size, direction].convert("RGB")
                worked_rgb = self.surfaces[size, direction, "depleted", 0][0].convert("RGB")
                # Every original bright metallic ore pixel must be worked, including a curved seam's tips beyond an initial straight patch proposal.
                for index, value in enumerate(normal):
                    point = (index % 96, index // 96)
                    if value != 0 and min(original_rgb.getpixel(point)) >= 205:
                        self.assertLess(min(worked_rgb.getpixel(point)), 205, (size, direction, point))

    def test_static_attachment_and_logical_phase_aliases(self):
        """Keep the separately rendered native probe out of the unchanged body attachment pose."""
        # Four public frames honestly reference a static body or two real surface phases; they do not create spin, hardware or additional extraction work.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                normal = self.bodies[size, direction].tobytes()
                for frame in range(4):
                    self.assertEqual(self.sources.pose(direction, size, "free", frame)[0].tobytes(), normal)
                    self.assertEqual(self.sources.pose(direction, size, "attached", frame)[0].tobytes(), normal)
                    expected = self.surfaces[size, direction, "mining", frame % 2][0].tobytes()
                    self.assertEqual(self.sources.pose(direction, size, "mining", frame)[0].tobytes(), expected)

    def test_small_refuses_changed_original_large_registration(self):
        """Validate approved Large ancestry before deriving a plausible Small transform."""
        document = copy.deepcopy(self.document)
        family = document["families"][self.asset]
        candidate = next(row for row in document["candidates"] if row["key"] == family["selections"]["free/N"])
        candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "Large original registration"):
            asteroids.AsteroidSources(self.asset, document).body("N", "small")

    def test_refuse_changed_saved_size_policy(self):
        """Refuse a silently widened art envelope without its source acceptance seal."""
        document = copy.deepcopy(self.document)
        document["families"][self.asset]["native_art_envelope"][2] += 1
        with self.assertRaisesRegex(ValueError, "saved-size policy"):
            asteroids.AsteroidSources(self.asset, document)

    def test_refuse_changed_small_physical_mask(self):
        """Bind all saved-size masks even when the requested part is Large."""
        document = copy.deepcopy(self.document)
        document["families"][self.asset]["surface_masks"]["small"]["N"][0][0] += 1
        with self.assertRaisesRegex(ValueError, "physical mask"):
            asteroids.AsteroidSources(self.asset, document).surface("N", "large", "mining", 0)

    def test_refuse_relabelled_first_phase_as_second(self):
        """Require a genuine second original rather than copying a first-phase source key."""
        document = copy.deepcopy(self.document)
        family = document["families"][self.asset]
        family["surface_sources"]["mining"]["1"]["N"] = family["surface_sources"]["mining"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "phase ancestry"):
            asteroids.AsteroidSources(self.asset, document).surface("N", "large", "mining", 1)

    def test_refuse_other_body_surface(self):
        """Never borrow a plausible ore patch from another asteroid class."""
        document = copy.deepcopy(self.document)
        key = document["families"][self.asset]["surface_sources"]["depleted"]["0"]["N"]
        candidate = next(row for row in document["candidates"] if row["key"] == key)
        candidate["asset_id"] = "asteroid.ice"
        with self.assertRaisesRegex(ValueError, "body/phase ancestry"):
            asteroids.AsteroidSources(self.asset, document).surface("N", "large", "depleted", 0)

    def test_refuse_known_contaminated_original(self):
        """Keep the rejected magenta-contaminated original from returning through valid crops."""
        document = copy.deepcopy(self.document)
        document["families"][self.asset]["surface_sources"]["mining"]["1"]["S"] = "metallic-asteroid-mining-b-s-sw-w-nw-v1"
        with self.assertRaisesRegex(ValueError, "rejected Space source"):
            asteroids.AsteroidSources(self.asset, document).surface("S", "large", "mining", 1)


if __name__ == "__main__":
    unittest.main()
