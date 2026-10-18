"""Verify genuine Assembly construction stages, native paid-work displays and the retained passive coupling.

@file space_art_assembly_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13I
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group74.3
Source appearance never supplies native work, stock, utility or service admission.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
import convert_space_assembly_art as assembly
import convert_space_assembly_states as states
from tools import orbital_tile_art as cuts
from tools import orbital_site_art as site

REVIEW = ROOT / "logs/proofs/mlm/native-art/space/assembly-complete-v1/source-manifest.json"


class AssemblyStateTests(unittest.TestCase):
    """Protect original physical states without inventing a second producer or energy controller."""

    @classmethod
    def setUpClass(cls):
        """Read concrete stored native source rectangles and exact whole-stage images.

        @param cls Fixture sharing immutable review/source pixels.
        """
        cls.source = art.read_record()
        cls.report = json.loads(REVIEW.read_text(encoding="utf-8"))
        for artifact in cls.report["artifacts"]:
            assert art.identity(ROOT / artifact["path"]) == artifact
        atlas = Image.open(REVIEW.parent / "module-assembly-authored.png")
        cls.parts = {}
        # Every stored rectangle must have one real physical pose; dictionary
        # overwrite cannot hide duplicate source rows or missing matrix entries.
        for row in cls.report["sprites"]:
            x, y, width, height = row["box"]
            key = (row["state"], row["frame"], row["tile_variant"], row["part"])
            assert key not in cls.parts
            cls.parts[key] = (row, atlas.crop((x, y, x + width, y + height)))
        cls.whole = {state: Image.open(REVIEW.parent / (state + ".png")) for state in states.POSES}

    def setUp(self):
        """Detach metadata before each deliberate source/ownership corruption.

        @param self Fixture whose mutations cannot alter actual source files.
        """
        self.document = copy.deepcopy(self.source)
        self.family = self.document["families"][assembly.ASSET]

    def candidate(self, key):
        """Find the exact editable original targeted by a focused refusal case.

        @param self Current detached source fixture.
        @param key Explicit selected or shared original identity.
        @return Detached candidate metadata only.
        """
        return next(row for row in self.document["candidates"] if row["key"] == key)

    def test_all64_cells_retain_actual_world_geometry_and_complete_pose_coverage(self):
        """Every required real world-cell/part/state/frame exists once at its original anchor.

        @param self Actual stored full native source matrix.
        """
        expected = {(state, frame, tile, part) for state in states.STATES for frame in range(2)
                    for tile in range(4) for part in ("back", "front")}
        self.assertEqual(set(self.parts), expected)
        self.assertEqual(self.report["logical_pose_count"], 64)
        for key, (row, image) in self.parts.items():
            self.assertEqual(row["direction"], "none")
            self.assertEqual(row["world_cell"], [row["tile_variant"] % 2, row["tile_variant"] // 2])
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(image.size, (64, 64))
            self.assertEqual(row["native_sha256"], hashlib.sha256(image.tobytes()).hexdigest())
            self.assertEqual(row["structurally_absent"], not any(image.get_flattened_data()))

    def test_independent_stored_pixel_reconstruction_has_no_seams_or_duplicates(self):
        """Every physical stage/frame independently reconstructs its exact original whole model.

        @param self Complete stored source matrix and whole-stage artifacts.
        """
        # Derive projection from real world coordinates rather than trusting
        # stored native origins or calling the converter's reconstruction helper.
        for state in states.STATES:
            for frame in range(2):
                image = Image.new("P", assembly.CANVAS, 0)
                seen = set()
                for tile in range(4):
                    wx, wy = tile % 2, tile // 2
                    ox, oy = 65 + 32 * (wy - wx), 15 + 16 * (wx + wy)
                    for part in ("back", "front"):
                        row, source = self.parts[state, frame, tile, part]
                        for y in range(64):
                            for x in range(64):
                                value = source.getpixel((x, y))
                                if not value:
                                    continue
                                point = (ox + x, oy + y)
                                self.assertNotIn(point, seen)
                                seen.add(point)
                                image.putpixel(point, value)
                expected_pose = "working" if state == "complete" and frame == 1 else state
                self.assertEqual(image.tobytes(), self.whole[expected_pose].tobytes())

    def test_24_static_aliases_and_real_complete_work_frame_are_explicit(self):
        """Only original paid manufacture has a distinct second source frame.

        @param self Every actual state/frame/cell/part in the stored matrix.
        """
        aliases, canonical = 0, 0
        # Construction and refusal remain still. Complete frame1 is a real green
        # source pose whose later selector must read the original paid-work receipt.
        for (state, frame, tile, part), (row, image) in self.parts.items():
            if frame == 1 and state != 'complete':
                self.assertEqual(row['alias_of'], dict(state=state, frame=0, direction='none', tile_variant=tile, part=part))
                self.assertEqual(image.tobytes(), self.parts[state, 0, tile, part][1].tobytes())
                aliases += 1
            else:
                self.assertIsNone(row['alias_of'])
                canonical += 1
        self.assertEqual((aliases, canonical), (24, 40))
        self.assertEqual(self.report['explicit_static_frame_aliases'], 24)
        self.assertEqual(self.report['physical_pose_count'], 5)
        self.assertNotEqual(self.whole['working'].tobytes(), self.whole['complete'].tobytes())

    def test_accepted_complete_base_and_all_original_tile_cuts_remain_exact(self):
        """New stages cannot replace the accepted Assembly, its passive coupling or physical depth cuts.

        @param self Current full source and immutable74.1/74.2 artifacts.
        """
        previous = ROOT / "logs/proofs/mlm/native-art/space/assembly-tile-cuts-v1"
        old = json.loads((previous / "tile-source-review.json").read_text(encoding="utf-8"))
        atlas = Image.open(previous / "native-cuts.png")
        self.assertEqual(self.whole["complete"].tobytes(), Image.open(previous / "reconstructed.png").tobytes())
        for row in old["sprites"]:
            x, y, width, height = row["box"]
            image = atlas.crop((x, y, x + width, y + height))
            self.assertEqual(image.tobytes(), self.parts["complete", 0, row["tile_variant"], row["part"]][1].tobytes())

    def test_unbuilt_plan_and_missing_partial_front_panel_are_visibly_distinct(self):
        """Genuine wire/partial construction cannot alias the installed gantry and console.

        @param self Original planning/physical-construction source fixture.
        """
        counts = {state: sum(value != 0 for value in image.get_flattened_data()) for state, image in self.whole.items()}
        self.assertLess(counts["blueprint"], counts["complete"] * 0.8)
        # A planning outline must retain its actual interior tile junction at
        # native size; a visible outer rim alone conceals the2x2 build layout.
        junction = [(x, y) for y in range(77, 84) for x in range(93, 100)]
        self.assertGreaterEqual(sum(bool(self.whole["blueprint"].getpixel(point)) for point in junction), 12)
        # The front tile interior must not retain the rejected rear-to-front
        # planning axis, which visually invents additional native floor regions.
        self.assertLess(sum(bool(self.whole["blueprint"].getpixel((x, y)))
                            for y in range(90, 102) for x in range(95, 98)), 4)
        self.assertNotEqual(self.whole["partial"].tobytes(), self.whole["complete"].tobytes())
        # Inspect a real interior region of the front tile: a missing plate must
        # expose genuine transparency around its authored support cross-braces.
        visible = [(x, y) for y in range(90, 102) for x in range(84, 108)
                   if self.whole["complete"].getpixel((x, y))]
        self.assertGreater(sum(not self.whole["partial"].getpixel(point) for point in visible), 30)
        for state in ("blueprint", "partial"):
            for tile in range(4):
                self.assertFalse(self.parts[state, 0, tile, "back"][0]["structurally_absent"])
                self.assertTrue(self.parts[state, 0, tile, "front"][0]["structurally_absent"])

    def test_paid_work_and_blocked_change_only_three_existing_glass_pixels(self):
        """Actual status never changes paid gantry/tool/approach hardware or paints a product.

        @param self Whole original complete/work/blocked pixels and declared console bounds.
        """
        complete = self.whole['complete']
        x0, y0, x1, y1 = states.STATUS_BOUNDS
        # Inspect every actual canvas pixel, not a reported preservation label.
        # Both independently authored materials must retain the original silhouette.
        for pose in ('working', 'blocked'):
            changed, coloured = 0, 0
            image = self.whole[pose]
            for y in range(160):
                for x in range(192):
                    a, b = complete.getpixel((x, y)), image.getpixel((x, y))
                    self.assertEqual(a == 0, b == 0)
                    if a == b:
                        continue
                    self.assertTrue(x0 <= x < x1 and y0 <= y < y1)
                    self.assertIn(a, states.BLUE_GLASS)
                    changed += 1
                    red, green, blue = image.getpalette()[b * 3:b * 3 + 3]
                    coloured += (green > red * 1.1 and green > blue * 1.1 and green > 100) if pose == 'working' else (red > 160 and green > 70 and red > green * 1.1 and blue < 90)
            self.assertEqual(changed, 3)
            self.assertGreater(coloured, 0)
            self.assertEqual(self.report['sources'][pose]['changed_pixels'], 3)
            self.assertEqual(self.report['sources'][pose]['protected_canvas_pixels'], 30717)
        for tile in range(4):
            front = self.parts['complete', 0, tile, 'front'][1].tobytes()
            self.assertEqual(front, self.parts['complete', 1, tile, 'front'][1].tobytes())
            self.assertEqual(front, self.parts['blocked', 0, tile, 'front'][1].tobytes())

    def test_generated_green_reused_amber_and_rejected_trials_keep_honest_ancestry(self):
        """Genuine new glass is distinct from unchanged material reuse and rejected machinery trials.

        @param self Actual selected original/material provenance and retained failed sources.
        """
        work = self.candidate(self.family['work_source'])
        amber = self.candidate(self.family['status_source'])
        shared = self.candidate(states.SHARED_GLASS)
        self.assertEqual(work['generated_from_original_key'], shared['key'])
        self.assertNotIn('reused_original_key', work)
        self.assertNotEqual(work['sha256'], shared['sha256'])
        self.assertEqual(amber['reused_original_key'], shared['key'])
        self.assertEqual(amber['sha256'], shared['sha256'])
        self.assertEqual(amber['path'], shared['path'])
        selected = [c for c in self.document['candidates'] if c['asset_id'] == assembly.ASSET and c['status'].startswith('selected')]
        self.assertEqual(len(selected), 6)
        self.assertEqual(sum(bool(c.get('reused_original_key')) for c in selected), 2)
        # Failed geometry/model trials remain unavailable even though every raw
        # file and original prompt is deliberately retained for truthful provenance.
        for c in self.document['candidates']:
            if c['asset_id'] == assembly.ASSET and c['status'].startswith('rejected'):
                with self.assertRaisesRegex(ValueError, 'rejected Space source'):
                    art.source_candidate(c['key'], self.document)

    def test_fresh_source_conversion_matches_every_published_native_cell(self):
        """Full original ancestry and conversion independently reproduce all64stored parts.

        @param self Direct current producer/source-consumer compatibility fixture.
        """
        fresh, images, evidence = states.matrix(self.document)
        self.assertEqual(len(fresh), 64)
        for row, image in fresh:
            key = (row["state"], row["frame"], row["tile_variant"], row["part"])
            self.assertEqual(image.tobytes(), self.parts[key][1].tobytes())

    def test_unsigned_state_policy_change_refuses_before_original_image_access(self):
        """Changed phase control cannot inherit the accepted static source signature.

        @param self Unsigned physical-state policy corruption fixture.
        """
        self.family["state_policy"]["frame_policy"] = "Use GUI wall time"
        with patch.object(site, "original_image", side_effect=AssertionError("unexpected image access")):
            with self.assertRaisesRegex(ValueError, "approved Assembly state policy changed"):
                states.native_states(self.document)

    def test_new_signature_cannot_invent_work_clock_product_or_idle_refusal(self):
        """A signed art dictionary cannot replace the actual paid manufacture receipt.

        @param self Re-signed phase, accounting, product and idle-status corruptions.
        """
        # Each independent false policy must refuse before any source raster read;
        # a prior failed clock claim cannot conceal acceptance of free work or hulls.
        for field in ('work_selection_policy', 'blocked_selection_policy', 'work_policy', 'product_policy', 'frame_policy'):
            self.document = copy.deepcopy(self.source)
            self.family = self.document['families'][assembly.ASSET]
            self.family['state_policy'][field] = 'Blink on GUI wall time, idle is blocked, two posts create two free hulls'
            self.family['approved_state_policy_sha256'] = cuts.signature(self.family['state_policy'])
            with patch.object(site, 'original_image', side_effect=AssertionError('unexpected image access')):
                with self.assertRaisesRegex(ValueError, 'native construction or paid manufacturing owners'):
                    states.native_states(self.document)

    def test_signed_partial_original_cannot_move_the_native_floor(self):
        """A new body registration cannot relocate the saved fixed world footprint.

        @param self Re-signed construction-stage geometry drift fixture.
        """
        candidate = self.candidate(self.family["body_sources"]["partial"])
        candidate["native_transform"]["native_reference"][0] += 1
        self.family["approved_body_registration"]["partial"] = site.registration_signature(candidate)
        with self.assertRaisesRegex(ValueError, "whole-model native floor contract changed"):
            assembly.native_body("partial", self.document)

    def test_signed_green_or_amber_registration_cannot_leave_the_existing_console(self):
        """Newly signed material placement cannot create another warning/work fixture.

        @param self Independent work and blocked mounting-contact corruption fixtures.
        """
        # Reset per real material so both approvals independently preserve the
        # exact same existing console, without copying a source or consumer owner.
        for phase, key, approval in (('work', 'work_source', 'approved_work_registration'), ('blocked', 'status_source', 'approved_status_registration')):
            self.document = copy.deepcopy(self.source)
            self.family = self.document['families'][assembly.ASSET]
            candidate = self.candidate(self.family[key])
            candidate['native_transform']['native_contact'][0] += 1
            self.family[approval] = site.registration_signature(candidate)
            with self.assertRaisesRegex(ValueError, 'leaves its existing native console'):
                states.display_component(self.document, phase)

    def test_missing_original_stage_cannot_count_as_full_coverage_with_a_new_hash(self):
        """One unbuilt body cannot silently fill the missing physical-construction state.

        @param self Re-signed incomplete source-stage selection fixture.
        """
        del self.family["body_sources"]["partial"]
        self.family["state_policy"] = states.expected_policy(self.document)
        self.family["approved_state_policy_sha256"] = cuts.signature(self.family["state_policy"])
        with self.assertRaisesRegex(ValueError, "differs from native construction or paid manufacturing owners"):
            states.checked_policy(self.document)

    def test_replaced_shared_original_bytes_are_refused_without_changing_real_files(self):
        """A retained status review never authorizes replacement material pixels.

        @param self Mocked genuine shared-source identity failure fixture.
        """
        target = (ROOT / self.candidate(states.SHARED_GLASS)["path"]).resolve()
        original_identity = art.identity

        def changed_identity(path):
            """Simulate changed original bytes without touching the preserved source.

            @param path Exact requested source/receipt/reference file.
            @return Real identity except the intentional shared-original hash mismatch.
            """
            result = original_identity(path)
            if path.resolve() == target:
                result["sha256"] = "0" * 64
            return result

        with patch.object(art, "identity", side_effect=changed_identity):
            with self.assertRaisesRegex(ValueError, "immutable Space source ancestry changed"):
                states.display_component(self.document, "blocked")

    def test_full_source_keeps_native_single_factory_admission_and_runtime_explicit(self):
        """Original source coverage grants no sprite allocation, product, energy or inventory.

        @param self Actual source-only native factory capability and full matrix handoff.
        """
        self.assertTrue(self.report['coverage_complete'])
        self.assertFalse(self.report['packed'])
        self.assertFalse(self.report['runtime_activated'])
        self.assertEqual(self.family['native_service_slots'], 0)
        self.assertEqual(self.family['native_stock_cap_per_label'], 0)
        self.assertFalse(self.family['native_power_producer'])
        self.assertEqual(self.family['native_producer_count'], 0)
        self.assertEqual(self.family['native_approach_slots'], 1)
        self.assertEqual(self.family['native_manufacture_slots'], 1)
        self.assertTrue(self.family['native_work_consumer'])
        self.assertEqual(self.family['native_work_consumer_count'], 1)
        self.assertEqual(self.family['native_work_capacity_j'], 750)


    def test_work_material_requires_its_real_complete_frame1_role(self):
        """A genuine material cannot claim another frame or become unchanged Core reuse.

        @param self Detached frame and generated-material ancestry corruptions.
        """
        # Both defects target metadata outside the registration hash, so the
        # explicit native-role guard must still reject each independently.
        for field, value in (('source_frame', 0), ('reused_original_key', states.SHARED_GLASS)):
            self.document = copy.deepcopy(self.source)
            self.family = self.document['families'][assembly.ASSET]
            c = self.candidate(self.family['work_source'])
            c[field] = value
            self.family['approved_work_registration'] = site.registration_signature(c)
            with self.assertRaisesRegex(ValueError, 'genuine frame1 generated ancestry'):
                states.display_component(self.document, 'work')

    def test_whole_material_refuses_real_non_glass_inside_the_window_bounds(self):
        """A bounding box alone cannot authorize repainting the original console trim.

        @param self Existing real grey pixel inside the outer glass mounting envelope.
        """
        material = cuts.indexed_canvas(assembly.CANVAS)
        self.assertNotIn(self.whole['complete'].getpixel((100, 64)), states.BLUE_GLASS)
        material.putpixel((100, 64), 85)
        with self.assertRaisesRegex(ValueError, 'outside existing blue glass'):
            states.with_display(self.whole['complete'], material, 'work')

    def test_actual_phase_pixels_retain_static_dos_palette_without_reserved_colours(self):
        """Paid display art introduces neither company remapping nor palette animation.

        @param self All five whole native original poses.
        """
        for image in self.whole.values():
            self.assertEqual(image.mode, 'P')
            self.assertFalse(set(image.get_flattened_data()) & (art.base.FORBIDDEN - {0}))

    def test_changed_work_original_bytes_cannot_inherit_selected_material_approval(self):
        """New green source ancestry remains immutable independently of reused amber.

        @param self Mocked own-green original identity without modifying actual files.
        """
        target = (ROOT / self.candidate(self.family['work_source'])['path']).resolve()
        actual_identity = art.identity

        def changed_work_identity(path):
            """Simulate changed own original pixels while retaining every real file.

            @param path Requested exact source, reference or receipt path.
            @return Actual identity except the intended green source mismatch.
            """
            value = actual_identity(path)
            if path.resolve() == target:
                value['sha256'] = '0' * 64
            return value

        with patch.object(art, 'identity', side_effect=changed_work_identity):
            with self.assertRaisesRegex(ValueError, 'immutable Space source ancestry changed'):
                states.display_component(self.document, 'work')

    def test_material_approval_cannot_change_work_into_another_physical_stage(self):
        """A fresh signature cannot attach a paid-work material to unfinished machinery.

        @param self Re-signed green component-state corruption fixture.
        """
        c = self.candidate(self.family['work_source'])
        c['state'] = 'partial'
        self.family['approved_work_registration'] = site.registration_signature(c)
        with self.assertRaisesRegex(ValueError, 'original role or registration changed'):
            states.display_component(self.document, 'work')


if __name__ == "__main__":
    unittest.main()
