"""Qualify the launch/fleet source freeze and existing-packer adapter without publication.

@file launch_fleet_handoff_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62H.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Only this source/append boundary is checked. Reports remain under
logs/proofs/mlm/native-art/space/launch-fleet-handoff-v1/; no native build or tick.
"""

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
import pack_m1_art as packing
from tools import cargo_rocket_art as art
from tools import launch_fleet_handoff as handoff


class LaunchFleetHandoffChecks(unittest.TestCase):
    """Require immutable accepted pixels, exact physical alias coverage and preserved native IDs."""

    @classmethod
    def setUpClass(cls):
        """Verify both source producers once and open their actual saved indexed atlases.

        @param cls Active focused packing-boundary suite with read-only shared buffers.
        @return None.
        """
        cls.accepted = handoff.checked_handoff()
        cls.public = art.read_json(art.PUBLIC)
        cls.families = {row["asset_id"]: row for row in cls.public["assets"]}
        cls.reports, cls.images, cls.poses = {}, {}, {}
        # Both independently accepted source producers must supply real stored
        # pixels before any shared packing comparison or append allocation.
        for asset in handoff.ASSETS:
            accepted = cls.accepted["sources"][asset]
            cls.reports[asset] = art.read_json(ROOT/accepted["report"]["path"])
            poses, image = handoff.source_poses(cls.families[asset], handoff=cls.accepted)
            cls.images[asset], cls.poses[asset] = image, poses

    def test_all1648_actual_source_positions_and528_explicit_absent_parts_are_preserved(self):
        """The adapter retains every qualified physical view/cell/part/state and rocket frame.

        @param self Active actual saved-source and physical absence check.
        @return None.
        """
        self.assertEqual(sum(len(poses) for poses in self.poses.values()), 1648)
        self.assertEqual(len(self.poses[handoff.ASSETS[0]]), 1536)
        self.assertEqual(len(self.poses[handoff.ASSETS[1]]), 112)
        # Actual transparent source parts stay explicit approved back/front
        # absence; a missing body or occupied shell cannot count as delivered art.
        for asset in handoff.ASSETS:
            empty = [row for row in self.poses[asset].values() if row["structurally_absent"]]
            self.assertEqual(len(empty), 528 if asset == handoff.ASSETS[0] else 0)
            self.assertTrue(all(row["part"] in ("back", "front") for row in empty))
            if asset == handoff.ASSETS[0]:
                self.assertEqual(sum(row["part"] == "back" for row in empty), 320)
                self.assertEqual(sum(row["part"] == "front" for row in empty), 208)
            self.assertEqual(set(self.poses[asset]), set(packing.required_keys(self.families[asset])))
            self.assertEqual(self.images[asset].mode, "P")
            self.assertEqual(self.images[asset].getpalette(), art.source.base.PALETTE_BYTES)
            self.assertEqual(self.images[asset].info["transparency"], 0)

    def test_existing_cropper_reconstructs_every_original_cell_and_world_anchor_exactly(self):
        """Transparent-margin cropping loses neither original pixels, holes nor physical contact.

        @param self Active shared-cropper versus immutable full-native-cell comparison.
        @return None.
        """
        # Reconstruct in the original native canvas using the adjusted anchor;
        # compare indexed bytes independently of the cropper's bounding algorithm.
        for asset in handoff.ASSETS:
            family, atlas = self.families[asset], self.images[asset]
            for row in self.poses[asset].values():
                indices, width, height, ax, ay = packing.cropped_pose(atlas, row)
                restored = Image.new("P", tuple(family["native_size"]), 0)
                cropped = Image.new("P", (width, height), 0)
                cropped.putdata(indices)
                restored.paste(cropped, (ax-family["anchor"][0], ay-family["anchor"][1]))
                x, y, native_width, native_height = row["box"]
                actual = atlas.crop((x, y, x+native_width, y+native_height))
                self.assertEqual(restored.tobytes(), actual.tobytes())
                self.assertEqual(hashlib.sha256(restored.tobytes()).hexdigest(), row["native_sha256"])
                if row["structurally_absent"]:
                    self.assertEqual((width, height), (64, 64))
                    self.assertEqual((ax, ay), tuple(family["anchor"]))

    def test_missing_duplicate_boolean_and_changed_physical_metadata_are_refused(self):
        """Counts cannot hide absent cells or typed geometry/physical-owner substitutions.

        @param self Active exact product and native source metadata refusal check.
        @return None.
        """
        # The same exact coverage boundary applies to twelve-cell facility
        # cuts and eight-heading rocket bodies, despite their different geometry.
        for asset in handoff.ASSETS:
            for mutation in ("missing", "duplicate", "boolean", "anchor", "bounds", "owner"):
                report = copy.deepcopy(self.reports[asset])
                row = report["sprites"][0]
                if mutation == "missing":
                    report["sprites"].pop()
                elif mutation == "duplicate":
                    report["sprites"][-1] = copy.deepcopy(row)
                elif mutation == "boolean":
                    row["tile_variant"] = False
                elif mutation == "anchor":
                    row["anchor"][0] += 1
                elif mutation == "bounds":
                    row["box"][0] = -64
                else:
                    row["native_origin" if asset == handoff.ASSETS[0] else "native_contact"][0] += 1
                with self.subTest(asset=asset, mutation=mutation), self.assertRaises(ValueError):
                    handoff.source_poses(self.families[asset], report=report, image=self.images[asset], handoff=self.accepted)

    def test_cross_heading_self_chained_and_wrong_frame_aliases_are_refused(self):
        """Every alias names its qualified direct physical source without view/frame drift.

        @param self Active complete direct-alias preservation and refusal check.
        @return None.
        """
        # Qualified aliases already declare exact reuse. Test independent
        # changed addresses while keeping original PNGs and logical counts intact.
        for asset in handoff.ASSETS:
            for mutation in ("heading", "self", "chain", "frame"):
                report = copy.deepcopy(self.reports[asset])
                row = next(row for row in report["sprites"] if row["alias_of"] is not None)
                if mutation == "heading":
                    row["alias_of"]["direction"] = "SW" if asset == handoff.ASSETS[0] else "NW"
                elif mutation == "self":
                    row["alias_of"] = {name: row[name] for name in handoff.FIELDS}
                elif mutation == "chain":
                    row["alias_of"]["state"] = "construction" if asset == handoff.ASSETS[0] else "service"
                    row["alias_of"]["frame"] = 1
                else:
                    row["alias_of"]["frame"] = 1
                with self.subTest(asset=asset, mutation=mutation), self.assertRaises(ValueError):
                    handoff.source_poses(self.families[asset], report=report, image=self.images[asset], handoff=self.accepted)

    def test_changed_png_pixels_palette_false_absence_and_rehashed_expectations_are_refused(self):
        """Actual PNGs remain tied to accepted independent canonical expectations.

        @param self Active raster corruption, forbidden color and absence refusal check.
        @return None.
        """
        # Detached actual PNG buffers exercise each material invariant without
        # changing the approved saved art or blessing new canonical expectations.
        for asset in handoff.ASSETS:
            for mutation in ("pixels", "palette", "transparency", "absence", "rehash"):
                report = copy.deepcopy(self.reports[asset])
                image = self.images[asset].copy()
                row = report["sprites"][0]
                if mutation == "palette":
                    palette = image.getpalette()
                    palette[3] = (palette[3]+1) % 256
                    image.putpalette(palette)
                elif mutation == "transparency":
                    image.info["transparency"] = 1
                elif mutation == "absence":
                    row["structurally_absent"] = not row["structurally_absent"]
                else:
                    x, y, width, height = row["box"]
                    image.putpixel((x, y), 1)
                    if mutation == "rehash":
                        digest = hashlib.sha256(image.crop((x, y, x+width, y+height)).tobytes()).hexdigest()
                        # Updating generated hashes cannot redefine a canonical
                        # source image already accepted by its original producer.
                        for entry in report["sprites"]+report["canonical_parts"]:
                            if entry["box"] == row["box"]:
                                entry["native_sha256"] = digest
                with self.subTest(asset=asset, mutation=mutation), self.assertRaises(ValueError):
                    handoff.source_poses(self.families[asset], report=report, image=image, handoff=self.accepted)

    def test_immutable_snapshot_source_proofs_and_public_bindings_cannot_be_replaced(self):
        """Historical source approval remains explicit; no new flag or reference can bless art.

        @param self Active frozen handoff, proof and source binding refusal check.
        @return None.
        """
        with self.assertRaisesRegex(ValueError, "never regenerate"):
            handoff.freeze()
        # A caller-supplied snapshot cannot expand this family's authority or
        # replace the independently accepted source/report/original expectations.
        for mutation in ("report", "proof", "scope", "canonical", "prefix"):
            accepted = copy.deepcopy(self.accepted)
            row = accepted["sources"][handoff.ASSETS[1]]
            if mutation == "report":
                row["report"]["sha256"] = "0"*64
            elif mutation == "proof":
                row["qualification"]["sha256"] = "0"*64
            elif mutation == "scope":
                accepted["native_start"] = 6000
            elif mutation == "canonical":
                next(iter(row["canonical"].values()))["native_sha256"] = "0"*64
            else:
                accepted["candidate_prefix_sha256"] = "0"*64
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                handoff.checked_handoff(accepted)
        family = copy.deepcopy(self.families[handoff.ASSETS[1]])
        family["authoring"]["atlas_manifest_sha256"] = "0"*64
        with self.assertRaises(ValueError):
            handoff.source_poses(family, handoff=self.accepted)

    def test_append_layout_retains_all_prior_ranges_and_actual_predecessor_payload(self):
        """The new1648positions append after6143without altering native or saved identities.

        @param self Active actual prefix, deterministic append and range refusal check.
        @return None.
        """
        handoff.validate_prepacking_prefix(self.accepted)
        ranges, end = handoff.layout(self.public, self.accepted)
        self.assertEqual([(row["native_offset"], row["sprite_count"]) for row in ranges], [(6143, 1536), (7679, 112)])
        self.assertEqual(end, 7791)
        self.assertEqual((ROOT/self.accepted["predecessor_payload"]["path"]).read_bytes(),
                         (ROOT/self.accepted["predecessor_payload_original"]["path"]).read_bytes())
        # Existing ranges and any repeated staged new append retain exact IDs;
        # unrelated MLVI definition-local namespaces/offsets remain untouched.
        for mutation in ("prior", "new", "domain", "definition"):
            public = copy.deepcopy(self.public)
            rows = {row["asset_id"]: row for row in public["assets"]}
            if mutation == "prior":
                rows["vehicle.model-15"]["packed"]["native_offset"] += 1
            elif mutation == "new":
                rows[handoff.ASSETS[1]]["packed"]["native_offset"] = 6143
            elif mutation == "domain":
                rows[handoff.ASSETS[1]]["tile_variants"] = True
            else:
                rows[handoff.ASSETS[1]]["packed"]["offset"] = 1
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                handoff.layout(public, self.accepted)


def main():
    """Run the current seven-case source/append qualification burst and retain its evidence.

    @return Zero only after all actual source, prefix and refusal checks pass.
    """
    start = time.monotonic()
    result = unittest.TextTestRunner(verbosity=2, stream=sys.stdout).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchFleetHandoffChecks))
    report = dict(_documentation=__doc__, status="passed" if result.wasSuccessful() else "failed", cases=result.testsRun,
                  elapsed_seconds=round(time.monotonic()-start, 3), helper=art.source.identity(Path(handoff.__file__)),
                  tests=art.source.identity(Path(__file__)), source_handoff=art.source.identity(handoff.HANDOFF),
                  logical_poses=1648, absent_back_parts=320, absent_front_parts=208, native_start=6143, native_end_exclusive=7791,
                  packed=False, runtime=False)
    (handoff.OUTPUT/"verification.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
