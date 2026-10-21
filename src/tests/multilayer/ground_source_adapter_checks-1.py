"""Refuse corrupt selected-ground source matrices, policies and frozen native ranges.

@file ground_source_adapter_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Detached corruption fixtures never modify immutable originals or published files.
"""

import copy
import unittest

from PIL import Image

from ground_pack_check_support import handoff, run_checks


class GroundSourceAdapterChecks(unittest.TestCase):
    """Exercise all four actual producer schemas and bounded source/range refusals."""

    @classmethod
    def setUpClass(cls):
        """Load the real qualified source scope without changing any repository input.

        @param cls Active immutable-source qualification suite.
        @return None.
        """
        cls.scope, cls.before, cls.public = handoff.read_scope()
        cls.families = {row["asset_id"]: row for row in cls.public["assets"]}

    def fixture(self, asset_id="mine.COAL"):
        """Create a detached actual producer report for one corruption check.

        @param self Active source-schema suite.
        @param asset_id Selected family whose real report is copied in memory.
        @return Family, independent scope, document cache, mutable pose rows.
        """
        scope = copy.deepcopy(self.scope)
        reference = next(row for row in scope["source_rows"] if row["asset_id"] == asset_id)
        document = handoff.read_json(reference["manifest"])
        kind = reference["adapter_kind"]
        if kind == "atlas_list_v1":
            atlas = next(row for row in document["atlases"] if row["path"] == reference["source_atlas"])
        else:
            atlas = document
        rows = atlas["source_parts" if kind == "physical_source_parts" else "sprites"]
        return copy.deepcopy(self.families[asset_id]), scope, {reference["manifest"]: document}, rows

    def assert_refused(self, family, scope, documents, message):
        """Require a specific refusal after the immutable report-identity boundary.

        @param self Active source-schema suite.
        @param family Detached current family.
        @param scope Detached adapter scope.
        @param documents In-memory producer-report corruption fixture.
        @param message Required refusal text identifying the failed boundary.
        @return None.
        """
        with self.assertRaisesRegex(ValueError, message):
            handoff.source_poses(family, scope, self.before, documents, {})

    def test_complete_real_sources_keep_all_schemas_and_exact_empty_company_policies(self):
        """All47 real families cover14,254 poses with228 approved absences and4,082 remap pixels.

        @param self Active all-source contract check.
        @return None.
        """
        documents, images, total, absent, company = {}, {}, 0, 0, 0
        # Traverse every declared producer, including all four schema adapters;
        # no representative-only sample can hide a missing selected family.
        for reference in self.scope["source_rows"]:
            poses, _image = handoff.source_poses(self.families[reference["asset_id"]], self.scope,
                                                self.before, documents, images)
            total += len(poses)
            absent += sum(row["structurally_absent"] for row in poses.values())
            company += sum(row["company_color_pixels"] for row in poses.values())
        self.assertEqual((total, absent, company), (14254, 228, 4082))
        self.assertEqual(len({row["adapter_kind"] for row in self.scope["source_rows"]}), 4)

    def test_missing_and_duplicate_poses_refuse(self):
        """An incomplete cell or duplicated identity cannot count as delivered source coverage.

        @param self Active pose-coverage refusal check.
        @return None.
        """
        family, scope, documents, rows = self.fixture()
        rows.pop()
        self.assert_refused(family, scope, documents, "complete source matrix")
        family, scope, documents, rows = self.fixture()
        rows.append(copy.deepcopy(rows[0]))
        self.assert_refused(family, scope, documents, "duplicate source pose")

    def test_bounds_anchor_and_native_canvas_refuse(self):
        """A source rectangle must fit its actual atlas and retain the compiled cell registration.

        @param self Active physical-registration refusal check.
        @return None.
        """
        # These are distinct registration failures: clipping an atlas edge,
        # moving the world contact and changing the approved cell dimensions.
        for field, value in [("box", [-1, 0, 64, 64]), ("anchor", [-30, -33]), ("box", [0, 0, 63, 64])]:
            family, scope, documents, rows = self.fixture()
            rows[0][field] = value
            self.assert_refused(family, scope, documents, "canvas/bounds/anchor")

    def test_foreign_pixels_report_or_adapter_refuse(self):
        """Hash drift and unsupported schemas cannot silently substitute foreign source art.

        @param self Active provenance/refusal check.
        @return None.
        """
        family, scope, documents, _rows = self.fixture()
        reference = next(row for row in scope["source_rows"] if row["asset_id"] == family["asset_id"])
        reference["manifest_sha256"] = "0" * 64
        self.assert_refused(family, scope, documents, "qualified source report changed")
        family, scope, documents, _rows = self.fixture()
        reference = next(row for row in scope["source_rows"] if row["asset_id"] == family["asset_id"])
        reference["adapter_kind"] = "guess_from_filename"
        self.assert_refused(family, scope, documents, "unsupported ground source adapter")
        family, scope, documents, rows = self.fixture()
        rows[0]["source_pixels_sha256"] = "0" * 64
        self.assert_refused(family, scope, documents, "source-part pixel identity")

    def test_unapproved_empty_or_false_structural_absence_refuses(self):
        """An absent roof needs its producer flag; an opaque cell cannot be relabeled as absent.

        @param self Active structural-policy refusal check.
        @return None.
        """
        family, scope, documents, rows = self.fixture("facility.rail-station")
        next(row for row in rows if row.get("structurally_absent"))["structurally_absent"] = False
        self.assert_refused(family, scope, documents, "missing art or contradicts structural absence")
        family, scope, documents, rows = self.fixture("facility.rail-station")
        next(row for row in rows if not row.get("structurally_absent"))["structurally_absent"] = True
        self.assert_refused(family, scope, documents, "missing art or contradicts structural absence")

    def test_missing_source_and_geometry_drift_refuse(self):
        """A stale public source or changed producer shape must stop packing before emitted IDs.

        @param self Active public-handoff refusal check.
        @return None.
        """
        family, scope, documents, _rows = self.fixture()
        family["source"]["status"] = "pending"
        self.assert_refused(family, scope, documents, "immutable source binding")
        family, scope, documents, _rows = self.fixture()
        family["anchor"][0] += 1
        self.assert_refused(family, scope, documents, "qualified native geometry")
        with self.assertRaisesRegex(ValueError, "removed or is not a file"):
            handoff.digest(handoff.bound("test-cache/ground-pack/required-absent-original.png"))

    def test_prefix_range_reassignment_refuses(self):
        """The complete current prefix includes Space/launch; its slots cannot be recycled.

        @param self Active complete-prefix preservation guard.
        @return None.
        """
        public = copy.deepcopy(self.public)
        name = self.before["native_prefix"][-1]["asset_id"]
        next(row for row in public["assets"] if row["asset_id"] == name)["packed"]["native_offset"] -= 1
        with self.assertRaisesRegex(ValueError, "published native prefix range"):
            handoff.read_scope(public)

    def test_company_remap_requires_explicit_policy_and_animation_is_forbidden(self):
        """Only deliberate vehicle remap indices198..205 may survive the frozen native palette.

        @param self Active color-policy corruption fixture.
        @return None.
        """
        image = Image.new("P", (2, 2), 198)
        image.putpalette(handoff.packing.base.PALETTE_BYTES)
        image.info["transparency"] = 0
        family = copy.deepcopy(self.families["vehicle.model-05"])
        handoff.validate_native_palette(image, family)
        family["company_colors"] = "none"
        with self.assertRaisesRegex(ValueError, "reserved source palette"):
            handoff.validate_native_palette(image, family)
        family["company_colors"] = "explicit_mask"
        image.putpixel((0, 0), next(iter(handoff.packing.base.FORBIDDEN - {0} - handoff.COMPANY_INDICES)))
        with self.assertRaisesRegex(ValueError, "forbidden animation/reserved"):
            handoff.validate_native_palette(image, family)
        image.putpixel((0, 0), 198)
        image.info["transparency"] = 1
        with self.assertRaisesRegex(ValueError, "palette/transparency/animation"):
            handoff.validate_native_palette(image, family)


if __name__ == "__main__":
    run_checks(GroundSourceAdapterChecks)
