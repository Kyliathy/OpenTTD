"""Focused durable foundation export and next-qualification transport checks.

@file space_checkpoint_export_check-1.py Synthetic transport using independent original worlds.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group22.
@see docs/zz-reach2/architecture/archi-historical-debug.md.
"""
import json
import shutil
import unittest
from unittest.mock import patch

from space_checkpoint import export_foundation, load_foundation, verify_foundation
from artifact_paths import create_run_context
from space_checkpoint_foundation_fixture import FoundationFixture
import space_checkpoint_foundation as owner


class DurableFoundationChecks(unittest.TestCase):
    """Keep both retained worlds verifiable after generated proof and diagnostics disappear."""

    def test_export_is_idempotent_and_independent_of_generated_roots(self):
        """Export exact compact roles, requalify selected retained inputs, then discard fixture products.

        @param self Isolated synthetic continuation transport test owner.
        """
        fixture = FoundationFixture(self)
        foundation = fixture.qualify()
        exported = export_foundation(foundation)
        self.assertEqual(export_foundation(foundation).manifest_path, exported.manifest_path)
        self.assertEqual(export_foundation(exported).manifest_path, exported.manifest_path)
        # Removing optional diagnostics cannot alter a subsequent qualification
        # of these unchanged synthetic current source/input/image witnesses.
        diagnostics = (fixture.root / "logs").resolve()
        self.assertTrue(diagnostics.is_relative_to(fixture.root.resolve()))
        if diagnostics.exists():
            shutil.rmtree(diagnostics)
        context = create_run_context("space", "checkpoint-qualification-next", root=fixture.root)
        following = owner.qualify_checkpoint(owner.load_checkpoint(root=fixture.root), fixture.native_results,
            fixture.ordinary_results, fixture.build_results, context.reports / "foundation.json")
        self.assertFalse(verify_foundation(following)["authorization_granted"])
        # This fixture explicitly owns every removed tree; the live repository
        # and any external worktree are outside these resolved deletion targets.
        for relative in ("mlm/reviews", "mlm/runtime", "build"):
            target = (fixture.root / relative).resolve()
            self.assertTrue(target.is_relative_to(fixture.root.resolve()))
            if target.exists():
                shutil.rmtree(target)
        with patch("subprocess.Popen", side_effect=AssertionError("export verification launched native process")):
            facts = verify_foundation(load_foundation(exported.manifest_path, root=fixture.root))
        self.assertFalse(facts["historical_images_required"])
        self.assertEqual(facts["next_task"], "Plan11/15CSGZVEEI")
        self.assertEqual(len(facts["configurations"]), 2)
        document = json.loads(exported.manifest_path.read_text())
        terminal = fixture.root / document["configurations"]["Debug"]["witnesses"]["native_terminal"]["path"]
        terminal.write_bytes(terminal.read_bytes() + b"changed")
        with self.assertRaisesRegex(ValueError, "input changed"):
            verify_foundation(exported)


if __name__ == "__main__":
    unittest.main()
