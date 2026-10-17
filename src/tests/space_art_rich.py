"""Check genuine rich mineral originals at every native saved size.

@file space_art_rich.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BFC
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group69
Only retained original pixels supply art; these checks grant no native work or disclosure.
"""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_asteroids as asteroids


def violet(color):
    """Identify the cool mineral independently of naturally warm host stone.

    @param color Actual decoded native RGB pixel.
    @return Whether its blue excess identifies the original violet inclusion.
    """
    red, green, blue = color
    return blue - green >= 35 and blue - red >= 20 and blue >= 110


def warm(color):
    """Identify gold remaining after original-source averaging and DOS quantization.

    @param color Actual decoded native RGB pixel, never a recolored diagnostic.
    @return Whether the original glint has a visible warm red/green excess.
    """
    red, green, blue = color
    return red >= 140 and red - blue >= 40 and green - blue >= 20


class RichOriginalTests(unittest.TestCase):
    """Protect the rich body's original mineral, stone and three-size state coverage.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md Group69.3
    """

    @classmethod
    def setUpClass(cls):
        """Convert this family's actual matrix once within the focused file.

        @param cls Test class holding bounded exact-document conversion results.
        """
        cls.document = art.read_record()
        cls.asset = "asteroid.rare-earth"
        cls.sources = asteroids.AsteroidSources(cls.asset, cls.document)
        cls.bodies, cls.surfaces = {}, {}
        # Every saved size must retain its own actual mineral cue; testing only Large could hide the earlier Small N/NE/W failures.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                cls.bodies[size, direction] = cls.sources.body(direction, size)[0]
                for state, phase in (("mining", 0), ("mining", 1), ("depleted", 0)):
                    cls.surfaces[size, direction, state, phase] = cls.sources.surface(direction, size, state, phase)

    def test_all_base_views_remain_the_reviewed_original(self):
        """Keep eight genuine original yaws rather than redrawing host stone.

        @param self Test instance comparing the accepted base atlas.
        """
        with art.Image.open(art.ROOT / "logs/proofs/mlm/native-art/space/rich-eight-free-v1/native.png") as atlas:
            # Each physical yaw stays bound to its accepted own original while local work states are introduced.
            for index, direction in enumerate(asteroids.DIRECTIONS):
                expected = atlas.crop((index * 96, 0, (index + 1) * 96, 80))
                self.assertEqual(expected.tobytes(), self.bodies["large", direction].tobytes())

    def test_saved_sizes_keep_uniform_original_proportions(self):
        """Respect actual quarter/half diameters despite raster rounding.

        @param self Test instance comparing each genuine body view.
        """
        # Both axes follow the same source reduction; a larger body cannot be relabelled Small or stretched independently to fit its art slot.
        for direction in asteroids.DIRECTIONS:
            large = self.bodies["large", direction].getbbox()
            for size, divisor in (("small", 4), ("medium", 2)):
                small = self.bodies[size, direction].getbbox()
                for low, high in ((0, 2), (1, 3)):
                    self.assertLessEqual(abs((small[high] - small[low]) * divisor - (large[high] - large[low])), divisor * 2)

    def test_every_surface_preserves_outside_stone_and_opacity(self):
        """Compare actual pixels outside each explicit physical fissure mask.

        @param self Test instance checking all72 assembled surface poses.
        """
        # Local mining/exhaustion may replace mineral and its immediate rim, never redraw the pit, enlarge the body or erase a contour through transparent part margins.
        for (size, direction, state, phase), (result, evidence) in self.surfaces.items():
            before, after = self.bodies[size, direction].tobytes(), result.tobytes()
            mask = {tuple(point) for point in evidence["physical_mask"]}
            self.assertEqual([value != 0 for value in before], [value != 0 for value in after])
            self.assertTrue(all(old == new for index, (old, new) in enumerate(zip(before, after))
                                if (index % 96, index // 96) not in mask), (size, direction, state, phase))
            self.assertFalse((set(after) - {0}) & art.base.FORBIDDEN)

    def test_both_glints_replace_original_violet_at_distinct_positions(self):
        """Prevent naturally warm host rock from passing the mining cue check.

        @param self Test instance requiring actual mineral contact at every size/yaw.
        """
        # Only changed gold ON formerly violet mineral proves an authored contact glint. Warm brown host pixels and two labels on the same phase cannot satisfy mining coverage.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                body = self.bodies[size, direction]
                rgb = body.convert("RGB")
                points = {(x, y) for y in range(80) for x in range(96)
                          if body.getpixel((x, y)) != 0 and violet(rgb.getpixel((x, y)))}
                self.assertTrue(points, (size, direction))
                glints = []
                for phase in (0, 1):
                    result = self.surfaces[size, direction, "mining", phase][0]
                    colors = result.convert("RGB")
                    glint = {point for point in points if warm(colors.getpixel(point))}
                    self.assertTrue(glint, (size, direction, phase))
                    self.assertTrue(all(body.getpixel(point) != result.getpixel(point) for point in glint))
                    glints.append(glint)
                self.assertNotEqual(glints[0], glints[1], (size, direction))

    def test_depleted_surface_removes_every_original_violet_pixel(self):
        """Check mineral exhaustion independently of the white-ore205 guard.

        @param self Test instance checking actual worked inclusion colors.
        """
        # Authorized exhaustion changes this purple mineral, so an unchanged violet pocket cannot pass merely because it was never bright white ore.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                body = self.bodies[size, direction]
                original = body.convert("RGB")
                worked = self.surfaces[size, direction, "depleted", 0][0].convert("RGB")
                for y in range(80):
                    for x in range(96):
                        point = (x, y)
                        # Only opaque original mineral contributes to the finite-inclusion check; transparent palette background conveys no resource state.
                        if body.getpixel(point) != 0 and violet(original.getpixel(point)):
                            self.assertFalse(violet(worked.getpixel(point)), (size, direction, point))

    def test_attached_body_and_logical_frames_have_honest_aliases(self):
        """Keep the separately drawn native probe and work clock out of source art.

        @param self Test instance resolving all public logical frames.
        """
        # Static free/attached/depleted poses and the two actual contact phases preserve the public four-frame contract without inventing body spin or additional native work.
        for size in asteroids.SIZES:
            for direction in asteroids.DIRECTIONS:
                body = self.bodies[size, direction].tobytes()
                states = [body, *(self.surfaces[size, direction, state, phase][0].tobytes()
                                  for state, phase in (("mining", 0), ("mining", 1), ("depleted", 0)))]
                self.assertEqual(len(set(states)), 4)
                for frame in range(4):
                    self.assertEqual(self.sources.pose(direction, size, "attached", frame)[0].tobytes(), body)
                    self.assertEqual(self.sources.pose(direction, size, "mining", frame)[0].tobytes(), states[1 + frame % 2])
                    self.assertEqual(self.sources.pose(direction, size, "depleted", frame)[0].tobytes(), states[3])

    def test_small_body_refuses_changed_large_registration(self):
        """Keep smaller output bound to the actual accepted Large original.

        @param self Test instance changing only an isolated document copy.
        """
        document = copy.deepcopy(self.document)
        family = document["families"][self.asset]
        candidate = next(row for row in document["candidates"] if row["key"] == family["selections"]["free/N"])
        candidate["native_transforms"]["N"]["source_reference"][1] += 1
        with self.assertRaisesRegex(ValueError, "Large original registration"):
            asteroids.AsteroidSources(self.asset, document).body("N", "small")

    def test_second_phase_refuses_first_phase_source(self):
        """Require a distinct real original beyond a different frame label.

        @param self Test instance substituting an isolated source reference.
        """
        document = copy.deepcopy(self.document)
        family = document["families"][self.asset]
        family["surface_sources"]["mining"]["1"]["SE"] = family["surface_sources"]["mining"]["0"]["SE"]
        with self.assertRaisesRegex(ValueError, "phase ancestry"):
            asteroids.AsteroidSources(self.asset, document).surface("SE", "small", "mining", 1)


if __name__ == "__main__":
    unittest.main()
