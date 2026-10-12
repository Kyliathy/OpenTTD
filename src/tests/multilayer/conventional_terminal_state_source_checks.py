"""Focused complete-source proof for actual terminal previews and loading presentation.

@file conventional_terminal_state_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task25.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All288 source slots retain actual native parent ownership. Hidden front hardware
stays hidden; complete source coverage grants no packed or runtime activation.
"""

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
sys.path.insert(0, str(ROOT / "src/tests"))
import convert_terminal_states as states
import multilayer_sprite_contract as contract


class ConventionalTerminalStateSourceChecks(unittest.TestCase):
    """Require complete real source coverage, fixed native geometry and retained glass ancestry."""

    @classmethod
    def setUpClass(cls):
        """Regenerate the active complete terminal source family once.

        @param cls Test class retaining public, native and immutable original evidence.
        @return None.
        """
        cls.record = json.loads(states.terminal.RECORD.read_text(encoding="utf-8"))
        cls.document = json.loads(states.terminal.PUBLIC.read_text(encoding="utf-8"))
        cls.public = next(row for row in cls.document["assets"] if row["asset_id"] == "facility.road-terminal")
        cls.report, cls.files = states.build(cls.record, cls.public)
        cls.atlas = Image.open(io.BytesIO(cls.files[states.OUTPUT / f"{states.PREFIX}-authored.png"]))
        cls.native, cls.originals, _ = states.base_sources(cls.record)
        cls.lenses = states.original_lenses(cls.report["original_status_glass"])

    def sprite(self, row):
        """Read one independently sorted native source parent from its exact atlas rectangle.

        @param self Active complete source proof.
        @param row Declared native state/frame/view/parent mapping.
        @return Indexed64x64 source, including true structural blanks.
        """
        x, y, width, height = row["box"]
        return self.atlas.crop((x, y, x + width, y + height))

    def test_exact_artifact_regeneration_and_shared_public_binding(self):
        """Complete source claims identify the exact atlas, original assembly and unactivated boundary.

        @param self Active complete source proof.
        @return None.
        """
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload, path.name)
        for row in self.report["preserved_native_view_artifacts"]:
            self.assertEqual(states.terminal.base.digest(ROOT / row["path"]), row["sha256"])
        states.validate_public_source(self.public, self.report)
        self.assertEqual(contract.validate_manifest(self.document), 48893)
        self.assertTrue(self.report["source_coverage_complete"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertIsNone(self.public["packed"]["offset"])

    def test_complete_matrix_with_only_actual_through_parent_absences(self):
        """All256 real sources and32 structural absences cover the full native public matrix.

        @param self Active complete source proof.
        @return None.
        """
        rows = self.report["sprites"]
        directions = ("NE", "SE", "SW", "NW", "X", "Y")
        expected = {(direction, state, frame, part) for direction in directions for state in states.STATES
                    for frame in range(4) for part in ("parent-0", "parent-1", "parent-2")}
        self.assertEqual(len(rows), 288)
        self.assertEqual({(row["direction"], row["state"], row["frame"], row["part"]) for row in rows}, expected)
        self.assertEqual(sum(row["structurally_absent"] for row in rows), 32)
        self.assertEqual(self.atlas.mode, "P")
        self.assertEqual(self.atlas.size, (1152, 1024))
        self.assertEqual(self.atlas.info["transparency"], 0)
        self.assertFalse(set(self.atlas.get_flattened_data()) & (states.terminal.base.FORBIDDEN - {0}))
        # Absence is determined by actual compiled parents, never by an empty
        # generated rectangle. Every other native slot must contain real pixels.
        for row in rows:
            absent = row["native_graphics"] >= 4 and row["part"] == "parent-2"
            self.assertEqual(row["structurally_absent"], absent)
            sprite = self.sprite(row)
            self.assertEqual(sprite.getbbox() is None, absent)
            self.assertEqual(row["source_pixels_sha256"], None if absent else hashlib.sha256(sprite.tobytes()).hexdigest())
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(row["world_cell"], [0, 0])
            self.assertEqual(row["native_packed_anchor"], self.public["native_packed_anchors"][row["direction"]][row["part"]])

    def test_only_one_original_glass_pixel_changes_and_all_alpha_stays_exact(self):
        """All physical geometry and other material remain exact in every native source state.

        @param self Active complete source proof.
        @return None.
        """
        # Compare every actual parent against its independently approved ready
        # original. Rear views, rails and hidden hardware cannot acquire cues.
        for row in self.report["sprites"]:
            if row["structurally_absent"]:
                continue
            graphic, part = row["native_graphics"], row["part"]
            original, actual = self.originals[graphic][part], self.sprite(row)
            target = self.report["equipment_geometry"]["cue_targets"][str(graphic)]
            allowed = set()
            # A visible cue belongs only to its one genuine housing parent;
            # neither neighboring rails nor original rear walls may inherit it.
            if target is not None and part == target["part"]:
                allowed = {tuple(point) for point in target["points"]}
            differences = {(index % 64, index // 64) for index, (old, new)
                           in enumerate(zip(original.get_flattened_data(), actual.get_flattened_data())) if old != new}
            self.assertTrue(differences <= allowed, (graphic, part, row["state"], row["frame"]))
            self.assertEqual(original.point(lambda value: 255 if value else 0).tobytes(),
                             actual.point(lambda value: 255 if value else 0).tobytes())
            if allowed:
                point = next(iter(allowed))
                self.assertEqual(actual.getpixel(point), self.lenses[row["state"]][row["authored_phase"]].getpixel((0, 0)))

    def test_static_preview_aliases_visible_loading_and_hidden_front_preservation(self):
        """Four authored phases affect only visible fronts; actual rear views remain physically unchanged.

        @param self Active complete source proof.
        @return None.
        """
        # Whole-view phase signatures establish visible animation, rather than
        # counting a changing pixel concealed by another source parent. Hidden
        # fronts intentionally alias their exact original whole in every state.
        for graphics in range(6):
            visible = graphics in (1, 2, 4)
            first_states = []
            original = states.views.compose([self.originals[graphics][part] for part in states.terminal.PARTS])
            for state in states.STATES:
                signatures = []
                for frame in range(4):
                    rows = [row for row in self.report["sprites"] if (row["native_graphics"], row["state"], row["frame"]) == (graphics, state, frame)]
                    parts = [None if row["structurally_absent"] else self.sprite(row) for row in rows]
                    signatures.append(states.views.compose(parts).tobytes())
                    self.assertEqual({row["authored_phase"] for row in rows}, {frame if visible and state == "working" else 0})
                self.assertEqual(len(set(signatures)), 4 if visible and state == "working" else 1, (graphics, state))
                first_states.append(signatures[0])
                if not visible:
                    self.assertEqual(signatures[0], original.tobytes(), (graphics, state))
            self.assertEqual(len(set(first_states)), 4 if visible else 1)
        self.assertEqual(self.report["native_state_policy"]["world_states"], ["ready", "working"])
        self.assertEqual(self.report["native_state_policy"]["preview_states"], ["construction", "blocked"])
        self.assertIn("TimerGameTick::counter", self.report["native_state_policy"]["frame_owner"])
        self.assertIn("RoadVehicle", self.report["native_state_policy"]["working"])

    def test_all_states_preserve_actual_entrances_and_through_lane_clearance(self):
        """State cues cannot introduce an entrance obstruction or repaint a rail over a truck.

        @param self Active complete source proof.
        @return None.
        """
        entrances = {0: [(x, y) for x in range(4) for y in (5, 9)],
                     1: [(x, y) for x in (5, 9) for y in range(12, 16)],
                     2: [(x, y) for x in range(12, 16) for y in (5, 9)],
                     3: [(x, y) for x in (5, 9) for y in range(4)]}
        # Independently inspect actual projected entrance/vehicle-centre points
        # in every state/frame. Source-only housing occlusion remains its real
        # native owner; through safety rails alone must clear both lane heights.
        for graphics in range(6):
            for state in states.STATES:
                for frame in range(4):
                    rows = [row for row in self.report["sprites"] if (row["native_graphics"], row["state"], row["frame"]) == (graphics, state, frame)]
                    parts = [None if row["structurally_absent"] else self.sprite(row) for row in rows]
                    if graphics < 4:
                        whole = states.views.compose(parts)
                        for wx, wy in entrances[graphics]:
                            self.assertEqual(whole.getpixel((31 + 2 * (wy - wx), 33 + wx + wy)), 0)
                    else:
                        for longitudinal in range(16):
                            for across in (5, 9):
                                wx, wy = (longitudinal, across) if graphics == 4 else (across, longitudinal)
                                for height in range(9):
                                    self.assertEqual(parts[1].getpixel((31 + 2 * (wy - wx), 33 + wx + wy - height)), 0)

    def test_changed_fitting_owner_donor_and_invented_preview_states_refuse(self):
        """Neither art-local loading nor relocated original glass can authorize source publication.

        @param self Active complete source proof.
        @return None.
        """
        for field in ("fitting", "hidden_front", "donor", "world_state", "construction_phase", "native_owner"):
            record, public = copy.deepcopy(self.record), copy.deepcopy(self.public)
            family = record["families"]["facility.road-terminal"]
            geometry = family["state_component_geometry"]
            if field == "fitting":
                geometry["cue_targets"]["1"]["points"][0][0] += 1
            elif field == "hidden_front":
                geometry["cue_targets"]["0"] = copy.deepcopy(geometry["cue_targets"]["1"])
            elif field == "donor":
                family["state_component_source"]["source_key"] = "foreign-lamp"
            elif field == "world_state":
                public["native_state_policy"]["world_states"].append("construction")
            elif field == "construction_phase":
                public["native_state_policy"]["frame_aliases"]["construction"] = [0, 1, 2, 3]
            else:
                geometry["native_presentation_owner_sha256"]["src/roadveh_cmd.cpp"] = "0" * 64
            with self.subTest(field=field), self.assertRaises(ValueError):
                states.checked_components(record, public)

    def test_missing_original_transparent_wall_and_enlarged_glass_refuse(self):
        """A fitting cannot expand onto the wall or empty space to manufacture a larger cue.

        @param self Active complete source proof.
        @return None.
        """
        component = copy.deepcopy(self.report["original_status_glass"])
        component["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "ancestry"):
            states.original_lenses(component)
        original, lens = self.originals[1]["parent-1"], self.lenses["ready"][0]
        for points in ([[0, 0]], [[64, 44]], [[10, 44], [11, 44]], [[0, 24]]):
            with self.subTest(points=points), self.assertRaises(ValueError):
                states.apply_lens(original, lens, {"part": "parent-1", "points": points})
        for path, payload in self.files.items():
            self.assertEqual(path.read_bytes(), payload)

    def test_false_complete_atlas_counts_source_or_packed_identity_refuse(self):
        """Public coverage binds exact source bytes and preserves the later packing boundary.

        @param self Active complete source proof.
        @return None.
        """
        for field in ("atlas", "count", "source", "packed", "runtime"):
            public = copy.deepcopy(self.public)
            if field == "atlas":
                public["authoring"]["native_sha256"] = "0" * 64
            elif field == "count":
                public["authoring"]["nonempty_source_parts"] = 288
            elif field == "source":
                public["source"]["sha256"] = "0" * 64
            elif field == "packed":
                public["packed"]["offset"] = 3588
            else:
                public["authoring"]["runtime_activated"] = True
            with self.subTest(field=field), self.assertRaises(ValueError):
                states.validate_public_source(public, self.report)


if __name__ == "__main__":
    unittest.main()
