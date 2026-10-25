"""Verify the merged layout against frozen original ranges without publishing art.

@file merge_sprite_layout_checks.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Generated header candidates and the report live in test-cache/merge-assets/group49.
No original, public manifest, source header or canonical image is written.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "media/multilayer"), str(ROOT / "src/tests/multilayer/support")]
from PIL import Image
import pack_m1_art as m1
import pack_space_art as space
import pack_orbital_art as orbital
import pack_launch_fleet_art as launch
import pack_ground_art as ground


class MergedSpriteLayoutChecks(unittest.TestCase):
    """Preserve every shipped family, reserved gap, signed origin and native limit."""

    @classmethod
    def setUpClass(cls):
        """Load the unchanged original-derived fixture and current packing owners.

        @param cls Focused layout qualification class.
        """
        cls.public = json.loads(m1.PUBLIC.read_text(encoding="utf-8"))
        cls.families = {row["asset_id"]: row for row in cls.public["assets"]}
        cls.frozen = json.loads((ROOT / "src/tests/fixtures/graphics/merge-asset-families.json").read_text(encoding="utf-8"))
        cls.scopes = {name: json.loads((ROOT / f"media/multilayer/{name}-pack-scope.json").read_text(encoding="utf-8"))
                      for name in ("m1", "space", "orbital", "launch", "ground")}
        cls.output = ROOT / "test-cache/merge-assets/group49"
        cls.output.mkdir(parents=True, exist_ok=True)

    def test_complete_original_occupied_set_and_no_donor_append(self):
        """Retain all original selected/unselected bindings and reserve the original preview gap.

        @param self Active original-derived occupied-set comparison.
        """
        frozen = {row["asset_id"]: row for row in self.frozen["families"]}
        owners = {}
        # Inspect every public binding, even if its family is outside the current
        # selection; an omission cannot release or recycle a published number.
        for family in self.public["assets"]:
            self.assertEqual(family["packed"], frozen[family["asset_id"]]["A_binding"])
            offset = family["packed"].get("native_offset")
            if offset is None:
                continue
            count = family["packed"]["sprite_count"]
            self.assertEqual(count, len(m1.required_keys(family)))
            self.assertLessEqual(offset + count, 65535)
            for index in range(offset, offset + count):
                self.assertNotIn(index, owners)
                owners[index] = family["asset_id"]
        # The existing core scope owns its unselected holes. Space's protected
        # prefix includes the independent eleven-sprite preview, not free slots.
        core = self.public["core_extension"]["planned_end_exclusive"]
        ranges, m1_end = m1.selected_ranges(self.public, self.scopes["m1"])
        occupied = set(owners) | set(range(core)) | set(range(m1_end, self.scopes["space"]["protected_native_end_exclusive"]))
        self.assertEqual((m1_end, self.scopes["space"]["native_start"]), (3588, 3599))
        self.assertEqual(occupied, set(range(22045)))
        self.assertFalse(any(row["disposition"] == "donor_only_append" for row in self.frozen["families"]))
        self.assertEqual(len(ranges), 43)

    def test_existing_header_candidates_match_every_scope_and_repeat(self):
        """Regenerate every typed range header through its sole existing owner.

        @param self Active current header-candidate comparison, without publication.
        """
        candidates = []
        # Scope order is the published native order. Public JSON declaration
        # order cannot transpose family enum values or relocate selected poses.
        for name, producer in (("m1", m1), ("space", space), ("orbital", orbital), ("launch", launch), ("ground", ground)):
            scope = self.scopes[name]
            rows = []
            for asset in scope["asset_ids"]:
                family = self.families[asset]
                rows.append({"asset_id": asset, "native_offset": family["packed"]["native_offset"],
                             "sprite_count": family["packed"]["sprite_count"],
                             **{key: family[key] for key in ("states", "frames", "directions", "tile_variants", "parts", "native_size", "anchor")}})
            # M1's preserved range order predates its source selection order;
            # its existing selector, rather than a public reorder, owns enum IDs.
            if name == "m1":
                rows, _ = m1.selected_ranges(self.public, scope)
            bound = max(row["native_offset"] + row["sprite_count"] for row in rows)
            args = (rows, scope["native_start"], bound) if name == "ground" else (rows, bound)
            candidate = producer.format_header(*args)
            self.assertEqual(candidate, producer.format_header(*args))
            self.assertEqual(candidate.decode("utf-8"), (ROOT / scope["header"]).read_text(encoding="utf-8"))
            target = self.output / Path(scope["header"]).name
            target.write_bytes(candidate)
            candidates.append({"path": target.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(candidate).hexdigest()})
        (self.output / "header-candidates.json").write_text(json.dumps({"_documentation": __doc__, "headers": candidates}, indent=2) + "\n", encoding="utf-8")

    def test_overlap_omitted_family_and_changed_extent_refuse(self):
        """Refuse copied metadata that reuses a published slot or changes its complete matrix.

        @param self Active focused range-refusal check.
        """
        scope, public = copy.deepcopy(self.scopes["m1"]), copy.deepcopy(self.public)
        appended = [row for row in scope["preserved_ranges"] if row["native_offset"] >= scope["appended_start"]]
        first, second = appended[:2]
        second["native_offset"] = first["native_offset"]
        next(row for row in public["assets"] if row["asset_id"] == second["asset_id"])["packed"]["native_offset"] = first["native_offset"]
        with self.assertRaisesRegex(ValueError, "overlapping"):
            m1.selected_ranges(public, scope)
        scope = copy.deepcopy(self.scopes["m1"]); scope["asset_ids"].remove(scope["preserved_ranges"][0]["asset_id"])
        with self.assertRaisesRegex(ValueError, "preserved"):
            m1.selected_ranges(self.public, scope)
        public = copy.deepcopy(self.public); public["assets"][0]["frames"] += 1
        with self.assertRaisesRegex(ValueError, "range changed"):
            m1.selected_ranges(public, self.scopes["m1"])

    def test_repeat_order_and_sealed_prefix_refuse_preview_reuse(self):
        """Keep public reorder harmless and stop fresh M1 allocation inside later shipped scopes.

        @param self Active stable-order and cross-scope preview-gap check.
        """
        public = copy.deepcopy(self.public); public["assets"].reverse()
        self.assertEqual(m1.selected_ranges(public, self.scopes["m1"]), m1.selected_ranges(self.public, self.scopes["m1"]))
        scope = copy.deepcopy(self.scopes["m1"])
        family = copy.deepcopy(self.families[scope["asset_ids"][-1]])
        family["asset_id"] = "fixture.fresh-family"
        family["packed"].pop("native_offset"); family["packed"].pop("sprite_count")
        public["assets"].append(family); scope["asset_ids"].append(family["asset_id"])
        with self.assertRaisesRegex(ValueError, "sealed published prefix"):
            m1.selected_ranges(public, scope)

    def test_action5_limit_exhaustion_without_later_scopes(self):
        """Exercise the existing U16 guard in an isolated hypothetical original-prefix owner.

        @param self Active limit refusal; no new real family is declared.
        """
        scope = copy.deepcopy(self.scopes["m1"]); public = copy.deepcopy(self.public)
        public["assets"] = [family for family in public["assets"] if family["asset_id"] in scope["asset_ids"]]
        family = copy.deepcopy(public["assets"][-1]); family["asset_id"] = "fixture.exhausted-family"
        family.update(states=["ready"], frames=1, directions=["none"], tile_variants=32768, parts=["back", "front"])
        family["packed"].pop("native_offset"); family["packed"].pop("sprite_count")
        public["assets"].append(family); scope["asset_ids"].append(family["asset_id"])
        with self.assertRaisesRegex(ValueError, "Action 5"):
            m1.selected_ranges(public, scope)

    def test_signed_parent_and_relative_roof_origins_remain_exact(self):
        """Use independently authored indexed pixels to expose signed-origin truncation or child drift.

        @param self Active tiny native anchor/relative-child crop check.
        """
        image = Image.new("P", (4, 4), 0); image.putpixel((1, 2), 1)
        key = ("ready", 0, "NE", 0, "back")
        pose = {"box": [0, 0, 4, 4], "anchor": [-31, -33], "native_packed_anchor": [-13, -9], "structurally_absent": False}
        family = {"asset_id": "facility.rail-station"}
        self.assertEqual(ground.native_pose(family, key, {key: pose}, image), ([1], 1, 1, -12, -7))
        roof, glass = (*key[:4], "roof"), (*key[:4], "glass")
        pixels, width, height, x, y = ground.native_pose(family, roof, {roof: pose, glass: {"structurally_absent": False}}, image)
        self.assertEqual((width, height, x, y), (4, 4, -13, -9)); self.assertEqual(pixels[9], 1)
        invalid = {**pose, "native_packed_anchor": [32768, -9]}
        with self.assertRaisesRegex(ValueError, "signed native anchor"):
            ground.native_pose(family, key, {key: invalid}, image)


if __name__ == "__main__":
    unittest.main(verbosity=2)
