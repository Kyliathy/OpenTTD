"""Focused retained-source alias checks for the metadata-only Space history audit.

@file space_history_audit_check.py Synthetic inputs never represent a native test pass.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group7.
"""

import copy
import hashlib
from pathlib import Path
import tempfile
import unittest

from space_history_audit import ROOT, private_call, select_retained, verified_aliases

EPOCH_LABEL = "original"


def original_function(value):
    """Expose a global dependency to verify isolation without altering original code.

    @param value Synthetic suffix supplied by the focused private-namespace check.
    @return The active global epoch label followed by the supplied suffix.
    """
    return EPOCH_LABEL + value


class HistoryAliasTests(unittest.TestCase):
    """Require exact source/archive relationships before selecting historical storage.

    @param self Active isolated test; actual Space evidence remains read-only.
    """

    def setUp(self):
        """Create source and retained epochs inside one disposable repository subtree.

        @param self Active retained-source test.
        @return None; register removal of only this newly allocated scratch directory.
        """
        scratch = ROOT / "test-cache/cleanup/history-alias-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / "src").mkdir()
        (self.root / "CMakeLists.txt").write_text("# Owner: r2fc-file-cleanup3.md; synthetic history root.\n", encoding="utf-8")
        source = self.root / "src/owner.cpp"
        source.write_bytes(b"// @see r2fc-file-cleanup3.md; original synthetic epoch.\n")
        retained = self.root / "retained.cpp"
        retained.write_bytes(source.read_bytes())
        self.source = self.read_identity(source)
        self.retained = self.read_identity(retained)
        self.manifest = dict(products=[dict(source=self.source, retained=self.retained)])
        source.write_bytes(b"// @see r2fc-file-cleanup3.md; advanced synthetic epoch.\n")

    def read_identity(self, path):
        """Calculate independent synthetic fixture identities for the alias boundary.

        @param self Active isolated source root.
        @param path Existing synthetic file below that root.
        @return Exact path/SHA256 record without using an acceptance producer.
        """
        return dict(path=path.relative_to(self.root).as_posix(), sha256=hashlib.sha256(path.read_bytes()).hexdigest())

    def test_advanced_source_uses_only_its_exact_declared_archive(self):
        """Logical source movement cannot bless current bytes or arbitrary matching names.

        @param self Active isolated test.
        @return None; select the separately verified retained identity only.
        """
        rows, aliases = verified_aliases(self.manifest, self.read_identity, self.root)
        self.assertEqual(rows[0]["source"], self.source["path"])
        self.assertEqual(select_retained(self.source, aliases, self.read_identity, self.root), self.retained)
        wrong = dict(path=self.source["path"], sha256="0" * 64)
        with self.assertRaisesRegex(ValueError, "missing exact"):
            select_retained(wrong, aliases, self.read_identity, self.root)

    def test_tampered_archive_and_disagreeing_authorities_fail(self):
        """A retained relationship requires equal recorded hashes and actual unchanged bytes.

        @param self Active isolated test.
        @return None; either mismatch prevents creation of a usable storage alias.
        """
        invalid = copy.deepcopy(self.manifest)
        invalid["products"][0]["source"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "disagrees"):
            verified_aliases(invalid, self.read_identity, self.root)
        (self.root / self.retained["path"]).write_bytes(b"// @see r2fc-file-cleanup3.md; tampered synthetic archive.\n")
        with self.assertRaisesRegex(ValueError, "bytes changed"):
            verified_aliases(self.manifest, self.read_identity, self.root)

    def test_missing_source_is_resolved_without_restoring_its_path(self):
        """Historical storage selection performs no runtime/source restoration.

        @param self Active isolated test.
        @return None; the missing original stays absent after exact archive selection.
        """
        source = self.root / self.source["path"]
        source.unlink()
        _rows, aliases = verified_aliases(self.manifest, self.read_identity, self.root)
        self.assertEqual(select_retained(self.source, aliases, self.read_identity, self.root), self.retained)
        self.assertFalse(source.exists())

    def test_private_call_preserves_original_code_and_globals(self):
        """Storage dependency selection must not patch the original verifier module globally.

        @param self Active private-call test.
        @return None; both functions retain one code object and independent globals.
        """
        selected = private_call(original_function, dict(EPOCH_LABEL="retained"))
        self.assertIs(selected.__code__, original_function.__code__)
        self.assertEqual(original_function("-fact"), "original-fact")
        self.assertEqual(selected("-fact"), "retained-fact")
        self.assertEqual(original_function.__globals__["EPOCH_LABEL"], "original")


if __name__ == "__main__":
    unittest.main()
