"""Verify Cistern-80 original sight-glass levels and opaque tank source geometry.

@file tube_fluid_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group51.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed inputs remain in memory; source art never creates native fluid stock.
"""

import copy
import unittest

import tube_capsule_source_checks as common


class TubeFluidSourceChecks(common.TubeCapsuleSourceChecks):
    """Apply capsule custody/component gates to actual visible and hidden fluid gauges."""

    ASSET_ID = "vehicle.model-11"
    BASE_KEY = "cistern-base-e-v1"

    def test_service_and_blocking_retain_actual_load(self):
        """Keep actual fluid level through mode changes without painting hidden contents.

        @param self Current isolated source/check instance.
        @return None; visible gauges have three levels and opaque views one sealed hull.
        """
        visible = self.family["components"]["cargo_visible_directions"]
        # Service and power waits never change contents. Only actual original
        # sight-glass views reveal a level; their hidden counterparts preserve
        # the same opaque shell while retaining independent load metadata.
        for direction in common.capsules.DIRECTIONS:
            forms = []
            for load in ("empty", "partial", "full"):
                image, row = self.pose(load, "operating", direction)
                cue = tuple(row["indicator"]["target"])
                image.putpixel(cue, 0)
                forms.append(image.tobytes())
                for mode in ("service", "blocked"):
                    other, other_row = self.pose(load, mode, direction)
                    other.putpixel(cue, 0)
                    self.assertEqual(image.tobytes(), other.tobytes(), (direction, load, mode))
                    self.assertEqual(other_row["load"], load)
            self.assertEqual(len(set(forms)), 3 if direction in visible else 1, direction)

    def test_known_excluded_heading_refuses(self):
        """Keep the duplicate E source excluded while preserving the selected base.

        @param self Current isolated source/check instance.
        @return None; a correctly labelled duplicate cannot replace an approved base.
        """
        next(row for row in self.family["selections"] if row["direction"] == "E")["source"] = "cistern-se-e-nw-w-v1"
        with self.assertRaisesRegex(ValueError, "excluded"):
            common.capsules.build(self.document, self.ASSET_ID)

    def test_fluid_levels_name_actual_glass_material(self):
        """Require level/component ancestry and explicit original fitting visibility.

        @param self Current isolated source/check instance.
        @return None; loose cargo cannot replace liquid inside a sealed level glass.
        """
        visible = self.family["components"]["cargo_visible_directions"]
        # Every nonempty pose names its genuinely authored half/full component.
        # Hidden views retain that cargo state with no invented material target.
        for row in self.body["sprites"]:
            if row["load"] == "empty":
                self.assertIsNone(row["cargo"])
                continue
            cargo = row["cargo"]
            self.assertEqual(cargo["cargo_form"], "sealed_fluid_sight_glass")
            self.assertEqual(cargo["source_state"], row["load"])
            self.assertEqual(cargo["level_visible"], row["direction"] in visible)
            self.assertEqual(cargo["source_crop"], self.family["components"]["cargo_source_crops"][row["load"]])
            self.assertTrue(cargo["path"].endswith("cistern-level-glass-v1.png"))

    def test_visible_level_height_changes_only_actual_glass(self):
        """Compare actual native half/full levels with their approved lower/upper slit.

        @param self Current isolated native source/check instance.
        @return None; no pressure band, tank surface or frame is repainted as liquid.
        """
        # Partial fills only the lower glass pixels; full adds the upper part.
        # The real fitting's small native area must still distinguish all levels.
        for direction in self.family["components"]["cargo_visible_directions"]:
            empty, unused = self.pose("empty", "operating", direction)
            partial, unused = self.pose("partial", "operating", direction)
            full, unused = self.pose("full", "operating", direction)
            lower, upper = [{tuple(p) for p in mask} for mask in self.family["components"]["hold_masks"][direction]]
            partial_changes = {(x, y) for y in range(48) for x in range(64) if empty.getpixel((x, y)) != partial.getpixel((x, y))}
            full_changes = {(x, y) for y in range(48) for x in range(64) if partial.getpixel((x, y)) != full.getpixel((x, y))}
            self.assertTrue(partial_changes)
            self.assertTrue(full_changes)
            self.assertTrue(partial_changes <= lower, direction)
            self.assertTrue(full_changes <= lower | upper, direction)

    def test_bulk_owner_cannot_supply_sealed_fluid_material(self):
        """Reject a Rock source at the dedicated sealed-fluid material boundary.

        @param self Current isolated source/check instance.
        @return None; sharing an actual cargo quantity cannot change its physical form.
        """
        components = copy.deepcopy(self.family["components"])
        components["cargo_key"] = "rock"
        with self.assertRaisesRegex(ValueError, "foreign sealed"):
            common.capsules.liquid_donors(components, self.document)

    def test_company_crop_cannot_expand_into_cyan_tanks(self):
        """Reject a changed blue-plaque crop despite the original tank image hashing.

        @param self Current isolated source/check instance.
        @return None; cyan tank shadows remain fixed material, never company remap.
        """
        self.family["components"]["company_source_boxes"]["SE"][0] -= 30
        with self.assertRaisesRegex(ValueError, "unapproved"):
            common.capsules.build(self.document, self.ASSET_ID)

    def test_false_gauge_visibility_refuses(self):
        """Reject provenance that hides an actually visible original sight glass.

        @param self Current isolated source/check instance.
        @return None; complete pose counts cannot conceal false physical visibility.
        """
        row = next(row for row in self.body["sprites"] if row["direction"] == "E" and row["load"] == "full")
        row["cargo"]["level_visible"] = False
        with self.assertRaisesRegex(ValueError, "ancestry"):
            common.capsules.validate(self.report, self.files, self.document, self.ASSET_ID)


if __name__ == "__main__":
    unittest.main()
