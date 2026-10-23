"""Refuse altered real stress counts,timings,budgets and resource peaks.

@file ground_stress_checks.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Select exact current observations explicitly;only metadata copies are corrupted.
"""

import argparse
import copy
import json
import unittest

from ground_network import ROOT
from ground_network_report import verify_reference
from ground_stress_report import verify_record

SELECTION = None


class StressObservationBoundaries(unittest.TestCase):
    """Actual stdout/native owners and earlier fixed budgets cannot be replaced by caller metadata."""

    @classmethod
    def setUpClass(cls):
        """Consume an explicitly selected actual complete current native matrix."""
        assert SELECTION is not None
        index = json.loads((ROOT / SELECTION).read_text(encoding="utf-8"))
        assert index["complete"] is True
        cls.baseline = json.loads(verify_reference(index["baseline"]).read_text(encoding="utf-8"))
        ref = next(row["report"] for row in index["records"] if row["key"] == ["Debug", "job", 512, 128, "straight"])
        cls.actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))

    def refuse(self, mutation):
        """A copy of real measured evidence must fail without touching its native logs or workload."""
        payload = copy.deepcopy(self.actual)
        mutation(payload)
        with self.assertRaises(AssertionError):
            verify_record(payload, self.baseline)

    def test_real_observation_passes(self):
        """Establish that the unmodified actual input passes this current validator."""
        self.assertEqual(verify_record(copy.deepcopy(self.actual), self.baseline)["jobs"], 128)

    def test_one_nanosecond_corruption_refuses(self):
        """Actual percentile arithmetic must still agree with every retained measured sample."""
        self.refuse(lambda p: p["timings"]["work"].__setitem__("p99_ns", p["timings"]["work"]["p99_ns"] + 1))

    def test_missing_slow_sample_refuses(self):
        """Removing a real slow owner update cannot improve the claimed distribution."""
        self.refuse(lambda p: p["samples"].remove(max(p["samples"], key=lambda row: row["ns"])))

    def test_relabelled_dense_count_refuses(self):
        """Native128-job work cannot become another density by changing its metadata."""
        self.refuse(lambda p: p.__setitem__("jobs", 32))

    def test_relaxed_budget_refuses(self):
        """The earlier frozen budget remains exact even if a caller prefers a larger allowance."""
        self.refuse(lambda p: p["budget"].__setitem__("simulation_p99_ns", p["budget"]["simulation_p99_ns"] * 2))

    def test_one_byte_peak_corruption_refuses(self):
        """Claimed process peaks must equal actual native Windows counters."""
        self.refuse(lambda p: p.__setitem__("peak_commit_bytes", p["peak_commit_bytes"] - 1))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    args = parser.parse_args()
    SELECTION = args.selection
    unittest.main(argv=[__file__])
