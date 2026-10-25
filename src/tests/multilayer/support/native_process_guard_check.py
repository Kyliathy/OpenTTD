"""Check actual owned-process failure evidence and Python/native routing refusals.

@file native_process_guard_check.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Persistent fixture outputs belong to test-cache/merge-harness or the explicit
RunContext. This suite never deletes fixtures or stops a discovered process.
"""
import ctypes
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

from artifact_paths import create_run_context
import runner

ROOT = Path(__file__).resolve().parents[4]
CHILD = Path(__file__).with_name("native_process_guard_child.py")
PROBE = ROOT / "test-cache/cleanup/artifact_paths_probe.exe"


def process_alive(pid):
    """Observe one PID recorded by our own fixture without signalling it.

    @param pid Exact process ID emitted by the actually launched fixture root.
    @return Whether its Windows process handle is still unsignalled.
    """
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = (ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong)
    kernel.OpenProcess.restype = ctypes.c_void_p
    kernel.WaitForSingleObject.argtypes = (ctypes.c_void_p, ctypes.c_ulong)
    kernel.CloseHandle.argtypes = (ctypes.c_void_p,)
    handle = kernel.OpenProcess(0x00100000, False, pid)
    if not handle:
        return False
    try:
        return kernel.WaitForSingleObject(handle, 2000) == 0x102
    finally:
        kernel.CloseHandle(handle)


class NativeProcessGuards(unittest.TestCase):
    """Retain genuine child failures while isolating unrelated processes and homes."""

    def setUp(self):
        """Reserve a fresh invocation with actual current process ownership.

        @param self Active process/routing test instance.
        """
        self.context = create_run_context("merge-harness", self._testMethodName.replace("_", "-"))

    def report(self, result):
        """Read the exact process report attached to a result or raised failure.

        @param self Current case whose explicit invocation owns the report.
        @param result Actual CompletedProcess or subprocess exception.
        @return Machine-readable current process observation.
        """
        return json.loads((ROOT / result.native_report["path"]).read_text(encoding="utf-8"))

    def test_nonzero_child_retains_separate_transcripts_and_reaped_exit(self):
        """An ordinary explicit failure remains inspectable without becoming success.

        @param self Active actual failing-child check.
        """
        result = runner.run_native(self.context, [sys.executable, CHILD, "exit"],
                                   capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 7)
        report = self.report(result)
        self.assertTrue(report["root_reaped"])
        self.assertFalse(report["timed_out"])
        self.assertEqual(len(report["transcripts"]), 2)
        self.assertIn("explicit fixture failure", result.stdout)
        self.assertTrue(report["producer_source"])

    def test_check_failure_is_raised_only_after_required_report(self):
        """Preserve subprocess check semantics and its actual failing process report.

        @param self Active raised-failure evidence check.
        """
        with self.assertRaises(subprocess.CalledProcessError) as failure:
            runner.run_native(self.context, [sys.executable, CHILD, "exit"],
                              check=True, capture_output=True, text=True, timeout=10)
        self.assertEqual(failure.exception.returncode, 7)
        self.assertTrue(self.report(failure.exception)["root_reaped"])

    def test_failed_root_also_retires_its_remaining_descendant(self):
        """A nonzero root cannot leave its child alive after the process result is returned.

        @param self Active actual failed-root/remaining-leaf lifecycle check.
        """
        pid_file = self.context.output("data", "failed-owned-pids.json")
        result = runner.run_native(self.context, [sys.executable, CHILD, "tree-fail", "--pid-file", pid_file],
                                   capture_output=True, text=True, timeout=10)
        pids = json.loads(pid_file.read_text())
        self.assertEqual(result.returncode, 7)
        self.assertFalse(process_alive(pids["child"]))
        self.assertTrue(self.report(result)["root_reaped"])

    def test_timeout_retires_owned_descendant_and_preserves_unrelated_sentinel(self):
        """Kill our live timed-out tree while a separately launched observer stays alive.

        @param self Active real Windows descendant-isolation check.
        """
        sentinel = subprocess.Popen([sys.executable, str(CHILD), "sleep", "--seconds", "30"],
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                    creationflags=subprocess.CREATE_NO_WINDOW)
        pid_file = self.context.output("data", "owned-pids.json")
        try:
            with self.assertRaises(subprocess.TimeoutExpired) as failure:
                runner.run_native(self.context, [sys.executable, CHILD, "tree", "--pid-file", pid_file],
                                  capture_output=True, text=True, timeout=2)
            pids = json.loads(pid_file.read_text(encoding="utf-8"))
            self.assertFalse(process_alive(pids["root"]))
            self.assertFalse(process_alive(pids["child"]))
            self.assertIsNone(sentinel.poll())
            record = self.report(failure.exception)
            self.assertTrue(record["root_reaped"])
            self.assertTrue(record["timed_out"])
            self.assertEqual(len(record["transcripts"]), 2)
        finally:
            # This distinct sentinel is also ours, but is never included in the
            # timed-out native tree. Retire its own handle only after assertions.
            if sentinel.poll() is None:
                sentinel.terminate()
            sentinel.wait(timeout=10)

    def test_native_runtime_wrong_root_and_output_escape_refuse(self):
        """The actual native path owner refuses foreign homes and traversing output names.

        @param self Active compiled routing refusal check; no game/decoder runs.
        """
        with self.assertRaises(ValueError):
            self.context.for_process("../../foreign")
        with self.assertRaises(ValueError):
            self.context.output("reports", "../original.json")
        environment = self.context.environment()
        environment["OTTD_TEST_RUNTIME_DIR"] = str(self.context.for_process("foreign").runtime_dir)
        wrong_runtime = subprocess.run([str(PROBE), "paths"], env=environment, capture_output=True,
                                       text=True, timeout=10, creationflags=subprocess.CREATE_NO_WINDOW)
        self.assertNotEqual(wrong_runtime.returncode, 0)
        self.assertIn("runtime directory disagrees", wrong_runtime.stderr)
        environment["OTTD_TEST_REPOSITORY"] = str(ROOT / "test-cache/merge-harness/not-a-source-root")
        wrong_root = subprocess.run([str(PROBE), "paths"], env=environment, capture_output=True,
                                    text=True, timeout=10, creationflags=subprocess.CREATE_NO_WINDOW)
        self.assertNotEqual(wrong_root.returncode, 0)
        environment = self.context.environment()
        escape = subprocess.run([str(PROBE), "output", "reports", "../original.json"], env=environment,
                                capture_output=True, text=True, timeout=10, creationflags=subprocess.CREATE_NO_WINDOW)
        self.assertNotEqual(escape.returncode, 0)

    def test_changed_producer_is_failed_evidence_with_original_identity(self):
        """A current source edit cannot certify an already executing native observer.

        @param self Active source-integrity refusal using a generated fixture only.
        """
        folder = ROOT / "test-cache/merge-harness" / self.context.run_id
        folder.mkdir(parents=True)
        producer = folder / "fixture_producer.py"
        producer.write_text('"""@see r2fc-file-cleanup3.md; integrity-only fixture, no functions."""\n', encoding="utf-8")
        actual = runner._run_owned_process

        def execute(arguments, options):
            """Run the real child, then simulate a source edit before evidence is finalized.

            @param arguments Original structured failing-child invocation.
            @param options Explicit current process streams/environment/budget.
            @return The actual real child result; no success status is fabricated.
            """
            result = actual(arguments, options)
            producer.write_text(producer.read_text() + "# Source changed during observation.\n", encoding="utf-8")
            return result

        with patch.object(runner, "_run_owned_process", side_effect=execute):
            with self.assertRaisesRegex(ValueError, "changed during execution"):
                runner.run_native(self.context, [sys.executable, CHILD, "exit"], native_producer=producer,
                                  capture_output=True, text=True, timeout=10)
        records = list(self.context.reports.glob("native-*.json"))
        self.assertEqual(len(records), 1)
        record = json.loads(records[0].read_text())
        self.assertFalse(record["input_integrity_passed"])
        self.assertTrue(record["root_reaped"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
