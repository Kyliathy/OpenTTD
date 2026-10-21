"""Exercise current processor qualification and meaningful false-proof refusals.

@file processor_contract_refresh_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group41B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only the current source handoff is checked; no historical whole-art regression.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
from tools import ground_processor_contract as contract
from tools import ground_processor_qualification as qualification
from tools.ground_station_contract import digest


class CurrentProcessorQualificationChecks(unittest.TestCase):
    """Preserve historical geometry/art while requiring actual current native evidence."""

    @classmethod
    def setUpClass(cls):
        """Load one published current native qualification and immutable predecessor.

        @param cls Focused source-boundary test class.
        """
        cls.current = json.loads(qualification.QUALIFICATION.read_text(encoding="utf-8"))
        cls.historical = json.loads(contract.DESCRIPTOR.read_text(encoding="utf-8"))

    def test_current_native_proof_preserves_all_four_original_geometry_contracts(self):
        """Only exact current code and seven-case proof reauthorize the same sixteen-cell layouts.

        @param self Current qualification fixture with actual retained native artifacts.
        """
        self.assertEqual(contract.load_geometry(), self.historical)
        self.assertEqual(qualification.validate_qualification(self.historical, contract.OWNERS), self.current)
        self.assertEqual(self.current["families"], self.historical["families"])
        # Current source matrices retain fixed native cells and physical state
        # owners; qualification cannot introduce extra directions or geometry.
        for name in contract.SELECTED:
            row = contract.processor_contract(name)
            self.assertEqual(row["directions"], ["none"])
            self.assertEqual(row["tile_variants"], 16)
            self.assertEqual(row["native_size"], [64, 64])
            self.assertEqual(row["anchor"], [-31, -33])
            self.assertEqual(row["source_height_limit"], 20)

    def test_foreign_geometry_stale_sources_and_failed_processes_remain_refused(self):
        """Success summaries cannot authorize another layout, owner epoch or application.

        @param self Current qualification fixture with explicit mutated candidates.
        """
        candidates = []
        # Exercise independent admission prerequisites, retaining valid native
        # evidence elsewhere so each forgery tests its own ownership boundary.
        for key, value in (("complete", False), ("families", []), ("schema_version", 99)):
            candidate = copy.deepcopy(self.current)
            candidate[key] = value
            candidates.append(candidate)
        changed = copy.deepcopy(self.current)
        changed["native_source_sha256"][contract.OWNERS[0]] = "0"*64
        candidates.append(changed)
        changed = copy.deepcopy(self.current)
        changed["build_source_sha256"].pop("CMakeLists.txt")
        candidates.append(changed)
        for field, value in (("exit_code", 1), ("test_cases", 6), ("assertions", 0),
                             ("process_executable", dict(path="foreign.exe", sha256="0"*64))):
            changed = copy.deepcopy(self.current)
            changed["native_proof"][field] = value
            candidates.append(changed)
        # Every independently forged prerequisite must fail without changing
        # either the immutable descriptor or its published current admission.
        descriptor, published = contract.DESCRIPTOR.read_bytes(), qualification.QUALIFICATION.read_bytes()
        for candidate in candidates:
            with self.subTest(candidate=candidate), self.assertRaises(ValueError):
                qualification.validate_qualification(self.historical, contract.OWNERS, candidate)
        self.assertEqual(contract.DESCRIPTOR.read_bytes(), descriptor)
        self.assertEqual(qualification.QUALIFICATION.read_bytes(), published)

    def test_matching_file_hash_cannot_promote_concatenated_or_foreign_native_trace(self):
        """Actual log content and native process evidence still control admission after rehashing.

        @param self Current native proof and repository-owned temporary log fixture.
        """
        original = (ROOT / self.current["native_proof"]["log"]["path"]).read_text(encoding="utf-8-sig")
        wrong = original.replace('"tile_local_id":33280', '"tile_local_id":99999', 1)
        self.assertNotEqual(wrong, original)
        variations = (original+original, wrong, original.replace("NATIVE_EXIT_CODE=0", "NATIVE_EXIT_CODE=1"))
        temporary = ROOT / "test-cache/processor-source"
        temporary.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=temporary, prefix="qualification-refusal-") as directory:
            log = Path(directory) / "native.log"
            # These complete actual-file candidates have matching fresh
            # hashes; rejection must come from semantics, not a stale checksum.
            for output in variations:
                log.write_text(output, encoding="utf-8")
                candidate = copy.deepcopy(self.current)
                candidate["native_proof"]["log"] = dict(path=log.relative_to(ROOT).as_posix(),
                    sha256=hashlib.sha256(log.read_bytes()).hexdigest())
                with self.assertRaises(ValueError):
                    qualification.validate_qualification(self.historical, contract.OWNERS, candidate)

    def test_all_completed_media_payloads_and_existing_applications_remain_exact(self):
        """Only the explicitly archived contract helper changes among the 1196 prior artifacts.

        @param self Current preservation proof after native execution.
        """
        preservation = self.current["preservation"]
        before_path = ROOT / preservation["before"]["path"]
        self.assertEqual(digest(before_path), preservation["before"]["sha256"])
        rows = json.loads(before_path.read_text(encoding="utf-8"))["files"]
        self.assertEqual(len(rows), preservation["artifacts"])
        transition = preservation["helper_transition"]
        # Requalification permits precisely its archived helper transition;
        # completed source, payload and game application bytes get no waiver.
        for row in rows:
            expected = transition["current"]["sha256"] if row == transition["original"] else row["sha256"]
            self.assertEqual(digest(ROOT / row["path"]), expected, row["path"])
        self.assertEqual(digest(ROOT / transition["retained"]["path"]), transition["original"]["sha256"])


if __name__ == "__main__":
    unittest.main()
