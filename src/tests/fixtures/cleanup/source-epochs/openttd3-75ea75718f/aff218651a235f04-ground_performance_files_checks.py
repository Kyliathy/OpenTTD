"""Reject corruption of actual file/load/first-native-work evidence.

@file ground_performance_files_checks.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21EB.
Only metadata copies change;native files,logs,frames and timing samples remain untouched.
"""

import argparse
import copy
import json
import unittest

from ground_network import ROOT
from ground_network_report import verify_reference
from ground_performance_files import inputs, verify_measurement
from ground_performance_files_report import verify_selection

SELECTION = None


class FileReconstructionBoundaries(unittest.TestCase):
    """Actual first work,observer overhead,clock and full file identity cannot be relabelled."""

    @classmethod
    def setUpClass(cls):
        assert SELECTION is not None
        cls.index = json.loads((ROOT / SELECTION).read_text(encoding="utf-8"))
        assert cls.index["complete"] is True
        cls.baseline = json.loads(verify_reference(cls.index["baseline"]).read_text(encoding="utf-8"))
        bank = json.loads(verify_reference(cls.index["bank"]).read_text(encoding="utf-8"))
        cls.selected = inputs(cls.baseline, bank)["Debug", "ordinary-512"]
        ref = next(row["report"] for row in cls.index["records"] if row["key"] == ["Debug", "ordinary-512", 0])
        cls.actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))

    def refuse(self, mutate):
        payload = copy.deepcopy(self.actual)
        mutate(payload)
        with self.assertRaises(AssertionError):
            verify_measurement(payload, self.baseline, self.selected)

    def test_actual_file_and_first_work_pass(self):
        self.assertEqual(verify_measurement(copy.deepcopy(self.actual), self.baseline, self.selected)["budget_excesses"], [])

    def test_missing_first_resumed_work_refuses(self):
        self.refuse(lambda p: p.__setitem__("first_resumed_work", []))

    def test_observer_overhead_corruption_refuses(self):
        self.refuse(lambda p: p["first_resumed_work"][0].__setitem__("initial_observer_ns", p["first_resumed_work"][0]["initial_observer_ns"] + 1))

    def test_first_native_clock_corruption_refuses(self):
        self.refuse(lambda p: p["first_resumed_work"][0].__setitem__("tick", p["first_resumed_work"][0]["tick"] + 1))

    def test_removed_slowest_tick_refuses(self):
        self.refuse(lambda p: p["tick_samples"].remove(max(p["tick_samples"], key=lambda row: row["simulation_ns"])))

    def test_one_byte_peak_corruption_refuses(self):
        self.refuse(lambda p: p.__setitem__("peak_commit_bytes", p["peak_commit_bytes"] - 1))

    def test_false_file_identity_refuses(self):
        self.refuse(lambda p: p["saves"]["final-world"].__setitem__("sha256", "0" * 64))

    def test_incomplete_selection_refuses(self):
        payload = copy.deepcopy(self.index)
        payload["complete"] = False
        with self.assertRaises(AssertionError):
            verify_selection(payload)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    args = parser.parse_args()
    SELECTION = args.selection
    unittest.main(argv=[__file__])
