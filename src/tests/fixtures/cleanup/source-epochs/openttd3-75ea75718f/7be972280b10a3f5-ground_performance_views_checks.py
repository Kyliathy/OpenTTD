"""Refuse corrupt actual full-loop view evidence without altering native worlds.

@file ground_performance_views_checks.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDD.
Tests change metadata copies only;all native input,output and timing artifacts remain immutable.
"""

import argparse
import copy
import json
import unittest

from ground_network import ROOT
from ground_network_report import verify_reference
from ground_performance_views import verify_measurement
from ground_performance_views_report import verify_selection

SELECTION = None


class ViewPerformanceBoundaries(unittest.TestCase):
    """Actual driver role,equal native clock and raw ticks cannot be relabelled."""

    @classmethod
    def setUpClass(cls):
        assert SELECTION is not None
        cls.index = json.loads((ROOT / SELECTION).read_text(encoding="utf-8"))
        assert cls.index["complete"] is True
        cls.baseline = json.loads(verify_reference(cls.index["baseline"]).read_text(encoding="utf-8"))
        bank = json.loads(verify_reference(cls.index["bank"]).read_text(encoding="utf-8"))
        cls.selected = bank["records"]["Debug"]["working-bore"]
        ref = next(row["report"] for row in cls.index["records"] if row["key"] == ["Debug", "working-bore", "headless"])
        cls.actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))

    def refuse(self, mutate):
        payload = copy.deepcopy(self.actual)
        mutate(payload)
        with self.assertRaises(AssertionError):
            verify_measurement(payload, self.baseline, "headless", self.selected)

    def test_actual_headless_native_world_passes(self):
        self.assertEqual(verify_measurement(copy.deepcopy(self.actual), self.baseline, "headless", self.selected)["budget_excesses"], [])

    def test_missing_slowest_tick_refuses(self):
        self.refuse(lambda p: p["tick_samples"].remove(max(p["tick_samples"], key=lambda row: row["simulation_ns"])))

    def test_role_relabelling_refuses(self):
        self.refuse(lambda p: p.__setitem__("view_role", "surface"))

    def test_gui_capability_relabelling_refuses(self):
        self.refuse(lambda p: p["view_observations"][0].__setitem__("gui_capability", 1))

    def test_changed_final_clock_refuses(self):
        self.refuse(lambda p: p["owners"][1].__setitem__("tick", p["owners"][1]["tick"] + 1))

    def test_one_byte_peak_corruption_refuses(self):
        self.refuse(lambda p: p.__setitem__("peak_commit_bytes", p["peak_commit_bytes"] - 1))

    def test_false_pure_file_identity_refuses(self):
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
