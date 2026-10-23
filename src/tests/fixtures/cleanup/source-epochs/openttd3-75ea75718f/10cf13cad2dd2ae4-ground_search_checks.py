"""Reject corrupted real whole-search evidence before closing the dense planner scope.

@file ground_search_checks.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only metadata copies are changed;actual native logs,queries and original worlds remain untouched.
"""

import argparse
import copy
import json
import unittest

from ground_network import ROOT
from ground_network_report import verify_reference
from ground_search_report import verify_search

SELECTION = None


class WholeSearchBoundaries(unittest.TestCase):
    """Full native query samples,count bounds,hardness and resource budgets cannot be relabelled."""

    @classmethod
    def setUpClass(cls):
        """Read the explicitly selected actual complete native observations and their earlier baseline."""
        assert SELECTION is not None
        index = json.loads((ROOT / SELECTION).read_text(encoding="utf-8"))
        assert index["complete"] is True
        dense = json.loads(verify_reference(index["dense_subset"]).read_text(encoding="utf-8"))
        cls.baseline = json.loads(verify_reference(dense["baseline"]).read_text(encoding="utf-8"))
        ref = next(r["report"] for r in index["records"] if r["key"] == ["Debug", "search", 512, 128, "remote-default", 0])
        cls.actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))

    def refuse(self, mutate):
        """Corrupt one actual report copy and require refusal from the current existing verifier."""
        payload = copy.deepcopy(self.actual)
        mutate(payload)
        with self.assertRaises(AssertionError):
            verify_search(payload, self.baseline)

    def test_actual_whole_query_passes(self):
        self.assertGreater(verify_search(copy.deepcopy(self.actual), self.baseline)["expansions"], 1000)

    def test_removed_slowest_query_refuses(self):
        self.refuse(lambda p: p["samples"].remove(max(p["samples"], key=lambda r: r["ns"])))

    def test_altered_expansion_count_refuses(self):
        self.refuse(lambda p: p["complete"][0].__setitem__("expansions", p["complete"][0]["expansions"] + 1))

    def test_false_current_hardness_refuses(self):
        self.refuse(lambda p: p.__setitem__("hardness", 1))

    def test_relaxed_prior_budget_refuses(self):
        self.refuse(lambda p: p["budget"].__setitem__("route_p99_ns", p["budget"]["route_p99_ns"] * 2))

    def test_one_byte_peak_corruption_refuses(self):
        self.refuse(lambda p: p.__setitem__("peak_commit_bytes", p["peak_commit_bytes"] - 1))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    args = parser.parse_args()
    SELECTION = args.selection
    unittest.main(argv=[__file__])
