"""Qualify genuine rocket ready source fit without changing native facility owners.

@file cargo_rocket_ready_fit_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62D.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
--review rebuilds the explicit ready-fit/ PNG/report outputs under the rocket
native-art proof directory. No complete phase source, packing or game claim.
"""

import argparse
import copy
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
from tools import cargo_rocket_ready_fit as fit


class CargoRocketReadyFitChecks(unittest.TestCase):
    """Bind twelve real source mounts to accepted originals and physical foregrounds."""

    @classmethod
    def setUpClass(cls):
        """Measure the immutable source matrix once for this bounded focused burst.

        @param cls Active qualification suite; shared results remain read-only.
        @return None.
        """
        cls.expected = art.read_json(fit.FIXTURE)
        cls.results, cls.scenes = fit.observations()

    def test_real_whole_pixels_foregrounds_contacts_and_occlusion_match_review(self):
        """Every contact uses its corresponding genuine actor and unchanged original cuts.

        @param self Active exact visual-fit fixture check.
        @return None.
        """
        self.assertEqual(self.expected["direction_model"], art.source.identity(directions.MODEL))
        self.assertEqual(self.results, self.expected["observations"])
        self.assertEqual(set(self.results), set(fit.FACILITY_HEADINGS))
        # Original whole/mask hashes and exact per-zone visibility detect a
        # shifted actor, changed cut or global-front overlay despite valid counts.
        for heading, row in self.results.items():
            self.assertEqual(row["rocket_heading"], heading)
            self.assertEqual([entry["component"] for entry in row["zones"]], list(fit.ZONES))
            for entry in row["zones"]:
                self.assertEqual(entry["visible_pixels"]+entry["occluded_pixels"], entry["source_pixels"])
                if entry["component"] == "workshop":
                    self.assertEqual(entry["visible_pixels"], 0)
        self.assertEqual(self.results["NE"]["zones"][0]["occluded_pixels"], 54)
        self.assertEqual(self.results["NW"]["zones"][2]["occluded_pixels"], 50)

    def test_saved_four_view_source_diagrams_are_the_exact_reviewed_outputs(self):
        """Read actual stored PNGs and require their frozen identities and native aspect.

        @param self Active saved diagnostic artifact integrity check.
        @return None.
        """
        report = art.read_json(fit.OUTPUT/"source-fit.json")
        self.assertEqual(report["observations"], self.expected["observations"])
        self.assertEqual(report["artifacts"], self.expected["images"])
        # Each independent diagram preserves source aspect and exact reviewed
        # bytes; a relabelled/repainted proof image cannot substitute for evidence.
        for reference in report["artifacts"]:
            self.assertEqual(art.source.identity(ROOT/reference["path"]), reference)
            with Image.open(ROOT/reference["path"]) as image:
                self.assertEqual((image.mode, image.size), ("RGB", (1280, 992)))

    def test_partial_ready_alias_and_moved_facility_contact_are_refused(self):
        """A four-diagonal shortcut or a moved visual mount cannot qualify source fit.

        @param self Active ready completeness/native-contact refusal check.
        @return None.
        """
        model = copy.deepcopy(art.read_json(directions.MODEL))
        document = copy.deepcopy(art.source.read_record())
        del model["selected"]["N"]
        model["genuine_ready_views_complete"] = False
        del document["families"][art.ASSET]["selections"]["ready/N"]
        with self.assertRaisesRegex(ValueError, "complete eight-heading"):
            fit.checked_ready_inputs(model, document)
        source_model = art.read_json(art.MODEL)
        facility = copy.deepcopy(art.read_json(ROOT/source_model["launch_fit_reference"]["path"]))
        contact = facility["views"]["NE"]["contacts"]["pad"]
        contact["physical_display_contact"][0] += 1
        contact["display_offset_from_native_custody"][0] += 1
        contact["declared_actor_canvas_box"][0] += 1
        contact["declared_actor_canvas_box"][2] += 1
        with self.assertRaises(ValueError):
            art.occupied.compose_states(facility, "NE")

    def test_completed_source_payload_and_native_owners_stay_protected_with_phase_gate_open(self):
        """Ready qualification preserves accepted source and grants no live/published rocket.

        @param self Active predecessor and honest coverage gate check.
        @return None.
        """
        public = art.read_json(art.PUBLIC)
        art.checked_predecessor(art.source.read_record(), public)
        art.checked_model(art.read_json(art.MODEL))
        row = next(row for row in public["assets"] if row["asset_id"] == art.ASSET)
        self.assertEqual(row, art.read_json(art.FIXTURE)["rocket_before"])
        report = art.read_json(fit.OUTPUT/"source-fit.json")
        for field in ("source_coverage", "packed", "runtime"):
            self.assertFalse(report[field])
        self.assertTrue(report["ready_source_fit_only"])


def main():
    """Run the active ready-fit qualification and save bounded evidence identities.

    @return Zero only after all exact source/fit/refusal checks pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true")
    args = parser.parse_args()
    if args.review:
        fit.rebuild()
    start = time.monotonic()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(CargoRocketReadyFitChecks))
    report = dict(_documentation=__doc__, status="passed" if result.wasSuccessful() else "failed", cases=result.testsRun,
                  elapsed_seconds=round(time.monotonic()-start, 3), helper=art.source.identity(Path(fit.__file__)),
                  tests=art.source.identity(Path(__file__)), fixture=art.source.identity(fit.FIXTURE),
                  fit_review=art.source.identity(fit.OUTPUT/"source-fit.json"),
                  direction_model=art.source.identity(directions.MODEL),
                  source_coverage=False, packed=False, runtime=False)
    (fit.OUTPUT/"verification.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
