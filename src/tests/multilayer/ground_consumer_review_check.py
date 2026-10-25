"""Focused independent Ground tuple, family ownership and live callback refusals.

@file ground_consumer_review_check.py Real retained bytes; no synthetic native acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20.
"""
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_rebuild import identity
from run_ground_consumer_checks import verify_consumer_inputs


class GroundConsumerChecks(unittest.TestCase):
    """Keep frozen source facts separate from current native drawing and gameplay acceptance."""

    def setUp(self):
        """Copy only the small immutable source expectations into owned repository scratch.

        @param self Active focused unittest owner.
        """
        cache = ROOT / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.root = Path(tempfile.mkdtemp(prefix="ground-consumer-", dir=cache))
        roles = {"pose-fixture": "src/tests/fixtures/graphics/ground-pose-tuples.bin",
                 "consumer-expectations": "src/tests/fixtures/graphics/ground-consumer-expectations.json",
                 "dispatch": "media/multilayer/contracts/ground-consumer-dispatch.json",
                 "family-ownership": "src/tests/fixtures/graphics/merge-asset-families.json"}
        # Every role keeps its original repository-relative identity; these are
        # independent inputs, never a made-up successful native process/report.
        for relative in roles.values():
            target = self.root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((ROOT / relative).read_bytes())
        self.inputs = {role: identity(self.root / relative, self.root) for role, relative in roles.items()}
        dispatch = json.loads((ROOT / roles["dispatch"]).read_text(encoding="utf-8"))
        sources = {"src/multilayer_sprite.cpp", "src/ground_station_sprite.cpp", "src/ground_sprite.h"}
        # Copy the real production bodies into explicit scratch. Refusal tests
        # can break one edge without editing the checkout or fabricating native execution.
        for row in dispatch["families"]:
            sources.add(row["draw_owner"])
            sources.update(edge["source"] for edge in row.get("native_chain", []))
            if "binding_header" in row:
                sources.add(row["binding_header"].replace(".h", ".cpp"))
        for relative in sources:
            target = self.root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((ROOT / relative).read_bytes())

    def write_dispatch(self, dispatch):
        """Persist a detached dispatch change with its truthful current checksum.

        @param self Active focused unittest owner.
        @param dispatch Modified scratch dispatch; immutable original facts stay unchanged.
        """
        path = self.root / self.inputs["dispatch"]["path"]
        path.write_text(json.dumps(dispatch), encoding="utf-8")
        self.inputs["dispatch"] = identity(path, self.root)

    def test_original_domain_and_implemented_scope_are_complete(self):
        """Verify all47current Ground callbacks without rewriting historical26deferred facts.

        @param self Active focused unittest owner.
        """
        actual = verify_consumer_inputs(dict(inputs=self.inputs), self.root)
        self.assertEqual(actual["source_poses"], 14254)
        self.assertEqual(actual["dispatch_counts"], dict(current_bound=47, current_existing=3, later_owner=0))
        self.assertEqual(actual["historical_dispatch_counts"], dict(current_bound=21, current_existing=3, later_owner=26))
        self.assertEqual(len(actual["live_native_bindings"]), 47)
        self.assertEqual(actual["imported_native_families"], 26)
        self.assertFalse(actual["viewport_claimed"])
        self.assertFalse(actual["whole_underground_complete"])

    def test_rehashed_changed_tuple_cannot_create_its_own_baseline(self):
        """Refuse modified tuple bytes even after both declared input hashes are replaced.

        @param self Active focused unittest owner.
        """
        path = self.root / self.inputs["pose-fixture"]["path"]
        changed = bytearray(path.read_bytes())
        changed[-1] ^= 1
        path.write_bytes(changed)
        self.inputs["pose-fixture"] = identity(path, self.root)
        contract_path = self.root / self.inputs["consumer-expectations"]["path"]
        contract = json.loads(contract_path.read_text(encoding="utf-8"))
        contract["fixture"].update(self.inputs["pose-fixture"])
        contract_path.write_text(json.dumps(contract), encoding="utf-8")
        self.inputs["consumer-expectations"] = identity(contract_path, self.root)
        with self.assertRaisesRegex(ValueError, "original tuple bytes"):
            verify_consumer_inputs(dict(inputs=self.inputs), self.root)

    def test_rehashed_dispatch_cannot_swap_m1_and_ground_owners(self):
        """Refuse count-preserving ownership changes despite a truthful dispatch checksum.

        @param self Active focused unittest owner.
        """
        path = self.root / self.inputs["dispatch"]["path"]
        dispatch = json.loads(path.read_text(encoding="utf-8"))
        ground = next(row for row in dispatch["families"] if "native_chain" in row)
        m1 = next(row for row in dispatch["families"] if row["dispatch"] == "current_existing")
        ground["dispatch"], m1["dispatch"] = m1["dispatch"], ground["dispatch"]
        self.write_dispatch(dispatch)
        with self.assertRaisesRegex(ValueError, "family ownership"):
            verify_consumer_inputs(dict(inputs=self.inputs), self.root)

    def test_imported_family_requires_a_connected_chain(self):
        """Refuse metadata-only ownership when its actual native chain is absent.

        @param self Active focused unittest owner.
        """
        dispatch = json.loads((self.root / self.inputs["dispatch"]["path"]).read_text())
        next(row for row in dispatch["families"] if "native_chain" in row)["native_chain"] = []
        self.write_dispatch(dispatch)
        with self.assertRaisesRegex(ValueError, "connected native callback"):
            verify_consumer_inputs(dict(inputs=self.inputs), self.root)

    def test_real_unrelated_body_cannot_qualify_a_callback(self):
        """Refuse a real definition that does not call the next drawing owner.

        @param self Active focused unittest owner.
        """
        dispatch = json.loads((self.root / self.inputs["dispatch"]["path"]).read_text())
        row = next(row for row in dispatch["families"] if "native_chain" in row)
        row["native_chain"][0] = dict(source="src/ground_sprite.h", symbol="GetGroundSprite")
        self.write_dispatch(dispatch)
        with self.assertRaises(AssertionError):
            verify_consumer_inputs(dict(inputs=self.inputs), self.root)

    def test_copied_test_source_cannot_qualify_production_drawing(self):
        """Refuse byte-identical callback bodies supplied from the fixture/test tree.

        @param self Active focused unittest owner.
        """
        dispatch = json.loads((self.root / self.inputs["dispatch"]["path"]).read_text())
        edge = next(row for row in dispatch["families"] if "native_chain" in row)["native_chain"][0]
        copied = self.root / "src/tests/copied-renderer.cpp"
        copied.write_bytes((self.root / edge["source"]).read_bytes())
        edge["source"] = "src/tests/copied-renderer.cpp"
        self.write_dispatch(dispatch)
        with self.assertRaisesRegex(ValueError, "production native source"):
            verify_consumer_inputs(dict(inputs=self.inputs), self.root)


if __name__ == "__main__":
    unittest.main()
