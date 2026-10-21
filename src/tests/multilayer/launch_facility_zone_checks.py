"""Focused pad/recovery/storage original-zone registration and provenance.

@file launch_facility_zone_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups61CB.2-61CB.4.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Use --component for the zone currently being authored. Rebuild its native
bitmap/review/proof under logs/proofs/mlm/native-art/space/launch-center-components-v1.
This verifies selected hardware, not a complete facility or runtime delivery.
"""

import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import unittest

from PIL import Image

from launch_facility_art_contract_checks import ROOT, PROOFS as NATIVE_PROOFS, digest, identity, read_json, verify_native_proof
from tools import launch_facility_source as source


MODEL = ROOT / "media/multilayer/source/space/launch-center-component-model-v1.json"
PROOFS = ROOT / "logs/proofs/mlm/native-art/space/launch-center-components-v1"
EXPECTED = {
    "pad": dict(cells=[[0, 0], [0, 1]], reference=[192, 88], width=80),
    "recovery": dict(cells=[[3, 0], [3, 1]], reference=[96, 136], width=80),
    "storage": dict(cells=[[0, 2], [1, 2], [2, 2], [3, 2]], reference=[192, 136], width=144),
}


def selected_receipt(component):
    """Resolve the exact reviewed version from the model rather than a filename guess.

    @param component Actual pad/recovery/storage role selected by the current task.
    @return Explicit selected generation receipt path beneath the repository.
    """
    selected = read_json(MODEL)["selected_components"][component]
    return ROOT / selected["generation_receipt"]["path"]


class LaunchZoneChecks(unittest.TestCase):
    """Require an original's actual native zone, stable physical size and exact ancestry."""

    component = "pad"

    def setUp(self):
        """Load the explicit selected original for the executing zone only.

        @param self Active independent zone-source test.
        @return None.
        """
        self.receipt = selected_receipt(self.component)
        self.candidate = read_json(self.receipt)

    def test_actual_native_zone_and_physical_reference(self):
        """Both current native producers qualify the exact cells and frozen source contact.

        @param self Active source-consumer producer-boundary test.
        @return None.
        """
        for configuration in ("RelWithDebInfo", "Debug"):
            verify_native_proof(read_json(NATIVE_PROOFS / f"native-{configuration}.json"))
        expected = EXPECTED[self.component]
        native = source.component_geometry(self.component, "NE")
        self.assertEqual(native["world_cells"], expected["cells"])
        self.assertEqual(self.candidate["native_transform"]["native_reference"], expected["reference"])
        self.assertEqual(self.candidate["native_width_target"], expected["width"])

    def test_native_pixels_and_cell_coverage_reproduce(self):
        """Independent conversions reproduce exact indexed pixels without losing zone coverage.

        @param self Active original-pixel and native-cell ownership test.
        @return None.
        """
        image, evidence = source.registered_component(self.candidate)
        repeated, second = source.registered_component(self.candidate)
        self.assertEqual(image.tobytes(), repeated.tobytes())
        self.assertEqual(evidence, second)
        self.assertEqual(evidence, self.candidate["registration_review"]["evidence"])
        self.assertEqual(hashlib.sha256(image.tobytes()).hexdigest(), evidence["native_sha256"])
        self.assertEqual(len(evidence["covered_pixels_per_cell"]), len(EXPECTED[self.component]["cells"]))
        self.assertTrue(all(evidence["covered_pixels_per_cell"]))
        stem = self.candidate["key"].removeprefix("launch-center-")
        with Image.open(PROOFS / f"{stem}-native.png") as stored:
            self.assertEqual(stored.tobytes(), image.tobytes())
            self.assertEqual(stored.getpalette(), image.getpalette())

    def test_source_zone_size_and_contact_corruption_refuse(self):
        """A changed byte identity, native contact, role or physical scale cannot publish.

        @param self Active independent source-refusal test.
        @return None.
        """
        reference = self.candidate["native_transform"]["native_reference"]
        mutations = [("sha256", "0" * 64), ("component", "workshop"), ("directions", []),
                     ("native_width_target", EXPECTED[self.component]["width"] + 1),
                     ("native_transform/native_reference", [reference[0] + 1, reference[1]]),
                     ("native_transform/uniform_scale", [1, 2]),
                     ("native_transform/crop_box", [1, 0, *self.candidate["image_size"]])]
        # Preserve every other field while corrupting one independent source
        # fact, so another guard cannot hide an omitted ownership/scale check.
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

    def test_selected_original_and_model_ancestry_are_exact(self):
        """Selection retains full source/request ancestry and makes only a partial-art claim.

        @param self Active original provenance and truthful-scope test.
        @return None.
        """
        self.assertEqual(digest(ROOT / self.candidate["path"]), self.candidate["sha256"])
        for reference in self.candidate["reference_inputs"]:
            self.assertEqual(digest(ROOT / reference["path"]), reference["sha256"])
        model = read_json(MODEL)
        selected = model["selected_components"][self.component]
        self.assertEqual(selected["generation_receipt"], identity(self.receipt))
        self.assertEqual(selected["native_transform"], self.candidate["native_transform"])
        self.assertEqual(self.candidate["status"], "selected_component")
        self.assertEqual(self.candidate["contact_review"]["status"], "approved")
        self.assertEqual(self.candidate["model_review"]["status"], "approved_component_model")
        self.assertGreaterEqual(self.candidate["layout_review"]["minimum_clearance"], 16)
        self.assertFalse(self.candidate["source_coverage"])
        self.assertFalse(self.candidate["packed"])
        self.assertFalse(self.candidate["runtime"])


def main():
    """Rebuild and verify only the explicitly selected native-zone component.

    @return None; publish exact partial-source evidence after the focused cases pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--component", choices=list(EXPECTED), default="pad")
    args = parser.parse_args()
    LaunchZoneChecks.component = args.component
    receipt = selected_receipt(args.component)
    candidate = read_json(receipt)
    image, evidence = source.registered_component(candidate)
    PROOFS.mkdir(parents=True, exist_ok=True)
    stem = candidate["key"].removeprefix("launch-center-")
    native_path = PROOFS / f"{stem}-native.png"
    review_path = PROOFS / f"{stem}-native-review.png"
    image.save(native_path, transparency=0)
    image.resize((1280, 896), Image.Resampling.NEAREST).save(review_path, transparency=0)
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchZoneChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    report = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61CB: selected native-zone original only; complete base/source/packing/runtime remain open.",
                  complete=True, component=args.component, focused_cases=result.testsRun,
                  original=identity(ROOT / candidate["path"]), receipt=identity(receipt), model=identity(MODEL),
                  native=identity(native_path), review=identity(review_path), registration=evidence,
                  sources=[identity(Path(__file__).resolve()), identity(Path(source.__file__))],
                  base_selected=False, source_coverage=False, packed=False, runtime=False)
    (PROOFS / f"{args.component}-verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
