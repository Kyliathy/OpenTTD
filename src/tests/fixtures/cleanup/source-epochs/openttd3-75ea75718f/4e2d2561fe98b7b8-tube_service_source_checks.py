"""Focused native service enclosure reuse, coverage and immutable-source checks.

@file tube_service_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed inputs remain in memory; selected M1 source/payload files stay unchanged.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_tube_facilities as service


class TubeServiceSourceChecks(unittest.TestCase):
    """Verify compatible one-tile reuse and reject fictitious geometry/phases.

    @see Plan13 task48B.3; operating/runtime acceptance remains separate.
    """

    @classmethod
    def setUpClass(cls):
        """Derive one valid source baseline for independent malformed report cases.

        @param cls Current test class, holding immutable decoded baseline data.
        @return None; no source file is changed or published.
        """
        cls.baseline, cls.encoded = service.build()

    def setUp(self):
        """Isolate source metadata and report for each refusal case.

        @param self Current test instance.
        @return None; mutations cannot leak across tests or reach repository files.
        """
        self.document = json.loads(service.tube.RECORD.read_text(encoding="utf-8"))
        self.family = next(row for row in self.document["families"] if row["asset_id"] == service.ASSET)
        self.report = copy.deepcopy(self.baseline)

    def test_exact_complete_source(self):
        """Verify128 actual view/state/phase/part poses and exact regeneration.

        @param self Current test instance.
        @return None; retained proof must match original sources read-only.
        """
        self.assertEqual(self.report["pose_count"], 128)
        service.validate(self.report, self.encoded, self.document)
        service.publish(check=True)

    def test_existing_m1_pixels_unchanged(self):
        """Prove the shared converter still regenerates original M1 service bytes.

        @param self Current test instance.
        @return None; the new source exactly reuses the prior complete native model.
        """
        record = json.loads(service.facilities.RECORD.read_text(encoding="utf-8"))
        model = next(row for row in record["families"] if row["asset_id"] == "facility.rail-service")
        component = next(row for row in record["components"] if row["key"] == model["component"])
        report, files = service.facilities.derive({"families": [model], "components": [component], "candidates": []})
        path = service.tube.base.repository_path(report["atlases"][0]["path"])
        self.assertEqual(files[path], path.read_bytes())
        self.assertEqual(files[path], self.encoded)

    def test_draft_six_cell_geometry_refuses(self):
        """Reject the old generic six-cell draft as an actual native depot.

        @param self Current test instance.
        @return None; a rendered plaza cannot change construction/capacity ownership.
        """
        self.family.update(footprint=[3, 2], tile_variants=6)
        with self.assertRaisesRegex(ValueError, "reuse geometry/state mismatch"):
            service.build(self.document)

    def test_wrong_source_model_refuses(self):
        """Reject a road enclosure substituted for the compatible native rail depot.

        @param self Current test instance.
        @return None; shared status lamps do not establish model compatibility.
        """
        self.family["reuse_family"] = "facility.road-service"
        with self.assertRaisesRegex(ValueError, "reuse geometry/state mismatch"):
            service.build(self.document)

    def test_changed_retained_original_refuses(self):
        """Reject changed source identity in the selected pure converter input.

        @param self Current test instance.
        @return None; mutate expected hash only, never actual original bytes.
        """
        selected = service.sources(self.document)
        selected["families"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable facility source changed"):
            service.facilities.derive(selected, source_record=service.tube.RECORD, output_dir=service.tube.REVIEW)

    def test_missing_foreground_state_refuses(self):
        """Reject source coverage missing the actual final NW foreground phase.

        @param self Current test instance.
        @return None; another view or repeated phase cannot fill its place.
        """
        self.report["atlases"][0]["sprites"].pop()
        with self.assertRaisesRegex(ValueError, "state/view/phase/part coverage"):
            service.validate(self.report, self.encoded, self.document)

    def test_invented_world_cell_refuses(self):
        """Reject a source mapping claiming an unbuilt adjacent depot cell.

        @param self Current test instance.
        @return None; native one-tile geometry owns every part's physical cell.
        """
        self.report["atlases"][0]["sprites"][0]["world_cell"] = [1, 0]
        with self.assertRaisesRegex(ValueError, "anchor/world-cell mismatch"):
            service.validate(self.report, self.encoded, self.document)

    def test_false_reuse_provenance_refuses(self):
        """Reject a fabricated model ancestry behind intact source pixels.

        @param self Current test instance.
        @return None; provenance must describe the actual retained model.
        """
        self.report["source_reuse"]["asset_id"] = "facility.road-service"
        with self.assertRaisesRegex(ValueError, "false reuse provenance"):
            service.validate(self.report, self.encoded, self.document)

    def test_duplicated_lamp_phase_refuses(self):
        """Reject repeating phase0 as phase1 while keeping all pose identities.

        @param self Current test instance.
        @return None; actual retained display phases must remain distinct.
        """
        with Image.open(io.BytesIO(self.encoded)) as atlas:
            broken = atlas.copy()
        sprites = self.report["atlases"][0]["sprites"]
        first = next(row for row in sprites if (row["state"], row["direction"], row["frame"], row["part"]) == ("ready", "SE", 0, "back"))
        second = next(row for row in sprites if (row["state"], row["direction"], row["frame"], row["part"]) == ("ready", "SE", 1, "back"))
        x, y, width, height = first["box"]
        broken.paste(broken.crop((x, y, x + width, y + height)), tuple(second["box"][:2]))
        with self.assertRaisesRegex(ValueError, "repeats an authored phase"):
            service.validate(self.report, service.tube.indexed_png(broken), self.document)

    def test_painted_source_pixel_refuses(self):
        """Reject a painted extension outside the retained service enclosure.

        @param self Current test instance.
        @return None; native source reuse never manufactures missing artwork.
        """
        with Image.open(io.BytesIO(self.encoded)) as atlas:
            broken = atlas.copy()
        broken.putpixel((0, 0), next(value for value in broken.get_flattened_data() if value != 0))
        with self.assertRaisesRegex(ValueError, "changes retained source pixels"):
            service.validate(self.report, service.tube.indexed_png(broken), self.document)


if __name__ == "__main__":
    unittest.main()
