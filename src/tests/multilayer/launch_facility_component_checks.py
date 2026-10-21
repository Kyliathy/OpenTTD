"""Focused original-workshop registration, native-zone ownership and refusals.

@file launch_facility_component_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61CB.1.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Rebuild the native indexed workshop/review and exact verification beneath
logs/proofs/mlm/native-art/space/launch-center-components-v1. This source
checkpoint grants no complete base, packed sprites or ordinary-game acceptance.
"""

from copy import deepcopy
import hashlib
import json
from pathlib import Path
import unittest

from PIL import Image

from launch_facility_art_contract_checks import ROOT, PROOFS as NATIVE_PROOFS, digest, identity, read_json, verify_native_proof
from tools import launch_facility_source as source


RECEIPT = ROOT / "media/multilayer/source/space/launch-center-workshop-ne-v1.json"
MODEL = ROOT / "media/multilayer/source/space/launch-center-component-model-v1.json"
PROOFS = ROOT / "logs/proofs/mlm/native-art/space/launch-center-components-v1"


class LaunchWorkshopComponentChecks(unittest.TestCase):
    """Bind the selected real workshop to actual native cells and measured contacts."""

    def setUp(self):
        """Load the explicitly selected original rather than discovering a latest file.

        @param self Active workshop source contract test.
        @return None.
        """
        self.candidate = read_json(RECEIPT)

    def test_native_world_zone_and_viewport_producer_are_current(self):
        """Both actual native configurations supply the same four real workshop cells.

        @param self Active producer and source-geometry boundary test.
        @return None.
        """
        for configuration in ("RelWithDebInfo", "Debug"):
            proof = read_json(NATIVE_PROOFS / f"native-{configuration}.json")
            verify_native_proof(proof)
            self.assertEqual(proof["assertions"], 305)
        geometry = source.component_geometry("workshop", "NE")
        self.assertEqual(geometry["world_cells"], [[1, 0], [2, 0], [1, 1], [2, 1]])
        self.assertEqual(geometry["floor_corners"], [[144, 80], [208, 112], [144, 144], [80, 112]])
        self.assertEqual(geometry["native_cell_origins"], [[113, 47], [81, 63], [145, 63], [113, 79]])

    def test_original_registration_reproduces_all_stored_native_pixels(self):
        """Two independent reductions reproduce the original indexed bitmap and zone coverage.

        @param self Active original-pixel reproducibility test.
        @return None.
        """
        first, evidence = source.registered_component(self.candidate)
        second, repeated = source.registered_component(self.candidate)
        self.assertEqual(first.tobytes(), second.tobytes())
        self.assertEqual(evidence, repeated)
        self.assertEqual(evidence, self.candidate["registration_review"]["evidence"])
        self.assertEqual(hashlib.sha256(first.tobytes()).hexdigest(), evidence["native_sha256"])
        self.assertEqual(len(evidence["covered_pixels_per_cell"]), 4)
        self.assertTrue(all(evidence["covered_pixels_per_cell"]))
        with Image.open(PROOFS / "workshop-ne-v1-native.png") as stored:
            self.assertEqual(stored.mode, "P")
            self.assertEqual(stored.tobytes(), first.tobytes())
            self.assertEqual(stored.getpalette(), first.getpalette())

    def test_shifted_contact_scale_zone_and_source_identity_refuse(self):
        """One-field corruption cannot move, resize or substitute the selected hardware.

        @param self Active native component corruption test.
        @return None.
        """
        mutations = [("sha256", "0" * 64), ("component", "pad"), ("directions", []),
                     ("native_width_target", 107), ("native_transform/native_reference", [145, 112]),
                     ("native_transform/uniform_scale", [1, 2]),
                     ("native_transform/source_reference", [630, 765]),
                     ("native_transform/native_zone_corners", [[0, 0]] * 4)]
        # Change each independent identity/geometry fact while preserving the
        # other fields, so a second mismatch cannot conceal a missing guard.
        for path, value in mutations:
            with self.subTest(field=path):
                broken = deepcopy(self.candidate)
                keys = path.split("/")
                target = broken
                for key in keys[:-1]:
                    target = target[key]
                target[keys[-1]] = value
                with self.assertRaises(ValueError):
                    source.registered_component(broken)

    def test_crop_measurement_and_missing_view_cannot_hide_lost_pixels(self):
        """Whole-source extraction and genuine original measurements precede cell coverage.

        @param self Active original-extraction refusal test.
        @return None.
        """
        for kind in ("crop", "bounds", "dimensions", "view"):
            broken = deepcopy(self.candidate)
            if kind == "crop":
                broken["native_transform"]["crop_box"][0] = 1
            elif kind == "bounds":
                # Preserve width to reach the original measurement check rather
                # than accidentally fail only the uniform-scale identity guard.
                broken["native_transform"]["measured_subject_bbox_in_crop"][0] += 1
                broken["native_transform"]["measured_subject_bbox_in_crop"][2] += 1
            elif kind == "dimensions":
                broken["image_size"][0] += 1
            else:
                broken["directions"] = ["unknown"]
            with self.assertRaises(ValueError):
                source.registered_component(broken)

    def test_wrong_whole_floor_originals_remain_rejected(self):
        """Selecting hardware does not reinterpret a failed whole-facility floor as valid.

        @param self Active complete-base distinction test.
        @return None.
        """
        for version in (5, 6):
            candidate = read_json(ROOT / f"media/multilayer/source/space/launch-center-base-ne-v{version}.json")
            self.assertEqual(candidate["status"], "rejected_nonuniform_tile_scale")
            self.assertGreater(candidate["geometry_review"]["maximum_corner_error"], 2)
            with self.assertRaises(ValueError):
                source.registered_base(candidate)
        with self.assertRaises(ValueError):
            source.registered_base(self.candidate)

    def test_actual_original_ancestry_and_selected_model_are_exact(self):
        """Originals, actual image inputs and the selected component remain independently bound.

        @param self Active component provenance and truthful-scope test.
        @return None.
        """
        self.assertEqual(digest(ROOT / self.candidate["path"]), self.candidate["sha256"])
        for reference in self.candidate["reference_inputs"]:
            self.assertEqual(digest(ROOT / reference["path"]), reference["sha256"])
        model = read_json(MODEL)
        selected = model["selected_components"]["workshop"]
        self.assertEqual(selected["generation_receipt"], identity(RECEIPT))
        self.assertEqual(selected["native_transform"], self.candidate["native_transform"])
        self.assertEqual(model["native_component_widths"], source.COMPONENT_WIDTHS)
        self.assertEqual(self.candidate["status"], "selected_component")
        self.assertEqual(self.candidate["contact_review"]["status"], "approved")
        self.assertEqual(self.candidate["model_review"]["status"], "approved_component_model")
        self.assertFalse(model["base_selected"])
        self.assertFalse(model["source_coverage"])
        self.assertFalse(model["packed"])
        self.assertFalse(model["runtime"])


def main():
    """Rebuild the selected workshop's native proof and run only its focused checks.

    @return None; publish exact source evidence only after all selected checks pass.
    """
    candidate = read_json(RECEIPT)
    image, evidence = source.registered_component(candidate)
    PROOFS.mkdir(parents=True, exist_ok=True)
    native_path = PROOFS / "workshop-ne-v1-native.png"
    review_path = PROOFS / "workshop-ne-v1-native-review.png"
    image.save(native_path, transparency=0)
    image.resize((1280, 896), Image.Resampling.NEAREST).save(review_path, transparency=0)
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchWorkshopComponentChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    report = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61CB.1: selected original workshop only; complete base/source/packing/runtime remain open.",
                  complete=True, focused_cases=result.testsRun, original=identity(ROOT / candidate["path"]),
                  receipt=identity(RECEIPT), model=identity(MODEL), native=identity(native_path), review=identity(review_path),
                  registration=evidence, sources=[identity(Path(__file__).resolve()), identity(Path(source.__file__))],
                  base_selected=False, source_coverage=False, packed=False, runtime=False)
    (PROOFS / "workshop-verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
