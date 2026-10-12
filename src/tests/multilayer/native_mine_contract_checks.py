"""Focused native-mine source geometry, state ownership and false-proof refusals.

@file native_mine_contract_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group27B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Compiled actual definitions authorize source geometry only. Sprite activation,
geological work, physical output and unselected families retain their owners.
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
from tools import ground_mine_contract as mine
import multilayer_sprite_contract as shared


class NativeMineContractChecks(unittest.TestCase):
    """Require exact native catalogue topology, actual state owners and protected prior deliveries."""

    @classmethod
    def setUpClass(cls):
        """Load the active native producer, shared contract and before identities once.

        @param cls Test class retaining complete native and public source evidence.
        @return None.
        """
        cls.geometry = mine.load_geometry()
        cls.document = json.loads(shared.MANIFEST.read_text(encoding="utf-8"))
        cls.rows = [row for row in cls.document["assets"] if row["asset_id"] in {f"mine.{resource}" for resource in mine.SELECTED}]
        cls.before = json.loads((ROOT / "logs/graphics/mine-source/20261012-geometry-01/before.json").read_text(encoding="utf-8"))

    def test_complete_successful_actual_native_catalogue_and_state_proof(self):
        """Ten installed definitions bind the real successful six-case native funding/state run.

        @param self Active native source producer proof.
        @return None.
        """
        self.assertEqual([row["resource"] for row in self.geometry["families"]], list(mine.RESOURCES))
        proof = self.geometry["native_proof"]
        self.assertEqual(proof["test_cases"], 6)
        self.assertEqual(proof["assertions"], 2715)
        self.assertEqual(proof["exit_code"], 0)
        self.assertEqual(mine.digest(ROOT / proof["binary"]), proof["binary_sha256"])
        self.assertEqual(mine.digest(ROOT / proof["log"]), proof["log_sha256"])
        output = (ROOT / proof["log"]).read_text(encoding="utf-8-sig")
        self.assertEqual(output.count("MINE_ART_GEOMETRY "), 10)
        self.assertTrue(output.rstrip().endswith("NATIVE_EXIT_CODE=0"))

    def test_selected_source_matrices_have_one_real_layout_and_nine_actual_cells(self):
        """Seven selected sources need288 slots each without invented four-way mine orientations.

        @param self Active native source producer proof.
        @return None.
        """
        self.assertEqual(len(self.rows), 7)
        # Subsequent compiled39B fixed Surface layouts remove only their draft
        # rotations; the seven mine matrices still require288 parts apiece.
        self.assertEqual(shared.validate_manifest(self.document), 42749)
        rebuilt = {row["asset_id"]: row for row in shared.build_manifest()["assets"]}
        # Real native coordinates define every source variant. The separate
        # loading floor cannot become extra industry art or drop out of funding.
        for row in self.rows:
            mine.validate_mine_contract(row)
            mine.validate_mine_contract(rebuilt[row["asset_id"]])
            self.assertEqual(row["directions"], ["none"])
            self.assertEqual(row["native_layout"], 0)
            self.assertEqual(row["native_cells"], [[x, y] for y in range(3) for x in range(3)])
            self.assertEqual(row["loading_cells"], [[0, 3], [1, 3], [0, 4], [1, 4]])
            self.assertFalse({tuple(cell) for cell in row["native_cells"]} & {tuple(cell) for cell in row["loading_cells"]})
            slots = len(row["directions"]) * len(row["states"]) * row["frames"] * row["tile_variants"] * len(row["parts"])
            self.assertEqual(slots, 288)

    def test_native_construction_and_finite_owner_presentation_remain_distinct(self):
        """Actual stages, real output capacity and permanent depletion cannot become art-local work.

        @param self Active native source producer proof.
        @return None.
        """
        policy = self.rows[0]["native_state_policy"]
        self.assertEqual(policy["world_states"], ["construction", "working", "blocked", "depleted"])
        self.assertEqual(policy["frame_aliases"], {"construction": [0, 1, 2, 3], "working": [0, 1, 2, 3], "blocked": [0, 0, 0, 0], "depleted": [0, 0, 0, 0]})
        self.assertIn("GetIndustryConstructionStage", policy["construction"])
        self.assertIn("GetLayerMineOperationalStatus", policy["working"])
        self.assertIn("Industry output", policy["working"])
        self.assertIn("remaining_reserve", policy["depleted"])
        self.assertIn("TimerGameTick::counter", policy["frame_owner"])
        with self.assertRaisesRegex(ValueError, "outside"):
            mine.mine_contract("BAUX")

    def test_draft_rotations_missing_cells_and_copied_state_owners_refuse(self):
        """No old draft topology or new quantity authority can authorize selected mine originals.

        @param self Active native source producer proof.
        @return None.
        """
        for field, value in (("directions", ["NE", "SE", "SW", "NW"]), ("footprint", [2, 2]),
                             ("native_layout", 1), ("native_cells", [[0, 0]]), ("loading_cells", []),
                             ("parts", ["back"]), ("source_height_limit", 60)):
            row = copy.deepcopy(self.rows[0])
            row[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                mine.validate_mine_contract(row)
        for field, value in (("working", "Sprite-local stock creates coal"), ("construction", "Free instant excavation opens the shaft"),
                             ("frame_aliases", {state: [0, 1, 2, 3] for state in self.rows[0]["states"]})):
            row = copy.deepcopy(self.rows[0])
            row["native_state_policy"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                mine.validate_mine_contract(row)

    def test_stale_partial_duplicate_and_foreign_native_geometry_refuse(self):
        """Checked catalogue descriptors preserve every actual native coordinate and identity.

        @param self Active native source producer proof.
        @return None.
        """
        for change in ("owner", "partial", "duplicate", "orientation", "coordinate", "tile_owner", "loading", "failed"):
            document = copy.deepcopy(self.geometry)
            if change == "owner":
                document["native_source_sha256"]["src/layer_industry_definitions.cpp"] = "0" * 64
            elif change == "partial":
                document["families"].pop()
            elif change == "duplicate":
                document["families"][-1] = copy.deepcopy(document["families"][0])
            elif change == "orientation":
                document["families"][0]["layout_count"] = 4
            elif change == "coordinate":
                document["families"][0]["cells"][-1]["cell"] = [0, 0]
            elif change == "tile_owner":
                document["families"][0]["cells"][0]["tile_local_id"] = 1
            elif change == "loading":
                document["families"][0]["loading"][-1] = [2, 2]
            else:
                document["native_proof"]["exit_code"] = 3
            with self.subTest(change=change), self.assertRaises(ValueError):
                mine.validate_geometry(document)

    def test_false_native_logs_cannot_replace_published_geometry(self):
        """Complete actual successful traces are mandatory before publication starts.

        @param self Active native source producer proof.
        @return None.
        """
        proof = self.geometry["native_proof"]
        native_log, binary = ROOT / proof["log"], ROOT / proof["binary"]
        output = native_log.read_text(encoding="utf-8-sig")
        marker = next(line for line in output.splitlines() if line.startswith("MINE_ART_GEOMETRY "))
        candidates = (output.replace("NATIVE_EXIT_CODE=0", "NATIVE_EXIT_CODE=3"),
                      output.replace("in 6 test cases", "in 5 test cases"),
                      output.replace(marker + "\n", "", 1), marker + "\n" + output)
        original_read = Path.read_text
        # Replace only malformed input reads. Real current native source and
        # binary identities still participate; every final write is observed.
        for candidate in candidates:
            def read_native_or_source(path, *args, **kwargs):
                """Substitute one invalid native log while preserving actual native source reads.

                @param path Requested native log or current native source path.
                @param args Original reader positional options.
                @param kwargs Original reader keyword options.
                @return Candidate log text or exact unchanged source content.
                """
                return candidate if path == native_log else original_read(path, *args, **kwargs)

            with mock.patch.object(Path, "read_text", autospec=True, side_effect=read_native_or_source), mock.patch.object(Path, "write_text") as write:
                with self.assertRaises(ValueError):
                    mine.publish_geometry(native_log, binary)
                write.assert_not_called()

    def test_only_selected_drafts_and_native_test_executable_change(self):
        """All138 other families and140 protected previous artifacts remain exact.

        @param self Active native source producer proof.
        @return None.
        """
        # The changed Debug test executable contains the new geometry trace and
        # refusal fixtures. All source pixels, payloads and other binaries stay
        # exact; only seven explicitly dispatched mine draft records change.
        for row in self.before["before_identities"]:
            if row["path"] in self.before["allowed_build_identity_change"]:
                continue
            self.assertEqual(mine.digest(ROOT / row["path"]), row["sha256"], row["path"])
        changed = []
        for row in self.document["assets"]:
            identity = hashlib.sha256(json.dumps(row, sort_keys=True).encode()).hexdigest()
            if identity == self.before["public_family_sha256"][row["asset_id"]]:
                continue
            changed.append(row["asset_id"])
            mine.validate_mine_contract(row)
            self.assertIsNone(row["packed"]["offset"])
            self.assertFalse(row["authoring"]["runtime_activated"])
        self.assertEqual(changed, [f"mine.{resource}" for resource in mine.SELECTED])


if __name__ == "__main__":
    unittest.main()
