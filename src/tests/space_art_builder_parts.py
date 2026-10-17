"""Verify actual carrier manipulators, hatch states and explicitly reused original gas.

@file space_art_builder_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BD
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63C
Source admission protects original hardware; no test advances native work, service, cargo or movement.
"""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_builder_parts as builder
import convert_space_mining_parts as parts
import convert_space_status_parts as status

ASSET = builder.ASSET


class BuilderComponentSourceTests(unittest.TestCase):
    """Require real physical phases, exact original pixel protection and immutable own-role ancestry."""

    def setUp(self):
        """Load detached original source definitions for each independent refusal case."""
        self.document = art.read_record()
        self.family = self.document["families"][ASSET]

    def test_actual_arm_phases_preserve_every_pixel_outside_the_swept_volume(self):
        """All16 moving arm poses preserve the exact original carrier, including colored mounting hardware."""
        # Compare actual indexed pixels independently of converter evidence. The empty bed and fixed ochre frame may not be repainted by a plausible regenerated body.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            editable = {tuple(p) for p in self.family["builder_edit_pixels"][direction]}
            original_rgb = base.convert("RGB")
            poses = []
            for phase in (0, 1):
                pose, evidence = builder.builder_pose(direction, phase, self.document)
                self.assertGreaterEqual(evidence["changed_pixels"], 4)
                for y in range(48):
                    for x in range(64):
                        colour = original_rgb.getpixel((x, y))
                        coloured_hardware = base.getpixel((x, y)) != 0 and max(colour) - min(colour) > 30
                        # The swept volume cannot authorize a new frame/paddle/hatch pixel, even inside its broad placement rectangle.
                        if (x, y) not in editable or coloured_hardware:
                            self.assertEqual(base.getpixel((x, y)), pose.getpixel((x, y)))
                poses.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(poses[0].get_flattened_data(), poses[1].get_flattened_data(), strict=True)), 4)

    def test_wrong_arm_phase_refuses(self):
        """One actual reaching original cannot fill the distinct fastening phase."""
        self.family["builder_sources"]["1"]["N"] = self.family["builder_sources"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "declared"):
            builder.builder_pose("N", 1, self.document)

    def test_changed_original_mount_refuses(self):
        """A later arm joint cannot drift away from its approved original carrier attachment."""
        self.family["builder_mounts"]["NW"][0][0] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            builder.builder_pose("NW", 0, self.document)

    def test_changed_swept_pixel_mask_refuses(self):
        """A wider moving region cannot silently authorize repainting the original bed or truss frame."""
        self.family["builder_edit_pixels"]["N"].pop()
        with self.assertRaisesRegex(ValueError, "registration"):
            builder.builder_pose("N", 0, self.document)

    def test_foreign_scanner_cannot_supply_construction_arms(self):
        """The distinct carrier must retain its own actual manipulator source."""
        self.family["builder_sources"]["0"]["N"] = self.document["families"]["vehicle.model-19"]["scanner_sources"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "declared"):
            builder.builder_pose("N", 0, self.document)

    def test_all_hatch_states_preserve_opacity_and_have_real_native_cues(self):
        """All24 status poses retain73,608 non-hatch pixels and actual colored connector/latch cues."""
        protected = 0
        # Compare the whole original craft, including hull pixels inside the wider registration rectangle. A numerical phase difference from averaged amber shading alone is insufficient.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            editable = {tuple(p) for p in self.family["status_edit_pixels"][direction]}
            phases = []
            for state, phase in (("blocked", 0), ("service", 0), ("service", 1)):
                pose, _ = status.status_pose(state, direction, phase, self.document, ASSET)
                rgb = pose.convert("RGB")
                colours = [rgb.getpixel(point) for point in editable]
                if state == "blocked":
                    self.assertTrue(any(red > 1.5 * green and red > 1.5 * blue for red, green, blue in colours))
                else:
                    self.assertTrue(any(blue > 1.4 * red and blue > 100 for red, green, blue in colours))
                    phases.append(pose)
                for y in range(48):
                    for x in range(64):
                        self.assertEqual(base.getpixel((x, y)) == 0, pose.getpixel((x, y)) == 0)
                        if (x, y) not in editable:
                            self.assertEqual(base.getpixel((x, y)), pose.getpixel((x, y)))
                            protected += 1
            self.assertNotEqual(phases[0].tobytes(), phases[1].tobytes())
        self.assertEqual(protected, 73608)

    def test_changed_hatch_mask_refuses(self):
        """Adding one original frame pixel inside the broad placement rectangle cannot change the approved hatch."""
        self.family["status_edit_pixels"]["N"].append([32, 35])
        with self.assertRaisesRegex(ValueError, "registration"):
            status.status_pose("service", "N", 0, self.document, ASSET)

    def test_foreign_hatch_cannot_supply_carrier_service(self):
        """Another craft's panel cannot replace the original amber carrier fitting."""
        self.family["status_sources"]["service"]["0"]["N"] = self.document["families"]["vehicle.model-19"]["status_sources"]["service"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "declared"):
            status.status_pose("service", "N", 0, self.document, ASSET)

    def test_actual_gas_phases_preserve_all_opaque_pixels_and_touch_the_rear(self):
        """Every registered pulse touches the carrier's frozen rear contacts and changes no original opaque pixel."""
        # Inspect actual output pixels against the original hull and explicit rear contact set; merely existing on the canvas cannot authorize a floating or foreign effect.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            poses = []
            for phase in (0, 1):
                pose, evidence = parts.component_pose("rear_plume", direction, phase, self.document, ASSET)
                self.assertFalse(evidence["source_instance"]["new_generated_artwork"])
                gas = []
                for y in range(48):
                    for x in range(64):
                        if base.getpixel((x, y)) != 0:
                            self.assertEqual(pose.getpixel((x, y)), base.getpixel((x, y)))
                        elif pose.getpixel((x, y)) != 0:
                            gas.append((x, y))
                self.assertTrue(gas)
                contacts = self.family["travel_outlet_contacts"][direction]
                self.assertEqual(min(max(abs(x - cx), abs(y - cy)) for x, y in gas for cx, cy in contacts), 1)
                poses.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(poses[0].get_flattened_data(), poses[1].get_flattened_data(), strict=True)), 2)

    def test_changed_donor_approval_refuses(self):
        """An unchanged PNG cannot borrow a provisional or changed original gas registration."""
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.document)
        candidate["donor_registration"]["N"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "approval"):
            parts.component_pose("rear_plume", "N", 0, self.document, ASSET)

    def test_gas_instance_cannot_change_source_geometry(self):
        """Only the receiving native outlet can change; original gas cropping and physical scale remain exact."""
        candidate = art.source_candidate(self.family["travel_sources"]["0"]["N"], self.document)
        candidate["native_transforms"]["N"]["source_part_box"][0] += 1
        with self.assertRaisesRegex(ValueError, "geometry"):
            parts.component_pose("rear_plume", "N", 0, self.document, ASSET)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
