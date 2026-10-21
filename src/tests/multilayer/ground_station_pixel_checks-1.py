"""Semantic corruption checks for actual native station/glass/vehicle source composition.

@file ground_station_pixel_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Three real appearance captures are copied only beneath repository test-cache.
The scoped unit matrix isolates pixel/depth semantics; production coverage is also tested.
"""

import json
from pathlib import Path
import tempfile
import time
import unittest
from unittest.mock import patch

import ground_station_pixel_checks as checks
from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import reference

EXECUTION = "logs/graphics/selected-ground-runtime/20261021-station-corrected-01/modules-debug-02/modules.json"
OUTPUT = ROOT / "logs/graphics/selected-ground-runtime/20261021-station-corrected-01/pixel-verifier-cases"


class NativeStationPixelRefusals(unittest.TestCase):
    """Exercise actual native canopy-front source, body depth and destination glass filtering."""

    def setUp(self):
        """Copy one real loading module's three native appearance views into isolated scratch.

        @param self Current semantic case; original native/source artifacts remain immutable.
        """
        cache = ROOT / "test-cache/selected-ground-station-pixel-verifier"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="native-station-", dir=cache)
        self.directory = Path(self.temporary.name).resolve()
        self.proof = handoff.read_json(EXECUTION)
        original = next(row for row in self.proof["observation"]["artifacts"] if row["path"].endswith("/station-modules.json"))
        self.scene = handoff.read_json(original["path"])
        # Canopy-front6 has measured positive glass/body and independent
        # foreground overlap. Retain all three real appearance baselines.
        self.scene["frames"] = [row for row in self.scene["frames"] if row["asset_id"] == "facility.rail-station"
                                and row["graphics"] == 6 and row["state"] == "working" and row["frame"] == 1]
        self.frames = self.scene["frames"]
        self.assertEqual(len(self.frames), 3)
        (self.directory / "reports").mkdir()
        (self.directory / "images").mkdir()
        self.metadata = self.directory / "reports/station-modules.json"
        self.paths = []
        for frame in self.frames:
            for extension in (".idx", ".ppm"):
                path = self.directory / "images" / (frame["basename"] + extension)
                path.write_bytes((handoff.bound(original["path"]).parent.parent / "images" / path.name).read_bytes())
                self.paths.append(path)
        self.execution = self.directory / "derived-execution.json"
        self.publish()

    def tearDown(self):
        """Delete only the verified repository-local scratch copy for this semantic case.

        @param self Current semantic case.
        """
        if not self.directory.is_relative_to(ROOT / "test-cache"):
            raise ValueError("station pixel scratch cleanup escaped repository")
        self.temporary.cleanup()

    def publish(self):
        """Rehash modified scratch metadata/rasters so semantic refusal cannot rely on stale hashes.

        @param self Current semantic case.
        """
        self.metadata.write_text(json.dumps(self.scene) + "\n", encoding="utf-8")
        self.proof["observation"]["artifacts"] = [reference(path) for path in [self.metadata, *self.paths]]
        self.execution.write_text(json.dumps(self.proof) + "\n", encoding="utf-8")

    def verify_module(self):
        """Isolate this real module while preserving native source, geometry, palette and provenance checks.

        @param self Current semantic case.
        @return Three-appearance exact source composition result, or its semantic refusal.
        """
        identities = {(row["asset_id"], row["graphics"], row["state"], row["frame"], row["appearance"]) for row in self.frames}
        with patch.object(checks, "expected_matrix", return_value=identities):
            return checks.verify(self.execution.relative_to(ROOT).as_posix())

    def test_actual_native_module_matches(self):
        """@param self Current case. The actual canopy source/glass/body composition has positive native foreground evidence."""
        result = self.verify_module()
        self.assertGreater(result["glass_vehicle_pixels"], 0)
        self.assertGreater(result["reversed_depth_different_pixels"], 0)

    def test_rehashed_wrong_native_pixel_refuses(self):
        """@param self Current case. A fresh raster hash cannot authorize a changed source-owned canopy pixel."""
        scope, before, public = handoff.read_scope()
        family = next(row for row in public["assets"] if row["asset_id"] == "facility.rail-station")
        poses, atlas = handoff.source_poses(family, scope, before, {}, {})
        points = checks.source_pixels(poses["working", 1, "NE", 3, "back"], atlas, (289, 131))
        x, y = next(iter(points))
        path = self.directory / "images" / (self.frames[0]["basename"] + ".idx")
        data = bytearray(path.read_bytes())
        data[y * 640 + x] = (data[y * 640 + x] + 1) % 256
        path.write_bytes(data)
        self.publish()
        with self.assertRaisesRegex(ValueError, "composition differs"):
            self.verify_module()

    def test_wrong_destination_glass_filter_refuses(self):
        """@param self Current case. Source RGB or an identity map cannot replace native destination glass recolour."""
        self.scene["transparent_remap"] = list(range(256))
        self.publish()
        with self.assertRaisesRegex(ValueError, "composition differs"):
            self.verify_module()

    def test_wrong_native_roof_depth_refuses(self):
        """@param self Current case. Moving the roof below the actual carrier cannot preserve native foreground composition."""
        self.frames[0]["parts"][2]["origin"][2] = -16
        self.publish()
        with self.assertRaises(ValueError):
            self.verify_module()

    def test_detached_glass_parent_refuses(self):
        """@param self Current case. Glass cannot gain an independent sorting box even when its source remains identical."""
        self.frames[0]["parts"][3]["parent"] = True
        self.publish()
        with self.assertRaisesRegex(ValueError, "relative-glass ownership"):
            self.verify_module()

    def test_missing_disjoint_platform_refuses(self):
        """@param self Current case. A missing native part cannot pass by omitting its source-owned comparison pixels."""
        self.frames[0]["parts"].pop(1)
        self.publish()
        with self.assertRaisesRegex(ValueError, "omits or duplicates"):
            self.verify_module()

    def test_wrong_source_phase_refuses(self):
        """@param self Current case. Another authored phase cannot replace this actual native source tuple."""
        for frame in self.frames:
            frame["frame"] = 2
        self.publish()
        with self.assertRaisesRegex(ValueError, "accepted source/absence/appearance"):
            self.verify_module()

    def test_rehashed_truncated_native_raster_refuses(self):
        """@param self Current case. Fresh provenance cannot conceal incomplete actual indexed capture data."""
        path = next(path for path in self.paths if path.suffix == ".idx")
        path.write_bytes(path.read_bytes()[:-1])
        self.publish()
        with self.assertRaisesRegex(ValueError, "unexpected dimensions"):
            self.verify_module()

    def test_partial_matrix_refuses_without_unit_scope(self):
        """@param self Current case. Production source qualification requires every real module, phase and appearance."""
        with self.assertRaisesRegex(ValueError, "incomplete or duplicated"):
            checks.verify(self.execution.relative_to(ROOT).as_posix())

    def test_failed_native_process_refuses(self):
        """@param self Current case. Plausible composition cannot authorize an unsuccessful actual process."""
        self.proof["passed"] = False
        self.publish()
        with self.assertRaisesRegex(ValueError, "successful actual native"):
            self.verify_module()


if __name__ == "__main__":
    OUTPUT.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    with (OUTPUT / "cases.log").open("w", encoding="utf-8") as stream:
        result = unittest.TextTestRunner(stream=stream, verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(NativeStationPixelRefusals))
    report = {"_documentation": __doc__, "passed": result.wasSuccessful(), "cases": result.testsRun,
              "seconds": round(time.monotonic() - started, 3), "terminal": reference(OUTPUT / "cases.log"),
              "native_execution": reference(handoff.bound(EXECUTION)),
              "inputs": [reference(Path(__file__)), reference(Path(checks.__file__))]}
    (OUTPUT / "verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("passed", "cases", "seconds")}))
    raise SystemExit(0 if result.wasSuccessful() else 1)
