"""Refuse corrupted actual utility evidence without modifying native data.

@file utility_performance_checks.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDB.
These checks consume an explicit complete selection;they never invent native grants or test output.
"""

import argparse
import copy
import json
import unittest

from ground_network import ROOT
from ground_network_report import verify_reference
from utility_performance import verify_measurement
from utility_performance_report import verify_selection

SELECTION = None


class UtilityPerformanceBoundaries(unittest.TestCase):
    """Native timing,energy,expenses,eligibility and OS peaks remain authoritative."""

    @classmethod
    def setUpClass(cls):
        assert SELECTION is not None
        cls.index = json.loads((ROOT / SELECTION).read_text(encoding="utf-8"))
        assert cls.index["complete"] is True
        cls.baseline = json.loads(verify_reference(cls.index["baseline"]).read_text(encoding="utf-8"))
        ref = next(row["report"] for row in cls.index["records"] if row["key"] == ["Debug", 4096, 4])
        cls.actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))

    def refuse(self, mutate):
        payload = copy.deepcopy(self.actual)
        mutate(payload)
        with self.assertRaises(AssertionError):
            verify_measurement(payload, self.baseline)

    def test_actual_settlement_passes(self):
        self.assertEqual(verify_measurement(copy.deepcopy(self.actual), self.baseline)["eligibility_edges"], 16384)

    def test_missing_slowest_interval_refuses(self):
        self.refuse(lambda p: p["samples"].remove(max(p["samples"], key=lambda row: row["allocation_ns"])))

    def test_energy_corruption_refuses(self):
        self.refuse(lambda p: p["complete"][0].__setitem__("consumed_j", p["complete"][0]["consumed_j"] + 1))

    def test_expense_corruption_refuses(self):
        self.refuse(lambda p: p["complete"][0].__setitem__("expense", p["complete"][0]["expense"] - 1))

    def test_eligibility_relabelling_refuses(self):
        self.refuse(lambda p: p.__setitem__("fanout", 1))

    def test_relaxed_budget_refuses(self):
        self.refuse(lambda p: p["budget"].__setitem__("simulation_p99_ns", p["budget"]["simulation_p99_ns"] * 2))

    def test_one_byte_peak_corruption_refuses(self):
        self.refuse(lambda p: p.__setitem__("peak_commit_bytes", p["peak_commit_bytes"] - 1))

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
