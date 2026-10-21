"""Qualify and publish the complete genuine cargo-rocket source handoff.

@file cargo_rocket_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62G.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Default rebuilds only this source group; --publish updates its owned metadata
after seven focused checks. --verify-published decodes the existing publication.
All generated reports/images remain under logs/proofs/mlm/native-art/space/.
"""

import argparse
import copy
import hashlib
from pathlib import Path
import sys
import time
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT/"media/multilayer"))
import convert_cargo_rocket_source as source
from tools import cargo_rocket_art as art
from tools import cargo_rocket_states as states


class CargoRocketSourceChecks(unittest.TestCase):
    """Check actual complete saved artwork, direct aliases, ownership and refusal boundaries."""

    require_published = False

    @classmethod
    def setUpClass(cls):
        """Load and independently decode current source once for this focused burst.

        @param cls Active source qualification suite with immutable shared inputs.
        @return None.
        """
        cls.report = art.read_json(source.MANIFEST)
        cls.policy, cls.document, cls.ready, cls.burn = source.checked_art()
        cls.decoded = source.decode(cls.report)
        cls.pixels = {source.key(row): pixels for row, pixels in cls.decoded}
        cls.phase_policy = art.read_json(states.POLICY)

    def test_actual112_saved_poses_keep_exact_bodies_palette_geometry_and_engine_ownership(self):
        """Every logical pose resolves real immutable indexed pixels at its native contact.

        @param self Active all-heading/state/frame and original-pixel ownership check.
        @return None.
        """
        self.assertEqual(len(self.decoded), 112)
        self.assertEqual(len(self.report["canonical_parts"]), 24)
        # Whole original hull pixels must survive every state/frame unchanged;
        # only their actual transparent background can reveal authored exhaust.
        for row, pixels in self.decoded:
            heading, state, frame = row["direction"], row["state"], row["frame"]
            body = self.pixels[source.key(source.pose(heading, "ready", 0))]
            self.assertEqual((pixels.mode, pixels.size), ("P", (64, 48)))
            self.assertEqual(pixels.getpalette(), art.source.base.PALETTE_BYTES)
            self.assertFalse(set(pixels.get_flattened_data()) & (set(art.source.base.FORBIDDEN)-{0}))
            self.assertEqual(row["anchor"], [-32, -32])
            self.assertEqual(row["native_contact"], [32, 32])
            pairs = list(zip(body.get_flattened_data(), pixels.get_flattened_data()))
            self.assertTrue(all(original == actual for original, actual in pairs if original))
            if state in self.policy["static_alias_states"]:
                self.assertEqual(pixels.tobytes(), body.tobytes())
            else:
                expected = self.burn["poses"][heading+"/"+str(frame)]
                self.assertEqual(hashlib.sha256(pixels.tobytes()).hexdigest(), expected["whole_pixels_sha256"])
                self.assertEqual(sum(original == 0 and actual != 0 for original, actual in pairs), expected["visible_effect_pixels"])
                self.assertEqual(list(pixels.getbbox()), expected["whole_bounds"])
        self.assertNotEqual(self.pixels[source.key(source.pose("NE", "ascent", 0))].tobytes(),
                            self.pixels[source.key(source.pose("NE", "ascent", 1))].tobytes())

    def test_actual_native_phase_receipts_resolve_complete_source_without_changing_owners(self):
        """Native flight funding/countdown owns animation; static states remain explicit aliases.

        @param self Active native-policy to delivered-source boundary check.
        @return None.
        """
        scenarios = [(None, {}, "ready"), ("GroundLoading", {}, "ready"), ("Preparing", {}, "preparing"),
                     ("OrbitalDelivery", {"blocked": "Approach"}, "orbit"), ("Turnaround", {}, "service"),
                     ("Holding", {}, "blocked"), ("Ascent", {}, "blocked"), ("ReturnRecovery", {}, "blocked")]
        # All source states are selected from existing native fields. Rendering
        # neither creates a burn receipt nor advances a preparation/flight tick.
        for phase, fields, expected_state in scenarios:
            before = copy.deepcopy(fields)
            selected = states.source_pose(phase, policy=self.phase_policy, **fields)
            self.assertEqual(selected["state"], expected_state)
            self.assertEqual(fields, before)
            for heading in art.DIRECTIONS:
                self.assertIn(source.key(source.pose(heading, selected["state"], selected["frame"])), self.pixels)
        for phase, state, recovered in (("Ascent", "ascent", False), ("ReturnRecovery", "recovery", True)):
            # Countdown selects an existing authored pulse; repeated source
            # queries cannot fund a flight leg or change the physical hull.
            for ticks, frame in ((222, 0), (214, 1), (206, 0), (198, 1)):
                selected = states.source_pose(phase, blocked="HomeLost", outbound_burned=True,
                                              recovery_burned=recovered, remaining_ticks=ticks, policy=self.phase_policy)
                self.assertEqual(selected, dict(state=state, frame=frame, effect="engine/frame"+str(frame)))
                for heading in art.DIRECTIONS:
                    active = self.pixels[source.key(source.pose(heading, state, frame))]
                    self.assertEqual(active.tobytes(), self.pixels[source.key(source.pose(heading, "ascent", frame))].tobytes())

    def test_missing_duplicate_and_malformed_actual_coverage_are_refused(self):
        """A complete-looking count cannot replace the exact native pose product or geometry.

        @param self Active missing/duplicate/type/geometry refusal check.
        @return None.
        """
        # Exercise each independent completeness boundary with one detached
        # change, leaving the actual published originals/report untouched.
        for mutation in ("missing", "duplicate", "canonical_missing", "canonical_duplicate", "boolean_frame",
                         "boolean_tile", "boolean_header", "canonical_contact", "canonical_mask", "canonical_frame",
                         "box", "contact", "anchor", "source", "coverage", "packed", "runtime"):
            report = copy.deepcopy(self.report)
            if mutation == "missing":
                report["sprites"].pop()
            elif mutation == "duplicate":
                report["sprites"][-1] = copy.deepcopy(report["sprites"][0])
            elif mutation == "canonical_missing":
                report["canonical_parts"].pop()
            elif mutation == "canonical_duplicate":
                report["canonical_parts"][-1] = copy.deepcopy(report["canonical_parts"][0])
            elif mutation == "boolean_frame":
                report["sprites"][0]["frame"] = False
            elif mutation == "boolean_tile":
                report["sprites"][0]["tile_variant"] = False
            elif mutation == "boolean_header":
                report["tile_variants"] = True
            elif mutation == "canonical_contact":
                report["canonical_parts"][0]["native_contact"][0] += 1
            elif mutation == "canonical_mask":
                report["canonical_parts"][0]["company_mask"] = [[32, 16]]
            elif mutation == "canonical_frame":
                report["canonical_parts"][0]["frame"] = False
            elif mutation == "box":
                report["sprites"][0]["box"][0] = False
            elif mutation == "contact":
                report["sprites"][0]["native_contact"][0] += 1
            elif mutation == "anchor":
                report["sprites"][0]["anchor"][1] -= 1
            elif mutation == "source":
                report["evidence"]["NE"]["original"]["sha256"] = "0"*64
            elif mutation == "coverage":
                report["coverage_complete"] = False
            elif mutation == "packed":
                report["packed"] = True
            else:
                report["runtime_activated"] = True
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                source.decode(report)

    def test_cross_heading_self_chained_and_wrong_active_frame_aliases_are_refused(self):
        """Aliases must name the declared same-heading direct canonical physical pose.

        @param self Active direct immutable alias and funded-frame refusal check.
        @return None.
        """
        # Each independent bad address must fail even though all112logical
        # positions remain present and their accepted pixel files are unchanged.
        for mutation in ("heading", "self", "chain", "active_frame", "static_frame", "alias_kind", "boolean_target"):
            report = copy.deepcopy(self.report)
            alias = next(row for row in report["sprites"] if row["state"] == "recovery" and row["frame"] == 1)
            if mutation == "heading":
                alias["alias_of"]["direction"] = "NE"
            elif mutation == "self":
                alias["alias_of"] = source.pose(alias["direction"], "recovery", 1)
            elif mutation == "chain":
                alias["alias_of"] = source.pose(alias["direction"], "preparing", 1)
            elif mutation == "active_frame":
                alias["alias_of"]["frame"] = 0
            elif mutation == "static_frame":
                alias = next(row for row in report["sprites"] if row["state"] == "service")
                alias["alias_of"]["frame"] = 1
            elif mutation == "alias_kind":
                alias["alias_kind"] = "static"
            else:
                alias["alias_of"]["tile_variant"] = False
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                source.decode(report)

    def test_stored_png_palette_transparency_mask_and_coherently_rehashed_pixels_are_refused(self):
        """Actual PNG decoding rejects visual drift even when new artifact hashes are supplied.

        @param self Active saved image, original-pixel fixture and explicit-empty-mask check.
        @return None.
        """
        atlas_path = source.OUTPUT/"cargo-rocket-authored.png"
        with Image.open(atlas_path) as image:
            original = image.copy()
        # These detached PNG buffers isolate each material format invariant;
        # rehashed on-disk artwork below separately exercises genuine-byte drift.
        for mutation in ("palette", "transparency", "mode", "forbidden", "mask"):
            pixels = original.copy()
            mask = Image.new("L", original.size, 0)
            if mutation == "palette":
                palette = pixels.getpalette()
                palette[3] = (palette[3]+1) % 256
                pixels.putpalette(palette)
            elif mutation == "transparency":
                pixels.info["transparency"] = 1
            elif mutation == "mode":
                pixels = pixels.convert("RGBA")
            elif mutation == "forbidden":
                pixels.putpixel((0, 0), next(value for value in art.source.base.FORBIDDEN if value != 0))
            else:
                mask.putpixel((0, 0), 255)
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                source.decode(self.report, atlas=pixels, mask=mask)
        report = copy.deepcopy(self.report)
        row = report["canonical_parts"][0]
        canonical_path = ROOT/row["image"]["path"]
        retained = {path: path.read_bytes() for path in (atlas_path, canonical_path)}
        # Rewrite both actual PNGs and all corresponding declared hashes to
        # show that the independent accepted original fixture still refuses drift.
        try:
            with Image.open(canonical_path) as image:
                pixels = image.copy()
            pixels.putpixel((32, 16), 1 if pixels.getpixel((32, 16)) != 1 else 2)
            pixels.save(canonical_path, transparency=0)
            changed = original.copy()
            changed.paste(pixels, (0, 0))
            changed.save(atlas_path, transparency=0)
            report["atlas"] = art.source.identity(atlas_path)
            changed_hash = hashlib.sha256(pixels.tobytes()).hexdigest()
            for entry in report["canonical_parts"]+report["sprites"]:
                if entry["box"] == [0, 0, 64, 48]:
                    entry.update(image=art.source.identity(canonical_path), native_sha256=changed_hash)
            with self.assertRaisesRegex(ValueError, "decoded pixels"):
                source.decode(report)
        finally:
            # Exact restoration preserves the already reviewed outputs and
            # prevents a negative test from becoming a subsequent source input.
            for path, data in retained.items():
                path.write_bytes(data)
        self.assertEqual(self.report["atlas"], art.source.identity(atlas_path))

    def test_forged_original_registration_missing_engine_and_unqualified_material_are_refused(self):
        """Qualified material needs its genuine original receipts and completed producer proofs.

        @param self Active original-ancestry, source registration and completion gate check.
        @return None.
        """
        # A source export cannot substitute a plausible original or completion
        # flag for the actual preceding selected-part/native qualification.
        for mutation in ("missing_engine", "duplicate_engine", "changed_receipt", "changed_transform", "wrong_frame"):
            document = copy.deepcopy(self.document)
            engine_key = "cargo-rocket-engine-frame-1-v1"
            selected = next(row for row in document["candidates"] if row["key"] == engine_key)
            if mutation == "missing_engine":
                document["candidates"].remove(selected)
            elif mutation == "duplicate_engine":
                document["candidates"].append(copy.deepcopy(selected))
            elif mutation == "changed_receipt":
                selected["generation_receipt"]["sha256"] = "0"*64
            elif mutation == "changed_transform":
                selected["native_transforms"]["AXIAL"]["native_reference"][0] += 1
            else:
                selected["component_frame"] = 0
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                source.checked_art(document=document)
        policy = copy.deepcopy(self.policy)
        policy["state_art_complete"] = False
        with self.assertRaises(ValueError):
            source.checked_art(policy=policy)
        policy = copy.deepcopy(self.policy)
        policy["engine_qualification"]["sha256"] = "0"*64
        with self.assertRaises(ValueError):
            source.checked_art(policy=policy)

    def test_all_contacts_explicit_mask_completed_predecessors_and_native_payload_are_preserved(self):
        """Reviewed native/x4 sheets, public geometry and all protected families stay exact.

        @param self Active full contact, source boundary and predecessor preservation check.
        @return None.
        """
        public = art.read_json(art.PUBLIC)
        art.checked_predecessor(self.document, public)
        row = next(row for row in public["assets"] if row["asset_id"] == art.ASSET)
        before = art.read_json(art.FIXTURE)["rocket_before"]
        self.assertEqual({name: value for name, value in row.items() if name not in ("source", "authoring")},
                         {name: value for name, value in before.items() if name not in ("source", "authoring")})
        self.assertEqual(row["company_colors"], "explicit_mask")
        self.assertEqual(self.report["company_mask"]["kind"], "explicit_empty")
        self.assertEqual(self.report["company_mask"]["pixels"], 0)
        self.assertEqual(len(self.report["contacts"]), 7)
        report = copy.deepcopy(self.report)
        report["contacts"].pop()
        with self.assertRaisesRegex(ValueError, "contact identities"):
            source.decode(report)
        family = self.document["families"][art.ASSET]
        if family["source_coverage"]:
            # Existing published owners must agree with each other. During an
            # explicit rebuild the qualified replacement is still provisional;
            # verify-published additionally requires its actual current bytes.
            if self.require_published:
                self.assertEqual(family["source_manifest"], art.source.identity(source.MANIFEST))
            self.assertEqual(row["source"]["status"], "authored")
            self.assertEqual(row["authoring"]["atlas_manifest_sha256"], family["source_manifest"]["sha256"])
        self.assertFalse(family["packed"])
        self.assertFalse(family["runtime_activated"])


def main():
    """Run one focused source burst and optionally publish the qualified owned handoff.

    @return Zero only when actual source checks and requested publication checks pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--publish", action="store_true")
    mode.add_argument("--verify-published", action="store_true")
    args = parser.parse_args()
    CargoRocketSourceChecks.require_published = args.verify_published
    started = time.monotonic()
    if not args.verify_published:
        source.derive()
    result = unittest.TextTestRunner(verbosity=2, stream=sys.stdout).run(unittest.defaultTestLoader.loadTestsFromTestCase(CargoRocketSourceChecks))
    verification = dict(_documentation=__doc__, status="passed" if result.wasSuccessful() else "failed", cases=result.testsRun,
                        elapsed_seconds=round(time.monotonic()-started, 3), tests=art.source.identity(Path(__file__)),
                        converter=art.source.identity(Path(source.__file__)), source_manifest=art.source.identity(source.MANIFEST),
                        logical_poses=112, canonical_images=24, direct_aliases=88, static_aliases=72, equivalent_active_aliases=16,
                        full_source_decoded=result.wasSuccessful(), published=False, packed=False, runtime=False)
    source.write_json(source.VERIFICATION, verification)
    if not result.wasSuccessful():
        return 1
    if args.publish:
        source.publish(art.read_json(source.MANIFEST))
    # Both explicit publication and read-only publication verification must
    # bind the real saved source, not merely the provisional conversion buffers.
    if args.publish or args.verify_published:
        # Publication is checked against actual source files and protected
        # payload after both owned JSON updates; packing/game acceptance stays open.
        document, public = art.source.read_record(), art.read_json(art.PUBLIC)
        art.checked_predecessor(document, public)
        source.decode(art.read_json(source.MANIFEST))
        family = document["families"][art.ASSET]
        row = next(row for row in public["assets"] if row["asset_id"] == art.ASSET)
        # Both public handoff owners must advertise these exact qualified bytes;
        # successful conversion alone never closes packing or runtime gates.
        if (family["source_coverage"] is not True or family["source_manifest"] != art.source.identity(source.MANIFEST)
                or row["authoring"]["atlas_manifest_sha256"] != family["source_manifest"]["sha256"]):
            raise ValueError("rocket source postpublication handoff is absent or changed")
        verification["published"] = True
        source.write_json(source.VERIFICATION, verification)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
