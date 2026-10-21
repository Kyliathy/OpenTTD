"""Run focused genuine-ready-heading checks for the active rocket source group.

@file cargo_rocket_direction_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups62B-62D.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
--review rebuilds source contacts; logs/proofs/mlm/native-art/space/cargo-rocket-directions-v1
holds reproducible PNGs/reports. Paid native state and public payloads are untouched.
"""

import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys
import time
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
from tools import cargo_rocket_art as art
from tools import cargo_rocket_directions as directions


class CargoRocketDirectionChecks(unittest.TestCase):
    """Protect selected physical headings, immutable ancestors and honest omissions."""

    def setUp(self):
        """Read fresh detached inputs so refusal cases cannot contaminate source metadata.

        @param self Active focused heading test.
        @return None.
        """
        self.document = art.source.read_record()
        self.model = art.read_json(directions.MODEL)
        self.public = art.read_json(art.PUBLIC)

    def test_each_selected_heading_matches_immutable_original_native_pixels_and_palette(self):
        """Every genuine heading retains its independent receipt, contact and indexed art.

        @param self Active frozen-heading check.
        @return None.
        """
        self.assertTrue({"N", "NE", "E", "SE"} <= set(self.model["selected"]))
        hashes = []
        # A separate reviewed fixture binds each actual view; a plausible copy
        # of another heading cannot fill a missing physical yaw slot.
        for direction in self.model["selected"]:
            pixels, evidence = directions.direction_view(direction, self.document, self.model)
            self.assertEqual(pixels.size, (64, 48))
            self.assertEqual(pixels.getpalette(), art.source.base.PALETTE_BYTES)
            self.assertFalse(set(pixels.get_flattened_data()) & (set(art.source.base.FORBIDDEN)-{0}))
            self.assertEqual(evidence["transform"]["native_reference"], [32, 32])
            hashes.append(hashlib.sha256(pixels.tobytes()).hexdigest())
        self.assertEqual(len(hashes), len(set(hashes)))

    def test_complete_predecessor_current_native_owners_and_public_geometry_stay_exact(self):
        """Preserve eighteen completed source families and current actual owner bindings.

        @param self Active predecessor/native/public guard check.
        @return None.
        """
        art.checked_predecessor(self.document, self.public)
        art.checked_model(art.read_json(art.MODEL))
        fixture = art.read_json(art.FIXTURE)
        self.assertEqual((fixture["family_count"], fixture["candidate_count"], fixture["exclusion_count"]), (18, 264, 26))
        row = next(row for row in self.public["assets"] if row["asset_id"] == art.ASSET)
        self.assertEqual(row, fixture["rocket_before"])
        self.assertEqual(art.read_json(art.MODEL)["required_logical_poses"], 112)

    def test_missing_heading_cross_heading_alias_and_fabricated_completion_are_refused(self):
        """Ready omissions cannot become copied views, phase source or published coverage.

        @param self Active incomplete-source refusal check.
        @return None.
        """
        # All currently omitted headings are real unmet source obligations,
        # never implicit aliases to a symmetric cylinder or neighboring view.
        for missing in set(art.DIRECTIONS)-set(self.model["selected"]):
            with self.assertRaisesRegex(ValueError, "missing genuine"):
                directions.direction_view(missing, self.document, self.model)
        model = copy.deepcopy(self.model)
        model["selected"]["E"] = copy.deepcopy(model["selected"]["N"])
        document = copy.deepcopy(self.document)
        document["families"][art.ASSET]["selections"]["ready/E"] = model["selected"]["N"]["candidate"]
        with self.assertRaises(ValueError):
            directions.direction_view("E", document, model)
        for field in ("genuine_ready_views_complete", "source_coverage", "packed", "runtime"):
            model = copy.deepcopy(self.model)
            # Completion of eight ready views will eventually be legitimate;
            # the other flags still require their own phase/packing/game gates.
            if field == "genuine_ready_views_complete" and len(model["selected"]) == 8:
                continue
            model[field] = True
            with self.subTest(field=field), self.assertRaises(ValueError):
                directions.checked_model(model, self.document)

    def test_rehashed_registration_wrong_yaw_and_fixture_substitution_are_refused(self):
        """Coherent family metadata cannot overrule the independent reviewed fixture.

        @param self Active selected-registration and physical-landmark refusal check.
        @return None.
        """
        for mutation in ("reference", "fixture", "yaw", "missing", "duplicate", "source"):
            document, model = copy.deepcopy(self.document), copy.deepcopy(self.model)
            key = model["selected"]["E"]["candidate"]
            candidate = next(row for row in document["candidates"] if row["key"] == key)
            if mutation == "reference":
                candidate["native_transforms"]["E"]["source_reference"][0] += 29
                document["families"][art.ASSET]["approved_base_registration"]["E"] = art.source.registration_signature(candidate, "E")
            elif mutation == "fixture":
                model["selected"]["E"]["fixture"] = copy.deepcopy(model["selected"]["N"]["fixture"])
            elif mutation == "yaw":
                candidate["physical_view"]["yaw_from_ne"] = 0
            elif mutation == "missing":
                document["candidates"].remove(candidate)
            elif mutation == "duplicate":
                document["candidates"].append(copy.deepcopy(candidate))
            else:
                candidate["sha256"] = "0"*64
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                directions.direction_view("E", document, model)

    def test_all_rocket_attempts_retain_exact_requests_inputs_and_rejected_exclusions(self):
        """Failed sheet/count attempts stay immutable and unavailable for selection.

        @param self Active complete rocket ancestry audit.
        @return None.
        """
        exclusions = {row["key"]: row for row in self.document["excluded_sources"]}
        # Rejection does not discard history or waive reference identity; later
        # correction candidates explicitly retain their unselected edit target.
        for candidate in (row for row in self.document["candidates"] if row["asset_id"] == art.ASSET):
            receipt = candidate["generation_receipt"]
            self.assertEqual(art.source.identity(ROOT/receipt["path"]), receipt)
            self.assertEqual(art.read_json(ROOT/receipt["path"]), {k:v for k,v in candidate.items() if k != "generation_receipt"})
            self.assertEqual(art.source.identity(ROOT/candidate["path"])["sha256"], candidate["sha256"])
            self.assertTrue(candidate["transparent_background"])
            self.assertIn("Use case:", candidate["prompt"])
            for reference in candidate["reference_inputs"]:
                self.assertTrue(reference["role"])
                self.assertEqual(art.source.identity(ROOT/reference["path"])["sha256"], reference["sha256"])
            if candidate["status"].startswith("rejected_"):
                self.assertEqual(exclusions[candidate["key"]]["status"], candidate["status"])
                with self.assertRaisesRegex(ValueError, "rejected"):
                    art.source.source_candidate(candidate["key"], self.document)

    def test_rejected_frame_sheet_fails_visible_alpha_separators_and_clearance(self):
        """Measure actual rejected pixels without confusing hidden RGB with opaque art.

        @param self Active real failed-output extraction check.
        @return None.
        """
        candidate = next(row for row in self.document["candidates"] if row["key"] == "cargo-rocket-ready-n-e-se-s-v1")
        image = Image.open(ROOT/candidate["path"]).convert("RGBA")
        self.assertEqual(image.size, (1536, 1024))
        self.assertEqual(image.getpixel((100, 100))[3], 0)
        # Only alpha-visible separator pixels count. Every requested source
        # rectangle independently fails; no guessed mask may rescue this sheet.
        for crop, expected in zip(directions.guides.CROPS, (545, 61, 567, 181)):
            part = image.crop(crop)
            bounds = part.getchannel("A").point([0]*16+[255]*240).getbbox()
            self.assertEqual(min(bounds[0], bounds[1], part.width-bounds[2], part.height-bounds[3]), 0)
            self.assertEqual(sum(art.source.source_separator(pixel) for pixel in part.get_flattened_data()), expected)

    def test_stored_native_atlas_decodes_exact_selected_views_with_open_phase_gate(self):
        """Decode the saved ready contact's actual indexed PNG, not in-memory claims.

        @param self Active saved-output and honest-coverage check.
        @return None.
        """
        report = art.read_json(directions.OUTPUT/"source-review.json")
        headings = [name for name in art.DIRECTIONS if name in self.model["selected"]]
        self.assertEqual(report["headings"], headings)
        self.assertEqual(report["direction_model"], art.source.identity(directions.MODEL))
        atlas = Image.open(directions.OUTPUT/"selected-ready-native.png")
        self.assertEqual(atlas.mode, "P")
        self.assertEqual(atlas.size, (64*len(headings), 48))
        # Each decoded column must remain its own frozen genuine body; source
        # flags still exclude the missing native flight states and runtime art.
        for index, direction in enumerate(headings):
            pixels, evidence = directions.direction_view(direction, self.document, self.model)
            self.assertEqual(atlas.crop((index*64, 0, (index+1)*64, 48)).tobytes(), pixels.tobytes())
        for field in ("source_coverage", "packed", "runtime"):
            self.assertFalse(report[field])


def main():
    """Run only the current rocket heading subset and write focused proof metadata.

    @return Zero when the bounded source/registration/refusal checks pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true")
    args = parser.parse_args()
    if args.review:
        directions.rebuild()
    start = time.monotonic()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(CargoRocketDirectionChecks))
    report = dict(_documentation=__doc__, status="passed" if result.wasSuccessful() else "failed", cases=result.testsRun,
                  elapsed_seconds=round(time.monotonic()-start, 3), helper=art.source.identity(Path(directions.__file__)),
                  tests=art.source.identity(Path(__file__)), model=art.source.identity(directions.MODEL),
                  source_review=art.source.identity(directions.OUTPUT/"source-review.json"),
                  covered_headings=list(art.read_json(directions.MODEL)["selected"]),
                  source_coverage=False, packed=False, runtime=False)
    (directions.OUTPUT/"verification.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
