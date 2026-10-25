"""Check complete original/merged ledgers, nested TABLE transport and partial-digest refusals.

@file complete_world_evidence_check.py
@see docs/zz-reach2/architecture/savegame/archi-savegame.md
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
Explicit prior native outputs supply metamorphic comparison inputs only. The
expected merged family set comes from unchanged originals, never those outputs.
Fixtures persist in RunContext saves; no original or historical receipt is edited.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import unittest

from artifact_paths import create_run_context
from savegame_tables import Reader, chunks, complete_world, decode_table, read_stream, table_header

ROOT = Path(__file__).resolve().parents[4]
spec = importlib.util.spec_from_file_location("complete_ground_owner", ROOT / "src/tests/multilayer/acceptance/native_ground.py")
owner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(owner)
sys.path.insert(0, str(ROOT / "src/tests"))
from merged_supply_receipts import merged_book, original_book

SUCCESSOR = None


class CompleteWorldEvidence(unittest.TestCase):
    """Compare every actual wire owner independently of narrower native diagnostics."""

    def setUp(self):
        """Copy an explicitly supplied merged native output into this test's generated owner.

        @param self Active metamorphic reader check; the supplied source is never changed.
        """
        self.context = create_run_context("merge-evidence", "complete-world")
        self.before = self.context.output("saves", "before.sav")
        self.before.write_bytes(SUCCESSOR.read_bytes())
        self.version, self.stream = read_stream(self.before)
        self.records = chunks(self.stream)

    def changed(self, name):
        """Change one owner's actual payload byte while retaining every other owner exactly.

        @param self Active real native-output comparison fixture.
        @param name Exact four-byte owner label selected for this refusal case.
        @return New uncompressed generated save with the unchanged original version.
        """
        selected, = [row for row in self.records if row["name"] == name]
        raw = bytearray(selected["raw"])
        # A nonempty final TABLE row or RIFF payload retains its original extent.
        # Only a value byte changes; transport lengths/header and other owners stay
        # exact. This is refusal data, never an expected native state baseline.
        if selected["kind"] == 0:
            payload = bytearray(selected["payload"])
            if payload:
                payload[-1] ^= 1
            else:
                payload.append(1)
            length = len(payload)
            raw = name + bytes([(length >> 24) << 4]) + (length & 0xffffff).to_bytes(3, "big") + payload
        else:
            self.assertTrue(selected["rows"][-1])
            position = selected["raw"].rfind(selected["rows"][-1], 0, len(raw) - 1) + len(selected["rows"][-1]) - 1
            raw[position] ^= 1
        stream = b"".join(bytes(raw) if row["name"] == name else row["raw"] for row in self.records) + bytes(4)
        target = self.context.output("saves", name.decode("ascii").lower() + "-changed.sav")
        target.write_bytes(b"OTTN" + self.version.to_bytes(2, "big") + bytes(2) + stream)
        return target

    def test_original_dialects_and_merged_complete_family_contract(self):
        """Read actual independent A/B original schemas and the full merged set.

        @param self Active original/merged wire reader coverage check.
        """
        contract = json.loads((ROOT / "src/tests/fixtures/saveload/merged-family-set.json").read_text())
        for original in contract["originals"]:
            path = ROOT / original["path"]
            self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), original["sha256"])
            version, all_owners, tables = complete_world(path)
            self.assertEqual(version, original["version"])
            self.assertEqual(len(all_owners), original["historical_family_count"])
            self.assertTrue(tables[b"DATE"])
            self.assertTrue(original_book(path, original["origin"])[1])
        version, all_owners, tables = complete_world(self.before)
        self.assertEqual(set(all_owners), {name.encode("ascii") for name in contract["families"]})
        self.assertTrue(merged_book(self.before)[1])
        ledger = owner.capture_ledger([self.before], complete_native=True)[0]
        self.assertEqual(len(ledger["owners"]), len(contract["families"]))
        self.assertEqual(set(ledger["independent_channels"]), {"cargo", "cash", "energy", "clocks", "rng"})
        self.assertEqual(owner.compare_state_digest(self.before, self.before, complete_native=True)["owner_count"], len(contract["families"]))

    def test_nested_schema_dense_holes_and_sparse_identity(self):
        """Nested schemas follow the complete parent header and cannot renumber retired slots.

        @param self Active exact native TABLE grammar check.
        """
        header = b"\x04\x02id\x0b\x05parts\x00\x04\x05stock\x00"
        data = b"\x00\x07\x02\x00\x03\x00\x04"
        decoded = decode_table(dict(name=b"TEST", kind=3, rows=[header, data, b"", data]))
        self.assertEqual(set(decoded), {0, 2})
        self.assertEqual(decoded[2], {"id": 7, "parts.count": 2, "parts[0].stock": 3, "parts[1].stock": 4})
        sparse = dict(name=b"TEST", kind=4, rows=[header, b"\x09" + data])
        self.assertEqual(decode_table(sparse), {9: decoded[0]})
        sparse["rows"].append(b"\x09" + data)
        with self.assertRaisesRegex(ValueError, "duplicate native sparse"):
            decode_table(sparse)
        with self.assertRaisesRegex(ValueError, "duplicate or unsupported"):
            table_header(Reader(b"\x04\x01x\x04\x01x\x00"))

    def test_script_vm_suffix_remains_exact_and_other_unknown_tails_refuse(self):
        """Only native script owners admit their declared VM suffix, retaining all its bytes.

        @param self Active explicit custom native TABLE-tail boundary check.
        """
        chunk = dict(name=b"GSDT", kind=3, rows=[b"\x04\x01x\x00", b"\x00\x07\xab\xcd"])
        self.assertEqual(decode_table(chunk)[0]["$native_script_tail"], "abcd")
        chunk["name"] = b"CAPA"
        with self.assertRaisesRegex(ValueError, "not fully decoded"):
            decode_table(chunk)

    def test_partial_digest_agreement_cannot_hide_owner_custody_clock_or_rng(self):
        """An equal narrow diagnostic never excuses any changed complete saved authority.

        @param self Active full-world refusal check over actual native serialized owners.
        """
        base = {row["name"]: row["raw"] for row in self.records}
        # These selected changes leave the illustrative narrow geology digest
        # equal. Full acceptance must still see ordinary state, supply custody,
        # electrical history, the saved clock and native RNG independently.
        for name in (b"VIEW", b"SSOR", b"UPLD", b"DATE", b"SRND"):
            changed = self.changed(name)
            actual = {row["name"]: row["raw"] for row in chunks(read_stream(changed)[1])}
            self.assertEqual(hashlib.sha256(base[b"GEOS"]).digest(), hashlib.sha256(actual[b"GEOS"]).digest())
            with self.assertRaises(owner.EvidenceError):
                owner.compare_state_digest(self.before, changed, complete_native=True)

    def test_missing_family_trailing_transport_and_partial_acceptance_refuse(self):
        """Counts and partial flags cannot turn an incomplete or trailing-byte save into equality.

        @param self Active complete-world framing and scope refusal check.
        """
        missing = self.context.output("saves", "missing.sav")
        missing.write_bytes(b"OTTN" + self.version.to_bytes(2, "big") + bytes(2) +
                            b"".join(row["raw"] for row in self.records if row["name"] != b"VIEW") + bytes(4))
        with self.assertRaisesRegex(ValueError, "merged family"):
            complete_world(missing)
        with self.assertRaisesRegex(ValueError, "trailing bytes"):
            chunks(self.stream + b"extra")
        with self.assertRaisesRegex(owner.EvidenceError, "partial"):
            owner.compare_state_digest(self.before, self.before, complete_native=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--successor", type=Path, required=True, help="Exact prior native441 output for metamorphic reader comparisons only.")
    arguments = parser.parse_args()
    SUCCESSOR = arguments.successor.resolve()
    if not SUCCESSOR.is_relative_to(ROOT):
        raise ValueError("comparison input escapes this checkout")
    unittest.main(argv=[sys.argv[0]], verbosity=2)
