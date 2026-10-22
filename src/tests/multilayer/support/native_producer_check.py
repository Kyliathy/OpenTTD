"""Verify explicit producer attribution through the actual shared child transport.

@file native_producer_check.py Real Python child only; no graphical/native-game qualification.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group19B.
"""

import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import uuid

from artifact_paths import RunContext, contained, create_run_context
from runner import run_native

ROOT = Path(__file__).resolve().parents[4]


class NativeProducerChecks(unittest.TestCase):
    """Preserve actual delegated ownership without hiding the transport entry point."""

    def test_real_child_records_explicit_owner_and_actual_entrypoint(self):
        """Use one real child to distinguish this producer from its unittest launcher.

        @param self Active child-transport attribution check.
        @return None; the temporary record proves transport only, never native artwork.
        """
        context = create_run_context("routing-checks", "native-producer", run_id="owner-check-" + uuid.uuid4().hex,
                                     root=ROOT, profile="routing-checks")
        self.addCleanup(self.remove_context, context)
        cache = ROOT / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="native-owner-child-", dir=cache) as temporary:
            child = Path(temporary) / "transport-child.py"
            child.write_text("# @file transport-child.py; @see r2fc-file-cleanup3.md19B; transport fixture only.\n"
                             "from pathlib import Path\nimport sys\n"
                             "Path(sys.argv[1]).write_text('{\"_documentation\":\"Owner: r2fc-file-cleanup3.md19B; Python transport fixture, no artwork\"}', encoding='utf-8')\n"
                             "print('explicit owner transport')\n", encoding="utf-8")
            owner = Path(__file__).resolve().relative_to(ROOT).as_posix()
            observation = context.reports / "native-transport-observation.json"
            result = run_native(context, [sys.executable, "-B", str(child), str(observation)], native_producer=owner,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60, check=True)
        document = json.loads((ROOT / result.native_report["path"]).read_text(encoding="utf-8"))
        self.assertEqual(document["producer"], owner)
        entrypoint = Path(sys.argv[0]).resolve()
        expected = entrypoint.relative_to(ROOT).as_posix() if entrypoint.is_relative_to(ROOT) else "external-caller"
        self.assertEqual(document["entrypoint"], expected)
        self.assertEqual(document["exit_status"], 0)
        self.assertIn(b"explicit owner transport", result.stdout)
        self.assertNotIn("native_loading_verified", document)
        self.assertTrue(document["transcripts"])
        self.assertIn(observation.relative_to(ROOT).as_posix(), [item["path"] for item in document["artifacts"]])

    def test_outside_or_non_source_owner_refuses_before_child_launch(self):
        """Reject an invalid producer hint before native transport can make a claim.

        @param self Active source-ownership refusal check.
        @return None; no run reservation, child or report is created.
        """
        context = RunContext(ROOT, "routing-checks", "native-producer", "unreserved-owner-refusal")
        # Each invalid actor breaks a distinct ownership boundary. A caller
        # cannot attribute execution to an outside file or repository document.
        for owner in ("../foreign.py", "CMakeLists.txt", "missing-producer.py"):
            with self.subTest(owner=owner), patch("subprocess.run", side_effect=AssertionError("invalid producer launched")):
                with self.assertRaises(ValueError):
                    run_native(context, [sys.executable, "-V"], native_producer=owner)

    def remove_context(self, context):
        """Remove only this stopped test's exact repository-local scratch owners.

        @param self Active isolated transport test.
        @param context This test's uniquely reserved output/runtime/log invocation.
        @return None; resolved containment is checked before every recursive removal.
        """
        paths = [context.directory, context.runtime_dir, context.logs,
                 ROOT / "mlm/runtime/.runs" / context.run_id]
        # Every owner was created by this test and its sole child has exited.
        # Never remove shared feature/profile parents or another invocation.
        for path in paths:
            target = contained(path, ROOT)
            if target.exists():
                shutil.rmtree(target)


if __name__ == "__main__":
    unittest.main()
