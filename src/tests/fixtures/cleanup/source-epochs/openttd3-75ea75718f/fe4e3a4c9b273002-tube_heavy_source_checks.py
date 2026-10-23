"""Verify Forge-05 solid heavy loads, open-frame alpha and foreground custody.

@file tube_heavy_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group53.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed inputs remain in memory; original art and native heavy cargo are untouched.
"""

import copy
import io
import unittest

from PIL import Image

import tube_capsule_source_checks as common


class TubeHeavySourceChecks(common.TubeCapsuleSourceChecks):
    """Require genuine solid modules only inside approved open-frame cargo volumes."""

    ASSET_ID = "vehicle.model-13"
    BASE_KEY = "forge-base-e-v1"

    def original_pose(self, direction):
        """Decode an own-model original independently of assembled load/state pixels.

        @param self Current isolated source/check instance.
        @param direction Actual engine heading to inspect.
        @return Indexed empty carrier and its checked source registration.
        """
        selected = next(row for row in self.family["selections"] if row["direction"] == direction)
        source = common.capsules.tube.retained_candidate(selected["source"], self.document)
        return common.capsules.models.native_pose(source, direction)

    def test_known_excluded_heading_refuses(self):
        """Keep the duplicate E view excluded while preserving the approved base.

        @param self Current isolated source/check instance.
        @return None; the correct label cannot authorize an unapproved physical view.
        """
        next(row for row in self.family["selections"] if row["direction"] == "E")["source"] = "forge-se-e-nw-w-v1"
        with self.assertRaisesRegex(ValueError, "excluded"):
            common.capsules.build(self.document, self.ASSET_ID)

    def test_original_hull_and_alpha_stay_exact(self):
        """Preserve the carrier while allowing only actual approved load-volume alpha.

        @param self Current isolated source/check instance.
        @return None; cargo adds solid module pixels without erasing frame/pad material.
        """
        # The open frame differs from a contained hopper: genuine load may
        # fill transparent apertures, while every pixel outside authorized
        # cargo/glass/company fittings remains the unchanged original carrier.
        for direction in common.capsules.DIRECTIONS:
            baseline, unused = self.original_pose(direction)
            holds = {tuple(p) for mask in self.family["components"]["hold_masks"][direction] for p in mask}
            for load in ("empty", "partial", "full"):
                image, row = self.pose(load, "operating", direction)
                allowed = holds | {tuple(row["indicator"]["target"])}
                if row["company_plaque"] is not None:
                    allowed.add(tuple(row["company_plaque"]["target"]))
                changed = {(x, y) for y in range(48) for x in range(64) if image.getpixel((x, y)) != baseline.getpixel((x, y))}
                erased = {(x, y) for y in range(48) for x in range(64) if baseline.getpixel((x, y)) and not image.getpixel((x, y))}
                added = {(x, y) for y in range(48) for x in range(64) if not baseline.getpixel((x, y)) and image.getpixel((x, y))}
                self.assertTrue(changed <= allowed, (direction, load))
                self.assertFalse(erased, (direction, load))
                self.assertTrue(added <= holds, (direction, load))

    def test_heavy_modules_have_actual_same_view_ancestry(self):
        """Bind every solid load to its complete original module and native projection.

        @param self Current isolated source/check instance.
        @return None; no donor wagon or neighbouring source cell supplies missing art.
        """
        # Actual rear/front module quantity and far/near drawing order are
        # different concepts. Preserve both explicitly with the original view.
        for row in self.body["sprites"]:
            if row["load"] == "empty":
                self.assertIsNone(row["cargo"])
                continue
            cargo = row["cargo"]
            self.assertEqual(cargo["cargo_form"], "heavy_pallet_module")
            self.assertEqual(cargo["source_view"], row["direction"])
            self.assertEqual(cargo["source_crop"], self.family["components"]["cargo_source_crops"][row["direction"]])
            self.assertIs(cargo["alpha_extension"], True)
            count = 1 if row["load"] == "partial" else 2
            self.assertEqual(cargo["target_boxes"], self.family["components"]["cargo_target_boxes"][row["direction"]][:count])
            self.assertEqual(cargo["placement_order"], [i for i in self.family["components"]["cargo_placement_order"][row["direction"]] if i < count])
            self.assertTrue(cargo["path"].endswith("forge-heavy-modules-v1.png"))
            self.assertNotIn("source_cell", cargo)

    def test_solid_load_fills_real_open_apertures(self):
        """Require actual new solid pixels rather than merely repainting empty pad tops.

        @param self Current isolated native source/check instance.
        @return None; each broad side view visibly fills both genuine open load spaces.
        """
        # Side views expose the actual empty frame aperture. Both genuine
        # payloads must gain alpha there, remaining inside their approved masks.
        for direction in ("E", "W"):
            empty, unused = self.pose("empty", "operating", direction)
            full, unused = self.pose("full", "operating", direction)
            added = {(x, y) for y in range(48) for x in range(64) if not empty.getpixel((x, y)) and full.getpixel((x, y))}
            for mask in self.family["components"]["hold_masks"][direction]:
                self.assertTrue(added & {tuple(p) for p in mask}, direction)

    def test_rear_quantity_and_near_module_occlusion(self):
        """Compare actual one/two-module pixels and preserve the closer existing module.

        @param self Current isolated native source/check instance.
        @return None; a farther full-load addition cannot paint over a nearer rear module.
        """
        observed_overlap = 0
        # Partial occupies only the real rear volume. In headings where that
        # module is closest, adding the farther front module preserves every
        # already visible overlapping rear pixel through correct depth order.
        for direction in common.capsules.DIRECTIONS:
            empty, unused = self.pose("empty", "operating", direction)
            partial, unused = self.pose("partial", "operating", direction)
            full, unused = self.pose("full", "operating", direction)
            rear, front = [{tuple(p) for p in mask} for mask in self.family["components"]["hold_masks"][direction]]
            first = {(x, y) for y in range(48) for x in range(64) if empty.getpixel((x, y)) != partial.getpixel((x, y))}
            second = {(x, y) for y in range(48) for x in range(64) if partial.getpixel((x, y)) != full.getpixel((x, y))}
            self.assertTrue(first)
            self.assertTrue(second)
            self.assertTrue(first <= rear, direction)
            self.assertTrue(second <= front, direction)
            if direction in ("N", "NE", "NW"):
                overlap = first & front
                observed_overlap += len(overlap)
                for point in overlap:
                    self.assertEqual(partial.getpixel(point), full.getpixel(point), direction)
        self.assertGreater(observed_overlap, 0)

    def test_component_crop_cannot_bleed_into_neighbor(self):
        """Reject matching metadata that includes part of a neighbouring original module.

        @param self Current isolated source/check instance.
        @return None; actual opacity bands independently bound every whole-source crop.
        """
        components = copy.deepcopy(self.family["components"])
        components["cargo_source_crops"]["N"][2] += 140
        source = next(row for row in self.document["candidates"] if row["key"] == "forge-heavy-modules-v1")
        source["component_views"] = copy.deepcopy(components["cargo_source_crops"])
        with self.assertRaisesRegex(ValueError, "actual source cell"):
            common.capsules.heavy_donors(components, self.document)

    def test_transparent_cargo_cannot_erase_frame(self):
        """Reject a loaded atlas whose transparent pixel removes an actual steel brace.

        @param self Current isolated source/check instance.
        @return None; valid added load alpha never permits lost carrier foreground.
        """
        row = next(row for row in self.body["sprites"] if (row["load"], row["mode"], row["direction"], row["frame"]) == ("full", "operating", "E", 0))
        path = common.capsules.tube.base.repository_path(self.body["path"])
        with Image.open(io.BytesIO(self.files[path])) as atlas:
            atlas.putpixel((row["box"][0] + 23, row["box"][1] + 24), 0)
            self.files[path] = common.capsules.tube.indexed_png(atlas)
        with self.assertRaisesRegex(ValueError, "changes original"):
            common.capsules.validate(self.report, self.files, self.document, self.ASSET_ID)

    def test_orange_clamps_remain_original_in_every_mode(self):
        """Keep hydraulic clamp material independent from cargo and service/power glass.

        @param self Current isolated native source/check instance.
        @return None; an orange clamp cannot be recoloured as a readiness indicator.
        """
        # Identify the actual orange fittings in the immutable original before
        # native downsampling blends them with dark steel. Their projected
        # pixels remain structural hardware even when the palette makes them
        # brown; testing brightness afterwards would miss both end views.
        for direction in common.capsules.DIRECTIONS:
            baseline, transform = self.original_pose(direction)
            selected = next(row for row in self.family["selections"] if row["direction"] == direction)
            source = common.capsules.tube.retained_candidate(selected["source"], self.document)
            with Image.open(common.capsules.tube.base.repository_path(source["path"])) as original:
                crop = original.convert("RGBA").crop(transform["native_crop"])
            mask = Image.new("L", crop.size)
            # Opaque, strongly warm original material is hydraulic hardware.
            # Transparent edges, grey frame and blue plaque cannot qualify.
            mask.putdata([255 if a >= 128 and r > 100 and r > 1.3 * g and r > 2 * b else 0
                          for r, g, b, a in crop.getdata()])
            width, height = transform["native_subject_size"]
            mask = mask.resize((width, height), Image.Resampling.BOX)
            dx, dy = transform["native_translation"]
            left, top = (64 - width) // 2 + dx, 40 - height + dy
            clamps = set()
            # A quarter-pixel original coverage identifies visible fittings
            # without promoting trace antialiasing into unrelated frame cells.
            for y in range(height):
                for x in range(width):
                    point = (left + x, top + y)
                    # Only an actually opaque native fitting needs protection;
                    # a transparent sampling fringe is outside the carrier.
                    if mask.getpixel((x, y)) >= 64 and baseline.getpixel(point) != 0:
                        clamps.add(point)
            self.assertTrue(clamps, direction)
            holds = {tuple(point) for hold in self.family["components"]["hold_masks"][direction] for point in hold}
            self.assertFalse(clamps & holds, direction)
            # Every quantity, equipment condition and genuine glass phase
            # preserves the same original hydraulic clamps independently.
            for load in ("empty", "partial", "full"):
                for mode in ("operating", "service", "blocked"):
                    for frame in (0, 1):
                        image, row = self.pose(load, mode, direction, frame)
                        self.assertNotIn(tuple(row["indicator"]["target"]), clamps)
                        for point in clamps:
                            self.assertEqual(image.getpixel(point), baseline.getpixel(point), (direction, load, mode, frame))


if __name__ == "__main__":
    unittest.main()
