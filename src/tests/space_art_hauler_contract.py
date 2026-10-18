"""Verify quantity-preserving hauler source contracts against live native owners.

@file space_art_hauler_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group66B.1
These source/metadata checks consume accepted native producer definitions.
They do not execute gameplay, allocate sprites or claim compiled/runtime art.
"""
import copy
import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art
from tools.orbital_art_contract import hauler_contract, validate_orbital_contract

ASSET = "vehicle.model-17"
ORIGINAL_STATES = ["empty", "partial", "full", "moving-empty", "moving-partial", "moving-full", "service", "blocked"]


def native_function(relative, signature):
    """Read one complete live native producer body for source-contract reconciliation.

    @param relative Repository-relative native implementation file.
    @param signature Exact delivered definition marker, without relying on stale line ranges.
    @return Complete function body including nested initialization/switch blocks; no native execution claim.
    @throws ValueError On an absent or incomplete named producer definition.
    """
    text = (ROOT / relative).read_text(encoding="utf-8")
    start = text.index("{", text.index(signature))
    depth = 0
    # Native service admission includes an indirect miner table and nested initializer/switch blocks. Stop only at the complete named producer boundary so an inner brace cannot conceal another admitted model.
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
        if depth == 0:
            return text[start + 1:index]
    raise ValueError("incomplete native source producer")


class HaulerQuantityContractTests(unittest.TestCase):
    """Protect original identities, actual freight appearances and service admission."""

    def setUp(self):
        """Read an independent public row and the selected original body definitions.

        @param self Detached native/source contract fixture.
        """
        public = json.loads((ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
        self.row = next(row for row in public["assets"] if row["asset_id"] == ASSET)
        self.record = art.read_record()
        self.family = self.record["families"][ASSET]

    def test_original_eight_state_ordinals_and_geometry_are_preserved(self):
        """New loaded-blocked pictures append without reinterpreting existing art identities.

        @param self Public/native ordinal and geometry fixture.
        """
        validate_orbital_contract(self.row)
        self.assertEqual(self.row["states"][:8], ORIGINAL_STATES)
        self.assertEqual(self.row["states"][8:], ["blocked-partial", "blocked-full"])
        self.assertEqual(self.row["native_model_id"], 17)
        self.assertEqual(self.row["native_size"], [64, 48])
        self.assertEqual(self.row["anchor"], [-32, -32])
        self.assertEqual(len(self.row["states"]) * len(self.row["directions"]) * self.row["frames"], 160)

    def test_each_blocked_quantity_has_an_actual_static_source_slot(self):
        """A loaded blocked barge keeps its cargo cue and cannot retain travel exhaust.

        @param self Empty/partial/full blocked appearance fixture.
        """
        # Every possible physical load keeps its own stopped interlock appearance; a generic empty blocked hull cannot erase real freight.
        for state in ("blocked", "blocked-partial", "blocked-full"):
            self.assertIn(state, self.row["states"])
            self.assertEqual(self.row["source_frame_contract"][state], [0, 0])
        self.assertIn("GetSpaceFreightCapacity", self.row["native_load_owner"])

    def test_unreachable_service_alias_matches_complete_native_admission(self):
        """Common supply compartments do not grant hauler17 a paid repair producer.

        @param self Live source-owner admission and alias fixture; compiled execution remains a later gate.
        """
        mining = native_function("src/space_mining_tuning.cpp", "GetSpaceMiningTuning(uint16_t model)")
        service = native_function("src/space_manifest.cpp", "GetSpaceServiceTuning(uint16_t model)")
        ready = native_function("src/orbital_service.cpp", "static bool ReadyCraft(")
        self.assertEqual(set(re.findall(r"case\s+([A-Za-z_0-9]+)\s*:", mining)), {"15", "SPACE_HEAVY_MINER_MODEL"})
        header = (ROOT / "src/space_mining_tuning.h").read_text(encoding="utf-8")
        heavy_model = int(re.search(r"SPACE_HEAVY_MINER_MODEL\s*=\s*(\d+)", header).group(1))
        self.assertNotEqual(heavy_model, 17)
        self.assertEqual(len(re.findall(r"\breturn\b", mining)), 3)
        self.assertIn("default: return nullptr;", mining)
        self.assertIn("if (const auto *miner = GetSpaceMiningTuning(model))", service)
        self.assertRegex(service, r"if\s*\(model == 19 \|\| model == 20\)\s*return SpaceServiceTuning")
        self.assertEqual(len(re.findall(r"\breturn\b", service)), 3)
        self.assertIn("return std::nullopt;", service)
        self.assertIn("GetSpaceServiceTuning(model).has_value()", ready)
        self.assertEqual(self.row["source_state_aliases"], {"service": "empty"})
        self.assertEqual(self.row["source_frame_contract"]["service"], [0, 0])

    def test_service_animation_cannot_be_reintroduced_by_a_draft(self):
        """An authored phase flag cannot invent native17repair work or a service clock.

        @param self Invalid draft source-service phase fixture.
        """
        self.row["source_frame_contract"]["service"] = [0, 1]
        with self.assertRaisesRegex(ValueError, "native orbital geometry/role contract changed"):
            validate_orbital_contract(self.row)

    def test_loaded_blocked_cannot_alias_an_empty_hull(self):
        """A fresh metadata alias cannot discard a positive actual cargo quantity.

        @param self Incorrect loaded-source alias fixture.
        """
        self.row["source_state_aliases"]["blocked-full"] = "empty"
        with self.assertRaisesRegex(ValueError, "native orbital geometry/role contract changed"):
            validate_orbital_contract(self.row)

    def test_all_eight_original_bodies_match_the_approved_geometry_review(self):
        """Quantity reconciliation leaves every selected source pixel/world transform intact.

        @param self Immutable eight-camera original identity and registration fixture.
        """
        path = ROOT / self.family["base_heading_review"]["path"]
        self.assertEqual(art.identity(path), self.family["base_heading_review"])
        report = json.loads(path.read_text(encoding="utf-8"))
        # The earlier accepted body review freezes each actual raw original and measured registration. Only state/alias metadata may advance in this group.
        for view in report["views"]:
            direction = view["direction"]
            candidate = art.source_candidate(self.family["selections"]["free/" + direction], self.record)
            self.assertEqual(art.identity(ROOT / candidate["path"]), view["original"])
            self.assertEqual(candidate["native_transforms"][direction], view["transform"])
            self.assertEqual(art.registration_signature(candidate, direction), self.family["approved_base_registration"][direction])

    def test_missing_loaded_blocked_camera_state_refuses(self):
        """A quantity state cannot disappear to preserve an obsolete smaller coverage count.

        @param self Incomplete source-matrix contract fixture.
        """
        self.row["states"].remove("blocked-partial")
        with self.assertRaisesRegex(ValueError, "native orbital geometry/role contract changed"):
            validate_orbital_contract(self.row)

    def test_other_role_or_missing_service_alias_refuses(self):
        """General freight cannot borrow a miner/constructor identity or an implicit alias.

        @param self Independently malformed native source-owner fixtures.
        """
        # Independent corruptions must each refuse; a correct pose count cannot conceal a changed model identity or missing unsupported-service declaration.
        for field, value in (("native_model_id", 16), ("native_role", "heavy_mining_robot"), ("source_state_aliases", {})):
            row = copy.deepcopy(self.row)
            row[field] = value
            with self.assertRaisesRegex(ValueError, "native orbital geometry/role contract changed"):
                validate_orbital_contract(row)


if __name__ == "__main__":
    unittest.main()
