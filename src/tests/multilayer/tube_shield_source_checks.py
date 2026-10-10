"""Verify Shield-85 sealed mineral roof meters and original native armour.

@file tube_shield_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group52.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
In-memory refusals never change retained artwork, native cargo or engine eligibility.
"""

import copy
import unittest

import tube_capsule_source_checks as common


class TubeShieldSourceChecks(common.TubeCapsuleSourceChecks):
    """Require sealed armour and independent cargo/mode fittings in every heading."""

    ASSET_ID = "vehicle.model-12"
    BASE_KEY = "shield-base-e-v1"

    def test_known_excluded_heading_refuses(self):
        """Keep the duplicate E view excluded while preserving the approved base.

        @param self Current isolated source/check instance.
        @return None; valid labels cannot replace a frozen own-model registration.
        """
        next(row for row in self.family["selections"] if row["direction"] == "E")["source"] = "shield-se-e-nw-w-v1"
        with self.assertRaisesRegex(ValueError, "excluded"):
            common.capsules.build(self.document, self.ASSET_ID)

    def test_load_meter_preserves_genuine_original_glass_ancestry(self):
        """Require actual M1 amber glass rather than loose mineral or painted armour.

        @param self Current isolated source/check instance.
        @return None; every loaded pose names the genuine reused state and phase.
        """
        # A roof meter displays the native cargo bucket while the mineral
        # remains inside opaque armour. Its actual glass source is independent
        # of the separate service/power indicator and body direction artwork.
        for row in self.body["sprites"]:
            if row["load"] == "empty":
                self.assertIsNone(row["cargo"])
                continue
            cargo = row["cargo"]
            self.assertEqual(cargo["cargo_form"], "sealed_mineral_roof_meter")
            self.assertEqual(cargo["source_state"], "construction")
            self.assertEqual(cargo["source_frame"], 0)
            self.assertEqual(cargo["path"], row["indicator"]["path"])
            self.assertEqual(cargo["sha256"], row["indicator"]["sha256"])
            self.assertNotIn("source_cell", cargo)

    def test_partial_lights_rear_meter_and_full_both(self):
        """Compare actual pixels against the two separate original roof fittings.

        @param self Current isolated native source/check instance.
        @return None; sealed cargo never repaints armour or changes body alpha.
        """
        # The genuine rear fitting represents partial cargo in every world
        # heading. Full adds only the second fitting, preserving the first
        # and every surrounding armour/frame pixel.
        for direction in common.capsules.DIRECTIONS:
            empty, unused = self.pose("empty", "operating", direction)
            partial, unused = self.pose("partial", "operating", direction)
            full, unused = self.pose("full", "operating", direction)
            rear, front = [{tuple(p) for p in mask} for mask in self.family["components"]["hold_masks"][direction]]
            partial_changes = {(x, y) for y in range(48) for x in range(64) if empty.getpixel((x, y)) != partial.getpixel((x, y))}
            full_changes = {(x, y) for y in range(48) for x in range(64) if partial.getpixel((x, y)) != full.getpixel((x, y))}
            self.assertEqual(len(rear), 1)
            self.assertEqual(len(front), 1)
            self.assertFalse(rear & front)
            self.assertEqual(partial_changes, rear, direction)
            self.assertEqual(full_changes, front, direction)

    def test_bulk_original_cannot_replace_sealed_meter(self):
        """Reject Rock material at the sealed-mineral meter component boundary.

        @param self Current isolated source/check instance.
        @return None; an opaque shielded cargo role does not inherit bulk ore art.
        """
        components = copy.deepcopy(self.family["components"])
        components["cargo_key"] = "rock"
        with self.assertRaisesRegex(ValueError, "foreign sealed"):
            common.capsules.sealed_meter_donors(components)


if __name__ == "__main__":
    unittest.main()
