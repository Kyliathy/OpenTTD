"""Focused selected-ground packing boundaries, immutable prior pixels and explicit masks.

@file selected_packing_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups79-80.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84B; explicit delivered callback review remains separate from public activation.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Actual GRF decoding/loading and generator-free packaging have separate producers.
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
import pack_m1_art as art

RUN = ROOT / "logs/graphics/selected-pack/20261016-pack-01"


class SelectedPackingChecks(unittest.TestCase):
    """Verify native source ownership before any new payload can claim acceptance.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md Groups79-80.
    """

    @classmethod
    def setUpClass(cls):
        """Derive the complete selected source set once for independent corruption probes.

        @param cls Active selected-packing fixture holding actual immutable source documents.
        @return None.
        """
        cls.public = json.loads(art.PUBLIC.read_text(encoding="utf-8"))
        cls.scope = json.loads(art.SCOPE.read_text(encoding="utf-8"))
        cls.before = json.loads((RUN / "before.json").read_text(encoding="utf-8"))
        cls.report, cls.files = art.derive()
        cls.families = {row["asset_id"]: row for row in cls.public["assets"]}
        cls.image = Image.open(io.BytesIO(cls.files[ROOT / cls.scope["png"]]))

    def source(self, asset):
        """Load one actual authoritative source report without changing public state.

        @param self Active selected-packing fixture.
        @param asset Existing selected family identifier.
        @return Family, report path and isolated mutable report copy.
        """
        family = self.families[asset]
        path = art.source_binding(family)["atlas_manifest"]
        return family, path, json.loads((ROOT / path).read_text(encoding="utf-8"))

    def test_complete_selected_set_preserves_existing_ids_and_external_span(self):
        """Ninety complete families append beyond every already published native identity.

        @param self Active selected-packing fixture.
        @return None.
        """
        self.assertEqual(len(self.report["families"]), 90)
        self.assertEqual(self.report["pose_count"], 17749)
        self.assertEqual(self.report["native_sprite_bound"], 17847)
        self.assertEqual(self.report["preserved_external_sprite_count"], 11)
        # Existing enum order and all native extents are stable, including the
        # collector previously appended after the original forty-two families.
        for previous, current in zip(self.before["packed"]["families"], self.report["families"]):
            for field in ("asset_id", "native_offset", "sprite_count"):
                self.assertEqual(current[field], previous[field])
        self.assertEqual(self.report["families"][43]["native_offset"], 3599)
        self.assertFalse(any(3588 <= row["native_offset"] < 3599 for row in self.report["sprites"]))

    def test_prior_selected_pixels_anchors_and_protected_sources_remain_exact(self):
        """A repacked atlas may move source rectangles but cannot change any shipped sprite.

        @param self Active selected-packing fixture.
        @return None.
        """
        current = {row["native_offset"]: row for row in self.report["sprites"]}
        with Image.open(RUN / "previous-native-sheet.png") as before:
            # Compare decoded indexed pixels and native anchors independently
            # of sheet coordinates, which legitimately change when rows grow.
            for old in self.before["packed"]["sprites"]:
                new = current[old["native_offset"]]
                self.assertEqual(old["anchor"], new["anchor"])
                ox, oy, ow, oh = old["box"]
                nx, ny, nw, nh = new["box"]
                self.assertEqual((ow, oh), (nw, nh))
                self.assertEqual(before.crop((ox, oy, ox + ow, oy + oh)).tobytes(),
                                 self.image.crop((nx, ny, nx + nw, ny + nh)).tobytes())
        for name, digest in self.before["fixed_sources"].items():
            self.assertEqual(art.base.digest(ROOT / name), digest, name)

    def test_external_collision_and_changed_published_range_refuse(self):
        """Neither independent Space art nor existing M1 owners can surrender their native IDs.

        @param self Active selected-packing fixture.
        @return None.
        """
        scope = copy.deepcopy(self.scope)
        scope["preserved_external_ranges"][0]["native_offset"] = 2436
        with self.assertRaisesRegex(ValueError, "overlapping selected native range"):
            art.selected_ranges(self.public, scope)
        public = copy.deepcopy(self.public)
        family = next(row for row in public["assets"] if row["asset_id"] == "facility.vent-collector")
        family["packed"]["native_offset"] += 1
        with self.assertRaisesRegex(ValueError, "published native range changed"):
            art.selected_ranges(public, self.scope)

    def test_missing_real_processor_slot_refuses(self):
        """A matching coverage flag cannot hide a missing actual construction/state source part.

        @param self Active selected-packing fixture.
        @return None.
        """
        family, path, document = self.source("facility.battery-works")
        document["source_parts"].pop()
        with self.assertRaisesRegex(ValueError, "incomplete source coverage"):
            art.source_poses(family, {path: document}, {})

    def test_bad_source_bounds_and_foreign_parent_anchor_refuse(self):
        """Native source cells and station parent anchors cannot drift during normalization.

        @param self Active selected-packing fixture.
        @return None.
        """
        family, path, document = self.source("facility.battery-works")
        document["source_parts"][0]["box"][0] = -1
        with self.assertRaisesRegex(ValueError, "source cell leaves atlas"):
            art.source_poses(family, {path: document}, {})
        family, path, document = self.source("facility.rail-station")
        document["sprites"][0]["native_packed_anchor"] = [0, 0]
        with self.assertRaisesRegex(ValueError, "native parent/child anchor changed"):
            art.source_poses(family, {path: document}, {})

    def test_company_mask_cannot_spread_to_material_or_cargo(self):
        """Explicit company shades are legal only at the exact authored fitting coordinates.

        @param self Active selected-packing fixture.
        @return None.
        """
        family = self.families["vehicle.model-05"]
        poses, image = art.source_poses(family, {}, {})
        pose = next(iter(poses.values()))
        x, y, width, height = pose["box"]
        cell = image.crop((x, y, x + width, y + height))
        cell.putpixel((0, 0), 200)
        with self.assertRaisesRegex(ValueError, "company palette leaves"):
            art.validate_company_mask(family, pose, cell)

    def test_animation_palette_remains_forbidden_even_on_company_models(self):
        """Supporting authored livery never enables palette-cycled animation material.

        @param self Active selected-packing fixture.
        @return None.
        """
        cell = Image.new("P", (2, 2), 0)
        cell.putpalette(art.base.PALETTE_BYTES)
        cell.info["transparency"] = 0
        cell.putpixel((0, 0), 216)
        with self.assertRaisesRegex(ValueError, "reserved source palette"):
            art.validate_source_image(cell, "vehicle.model-05", "explicit_mask")

    def test_empty_parent_requires_actual_nullable_geometry(self):
        """Transparent station roof slots are structural; physical processor parts cannot disappear.

        @param self Active selected-packing fixture.
        @return None.
        """
        family, path, document = self.source("facility.electronics-works")
        document["source_parts"][0]["structurally_absent"] = True
        with self.assertRaisesRegex(ValueError, "structural absence differs"):
            art.source_poses(family, {path: document}, {})

    def test_runtime_selection_cannot_promote_new_source_only_families(self):
        """New packed images cannot acquire a callback or gallery acceptance by prefix alone.

        @param self Active selected-packing fixture.
        @return None.
        """
        import activate_m1_art as activation
        delivered = activation.runtime_ranges(self.report)
        original = set(self.before["scope"]["asset_ids"])
        self.assertEqual({row["asset_id"] for row in delivered}, original | activation.SELECTED_RUNTIME_ADDITIONS)
        self.assertEqual(len(delivered), 69)
        self.assertEqual(sum(row["sprite_count"] for row in delivered), 10557)
        self.assertEqual(sum(row["sprite_count"] for row in delivered if row["asset_id"] in original), 3495)
        # Delivered tube/first-plant callbacks add an explicit bounded review
        # scope. Other ground models, processors and future controls still
        # cannot acquire authority from packed image availability.
        for asset in ("vehicle.model-05", "facility.battery-works", "control.launch"):
            with self.assertRaisesRegex(ValueError, "not in delivered runtime scope"):
                activation.binding_for(asset)
        # Even a forged selection cannot reuse the implemented train/widget
        # callback for a different model/provider. No evidence or source file
        # is edited: only the isolated metadata reader receives the corruption.
        from unittest.mock import Mock, patch
        candidate = copy.deepcopy(self.scope)
        candidate["runtime_asset_ids"].extend(["vehicle.model-05", "control.launch"])
        metadata = Mock()
        metadata.read_text.return_value = json.dumps(candidate)
        with patch.object(activation.pack, "SCOPE", metadata):
            with self.assertRaisesRegex(ValueError, "model-specific native consumer"):
                activation.binding_for("vehicle.model-05")
            with self.assertRaisesRegex(ValueError, "control-specific native consumer"):
                activation.binding_for("control.launch")
        broken = copy.deepcopy(self.report)
        broken["families"] = [row for row in broken["families"] if row["asset_id"] != "facility.vent-collector"]
        with self.assertRaisesRegex(ValueError, "missing from packed selection"):
            activation.runtime_ranges(broken)

    def test_published_payload_binds_every_selected_range_to_actual_loaded_bytes(self):
        """A source-only count cannot substitute for the bound reproducible cached-loader payload.

        @param self Active selected-packing fixture after actual payload binding.
        @return None.
        """
        published = json.loads((ROOT / self.scope["report"]).read_text(encoding="utf-8"))
        identity = published["packed_grf"]
        self.assertEqual(art.base.digest(ROOT / identity["path"]), identity["sha256"])
        self.assertEqual(published["families"], self.report["families"])
        self.assertEqual(identity["preserved_external_sprites"], 11)
        self.assertIn(art.loader_marker(published), (ROOT / identity["loader_log"]).read_text(encoding="utf-8"))
        # Every source family must name its exact emitted range and the same
        # actually decoded GRF. Runtime selection does not alter these bindings.
        for row in published["families"]:
            packed = self.families[row["asset_id"]]["packed"]
            self.assertEqual(packed["native_offset"], row["native_offset"])
            self.assertEqual(packed["sprite_count"], row["sprite_count"])
            self.assertEqual(packed["sha256"], identity["sha256"])
            self.assertEqual(packed["route"], self.scope["native_route"])


if __name__ == "__main__":
    unittest.main()
