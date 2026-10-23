"""Reject relabelled or corrupted actual physical tube measurements.

@file tube_performance_checks.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDC.
Only metadata copies change;native logs,packets,worlds and source identities remain immutable.
"""

import argparse
import copy
import json
import unittest

from ground_network import ROOT
from ground_network_report import verify_reference
from tube_performance import verify_measurement
from tube_performance_report import verify_selection

SELECTION = None


class PhysicalTubeBoundaries(unittest.TestCase):
    """Actual native work,energy and invoice quantities cannot be replaced by convenient metadata."""

    @classmethod
    def setUpClass(cls):
        assert SELECTION is not None
        cls.index = json.loads((ROOT / SELECTION).read_text(encoding="utf-8"))
        assert cls.index["complete"] is True
        cls.baseline = json.loads(verify_reference(cls.index["baseline"]).read_text(encoding="utf-8"))
        ref = next(row["report"] for row in cls.index["records"] if row["key"] == ["Debug", 256, 64, 4096, 0, 0])
        cls.actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))

    def refuse(self, mutate):
        payload = copy.deepcopy(self.actual)
        mutate(payload)
        with self.assertRaises(AssertionError):
            verify_measurement(payload, self.baseline)

    def test_actual_physical_bound_passes(self):
        self.assertEqual(verify_measurement(copy.deepcopy(self.actual), self.baseline)["path_cells"], 16384)

    def test_removed_slowest_tick_refuses(self):
        self.refuse(lambda p: p["ticks"].remove(max(p["ticks"], key=lambda row: row["preparation_ns"] + row["work_ns"])))

    def test_energy_corruption_refuses(self):
        self.refuse(lambda p: p["complete"][0].__setitem__("consumed_j", p["complete"][0]["consumed_j"] + 1))

    def test_invoice_corruption_refuses(self):
        self.refuse(lambda p: p["complete"][0].__setitem__("usage_cost", p["complete"][0]["usage_cost"] - 1))

    def test_idle_relabelled_motion_refuses(self):
        self.refuse(lambda p: p.__setitem__("motion", 1))

    def test_relaxed_prior_budget_refuses(self):
        self.refuse(lambda p: p["budget"].__setitem__("simulation_p99_ns", p["budget"]["simulation_p99_ns"] * 2))

    def test_one_byte_peak_corruption_refuses(self):
        self.refuse(lambda p: p.__setitem__("peak_commit_bytes", p["peak_commit_bytes"] - 1))

    def test_missing_matrix_refuses(self):
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
