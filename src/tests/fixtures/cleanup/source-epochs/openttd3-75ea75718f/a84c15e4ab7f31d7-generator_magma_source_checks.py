"""Protect distinct2000 MagmaLoop original source and passive native ownership.

@file generator_magma_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group56.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Decoded source art must conserve the sixteen-cell model; it cannot invent the
2000 paid construction terms or activate a generator through an art descriptor.
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


class MagmaLoopSourceChecks(unittest.TestCase):
    """Check all MagmaLoop source states against real originals and native geometry.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks56.1-56.3.
    """

    @classmethod
    def setUpClass(cls):
        """Read actual generated proof and independently validate its original models.

        @param cls Focused check class sharing immutable baseline sources.
        @return None; malformed cases use copies and never rewrite original/proof files.
        """
        cls.asset = "facility.magma-powerhouse"
        cls.record = json.loads(art.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(art.tube.PUBLIC.read_text(encoding="utf-8"))
        cls.family, cls.spec, cls.phases, cls.models = art.generator_source_inputs(cls.asset, cls.record, cls.public)
        cls.report = json.loads((art.tube.REVIEW / "magmaloop-source-manifest.json").read_text(encoding="utf-8"))
        cls.encoded = (ROOT / cls.report["path"]).read_bytes()
        cls.atlas = Image.open(io.BytesIO(cls.encoded))
        cls.atlas.load()

    def test_actual_dated_passive_geometry(self):
        """Keep native2000 source geometry distinct from1980 construction admission.

        @param self Current focused source-check instance.
        @return None; the actual descriptor remains4x4/sixteen cells with no first invoice.
        """
        native = self.spec["geometry_notes"]["native_generator_geometry"]
        self.assertEqual(native["model"], "MagmaLoop")
        self.assertEqual(native["introduction_year"], 2000)
        self.assertEqual(native["world_cells"], [[x, y] for y in range(4) for x in range(4)])
        self.assertIsNone(native["first_build_contract_kind"])
        self.assertFalse(native["native_activation_from_descriptor"])
        self.assertEqual(self.spec["tile_variants"], 16)

    def test_selected_views_exclude_short_vessel_draft(self):
        """Preserve the genuine corrected NE and four independent physical orientations.

        @param self Current focused source-check instance.
        @return None; the rejected NEv1 cannot re-enter selection through a valid raw hash.
        """
        self.assertEqual(self.family["whole_view_selections"]["NE"], "magma-campus-ne-v2")
        self.assertEqual(len(set(self.family["whole_view_selections"].values())), 4)
        hashes = {self.models[key][2]["original_native_sha256"] for key in self.family["whole_view_selections"].values()}
        self.assertEqual(len(hashes), 4)
        with self.assertRaisesRegex(ValueError, "missing or excluded"):
            art.native_base("magma-campus-ne-v1", self.record, self.spec)

    def test_all_originals_reconstruct_from_unique_native_cells(self):
        """Independently reassemble all sixteen original hardware views from stored pieces.

        @param self Current focused source-check instance.
        @return None; all256 nonempty sections preserve the full original pixels exactly.
        """
        self.assertEqual(len(self.models), 16)
        # Read every stored native section independently. Overlapping pixels,
        # missing floor cells and a clipped machine cannot hide behind an atlas hash.
        for original, pieces, source in self.models.values():
            rebuilt = art.indexed_canvas(art.CANVAS)
            occupied = set()
            self.assertEqual(len(pieces), 16)
            for piece, (ox, oy) in zip(pieces, art.ORIGINS):
                self.assertIsNotNone(piece.getbbox())
                for y in range(64):
                    for x in range(64):
                        value = piece.getpixel((x, y))
                        if value == 0:
                            continue
                        point = (ox + x, oy + y)
                        self.assertNotIn(point, occupied, source["key"])
                        occupied.add(point)
                        rebuilt.putpixel(point, value)
            self.assertEqual(rebuilt.tobytes(), original.tobytes(), source["key"])

    def test_native_matrix_has2560_unique_poses(self):
        """Require every true state/frame/view/cell/part identity once.

        @param self Current focused source-check instance.
        @return None; padded counts and semantic holes cannot complete source coverage.
        """
        expected = {(state, frame, direction, tile, part) for state in art.STATES for frame in range(4)
                    for direction in art.DIRECTIONS for tile in range(16) for part in ("back", "front")}
        actual = [(r["state"], r["frame"], r["direction"], r["tile_variant"], r["part"]) for r in self.report["sprites"]]
        self.assertEqual(len(actual), 2560)
        self.assertEqual(set(actual), expected)
        self.assertEqual(self.report["pose_count"], 2560)

    def test_decoded_state_poses_preserve_all_non_glass_pixels_and_alpha(self):
        """Compare all actual atlas poses directly with the retained hardware pixels.

        @param self Current focused source-check instance.
        @return None; all80 poses retain the model outside its small existing lamp interior.
        """
        # Physical world origins reconstruct the actual stored atlas; this
        # comparison does not call the compositor to manufacture its expectation.
        for state in art.STATES:
            for frame in range(4):
                for direction in art.DIRECTIONS:
                    stage = str(frame) if state == "construction" else None
                    key = self.family["construction_frame_sources"][stage][direction] if stage is not None else self.family["whole_view_selections"][direction]
                    original = self.models[key][0].tobytes()
                    back = art.assemble_generator_pose(self.report, self.atlas, state, frame, direction).tobytes()
                    front = art.assemble_generator_pose(self.report, self.atlas, state, frame, direction, "front").tobytes()
                    self.assertEqual(back, front)
                    x, y, width, height = self.family["indicator_targets"][key]
                    glass = {(y + dy) * art.CANVAS[0] + x + dx for dy in range(height) for dx in range(width)}
                    changed = {i for i, (a, b) in enumerate(zip(original, back)) if a != b}
                    self.assertTrue(changed <= glass, (state, frame, direction))
                    self.assertTrue(all((a == 0) == (b == 0) for a, b in zip(original, back)))

    def test_constructed_appearance_grants_no1980_invoice_or_operation(self):
        """Keep stage3 amber and actual2000 work timing with its future feature owner.

        @param self Current focused source-check instance.
        @return None; source completion cannot borrow592 ticks or allocate runtime sprites.
        """
        self.assertEqual(self.family["construction_frame_sources"]["3"], self.family["whole_view_selections"])
        self.assertIsNone(self.report["construction_ticks"]["total"])
        self.assertIsNone(self.report["construction_ticks"]["stage_length"])
        self.assertTrue(self.report["construction_ticks"]["completion_separate"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertIsNone(self.spec["packed"]["offset"])
        # Final installed hardware still represents unfinished native work;
        # only the real construction owner may set actual completion.
        for row in self.report["sprites"]:
            if row["state"] != "construction":
                continue
            self.assertEqual(row["indicator"]["frame"], 0)
            self.assertEqual(row["native_construction_stage"], row["frame"])

    def test_existing_authored_heat_glass_is_reused_with_distinct_phases(self):
        """Bind shortage presentation to the genuine shared purple original.

        @param self Current focused source-check instance.
        @return None; all four native2x2 phases remain distinct, without procedural recoloring.
        """
        phases, component = self.phases["heat_starved"]
        self.assertEqual(component["key"], "generator-heat-starved-glass")
        native = [art.tube.base.quantize(p.resize((2, 2), Image.Resampling.BOX), {}).tobytes() for p in phases]
        self.assertEqual(len(set(native)), 4)
        rows = [r for r in self.report["sprites"] if r["state"] == "heat_starved"]
        self.assertEqual({r["indicator"]["path"] for r in rows}, {component["path"]})
        self.assertEqual({r["indicator"]["sha256"] for r in rows}, {component["sha256"]})

    def test_missing_stage_cannot_inherit_ready_hardware(self):
        """Refuse a missing incomplete stage before any final-hardware alias can hide it.

        @param self Current focused source-check instance.
        @return None; all twelve incomplete original views remain mandatory.
        """
        record = copy.deepcopy(self.record)
        family = next(r for r in record["families"] if r["asset_id"] == self.asset)
        del family["construction_frame_sources"]["2"]
        with self.assertRaisesRegex(ValueError, "native state/frame ownership"):
            art.generator_source_inputs(self.asset, record, self.public)

    def test_venttap_original_cannot_supply_magma_hardware(self):
        """Reject another generator's valid source under the2000 physical contract.

        @param self Current focused source-check instance.
        @return None; common4x4 geometry does not make different hardware interchangeable.
        """
        with self.assertRaisesRegex(ValueError, "verified native geometry"):
            art.native_base("venttap-campus-ne-v1", self.record, self.spec)

    def test_frozen_registration_cannot_drift(self):
        """Keep a reviewed view's physical floor and hardware registration immutable.

        @param self Current focused source-check instance.
        @return None; moving the corrected NE source requires a deliberate new approval.
        """
        record = copy.deepcopy(self.record)
        source = next(r for r in record["candidates"] if r["key"] == "magma-campus-ne-v2")
        source["native_position"][1] += 1
        with self.assertRaisesRegex(ValueError, "reviewed original"):
            art.generator_source_inputs(self.asset, record, self.public)

    def test_changed_fitting_or_borrowed_invoice_refuses(self):
        """Protect the physical lamp and the absence of invented2000 paid terms.

        @param self Current focused source-check instance.
        @return None; both changes invalidate the frozen source/equipment contract.
        """
        # Each malformed record exercises one boundary independently, retaining
        # all other approved originals and semantic source fields.
        for mode in ("fitting", "invoice"):
            record = copy.deepcopy(self.record)
            family = next(r for r in record["families"] if r["asset_id"] == self.asset)
            if mode == "fitting":
                family["indicator_targets"][family["whole_view_selections"]["NE"]][0] += 1
            else:
                family["construction_ticks"]["total"] = 592
            with self.assertRaisesRegex(ValueError, "approved original fittings"):
                art.generator_source_inputs(self.asset, record, self.public)

    def test_false_stage_ancestry_refuses(self):
        """Reject valid-looking pixels with incorrect native work-stage metadata.

        @param self Current focused source-check instance.
        @return None; each actual paid-stage binding must retain its original source ancestry.
        """
        report = copy.deepcopy(self.report)
        report["sprites"][0]["native_construction_stage"] = 3
        with self.assertRaisesRegex(ValueError, "false original/frame/equipment provenance"):
            art.validate_generator(report, self.encoded, self.record, self.public)

    def test_changed_actual_atlas_pixel_refuses(self):
        """Reject an actual changed indexed pixel with truthful metadata and palette.

        @param self Current focused source-check instance.
        @return None; immutable source regeneration verifies encoded pixels beyond report hashes.
        """
        changed = self.atlas.copy()
        changed.putpixel((0, 0), 1 if changed.getpixel((0, 0)) != 1 else 2)
        with self.assertRaisesRegex(ValueError, "original model/component pixels or ancestry"):
            art.validate_generator(self.report, art.tube.indexed_png(changed), self.record, self.public)


if __name__ == "__main__":
    unittest.main()
