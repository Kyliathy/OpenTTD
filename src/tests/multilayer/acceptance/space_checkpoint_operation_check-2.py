"""Bounded new-operation configuration, provenance and closed-scope refusals.

@file space_checkpoint_operation_check-2.py Synthetic transport, no application execution.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group12B.
"""

import copy
import json
import unittest

from space_checkpoint_operation_fixture import OperationFixture
import space_checkpoint_operation as owner


class OperationRefusalChecks(unittest.TestCase):
    """Reject relabeled historical receipts and changed actual process/source evidence.

    @see r2fc-file-cleanup3.md Group12B closed financial scope contract.
    """

    def test_scope_configuration_process_and_source_failures(self):
        """Exercise independent failure boundaries before any successor is published.

        @param self Active isolated repository-local test.
        @return None; failed publication leaves no selectable operation receipt.
        """
        fixture = OperationFixture(self)
        configuration = "RelWithDebInfo"
        original = copy.deepcopy(fixture.documents[configuration])
        path = fixture.root / fixture.operations[configuration]["path"]
        # A historical paid stage, wrong world, load-only tag or reopened policy
        # must fail before the new task can claim any business validation.
        for field, value in (("phase", "construction-cash-work"), ("configuration", "Debug"),
                             ("task", "Plan11/15CSGZVEEH"), ("tag", "[.space-checkpoint-load]"),
                             ("financial_policy", dict(owner.POLICY, further_paid_scope_authorized=True))):
            changed = dict(original, **{field: value})
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "scope"):
                owner._result_shape(changed, "Plan11/15CSGZVEEI", configuration)
        with self.assertRaisesRegex(TypeError, "validator"):
            fixture.publish(None)
        changed = dict(original, source=fixture.documents["Debug"]["source"])
        fixture.operations[configuration] = fixture.base.write(path, changed)
        with self.assertRaisesRegex(ValueError, "different predecessor"):
            fixture.publish()
        process_path = fixture.root / original["native_process"]["path"]
        process = json.loads(process_path.read_text(encoding="utf-8"))
        process["exit_status"] = 7
        changed = dict(original, native_process=fixture.base.write(process_path, process))
        fixture.operations[configuration] = fixture.base.write(path, changed)
        with self.assertRaisesRegex(ValueError, "actual process"):
            fixture.publish()
        fixture.operations[configuration] = fixture.base.write(path, original)
        saved = fixture.predecessor.manifest_path.read_bytes()
        fixture.predecessor.manifest_path.write_bytes(saved + b" ")
        with self.assertRaisesRegex(ValueError, "changed since typed"):
            fixture.publish()
        fixture.predecessor.manifest_path.write_bytes(saved)
        self.assertFalse(fixture.destination.exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
