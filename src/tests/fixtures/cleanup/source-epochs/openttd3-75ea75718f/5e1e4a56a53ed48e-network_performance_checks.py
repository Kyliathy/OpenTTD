"""Reject corrupted actual native peer performance evidence.

@file network_performance_checks.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21EC.
Both default-unit ordinary worlds and zero-unit original fixtures must pass the same native comparer.
"""

import argparse
import copy
import json
import unittest

from ground_network import ROOT
from ground_network_report import verify_reference
from network_performance import inputs
from network_performance_report import verify_measurement, verify_selection

SELECTION = None


class LiveJoinPerformanceBoundaries(unittest.TestCase):
    """Real reconstructed first work,complete native files and Windows peer peaks cannot be relabelled."""

    @classmethod
    def setUpClass(cls):
        assert SELECTION is not None
        cls.index = json.loads((ROOT / SELECTION).read_text(encoding="utf-8"))
        assert cls.index["complete"] is True
        cls.baseline = json.loads(verify_reference(cls.index["baseline"]).read_text(encoding="utf-8"))
        cls.inputs = inputs()
        cls.actuals = {}
        for label in ("ordinary-1024", "thermal-operating"):
            ref = next(row["report"] for row in cls.index["records"] if row["key"] == ["Debug", label])
            cls.actuals[label] = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        cls.actual = cls.actuals["ordinary-1024"]

    def refuse(self, mutate):
        payload = copy.deepcopy(self.actual)
        mutate(payload)
        with self.assertRaises(AssertionError):
            verify_measurement(payload, self.baseline, self.inputs["Debug", "ordinary-1024"])

    def test_actual_default_and_legacy_local_units_pass(self):
        for label, actual in self.actuals.items():
            self.assertEqual(verify_measurement(copy.deepcopy(actual), self.baseline, self.inputs["Debug", label])["budget_excesses"], [])

    def test_one_ns_timing_corruption_refuses(self):
        self.refuse(lambda p: p.__setitem__("join_upper_bound_ns", p["join_upper_bound_ns"] + 1))

    def test_missing_actual_next_checkpoint_refuses(self):
        self.refuse(lambda p: p["comparisons"].pop(1))

    def test_wrong_native_input_refuses(self):
        self.refuse(lambda p: p["selected"].__setitem__("input", self.inputs["Debug", "thermal-operating"]["input"]))

    def test_unknown_local_exception_refuses(self):
        self.refuse(lambda p: p["source_local_units"].__setitem__("economy.inflation", 0))

    def test_one_byte_peer_peak_corruption_refuses(self):
        self.refuse(lambda p: p["peak_commit_bytes"].__setitem__("client", p["peak_commit_bytes"]["client"] - 1))

    def test_combined_private_corruption_refuses(self):
        self.refuse(lambda p: p.__setitem__("sampled_combined_private_bytes", p["sampled_combined_private_bytes"] + 1))

    def test_incomplete_pair_selection_refuses(self):
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
