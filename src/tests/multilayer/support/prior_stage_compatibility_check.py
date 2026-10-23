"""Keep original old-format geology inputs independent of current generated saves.

@file prior_stage_compatibility_check.py No native process or fixture mutation.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group21.
@see docs/zz-reach2/architecture/archi-historical-debug.md.
"""
import copy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_save_fixture import chunks, preserved_geology, read_stream


class PriorCompatibilityChecks(unittest.TestCase):
    """Allow the documented zero-hardness header migration and refuse changed original authority."""

    def setUp(self):
        """Read an immutable historical partial world and create a detached migration fixture.

        @param self Original-format compatibility test owner.
        """
        self.path = ROOT / "src/tests/fixtures/geology/r2mlvs2-g-partial.sav"
        version, stream = read_stream(self.path)
        self.assertIn(version, range(372, 379))
        self.original = {row["name"]: row for row in chunks(stream)}
        self.restored = copy.deepcopy(self.original)
        old = self.original[b"GEOS"]["payload"]
        self.restored[b"GEOS"]["payload"] = b"\0\2" + old[2:29] + bytes(10) + old[29:]

    def test_original_old_input_allows_only_documented_zero_hardness_migration(self):
        """Preserve independent saved terrain/deposit/span bytes across current resaving.

        @param self Historical fixture preservation test owner.
        """
        preserved_geology(self.original, self.restored)

    def test_changed_terrain_or_invented_hardness_refuses(self):
        """A new header cannot justify changing saved rock or inventing regional geology.

        @param self Terrain and hardness authority refusal owner.
        """
        payload = bytearray(self.restored[b"GEOS"]["payload"])
        payload[29] = 1
        self.restored[b"GEOS"]["payload"] = bytes(payload)
        with self.assertRaisesRegex(ValueError, "terrain changed"):
            preserved_geology(self.original, self.restored)

    def test_changed_deposit_authority_refuses(self):
        """Original reserves/surveys remain exact even when terrain migration is permitted.

        @param self Deposit and span authority refusal owner.
        """
        self.restored[b"GDEP"]["raw"] += b"changed"
        with self.assertRaisesRegex(ValueError, "deposit/span"):
            preserved_geology(self.original, self.restored)


if __name__ == "__main__":
    unittest.main()
