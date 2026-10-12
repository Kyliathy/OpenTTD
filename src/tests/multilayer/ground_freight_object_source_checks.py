"""Check genuine whole-object wagon loads and preserve actual native cargo meaning.

@file ground_freight_object_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20D
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
These are source/geometry checks; moving/status/purchase and runtime remain pending.
"""

import copy
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_ground_freight_objects as objects


class GroundFreightObjectSourceChecks(unittest.TestCase):
    """Protect complete original object views, source forms and physical foreground.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md tasks20D.1-20D.4
    """

    @classmethod
    def setUpClass(cls):
        """Regenerate the complete bounded object slice once for this focused burst.

        @param cls Focused check class caching immutable source expectations.
        @return None.
        """
        fleet = objects.fleet
        cls.record = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
        cls.public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
        cls.originals = {objects.TUBE_RECORD.relative_to(ROOT).as_posix(): json.loads(objects.TUBE_RECORD.read_text(encoding="utf-8"))}
        cls.report, cls.files = objects.build(cls.record, cls.public, cls.originals)
        cls.atlas = Image.open(io.BytesIO(cls.files[ROOT / cls.report["path"]]))
        base_report = json.loads(cls.files[fleet.OUTPUT / "vehicle-model-05-base-source-manifest.json"])
        base_atlas = Image.open(io.BytesIO(cls.files[ROOT / base_report["path"]]))
        cls.bodies = {}
        # Each physical heading's original bytes remain the independent body
        # reference; a loaded atlas cannot redefine its own contact hardware.
        for row in base_report["sprites"]:
            x, y, width, height = row["box"]
            cls.bodies[row["direction"]] = base_atlas.crop((x, y, x + width, y + height))

    def pose(self, row):
        """Decode one actual selected load pose from the source atlas.

        @param self Current focused source check instance.
        @param row Explicit native state/view/variant rectangle in the staged report.
        @return Indexed64x48 image; no simulation state or inferred inventory.
        """
        x, y, width, height = row["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def resign(self, record, name):
        """Approve only isolated malformed metadata to exercise deeper refusal gates.

        @param self Current focused source check instance.
        @param record In-memory negative fixture; curated inputs remain immutable.
        @param name Registered component whose deliberately changed crop is tested.
        @return None; update the isolated signature without writing any source file.
        """
        family = record["families"]["vehicle.model-05"]
        family["approved_oriented_components"][name] = objects.component_signature(family["oriented_components"][name]["registration"], family)

    def test_exact_artifacts_and_complete_frame_zero_refit_coverage(self):
        """Cover all17 real refits while exposing every absent gear/status source.

        @param self Current focused source check instance.
        @return None.
        """
        self.assertEqual(self.report["oriented_frame0_pose_count"], 144)
        self.assertEqual(self.report["bulk_frame0_pose_count"], 128)
        self.assertEqual(self.report["empty_frame0_alias_count"], 136)
        self.assertEqual(self.report["staged_body_matrix_pose_count"], 408)
        self.assertEqual(self.report["required_body_matrix_pose_count"], 1360)
        self.assertEqual(self.report["missing_pose_count"], 952)
        self.assertFalse(self.report["source_coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual({row["cargo_label"] for row in self.report["sprites"]}, {"RARE", "RFMT", "STEL", "SCMP", "RBPT", "ELEC", "BATY", "SMIK", "GOOD"})
        self.assertTrue(all(row["frame"] == 0 for row in self.report["sprites"]))
        # Every declared byte artifact must regenerate from exact retained
        # originals; metadata alone cannot replace an absent body/load contact.
        for path, encoded in self.files.items():
            self.assertEqual(path.read_bytes(), encoded, path.name)

    def test_foreground_contacts_and_background_extension_remain_exact(self):
        """Permit only real cargo inside the public raised volume behind native steel.

        @param self Current focused source check instance.
        @return None.
        """
        spec = next(row for row in self.public["assets"] if row["asset_id"] == "vehicle.model-05")
        allowed, rims = {}, {}
        # Independent public-geometry masks test actual decoded output, without
        # asking the composition routine which pixels it intended to repaint.
        for direction in objects.fleet.DIRECTIONS:
            mask = Image.new("L", (64, 48), 0)
            ImageDraw.Draw(mask).polygon([tuple(point) for point in spec["cargo_geometry"]["protected_load_envelopes"][direction]], fill=1)
            allowed[direction] = mask.tobytes()
            rim = Image.new("L", (64, 48), 0)
            draw = ImageDraw.Draw(rim)
            for line in spec["cargo_geometry"]["foreground_rim_lines"][direction]:
                # Original near steel must survive each load with its exact
                # own colors, rather than a cargo-colored replacement edge.
                draw.line([tuple(point) for point in line], fill=1, width=1)
            rims[direction] = rim.tobytes()
        for row in self.report["sprites"]:
            direction = row["direction"]
            before, after = self.bodies[direction].tobytes(), self.pose(row).tobytes()
            self.assertTrue(all(not original or final for original, final in zip(before, after)))
            self.assertEqual(bytes(value for index, value in enumerate(before) if not allowed[direction][index]),
                             bytes(value for index, value in enumerate(after) if not allowed[direction][index]))
            self.assertEqual(bytes(value for index, value in enumerate(before) if value and rims[direction][index]),
                             bytes(value for index, value in enumerate(after) if before[index] and rims[direction][index]))
            self.assertEqual(row["anchor"], [-32, -32])

    def test_empty_partial_full_are_distinct_for_every_actual_refit_and_heading(self):
        """Keep packet-backed fill categories readable despite shared outer packages.

        @param self Current focused source check instance.
        @return None.
        """
        selected = {(row["cargo_label"], row["direction"], row["state"]): row for row in self.report["sprites"]}
        # A hidden/duplicated partial or full object cannot satisfy native source
        # coverage simply because an alias carries the correct cargo label.
        for label in self.report["component_aliases"]:
            for direction in objects.fleet.DIRECTIONS:
                body = self.bodies[direction].tobytes()
                partial = self.pose(selected[label, direction, "partial"]).tobytes()
                full = self.pose(selected[label, direction, "full"]).tobytes()
                self.assertEqual(len({body, partial, full}), 3, (label, direction))

    def test_declared_exterior_aliases_preserve_separate_native_variants(self):
        """Allow common protected containers without aliasing actual cargo identities.

        @param self Current focused source check instance.
        @return None.
        """
        selected = {(row["cargo_label"], row["direction"], row["state"]): row for row in self.report["sprites"]}
        packages = ("RARE", "RBPT", "ELEC", "BATY", "SMIK", "GOOD")
        self.assertEqual(len({row["tile_variant"] for row in self.report["sprites"]}), 9)
        # Identical sealed exteriors hide different actual contents. Only pixels
        # are shared; every source row still names its own immutable cargo slot.
        for direction in objects.fleet.DIRECTIONS:
            for load in ("partial", "full"):
                reference = self.pose(selected["RARE", direction, load]).tobytes()
                self.assertTrue(all(self.pose(selected[label, direction, load]).tobytes() == reference for label in packages))
                self.assertEqual(self.pose(selected["RFMT", direction, load]).tobytes(), self.pose(selected["STEL", direction, load]).tobytes())
        # Broad native end/side views must retain distinguishable container,
        # cast-bar and prefabricated-module forms after palette/downsampling.
        for direction in ("N", "E"):
            self.assertEqual(len({self.pose(selected[label, direction, "full"]).tobytes() for label in ("RARE", "RFMT", "SCMP")}), 3)

    def test_native_palette_and_transparency_have_no_implicit_effects(self):
        """Keep DOS source alpha and forbidden palette semantics exact.

        @param self Current focused source check instance.
        @return None.
        """
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertEqual(self.atlas.getpalette(), objects.fleet.base.PALETTE_BYTES)
        self.assertFalse(set(self.atlas.get_flattened_data()) & (objects.fleet.base.FORBIDDEN - {0}))

    def test_actual_bank_order_and_rear_partial_occupancy(self):
        """Keep whole payload depth and rear-bank placement correct in every view.

        @param self Current focused source check instance.
        @return None.
        """
        geometry = next(row for row in self.public["assets"] if row["asset_id"] == "vehicle.model-05")["cargo_geometry"]
        selected = {(row["cargo_label"], row["direction"], row["state"]): row for row in self.report["sprites"]}
        rear = {"N": 1, "NE": 1, "E": 0, "SE": 0, "S": 0, "SW": 0, "W": 1, "NW": 1}
        order_sensitive = 0
        # Test real source objects, not dummy colors: native overlap may hide a
        # far case behind a near case, while the actual wagon's rim remains above
        # both. Reversing their order must alter at least one visible source view.
        for label, component in (("RARE", "protected-package"), ("RFMT", "metal-ingot"), ("SCMP", "station-module")):
            for direction in objects.fleet.DIRECTIONS:
                canvases, _evidence = objects.native_objects(component, direction, self.record, self.originals)
                far, near = (image.tobytes() for image in canvases)
                body = self.bodies[direction].tobytes()
                allowed = Image.new("L", (64, 48), 0)
                ImageDraw.Draw(allowed).polygon([tuple(point) for point in geometry["protected_load_envelopes"][direction]], fill=1)
                rim = Image.new("L", (64, 48), 0)
                draw = ImageDraw.Draw(rim)
                for line in geometry["foreground_rim_lines"][direction]:
                    # Near steel is original physical foreground; no package
                    # depth order may repaint it into a cargo-colored edge.
                    draw.line([tuple(point) for point in line], fill=1, width=1)
                bounds, steel = allowed.tobytes(), rim.tobytes()
                # Opaque original steel wins outside the aperture or at its rim;
                # inside, the near original covers the far one where they overlap.
                foreground = [bool(value and (not bounds[index] or steel[index])) for index, value in enumerate(body)]
                expected = bytes(body[index] if foreground[index] else near[index] or far[index] or body[index] for index in range(64 * 48))
                reversed_depth = bytes(body[index] if foreground[index] else far[index] or near[index] or body[index] for index in range(64 * 48))
                partial_object = canvases[rear[direction]].tobytes()
                partial = bytes(body[index] if foreground[index] else partial_object[index] or body[index] for index in range(64 * 48))
                self.assertEqual(self.pose(selected[label, direction, "full"]).tobytes(), expected)
                self.assertEqual(self.pose(selected[label, direction, "partial"]).tobytes(), partial)
                order_sensitive += expected != reversed_depth
        self.assertGreater(order_sensitive, 0)

    def test_native_object_projection_cannot_shift_after_approval(self):
        """Preserve the complete object's scale and bottom-aligned physical bank.

        @param self Current focused source check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        view = record["families"]["vehicle.model-05"]["oriented_components"]["protected-package"]["registration"]["views"]["E"]
        view["native_positions"][0][0] += 1
        self.resign(record, "protected-package")
        with self.assertRaisesRegex(ValueError, "native projection moved"):
            objects.native_objects("protected-package", "E", record, self.originals)

    def test_new_background_pixels_cannot_be_hidden_by_an_erasing_mask(self):
        """Reject a misplaced whole object beyond the approved physical load volume.

        @param self Current focused source check instance.
        @return None.
        """
        family = self.record["families"]["vehicle.model-05"]
        canvases, _evidence = objects.native_objects("protected-package", "N", self.record, self.originals)
        altered = canvases[1].copy()
        altered.putpixel((0, 0), 15)
        with self.assertRaisesRegex(ValueError, "into background"):
            objects.compose_objects(self.bodies["N"], [canvases[0], altered], "N", "partial", family)

    def test_wrong_heading_or_neighbouring_object_crop_is_refused(self):
        """Require the real parent's selected view before any native object projection.

        @param self Current focused source check instance.
        @return None.
        """
        # Independently exercise an incorrect heading and a crop that contains
        # two payloads, after bypassing only the isolated fixture's signature gate.
        for corruption in ("heading", "overlap"):
            record = copy.deepcopy(self.record)
            views = record["families"]["vehicle.model-05"]["oriented_components"]["protected-package"]["registration"]["views"]
            views["N"]["source_window"] = views["NE"]["source_window"] if corruption == "heading" else [64, 112, 704, 432]
            self.resign(record, "protected-package")
            with self.assertRaises(ValueError):
                objects.native_objects("protected-package", "N", record, self.originals)

    def test_missing_real_view_never_inherits_a_rotated_substitute(self):
        """Keep absent object geometry explicit even when its shape is symmetric.

        @param self Current focused source check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        del record["families"]["vehicle.model-05"]["oriented_components"]["metal-ingot"]["registration"]["views"]["NW"]
        self.resign(record, "metal-ingot")
        with self.assertRaisesRegex(ValueError, "genuine source heading"):
            objects.component_spec("metal-ingot", record)

    def test_foreign_private_source_record_is_refused_before_lookup(self):
        """Keep cache paths as provenance, never an alternate asset-source authority.

        @param self Current focused source check instance.
        @return None.
        """
        view = copy.deepcopy(self.record["families"]["vehicle.model-05"]["oriented_components"]["metal-ingot"]["registration"]["views"]["N"])
        view["source_record"] = "private-cache/originals.json"
        with self.assertRaisesRegex(ValueError, "foreign"):
            objects.original_candidate(view, self.record, self.originals)

    def test_ingots_cannot_inherit_plate_or_grain_source_identity(self):
        """Keep real physical forms despite plausible source labels or filenames.

        @param self Current focused source check instance.
        @return None.
        """
        record = copy.deepcopy(self.record)
        registration = record["families"]["vehicle.model-05"]["oriented_components"]["metal-ingot"]["registration"]
        registration["component_kind"] = "isotropic_granules"
        self.resign(record, "metal-ingot")
        with self.assertRaisesRegex(ValueError, "nonoriented"):
            objects.component_spec("metal-ingot", record)
        record = copy.deepcopy(self.record)
        view = record["families"]["vehicle.model-05"]["oriented_components"]["metal-ingot"]["registration"]["views"]["N"]
        module = self.record["families"]["vehicle.model-05"]["oriented_components"]["station-module"]["registration"]["views"]["N"]
        view.update({key: module[key] for key in ("source_record", "source_key", "source_path", "source_sha256", "source_window")})
        self.resign(record, "metal-ingot")
        with self.assertRaisesRegex(ValueError, "plate or container"):
            objects.native_objects("metal-ingot", "N", record, self.originals)

    def test_source_traits_cannot_turn_covered_concentrate_into_bulk(self):
        """Read native physical traits rather than trusting a local display-table edit.

        @param self Current focused source check instance.
        @return None.
        """
        forms = objects.native_cargo_forms()
        self.assertEqual(forms["RARE"], "covered_granules")
        self.assertEqual(forms["RFMT"], "ingots")
        self.assertEqual(forms["STEL"], "ingots")
        record = copy.deepcopy(self.record)
        next(row for row in record["families"]["vehicle.model-05"]["cargo_source_matrix"] if row["cargo_label"] == "RARE")["form"] = "dry_bulk"
        with self.assertRaisesRegex(ValueError, "native content owner"):
            objects.build(record, self.public, self.originals)


if __name__ == "__main__":
    unittest.main()
