"""Focused native orbital source-geometry and identity drift/refusal checks.

@file orbital_art_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13B
Fixtures compare planned graphics with actual native definition/role owners.
They create no raster, sprite allocation, physical inventory or gameplay state.
"""
import copy
import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
from tools.orbital_art_contract import MODULE_LAYOUTS, validate_orbital_contract


def family(asset):
    """Read one explicit current public source contract for an isolated check.

    @param asset Stable public graphics family identity.
    @return Independently decoded selected row; mutation cannot affect another case.
    """
    public = json.loads((ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    return next(row for row in public["assets"] if row["asset_id"] == asset)


class OrbitalArtContractTests(unittest.TestCase):
    """Protect native model meaning and saved geometry before source authoring."""

    def test_actual_compiled_definition_footprints(self):
        """Compare every mapped source footprint with its actual native definition.

        @param self Isolated source/native drift fixture.
        """
        source = (ROOT / "src/multilayer_space_content.cpp").read_text(encoding="utf-8")
        sizes = {kind: [int(width), int(height)] for kind, width, height in re.findall(
            r"\{SpaceBuildKind::(\w+),[^\n]*Year\{\d+\},\s*(\d+),\s*(\d+)\}", source)}
        # Shared graphics may cover several actual processor kinds, but each one must retain its own real paid footprint.
        for asset in MODULE_LAYOUTS:
            row = family(asset)
            validate_orbital_contract(row)
            for kind in row["native_build_kinds"]:
                self.assertEqual(row["footprint"], sizes[kind], (asset, kind))
            self.assertEqual(row["tile_variants"], row["footprint"][0] * row["footprint"][1])

    def test_native_site_has_no_saved_rotation(self):
        """A graphics template cannot invent four orientations for a fixed native site.

        @param self Isolated saved-layout fixture.
        """
        source = (ROOT / "src/space_structure.h").read_text(encoding="utf-8")
        declaration = re.search(r"struct SpaceStructureState \{(.*?)\n\};", source, re.S).group(1)
        self.assertNotRegex(declaration, r"\b(?:rotation|orientation|direction)\s*[={;]")
        for asset in MODULE_LAYOUTS:
            self.assertEqual(family(asset)["directions"], ["none"])

    def test_actual_heavy_miner_retains_distinct_native_id(self):
        """The appended mining model cannot repurpose the original robot or hauler.

        @param self Isolated canonical-model fixture.
        """
        source = (ROOT / "src/space_mining_tuning.h").read_text(encoding="utf-8")
        model = int(re.search(r"SPACE_HEAVY_MINER_MODEL\s*=\s*(\d+)", source).group(1))
        self.assertEqual(family("vehicle.model-21")["native_model_id"], model)
        self.assertEqual(family("vehicle.model-16")["native_role"], "retained_heavy_construction_robot")
        self.assertEqual(family("vehicle.model-17")["native_role"], "orbital_cargo_hauler")

    def test_paid_footprint_shrink_refuses(self):
        """Refuse the previous2x2 draft for actual3x3 Core and processing sites.

        @param self Isolated malformed native geometry.
        """
        for asset in ("module.nucleus", "module.processor"):
            row = family(asset)
            row.update(footprint=[2, 2], tile_variants=4)
            with self.assertRaisesRegex(ValueError, "native orbital"):
                validate_orbital_contract(row)

    def test_invented_orientation_refuses(self):
        """Refuse copied building rotations unsupported by any saved native field.

        @param self Isolated malformed rotation fixture.
        """
        row = family("module.dock")
        row["directions"] = ["NE", "SE", "SW", "NW"]
        with self.assertRaisesRegex(ValueError, "native orbital"):
            validate_orbital_contract(row)

    def test_borrowed_construction_work_refuses(self):
        """Robot16 cannot animate an unsupported work pose using probe20's job.

        @param self Isolated incorrect legacy-role source policy.
        """
        source = (ROOT / "src/orbital_station.cpp").read_text(encoding="utf-8")
        ready = re.search(r"static bool ReadyProbe\([^)]*\)\s*\{(.*?)\n\}", source, re.S).group(1)
        self.assertIn("GetSpaceVehicleModel(v) == 20", ready)
        row = family("vehicle.model-16")
        row["source_frame_contract"]["working"] = [0, 1]
        with self.assertRaisesRegex(ValueError, "native orbital"):
            validate_orbital_contract(row)

    def test_hauler_movement_retains_actual_load_cue(self):
        """Loaded movement cannot collapse into a misleading generic empty hull.

        @param self Isolated load/motion coverage fixture.
        """
        row = family("vehicle.model-17")
        # Each actual load condition needs both a clean stopped pose and admitted travelling poses; stopped cargo may not retain a frozen exhaust source.
        for load in ("empty", "partial", "full"):
            self.assertIn(load, row["states"])
            self.assertIn("moving-" + load, row["states"])
            self.assertEqual(row["source_frame_contract"][load], [0, 0])
        row["states"].remove("moving-full")
        with self.assertRaisesRegex(ValueError, "native orbital"):
            validate_orbital_contract(row)

    def test_missing_mining_state_or_repurposed_model_refuses(self):
        """Refuse omitted genuine work coverage and an alias to the old robot ID.

        @param self Isolated incomplete or confused mining contract.
        """
        original = family("vehicle.model-21")
        # Independent corruptions must each refuse; a valid identity cannot hide missing mining source and a complete pose list cannot excuse role reuse.
        for corruption in ("state", "model"):
            row = copy.deepcopy(original)
            if corruption == "state":
                row["states"].remove("mining")
            else:
                row["native_model_id"] = 16
            with self.assertRaisesRegex(ValueError, "native orbital"):
                validate_orbital_contract(row)


if __name__ == "__main__":
    unittest.main()
