"""Focused compiled processor geometry/state handoff and false-proof refusals.

@file native_processor_contract_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native funding/state tests authorize fixed source geometry only; original art,
packed ranges, physical transport and ordinary-game rendering remain pending.
"""

import copy
import json
from pathlib import Path
import sys
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests"))
from tools import ground_processor_contract as processor
import multilayer_sprite_contract as shared


class NativeProcessorContractChecks(unittest.TestCase):
    """Require actual native sixteen-cell source matrices and pure physical-owner status."""

    @classmethod
    def setUpClass(cls):
        """Load the active compiled descriptor, shared source and preserved predecessor identities.

        @param cls Test class retaining the complete native/public processor handoff.
        @return None.
        """
        cls.geometry = processor.load_geometry()
        cls.document = json.loads(shared.MANIFEST.read_text(encoding="utf-8"))
        cls.rows = [row for row in cls.document["assets"] if row["asset_id"] in processor.SELECTED]
        cls.before = json.loads((ROOT / "logs/graphics/processor-source/20261012-geometry-01/before.json").read_text(encoding="utf-8"))

    def test_complete_current_compiled_native_funding_and_presentation_proof(self):
        """One successful seven-case executable run supplies all four actual funded native layouts.

        @param self Active compiled processor source boundary proof.
        @return None.
        """
        proof = self.geometry["native_proof"]
        self.assertEqual(proof["test_cases"], 7)
        self.assertEqual(proof["exit_code"], 0)
        self.assertGreater(proof["assertions"], 0)
        self.assertEqual(processor.digest(ROOT / proof["binary"]), proof["binary_sha256"])
        self.assertEqual(processor.digest(ROOT / proof["log"]), proof["log_sha256"])
        output = (ROOT / proof["log"]).read_text(encoding="utf-8-sig")
        self.assertTrue(output.rstrip().endswith("NATIVE_EXIT_CODE=0"))
        self.assertEqual(output.count("PROCESSOR_ART_GEOMETRY "), 4)
        traces = [json.loads(line.removeprefix("PROCESSOR_ART_GEOMETRY ")) for line in output.splitlines()
                  if line.startswith("PROCESSOR_ART_GEOMETRY ")]
        self.assertEqual(traces, self.geometry["families"])

    def test_selected_public_and_rebuilt_source_use_one_real_sixteen_cell_layout(self):
        """Only four selected draft rotations change; every native source matrix requires512 slots.

        @param self Active compiled processor source boundary proof.
        @return None.
        """
        self.assertEqual(len(self.rows), 4)
        self.assertEqual(shared.validate_manifest(self.document), 42749)
        rebuilt = {row["asset_id"]: row for row in shared.build_manifest()["assets"]}
        # Exact native order determines source identity and later per-tile
        # selection; a guessed square or four painted orientations cannot do so.
        for row in self.rows:
            processor.validate_processor_contract(row)
            processor.validate_processor_contract(rebuilt[row["asset_id"]])
            self.assertEqual(row["directions"], ["none"])
            self.assertEqual(row["native_layout"], 0)
            self.assertEqual(row["native_cells"], [[x, y] for y in range(4) for x in range(4)])
            self.assertEqual(len(row["states"]) * row["frames"] * row["tile_variants"] * len(row["parts"]), 512)
            self.assertEqual(row["source_height_limit"], 20)

    def test_state_contract_keeps_actual_stock_work_and_per_tile_construction_owners(self):
        """Native idle/working/full status and actual per-cell stages retain separate quantity ownership.

        @param self Active compiled processor source boundary proof.
        @return None.
        """
        policy = self.rows[0]["native_state_policy"]
        self.assertEqual(policy["world_states"], ["construction", "ready", "working", "blocked"])
        self.assertEqual(policy["frame_aliases"], {"construction": [0, 1, 2, 3], "ready": [0, 0, 0, 0], "working": [0, 1, 2, 3], "blocked": [0, 0, 0, 0]})
        self.assertIn("GetIndustryConstructionStage", policy["construction"])
        self.assertIn("GetLayerProcessorOperationalStatus", policy["working"])
        self.assertIn("CaptureLayerProcessorInventory", policy["working"])
        self.assertIn("PlanMultiLayerRecipeBatches", policy["working"])
        self.assertIn("independently of banked work", policy["ready"])
        self.assertIn("Industry accepted/produced", policy["quantity_owner"])
        self.assertIn("TimerGameTick::counter", policy["frame_owner"])

    def test_draft_views_missing_cells_shifted_height_and_art_state_owners_refuse(self):
        """Declared source changes must agree with every compiled physical/state field.

        @param self Active compiled processor source boundary proof.
        @return None.
        """
        changes = {"directions": ["NE", "SE", "SW", "NW"], "native_cells": [[0, 0]],
                   "native_layout": 1, "source_height_limit": 48, "native_size": [96, 64],
                   "anchor": [-32, -33], "tile_variants": 15, "native_state_policy": {"working": "art-owned work"}}
        # Each independently forged geometry/state field is refused before
        # originals or packed offsets could consume its false source matrix.
        for key, value in changes.items():
            row = copy.deepcopy(self.rows[0])
            row[key] = value
            with self.subTest(field=key), self.assertRaises(ValueError):
                processor.validate_processor_contract(row)

    def test_partial_duplicate_wrong_native_and_failed_proofs_never_publish(self):
        """Native trace shape cannot promote a failed process or incomplete compiled producer.

        @param self Active compiled processor source boundary proof.
        @return None.
        """
        proof = self.geometry["native_proof"]
        output = (ROOT / proof["log"]).read_text(encoding="utf-8-sig")
        lines = output.splitlines()
        trace = next(line for line in lines if line.startswith("PROCESSOR_ART_GEOMETRY "))
        incomplete = "\n".join(line for line in lines if line != trace)
        wrong = output.replace('"tile_local_id":33280', '"tile_local_id":99999', 1)
        self.assertNotEqual(wrong, output)
        candidates = [output.replace("NATIVE_EXIT_CODE=0", "NATIVE_EXIT_CODE=1"), incomplete,
                      output.replace(trace, trace + "\n" + trace, 1), wrong,
                      output + "\n" + output, output.removesuffix("NATIVE_EXIT_CODE=0\n")]
        before = processor.DESCRIPTOR.read_bytes()
        # Failure must leave the published compiled handoff byte-identical;
        # mocked log reads exercise refusals without creating scratch fixtures.
        for candidate in candidates:
            with mock.patch.object(Path, "read_text", return_value=candidate), self.assertRaises(ValueError):
                processor.publish_geometry(ROOT / proof["log"], ROOT / proof["binary"])
            self.assertEqual(processor.DESCRIPTOR.read_bytes(), before)

    def test_native_identity_missing_cells_and_stale_owner_hashes_refuse(self):
        """Exact primary/IIDS/TIDS lineage and complete current compiled sources are mandatory.

        @param self Active compiled processor source boundary proof.
        @return None.
        """
        candidates = []
        # These separate identities must come from the installed selected
        # family; a successful run cannot authorize foreign primary/layout IDs.
        for field, value in (("layout_count", 4), ("primary_recipe", "RBOT"), ("industry_local_id", 0x8100)):
            changed = copy.deepcopy(self.geometry)
            changed["families"][0][field] = value
            candidates.append(changed)
        missing = copy.deepcopy(self.geometry)
        missing["families"][0]["cells"].pop()
        candidates.append(missing)
        stale = copy.deepcopy(self.geometry)
        stale["native_source_sha256"][processor.OWNERS[0]] = "0" * 64
        candidates.append(stale)
        # Each apparent native proof keeps the success summary but changes a
        # physical identity or source owner; summary-only acceptance is refused.
        for candidate in candidates:
            with self.assertRaises(ValueError):
                processor.validate_geometry(candidate)

    def test_previous_art_binaries_mine_families_and_unselected_public_rows_stay_exact(self):
        """Processor geometry preserves previous selected art, all four binaries and every unrelated row.

        @param self Active compiled processor source boundary proof.
        @return None.
        """
        from convert_mine_art import RECORD, signature
        self.assertEqual(processor.digest(RECORD), self.before["mine_record_sha256"])
        approved = {row["path"]: row for row in self.before.get("approved_boundary_updates", [])}
        # Named producer-code changes are separately recorded with their old
        # identities; source pixels, packed data and prior binaries stay exact.
        for row in self.before["preserved_identities"]:
            expected = approved.get(row["path"], row)
            self.assertEqual(processor.digest(ROOT / row["path"]), expected["sha256"], row["path"])
        for row in self.document["assets"]:
            if row["asset_id"] not in processor.SELECTED:
                self.assertEqual(signature(row), self.before["public_family_sha256"][row["asset_id"]], row["asset_id"])
        for asset_id in ("facility.component-works", "facility.robotics-works", "facility.mirror-kit-works"):
            with self.assertRaisesRegex(ValueError, "outside selected"):
                processor.processor_contract(asset_id)
