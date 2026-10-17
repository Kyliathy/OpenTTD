"""Verify real authored status parts preserve the original native craft.

@file space_art_status.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63.3
Detached malformed records exercise source refusal without editing original pixels or granting native cargo/service.
"""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "media/multilayer"))
import convert_space_art as art
import convert_space_status_parts as status


class StatusOriginalTests(unittest.TestCase):
    """Require actual cues, unchanged hull opacity and approved per-view registration."""

    def setUp(self):
        """Load detached definitions for each refusal, preserving selected originals."""
        self.document = art.read_record()
        self.family = self.document["families"][status.parts.ASSET]
        key = self.family["status_sources"]["service"]["0"]["N"]
        self.candidate = next(row for row in self.document["candidates"] if row["key"] == key)

    def test_actual_cues_protect_all_original_hardware_and_opacity(self):
        """All32 real cue views preserve opacity everywhere and exact hardware pixels outside their original panel."""
        protected = 0
        # Inspect every pixel independently of converter counters, catching a small status rectangle erasing opaque bus pixels.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            left, top, right, bottom = self.family["status_component_bounds"][direction]
            phases = []
            for state, phase in status.STATES:
                pose, _ = status.status_pose(state, direction, phase, self.document)
                self.assertEqual(pose.getpalette(), base.getpalette())
                self.assertGreater(sum(a != b for a, b in zip(base.get_flattened_data(), pose.get_flattened_data(), strict=True)), 0)
                self.assertFalse((set(pose.get_flattened_data()) - {0}) & art.base.FORBIDDEN)
                for y in range(48):
                    for x in range(64):
                        self.assertEqual(base.getpixel((x, y)) == 0, pose.getpixel((x, y)) == 0)
                        if not (left <= x < right and top <= y < bottom):
                            self.assertEqual(base.getpixel((x, y)), pose.getpixel((x, y)))
                            protected += 1
                if state == "service":
                    phases.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(phases[0].get_flattened_data(), phases[1].get_flattened_data(), strict=True)), 1)
        self.assertEqual(protected, 97532)
        self.assertFalse(self.family["runtime_activated"])

    def test_static_cue_has_no_fake_second_authored_phase(self):
        """Logical static aliases cannot be presented as another original animation phase."""
        with self.assertRaisesRegex(ValueError, "phase"):
            status.status_pose("full", "N", 1, self.document)

    def test_changed_panel_region_refuses(self):
        """A wider edit region cannot silently recolor neighboring hull or tools."""
        self.family["status_component_bounds"]["N"][0] -= 1
        with self.assertRaisesRegex(ValueError, "registration"):
            status.status_pose("service", "N", 0, self.document)

    def test_changed_panel_contact_refuses(self):
        """Later state work cannot drift the approved panel off its original access surface."""
        self.candidate["native_transforms"]["N"]["native_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            status.status_pose("service", "N", 0, self.document)

    def test_wrong_state_source_refuses(self):
        """A service cue cannot borrow a full-hold source and impersonate actual inspection work."""
        self.family["status_sources"]["service"]["0"]["N"] = self.family["status_sources"]["full"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "declared"):
            status.status_pose("service", "N", 0, self.document)

    def test_wrong_mechanical_phase_refuses(self):
        """One actual service still cannot fill both authored mechanical phases."""
        self.family["status_sources"]["service"]["1"]["N"] = self.candidate["key"]
        with self.assertRaisesRegex(ValueError, "declared"):
            status.status_pose("service", "N", 1, self.document)

    def test_missing_actual_heading_refuses(self):
        """No mirrored or neighboring panel substitutes a missing genuine native view."""
        del self.family["status_sources"]["service"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "heading"):
            status.status_pose("service", "N", 0, self.document)

    def test_changed_original_receipt_refuses(self):
        """Selected pixels require their exact actual prompt receipt, beyond a plausible filename."""
        self.candidate["generation_receipt"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            status.status_pose("service", "N", 0, self.document)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
