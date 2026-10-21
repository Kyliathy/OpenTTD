"""Focused semantic corruption checks for actual selected-industry pixel verification.

@file ground_industry_pixel_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHB.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Derived one-frame inputs live only in repository test-cache; freshly rehashed
bad pixels must still refuse. No native capture or accepted source is changed.
"""

import json
from pathlib import Path
import tempfile
import time
import unittest

from PIL import Image

import ground_industry_pixel_checks as checks
from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import reference

EXECUTION = "logs/graphics/selected-ground-runtime/20261021-industry-01/states-debug/states.json"
OUTPUT = ROOT / "logs/graphics/selected-ground-runtime/20261021-industry-01/pixel-verifier-cases"


class NativeIndustryPixelRefusals(unittest.TestCase):
    """Exercise real source/capture semantics through disposable copies of one actual working scene."""

    def setUp(self):
        """Create one explicitly derived repository-owned native raster fixture.

        @param self Current isolated case; original process/source artifacts remain immutable.
        """
        cache = ROOT / "test-cache/selected-ground-pixel-verifier"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="native-scene-", dir=cache)
        self.directory = Path(self.temporary.name).resolve()
        proof = handoff.read_json(EXECUTION)
        metadata = next(row for row in proof["observation"]["captures"] if row["path"].endswith("/industry-fixture.json"))
        original = handoff.bound(metadata["path"])
        scene = handoff.read_json(metadata["path"])
        self.frame = next(row for row in scene["frames"] if row["asset_id"] == "mine.COAL" and row["scene"] == "working-1")
        self.frame = dict(self.frame)
        self.scene = {"_documentation": __doc__, "frames": [self.frame]}
        (self.directory / "reports").mkdir()
        (self.directory / "images").mkdir()
        self.index = self.directory / "images" / (self.frame["basename"] + ".idx")
        self.rgb = self.index.with_suffix(".ppm")
        # Copy only this test's one actual scene. No fabricated alternative
        # imagery can satisfy the positive case's accepted source comparison.
        for path in (self.index, self.rgb):
            path.write_bytes((original.parent.parent / "images" / path.name).read_bytes())
        self.metadata = self.directory / "reports/industry-fixture.json"
        self.proof = proof
        self.proof["_documentation"] = __doc__
        self.execution = self.directory / "derived-native-execution.json"
        self.publish()

    def tearDown(self):
        """Remove only this case's verified repository scratch directory.

        @param self Current isolated case.
        """
        if not self.directory.is_relative_to(ROOT / "test-cache"):
            raise ValueError("pixel corruption cleanup target escaped repository test-cache")
        self.temporary.cleanup()

    def publish(self):
        """Rehash derived inputs so semantic refusal cannot be satisfied by stale provenance alone.

        @param self Current isolated case.
        """
        self.metadata.write_text(json.dumps(self.scene) + "\n", encoding="utf-8")
        self.proof["observation"]["captures"] = [reference(path) for path in (self.metadata, self.index, self.rgb)]
        self.execution.write_text(json.dumps(self.proof) + "\n", encoding="utf-8")

    def accepted_pixels(self):
        """Read expected source ownership independently from this case's copied native raster.

        @param self Current isolated case.
        @return Projected actual accepted Coal source pixels for locating a real corruption target.
        """
        scope = handoff.read_json("media/multilayer/ground-pack-scope.json")
        source = next(row for row in scope["source_rows"] if row["asset_id"] == "mine.COAL")
        document = handoff.read_json(source["manifest"])
        poses = [row for row in document["source_parts"] if row["state"] == "working" and row["frame"] == 1]
        with Image.open(handoff.bound(source["source_atlas"])) as atlas:
            return checks.project(poses, atlas, self.frame)

    def test_actual_native_working_pixels_match(self):
        """@param self Current case. Actual native working pixels retain exact accepted source ownership."""
        result = checks.verify(self.execution.relative_to(ROOT).as_posix())
        self.assertTrue(result["passed"])
        self.assertGreater(result["matched_source_pixels"], 10000)

    def test_freshly_rehashed_wrong_native_pixel_refuses(self):
        """@param self Current case. A truthful new hash cannot authorize one wrong machinery pixel."""
        (x, y), value = next(iter(self.accepted_pixels().items()))
        data = bytearray(self.index.read_bytes())
        data[y * 640 + x] = value + 1 if value < 255 else 1
        self.index.write_bytes(data)
        self.publish()
        with self.assertRaisesRegex(ValueError, "source pixels differ"):
            checks.verify(self.execution.relative_to(ROOT).as_posix())

    def test_wrong_working_phase_refuses(self):
        """@param self Current case. Another declared phase cannot stand in for this actual working raster."""
        self.frame["frame"] = 2
        self.publish()
        with self.assertRaisesRegex(ValueError, "source pixels differ"):
            checks.verify(self.execution.relative_to(ROOT).as_posix())

    def test_wrong_native_footprint_refuses(self):
        """@param self Current case. A guessed footprint cannot move accepted source cells into a different geometry."""
        self.frame["width"] = 4
        self.publish()
        with self.assertRaisesRegex(ValueError, "exact native cell/part"):
            checks.verify(self.execution.relative_to(ROOT).as_posix())

    def test_unknown_source_state_refuses(self):
        """@param self Current case. Missing states do not fall back to another accepted source pose."""
        self.frame["state"] = "unknown"
        self.publish()
        with self.assertRaisesRegex(ValueError, "exact native cell/part"):
            checks.verify(self.execution.relative_to(ROOT).as_posix())

    def test_rehashed_truncated_native_raster_refuses(self):
        """@param self Current case. Fresh provenance cannot conceal a clipped or incomplete native index raster."""
        self.index.write_bytes(self.index.read_bytes()[:-1])
        self.publish()
        with self.assertRaisesRegex(ValueError, "unexpected dimensions"):
            checks.verify(self.execution.relative_to(ROOT).as_posix())

    def test_unsuccessful_native_process_refuses(self):
        """@param self Current case. Failed native execution cannot gain acceptance from plausible pixels."""
        self.proof["passed"] = False
        self.publish()
        with self.assertRaisesRegex(ValueError, "successful actual native"):
            checks.verify(self.execution.relative_to(ROOT).as_posix())


if __name__ == "__main__":
    OUTPUT.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    with (OUTPUT / "cases.log").open("w", encoding="utf-8") as stream:
        result = unittest.TextTestRunner(stream=stream, verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(NativeIndustryPixelRefusals))
    report = {"_documentation": __doc__, "passed": result.wasSuccessful(), "cases": result.testsRun,
              "seconds": round(time.monotonic() - started, 3), "terminal": reference(OUTPUT / "cases.log"),
              "native_execution": reference(handoff.bound(EXECUTION)),
              "inputs": [reference(Path(__file__)), reference(Path(checks.__file__))]}
    (OUTPUT / "verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report))
    raise SystemExit(0 if result.wasSuccessful() else 1)
