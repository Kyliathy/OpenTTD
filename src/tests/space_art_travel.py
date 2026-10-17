"""Protect the complete original craft while actual authored travel pulses occupy empty native space.

@file space_art_travel.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BB
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group63.3
Actual retained clean-alpha effect originals supply both pulses; malformed detached receipts test refusal without editing their pixels or advancing gameplay.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_mining_parts as parts


class TravelOriginalTests(unittest.TestCase):
    """Require every old opaque hull/nozzle/paddle/tool pixel to survive both genuine pulse phases."""

    def setUp(self):
        """Read independent candidate definitions so each malformed-source test preserves selected original records."""
        self.document = art.read_record()
        self.family = self.document["families"][parts.ASSET]
        key = self.family["travel_sources"]["0"]["N"]
        self.candidate = next(row for row in self.document["candidates"] if row["key"] == key)

    def test_actual_pulses_preserve_complete_original_craft(self):
        """All sixteen real directional pulses preserve every original opaque craft pixel and all transparency outside the effect region."""
        protected = 0
        # Inspect every original native pixel independently; source ancestry or an outside-rectangle count alone cannot catch a rectangle cutting through the bus at its engine joint.
        for direction in self.family["directions"]:
            base, _ = art.native_view(self.family["selections"]["free/" + direction], direction, self.document)
            left, top, right, bottom = self.family["travel_component_bounds"][direction]
            phases = []
            for phase in (0, 1):
                pose, evidence = parts.travel_pose(direction, phase, self.document)
                self.assertEqual(pose.getpalette(), base.getpalette())
                self.assertGreater(evidence["changed_pixels_from_base"], 0)
                for y in range(48):
                    for x in range(64):
                        # Only an originally transparent pixel inside the declared physical pulse region may receive actual authored plume pixels.
                        if base.getpixel((x, y)) != 0 or not (left <= x < right and top <= y < bottom):
                            self.assertEqual(base.getpixel((x, y)), pose.getpixel((x, y)))
                            protected += 1
                phases.append(pose)
            self.assertGreaterEqual(sum(a != b for a, b in zip(phases[0].get_flattened_data(), phases[1].get_flattened_data(), strict=True)), 2)
        self.assertEqual(protected, 47478)
        self.assertFalse(self.family["runtime_activated"])

    def test_changed_effect_root_refuses(self):
        """An approved pulse cannot detach from its actual original nozzle outlet after registration changes."""
        self.candidate["native_transforms"]["N"]["native_reference"][1] -= 1
        with self.assertRaisesRegex(ValueError, "registration"):
            parts.travel_pose("N", 0, self.document)

    def test_changed_effect_rectangle_refuses(self):
        """An enlarged physical effect region cannot silently authorize another selected output."""
        self.family["travel_component_bounds"]["N"][0] -= 1
        with self.assertRaisesRegex(ValueError, "registration"):
            parts.travel_pose("N", 0, self.document)

    def test_changed_authored_part_selection_refuses(self):
        """A changed original subrectangle cannot substitute hardware or another pulse for the selected effect pixels."""
        self.candidate["native_transforms"]["N"]["source_part_box"][1] -= 1
        with self.assertRaisesRegex(ValueError, "registration"):
            parts.travel_pose("N", 0, self.document)

    def test_unauthored_phase_refuses(self):
        """A missing pulse phase cannot be synthesized by reusing the other original."""
        with self.assertRaisesRegex(ValueError, "phase"):
            parts.travel_pose("N", 2, self.document)

    def test_hardware_replacement_source_refuses(self):
        """A whole-nozzle source declaration cannot replace the immutable original craft during travel."""
        self.candidate["component_kind"] = "rear_nozzle"
        with self.assertRaisesRegex(ValueError, "declared authored"):
            parts.travel_pose("N", 0, self.document)

    def test_part_outside_validated_view_refuses(self):
        """An unapproved part crop still cannot escape the complete source view's measured separation."""
        self.family.pop("approved_travel_registration")
        self.candidate["native_transforms"]["N"]["source_part_box"][0] = -1
        with self.assertRaisesRegex(ValueError, "validated source view"):
            parts.travel_pose("N", 0, self.document)

    def test_invalid_native_contact_refuses(self):
        """A source candidate cannot place an effect reference outside the actual fixed native canvas."""
        self.family.pop("approved_travel_registration")
        self.candidate["native_transforms"]["N"]["native_reference"] = [-1, 44]
        with self.assertRaisesRegex(ValueError, "reference leaves"):
            parts.travel_pose("N", 0, self.document)

    def test_actual_printed_checkerboard_source_refuses(self):
        """The real rejected extraction attempt cannot gain acceptance by choosing a small effect rectangle inside its printed background."""
        receipt_path = ROOT / "media/multilayer/source/space/small-miner-plume-a-n-ne-e-se-v1.json"
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        candidate = copy.deepcopy(self.candidate)
        candidate.update(key=receipt["key"], path=receipt["path"], sha256=hashlib.sha256((ROOT / receipt["path"]).read_bytes()).hexdigest(),
                         generation_receipt=art.identity(receipt_path), references=[{key: row[key] for key in ("path", "sha256")} for row in receipt["reference_inputs"]])
        candidate["native_transforms"]["N"]["measured_subject_bbox_in_crop"] = [0, 0, 704, 448]
        self.family.pop("approved_travel_registration")
        self.document["candidates"].append(candidate)
        self.family["travel_sources"]["0"]["N"] = candidate["key"]
        with self.assertRaisesRegex(ValueError, "clearance"):
            parts.travel_pose("N", 0, self.document)


if __name__ == "__main__":
    unittest.main(testRunner=unittest.TextTestRunner(stream=sys.stdout))
