"""Verify Crate-75 sealed cargo forms and its own original native source.

@file tube_container_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group50.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
In-memory refusals never change retained art, public metadata or native vehicles.
"""

import copy
import unittest

import tube_capsule_source_checks as common


class TubeContainerSourceChecks(common.TubeCapsuleSourceChecks):
    """Apply the shared cargo/mode/hull gates to actual sealed-package artwork."""

    ASSET_ID = "vehicle.model-10"
    BASE_KEY = "crate-base-e-v1"
    EXCLUDED_KEY = "crate-n-ne-s-sw-v1"

    def test_packaged_cargo_has_own_same_view_original(self):
        """Require sealed-package ancestry rather than borrowing loose Rock material.

        @param self Current isolated source/check instance.
        @return None; every loaded heading names its original component crop.
        """
        # All states retain actual packaged cargo identity; sharing a native
        # cargo quantity with ordinary wagons does not authorize bulk ore art.
        for row in self.body["sprites"]:
            if row["load"] == "empty":
                self.assertIsNone(row["cargo"])
                continue
            cargo = row["cargo"]
            self.assertEqual(cargo["cargo_form"], "sealed_package_module")
            self.assertEqual(cargo["source_view"], row["direction"])
            self.assertEqual(cargo["source_crop"], self.family["components"]["cargo_source_crops"][row["direction"]])
            self.assertTrue(cargo["path"].endswith("crate-packages-v1.png"))
            self.assertNotIn("source_cell", cargo)

    def test_partial_occupies_rear_and_full_both_pockets(self):
        """Compare visible cargo changes against the two original native package bays.

        @param self Current isolated native source instance.
        @return None; every direction preserves distinct actual one/two-bay loads.
        """
        # The first approved mask is the physical rear pocket in every view.
        # Cargo quantity adds a second package without moving either fitting.
        for direction in common.capsules.DIRECTIONS:
            empty, unused = self.pose("empty", "operating", direction)
            partial, unused = self.pose("partial", "operating", direction)
            full, unused = self.pose("full", "operating", direction)
            rear, front = [{tuple(p) for p in mask} for mask in self.family["components"]["hold_masks"][direction]]
            changed_partial = {(x, y) for y in range(48) for x in range(64) if empty.getpixel((x, y)) != partial.getpixel((x, y))}
            changed_full = {(x, y) for y in range(48) for x in range(64) if partial.getpixel((x, y)) != full.getpixel((x, y))}
            self.assertTrue(changed_partial)
            self.assertTrue(changed_full)
            self.assertTrue(changed_partial <= rear, direction)
            self.assertTrue(changed_full <= front, direction)

    def test_bulk_owner_cannot_supply_sealed_package_component(self):
        """Reject an M1 Rock owner at the dedicated packaged-cargo decode boundary.

        @param self Current isolated source/check instance.
        @return None; changing a key cannot change the physical cargo meaning.
        """
        components = copy.deepcopy(self.family["components"])
        components["cargo_record"] = "media/multilayer/source/m1/generation-record.json"
        components["cargo_key"] = "rock"
        with self.assertRaisesRegex(ValueError, "foreign sealed"):
            common.capsules.package_donors(components, self.document)

    def test_package_registration_drift_refuses(self):
        """Reject a changed lid/strap crop despite an unchanged original PNG identity.

        @param self Current isolated source/check instance.
        @return None; actual native package material needs its approved registration.
        """
        self.family["components"]["cargo_source_crops"]["E"][0] += 1
        with self.assertRaisesRegex(ValueError, "unapproved"):
            common.capsules.build(self.document, self.ASSET_ID)


if __name__ == "__main__":
    unittest.main()
