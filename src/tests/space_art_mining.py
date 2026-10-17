"""Focused actual authored mining-phase and protected original hull pixel checks.

@file space_art_mining.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63.3
Detached invalid receipts test refusal only; no test edits original pixels or advances gameplay work.
"""
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_mining_parts as mining


class MiningOriginalTests(unittest.TestCase):
    """Require actual distinct native tool phases and immutable hull/part ancestry in all eight headings."""

    def setUp(self):
        """Read a new detached record for each refusal so selected original sources remain immutable."""
        self.document = art.read_record()
        self.family = self.document["families"][mining.ASSET]
        key = self.family["mining_sources"]["0"]["N"]
        self.candidate = next(row for row in self.document["candidates"] if row["key"] == key)

    def test_actual_two_phase_source_and_protected_body(self):
        """Every genuine direction retains all original nonmechanism palette/opacity pixels and two distinct authored tool poses."""
        protected = 0
        # Compare the original native base independently at every visible and transparent nonmechanism pixel; regenerated lookalikes cannot pass this equality.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            phases = []
            left, top, right, bottom = self.family["mining_component_bounds"][direction]
            for phase in (0, 1):
                pose, evidence = mining.mining_pose(direction, phase, self.document)
                self.assertEqual(pose.getpalette(), base.getpalette())
                self.assertEqual(evidence["protected_pixels"], 64 * 48 - (right - left) * (bottom - top))
                self.assertGreater(evidence["changed_pixels_from_base"], 0)
                for y in range(48):
                    for x in range(64):
                        # Actual hook/drill parts may move; every other original body/nozzle/paddle pixel and its transparency must remain byte-identical.
                        if left <= x < right and top <= y < bottom:
                            continue
                        self.assertEqual(base.getpixel((x, y)), pose.getpixel((x, y)))
                        protected += 1
                phases.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(phases[0].get_flattened_data(), phases[1].get_flattened_data(), strict=True)), 4)
        self.assertEqual(protected, 45514)
        self.assertFalse(self.family["runtime_activated"])

    def test_missing_phase_refuses(self):
        """An unauthored phase cannot repeat another still image to pretend animation coverage."""
        with self.assertRaisesRegex(ValueError, "phase"):
            mining.mining_pose("N", 2, self.document)

    def test_missing_heading_source_refuses(self):
        """A missing original directional phase cannot fall back to another heading."""
        del self.family["mining_sources"]["0"]["N"]
        with self.assertRaisesRegex(ValueError, "source heading"):
            mining.mining_pose("N", 0, self.document)

    def test_missing_genuine_source_view_refuses(self):
        """A source without the selected genuine view cannot rotate another picture into it."""
        del self.candidate["native_transforms"]["N"]
        with self.assertRaisesRegex(ValueError, "genuine"):
            mining.mining_pose("N", 0, self.document)

    def test_wrong_phase_source_refuses(self):
        """The other mechanical phase cannot substitute for the specifically requested authored original."""
        self.family["mining_sources"]["0"]["N"] = self.family["mining_sources"]["1"]["N"]
        with self.assertRaisesRegex(ValueError, "declared authored"):
            mining.mining_pose("N", 0, self.document)

    def test_wrong_physical_part_refuses(self):
        """A full-hull redraw cannot claim permission to replace the approved front mechanism."""
        self.candidate["component_kind"] = "whole_hull"
        with self.assertRaisesRegex(ValueError, "declared authored"):
            mining.mining_pose("N", 0, self.document)

    def test_changed_component_bounds_refuses(self):
        """An enlarged moving-part region cannot absorb protected yellow access-panel or rear hardware pixels."""
        self.family["mining_component_bounds"]["N"][3] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            mining.mining_pose("N", 0, self.document)

    def test_changed_component_registration_refuses(self):
        """A changed source reference cannot move a selected mechanism relative to its unchanged bus."""
        self.candidate["native_transforms"]["N"]["source_reference"][0] += 1
        with self.assertRaisesRegex(ValueError, "registration"):
            mining.mining_pose("N", 0, self.document)

    def test_changed_source_reference_refuses(self):
        """A forged actual edit-input hash cannot authorize a mechanism with different ancestry."""
        self.candidate["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            mining.mining_pose("N", 0, self.document)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
