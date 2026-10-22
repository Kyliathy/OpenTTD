"""Semantic refusal checks for actual native vehicle body/source comparison.

@file ground_transport_body_pixel_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20BB; explicit current capture/source selection.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHC.
@see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
One real native pose is copied beneath repository test-cache. A scoped unit
matrix isolates pixel semantics; production full-matrix validation is also tested.
"""

import json
from pathlib import Path
import tempfile
import time
import unittest
from unittest.mock import patch

import ground_transport_body_pixel_checks as checks
from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import reference


from ground_pixel_refusal_support import run_checks, scratch_root

EXECUTION = None


class NativeTransportPixelRefusals(unittest.TestCase):
    """Exercise newly rehashed corruptions of an actual loaded, moving native mineral wagon."""

    def setUp(self):
        """Prepare a disposable native-pose copy without changing original source or capture files.

        @param self Current isolated semantic case.
        """
        cache = scratch_root("selected-ground-body-pixel-verifier")
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="native-body-", dir=cache)
        self.directory = Path(self.temporary.name).resolve()
        self.proof = handoff.read_json(EXECUTION)
        original = next(row for row in self.proof["observation"]["artifacts"] if row["path"].endswith("/transport-rail.json"))
        self.scene = handoff.read_json(original["path"])
        self.frame = next(row for row in self.scene["frames"] if row["model"] == 5 and row["state_index"] == 1
                          and row["frame"] == 1 and row["direction"] == 6 and row.get("context") != "list")
        self.scene["frames"] = [self.frame]
        (self.directory / "reports").mkdir()
        (self.directory / "images").mkdir()
        self.metadata = self.directory / "reports/transport-rail.json"
        self.index = self.directory / "images" / (self.frame["basename"] + ".idx")
        self.rgb = self.index.with_suffix(".ppm")
        # Only an actual source-qualified native pose can satisfy the positive
        # unit case. Every corruption below rehashes its modified scratch bytes.
        for path in (self.index, self.rgb):
            path.write_bytes((handoff.bound(original["path"]).parent.parent / "images" / path.name).read_bytes())
        self.execution = self.directory / "derived-execution.json"
        self.publish()

    def tearDown(self):
        """Delete only the verified repository-owned temporary pose.

        @param self Current isolated semantic case.
        """
        if not self.directory.is_relative_to(ROOT / "test-cache"):
            raise ValueError("body pixel scratch cleanup escaped repository")
        self.temporary.cleanup()

    def publish(self):
        """Refresh scratch provenance so semantic checks, rather than stale hashes, must refuse.

        @param self Current isolated semantic case.
        """
        self.metadata.write_text(json.dumps(self.scene) + "\n", encoding="utf-8")
        self.proof["observation"]["artifacts"] = [reference(path) for path in (self.metadata, self.index, self.rgb)]
        self.execution.write_text(json.dumps(self.proof) + "\n", encoding="utf-8")

    def verify_pose(self):
        """Isolate this one real pose while retaining the production source/provenance/pixel checks.

        @param self Current isolated semantic case.
        @return Actual one-pose source comparison result, or its semantic refusal.
        """
        identity = (self.frame["model"], self.frame["cargo_label"], self.frame["state_index"], self.frame["frame"], self.frame["direction"], "map")
        with patch.object(checks, "expected_matrix", return_value={identity}):
            return checks.verify(self.execution.relative_to(ROOT).as_posix(), self.source_view)

    def test_actual_native_pose_matches(self):
        """@param self Current case. Real packet-backed native pixels match accepted source and company remaps."""
        self.assertGreater(self.verify_pose()["matched_source_pixels"], 100)

    def test_rehashed_wrong_native_pixel_refuses(self):
        """@param self Current case. A fresh raster hash cannot authorize a changed opaque source pixel."""
        family = self.source_view.families[self.frame["asset_id"]]
        poses, atlas = self.source_view.poses(family["asset_id"])
        pose = poses[checks.source_key(self.frame, family)]
        left, top, width, height = pose["box"]
        data = atlas.crop((left, top, left + width, top + height)).tobytes()
        offset = next(index for index, value in enumerate(data) if value != 0)
        x = self.frame["baseline"][0] + pose["anchor"][0] + offset % width
        y = self.frame["baseline"][1] + pose["anchor"][1] + offset // width
        native = bytearray(self.index.read_bytes())
        native[y * 640 + x] = (native[y * 640 + x] + 1) % 256
        self.index.write_bytes(native)
        self.publish()
        with self.assertRaisesRegex(ValueError, "body source mismatch"):
            self.verify_pose()

    def test_wrong_source_phase_refuses(self):
        """@param self Current case. A different wheel phase cannot replace the actual native moving pose."""
        self.frame["frame"] = 0
        self.publish()
        with self.assertRaisesRegex(ValueError, "body source mismatch"):
            self.verify_pose()

    def test_wrong_source_cargo_refuses(self):
        """@param self Current case. Iron source freight cannot replace actual Coal packet presentation."""
        self.frame["cargo_label"] = "IORE"
        self.publish()
        with self.assertRaisesRegex(ValueError, "body source mismatch"):
            self.verify_pose()

    def test_wrong_native_contact_refuses(self):
        """@param self Current case. An arbitrary shifted baseline cannot bypass native sorting contact ownership."""
        self.frame["baseline"][0] += 1
        self.publish()
        with self.assertRaisesRegex(ValueError, "native sorting origin/contact"):
            self.verify_pose()

    def test_rehashed_truncated_raster_refuses(self):
        """@param self Current case. Fresh provenance cannot conceal incomplete native indexed output."""
        self.index.write_bytes(self.index.read_bytes()[:-1])
        self.publish()
        with self.assertRaisesRegex(ValueError, "viewport size changed"):
            self.verify_pose()

    def test_partial_matrix_refuses_without_unit_scope(self):
        """@param self Current case. Production validation requires the entire independent native coverage matrix."""
        with self.assertRaisesRegex(ValueError, "exact required cargo/state/view coverage"):
            checks.verify(self.execution.relative_to(ROOT).as_posix(), self.source_view)

    def test_failed_native_process_refuses(self):
        """@param self Current case. Plausible pixels cannot qualify an unsuccessful native process."""
        self.proof["passed"] = False
        self.publish()
        with self.assertRaisesRegex(ValueError, "successful actual native"):
            self.verify_pose()


if __name__ == "__main__":
    run_checks(NativeTransportPixelRefusals, 'rail')
