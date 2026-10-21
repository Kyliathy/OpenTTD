"""Verify one explicitly selected genuine launch-center quarter-turn component.

@file launch_facility_rotation_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups61DB-61DE.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Use --direction and --component for the current original only. Native proofs,
exact NE predecessor identities and current source inputs are read-only. Native
PNG/review/proof outputs rebuild under logs/proofs/mlm/native-art/space/.
"""

import argparse
from copy import deepcopy
import hashlib
import json
import unittest

from PIL import Image

from launch_facility_art_contract_checks import ROOT, PROOFS as NATIVE_PROOFS, read_json, verify_native_proof
from tools import launch_facility_base as base
from tools import launch_facility_source as source

MODEL = ROOT / "media/multilayer/source/space/launch-center-rotation-model-v1.json"
PROOFS = ROOT / "logs/proofs/mlm/native-art/space/launch-center-components-v1"
FACADES = {"SE": ["local_-Y", "local_+X"], "SW": ["local_-X", "local_-Y"], "NW": ["local_+Y", "local_-X"]}


class LaunchRotationChecks(unittest.TestCase):
    """Require genuine selected-view identity, native zone registration and exact ancestry."""

    direction = "SE"
    component = "workshop"

    def setUp(self):
        """Resolve this selected original through an exact versioned model reference.

        @param self Active focused native-view source test.
        @return None.
        """
        self.model = read_json(MODEL)
        self.selected = self.model["selected_components"][self.direction][self.component]
        self.candidate = base.bound_json(self.selected["generation_receipt"])

    def test_actual_native_view_zone_and_physical_facades(self):
        """Both compiled producers and the reviewed physical view qualify this source.

        @param self Active native geometry and selected physical model check.
        @return None.
        """
        for configuration in ("RelWithDebInfo", "Debug"):
            verify_native_proof(read_json(NATIVE_PROOFS / f"native-{configuration}.json"))
        self.assertEqual(self.candidate["directions"], [self.direction])
        self.assertEqual(self.candidate["component"], self.component)
        self.assertEqual(self.candidate["physical_view"]["visible_facades"], FACADES[self.direction])
        native = source.component_geometry(self.component, self.direction)
        self.assertEqual(self.candidate["physical_view"]["native_rotation"], native["native_rotation"])
        self.assertEqual(self.candidate["native_transform"]["native_zone_corners"], native["floor_corners"])
        self.assertEqual(self.candidate["native_width_target"], source.COMPONENT_WIDTHS[self.component])

    def test_native_indexed_pixels_and_actual_zone_coverage_reproduce(self):
        """Independent conversions retain original pixels and every real zone cell.

        @param self Active repeated original/native-pixel acceptance check.
        @return None.
        """
        image, evidence = source.registered_component(self.candidate)
        repeated, second = source.registered_component(self.candidate)
        self.assertEqual((image.tobytes(), evidence), (repeated.tobytes(), second))
        self.assertEqual(evidence, self.candidate["registration_review"]["evidence"])
        self.assertTrue(all(evidence["covered_pixels_per_cell"]))
        self.assertEqual(evidence["native_sha256"], hashlib.sha256(image.tobytes()).hexdigest())
        stem = self.candidate["key"].removeprefix("launch-center-")
        with Image.open(PROOFS / f"{stem}-native.png") as stored:
            self.assertEqual((stored.tobytes(), stored.getpalette()), (image.tobytes(), image.getpalette()))

    def test_wrong_view_source_contact_scale_and_crop_refuse(self):
        """A relabelled NE source or changed physical registration cannot become this view.

        @param self Active independent original/pose/contact refusal check.
        @return None.
        """
        mutations = {"sha256": "0" * 64, "directions": ["NE"], "native_width_target": 1,
                     "native_transform/native_reference": [0, 0], "native_transform/uniform_scale": [1, 2],
                     "native_transform/crop_box": [1, 0, *self.candidate["image_size"]]}
        # Change one original or native fact at a time; preserving the other
        # valid facts exposes any missing source/view/registration guard.
        for field, value in mutations.items():
            with self.subTest(field=field):
                broken = deepcopy(self.candidate)
                keys = field.split("/")
                target = broken
                for key in keys[:-1]:
                    target = target[key]
                target[keys[-1]] = value
                with self.assertRaises(ValueError):
                    source.registered_component(broken)

    def test_exact_request_ancestry_selected_review_and_ne_predecessor(self):
        """The new view retains real tool inputs while accepted NE source stays immutable.

        @param self Active source ancestry, physical review and predecessor check.
        @return None.
        """
        candidate = self.candidate
        self.assertEqual(base.identity(ROOT / candidate["path"]), self.selected["original"])
        self.assertEqual(candidate["native_transform"], self.selected["native_transform"])
        self.assertEqual(candidate["tool"], "built-in image_gen")
        self.assertIn("Use case:", candidate["prompt"])
        self.assertGreaterEqual(candidate["layout_review"]["minimum_clearance"], 16)
        self.assertEqual(candidate["status"], "selected_component")
        self.assertEqual(candidate["contact_review"]["status"], "approved")
        self.assertEqual(candidate["model_review"]["status"], "approved_component_model")
        for reference in candidate["reference_inputs"]:
            self.assertEqual(base.identity(ROOT / reference["path"]), {key: reference[key] for key in ("path", "sha256")})
            self.assertTrue(reference["role"])
        predecessor = base.bound_json(self.model["selected_ne_base"])
        ne_model = base.bound_json(predecessor["component_model"])
        # Every original NE component remains exact, not only the new view's
        # immediate reference; a rotation cannot silently replace the base.
        for selected in ne_model["selected_components"].values():
            original = base.bound_json(selected["generation_receipt"])
            self.assertEqual(base.identity(ROOT / original["path"]), selected["original"])
        for field in ("source_coverage", "packed", "runtime"):
            self.assertFalse(candidate[field])
            self.assertFalse(self.model[field])


def main():
    """Rebuild only the explicitly selected current native-view component.

    @return None; publish exact source-only proof after four focused checks pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--direction", choices=list(FACADES), default="SE")
    parser.add_argument("--component", choices=list(source.COMPONENT_WIDTHS), default="workshop")
    args = parser.parse_args()
    LaunchRotationChecks.direction, LaunchRotationChecks.component = args.direction, args.component
    selected = read_json(MODEL)["selected_components"][args.direction][args.component]
    candidate = base.bound_json(selected["generation_receipt"])
    image, evidence = source.registered_component(candidate)
    stem = candidate["key"].removeprefix("launch-center-")
    image.save(PROOFS / f"{stem}-native.png")
    image.resize((1280, 896), Image.Resampling.NEAREST).save(PROOFS / f"{stem}-native-review.png")
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchRotationChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    proof = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Groups61DB-61DD; selected genuine component only.",
                 status="passed", cases=result.testsRun, direction=args.direction, component=args.component,
                 candidate=selected["generation_receipt"], model=base.identity(MODEL), evidence=evidence,
                 helper=base.identity(ROOT / "media/multilayer/tools/launch_facility_source.py"),
                 test=base.identity(ROOT / "src/tests/multilayer/launch_facility_rotation_checks.py"),
                 source_coverage=False, packed=False, runtime=False)
    (PROOFS / f"{args.component}-{args.direction.lower()}-verification.json").write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
