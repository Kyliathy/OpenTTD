"""Focused domain and current-versus-historical Ground closure refusals.

@file selected_ground_closure_check.py Independent retained inputs remain immutable.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group20BC.
@see docs/zz-reach2/architecture/archi-historical-debug.md.
"""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import qualify_ground_selected_closure as closure


class SelectedGroundClosureChecks(unittest.TestCase):
    """Protect exact selected ownership and reject historical or mixed current evidence."""

    def setUp(self):
        """Read retained dispatch and canonical geometry without generating expectations.

        @param self Focused closure refusal test owner.
        """
        self.dispatch = json.loads((closure.ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json").read_text())
        public = json.loads(closure.handoff.PUBLIC.read_text())
        self.families = {row["asset_id"]: row for row in public["assets"]}
        self.selected = {row["asset_id"] for row in self.dispatch["families"]}
        self.domain = set(range(22045))
        self.views = {mode: dict(path="current-" + mode, sha256="unit-reference") for mode in closure.MODES}

    def coverage(self):
        """Join the retained original domain for one isolated refusal fixture.

        @param self Fixture owner containing an explicit dispatch and native domain.
        @return Selected current/later source coverage rows.
        """
        return closure.coverage_rows(self.dispatch, self.families, self.selected, self.domain, self.views)

    def test_exact_scope_keeps_all_deferred_gates_open(self):
        """Bind current views while preserving all twenty-six deferred owner tasks.

        @param self Exact selected source-domain test owner.
        """
        rows = self.coverage()
        self.assertEqual(sum(row["sprite_count"] for row in rows), 15488)
        later = [row for row in rows if row["dispatch"] == "later_owner"]
        self.assertEqual(len(later), 26)
        self.assertTrue(all(row["actual_view_evidence"] is None for row in later))
        self.assertTrue(all(row["operating_claimed"] is False for row in rows))
        body = next(row for row in rows if row["asset_id"] == "vehicle.model-08")
        self.assertEqual(body["actual_view_evidence"], self.views["road"])

    def test_missing_family_or_native_offset_refuses(self):
        """Reject a matching subset of families and an incomplete decoded sprite domain.

        @param self Partial selected-domain refusal owner.
        """
        self.domain.remove(7791)
        with self.assertRaisesRegex(ValueError, "domain/range"):
            self.coverage()
        self.domain.add(7791)
        self.dispatch["families"].pop()
        with self.assertRaisesRegex(ValueError, "fifty-family"):
            self.coverage()

    def test_changed_native_range_refuses(self):
        """Keep dispatched offsets tied to the original independently owned source geometry.

        @param self Wrong native owner-range refusal owner.
        """
        self.dispatch["families"][0]["native_offset"] += 1
        with self.assertRaisesRegex(ValueError, "domain/range"):
            self.coverage()

    def test_deferred_owner_cannot_lose_its_dispatch(self):
        """Require the explicit unfinished gameplay task after successful art qualification.

        @param self Deferred gameplay boundary refusal owner.
        """
        row = next(row for row in self.dispatch["families"] if row["dispatch"] == "later_owner")
        row.pop("reason")
        with self.assertRaisesRegex(ValueError, "unfinished gameplay"):
            self.coverage()

    def test_missing_capture_refuses_before_upstream_access(self):
        """Reject incomplete mode selection without consulting historical or current outputs.

        @param self Explicit capture-selection refusal owner.
        """
        with self.assertRaisesRegex(ValueError, "all six explicit"):
            closure.qualify("RelWithDebInfo", {}, {})

    def test_mixed_selector_refuses(self):
        """Reject a capture from another selector even after its own owner accepted it.

        @param self Mixed current-epoch refusal owner.
        """
        with patch.object(closure, "checked_result", return_value=dict(source_selector={"path": "other"})):
            with self.assertRaisesRegex(ValueError, "another source/decoder/selector"):
                closure.immutable_views({}, "collector", "RelWithDebInfo", {"path": "selected"})

    def test_historical_receipt_and_truncated_native_fixture_refuse(self):
        """Old log receipt selection and incomplete fixtures cannot qualify current views.

        @param self Historical/current evidence boundary refusal owner.
        """
        with self.assertRaises(ValueError):
            closure.immutable_views(dict(path="logs/old-ground.json", sha256="old"), "collector", "RelWithDebInfo", {})
        with tempfile.TemporaryDirectory(dir=closure.ROOT / "test-cache/cleanup", prefix="ground-closure-") as temporary:
            path = Path(temporary) / "incomplete.bin"
            path.write_bytes(b"GRDART1\0")
            with self.assertRaisesRegex(ValueError, "header"):
                closure.native_domain(path)


if __name__ == "__main__":
    unittest.main()
