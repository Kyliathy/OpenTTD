"""Check decoded original tube/geothermal menu states and source corruption refusals.

@file feature_control_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group78C.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md completed Group78B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Source checks supply no toolbar binding, packed sprite, availability or operating provider.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_feature_controls as art


class FeatureControlSourceChecks(unittest.TestCase):
    """Inspect six actual native glyphs and refuse independently corrupted inputs.

    Plan7 owns shared menu geometry; feature commands retain admission and runtime.
    Each fixture modifies copied metadata or decoded pixels, never originals.
    """

    @classmethod
    def setUpClass(cls):
        """Decode the retained original record and one complete source baseline.

        @param cls Test class receiving unchanged baseline provenance and atlas bytes.
        @return None; every refusal starts from a separate deep copy.
        """
        cls.record = json.loads(art.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(art.base.PUBLIC.read_text(encoding="utf-8"))
        cls.reports, cls.files = art.build(cls.record, cls.public)

    def test_six_complete_exact_source_states(self):
        """Require exact decoded three-state coverage for both actual menu identities.

        @param self Current isolated test case.
        @return None; public source remains separate from packed/runtime activation.
        """
        art.validate(self.reports, self.files, self.record, self.public)
        self.assertEqual(sum(len(report["sprites"]) for report in self.reports.values()), 6)
        for asset_id, report in self.reports.items():
            self.assertEqual(art.base.repository_path(report["path"]).read_bytes(), self.files[asset_id])
            self.assertEqual(art.public_family(asset_id, self.public)["consumer"], 7)
            # The public contract already reserves optional-extension offsets
            # 152/155. Source evidence must not turn those declarations into a
            # packed/runtime claim; their actual preservation is checked at publication.
            self.assertNotIn("packed", report)
            self.assertFalse(report["runtime_activated"])

    def test_native_bounds_distinct_states_and_original_pixels(self):
        """Inspect actual crop geometry, unchanged original resampling and UI-state differences.

        @param self Current isolated test case.
        @return None; copied states, shifted glyphs and false ancestry cannot satisfy coverage.
        """
        self.assertEqual(len({row["sha256"] for row in self.record["candidates"]}), 6)
        # Each independently generated state retains the same native extent
        # within its family. Actual indexed pixels match its retained original.
        for family in self.record["families"]:
            asset_id = family["asset_id"]
            bounds, poses = [], []
            with Image.open(io.BytesIO(self.files[asset_id])) as atlas:
                for column, state in enumerate(art.STATES):
                    pose = atlas.crop((column * 20, 0, column * 20 + 20, 20))
                    original, source = art.native_original(family["state_selections"][state], self.record, self.public)
                    self.assertEqual(pose.tobytes(), original.tobytes())
                    bounds.append(pose.getbbox())
                    poses.append(pose.tobytes())
            self.assertEqual(len(set(bounds)), 1)
            self.assertEqual(len(set(poses)), 3)

    def test_missing_family_and_state_refuse(self):
        """Remove a whole feature or one required independently authored UI state.

        @param self Current isolated test case.
        @return None; total bytes from another icon cannot replace missing source identity.
        """
        record = copy.deepcopy(self.record)
        record["families"].pop()
        with self.assertRaisesRegex(ValueError, "family coverage"):
            art.build(record, self.public)
        record = copy.deepcopy(self.record)
        record["families"][0]["state_selections"].pop("disabled")
        with self.assertRaisesRegex(ValueError, "authored feature control state"):
            art.build(record, self.public)

    def test_duplicate_state_and_foreign_original_refuse(self):
        """Relabel enabled as selected or substitute the other feature's original.

        @param self Current isolated test case.
        @return None; captions and correct raw hashes cannot create another model/state.
        """
        record = copy.deepcopy(self.record)
        record["families"][0]["state_selections"]["pressed"] = record["families"][0]["state_selections"]["enabled"]
        with self.assertRaisesRegex(ValueError, "duplicate authored"):
            art.build(record, self.public)
        record = copy.deepcopy(self.record)
        record["families"][0]["state_selections"]["enabled"] = record["families"][1]["state_selections"]["enabled"]
        with self.assertRaisesRegex(ValueError, "substitutes"):
            art.build(record, self.public)

    def test_rejected_original_and_frozen_registration_refuse(self):
        """Exclude an original or shift its measured native registration after review.

        @param self Current isolated test case.
        @return None; review exclusions and fixed menu anchors remain authoritative.
        """
        record = copy.deepcopy(self.record)
        record["candidates"][0]["status"] = "rejected_test_only"
        with self.assertRaisesRegex(ValueError, "rejected"):
            art.build(record, self.public)
        record = copy.deepcopy(self.record)
        record["candidates"][0]["native_transform"]["native_position"][0] += 1
        with self.assertRaisesRegex(ValueError, "reviewed registration"):
            art.build(record, self.public)

    def test_changed_reference_and_missing_approval_refuse(self):
        """Corrupt edit ancestry or remove physical glyph approval without source-file writes.

        @param self Current isolated test case.
        @return None; immutable style/model references and reviewed placement are required.
        """
        record = copy.deepcopy(self.record)
        record["candidates"][0]["references"][0]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable"):
            art.build(record, self.public)
        public = copy.deepcopy(self.public)
        art.public_family("control.tube", public)["geometry_notes"]["approved_control_registrations"].clear()
        with self.assertRaisesRegex(ValueError, "reviewed native registration"):
            art.build(self.record, public)

    def test_ui_owner_world_rotation_and_native_anchor_refuse(self):
        """Try a feature-ledger owner, fictitious world directions or shifted menu anchor.

        @param self Current isolated test case.
        @return None; Plan7 UI geometry remains separate from native feature admission.
        """
        for mutation in ("owner", "rotation", "anchor"):
            public = copy.deepcopy(self.public)
            family = art.public_family("control.tube", public)
            if mutation == "owner":
                family["consumer"] = 17
            elif mutation == "rotation":
                family["directions"] = ["NE", "SE"]
            else:
                family["anchor"][0] = -1
            with self.assertRaisesRegex(ValueError, "geometry/state/owner"):
                art.build(self.record, public)

    def test_missing_duplicate_and_false_atlas_ancestry_refuse(self):
        """Corrupt actual pose identities or original metadata while retaining plausible pixels.

        @param self Current isolated test case.
        @return None; exact source mappings must agree with every retained original state.
        """
        for mutation in ("missing", "duplicate", "source"):
            reports = copy.deepcopy(self.reports)
            sprites = reports["control.tube"]["sprites"]
            if mutation == "missing":
                sprites.pop()
            elif mutation == "duplicate":
                sprites[-1] = copy.deepcopy(sprites[0])
            else:
                sprites[0]["source_sha256"] = "0" * 64
            with self.assertRaises(ValueError):
                art.validate(reports, self.files, self.record, self.public)

    def test_actual_pixel_palette_and_empty_state_refuse(self):
        """Change actual decoded pixels, reserved palette use or a missing raster state.

        @param self Current isolated test case.
        @return None; unchanged metadata cannot conceal corrupted native glyphs.
        """
        for mutation in ("pixel", "palette", "empty"):
            files = dict(self.files)
            with Image.open(io.BytesIO(files["control.tube"])) as image:
                atlas = image.copy()
            x, y = next((x, y) for y in range(20) for x in range(20) if atlas.getpixel((x, y)) != 0)
            if mutation == "pixel":
                atlas.putpixel((x, y), 1 if atlas.getpixel((x, y)) != 1 else 2)
            elif mutation == "palette":
                atlas.putpixel((x, y), min(art.base.FORBIDDEN - {0}))
            else:
                atlas.paste(0, (20, 0, 40, 20))
            files["control.tube"] = art.tube.indexed_png(atlas)
            with self.assertRaises(ValueError):
                art.validate(self.reports, files, self.record, self.public)

    def test_false_runtime_claim_and_missing_publication_refuse(self):
        """Reject source evidence relabelled as a toolbar result or omitted whole family.

        @param self Current isolated test case.
        @return None; actual runtime controls and packing require later owning work.
        """
        reports = copy.deepcopy(self.reports)
        reports["control.tube"]["runtime_activated"] = True
        with self.assertRaisesRegex(ValueError, "coverage or runtime"):
            art.validate(reports, self.files, self.record, self.public)
        reports = copy.deepcopy(self.reports)
        reports.pop("control.geothermal")
        with self.assertRaisesRegex(ValueError, "publication coverage"):
            art.validate(reports, self.files, self.record, self.public)


if __name__ == "__main__":
    unittest.main()
