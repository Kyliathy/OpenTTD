"""Focused native-to-source station handoff and false-evidence refusals.

@file conventional_station_contract_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No original raster is generated and no packed sprite or world is modified.
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
from tools import ground_station_contract as station
import multilayer_sprite_contract as shared


class ConventionalStationContractChecks(unittest.TestCase):
    """Keep all legal native cells, independent sorting and actual state owners in the source contract."""

    @classmethod
    def setUpClass(cls):
        """Read the public native producer and shared manifest once for this focused burst.

        @param cls Test class retaining immutable native/public handoff inputs.
        @return None.
        """
        cls.geometry = station.load_geometry()
        cls.document = json.loads(shared.MANIFEST.read_text(encoding="utf-8"))
        cls.row = next(row for row in cls.document["assets"] if row["asset_id"] == "facility.rail-station")

    def test_compiled_geometry_covers_actual_native_parents(self):
        """Cover every native layout and preserve the unusual Y rear-pillar height.

        @param self Active focused check instance.
        @return None.
        """
        self.assertEqual([row["graphics"] for row in self.geometry["layouts"]], list(range(8)))
        self.assertEqual(self.geometry["layouts"][4]["parts"][0]["extent"][2], 7)
        self.assertEqual(self.geometry["layouts"][5]["parts"][0]["extent"][2], 2)
        self.assertEqual(self.geometry["native_proof"]["test_cases"], 4)
        # Native terminal25B replaces its draft512 slots with288 actual
        # bay/through slots; all other public family requirements stay exact.
        self.assertEqual(shared.validate_manifest(self.document), 48893)

    def test_source_matrix_covers_all_cells_and_true_roof_absences(self):
        """Map all eight graphics to512 slots, with128 absent roof/glass slots.

        @param self Active focused check instance.
        @return None.
        """
        station.validate_station_contract(self.row)
        slots = sum(len(self.row["parts"]) * self.row["frames"] * len(self.row["states"])
                    for _ in self.geometry["layouts"])
        absent = sum((sum(part is None for part in layout["parts"]) + int(layout["roof_glass"] is None)) * self.row["frames"] * len(self.row["states"])
                     for layout in self.geometry["layouts"])
        self.assertEqual((slots, absent, slots - absent), (512, 128, 384))
        rebuilt = next(row for row in shared.build_manifest()["assets"] if row["asset_id"] == "facility.rail-station")
        station.validate_station_contract(rebuilt)

    def test_fixed_building_and_directed_door_drafts_refuse(self):
        """Reject stale rectangles and extra view labels that omit legal modular platforms.

        @param self Active focused check instance.
        @return None.
        """
        # Each old draft field independently changes physical source coverage;
        # exact native validation must refuse before selecting or packing art.
        for key, value in (("footprint", [3, 2]), ("tile_variants", 6), ("directions", shared.DOORS),
                           ("native_graphics", {"NE": [0, 2], "SE": [1, 3]})):
            with self.subTest(field=key):
                row = copy.deepcopy(self.row)
                row[key] = value
                with self.assertRaises(ValueError):
                    station.validate_station_contract(row)

    def test_roof_cannot_merge_into_low_foreground_or_erase_canopies(self):
        """Preserve independent elevated parents and refuse invented roof absence on canopy roles.

        @param self Active focused check instance.
        @return None.
        """
        row = copy.deepcopy(self.row)
        row["parts"] = ["back", "front"]
        with self.assertRaises(ValueError):
            station.validate_station_contract(row)
        row = copy.deepcopy(self.row)
        row["nullable_parts"]["roof"].append(2)
        with self.assertRaises(ValueError):
            station.validate_station_contract(row)
        geometry = copy.deepcopy(self.geometry)
        geometry["layouts"][4]["parts"][2]["origin"][2] = 0
        with self.assertRaisesRegex(ValueError, "elevated"):
            station.validate_geometry(geometry)

    def test_glass_retains_native_child_palette_attachment_and_source_slot(self):
        """A native transparent child cannot become opaque roof art or another sorting parent.

        @param self Active focused check instance.
        @return None.
        """
        # Native glass offsets, transparency and palette must match compiled
        # drawing behavior. Losing any fact can obscure trains or show stock
        # glass in the wrong place even when the structural roof looks plausible.
        for key, value in (("screen_offset", [1, 0]), ("transparent", False), ("palette", "PAL_NONE")):
            with self.subTest(child_field=key):
                geometry = copy.deepcopy(self.geometry)
                geometry["layouts"][4]["roof_glass"][key] = value
                with self.assertRaisesRegex(ValueError, "transparent roof-child"):
                    station.validate_geometry(geometry)
        row = copy.deepcopy(self.row)
        row["parts"] = ["back", "front", "roof"]
        with self.assertRaises(ValueError):
            station.validate_station_contract(row)
        row = copy.deepcopy(self.row)
        row["native_child_parts"]["glass"]["parent"] = "front"
        with self.assertRaises(ValueError):
            station.validate_station_contract(row)
        row = copy.deepcopy(self.row)
        row["nullable_parts"]["glass"].append(2)
        with self.assertRaises(ValueError):
            station.validate_station_contract(row)

    def test_native_image_origins_preserve_one_common_source_canvas(self):
        """Parent and relative child image offsets must not shift platforms or elevate roofs twice.

        @param self Active focused source contract check.
        @return None.
        """
        anchors = self.row["native_packed_anchors"]
        self.assertEqual(anchors["0"]["front"], [-53, -44])
        self.assertEqual(anchors["1"]["front"], [-9, -44])
        self.assertEqual(anchors["4"]["roof"], [-31, -17])
        self.assertEqual(anchors["4"]["glass"], [0, 0])
        self.assertIsNone(anchors["0"]["glass"])
        # Compose the renderer's actual origin/offset projection with each
        # packed image anchor. Every parent must land on the same source-frame
        # top-left; glass is then relative to that roof image's left/top.
        for layout in self.geometry["layouts"]:
            for part, bounds in zip(("back", "front", "roof"), layout["parts"]):
                if bounds is None:
                    continue
                x, y, z = [bounds["origin"][index] + bounds["offset"][index] for index in range(3)]
                anchor = anchors[str(layout["graphics"])][part]
                self.assertEqual([2 * (y - x) + anchor[0], x + y - z + anchor[1]], [-31, -33])
            if layout["roof_glass"] is not None:
                child = anchors[str(layout["graphics"])]["glass"]
                self.assertEqual([-31 + child[0], -33 + child[1]], [-31, -33])

    def test_double_applied_parent_or_child_source_anchor_refuses(self):
        """A source-canvas anchor cannot replace actual native front/roof/child packing offsets.

        @param self Active focused source contract check.
        @return None.
        """
        # These superficially familiar offsets shift a front strip, lift the
        # roof twice or displace glass away from its original structural frame.
        for graphics, part in (("0", "front"), ("4", "roof"), ("4", "glass")):
            row = copy.deepcopy(self.row)
            row["native_packed_anchors"][graphics][part] = [-31, -33]
            with self.assertRaises(ValueError):
                station.validate_station_contract(row)
        row = copy.deepcopy(self.row)
        row["native_anchor_renderer_sha256"]["src/viewport.cpp"] = "0" * 64
        with self.assertRaises(ValueError):
            station.validate_station_contract(row)

    def test_atomic_preview_states_cannot_become_world_states(self):
        """Refuse fictitious construction/blocked owners and loading inferred from an art clock.

        @param self Active focused check instance.
        @return None.
        """
        # Native commands build atomically. Preview states cannot create an
        # installed work controller, and source animation cannot create loading.
        for key, value in (("world_states", ["construction", "ready", "working", "blocked"]),
                           ("blocked", "Persistent default station Blocked flag"),
                           ("working", "An independent sprite timer creates cargo activity")):
            with self.subTest(policy=key):
                row = copy.deepcopy(self.row)
                row["native_state_policy"][key] = value
                with self.assertRaises(ValueError):
                    station.validate_station_contract(row)

    def test_changed_native_owner_or_incomplete_geometry_requires_new_proof(self):
        """A stale renderer/approach owner or missing native view cannot authorize source work.

        @param self Active focused check instance.
        @return None.
        """
        geometry = copy.deepcopy(self.geometry)
        geometry["native_source_sha256"]["src/excavation_guard.cpp"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "owner changed"):
            station.validate_geometry(geometry)
        geometry = copy.deepcopy(self.geometry)
        geometry["layouts"].pop()
        with self.assertRaisesRegex(ValueError, "eight default"):
            station.validate_geometry(geometry)
        geometry = copy.deepcopy(self.geometry)
        geometry["native_proof"]["exit_code"] = 3
        with self.assertRaisesRegex(ValueError, "successful native proof"):
            station.validate_geometry(geometry)

    def test_false_native_evidence_never_overwrites_the_checked_descriptor(self):
        """Reject failed, partial and duplicate records before touching the public descriptor.

        @param self Active focused check instance.
        @return None.
        """
        proof = self.geometry["native_proof"]
        native_log, binary = ROOT / proof["log"], ROOT / proof["binary"]
        output = native_log.read_text(encoding="utf-8-sig")
        original_read = Path.read_text
        marker = next(line for line in output.splitlines() if line.startswith("STATION_ART_GEOMETRY "))
        candidates = (output.replace("NATIVE_EXIT_CODE=0", "NATIVE_EXIT_CODE=3"),
                      output.replace("in 4 test cases", "in 3 test cases"),
                      output.replace(marker + "\n", "", 1), marker + "\n" + output)
        # Mock only the log read and final write. Native sources and executable
        # hashes remain real, so a failed evidence gate must refuse publication.
        for candidate in candidates:
            with self.subTest(candidate_length=len(candidate)):
                def read_native_or_source(path, *args, **kwargs):
                    """Substitute only the negative native log while retaining real source hashes.

                    @param path Requested native log or actual source path.
                    @param args Original text-reader positional options.
                    @param kwargs Original text-reader keyword options.
                    @return Candidate log text or the unmodified native source.
                    """
                    return candidate if path == native_log else original_read(path, *args, **kwargs)

                with mock.patch.object(Path, "read_text", autospec=True, side_effect=read_native_or_source), mock.patch.object(Path, "write_text") as write:
                    with self.assertRaises(ValueError):
                        station.publish_geometry(native_log, binary)
                    write.assert_not_called()

    def test_station_reconciliation_preserves_all_other_public_families(self):
        """Verify the active group's exact shared-schema scope and all existing native ranges.

        @param self Active focused check instance.
        @return None.
        """
        audit = json.loads((ROOT / "logs/graphics/rail-station/20261011-geometry-01/schema-reconciliation.json").read_text(encoding="utf-8"))
        self.assertEqual(audit["changed_families"], ["facility.rail-station"])
        # Every unrelated public record, including all packed routes and
        # completed vehicles, must retain its exact canonical JSON identity.
        for row in self.document["assets"]:
            if row["asset_id"] == "facility.rail-station":
                continue
            if row["asset_id"] == "facility.road-terminal":
                # The later25B producer intentionally supersedes the old
                # terminal draft recorded during station24B. Require its
                # complete current native contract instead of freezing that
                # invalid2x2 rectangle as an unrelated immutable source.
                shared.validate_terminal_contract(row)
                continue
            # Later27B reconciles selected mine drafts through compiled actual
            # geometry. Preserve that complete owner instead of freezing the
            # earlier fictitious four-view mine matrix as immutable source.
            if row["asset_id"] in {f"mine.{resource}" for resource in shared.SELECTED_MINES}:
                shared.validate_mine_contract(row)
                continue
            identity = hashlib.sha256(json.dumps(row, sort_keys=True).encode()).hexdigest()
            self.assertEqual(identity, audit["unchanged_family_sha256"][row["asset_id"]])


if __name__ == "__main__":
    unittest.main()
