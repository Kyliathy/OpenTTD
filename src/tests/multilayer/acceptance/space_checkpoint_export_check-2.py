"""Focused chained successor export with original process paths retained as history.

@file space_checkpoint_export_check-2.py Synthetic zero-tick query transport, no gameplay proof.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group22.
@see docs/zz-reach2/architecture/archi-historical-debug.md.
"""
import json
import shutil
import unittest

from space_checkpoint import export_foundation, verify_foundation
from space_checkpoint_operation_fixture import OperationFixture
import space_checkpoint_foundation as owner


class DurableSuccessorChecks(unittest.TestCase):
    """Preserve actual original source/path facts while relocating the selected physical lineage."""

    def test_two_successors_export_their_complete_selected_lineage(self):
        """Keep source/save/process identity joins across two exported operation receipts.

        @param self Isolated synthetic successor transport test owner.
        """
        fixture = OperationFixture(self)
        first = fixture.publish()
        selected = verify_foundation(first)
        # A second independently declared synthetic query consumes the exact
        # first selected output; matching original-world bytes alone do not
        # establish that required predecessor path/role relationship.
        for configuration, operation in fixture.documents.items():
            operation["source"] = selected["configurations"][configuration]["source"]
            process_path = fixture.root / operation["native_process"]["path"]
            process = json.loads(process_path.read_text())
            process["inputs"] = [operation["source"]]
            operation["native_process"] = fixture.base.write(process_path, process)
            fixture.operations[configuration] = fixture.base.write(fixture.root / fixture.operations[configuration]["path"], operation)
        fixture.predecessor = first
        fixture.destination = fixture.context.reports / "second-operation.json"
        successor = fixture.publish()
        exported = export_foundation(successor)
        before = verify_foundation(exported)
        # Discard only this fixture's generated evidence. Durable ancestors and
        # saved bytes must remain sufficient without the historical applications.
        for relative in ("mlm/reviews", "mlm/runtime", "logs", "build"):
            target = (fixture.root / relative).resolve()
            self.assertTrue(target.is_relative_to(fixture.root.resolve()))
            if target.exists():
                shutil.rmtree(target)
        self.assertEqual(verify_foundation(exported), before)
        self.assertFalse(before["authorization_granted"])
        self.assertFalse(before["historical_images_required"])
        document = json.loads(exported.manifest_path.read_text())
        original = owner.read(document["configurations"]["Debug"]["witnesses"]["operation_result"], fixture.root)
        self.assertNotEqual(original["source"]["path"], document["configurations"]["Debug"]["source"]["path"])
        self.assertEqual(original["source"]["sha256"], document["configurations"]["Debug"]["source"]["sha256"])


if __name__ == "__main__":
    unittest.main()
