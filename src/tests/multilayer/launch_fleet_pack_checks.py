"""Qualify deterministic launch/fleet packing inputs before scoped native staging.

@file launch_fleet_pack_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62I.1.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Six focused cases compare fresh conversions, exact packed crops/anchors and
append/refusal boundaries. Actual GRF decoding and gameplay remain separate.
"""

import argparse
import copy
import hashlib
import io
import json
from pathlib import Path
import sys
import time
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT/"media/multilayer"))
import pack_launch_fleet_art as packing
from tools import cargo_rocket_art as art
from tools import launch_fleet_handoff as handoff


class LaunchFleetPackChecks(unittest.TestCase):
    """Preserve all qualified original pixels and native IDs in deterministic own append outputs."""

    @classmethod
    def setUpClass(cls):
        """Derive fresh source conversions independently, without reusing image/cell caches.

        @param cls Active packing-input qualification suite with immutable shared outputs.
        @return None.
        """
        cls.scope, cls.handoff = packing.read_scope()
        cls.public = art.read_json(art.PUBLIC)
        cls.report, cls.files = packing.derive()
        cls.second_report, cls.second_files = packing.derive()
        cls.image = Image.open(io.BytesIO(cls.files[ROOT/cls.scope["png"]]))
        cls.families = {row["asset_id"]: row for row in cls.public["assets"]}

    def test_two_fresh_conversions_have_identical_report_png_nfo_and_range_header_bytes(self):
        """Native append identities and all own file bytes reproduce from the same originals.

        @param self Active deterministic source-to-input comparison.
        @return None.
        """
        self.assertEqual(self.report, self.second_report)
        self.assertEqual(self.files, self.second_files)
        self.assertEqual(set(self.files), {ROOT/self.scope[name] for name in ("png", "nfo", "header")})
        self.assertEqual(self.report["pose_count"], 1648)
        self.assertEqual((self.report["native_start"], self.report["native_sprite_bound"]), (6143, 7791))
        self.assertEqual([row["native_offset"] for row in self.report["sprites"]], list(range(6143, 7791)))
        self.assertEqual(self.image.mode, "P")
        self.assertEqual(self.image.info["transparency"], 0)
        self.assertEqual(self.image.getpalette(), art.source.base.PALETTE_BYTES)
        self.assertFalse(set(self.image.get_flattened_data()) & (set(art.source.base.FORBIDDEN)-{0}))

    def test_every_packed_crop_reconstructs_its_qualified_original_native_pixels_and_anchor(self):
        """Both empty and opaque parts retain exact world registration and internal holes.

        @param self Active all1648actual packed rectangles versus source PNG comparison.
        @return None.
        """
        originals = {}
        # Reconstruct every full source cell using only its adjusted packed
        # anchor. This independently catches lost feet, cavities or tile offsets.
        for row in self.report["sprites"]:
            path = row["source_atlas"]
            if path not in originals:
                with Image.open(ROOT/path) as source:
                    originals[path] = source.copy()
            x, y, width, height = row["box"]
            actual = self.image.crop((x, y, x+width, y+height))
            self.assertEqual(hashlib.sha256(actual.tobytes()).hexdigest(), row["pixel_sha256"])
            family = self.families[row["asset_id"]]
            restored = Image.new("P", tuple(family["native_size"]), 0)
            restored.paste(actual, (row["anchor"][0]-row["source_anchor"][0], row["anchor"][1]-row["source_anchor"][1]))
            sx, sy, sw, sh = row["source_box"]
            expected = originals[path].crop((sx, sy, sx+sw, sy+sh))
            self.assertEqual(restored.tobytes(), expected.tobytes(), row["native_offset"])
            self.assertEqual(hashlib.sha256(expected.tobytes()).hexdigest(), row["source_native_sha256"])

    def test_approved320_empty_backs_and208_empty_fronts_keep_complete_native_slots(self):
        """Approved physical absence cannot become fabricated hardware or an omitted native ID.

        @param self Active actual transparent-cell crop and original registration check.
        @return None.
        """
        absent = [row for row in self.report["sprites"] if row["structurally_absent"]]
        self.assertEqual(len(absent), 528)
        self.assertEqual(sum(row["part"] == "back" for row in absent), 320)
        self.assertEqual(sum(row["part"] == "front" for row in absent), 208)
        # The shared cropper keeps an explicitly absent native cell at its
        # original full size/contact, allowing the loader to compare exact zeros.
        for row in absent:
            self.assertEqual(row["asset_id"], "facility.launch-center")
            self.assertEqual(row["box"][2:], [64, 64])
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(row["pixel_sha256"], hashlib.sha256(bytes(4096)).hexdigest())

    def test_native_nfo_and_range_header_activate_only_the_exact_new_append(self):
        """Existing generators emit own artwork paths and exact state/view/cell/part domains.

        @param self Active real emitted NFO/range-header identity and bounds check.
        @return None.
        """
        nfo = self.files[ROOT/self.scope["nfo"]].decode()
        header = self.files[ROOT/self.scope["header"]].decode()
        self.assertEqual(nfo.count("sprites/launch_final.png"), 1648)
        self.assertEqual(nfo.count("05 9C"), 1)
        self.assertIn("05 9C FF 70 06 FF FF 17", nfo)
        self.assertNotIn("sprites/space_final.png", nfo)
        self.assertNotIn("sprites/orbital_final.png", nfo)
        self.assertIn("LAUNCH_FLEET_SPRITE_START = 6143", header)
        self.assertIn("LAUNCH_FLEET_SPRITE_END = 7791", header)
        self.assertIn("{6143, 1536, 4, 4, 4, 12, 2}", header)
        self.assertIn("{7679, 112, 7, 2, 8, 1, 1}", header)
        self.assertIn("uint8_t parts;", header)
        self.assertFalse(self.report["runtime_activated"])
        self.assertIsNone(self.report["packed_grf"])

    def test_missing_duplicate_changed_source_and_moved_native_ranges_are_refused(self):
        """No source defect or changed published ID can be hidden by a larger staged append.

        @param self Active independent public source/native allocation refusal check.
        @return None.
        """
        # Each detached defect fails before publication, while the actual
        # source contracts and every earlier packed source remain untouched.
        for mutation in ("missing", "duplicate", "source", "prior", "new", "domain"):
            public = copy.deepcopy(self.public)
            rows = {row["asset_id"]: row for row in public["assets"]}
            rocket = rows["vehicle.model-14"]
            if mutation == "missing":
                public["assets"].remove(rocket)
            elif mutation == "duplicate":
                public["assets"].append(copy.deepcopy(rocket))
            elif mutation == "source":
                rocket["authoring"]["atlas_manifest_sha256"] = "0"*64
            elif mutation == "prior":
                rows["vehicle.model-15"]["packed"]["native_offset"] += 1
            elif mutation == "new":
                rocket["packed"]["native_offset"] = 6143
            else:
                rocket["tile_variants"] = True
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                packing.derive(public)

    def test_prior_inputs_and_retained_payload_are_exact_and_writer_refuses_foreign_paths(self):
        """The new stage writer cannot overwrite previous sources or additional destinations.

        @param self Active actual prior input identities and scoped-write refusal check.
        @return None.
        """
        # Actual previous sheets/declarations and archived payload remain exact;
        # their new shared-GRF decoding gate is deliberately outside source staging.
        for reference in self.handoff["protected_outputs"]:
            self.assertEqual(art.source.identity(ROOT/reference["path"]), reference)
        self.assertEqual(art.source.identity(ROOT/self.handoff["predecessor_payload"]["path"]), self.handoff["predecessor_payload"])
        for mutation in ("extra", "missing", "redirected"):
            files = dict(self.files)
            if mutation == "missing":
                files.pop(ROOT/self.scope["png"])
            elif mutation == "redirected":
                files[ROOT/"test-cache/unowned-launch-art.png"] = files.pop(ROOT/self.scope["png"])
            else:
                files[ROOT/"test-cache/unowned-launch-art.png"] = b"invalid extra output"
            with self.subTest(mutation=mutation), self.assertRaisesRegex(ValueError, "output path"):
                packing.publish(self.report, files)
        self.assertFalse((ROOT/"test-cache/unowned-launch-art.png").exists())


def main():
    """Qualify this packing-input group and optionally stage its exact three owned outputs.

    @return Zero only after six focused cases and requested actual staging checks pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", action="store_true")
    args = parser.parse_args()
    start = time.monotonic()
    result = unittest.TextTestRunner(verbosity=2, stream=sys.stdout).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchFleetPackChecks))
    scope = art.read_json(packing.SCOPE)
    verification = dict(_documentation=__doc__, status="passed" if result.wasSuccessful() else "failed", cases=result.testsRun,
                        elapsed_seconds=round(time.monotonic()-start, 3), packer=art.source.identity(Path(packing.__file__)),
                        tests=art.source.identity(Path(__file__)), scope=art.source.identity(packing.SCOPE),
                        source_handoff=scope["source_handoff"], two_fresh_conversions_equal=result.wasSuccessful(),
                        logical_poses=1648, absent_back_parts=320, absent_front_parts=208, staged=False, packed=False, runtime=False)
    destination = ROOT/scope["qualification"]
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(verification, indent=2)+"\n", encoding="utf-8")
    if not result.wasSuccessful():
        return 1
    if args.stage:
        packing.publish(LaunchFleetPackChecks.report, LaunchFleetPackChecks.files)
        # Actual staged byte comparison closes only this source-input step;
        # native decoding/payload checksums and game presentation remain open.
        for path, data in LaunchFleetPackChecks.files.items():
            if path.read_bytes() != data:
                raise ValueError("launch/fleet actual staged output differs from qualified bytes")
        verification["staged"] = True
        destination.write_text(json.dumps(verification, indent=2)+"\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
