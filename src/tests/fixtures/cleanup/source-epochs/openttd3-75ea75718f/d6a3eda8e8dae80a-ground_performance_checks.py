"""Refuse incomplete or corrupted actual performance observations before budget publication.

@file ground_performance_checks.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Copies alter report metadata only;the real native maps/logs/samples remain immutable.
"""

import copy
import json
import unittest

from ground_network import ROOT
from runner import create_run_context
from ground_performance_report import accept

BASELINE = "mlm/reviews/performance/ground-maps/20261019-215123-87f72fea0a/reports/ground-map-Debug-classic-128.json"


class GroundPerformanceBoundaries(unittest.TestCase):
    """Retain actual sample/native-log agreement and complete workload selection as required acceptance boundaries."""

    @classmethod
    def setUpClass(cls):
        """Use the actual first native baseline,never expected timing values or a fabricated native log."""
        cls.original = json.loads((ROOT / BASELINE).read_text(encoding="utf-8"))
        cls.context = create_run_context("performance", "ground-map-refusals")

    def check_copy_refused(self, label, mutate):
        """Alter one actual report copy and require refusal without writing a combined verdict."""
        payload = copy.deepcopy(self.original)
        mutate(payload)
        path = self.context.output("reports", label + ".json")
        path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        target = self.context.logs / (label + "-must-not-exist.json")
        with self.assertRaises(AssertionError):
            accept([path.relative_to(ROOT).as_posix()], target.relative_to(ROOT).as_posix())
        self.assertFalse(target.exists())

    def test_incomplete_actual_matrix_refuses(self):
        """One valid real observation cannot complete the required profile/build/map matrix."""
        target = self.context.logs / "incomplete-must-not-exist.json"
        with self.assertRaises(AssertionError):
            accept([BASELINE], target.relative_to(ROOT).as_posix())
        self.assertFalse(target.exists())

    def test_changed_one_nanosecond_percentile_refuses(self):
        """A one-unit measured-summary corruption must disagree with the real complete samples."""
        def alter(payload):
            payload["timings"]["simulation_ns"]["p99_ns"] += 1
        self.check_copy_refused("changed-percentile", alter)

    def test_removed_slow_tick_refuses(self):
        """Dropping the actual slowest sample cannot improve a claimed percentile."""
        def alter(payload):
            payload["tick_samples"].remove(max(payload["tick_samples"], key=lambda row: row["simulation_ns"]))
        self.check_copy_refused("missing-slow-tick", alter)

    def test_relabelled_map_size_refuses(self):
        """A compact actual world cannot stand in for a representative map by changing metadata."""
        def alter(payload):
            payload["axis"] = 512
        self.check_copy_refused("wrong-map-size", alter)

    def test_relabelled_process_peak_refuses(self):
        """Claimed process peaks must equal actual retained Windows counters."""
        def alter(payload):
            payload["peak_commit_bytes"] -= 1
        self.check_copy_refused("changed-peak", alter)


if __name__ == "__main__":
    unittest.main()
