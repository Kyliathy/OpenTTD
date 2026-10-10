"""Focused capsule load custody, original component and native source checks.

@file tube_capsule_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups49-53.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Malformed records/atlases remain in memory; originals and native gameplay are untouched.
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
import convert_tube_capsule_states as capsules


class TubeCapsuleSourceChecks(unittest.TestCase):
    """Require actual cargo forms through all native service/power conditions and genuine source phases."""

    ASSET_ID = "vehicle.model-09"
    BASE_KEY = "mole-base-e-v1"
    EXCLUDED_KEY = "mole-views-v1"

    @classmethod
    def setUpClass(cls):
        """Build one valid immutable baseline for independent source/refusal cases.

        @param cls Current class retaining canonical model/component source bytes.
        @return None; no repository source/public contract is modified.
        """
        cls.baseline, cls.baseline_files = capsules.build(asset_id=cls.ASSET_ID)

    def setUp(self):
        """Isolate each malformed provenance/report/atlas from the next case.

        @param self Current test instance.
        @return None; mutations remain in this test's own memory.
        """
        self.document = json.loads(capsules.tube.RECORD.read_text(encoding="utf-8"))
        self.family = next(row for row in self.document["families"] if row["asset_id"] == self.ASSET_ID)
        self.report = copy.deepcopy(self.baseline)
        self.files = self.baseline_files.copy()
        self.body = self.report["atlases"][0]

    def pose(self, load, mode, direction, frame=0):
        """Read one actual native pose without trusting global atlas ordering.

        @param self Current test instance holding isolated source bytes.
        @param load Actual empty/partial/full visible cargo bucket.
        @param mode Operating/service/blocked condition independent of cargo.
        @param direction Actual engine heading.
        @param frame Genuine original lamp phase,0 or1.
        @return Indexed64x48 source crop and its exact provenance row.
        """
        row = next(row for row in self.body["sprites"] if (row["load"], row["mode"], row["direction"], row["frame"]) == (load, mode, direction, frame))
        x, y, width, height = row["box"]
        with Image.open(io.BytesIO(self.files[capsules.tube.base.repository_path(self.body["path"])])) as atlas:
            return atlas.crop((x, y, x + width, y + height)), row

    def test_complete_exact_source_regeneration(self):
        """Require144 real body/two purchase poses and deterministic full proof.

        @param self Current test instance.
        @return None; complete original models/components/contacts regenerate read-only.
        """
        self.assertEqual(self.report["pose_count"], 146)
        capsules.validate(self.report, self.files, self.document, self.ASSET_ID)
        capsules.publish(check=True, asset_id=self.ASSET_ID)

    def test_service_and_blocking_retain_actual_load(self):
        """Compare actual pixels: each condition changes glass while retaining load.

        @param self Current test instance.
        @return None; all three cargo forms remain distinct in every actual facing.
        """
        # A service/power wait never empties a physical hold. Remove only the
        # genuine glass coordinate, then compare complete cargo/hull pixels.
        for direction in capsules.DIRECTIONS:
            forms = []
            for load in ("empty", "partial", "full"):
                image, row = self.pose(load, "operating", direction)
                cue = tuple(row["indicator"]["target"])
                image.putpixel(cue, 0)
                forms.append(image.tobytes())
                for mode in ("service", "blocked"):
                    other, unused = self.pose(load, mode, direction)
                    other.putpixel(cue, 0)
                    self.assertEqual(image.tobytes(), other.tobytes(), (direction, load, mode))
            self.assertEqual(len(set(forms)), 3, direction)

    def test_phases_change_only_genuine_glass(self):
        """Verify two real distinct phases without moving hull/cargo/company material.

        @param self Current test instance.
        @return None; repeated static poses cannot count as animation.
        """
        # All actual combinations share fixed chassis/load geometry. Only the
        # retained original one-pixel glass is permitted to differ by phase.
        for direction in capsules.DIRECTIONS:
            for load in ("empty", "partial", "full"):
                for mode in ("operating", "service", "blocked"):
                    first, row = self.pose(load, mode, direction, 0)
                    second, unused = self.pose(load, mode, direction, 1)
                    changed = {(x, y) for y in range(48) for x in range(64) if first.getpixel((x, y)) != second.getpixel((x, y))}
                    self.assertEqual(changed, {tuple(row["indicator"]["target"])})

    def test_original_hull_and_alpha_stay_exact(self):
        """Require original body pixels outside genuine cargo/glass/company fittings.

        @param self Current test instance.
        @return None; cargo never expands alpha or repaints original structural fittings.
        """
        # Compare against independently decoded original hulls, preserving
        # opaque shape even when genuine cargo material fills either recess.
        for direction in capsules.DIRECTIONS:
            selected = next(row for row in self.family["selections"] if row["direction"] == direction)
            source = capsules.tube.retained_candidate(selected["source"], self.document)
            baseline, unused = capsules.models.native_pose(source, direction)
            holds = {tuple(p) for mask in self.family["components"]["hold_masks"][direction] for p in mask}
            for load in ("empty", "partial", "full"):
                image, row = self.pose(load, "operating", direction)
                allowed = holds | {tuple(row["indicator"]["target"])}
                if row["company_plaque"] is not None:
                    allowed.add(tuple(row["company_plaque"]["target"]))
                self.assertEqual(bytes(v != 0 for v in image.get_flattened_data()), bytes(v != 0 for v in baseline.get_flattened_data()))
                changed = {(x, y) for y in range(48) for x in range(64) if image.getpixel((x, y)) != baseline.getpixel((x, y))}
                self.assertTrue(changed <= allowed, (direction, load))

    def test_purchase_lock_retains_own_model(self):
        """Compare actual UI pixels and retain original model identity under its badge.

        @param self Current test instance.
        @return None; the calendar lock stays outside world artwork.
        """
        purchase = self.report["atlases"][1]
        with Image.open(io.BytesIO(self.files[capsules.tube.base.repository_path(purchase["path"])])) as atlas:
            available = atlas.crop((0, 0, 20, 20))
            locked = atlas.crop((20, 0, 40, 20))
            self.assertNotEqual(available.tobytes(), locked.tobytes())
            # Every original opaque preview pixel remains unchanged. Only
            # genuine badge alpha fills formerly transparent UI space below it.
            for y in range(20):
                for x in range(20):
                    if available.getpixel((x, y)) != 0:
                        self.assertEqual(available.getpixel((x, y)), locked.getpixel((x, y)))
        self.assertEqual(purchase["sprites"][1]["lock_component"]["family"], "control.future-lock")

    def test_missing_loaded_blocked_pose_refuses(self):
        """Reject a missing full-load blocked south-facing source pose.

        @param self Current test instance.
        @return None; a repeated empty/operating pose cannot replace it.
        """
        self.body["sprites"][:] = [row for row in self.body["sprites"] if (row["state"], row["direction"], row["frame"]) != ("blocked-full", "S", 1)]
        with self.assertRaisesRegex(ValueError, "coverage"):
            capsules.validate(self.report, self.files, self.document, self.ASSET_ID)

    def test_false_cargo_mode_contract_refuses(self):
        """Reject service/full deliberately relabelled as an empty physical hold.

        @param self Current test instance.
        @return None; actual native cargo remains an independent state owner.
        """
        self.family["state_map"]["service-full"]["load"] = "empty"
        with self.assertRaisesRegex(ValueError, "cargo/mode"):
            capsules.build(self.document, self.ASSET_ID)

    def test_known_excluded_heading_refuses(self):
        """Keep the original repeated north-facing S cell excluded.

        @param self Current test instance.
        @return None; a correct heading label cannot rehabilitate the wrong face.
        """
        next(row for row in self.family["selections"] if row["direction"] == "S")["source"] = self.EXCLUDED_KEY
        with self.assertRaisesRegex(ValueError, "excluded|unselected"):
            capsules.build(self.document, self.ASSET_ID)

    def test_changed_original_hash_refuses(self):
        """Reject changed source provenance without modifying the actual original.

        @param self Current test instance.
        @return None; stable filenames alone cannot identify authored pixels.
        """
        next(row for row in self.document["candidates"] if row["key"] == self.BASE_KEY)["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "immutable"):
            capsules.build(self.document, self.ASSET_ID)

    def test_unapproved_cargo_target_refuses(self):
        """Reject a new cargo mask placed over original structural material.

        @param self Current test instance.
        @return None; component registration needs the frozen public source approval.
        """
        self.family["components"]["hold_masks"]["E"][0].append([32, 31])
        with self.assertRaisesRegex(ValueError, "unapproved"):
            capsules.build(self.document, self.ASSET_ID)

    def test_missing_genuine_lamp_phase_refuses(self):
        """Reject two copies of one authored lamp phase before composition.

        @param self Current test instance.
        @return None; no procedural colour or fake animation supplies missing art.
        """
        self.family["components"]["indicator_frames"] = [0, 0]
        with self.assertRaisesRegex(ValueError, "lamp contract"):
            capsules.status_lamps(self.family["components"])

    def test_company_remap_cannot_tint_cargo(self):
        """Reject a primary company palette index painted outside its original fitting.

        @param self Current test instance.
        @return None; original corporate fittings are the only remapped pixels.
        """
        path = capsules.tube.base.repository_path(self.body["path"])
        with Image.open(io.BytesIO(self.files[path])) as atlas:
            atlas.putpixel((2 * 64 + 24, 31), 198)
            self.files[path] = capsules.tube.indexed_png(atlas)
        with self.assertRaisesRegex(ValueError, "company mask"):
            capsules.validate(self.report, self.files, self.document, self.ASSET_ID)

    def test_painted_original_hull_refuses(self):
        """Reject arbitrary original-structure repainting with otherwise complete metadata.

        @param self Current test instance.
        @return None; deterministic source conversion is the sole pixel authority.
        """
        path = capsules.tube.base.repository_path(self.body["path"])
        with Image.open(io.BytesIO(self.files[path])) as atlas:
            atlas.putpixel((2 * 64 + 32, 31), 15)
            self.files[path] = capsules.tube.indexed_png(atlas)
        with self.assertRaisesRegex(ValueError, "changes original"):
            capsules.validate(self.report, self.files, self.document, self.ASSET_ID)

    def test_false_component_ancestry_refuses(self):
        """Reject cargo pixels relabelled as a different immutable original.

        @param self Current test instance.
        @return None; a complete atlas cannot conceal false source ownership.
        """
        next(row for row in self.body["sprites"] if row["cargo"] is not None)["cargo"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            capsules.validate(self.report, self.files, self.document, self.ASSET_ID)

    def test_false_runtime_activation_refuses(self):
        """Keep source acceptance separate from packed operating capsule behavior.

        @param self Current test instance.
        @return None; artwork never activates purchase, dates, cargo movement or energy.
        """
        self.report["runtime_activated"] = True
        with self.assertRaisesRegex(ValueError, "source/runtime"):
            capsules.validate(self.report, self.files, self.document, self.ASSET_ID)

    def test_native_registration_drift_refuses(self):
        """Reject a shifted hull whose unchanged original image still hashes correctly.

        @param self Current test instance.
        @return None; accepted chassis contacts cannot drift during state assembly.
        """
        next(row for row in self.document["candidates"] if row["key"] == self.BASE_KEY)["native_translation"][0] = 1
        with self.assertRaisesRegex(ValueError, "model registration"):
            capsules.build(self.document, self.ASSET_ID)


if __name__ == "__main__":
    unittest.main()
