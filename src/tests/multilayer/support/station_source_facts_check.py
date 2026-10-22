"""Verify original facility geometry roles without old reports or Git access.

@file station_source_facts_check.py No native execution, source mutation or baseline update.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DD.
"""

import copy
import importlib
import json
import unittest
from unittest.mock import patch

import station_source_facts as facts
from source_input_guard import no_old_proofs
import regenerate_ground_sources as source


class StationOriginalTests(unittest.TestCase):
    """Keep historical physical geometry separate from actual current native qualification."""

    def test_original_inputs_verify_without_old_products_or_git(self):
        """Read all exact original descriptor/record bytes from durable owners only.

        @param self Active original geometry identity check.
        @return None; original reports/Git objects are preservation witnesses, not runtime inputs.
        """
        with no_old_proofs(facts.ROOT), patch.object(facts.subprocess, "run", side_effect=AssertionError("ordinary source check invoked Git")):
            document = facts.load_originals()
            self.assertFalse(document["native_proof"])
            self.assertEqual(set(document["families"]), set(facts.OWNERS))
            # Both actual record and geometry must retain their independent
            # original identities; a descriptor name alone cannot authorize art.
            for asset, row in document["families"].items():
                for role in ("descriptor", "record"):
                    path = facts.ROOT / row[role]["path"]
                    value = json.loads(path.read_text(encoding="utf-8"))
                    self.assertEqual(facts.verify_original(value, role, asset=asset), row)

    def test_missing_changed_and_detached_originals_refuse(self):
        """Reject missing expectations and coherent source-era record/geometry edits.

        @param self Active independent original fixture refusal check.
        @return None; all faults are detached or mocked and cannot update a baseline.
        """
        with patch.object(facts, "FIXTURE", facts.FIXTURE.with_name("absent-station-source-geometry.json")):
            with self.assertRaisesRegex(ValueError, "expectation is missing"):
                facts.load_originals()
        with patch.object(facts, "FIXTURE_SHA256", "0" * 64):
            with self.assertRaisesRegex(ValueError, "no automatic baseline update"):
                facts.load_originals()
        row = facts.load_originals()["families"]["facility.rail-station"]
        original = json.loads((facts.ROOT / row["record"]["path"]).read_text(encoding="utf-8"))
        changed = copy.deepcopy(original)
        changed["families"]["facility.rail-station"]["state_component_geometry"]["native_canvas"] = [1, 1]
        with self.assertRaisesRegex(ValueError, "original station source record changed"):
            facts.verify_original(changed, "record", asset="facility.rail-station")

    def test_current_native_defaults_remain_strict_and_aliases_restore(self):
        """Refuse stale native owners and restore exact original imported readers after failure.

        @param self Active native/source role isolation check; no executable is run.
        @return None; source-only physical reconstruction cannot leak native authority.
        """
        # Both descriptor epochs are historical. Default native freshness must
        # still refuse them before any original art reconstruction is selected.
        for asset, name in facts.OWNERS.items():
            owner = source.ground_station_contract if name == "station" else source.ground_terminal_contract
            module, _record = source.converter(asset)
            related = source.related_converters(module)
            aliases = list(related)
            if name == "station":
                aliases.append(importlib.import_module("create_station_art_guides"))
            previous = {module: module.load_geometry for module in [owner, *aliases] if hasattr(module, "load_geometry")}
            with self.assertRaisesRegex(ValueError, "geometry owner changed"):
                owner.load_geometry()
            with self.assertRaisesRegex(RuntimeError, "isolation test"):
                with source.historical_geometry(asset=asset, related=related), no_old_proofs(facts.ROOT):
                    owner.load_geometry()
                    # Actual imported geometry aliases must select precisely
                    # the same source-era descriptor while this leaf executes.
                    for alias in previous:
                        alias.load_geometry()
                    raise RuntimeError("isolation test")
            for alias, reader in previous.items():
                self.assertIs(alias.load_geometry, reader)
            with self.assertRaisesRegex(ValueError, "geometry owner changed"):
                owner.load_geometry()


if __name__ == "__main__":
    unittest.main(verbosity=2)
