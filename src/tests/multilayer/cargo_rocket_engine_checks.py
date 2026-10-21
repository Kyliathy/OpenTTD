"""Qualify actual engine originals, unchanged hulls and all16 saved burn composites.

@file cargo_rocket_engine_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62F.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
--review rebuilds engine-native/contact PNGs and reports under the explicit
logs/proofs/mlm/native-art/space/cargo-rocket-engine-v1/ proof owner only.
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
sys.path.insert(0, str(ROOT/"media/multilayer"))
from tools import cargo_rocket_art as art
from tools import cargo_rocket_directions as directions
from tools import cargo_rocket_engine as engine
from tools import cargo_rocket_states as states

FIXTURE = ROOT/"src/tests/multilayer/cargo-rocket-engine-composition-fixture.json"


class CargoRocketEngineChecks(unittest.TestCase):
    """Require genuine registered parts, exact original body ownership and native gates."""

    @classmethod
    def setUpClass(cls):
        """Validate shared immutable inputs once for this bounded source burst.

        @param cls Active engine qualification suite; shared buffers stay read-only.
        @return None.
        """
        cls.document, cls.model = art.source.read_record(), art.read_json(engine.MODEL)
        cls.expected = art.read_json(FIXTURE)
        engine.checked_model(cls.model, cls.document)
        cls.parts = {frame: engine.engine_part(frame, cls.document, cls.model)[0] for frame in (0, 1)}
        cls.bodies = {heading: directions.direction_view(heading, cls.document)[0] for heading in art.DIRECTIONS}
        cls.report = art.read_json(engine.OUTPUT/"source-review.json")

    def test_two_genuine_parts_retain_frozen_registration_native_pixels_and_distinct_frames(self):
        """Actual source parts preserve exact reviewed pixels, mount, palette and pulse change.

        @param self Active original-part/native-contact check.
        @return None.
        """
        self.assertEqual(self.expected["engine_model"], art.source.identity(engine.MODEL))
        self.assertEqual(self.expected["policy"], art.source.identity(states.POLICY))
        hashes = []
        for frame, count in ((0, 50), (1, 40)):
            pixels = self.parts[frame]
            self.assertEqual((pixels.mode, pixels.size), ("P", (64, 48)))
            self.assertEqual(pixels.getpalette(), art.source.base.PALETTE_BYTES)
            self.assertEqual(sum(value != 0 for value in pixels.get_flattened_data()), count)
            self.assertFalse(set(pixels.get_flattened_data()) & (set(art.source.base.FORBIDDEN)-{0}))
            hashes.append(hashlib.sha256(pixels.tobytes()).hexdigest())
        self.assertNotEqual(hashes[0], hashes[1])
        # Source clearance/envelope uses alpha16, while native DOS quantization
        # retains alpha128. Faint edge columns/the last row are not indexed art;
        # their removal is the established quantizer, not clipping or source editing.
        self.assertEqual(self.parts[0].getbbox(), (29, 33, 37, 44))
        self.assertEqual(self.parts[1].getbbox(), (29, 33, 36, 41))

    def test_actual_saved16_images_preserve_every_body_pixel_and_reviewed_part_occlusion(self):
        """Decode original PNGs independently and compare frozen whole/ownership evidence.

        @param self Active all-heading/frame stored source integrity check.
        @return None.
        """
        entries = self.report["frames"]
        self.assertEqual(len(entries), 16)
        self.assertEqual(len({(row["direction"], row["frame"]) for row in entries}), 16)
        atlas = Image.open(engine.OUTPUT/"burn-native.png")
        self.assertEqual((atlas.mode, atlas.size), ("P", (512, 96)))
        # Each decoded image keeps all original nozzle/feet/hull colors. Only
        # real transparent body positions may reveal the genuine part beneath.
        for entry in entries:
            heading, frame = entry["direction"], entry["frame"]
            expected = self.expected["poses"][heading+"/"+str(frame)]
            self.assertEqual(art.source.identity(ROOT/entry["image"]["path"]), entry["image"])
            pixels = Image.open(ROOT/entry["image"]["path"])
            self.assertEqual((pixels.mode, pixels.size), ("P", (64, 48)))
            self.assertEqual(hashlib.sha256(pixels.tobytes()).hexdigest(), expected["whole_pixels_sha256"])
            self.assertEqual(pixels.getpalette(), art.source.base.PALETTE_BYTES)
            for hull, effect, actual in zip(self.bodies[heading].get_flattened_data(), self.parts[frame].get_flattened_data(), pixels.get_flattened_data()):
                self.assertEqual(actual, hull if hull else effect)
            self.assertEqual({name: entry[name] for name in expected}, expected)
            column = art.DIRECTIONS.index(heading)
            self.assertEqual(atlas.crop((column*64, frame*48, column*64+64, frame*48+48)).tobytes(), pixels.tobytes())
        # Current production composition must still reproduce the reviewed
        # pair after strict input/frame guards were added; no fixture is rebased.
        for frame in (0, 1):
            pixels, evidence = engine.compose_state("NE", "ascent", frame, self.document, self.model)
            self.assertEqual(hashlib.sha256(pixels.tobytes()).hexdigest(), self.expected["poses"]["NE/"+str(frame)]["whole_pixels_sha256"])

    def test_static_aliases_and_actual_funded_policy_never_repaint_an_idle_hull(self):
        """Only actual qualified native flight choices select the authored source effects.

        @param self Active static-body and paid native source selection check.
        @return None.
        """
        policy = art.read_json(states.POLICY)
        for state in policy["static_alias_states"]:
            for frame in (0, 1):
                pixels, evidence = engine.compose_state("NE", state, frame, self.document, self.model)
                self.assertEqual(pixels.tobytes(), self.bodies["NE"].tobytes())
                self.assertEqual(evidence["alias"], "ready/NE/0")
                self.assertIsNone(evidence["engine"])
        unpaid = states.source_pose("Ascent", "None", False, False, 222, policy)
        pixels, evidence = engine.compose_state("NE", unpaid["state"], unpaid["frame"], self.document, self.model)
        self.assertEqual(pixels.tobytes(), self.bodies["NE"].tobytes())
        funded = states.source_pose("ReturnRecovery", "HomeLost", True, True, 214, policy)
        pixels, evidence = engine.compose_state("NE", funded["state"], funded["frame"], self.document, self.model)
        self.assertEqual(hashlib.sha256(pixels.tobytes()).hexdigest(), self.expected["poses"]["NE/1"]["whole_pixels_sha256"])

    def test_changed_geometry_missing_frame_wrong_frame_and_rehashed_mount_are_refused(self):
        """Plausible replacements cannot override independent source/registration fixtures.

        @param self Active source-part/geometry/frame drift refusal check.
        @return None.
        """
        for mutation in ("mount", "envelope", "missing", "cross_frame", "rehashed_reference", "boolean_frame", "source"):
            document, model = copy.deepcopy(self.document), copy.deepcopy(self.model)
            key = model["selected"]["0"]["candidate"]
            candidate = next(row for row in document["candidates"] if row["key"] == key)
            if mutation == "mount":
                model["native_mount"] = [33, 34]
            elif mutation == "envelope":
                model["native_envelope"] = [0, 0, 64, 48]
            elif mutation == "missing":
                del model["selected"]["1"]
            elif mutation == "cross_frame":
                model["selected"]["0"] = copy.deepcopy(model["selected"]["1"])
            elif mutation == "rehashed_reference":
                candidate["native_transforms"]["AXIAL"]["source_reference"][0] += 75
            elif mutation == "boolean_frame":
                candidate["component_frame"] = False
            else:
                candidate["sha256"] = "0"*64
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                engine.engine_part(0, document, model)
        for frame in (-1, 2, True):
            with self.assertRaises(ValueError):
                engine.engine_part(frame, self.document, self.model)

    def test_original_converter_refuses_outside_crop_scale_contact_and_widened_canvas(self):
        """Native clipping guards operate before pasting independently of receipt hashes.

        @param self Active original-pixel extraction/registration refusal check.
        @return None.
        """
        key = self.model["selected"]["0"]["candidate"]
        original = next(row for row in self.document["candidates"] if row["key"] == key)
        for mutation in ("crop", "scale", "contact", "envelope"):
            candidate = copy.deepcopy(original)
            transform = candidate["native_transforms"]["AXIAL"]
            envelope = [27, 32, 38, 46]
            if mutation == "crop":
                transform["crop_box"][2] = 2000
            elif mutation == "scale":
                transform["uniform_scale"] = [1, 20]
            elif mutation == "contact":
                transform["native_reference"] = [0, 0]
            else:
                envelope = [0, 0, 128, 96]
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                art.source.source_pixels(candidate, "AXIAL", envelope)

    def test_all_engine_attempts_retain_full_prompt_inputs_alpha_and_rejection_history(self):
        """Failed projection drafts remain immutable/excluded instead of silently repaired.

        @param self Active complete source ancestry/model review audit.
        @return None.
        """
        exclusions = {row["key"]: row for row in self.document["excluded_sources"]}
        candidates = [row for row in self.document["candidates"] if row.get("component") == "engine_effect"]
        self.assertEqual(len(candidates), 4)
        # Every genuine attempt retains the actual executed prompt and image
        # inputs, even when its physical emitter projection was rejected.
        for candidate in candidates:
            reference = candidate["generation_receipt"]
            self.assertEqual(art.source.identity(ROOT/reference["path"]), reference)
            self.assertEqual(art.read_json(ROOT/reference["path"]), {k:v for k,v in candidate.items() if k != "generation_receipt"})
            self.assertEqual(art.source.identity(ROOT/candidate["path"])["sha256"], candidate["sha256"])
            self.assertEqual(candidate["tool"], "built-in image_gen")
            self.assertTrue(candidate["transparent_background"])
            self.assertIn("Use case:", candidate["prompt"])
            with Image.open(ROOT/candidate["path"]) as raw:
                self.assertEqual(raw.mode, "RGBA")
                self.assertEqual(list(raw.size), candidate["actual_size"])
            for actual in candidate["reference_inputs"]:
                self.assertTrue(actual["role"])
                self.assertEqual(art.source.identity(ROOT/actual["path"])["sha256"], actual["sha256"])
            if candidate["status"].startswith("rejected_"):
                self.assertEqual(exclusions[candidate["key"]]["status"], candidate["status"])
                with self.assertRaisesRegex(ValueError, "rejected"):
                    art.source.source_candidate(candidate["key"], self.document)

    def test_predecessor_ready_bodies_native_owners_and_public_payload_stay_exact(self):
        """Component qualification cannot mutate completed source or publish absent poses.

        @param self Active protected source/owner and still-open handoff gate check.
        @return None.
        """
        public = art.read_json(art.PUBLIC)
        art.checked_predecessor(self.document, public)
        states.checked_policy(art.read_json(states.POLICY))
        row = next(row for row in public["assets"] if row["asset_id"] == art.ASSET)
        before = art.read_json(art.FIXTURE)["rocket_before"]
        self.assertEqual({k:v for k,v in row.items() if k not in ("source", "authoring")},
                         {k:v for k,v in before.items() if k not in ("source", "authoring")})
        if self.document["families"][art.ASSET]["source_coverage"]:
            # A later legitimate complete-source handoff may update only its
            # own source row; it must retain actual112-position qualification.
            reference = self.document["families"][art.ASSET]["source_manifest"]
            report = art.read_json(ROOT/reference["path"])
            self.assertTrue(report["coverage_complete"])
            self.assertEqual(report["logical_pose_count"], 112)
        else:
            self.assertEqual(row["source"], before["source"])
        for field in ("source_coverage", "packed", "runtime"):
            self.assertFalse(self.report[field])


def main():
    """Run focused original/body/frame/refusal checks and save exact qualification inputs.

    @return Zero only after the active engine-source group passes.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true")
    args = parser.parse_args()
    if args.review:
        engine.rebuild()
    start = time.monotonic()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(CargoRocketEngineChecks))
    report = dict(_documentation=__doc__, status="passed" if result.wasSuccessful() else "failed", cases=result.testsRun,
                  elapsed_seconds=round(time.monotonic()-start, 3), helper=art.source.identity(Path(engine.__file__)),
                  tests=art.source.identity(Path(__file__)), engine_model=art.source.identity(engine.MODEL),
                  fixture=art.source.identity(FIXTURE), source_review=art.source.identity(engine.OUTPUT/"source-review.json"),
                  engine_compositions=16, source_coverage=False, packed=False, runtime=False)
    (engine.OUTPUT/"verification.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
