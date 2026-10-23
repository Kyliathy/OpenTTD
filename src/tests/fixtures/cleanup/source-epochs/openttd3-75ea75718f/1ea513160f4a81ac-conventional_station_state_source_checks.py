"""Focused full-source proof for atomic station previews and real loading presentation.

@file conventional_station_state_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task24.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No runtime renderer is activated. Native ready views, original glass ancestry,
source-frame aliases and all512 public source slots are checked independently.
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
import convert_station_states as states


class ConventionalStationStateSourceChecks(unittest.TestCase):
    """Require complete original source coverage without invented work, cargo or rendering owners."""

    @classmethod
    def setUpClass(cls):
        """Regenerate only the active complete station source family once.

        @param cls Test class retaining original, native and public artifact identities.
        @return None.
        """
        cls.record = json.loads(states.loader.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(states.loader.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == "facility.rail-station")
        cls.report, cls.files = states.build(cls.record, cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[states.OUTPUT / f"{states.PREFIX}-authored.png"]))
        cls.native, cls.originals, _ = states.base_sources(cls.record)
        cls.lenses = states.original_lenses(cls.report["original_status_glass"])

    def sprite(self, row):
        """Read one actual native source owner from its declared atlas rectangle.

        @param self Active complete source proof.
        @param row Explicit state/frame/native-cell/part source mapping.
        @return Indexed64x64 source image, including true structural blanks.
        """
        x, y, width, height = row["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def test_exact_full_artifact_regeneration_and_public_binding(self):
        """Complete public source claims identify the exact original assembly and generated atlas.

        @param self Active complete source proof.
        @return None.
        """
        # Compare exact bytes for every full-source artifact and each retained
        # low/canopy input review; old approved source views remain untouched.
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        for artifact in self.report["preserved_native_view_artifacts"]:
            self.assertEqual(states.loader.base.digest(ROOT / artifact["path"]), artifact["sha256"])
        states.validate_public_source(self.public, self.report)
        self.assertTrue(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertIsNone(self.public["packed"]["offset"])

    def test_complete_matrix_and_native_structural_absences(self):
        """Every public state/frame/role/axis/part exists with only128 genuine low-role absences.

        @param self Active complete source proof.
        @return None.
        """
        rows = self.report["sprites"]
        expected = {(direction, role, state, frame, part) for direction in ("NE", "SE") for role in range(4)
                    for state in ("construction", "ready", "working", "blocked") for frame in range(4)
                    for part in ("back", "front", "roof", "glass")}
        self.assertEqual(len(rows), 512)
        self.assertEqual({(row["direction"], row["tile_variant"], row["state"], row["frame"], row["part"]) for row in rows}, expected)
        self.assertEqual(sum(row["structurally_absent"] for row in rows), 128)
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.size, (2048, 1024))
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertFalse(set(self.atlas.get_flattened_data()) & (states.loader.base.FORBIDDEN - {0}))
        # Absence follows the compiled geometry rather than a blank source
        # window, so no missing canopy/glass view can disappear from coverage.
        for row in rows:
            structural = row["native_graphics"] < 4 and row["part"] in ("roof", "glass")
            self.assertEqual(row["structurally_absent"], structural)
            self.assertEqual(self.sprite(row).getbbox() is None, structural)
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(row["world_cell"], [0, 0])

    def test_only_original_equipment_glass_changes_and_alpha_stays_exact(self):
        """All state/phase differences stay inside the two opaque approved glass pixels.

        @param self Active complete source proof.
        @return None.
        """
        # Compare each complete source part with its independently approved
        # native ready original; roof/glass and contacts must remain byte-exact.
        for row in self.report["sprites"]:
            if row["structurally_absent"]:
                continue
            graphic, part = row["native_graphics"], row["part"]
            original = self.originals[graphic][part]
            target = self.report["equipment_geometry"]["cue_targets"][str(graphic)]
            allowed = {tuple(point) for point in target["points"]} if part == target["part"] else set()
            actual = self.sprite(row)
            differences = {(index % 64, index // 64) for index, (old, new)
                           in enumerate(zip(original.get_flattened_data(), actual.get_flattened_data())) if old != new}
            self.assertTrue(differences <= allowed, (graphic, part, row["state"], row["frame"]))
            self.assertEqual(original.point(lambda value: 255 if value else 0).tobytes(),
                             actual.point(lambda value: 255 if value else 0).tobytes())
            if allowed:
                donor = list(self.lenses[row["state"]][row["authored_phase"]].get_flattened_data())
                self.assertEqual([actual.getpixel(tuple(point)) for point in target["points"]], donor)

    def test_static_preview_aliases_and_four_distinct_loading_phases(self):
        """Atomic preview/ready/refusal frames stay static; only actual loading may select four phases.

        @param self Active complete source proof.
        @return None.
        """
        # Source-phase uniqueness is checked at visible native whole-pose scale:
        # a hidden cue or quantized duplicate cannot count as loading animation.
        for graphics in range(8):
            state_signatures = []
            for state in states.STATES:
                signatures = []
                for frame in range(4):
                    rows = [row for row in self.report["sprites"] if (row["native_graphics"], row["state"], row["frame"]) == (graphics, state, frame)]
                    parts = {row["part"]: None if row["structurally_absent"] else self.sprite(row) for row in rows}
                    signatures.append(states.compose(parts).tobytes())
                    self.assertEqual({row["authored_phase"] for row in rows}, {frame if state == "working" else 0})
                self.assertEqual(len(set(signatures)), 4 if state == "working" else 1, (graphics, state))
                state_signatures.append(signatures[0])
            self.assertEqual(len(set(state_signatures)), 4, graphics)
        self.assertEqual(self.report["native_state_policy"]["world_states"], ["ready", "working"])
        self.assertEqual(self.report["native_state_policy"]["preview_states"], ["construction", "blocked"])
        self.assertIn("TimerGameTick::counter", self.report["native_state_policy"]["frame_owner"])

    def test_all_source_states_preserve_native_rail_floor_and_foreground_clearance(self):
        """Equipment glass cannot paint a rail-floor bridge or cover trains outside real canopy posts.

        @param self Active complete source proof.
        @return None.
        """
        # Check actual projected rail-centre points independently of converter
        # validators. Canopy foreground posts may obscure only their genuine
        # physical columns; new status pixels lie below or outside train heights.
        for graphics in range(8):
            axis_x = graphics % 2 == 0
            original_front = self.originals[graphics]["front"]
            for row in self.report["sprites"]:
                if row["native_graphics"] != graphics or row["part"] != "front":
                    continue
                front = self.sprite(row)
                for longitudinal in range(16):
                    wx, wy = (longitudinal, 8) if axis_x else (8, longitudinal)
                    x, floor_y = 31 + 2 * (wy - wx), 33 + wx + wy
                    for height in range(9):
                        self.assertEqual(front.getpixel((x, floor_y - height)),
                                         original_front.getpixel((x, floor_y - height)))

    def test_changed_state_owner_fitting_and_foreign_donor_refuse(self):
        """Native geometry, real loading policy and exact retained material ancestry are mandatory.

        @param self Active complete source proof.
        @return None.
        """
        # Each malformed isolated input must refuse before original conversion
        # or artifact writes, preserving the last complete public source proof.
        for field in ("fitting", "donor", "world_state", "construction_phase", "native_owner"):
            record, public = copy.deepcopy(self.record), copy.deepcopy(self.public)
            family = record["families"]["facility.rail-station"]
            if field == "fitting":
                family["state_component_geometry"]["cue_targets"]["0"]["points"][0][0] += 1
            elif field == "donor":
                family["state_component_source"]["source_key"] = "foreign-lamp"
            elif field == "world_state":
                public["native_state_policy"]["world_states"].append("construction")
            elif field == "construction_phase":
                public["native_state_policy"]["frame_aliases"]["construction"] = [0, 1, 2, 3]
            else:
                family["state_component_geometry"]["native_presentation_owner_sha256"]["src/rail_gui.cpp"] = "0" * 64
            with self.subTest(field=field), self.assertRaises(ValueError):
                states.checked_components(record, public)

    def test_missing_original_and_transparent_or_duplicate_fittings_refuse(self):
        """No substitute source color or new opaque pixel may rescue incomplete original equipment.

        @param self Active complete source proof.
        @return None.
        """
        component = copy.deepcopy(self.report["original_status_glass"])
        component["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            states.original_lenses(component)
        original = self.originals[0]["back"]
        lens = self.lenses["ready"][0]
        for points in ([[0, 0], [1, 0]], [[4, 46], [4, 46]], [[64, 46], [5, 46]]):
            with self.subTest(points=points), self.assertRaisesRegex(ValueError, "fitting"):
                states.apply_lens(original, lens, {"part": "back", "points": points})

    def test_false_complete_public_atlas_or_source_identity_refuse(self):
        """Exact source pointers, counts and unpacked boundary prevent a false delivery claim.

        @param self Active complete source proof.
        @return None.
        """
        # Public metadata is validated against actual fresh source assembly;
        # another atlas, wrong counts or a premature packed route cannot pass.
        for field in ("atlas", "count", "source", "packed"):
            public = copy.deepcopy(self.public)
            if field == "atlas":
                public["authoring"]["native_sha256"] = "0" * 64
            elif field == "count":
                public["authoring"]["nonempty_source_parts"] = 512
            elif field == "source":
                public["source"]["sha256"] = "0" * 64
            else:
                public["packed"]["offset"] = 3588
            with self.subTest(field=field), self.assertRaises(ValueError):
                states.validate_public_source(public, self.report)


if __name__ == "__main__":
    unittest.main()
