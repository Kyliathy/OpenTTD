"""Check the genuine rocket base without reopening completed source families.

@file cargo_rocket_base_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62.1.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Rebuild review with --review; focused unittest output/proof stays in logs/.
No native hull, game tick, public art source or packed payload is written.
"""

import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys
import time
import unittest
from unittest import mock

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
from tools import cargo_rocket_art as art
from launch_facility_art_contract_checks import verify_native_proof

BASE_FIXTURE = ROOT / "src/tests/multilayer/cargo-rocket-base-fixture.json"
FIXED_FIELDS = ("asset_id", "native_model", "public_geometry", "required_logical_poses", "native_inputs",
                "native_roles", "model", "body_native_envelope", "native_art_envelope", "native_contact",
                "phase_policy", "launch_fit_reference", "source_predecessor_fixture")


class CargoRocketBaseChecks(unittest.TestCase):
    """Require original source, native geometry and fixed predecessor preservation."""

    def setUp(self):
        """Load exact inputs afresh so refusal mutations cannot contaminate later cases.

        @param self Active focused base test.
        @return None.
        """
        self.document = art.source.read_record()
        self.model = art.read_json(art.MODEL)
        self.expected = art.read_json(BASE_FIXTURE)
        self.public = art.read_json(art.PUBLIC)
        self.key = self.expected["candidate"]
        self.candidate = next(row for row in self.document["candidates"] if row["key"] == self.key)

    def test_complete_predecessor_and_other_public_sources_stay_exact(self):
        """Preserve eighteen completed families, all old raw pixels and public payloads.

        @param self Active append-only authoring check.
        @return None.
        """
        art.checked_predecessor(self.document, self.public)
        fixture = art.read_json(art.FIXTURE)
        self.assertEqual((fixture["family_count"], fixture["candidate_count"], fixture["exclusion_count"]), (18, 264, 26))
        self.assertEqual(len(fixture["other_public_rows"]), 147)
        for target in ("family", "prefix", "public"):
            # A coherent edit outside the new rocket family is still forbidden,
            # regardless of whether its own source bytes remain valid artwork.
            document, public = copy.deepcopy(self.document), copy.deepcopy(self.public)
            if target == "family":
                document["families"]["facility.launch-center"]["source_coverage"] = False
            elif target == "prefix":
                document["candidates"][0]["status"] = "changed"
            else:
                next(row for row in public["assets"] if row["asset_id"] == "vehicle.model-15")["anchor"] = [-31, -32]
            with self.assertRaises(ValueError):
                art.checked_predecessor(document, public)

    def test_actual_native_owners_and_original_public_geometry_remain_bound(self):
        """Read current native producer receipts and refuse changed model geometry.

        @param self Active native/public contract check.
        @return None.
        """
        art.checked_model(self.model)
        # Both real native producer processes were run at the launch boundary;
        # their source/executable identities must still match for source fit.
        for configuration in ("Debug", "RelWithDebInfo"):
            proof = art.read_json(ROOT / f"logs/proofs/mlm/native-art/space/launch-center-contract-v2/native-{configuration}.json")
            verify_native_proof(proof)
        for name, value in (("native_contact", [33, 32]), ("body_native_envelope", [0, 0, 64, 48]),
                            ("required_logical_poses", 16), ("native_art_envelope", [0, 0, 128, 96])):
            model = copy.deepcopy(self.model)
            model[name] = value
            with self.assertRaises(ValueError):
                art.checked_model(model)
        for name, value in (("anchor", [-31, -32]), ("frames", 1), ("directions", ["NE"]), ("parts", ["back", "front"])):
            model = copy.deepcopy(self.model)
            model["public_geometry"][name] = value
            with self.assertRaises(ValueError):
                art.checked_model(model)

    def test_selected_original_prompt_registration_and_pixels_match_frozen_review(self):
        """Require the exact reviewed whole model and immutable ready-base pixels.

        @param self Active selected source/golden-pixel check.
        @return None.
        """
        pixels, evidence = art.candidate_view(self.key, "NE", self.document, self.model)
        self.assertEqual(self.candidate["status"], "selected_base")
        self.assertEqual(self.candidate["generation_receipt"], self.expected["generation_receipt"])
        self.assertEqual({key: self.candidate[key] for key in ("path", "sha256")}, self.expected["original"])
        self.assertEqual(art.source.registration_signature(self.candidate, "NE"), self.expected["registration_sha256"])
        self.assertEqual(hashlib.sha256(pixels.tobytes()).hexdigest(), self.expected["native_pixels_sha256"])
        self.assertEqual(evidence["native_subject_bounds"], [19, 3, 46, 37])
        self.assertEqual(sum(value != 0 for value in pixels.get_flattened_data()), 423)
        self.assertEqual(pixels.getpalette(), art.source.base.PALETTE_BYTES)
        self.assertFalse(set(pixels.get_flattened_data()) & (set(art.source.base.FORBIDDEN) - {0}))
        self.assertEqual(art.semantic_digest({key: self.model[key] for key in FIXED_FIELDS}), self.expected["fixed_model_sha256"])
        with Image.open(ROOT / self.candidate["path"]) as raw:
            self.assertEqual(raw.mode, "RGBA")
            self.assertEqual(raw.size, (1254, 1254))
        self.assertIn("Use case: stylized-concept", self.candidate["prompt"])
        self.assertEqual(self.candidate["references"], [])

    def test_missing_duplicate_rejected_changed_original_and_receipt_are_refused(self):
        """A plausible model cannot bypass exact source/receipt selection.

        @param self Active original ancestry refusal check.
        @return None.
        """
        for mutation in ("missing", "duplicate", "rejected", "original", "receipt", "missing_heading"):
            document = copy.deepcopy(self.document)
            candidate = next(row for row in document["candidates"] if row["key"] == self.key)
            direction = "NE"
            if mutation == "missing":
                document["candidates"].remove(candidate)
            elif mutation == "duplicate":
                document["candidates"].append(copy.deepcopy(candidate))
            elif mutation == "rejected":
                candidate["status"] = "rejected_model"
            elif mutation == "original":
                candidate["sha256"] = "0" * 64
            elif mutation == "receipt":
                candidate["generation_receipt"]["sha256"] = "0" * 64
            else:
                direction = "N"
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                art.candidate_view(self.key, direction, document, self.model)

    def test_converter_refuses_clipping_bad_scale_bbox_and_reference(self):
        """Whole-source guards remain effective independently of receipt hashes.

        @param self Active actual-original conversion refusal check.
        @return None.
        """
        for mutation in ("crop", "bbox", "scale", "contact", "envelope"):
            candidate = copy.deepcopy(self.candidate)
            transform = candidate["native_transforms"]["NE"]
            envelope = self.model["body_native_envelope"]
            if mutation == "crop":
                transform["crop_box"][2] = 1400
            elif mutation == "bbox":
                transform["measured_subject_bbox_in_crop"][0] += 1
            elif mutation == "scale":
                transform["uniform_scale"] = [1, 10]
            elif mutation == "contact":
                transform["native_reference"] = [0, 0]
            else:
                envelope = [0, 0, 128, 96]
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                art.source.source_pixels(candidate, "NE", envelope)

    def test_coherently_rehashed_registration_cannot_move_the_selected_base(self):
        """The selected model binding outranks a changed family registration hash.

        @param self Active coherent native-reference drift refusal check.
        @return None.
        """
        document = copy.deepcopy(self.document)
        candidate = next(row for row in document["candidates"] if row["key"] == self.key)
        candidate["native_transforms"]["NE"]["source_reference"][0] += 29
        document["families"][art.ASSET]["approved_base_registration"]["NE"] = art.source.registration_signature(candidate, "NE")
        forged = {key: value for key, value in candidate.items() if key != "generation_receipt"}
        read_json = art.read_json

        def forged_receipt(path):
            """Simulate coherent changed receipt metadata without rewriting originals.

            @param path Explicit document requested by the production guard.
            @return Detached forged receipt only at the exact candidate path.
            """
            return forged if path == ROOT / candidate["generation_receipt"]["path"] else read_json(path)

        with mock.patch.object(art, "read_json", side_effect=forged_receipt):
            with self.assertRaisesRegex(ValueError, "selected cargo rocket base changed"):
                art.candidate_view(self.key, "NE", document, self.model)

    def test_actual_zone_fit_uses_original_foregrounds_and_keeps_runtime_open(self):
        """Verify fixed source mounts and honest occlusion without modifying the facility.

        @param self Active occupied-source fit and incomplete-coverage check.
        @return None.
        """
        pixels, evidence = art.candidate_view(self.key, "NE", self.document, self.model)
        model = art.read_json(ROOT / self.model["launch_fit_reference"]["path"])
        fixture = art.occupied.compose_states(model, "NE")
        observations = []
        # Frozen counts bind real source silhouettes and reviewed physical cuts;
        # a sealed workshop may fully hide a hull without a false reveal mask.
        for component in ("pad", "workshop", "recovery"):
            scene, observation = art.fit_scene(pixels, model, "NE", component, fixture)
            observations.append(observation)
        self.assertEqual(observations, self.expected["fit"])
        row = next(row for row in self.public["assets"] if row["asset_id"] == art.ASSET)
        self.assertEqual(row, art.read_json(art.FIXTURE)["rocket_before"])
        for owner in (self.model, self.candidate, self.document["families"][art.ASSET]):
            self.assertFalse(owner["source_coverage"])
            self.assertFalse(owner["packed"])


def main():
    """Run only the active rocket base group and save its bounded evidence.

    @return Process status; unittest details are redirected by the caller to logs/.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true")
    args = parser.parse_args()
    if args.review:
        art.rebuild()
    start = time.monotonic()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(CargoRocketBaseChecks))
    report = dict(_documentation=__doc__, status="passed" if result.wasSuccessful() else "failed",
                  cases=result.testsRun, elapsed_seconds=round(time.monotonic()-start, 3),
                  helper=art.source.identity(Path(art.__file__)), tests=art.source.identity(Path(__file__)),
                  model=art.source.identity(art.MODEL), fixture=art.source.identity(BASE_FIXTURE),
                  source_review=art.source.identity(art.OUTPUT / "source-review.json"),
                  source_coverage=False, packed=False, runtime=False)
    art.OUTPUT.mkdir(parents=True, exist_ok=True)
    (art.OUTPUT / "verification.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
