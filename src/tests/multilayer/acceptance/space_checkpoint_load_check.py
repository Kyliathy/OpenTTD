"""Focused load-proof refusals using retained independent saves, never a game run.

@file space_checkpoint_load_check.py Fault injection for the dedicated native load contract.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group11.
Synthetic process metadata is deliberately unqualified; actual execution is Group15.
"""

import copy
import json
import lzma
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch

import space_checkpoint_load as load
from savegame_tables import chunks as transport_chunks, read_stream


class CheckpointLoadChecks(unittest.TestCase):
    """Exercise actual save bytes with isolated, explicitly synthetic process records.

    @param self Active repository-local fixture test, never an executed qualification.
    """

    def setUp(self):
        """Copy the typed seal's small dependency closure and build one mock receipt.

        @param self Active isolated test.
        @return None; all fault injection stays under this test's scratch directory.
        """
        cache = load.ROOT / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="load-proof-", dir=cache)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        seal_path = load.ROOT / "src/tests/fixtures/checkpoints/space/checkpoint.json"
        self.seal = json.loads(seal_path.read_text(encoding="utf-8"))
        history = json.loads((load.ROOT / self.seal["history"]["path"]).read_text(encoding="utf-8"))
        paths = {seal_path.relative_to(load.ROOT).as_posix(), *load.SOURCE_FILES}
        paths.update(self.seal[field]["path"] for field in ("history", "historical_validation", "migration_map"))
        paths.update(row["retained"]["path"] for row in history["live_retained_inputs"])
        paths.update(row[field]["path"] for row in self.seal["configurations"].values()
                     for field in ("save", "expected_observation", "expected_chunks"))
        # Only declared immutable history and current source fixtures are copied.
        # No old runtime, EXE, paid output or producer-created expected baseline is used.
        for relative in paths:
            destination = self.root / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(load.ROOT / relative, destination)
        (self.root / "CMakeLists.txt").write_text("# @see r2fc-file-cleanup3.md; synthetic load-proof root.\n", encoding="utf-8")
        self.make_receipt("RelWithDebInfo")

    def make_receipt(self, configuration):
        """Build synthetic transport metadata around independent retained save bytes.

        @param self Active isolated test.
        @param configuration Explicit seal configuration represented by this receipt.
        @return None; no claim of native execution/build qualification is published.
        """
        selected = self.seal["configurations"][configuration]
        source = self.root / selected["save"]["path"]
        self.context = load.create_run_context("space", load.SCENARIO, root=self.root)
        self.application = self.root / "build" / configuration / "openttd_surface_capital_test.exe"
        self.application.parent.mkdir(parents=True, exist_ok=True)
        self.application.write_bytes(b"synthetic transport fixture; not an executable or qualification")
        self.before = self.context.saves / "checkpoint-before.sav"
        self.output = self.context.saves / "checkpoint-after.sav"
        shutil.copyfile(source, self.before)
        shutil.copyfile(source, self.output)
        expected = json.loads((self.root / selected["expected_observation"]["path"]).read_text(encoding="utf-8"))["observation"]
        date = load.table(source, b"DATE")[0]
        revisions = load.table(source, b"GLOG")
        last = revisions[max(revisions)]
        self.native = dict(company=0, cash=expected["company"]["cash"], debt=expected["company"]["debt"],
            tick_before=date["tick_counter"], tick_after=date["tick_counter"],
            revision_text=last["action[0].revision[0].revision.text"],
            revision_modified=last["action[0].revision[0].revision.modified"],
            revision_newgrf=last["action[0].revision[0].revision.newgrf"], save_version=440)
        self.terminal = self.context.evidence / "native-synthetic-stdout.txt"
        self.terminal.write_text("SPACE_CHECKPOINT_NATIVE " + " ".join(f"{key}={value}" for key, value in self.native.items()) +
            "\nSPACE_CHECKPOINT_COMPLETE phase=load-only ticks=0 snapshots=2\nAll tests passed (17 assertions in 1 test case)\n", encoding="utf-8")
        source_reference = selected["save"]
        self.process_path = self.context.reports / "native-synthetic.json"
        self.process = dict(exit_status=0, inputs=[source_reference], executable=load.identity(self.application, self.root),
            argv=[str(self.application), load.TAG, "--durations", "yes"], configuration=configuration,
            run_id=self.context.run_id, feature="space", scenario=self.context.scenario,
            runtime=self.context.runtime_dir.relative_to(self.root).as_posix(),
            artifacts=[load.identity(path, self.root) for path in (self.before, self.output, self.terminal)],
            transcripts=[load.identity(self.terminal, self.root)])
        self.process_path.write_text(json.dumps(self.process), encoding="utf-8")
        self.document = dict(_documentation="Owner: r2fc-file-cleanup3.md11; synthetic verifier fixture, never executed qualification.",
            schema_version=1, kind=load.KIND, complete=True, configuration=configuration, phase=load.PHASE, tag=load.TAG,
            seal=load.identity(self.root / "src/tests/fixtures/checkpoints/space/checkpoint.json", self.root),
            source=source_reference, application=load.identity(self.application, self.root), run_id=self.context.run_id,
            before=load.identity(self.before, self.root), output=load.identity(self.output, self.root),
            terminal=load.identity(self.terminal, self.root), native_process=load.identity(self.process_path, self.root),
            source_files=[load.identity(self.root / path, self.root) for path in load.SOURCE_FILES],
            observed=expected, gameplay_ticks_executed=0)

    def update_process(self):
        """Rebind mutated synthetic metadata so tests exercise semantic refusals.

        @param self Active isolated test.
        @return None; this never turns the fixture into actual execution evidence.
        """
        self.process_path.write_text(json.dumps(self.process), encoding="utf-8")
        self.document["native_process"] = load.identity(self.process_path, self.root)

    def mutate_before(self, family, field=None, width=None):
        """Inject one valid-container clock or physical-family fault into a snapshot.

        @param self Active isolated test.
        @param family Actual four-byte chunk identifier to alter.
        @param field Named scalar DATE field, or None to flip a physical payload byte.
        @param width Exact original scalar byte width when changing a clock field.
        @return None; independent source/golden observations remain untouched.
        """
        header = self.before.read_bytes()[:8]
        version, stream = read_stream(self.before)
        self.assertEqual(version, 440)
        rows = transport_chunks(stream)
        old = load.identity(self.before, self.root)
        # Preserve all other raw families/header framing. This is fault injection,
        # not a changed producer writing a new expected/golden gameplay state.
        for row in rows:
            if row["name"] != family:
                continue
            if field is None:
                row["raw"] = row["raw"][:-1] + bytes([row["raw"][-1] ^ 1])
            else:
                original = load.table(self.before, b"DATE")[0][field]
                needle = original.to_bytes(width, "big")
                self.assertEqual(row["raw"].count(needle), 1)
                row["raw"] = row["raw"].replace(needle, (original + 1).to_bytes(width, "big"), 1)
        self.before.write_bytes(header + lzma.compress(b"".join(row["raw"] for row in rows) + bytes(4)))
        replacement = load.identity(self.before, self.root)
        self.document["before"] = replacement
        self.process["artifacts"] = [replacement if reference == old else reference for reference in self.process["artifacts"]]
        self.update_process()

    def test_retained_data_verifies_read_only_for_both_configurations(self):
        """Both distinct original saves satisfy actual owner/clock/family comparisons.

        @param self Active isolated test with explicitly synthetic process transport.
        @return None; all filesystem writes/process launches are forbidden in verification.
        """
        for configuration in ("RelWithDebInfo", "Debug"):
            if configuration != self.document["configuration"]:
                self.make_receipt(configuration)
            with patch.object(Path, "mkdir", side_effect=AssertionError("verification writes")), \
                    patch.object(Path, "write_bytes", side_effect=AssertionError("verification writes")), \
                    patch.object(Path, "write_text", side_effect=AssertionError("verification writes")), \
                    patch("subprocess.Popen", side_effect=AssertionError("verification launches process")):
                result = load.verify_load_result(self.document, self.root)
            self.assertEqual(result["chunk_families"], 121)
            self.assertEqual(result["saved_family_comparisons"], 242)
            self.assertEqual(result["clocks"]["source"]["tick_counter"], 2923387)
            self.assertEqual(result["gameplay_ticks_executed"], 0)

    def test_wrong_save_and_configuration_fail(self):
        """A correct process cannot qualify the other configuration's original save.

        @param self Active isolated test.
        @return None; configuration mismatch is rejected before saved-output comparison.
        """
        self.document["source"] = self.seal["configurations"]["Debug"]["save"]
        with self.assertRaisesRegex(ValueError, "wrong save/configuration"):
            load.verify_load_result(self.document, self.root)
        selected = load.load_checkpoint(configuration="Debug", root=self.root)
        with patch("subprocess.Popen", side_effect=AssertionError("wrong configuration launched")), \
                self.assertRaisesRegex(ValueError, "typed selection"):
            load.execute_load_check(selected, "RelWithDebInfo", self.application)

    def test_tick_advance_is_not_hidden_by_date_exception(self):
        """A one-tick DATE mutation fails even when its hash/process metadata is rebound.

        @param self Active isolated test.
        @return None; zero-tick recovery checks the actual stored native counter.
        """
        self.mutate_before(b"DATE", "tick_counter", 8)
        with self.assertRaisesRegex(ValueError, "clock/state changed: tick_counter"):
            load.verify_load_result(self.document, self.root)

    def test_rng_change_is_not_hidden_by_date_exception(self):
        """A preserved tick alone cannot conceal changed random/scheduler state.

        @param self Active isolated test.
        @return None; every saved DATE field stays bound to the original.
        """
        self.mutate_before(b"DATE", "random_state[0]", 4)
        with self.assertRaisesRegex(ValueError, "clock/state changed: random_state"):
            load.verify_load_result(self.document, self.root)

    def test_changed_physical_family_fails(self):
        """Native snapshots cannot agree only on selected money/material summaries.

        @param self Active isolated test.
        @return None; unrelated map ownership is protected by the full121-family check.
        """
        self.mutate_before(b"MAPT")
        with self.assertRaisesRegex(ValueError, "changed family"):
            load.verify_load_result(self.document, self.root)

    def test_wrong_tag_phase_and_unsuccessful_process_fail(self):
        """A successful old work/query case or failed native process cannot qualify loading.

        @param self Active isolated test.
        @return None; phase/tag/status checks stay separate from generic complete flags.
        """
        invalid = copy.deepcopy(self.document)
        invalid["phase"] = "construction-cash-work"
        with self.assertRaisesRegex(ValueError, "schema/tag/phase"):
            load.verify_load_result(invalid, self.root)
        self.process["argv"][1] = "[.surface-construction-cash]"
        self.update_process()
        with self.assertRaisesRegex(ValueError, "process inputs/image/tag/status"):
            load.verify_load_result(self.document, self.root)
        self.process["argv"][1] = load.TAG
        self.process["exit_status"] = 1
        self.update_process()
        with self.assertRaisesRegex(ValueError, "process inputs/image/tag/status"):
            load.verify_load_result(self.document, self.root)

    def test_incomplete_process_outputs_and_missing_terminal_fail(self):
        """Native process membership requires both snapshots and the actual retained transcript.

        @param self Active isolated test.
        @return None; output files alone never stand in for a complete native process.
        """
        self.process["artifacts"].remove(self.document["output"])
        self.update_process()
        with self.assertRaisesRegex(ValueError, "incomplete native process"):
            load.verify_load_result(self.document, self.root)
        self.terminal.unlink()
        with self.assertRaises(FileNotFoundError):
            load.verify_load_result(self.document, self.root)

    def test_single_revision_append_rejects_unrelated_actions(self):
        """Only one exact load revision can differ, preserving the full older history.

        @param self Active isolated test.
        @return None; extra settings/GRF/action metadata cannot hide in a GLOG exemption.
        """
        source = self.root / self.document["source"]["path"]
        original = load.table(source, b"GLOG")
        appended = copy.deepcopy(original[max(original)])
        appended["tick"] = 2923387
        new_native = dict(self.native, revision_text=b"new-current-fixture-revision".hex())
        appended["action[0].revision[0].revision.text"] = new_native["revision_text"]
        after = dict(original, **{})
        after[len(original)] = appended

        def fixture_table(path, name):
            """Select independent original or fault-injected semantic history.

            @param path Original source or candidate paused output.
            @param name DATE or GLOG family requested by the strict revision verifier.
            @return Synthetic named rows for this semantic-policy check only.
            """
            if name == b"DATE":
                return {0: {"tick_counter": 2923387}}
            return original if path == source else after

        with patch.object(load, "table", side_effect=fixture_table):
            self.assertTrue(load.compare_revision(source, self.before, new_native))
            appended["action[0].setting.count"] = 1
            with self.assertRaisesRegex(ValueError, "sole load revision"):
                load.compare_revision(source, self.before, new_native)


if __name__ == "__main__":
    unittest.main(verbosity=2)
