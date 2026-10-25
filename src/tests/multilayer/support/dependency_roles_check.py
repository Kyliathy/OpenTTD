"""Check typed live dependencies without treating historical quotations as inputs.

@file dependency_roles_check.py Current roles, cold aliases and bounded atomic retry.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group28.
@see docs/zz-reach2/architecture/archi-historical-debug.md Historical facts and current proof.
All file mutations use disposable repository-local fixtures; no native game runs.
"""

import ast
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import artifact_rebuild as rebuild
from artifact_paths import repository_root
from artifact_recipes import check_reference_roles


class DependencyRoleChecks(unittest.TestCase):
    """Keep diagnostic/history facts distinct from physical continuation dependencies.

    @param self Active fixture owner beneath this repository's test-cache.
    """

    def setUp(self):
        """Reserve an isolated checkout and independent retained/output payloads.

        @param self Active isolated test.
        @return None; fixtures persist in this reserved scratch root without recursive cleanup.
        """
        cache = repository_root() / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.root = Path(tempfile.mkdtemp(prefix="dependency-roles-", dir=cache))
        self.write("CMakeLists.txt", b"# @see r2fc2-file-cleanup3.md; isolated role fixture.\n")
        self.expected = self.write("src/tests/fixtures/expected.bin", b"independent expected bytes")
        self.output = self.write("mlm/reviews/fixture/current/selected/data/current.bin", b"actual current bytes")

    def write(self, relative, data):
        """Write independent bytes into one explicit disposable fixture path.

        @param self Active isolated fixture owner.
        @param relative Fixture-relative source/input/output spelling.
        @param data Independently supplied bytes, never inferred expected output.
        @return Written fixture path.
        """
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def reference(self, path, role=None):
        """Describe one actual fixture file with an optional explicit reference role.

        @param self Active isolated fixture owner.
        @param path Existing independent fixture file.
        @param role Optional physical/history role annotation.
        @return Exact actual path/hash record, never a native acceptance claim.
        """
        row = rebuild.identity(path, self.root)
        if role is not None:
            row["role"] = role
        return row

    def current(self):
        """Create one typed current envelope with independent expected/output roles.

        @param self Active isolated fixture owner.
        @return Mutable current manifest used only by this test.
        """
        return dict(kind="artifact_recipe_stage_result", run_id="selected",
                    inputs=[self.reference(self.expected)], outputs=[self.reference(self.output)])

    def test_diagnostics_and_historical_quotations_need_no_old_files(self):
        """Only schema-owned live roles are dereferenced; logs remain valid diagnostics.

        @param self Active isolated role boundary check.
        @return None; vanished historical EXEs/logs and unwritten diagnostics are accepted.
        """
        document = self.current()
        document.update(diagnostics=[dict(path="logs/current.log")],
            historical_origin=dict(path="logs/vanished/old.exe", sha256="0" * 64),
            facts=dict(quoted=dict(role="historical_fact", path="C:/old-workstation/old.exe"),
                       diagnostic=dict(role="diagnostic", path="test-cache/optional.txt")),
            notes="A quoted logs/old.log string is not a dependency.")
        self.assertEqual(check_reference_roles(document, self.root), 2)
        self.assertFalse((self.root / "logs").exists())

    def test_nested_receipt_live_log_dependency_is_rejected(self):
        """A nested current receipt cannot hide a physical required input in old logs.

        @param self Active isolated role boundary check.
        @return None; the existing old file is refused despite an exact valid hash.
        """
        old = self.write("logs/old/required.json", b'{"old":"receipt"}')
        document = self.current()
        document["facts"] = dict(nested_receipt=dict(inputs=[self.reference(old)]))
        with self.assertRaisesRegex(ValueError, "inside deletion roots"):
            check_reference_roles(document, self.root)

    def test_stale_executable_and_fixture_output_roles_refuse(self):
        """Existing archived bytes cannot be current and fixtures cannot be outputs.

        @param self Active isolated role boundary check.
        @return None; three image owners and the immutable fixture are checked separately.
        """
        for relative in ("logs/old/retained.exe", "build/archive/retained.exe", "mlm/runtime/native/old-run/main/openttd_console.exe"):
            document = self.current()
            document["application"] = self.reference(self.write(relative, b"same historical image bytes"))
            with self.subTest(relative=relative), self.assertRaisesRegex(ValueError, "stale archived"):
                check_reference_roles(document, self.root)
        document = self.current()
        document["outputs"] = [self.reference(self.expected)]
        with self.assertRaisesRegex(ValueError, "fixture overwrite"):
            check_reference_roles(document, self.root)
        self.assertEqual(self.expected.read_bytes(), b"independent expected bytes")

    def test_selected_prerequisite_requires_exact_supplier_and_invocation(self):
        """An explicitly prepared input must belong to this selected graph and run.

        @param self Active isolated prerequisite check.
        @return None; neither a missing supplier nor another run can be substituted.
        """
        reference = self.reference(self.output, "generated_prerequisite")
        document = self.current()
        document["inputs"] = [reference]
        with self.assertRaisesRegex(ValueError, "unbound generated"):
            check_reference_roles(document, self.root)
        self.assertEqual(check_reference_roles(document, self.root, [reference]), 1)
        with self.assertRaisesRegex(ValueError, "unbound generated"):
            check_reference_roles(document, self.root, [reference], run_id="other")

    def test_selected_cross_run_input_requires_exact_validated_supplier(self):
        """Allow precise checked source bytes while keeping outputs in the capture run.

        @param self Active isolated transport check; no native evidence is fabricated.
        """
        source = self.reference(self.output, "generated_prerequisite")
        capture = self.write("mlm/reviews/fixture/current/capture/data/frame.bin", b"current capture bytes")
        document = dict(kind="artifact_recipe_stage_result", run_id="capture",
                        inputs=[source], outputs=[self.reference(capture)])
        key = (source["path"], source["sha256"])
        with self.assertRaisesRegex(ValueError, "unbound generated"):
            check_reference_roles(document, self.root, [source])
        self.assertEqual(check_reference_roles(document, self.root, [source], generated_runs={key: "selected"}), 2)
        with self.assertRaisesRegex(ValueError, "unbound generated"):
            check_reference_roles(document, self.root, [source], generated_runs={key: "different"})
        with self.assertRaisesRegex(ValueError, "exact selected prerequisite"):
            check_reference_roles(document, self.root, [source], generated_runs={(source["path"], "0" * 64): "selected"})
        # Even equal approved source bytes cannot be relabelled as this run's
        # output, preventing a capture from acquiring a source overwrite owner.
        document["outputs"] = [self.reference(self.output)]
        with self.assertRaisesRegex(ValueError, "unbound generated"):
            check_reference_roles(document, self.root, [source], generated_runs={key: "selected"})

    def test_missing_or_corrupt_expected_input_is_never_regenerated(self):
        """Independent expectations refuse byte damage or absence without repair.

        @param self Active isolated expectation check.
        @return None; the guard never copies current actual output over expected input.
        """
        document = self.current()
        self.expected.write_bytes(b"corrupt independent expected")
        with self.assertRaisesRegex(ValueError, "corrupt live"):
            check_reference_roles(document, self.root)
        self.assertEqual(self.expected.read_bytes(), b"corrupt independent expected")
        self.expected.unlink()
        with self.assertRaisesRegex(ValueError, "missing live"):
            check_reference_roles(document, self.root)
        self.assertFalse(self.expected.exists())

    def test_cold_recipe_inspection_uses_retained_alias_without_old_original(self):
        """Removing the old generated input does not disable its retained-first recipe.

        @param self Active isolated catalogue and alias owner.
        @return None; inspection creates no generated folders and reports executable=True.
        """
        retained = self.write("src/tests/fixtures/contract.json", b'{"independent":"contract"}')
        logical = "mlm/reviews/old/fixture/old-run/saves/contract.json"
        reference = self.reference(retained)
        self.write("src/tests/fixtures/multilayer-inputs.json", json.dumps(dict(schema_version=2,
                   alias_precedence="retained_before_existing", inputs={logical: reference})).encode())
        producer = self.write("src/producer.py", b'"""@file producer.py @see r2fc2-file-cleanup3.md; inspection fixture only."""\n')
        recipe = dict(id="fixture", producer=producer.relative_to(self.root).as_posix(), feature="fixture", scenario="current",
            target=None, test_tags=[], tag_sources=[], execution="pure_conversion", enabled=True, pending_groups=[],
            publishes_source=False, stages=["inspect", "build", "prepare", "verify"], configuration_required=False,
            configuration_reuse="source_bytes_only", comparison="independent_source_bytes", tools=[],
            retained_inputs=[dict(role="contract", path=logical, sha256=reference["sha256"])], requires=[],
            outputs=[dict(role="manifest", path="reports/current.json")], original_parameters={}, native_timeout_seconds=60)
        self.write("mlm/review-producers.json", json.dumps(dict(schema_version=2, recipe_schema_version=1, recipes=[recipe], producers=[])).encode())
        with patch.object(Path, "mkdir", side_effect=AssertionError("inspection writes")):
            selection = rebuild.load_recipe("fixture", root=self.root)
        self.assertTrue(selection["executable"])
        self.assertEqual(selection["missing_inputs"], [])
        self.assertFalse((self.root / logical).exists())

    def test_atomic_report_retry_is_bounded_and_preserves_previous_state(self):
        """Transient Windows replacement errors retry; persistent errors cannot publish.

        @param self Active isolated report owner.
        @return None; each successful document is complete and failures keep prior bytes.
        """
        path = self.write("mlm/reviews/fixture/current/selected/reports/state.json", b'{"complete":false}')
        original_replace = Path.replace
        attempts = []

        def transient(source, destination):
            """Simulate two Windows sharing/access failures, then real atomic replacement.

            @param source Completed temporary report path.
            @param destination Invocation-owned mutable state path.
            @return Actual published path on the third attempt.
            """
            attempts.append(destination)
            if len(attempts) <= 2:
                error = PermissionError("injected transient access failure")
                error.winerror = 5
                raise error
            return original_replace(source, destination)

        with patch.object(Path, "replace", transient), patch.object(rebuild.time, "sleep") as sleep:
            rebuild.write_state(path, dict(complete=True))
        self.assertEqual(len(attempts), 3)
        self.assertEqual(sleep.call_count, 2)
        self.assertEqual(json.loads(path.read_text()), dict(complete=True))
        previous = path.read_bytes()
        error = PermissionError("persistent sharing refusal")
        error.winerror = 32
        with patch.object(Path, "replace", side_effect=error) as replace, patch.object(rebuild.time, "sleep"):
            with self.assertRaises(PermissionError):
                rebuild.write_state(path, dict(complete=False))
        self.assertEqual(replace.call_count, 4)
        self.assertEqual(path.read_bytes(), previous)


class LegacyCacheEntryChecks(unittest.TestCase):
    """Check actual retired CLI ordering and actual migrated reader adapter declarations.

    @param self Read-only observer of current source; no old publication is invoked.
    """

    def test_retired_publications_refuse_before_importing_cache_readers(self):
        """Actual legacy CLIs stop before imports, writes or historical EXE collection.

        @param self Active read-only source/CLI check.
        @return None; nonzero exit and explicit retirement are required for each owner.
        """
        root = repository_root()
        for name in ("launch_fleet_runtime_publication.py", "launch_craft_window_publication.py", "ground_station_offset_publication.py"):
            path = root / "src/tests/multilayer" / name
            tree = ast.parse(path.read_text(encoding="utf-8"))
            guard = next(node for node in tree.body if isinstance(node, ast.If))
            first_import = next(node for node in tree.body if isinstance(node, (ast.Import, ast.ImportFrom)))
            self.assertLess(guard.lineno, first_import.lineno)
            result = subprocess.run([sys.executable, "-B", str(path), "--help"], capture_output=True, text=True, timeout=15)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("retired", result.stderr)

    def test_known_reader_dispositions_require_delivered_adapter_and_no_logs(self):
        """Migrated continuation readers retain explicit live adapters, not diagnostic prerequisites.

        @param self Active read-only consumer-contract check.
        @return None; named current reader/adapter files must physically exist.
        """
        root = repository_root()
        document = json.loads((root / "src/tests/fixtures/cleanup/consumer-dispositions.json").read_text())
        for row in document["consumers"]:
            self.assertTrue((root / row["source"]).is_file())
            self.assertIsNot(row.get("diagnostic_logs_required"), True)
            if "adapter" in row:
                self.assertTrue((root / row["adapter"]).is_file())


if __name__ == "__main__":
    unittest.main(verbosity=2)
