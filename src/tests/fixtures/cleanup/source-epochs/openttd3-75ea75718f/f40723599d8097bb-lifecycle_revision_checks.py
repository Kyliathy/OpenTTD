"""Focused refusal checks for exact native lifecycle revision-log migration.

@file lifecycle_revision_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md15D.
Use actual retained original/current output plus metadata copies; never edit saves.
"""

import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
import lifecycle_revision_log as revision


class LifecycleRevisionChecks(unittest.TestCase):
    """Allow only the exact native build-revision append without losing old history.

    @see r2mlvs15-savegame-determinism-and-lifecycle.md15D.2; no native process/full regression.
    """

    @classmethod
    def setUpClass(cls):
        """Read the exact diagnostic original and the native-passing first current read.

        @param cls Test class receiving shared immutable original wire tables.
        @return None; no artifact is rewritten or discovered through latest-run lookup.
        """
        selected = json.loads((ROOT / "logs/lifecycle/owner-return/20261018-native-01/thermal-current-inputs.json").read_text())
        assert selected["complete"]
        observation = next(row for row in selected["records"] if row.get("original_case") == "retired-3" and row["configuration"] == "Debug")
        report = json.loads((ROOT / observation["report"]).read_text())
        result = report["results"][0]
        cls.first = result["original"]["input"]
        cls.second = result["first"]["outputs"]["read"]["input"]
        cls.tick = revision.saved_table(ROOT / cls.first["path"], b"DATE")[0]["tick_counter"]
        cls.before = next(row for row in revision.chunks(revision.read_stream(ROOT / cls.first["path"])[1]) if row["name"] == b"GLOG")
        cls.after = next(row for row in revision.chunks(revision.read_stream(ROOT / cls.second["path"])[1]) if row["name"] == b"GLOG")
        cls.old_table = revision.saved_table(ROOT / cls.first["path"], b"GLOG")
        cls.new_table = revision.saved_table(ROOT / cls.second["path"], b"GLOG")
        cls.expected = revision.expected_revision()

    def test_exact_native_revision_append(self):
        """Accept all125 exact gameplay/other chunks plus the complete typed revision action.

        @param self Current read-only testcase.
        @return None; appended event must be independently decoded and retained.
        """
        proof = revision.compare_original(self.first, self.second, self.tick)
        self.assertEqual(proof["complete_chunks_exact"], 125)
        self.assertTrue(proof["revision_append"]["all_other_complete_chunks_exact"])
        self.assertEqual(proof["revision_append"]["event"]["tick"], self.tick)

    def test_wrong_revision_tick_or_other_change_refuses(self):
        """Reject wrong build/date/schema values and extra native settings/GRF changes.

        @param self Current metadata-copy testcase.
        @return None; every distinct corruption must fail exact event validation.
        """
        for field in ("tick", "action[0].revision[0].revision.text", "action[0].revision[0].revision.slver", "action[0].setting.count", "action.count"):
            # A build event cannot conceal altered time, compatibility or any
            # simultaneous gameplay/content setting under a diagnostic alias.
            rows = copy.deepcopy(self.new_table)
            row = rows[len(self.old_table)]
            row[field] = "00" if isinstance(row[field], str) else row[field] + 1
            with self.subTest(field=field), self.assertRaises(AssertionError):
                revision.validate_append(self.before, self.after, self.old_table, rows, self.tick, self.expected)

    def test_changed_old_history_or_extra_row_refuses(self):
        """Keep all prior raw diagnostic rows and permit exactly one append.

        @param self Current isolated testcase.
        @return None; neither history rewriting nor two added actions can pass.
        """
        for fault in ("history", "extra"):
            # The preserved prefix includes the complete schema and every old
            # action, so no earlier unknown native field can be normalized away.
            candidate = copy.deepcopy(self.after)
            if fault == "history":
                candidate["rows"][1] += b"\0"
            else:
                candidate["rows"].append(candidate["rows"][-1])
            with self.subTest(fault=fault), self.assertRaises(AssertionError):
                revision.validate_append(self.before, candidate, self.old_table, self.new_table, self.tick, self.expected)

    def test_gameplay_chunk_change_refuses(self):
        """Refuse a simultaneous changed cargo/industry owner even with a valid revision log.

        @param self Current isolated testcase.
        @return None; the diagnostic exception never excludes another chunk.
        """
        original = revision.world_chunks
        after = original(ROOT / self.second["path"])
        after[b"INDY"] += b"changed"
        with patch.object(revision, "world_chunks", side_effect=[original(ROOT / self.first["path"]), after]), self.assertRaises(AssertionError):
            revision.compare_original(self.first, self.second, self.tick)

    def test_generated_integer_parser_refuses_executable_input(self):
        """Refuse names or calls in expected build-version arithmetic.

        @param self Current isolated testcase.
        @return None; only the generated literal arithmetic is admitted.
        """
        for value in ("read_secret()", "unexpected_name", "1 ** 9000"):
            with self.subTest(value=value), self.assertRaises(AssertionError):
                revision.constant_integer(revision.ast.parse(value, mode="eval").body)


if __name__ == "__main__":
    unittest.main()
