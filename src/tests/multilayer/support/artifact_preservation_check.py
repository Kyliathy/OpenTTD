"""Focused exact-byte save migration checks, independent of native save decoding.

@file artifact_preservation_check.py Legacy/corrupt inputs and complete alias safety.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group9.
"""

import copy
import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from artifact_paths import repository_root
from artifact_preservation import migrated_catalogue, preserve_payloads, verify_preservation


class PreservationChecks(unittest.TestCase):
    """Preserve differing/duplicate original payloads in a disposable source root.

    @param self Active repository-local test, never the user's real save owners.
    """

    def setUp(self):
        """Create two same-name different saves plus a duplicate corrupt legacy alias.

        @param self Active isolated test.
        @return None; scratch is removed only within this repository.
        """
        cache = repository_root() / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=cache)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.manifest = dict(schema_version=1, original_path_count=3, payloads=[])
        # Unknown/corrupt save-like content remains a required input. Distinct
        # original parents preserve same-name files without collapsing identity.
        for originals, payload in [(('logs/a/current.sav', 'mlm/runtime/b/old.ss0'), b'unsupported legacy'),
                                   (('logs/b/current.sav',), b'OTTX deliberately malformed')]:
            for original in originals:
                path = self.root / original
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(payload)
            self.manifest["payloads"].append(dict(original_paths=list(originals),
                destination='savegames/retained/' + originals[0], sha256=hashlib.sha256(payload).hexdigest(),
                role="unknown_save", disposition="export", verification="inventoried_not_exported"))

    def test_all_original_aliases_and_corrupt_payloads_verify(self):
        """Exact preservation does not attempt to decode or repair old save bytes.

        @param self Active isolated test.
        @return None; every alias and each conflicting basename remains represented.
        """
        result = preserve_payloads(self.manifest, self.root)
        with patch.object(Path, "mkdir", side_effect=AssertionError("verification writes")), \
                patch.object(Path, "write_bytes", side_effect=AssertionError("verification writes")):
            facts = verify_preservation(result, self.root)
        self.assertEqual((facts["payloads"], facts["original_aliases"]), (2, 3))
        catalogue = migrated_catalogue(dict(inputs={}), result, self.root)
        self.assertEqual(catalogue["alias_precedence"], "retained_before_existing")
        self.assertNotEqual(catalogue["inputs"]["logs/a/current.sav"], catalogue["inputs"]["logs/b/current.sav"])
        self.assertEqual(catalogue["inputs"]["logs/a/current.sav"], catalogue["inputs"]["mlm/runtime/b/old.ss0"])

    def test_changed_original_and_conflicting_retained_bytes_fail(self):
        """A preexisting destination cannot substitute for a changed original.

        @param self Active isolated test.
        @return None; changed and conflicting identities each fail independently.
        """
        result = preserve_payloads(self.manifest, self.root)
        original = self.root / "mlm/runtime/b/old.ss0"
        original.write_bytes(b"changed original")
        with self.assertRaisesRegex(ValueError, "alias changed"):
            preserve_payloads(self.manifest, self.root)
        original.write_bytes(b"unsupported legacy")
        (self.root / result["payloads"][0]["destination"]).write_bytes(b"conflicting retained")
        with self.assertRaisesRegex(ValueError, "conflicting retained"):
            preserve_payloads(self.manifest, self.root)

    def test_same_destination_different_payload_is_rejected(self):
        """Deduplication is permitted only when payload identities agree.

        @param self Active isolated test.
        @return None; same-name/different-content exports never overwrite each other.
        """
        invalid = copy.deepcopy(self.manifest)
        invalid["payloads"][1]["destination"] = invalid["payloads"][0]["destination"]
        with self.assertRaisesRegex(ValueError, "different inventoried content"):
            preserve_payloads(invalid, self.root)

    def test_retained_only_verification_survives_original_removal(self):
        """The durable migration supports a cleaned checkout without old aliases.

        @param self Active isolated test.
        @return None; removing only synthetic originals leaves every export intact.
        """
        result = preserve_payloads(self.manifest, self.root)
        for row in result["payloads"]:
            for original in row["original_paths"]:
                (self.root / original).unlink()
        self.assertEqual(verify_preservation(result, self.root, originals=False)["original_aliases"], 3)
        with self.assertRaises(FileNotFoundError):
            verify_preservation(result, self.root)

    def test_long_original_alias_uses_bounded_owner_without_losing_basename(self):
        """Deep review inputs remain transferable through ordinary Windows paths.

        @param self Active isolated test.
        @return None; a shortened destination preserves the exact original alias.
        """
        component = "x" * max(10, 230 - len(str(self.root)) - len("/logs//current.sav"))
        original = f"logs/{component}/current.sav"
        source = self.root / original
        source.parent.mkdir(parents=True)
        payload = b"unsupported long-path original"
        source.write_bytes(payload)
        self.manifest["payloads"].append(dict(original_paths=[original],
            destination="savegames/retained/" + original, sha256=hashlib.sha256(payload).hexdigest(),
            role="unknown_save", disposition="export", verification="inventoried_not_exported"))
        self.manifest["original_path_count"] += 1
        result = preserve_payloads(self.manifest, self.root)
        row = result["payloads"][-1]
        self.assertIn("/by-hash/", row["destination"])
        self.assertEqual(Path(row["destination"]).name, "current.sav")
        self.assertEqual(row["original_paths"], [original])
        self.assertEqual((self.root / row["destination"]).read_bytes(), payload)


if __name__ == "__main__":
    unittest.main(verbosity=2)
