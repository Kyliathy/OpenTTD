"""Focused original/model/state refusals for one selected Surface processor.

@file selected_processor_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups41-42.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Run each asset separately: these source checks do not grant packed/runtime status.
"""

import argparse
import copy
import hashlib
import io
import json
from pathlib import Path
import sys
import unittest

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_selected_processors as plant

ASSET = "facility.battery-works"


class SelectedProcessorSourceChecks(unittest.TestCase):
    """Verify genuine source stages, native ownership and earlier artifact preservation.

    @see r2mlvs14-remaining-assets-and-runtime-integration.md Groups41-42.
    """

    @classmethod
    def setUpClass(cls):
        """Regenerate one complete source matrix from its immutable originals.

        @param cls Active per-family proof fixture, never a gameplay participant.
        @return None.
        """
        cls.record = json.loads(plant.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(plant.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == ASSET)
        cls.donor = next(row for row in cls.document["assets"] if row["asset_id"] == plant.chemical.ASSET)
        cls.family = cls.record["families"][ASSET]
        cls.report, cls.files = plant.build(cls.record, cls.public, cls.donor)
        cls.atlas = Image.open(io.BytesIO(cls.files[plant.paths(ASSET)[0]]))

    def sprite(self, state, frame, variant, part=0):
        """Select an actual independent native source part from the regenerated atlas.

        @param self Active source proof fixture.
        @param state Native construction, ready, working or blocked state.
        @param frame Actual construction stage or synchronized native phase.
        @param variant Actual row-major machinery cell identity.
        @param part Original whole underlay zero or foreground subset one.
        @return Indexed native 64x64 original pixels at the fixed anchor.
        """
        x, y = (variant * 2 + part) * 64, (plant.STATES.index(state) * 4 + frame) * 64
        return self.atlas.crop((x, y, x + 64, y + 64))

    def test_exact_reproduction_and_complete_live_matrix(self):
        """All source artifacts and native slots reproduce exactly from retained originals.

        @param self Active source proof with freshly regenerated artifacts.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        plant.validate_public_source(self.public, self.report)
        self.assertEqual(len(self.report["source_parts"]), 512)
        self.assertEqual(self.atlas.size, (2048, 1024))
        self.assertEqual(self.atlas.info["transparency"], 0)

    def test_construction_contacts_and_original_foreground(self):
        """All independent stages keep paid floor contacts and genuinely distinct installation work.

        @param self Active source proof fixture.
        @return None.
        """
        # A campus composition can conceal contact errors; each native cell
        # must stand alone and retain the exact upper pixels of its own body.
        for variant in range(16):
            bodies = []
            for stage in range(4):
                back, front = self.sprite("construction", stage, variant), self.sprite("construction", stage, variant, 1)
                plant.mine.validate_cell(back, variant)
                self.assertIsNotNone(front.getbbox())
                self.assertTrue(all(not upper or upper == source for source, upper
                                    in zip(back.get_flattened_data(), front.get_flattened_data())))
                bodies.append(back.tobytes())
            self.assertEqual(len(set(bodies)), 4, variant)

    def test_native_status_only_changes_existing_glass(self):
        """Idle/full stay static, while working changes only the pre-existing control fitting.

        @param self Active source proof fixture.
        @return None.
        """
        # Render phase cannot move stock, install equipment or earn work.
        # Every completed body stays byte-exact outside the existing hut lens.
        for variant in range(16):
            complete = self.sprite("construction", 3, variant)
            for state in ("ready", "working", "blocked"):
                for frame in range(4):
                    actual = self.sprite(state, frame, variant)
                    for y in range(64):
                        for x in range(64):
                            if variant == 11 and 30 <= x < 33 and 43 <= y < 46:
                                continue
                            self.assertEqual(actual.getpixel((x, y)), complete.getpixel((x, y)))
            for state in ("ready", "blocked"):
                self.assertEqual(len({self.sprite(state, phase, variant).tobytes() for phase in range(4)}), 1)
        self.assertEqual(len({self.sprite("working", phase, 11).tobytes() for phase in range(4)}), 4)

    def test_original_four_angle_frames_and_immutable_stage_ancestry(self):
        """All new machine stages retain measured isolated angles and exact original owners.

        @param self Active source proof fixture.
        @return None.
        """
        # Canonical NE supplies native pixels. All four retained genuine
        # angles must independently exclude guides, haze and other subjects.
        for stage, selections in self.family["original_cells"].items():
            for selection in selections:
                source, component = plant.mine.component_record(self.record, selection)
                plant.mine.facilities.verify_original(source)
                self.assertEqual(source["construction_stage"], int(stage))
                self.assertEqual(component["construction_stage"], int(stage))
                self.assertEqual(source["processor_asset_id"], ASSET)
                self.assertTrue(source["empty_equipment"])
                self.assertEqual({row["angle"] for row in source["review_angle_windows"]}, {"NE", "SE", "SW", "NW"})
                with Image.open(ROOT / source["path"]) as raw:
                    for row in source["review_angle_windows"]:
                        crop = raw.convert("RGBA").crop(row["source_window"])
                        bounds = crop.getchannel("A").point(lambda a: 255 if a >= 16 else 0).getbbox()
                        self.assertEqual(list(bounds), row["measured_alpha16_bbox"])
                        self.assertGreaterEqual(min(bounds[0], bounds[1], crop.width - bounds[2], crop.height - bounds[3]), 16)
                        self.assertFalse(any(a >= 16 and r > 220 and g < 60 and b > 220
                                             for r, g, b, a in crop.get_flattened_data()))

    def test_false_native_view_and_incomplete_or_stale_reports_refuse(self):
        """Plausible source flags cannot authorize foreign views, missing slots or stale pixels.

        @param self Active source proof fixture.
        @return None.
        """
        public = copy.deepcopy(self.public)
        public["directions"] = ["NE", "SE", "SW", "NW"]
        with self.assertRaises(ValueError):
            plant.chemical.plant.shared.validate_processor_contract(public)
        # The live report is immutable acceptance evidence: altered rows or
        # invented source identities must refuse before public state changes.
        for field, value in (("source_parts", self.report["source_parts"][:-1]),
                             ("source_sha256", "0" * 64), ("runtime_activated", True)):
            report = copy.deepcopy(self.report)
            report[field] = value
            with self.assertRaises(ValueError):
                plant.validate_public_source(self.public, report)

    def test_unrelated_sources_payloads_and_binaries_remain_exact(self):
        """Selected source publication preserves every previous family and native payload.

        @param self Active source proof fixture.
        @return None.
        """
        before = json.loads((ROOT / self.family["verification_baseline"]).read_text(encoding="utf-8"))
        allowed = {"media/multilayer/contracts/multilayer_sprite_manifest.json",
                   "media/multilayer/source/ground/processor-generation-record.json",
                   "media/multilayer/contracts/native-processor-geometry.json",
                   "media/multilayer/contracts/native-mine-geometry.json"}
        # The two native descriptors were refreshed by their actual focused
        # producers after demo changes. Art inputs, payloads and binaries stay exact.
        for name, digest in before["files"].items():
            if name not in allowed:
                self.assertEqual(hashlib.sha256((ROOT / name).read_bytes()).hexdigest(), digest, name)
        for row in self.document["assets"]:
            if row["asset_id"] in before["unrelated_public_rows"]:
                self.assertEqual(row, before["unrelated_public_rows"][row["asset_id"]])
        # The later family must preserve the complete earlier source record,
        # not merely its public atlas pointer or raw-file hashes.
        for asset, family in before.get("completed_families", {}).items():
            self.assertEqual(self.record["families"][asset], family)
        previous = {row["id"]: row for row in before.get("completed_candidates", [])}
        for candidate in self.record["candidates"]:
            if candidate["id"] in previous:
                self.assertEqual(candidate, previous[candidate["id"]])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset", choices=plant.SELECTED, default=ASSET)
    args, remaining = parser.parse_known_args()
    ASSET = args.asset
    unittest.main(argv=[sys.argv[0], *remaining])
