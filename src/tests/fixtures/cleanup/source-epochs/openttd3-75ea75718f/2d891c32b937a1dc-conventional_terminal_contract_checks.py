"""Focused six-layout native road-terminal source handoff and false-evidence refusals.

@file conventional_terminal_contract_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group25B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task25.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No raster or world mutation occurs. Geometry must come from a successful actual
native command/loading run. Later authored source requires its exact complete
atlas binding; packing and runtime retain their independent delivery boundaries.
"""

import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests"))
from tools import ground_terminal_contract as terminal
import multilayer_sprite_contract as shared


class ConventionalTerminalContractChecks(unittest.TestCase):
    """Preserve every actual native bay/through parent, source matrix and existing delivered owner."""

    @classmethod
    def setUpClass(cls):
        """Read authoritative native/public artifacts once for the active producer burst.

        @param cls Check class retaining exact compiled and shared source contracts.
        @return None.
        """
        cls.geometry = terminal.load_geometry()
        cls.document = json.loads(shared.MANIFEST.read_text(encoding="utf-8"))
        cls.row = next(row for row in cls.document["assets"] if row["asset_id"] == "facility.road-terminal")

    def test_six_native_views_and_complete_source_parent_matrix(self):
        """Actual six one-tile layouts require288 source slots with32 genuine through-parent absences.

        @param self Active terminal producer check.
        @return None.
        """
        terminal.validate_terminal_contract(self.row)
        self.assertEqual([row["graphics"] for row in self.geometry["layouts"]], list(range(6)))
        slots = len(self.row["directions"]) * len(self.row["states"]) * self.row["frames"] * len(self.row["parts"]) * self.row["tile_variants"]
        absent = sum(sum(parent is None for parent in row["parents"]) for row in self.geometry["layouts"]) * 4 * 4
        self.assertEqual((slots, absent, slots - absent), (288, 32, 256))
        self.assertEqual(shared.validate_manifest(self.document), 48893)
        rebuilt = next(row for row in shared.build_manifest()["assets"] if row["asset_id"] == "facility.road-terminal")
        terminal.validate_terminal_contract(rebuilt)
        self.assertIn(self.row["source"]["status"], ("planned", "authored"))
        # Native geometry was published before originals. Once task25.3 supplies
        # complete source, accept that progression only through its actual full
        # atlas/count/original binding, never by weakening geometry assertions.
        if self.row["source"]["status"] == "authored":
            import convert_terminal_states as states
            report = json.loads((ROOT / self.row["authoring"]["atlas_manifest"]).read_text(encoding="utf-8"))
            states.validate_public_source(self.row, report)
            self.assertEqual(terminal.digest(ROOT / report["source"]), report["source_sha256"])
        self.assertIsNone(self.row["packed"]["offset"])

    def test_bay_directions_and_through_axes_cannot_replace_each_other(self):
        """Reject the old campus and any omitted or relabelled genuine native cell.

        @param self Active terminal producer check.
        @return None.
        """
        # Every field independently controls actual source coverage, topology
        # or sorting. Plausible four-view/two-piece art cannot inherit approval.
        for field, value in (("footprint", [2, 2]), ("tile_variants", 4), ("directions", ["NE", "SE", "SW", "NW"]),
                             ("parts", ["back", "front"]), ("native_graphics", {"NE": 0, "SE": 1, "SW": 2, "NW": 3})):
            row = copy.deepcopy(self.row)
            row[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                terminal.validate_terminal_contract(row)
        document = copy.deepcopy(self.geometry)
        document["layouts"][0]["axis"] = "X"
        with self.assertRaisesRegex(ValueError, "entrance/axis"):
            terminal.validate_geometry(document)
        document = copy.deepcopy(self.geometry)
        document["layouts"][4]["entrance"] = "NE"
        with self.assertRaisesRegex(ValueError, "entrance/axis"):
            terminal.validate_geometry(document)

    def test_native_bay_third_parent_and_through_absence_are_required(self):
        """A third bay sorting owner cannot disappear or become an invented through wall.

        @param self Active terminal producer check.
        @return None.
        """
        for graphics in range(6):
            document = copy.deepcopy(self.geometry)
            parents = document["layouts"][graphics]["parents"]
            parents[2] = None if graphics < 4 else copy.deepcopy(parents[0])
            with self.subTest(graphics=graphics), self.assertRaisesRegex(ValueError, "parent coverage"):
                terminal.validate_geometry(document)
        document = copy.deepcopy(self.geometry)
        document["layouts"].append(copy.deepcopy(document["layouts"][0]))
        with self.assertRaisesRegex(ValueError, "six actual"):
            terminal.validate_geometry(document)

    def test_packed_offsets_compensate_every_actual_native_parent_origin(self):
        """Native drawing positions reconstruct the common source canvas without moving sorting boxes.

        @param self Active terminal producer check.
        @return None.
        """
        # Add actual RemapCoords(origin+offset) independently to the packed
        # offsets; every real parent must draw the same64x64 source canvas.
        for layout in self.geometry["layouts"]:
            for index, parent in enumerate(layout["parents"]):
                anchor = self.row["native_packed_anchors"][layout["direction"]][f"parent-{index}"]
                if parent is None:
                    self.assertIsNone(anchor)
                    continue
                x, y, z = [origin + offset for origin, offset in zip(parent["origin"], parent["offset"])]
                self.assertEqual([anchor[0] + 2 * (y - x), anchor[1] + x + y - z], [-31, -33])
        row = copy.deepcopy(self.row)
        row["native_packed_anchors"]["NE"]["parent-0"] = [-31, -33]
        with self.assertRaises(ValueError):
            terminal.validate_terminal_contract(row)

    def test_stale_native_owner_and_invented_world_preview_states_refuse(self):
        """Changed producers or art-owned loading cannot approve dependent source generation.

        @param self Active terminal producer check.
        @return None.
        """
        document = copy.deepcopy(self.geometry)
        document["native_source_sha256"]["src/station_cmd.cpp"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "owner changed"):
            terminal.validate_geometry(document)
        # Native atomic build/refusal is preview-only. Existing physical road
        # loading and the synchronized presentation clock remain separate owners.
        for field, value in (("world_states", ["construction", "ready", "working", "blocked"]),
                             ("working", "Sprite-local cargo queue creates a loading truck"),
                             ("frame_aliases", {state: [0, 1, 2, 3] for state in self.row["states"]})):
            row = copy.deepcopy(self.row)
            row["native_state_policy"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                terminal.validate_terminal_contract(row)

    def test_failed_partial_duplicate_and_fallback_native_logs_cannot_publish(self):
        """Require complete successful native geometry/command/loading evidence before any write.

        @param self Active terminal producer check.
        @return None.
        """
        proof = self.geometry["native_proof"]
        native_log, binary = ROOT / proof["log"], ROOT / proof["binary"]
        output = native_log.read_text(encoding="utf-8-sig")
        original_read = Path.read_text
        marker = next(line for line in output.splitlines() if line.startswith("TERMINAL_ART_GEOMETRY "))
        candidates = (output.replace("NATIVE_EXIT_CODE=0", "NATIVE_EXIT_CODE=3"),
                      output.replace("in 5 test cases", "in 4 test cases"),
                      output.replace(marker + "\n", "", 1), marker + "\n" + output)
        # Mock only malformed native-log reads and final writes. Real current
        # source/binary identities still bind the candidate proof and refusal.
        for candidate in candidates:
            def read_native_or_source(path, *args, **kwargs):
                """Substitute one invalid native log while preserving real owner-source reads.

                @param path Requested native log or current native source path.
                @param args Original reader positional options.
                @param kwargs Original reader keyword options.
                @return Candidate log text or unchanged source content.
                """
                return candidate if path == native_log else original_read(path, *args, **kwargs)

            with mock.patch.object(Path, "read_text", autospec=True, side_effect=read_native_or_source), mock.patch.object(Path, "write_text") as write:
                with self.assertRaises(ValueError):
                    terminal.publish_geometry(native_log, binary)
                write.assert_not_called()

    def test_only_terminal_public_row_and_new_test_binary_change(self):
        """Preserve all144 other public families and every protected station/fleet/native artifact.

        @param self Active terminal producer check.
        @return None.
        """
        before = json.loads((ROOT / "logs/graphics/road-terminal/20261012-geometry-01/before.json").read_text(encoding="utf-8"))
        # The native test executable alone includes the new producer. Its
        # actual new identity belongs to this group's proof; shipped payload,
        # three other binaries and completed source artifacts stay exact.
        for item in before["before_identities"]:
            if item["path"] in before["allowed_build_identity_change"]:
                continue
            self.assertEqual(terminal.digest(ROOT / item["path"]), item["sha256"], item["path"])
        for row in self.document["assets"]:
            if row["asset_id"] == "facility.road-terminal":
                continue
            # Later27B intentionally replaces only selected native mine draft
            # rotations; require its complete current contract while keeping
            # every other prior family identity and all packed artifacts exact.
            if row["asset_id"] in {f"mine.{resource}" for resource in shared.SELECTED_MINES}:
                shared.validate_mine_contract(row)
                continue
            self.assertEqual(hashlib.sha256(json.dumps(row, sort_keys=True).encode()).hexdigest(),
                             before["public_family_sha256"][row["asset_id"]], row["asset_id"])


if __name__ == "__main__":
    unittest.main()
