"""Inspect decoded paid electrical riser source and meaningful corruption refusals.

@file riser_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group60C.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups5B/5.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
These source checks create no machinery, electrical grant, packed range or runtime binding.
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
import convert_riser_art as art


class RiserSourceChecks(unittest.TestCase):
    """Verify original native sections, tiny glass changes and paid stage boundaries.

    Native admission/construction separately proves the real fixed machinery layout.
    These cases independently decode stored crops and corrupt copied source inputs.
    """

    @classmethod
    def setUpClass(cls):
        """Read actual retained inputs and construct one baseline source publication.

        @param cls Test class receiving unchanged baseline provenance and decoded atlas.
        @return None; every refusal case receives a separate deep copy.
        """
        cls.record = json.loads(art.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(art.tube.PUBLIC.read_text(encoding="utf-8"))
        cls.family, cls.models, cls.component, cls.phases = art.source_inputs(cls.record, cls.public)
        cls.report, cls.encoded = art.build(cls.record, cls.public)
        with Image.open(io.BytesIO(cls.encoded)) as atlas:
            cls.atlas = atlas.copy()

    def test_complete_exact_publication(self):
        """Require128 exact identities on the fixed electrical four-cell native layout.

        @param self Current isolated test case.
        @return None; source publication still assigns no packed range or runtime.
        """
        art.validate(self.report, self.encoded, self.record, self.public)
        self.assertEqual(len(self.report["sprites"]), 128)
        self.assertEqual(art.ATLAS.read_bytes(), self.encoded)
        spec = art.public_family(self.public)
        self.assertEqual(spec["directions"], ["none"])
        self.assertIsNone(spec["packed"]["offset"])
        self.assertFalse(self.report["runtime_activated"])

    def test_four_originals_reconstruct_all_sixteen_native_sections(self):
        """Reconstruct each actual whole stage from its four independently stored cells.

        @param self Current isolated test case.
        @return None; every pixel has one physical owner and every native cell is present.
        """
        self.assertEqual(len({source["sha256"] for whole, pieces, source in self.models.values()}), 4)
        # Incomplete stage0..2 and approved ready each retain all four cells;
        # dropped/duplicated pixels or a one-cell machine cannot satisfy this.
        for whole, pieces, source in self.models.values():
            self.assertEqual(len(pieces), 4)
            self.assertTrue(all(piece.getbbox() is not None for piece in pieces))
            self.assertEqual(art.assemble(pieces).tobytes(), whole.tobytes())

    def test_all_decoded_poses_preserve_original_body_opacity_and_foreground(self):
        """Compare all sixteen stored whole poses outside their reviewed glass interiors.

        @param self Current isolated test case.
        @return None; phases cannot repaint hardware, move contacts or add a vehicle wall.
        """
        # Actual atlas reconstruction supplies each pose. Its matching retained
        # original establishes every nonglass indexed pixel and all opacity.
        for state in art.STATES:
            for frame in range(4):
                stage = frame if state == "construction" else 3
                original = self.models[str(stage) if stage < 3 else "ready"][0]
                pose = art.assembled_pose(self.report, self.atlas, state, frame)
                front = art.assembled_pose(self.report, self.atlas, state, frame, "front")
                self.assertIsNone(front.getbbox())
                x, y, width, height = self.family["indicator_targets"][str(stage)]
                for py in range(art.CANVAS[1]):
                    for px in range(art.CANVAS[0]):
                        a, b = original.getpixel((px, py)), pose.getpixel((px, py))
                        self.assertEqual(a != 0, b != 0)
                        if not (x <= px < x + width and y <= py < y + height):
                            self.assertEqual(a, b)

    def test_paid_stage3_is_amber_ready_hardware_before_separate_completion(self):
        """Retain native296/74 timing and separate stage3 from completed readiness.

        @param self Current isolated test case.
        @return None; final-looking hardware cannot grant operation before native completion.
        """
        self.assertEqual(self.report["construction_ticks"], {"total": 296, "stage_length": 74, "stages": [0, 1, 2, 3], "completion_separate": True})
        self.assertEqual(self.family["construction_indicator_phase"], 0)
        original = self.models["ready"][0]
        expected = art.compose_glass(original, self.phases[0][0], self.family["indicator_targets"]["3"])
        stage3 = art.assembled_pose(self.report, self.atlas, "construction", 3)
        self.assertEqual(stage3.tobytes(), expected.tobytes())
        self.assertNotEqual(stage3.tobytes(), art.assembled_pose(self.report, self.atlas, "ready", 0).tobytes())

    def test_actual_glass_phases_are_distinct_and_corrected_model_is_selected(self):
        """Inspect native phase diversity and retain the rejected nine-cell floor draft.

        @param self Current isolated test case.
        @return None; duplicate phases or a rejected physical layout cannot enter source.
        """
        self.assertEqual(self.family["whole_selections"]["ready"], "utility-riser-ready-v2")
        # Every completed state has four genuinely visible original phases at
        # the tiny native2x2 fitting, without regenerating the rest of the model.
        for state in ("ready", "working", "blocked"):
            self.assertEqual(len({art.assembled_pose(self.report, self.atlas, state, frame).tobytes() for frame in range(4)}), 4)
        with self.assertRaisesRegex(ValueError, "rejected"):
            art.native_original("utility-riser-ready-v1", self.record, self.public)

    def test_missing_stage_and_ready_substitution_refuse(self):
        """Remove incomplete hardware or reuse ready under an earlier construction label.

        @param self Current isolated test case.
        @return None; original stage coverage is checked before any publication.
        """
        for missing in (True, False):
            record = copy.deepcopy(self.record)
            family = next(row for row in record["families"] if row["asset_id"] == art.ASSET)
            if missing:
                family["whole_selections"].pop("1")
            else:
                family["whole_selections"]["1"] = family["whole_selections"]["ready"]
            with self.assertRaisesRegex(ValueError, "stage/equipment"):
                art.build(record, self.public)

    def test_native_rotation_domain_and_work_bill_corruption_refuse(self):
        """Try fictitious rotations, thermal demand and a copied generator labour bill.

        @param self Current isolated test case.
        @return None; the native riser remains fixed-layout electrical296-tick machinery.
        """
        for mutation in ("rotation", "thermal", "clock"):
            public = copy.deepcopy(self.public)
            family = next(row for row in public["assets"] if row["asset_id"] == art.ASSET)
            if mutation == "rotation":
                family["directions"] = ["NE", "SE", "SW", "NW"]
            elif mutation == "thermal":
                family["states"].append("heat_starved")
            else:
                family["geometry_notes"]["construction_ticks"]["total"] = 592
            with self.assertRaisesRegex(ValueError, "actual native"):
                art.build(self.record, public)

    def test_frozen_native_registration_and_equipment_drift_refuse(self):
        """Move hardware or enlarge a lamp after its physical source review.

        @param self Current isolated test case.
        @return None; unchanged original hashes cannot authorize different native geometry.
        """
        record = copy.deepcopy(self.record)
        next(row for row in record["candidates"] if row["key"] == "utility-riser-ready-v2")["native_transform"]["native_position"][1] += 1
        with self.assertRaisesRegex(ValueError, "reviewed registration"):
            art.build(record, self.public)
        record = copy.deepcopy(self.record)
        family = next(row for row in record["families"] if row["asset_id"] == art.ASSET)
        family["indicator_targets"]["0"][2] = 3
        with self.assertRaisesRegex(ValueError, "equipment approval"):
            art.build(record, self.public)

    def test_source_reference_corruption_refuses(self):
        """Alter original edit ancestry without modifying any retained source file.

        @param self Current isolated test case.
        @return None; rejected draft ancestry stays traceable but cannot change silently.
        """
        record = copy.deepcopy(self.record)
        next(row for row in record["candidates"] if row["key"] == "utility-riser-ready-v2")["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable"):
            art.build(record, self.public)

    def test_missing_duplicate_and_false_cell_ancestry_refuse(self):
        """Corrupt actual source pose identities and retained world-cell ancestry.

        @param self Current isolated test case.
        @return None; correct counts or plausible pixels do not hide a missing native cell.
        """
        for mutation in ("missing", "duplicate", "cell"):
            report = copy.deepcopy(self.report)
            if mutation == "missing":
                report["sprites"].pop()
            elif mutation == "duplicate":
                report["sprites"][-1] = copy.deepcopy(report["sprites"][0])
            else:
                report["sprites"][0]["world_cell"] = [9, 9]
            with self.assertRaises(ValueError):
                art.validate(report, self.encoded, self.record, self.public)

    def test_changed_decoded_pixel_and_palette_refuse(self):
        """Corrupt one real stored body pixel and a separate native palette entry.

        @param self Current isolated test case.
        @return None; source metadata cannot conceal changed native raster bytes.
        """
        atlas = self.atlas.copy()
        x, y = next((x, y) for y in range(atlas.height) for x in range(atlas.width) if atlas.getpixel((x, y)) != 0)
        atlas.putpixel((x, y), 1 if atlas.getpixel((x, y)) != 1 else 2)
        with self.assertRaisesRegex(ValueError, "native pixels"):
            art.validate(self.report, art.tube.indexed_png(atlas), self.record, self.public)
        atlas = self.atlas.copy()
        palette = atlas.getpalette()
        palette[3] ^= 1
        atlas.putpalette(palette)
        with self.assertRaisesRegex(ValueError, "palette"):
            art.validate(self.report, art.tube.indexed_png(atlas), self.record, self.public)

    def test_false_completion_runtime_and_changed_cap_opacity_refuse(self):
        """Reject false runtime/clock claims and transparent original lamp inserts.

        @param self Current isolated test case.
        @return None; source review cannot replace paid completion or erase hardware.
        """
        for mutation in ("runtime", "clock"):
            report = copy.deepcopy(self.report)
            if mutation == "runtime":
                report["runtime_activated"] = True
            else:
                report["construction_ticks"]["completion_separate"] = False
            with self.assertRaisesRegex(ValueError, "runtime or changes native work"):
                art.validate(report, self.encoded, self.record, self.public)
        component = Image.new("RGBA", (3, 3))
        with self.assertRaisesRegex(ValueError, "opacity"):
            art.compose_glass(self.models["ready"][0], component, self.family["indicator_targets"]["3"])


if __name__ == "__main__":
    unittest.main()
