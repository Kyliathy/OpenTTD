"""Verify complete Powerhouse original-pixel states before native packing.

@file generator_state_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group55B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Actual decoded sections, opaque hardware and frozen component ancestry are checked
independently; source completion grants no operating, cargo or utility behavior.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_generator_art as art


class GeneratorStateSourceChecks(unittest.TestCase):
    """Protect all actual native states and original pixels in the2560-pose source.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks55B.3-55B.4.
    """

    @classmethod
    def setUpClass(cls):
        """Load one actual generated atlas and independently checked original hardware.

        @param cls Focused source-check class sharing immutable baseline inputs.
        @return None; malformed cases copy metadata and never overwrite source proof.
        """
        cls.record = json.loads(art.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(art.tube.PUBLIC.read_text(encoding="utf-8"))
        cls.asset = "facility.vent-powerhouse"
        cls.family, cls.spec, cls.phases, cls.models = art.generator_source_inputs(cls.asset, cls.record, cls.public)
        cls.report = json.loads((art.tube.REVIEW / "venttap-source-manifest.json").read_text(encoding="utf-8"))
        cls.encoded = (ROOT / cls.report["path"]).read_bytes()
        cls.atlas = Image.open(io.BytesIO(cls.encoded))
        cls.atlas.load()

    def test_complete_unique_native_matrix(self):
        """Require every actual state, frame, orientation, world cell and solid part.

        @param self Current focused source-check instance.
        @return None; exactly2560 distinct semantic poses cover the native family.
        """
        actual = [(r["state"], r["frame"], r["direction"], r["tile_variant"], r["part"]) for r in self.report["sprites"]]
        expected = {(state, frame, direction, tile, part)
                    for state in ("construction", "ready", "working", "blocked", "heat_starved")
                    for frame in range(4) for direction in ("NE", "SE", "SW", "NW")
                    for tile in range(16) for part in ("back", "front")}
        self.assertEqual(len(actual), 2560)
        self.assertEqual(len(set(actual)), 2560)
        self.assertEqual(set(actual), expected)
        self.assertEqual(self.report["pose_count"], 2560)

    def test_all_decoded_poses_preserve_original_hardware_and_alpha(self):
        """Compare assembled atlas pixels with immutable native models outside real glass.

        @param self Current focused source-check instance.
        @return None; all80 complete poses preserve equipment, floor, alpha and both parts.
        """
        # Read the stored atlas through each real native world origin. Original
        # non-glass bytes are compared directly, rather than regenerating the
        # compositor's claimed result or accepting a matching declared hash.
        for state in art.STATES:
            for frame in range(4):
                for direction in art.DIRECTIONS:
                    stage = str(frame) if state == "construction" else None
                    key = self.family["construction_frame_sources"][stage][direction] if stage is not None else self.family["whole_view_selections"][direction]
                    original = self.models[key][0]
                    back = art.assemble_generator_pose(self.report, self.atlas, state, frame, direction)
                    front = art.assemble_generator_pose(self.report, self.atlas, state, frame, direction, "front")
                    self.assertEqual(back.tobytes(), front.tobytes())
                    x, y, width, height = self.family["indicator_targets"][key]
                    glass = {(y + dy) * art.CANVAS[0] + x + dx for dy in range(height) for dx in range(width)}
                    before, after = original.tobytes(), back.tobytes()
                    differences = {index for index, (a, b) in enumerate(zip(before, after)) if a != b}
                    self.assertTrue(differences <= glass, (state, frame, direction))
                    self.assertTrue(all((a == 0) == (b == 0) for a, b in zip(before, after)))

    def test_genuine_glass_has_four_distinct_phases_at_actual_fitting_sizes(self):
        """Keep authored animation legible even in the narrow NE stage2 fitting.

        @param self Current focused source-check instance.
        @return None; each completed mode has four distinct actual native glass phases.
        """
        sizes = {tuple(target[2:]) for target in self.family["indicator_targets"].values()}
        # The smallest physical fitting must retain genuine phase differences;
        # distinct large originals alone do not prove useful native animation.
        for state, (phases, component) in self.phases.items():
            self.assertTrue((ROOT / component["path"]).is_file())
            for size in sizes:
                native = [art.tube.base.quantize(p.resize(size, Image.Resampling.BOX), {}).tobytes() for p in phases]
                self.assertEqual(len(set(native)), 4, (state, size))

    def test_construction_stage3_is_unfinished_ready_hardware_under_amber(self):
        """Preserve final-quarter work independently of installed-equipment appearance.

        @param self Current focused source-check instance.
        @return None; stage3 uses ready originals and amber phase0 before native completion.
        """
        rows = [r for r in self.report["sprites"] if r["state"] == "construction" and r["frame"] == 3]
        self.assertEqual(len(rows), 128)
        for row in rows:
            self.assertEqual(row["source_key"], self.family["whole_view_selections"][row["direction"]])
            self.assertEqual(row["native_construction_stage"], 3)
            self.assertEqual(row["indicator"]["state"], "construction")
            self.assertEqual(row["indicator"]["frame"], 0)
        self.assertEqual(self.report["construction_ticks"]["stage_length"], 148)
        self.assertEqual(self.report["construction_ticks"]["total"], 592)
        self.assertTrue(self.report["construction_ticks"]["completion_separate"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertIsNone(self.spec["packed"]["offset"])

    def test_changed_fitting_refuses_before_composition(self):
        """Refuse a plausible opaque target shifted outside its reviewed glass.

        @param self Current focused source-check instance.
        @return None; frozen approval protects the original cabinet mounting.
        """
        record = copy.deepcopy(self.record)
        family = next(r for r in record["families"] if r["asset_id"] == self.asset)
        family["indicator_targets"][family["whole_view_selections"]["NE"]][0] += 1
        with self.assertRaisesRegex(ValueError, "approved original fittings"):
            art.generator_source_inputs(self.asset, record, self.public)

    def test_changed_heat_original_refuses(self):
        """Reject changed Heat glass bytes instead of synthesizing a replacement color.

        @param self Current focused source-check instance.
        @return None; actual authored component ancestry remains immutable.
        """
        record = copy.deepcopy(self.record)
        record["components"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable facility source"):
            art.generator_source_inputs(self.asset, record, self.public)

    def test_missing_heat_phase_refuses(self):
        """Refuse incomplete phase coverage even with unchanged source image bytes.

        @param self Current focused source-check instance.
        @return None; no missing phase can silently reuse an existing phase.
        """
        record = copy.deepcopy(self.record)
        record["components"][0]["glass_boxes"].pop()
        with self.assertRaisesRegex(ValueError, "equipment/native-frame coverage"):
            art.generator_source_inputs(self.asset, record, self.public)

    def test_overlapping_heat_crops_refuse(self):
        """Reject two phases borrowing the same original glass rectangle.

        @param self Current focused source-check instance.
        @return None; source windows remain independent of later native sampling.
        """
        record = copy.deepcopy(self.record)
        boxes = record["components"][0]["glass_boxes"]
        boxes[1] = list(boxes[0])
        with self.assertRaisesRegex(ValueError, "crops overlap"):
            art.generator_glass_phases(self.family, record)

    def test_transparent_glass_cannot_paint_missing_hardware(self):
        """Keep every actual hardware pixel opaque when replacing a tiny fitting.

        @param self Current focused source-check instance.
        @return None; empty component alpha refuses before native cell edits.
        """
        key = self.family["whole_view_selections"]["NE"]
        original, pieces, unused = self.models[key]
        with self.assertRaisesRegex(ValueError, "loses opacity"):
            art.compose_generator_glass(original, pieces, Image.new("RGBA", (3, 3)), self.family["indicator_targets"][key])

    def test_changed_paid_work_and_final_alias_refuse(self):
        """Prevent art metadata from changing native work or declaring completion early.

        @param self Current focused source-check instance.
        @return None; native592-tick work and ready-only final hardware alias remain fixed.
        """
        # Separate malformed records test both independent business boundaries
        # without carrying one mutation into another refusal assertion.
        for mode in ("work", "alias"):
            record = copy.deepcopy(self.record)
            family = next(r for r in record["families"] if r["asset_id"] == self.asset)
            if mode == "work":
                family["construction_ticks"]["total"] -= 1
            else:
                family["construction_frame_sources"]["3"] = dict(family["construction_frame_sources"]["2"])
            with self.assertRaises(ValueError):
                art.generator_source_inputs(self.asset, record, self.public)

    def test_missing_and_duplicate_pose_refuse(self):
        """Reject semantic holes even when a duplicate preserves the declared total count.

        @param self Current focused source-check instance.
        @return None; every true native pose must occur once.
        """
        for duplicate in (False, True):
            report = copy.deepcopy(self.report)
            report["sprites"].pop()
            if duplicate:
                report["sprites"].append(copy.deepcopy(report["sprites"][0]))
            with self.assertRaisesRegex(ValueError, "state/stage/view/cell/part coverage"):
                art.validate_generator(report, self.encoded, self.record, self.public)

    def test_false_stage_and_original_ancestry_refuse(self):
        """Reject truthful pixels paired with false paid-stage or source metadata.

        @param self Current focused source-check instance.
        @return None; source reports cannot misrepresent hardware or native frame meaning.
        """
        for field, value in (("native_construction_stage", 3), ("source_sha256", "0" * 64)):
            report = copy.deepcopy(self.report)
            report["sprites"][0][field] = value
            with self.assertRaisesRegex(ValueError, "false original/frame/equipment provenance"):
                art.validate_generator(report, self.encoded, self.record, self.public)

    def test_changed_atlas_pixel_refuses_against_immutable_originals(self):
        """Refuse an actual altered source pixel with otherwise valid metadata and palette.

        @param self Current focused source-check instance.
        @return None; regeneration checks real encoded bytes beyond declared hashes.
        """
        changed = self.atlas.copy()
        changed.putpixel((0, 0), 1 if changed.getpixel((0, 0)) != 1 else 2)
        with self.assertRaisesRegex(ValueError, "original model/component pixels or ancestry"):
            art.validate_generator(self.report, art.tube.indexed_png(changed), self.record, self.public)


if __name__ == "__main__":
    unittest.main()
