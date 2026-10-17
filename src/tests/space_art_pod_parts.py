"""Verify real passive-pod tab phases and their protected original hinge/shell geometry.

@file space_art_pod_parts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BE
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group64.3
These source checks create no native descent work, shell, packets or recovered asset.
"""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_pod_parts as pod


class PodTabSourceTests(unittest.TestCase):
    """Require two real original tab parts, fixed protected contacts and actual visible phase differences."""

    def setUp(self):
        """Load detached source metadata for each independent physical/refusal case."""
        self.document = art.read_record()
        self.family = self.document["families"][pod.ASSET]
        self.candidate = art.source_candidate(self.family["descent_sources"]["0"]["N"], self.document)

    def test_all_actual_phases_protect_fixed_hull_hinges_and_original_opacity(self):
        """All16 poses protect48,270 outside-volume pixels, every original joint and all original shell opacity."""
        protected = 0
        # Independently compare every actual indexed pixel. Near plates may occlude gray heat-shield pixels only inside their declared physical projections; the original white cap/orange belt and all other hull pixels stay exact.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            editable = {tuple(p) for p in self.family["tab_edit_pixels"][direction]}
            rgb = base.convert("RGB")
            poses = []
            for phase in (0, 1):
                pose, evidence = pod.tab_pose(direction, phase, self.document)
                self.assertEqual(len(evidence["tabs"]), 2)
                self.assertEqual([p["contact_distance"] for p in evidence["tabs"]], [1, 1])
                for y in range(48):
                    for x in range(64):
                        old, new = base.getpixel((x, y)), pose.getpixel((x, y))
                        colour = rgb.getpixel((x, y))
                        fixed_colour = old != 0 and (max(colour) > 140 or max(colour) - min(colour) > 45)
                        if old != 0:
                            self.assertNotEqual(new, 0)
                        # All fixed hardware stays exact even inside a broad placement rectangle; a source body fragment cannot stand in for a moving tab.
                        if (x, y) not in editable or fixed_colour:
                            self.assertEqual(old, new)
                        if (x, y) not in editable:
                            protected += 1
                for hinge in self.family["tab_hinges"][direction]:
                    for point in hinge["contacts"]:
                        self.assertEqual(base.getpixel(tuple(point)), pose.getpixel(tuple(point)))
                poses.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(poses[0].get_flattened_data(), poses[1].get_flattened_data(), strict=True)), 2)
        self.assertEqual(protected, 48270)

    def test_wrong_authored_phase_refuses(self):
        """An actual modest-open source cannot be relabeled as the wider-open mechanical phase."""
        self.family["descent_sources"]["1"]["N"] = self.candidate["key"]
        with self.assertRaisesRegex(ValueError, "declared"):
            pod.tab_pose("N", 1, self.document)

    def test_third_tab_refuses(self):
        """An extra source rectangle cannot create a third fitting on the two-tab shell."""
        transforms = self.candidate["native_tab_transforms"]["N"]
        transforms.append(dict(transforms[0]))
        with self.assertRaisesRegex(ValueError, "exactly two"):
            pod.tab_pose("N", 0, self.document)

    def test_changed_part_reference_refuses(self):
        """Each actual tab face stays at its independently approved original hinge contact."""
        self.candidate["native_tab_transforms"]["N"][0]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            pod.tab_pose("N", 0, self.document)

    def test_changed_projected_mask_refuses(self):
        """A later smaller or wider editable plate region cannot silently change the approved source composition."""
        self.family["tab_part_edit_pixels"]["N"][0].pop()
        with self.assertRaisesRegex(ValueError, "registration"):
            pod.tab_pose("N", 0, self.document)

    def test_fixed_hinge_cannot_become_editable(self):
        """An original joint remains foreground even if a modified mask tries to include it."""
        point = self.family["tab_hinges"]["N"][0]["contacts"][0]
        self.family["tab_edit_pixels"]["N"].append(point)
        with self.assertRaisesRegex(ValueError, "hinge"):
            pod.tab_pose("N", 0, self.document)

    def test_foreign_mechanism_cannot_supply_passive_pod_tabs(self):
        """The expendable shell cannot borrow a carrier manipulator merely because both are small gray hardware."""
        self.family["descent_sources"]["0"]["N"] = self.document["families"]["vehicle.model-20"]["builder_sources"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "declared"):
            pod.tab_pose("N", 0, self.document)

    def test_known_extra_equipment_and_control_views_refuse(self):
        """The retained three-tab NE failure and unchanged phaseA controls cannot masquerade as phaseB."""
        # These originals retain other useful data, so rejection must apply to the explicitly failed/unrequested view rather than relying only on the whole-file status.
        for direction, key in (("NE", "return-pod-tabs-b-ne-se-sw-nw-v1"), ("SE", "return-pod-tabs-b-ne-control-v1")):
            self.family["descent_sources"]["1"][direction] = key
            with self.assertRaisesRegex(ValueError, "unaccepted"):
                pod.tab_pose(direction, 1, self.document)

    def test_hazy_original_cannot_supply_a_small_cropped_plate(self):
        """A small plausible physical crop cannot bypass rejection of the complete original alpha background."""
        self.family["descent_sources"]["1"]["NE"] = "return-pod-tabs-b-ne-se-sw-nw-v2"
        with self.assertRaisesRegex(ValueError, "rejected"):
            pod.tab_pose("NE", 1, self.document)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
