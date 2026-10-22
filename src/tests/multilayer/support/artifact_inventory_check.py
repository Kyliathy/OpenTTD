"""Focused retention and read-only checks for the cleanup inventory boundary.

@file artifact_inventory_check.py No native process or acceptance-module imports.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group2.
"""

from dataclasses import replace
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from artifact_inventory import classify_reference, collect_candidates, migration_manifest
from artifact_paths import repository_root


class InventoryTests(unittest.TestCase):
    """Verify preservation using disposable source-shaped trees inside test-cache.

    @param self Per-test isolated fixture and assertions; actual saves remain read-only.
    """

    def setUp(self):
        """Create one owned temporary source root and register only its removal.

        @param self Active inventory test instance.
        @return None; all generated fixtures stay beneath repository test-cache.
        """
        scratch = repository_root() / "test-cache/cleanup/inventory-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / "src").mkdir()
        (self.root / "CMakeLists.txt").write_text("# Owner: r2fc-file-cleanup3.md; disposable source shape.\n", encoding="utf-8")

    def fixture(self, name, content):
        """Write an explicitly selected fixture inside this test's owned root.

        @param self Active isolated test.
        @param name Repository-relative fixture path.
        @param content Exact synthetic bytes needed for the behavioral check.
        @return Created fixture Path.
        """
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        return path

    def test_headers_names_and_unsupported_saves_stay_protected(self):
        """Retain renamed and unsupported containers plus undecodable backup names.

        @param self Active isolated test.
        @return None; no decoder is called and no source is exported.
        """
        self.fixture("build/save/renamed.payload", b"OTTD\x01\x00\x00\x00unsupported lzo")
        self.fixture("mlm/runtime/future.bin", b"OTTYfuture container")
        self.fixture("logs/checkpoint.sav.old", b"unrecognized historical bytes")
        self.fixture("test-cache/legacy.sv0", b"old game bytes")
        self.fixture("src/ordinary.bin", b"not a save")
        self.fixture("build/openttd.cfg", b"; Owner: r2fc-file-cleanup3.md fixture.\n")
        rows = collect_candidates(self.root)
        self.assertEqual(len(rows), 5)
        self.assertTrue(all(row.disposition == "export" for row in rows))
        self.assertEqual(next(row.format for row in rows if row.original_path.endswith("payload")), "lzo")
        self.assertEqual(sum(row.role == "unknown_save" for row in rows), 3)

    def test_inventory_and_import_write_nothing(self):
        """Inventory is read-only and importing the helper cannot reserve native runs.

        @param self Active isolated test.
        @return None; compare complete file sets and bytes before/after observation.
        """
        self.fixture("build/save/input.sav", b"OTTXimmutable synthetic input")
        before = {path.relative_to(self.root): path.read_bytes() for path in self.root.rglob("*") if path.is_file()}
        collect_candidates(self.root)
        after = {path.relative_to(self.root): path.read_bytes() for path in self.root.rglob("*") if path.is_file()}
        self.assertEqual(before, after)
        script = "import sys; import artifact_inventory; assert not any('surface_' in key for key in sys.modules)"
        subprocess.run([sys.executable, "-B", "-c", script], cwd=Path(__file__).parent,
            check=True, capture_output=True, text=True, timeout=60)

    def test_missing_and_changed_are_not_rebased(self):
        """Report missing originals and wrong expected bytes independently.

        @param self Active isolated test.
        @return None; publication refuses either unverified result.
        """
        self.fixture("build/changed.sav", b"OTTNactual")
        missing = classify_reference(dict(path="build/missing.sav", sha256="0" * 64), self.root, role="save")
        changed = classify_reference(dict(path="build/changed.sav", sha256="0" * 64), self.root)
        self.assertEqual(missing.verification, "missing")
        self.assertEqual(changed.verification, "changed")
        self.assertEqual(changed.sha256, "0" * 64)
        self.assertNotEqual(changed.actual_sha256, changed.sha256)
        # Both failure types must prevent an apparently valid migration contract.
        for row in (missing, changed):
            with self.assertRaises(ValueError):
                migration_manifest([row])

    def test_alias_precedence_and_expected_identity(self):
        """Existing originals win; absent keys use only verified explicit aliases.

        @param self Active isolated test.
        @return None; conflicting or corrupt aliases never certify a save.
        """
        old = self.fixture("build/original.sav", b"OTTNoriginal")
        target = self.fixture("src/tests/fixtures/input.sav", b"OTTNrelocated")
        digest = hashlib.sha256(target.read_bytes()).hexdigest()
        aliases = {"build/original.sav": dict(path="src/tests/fixtures/input.sav", sha256=digest)}
        original = classify_reference("build/original.sav", self.root, aliases=aliases)
        self.assertEqual(original.actual_sha256, hashlib.sha256(old.read_bytes()).hexdigest())
        old.unlink()
        self.assertEqual(classify_reference("build/original.sav", self.root, aliases=aliases).verification, "alias_verified")
        with self.assertRaisesRegex(ValueError, "conflicting alias"):
            classify_reference(dict(path="build/original.sav", sha256="0" * 64), self.root, aliases=aliases)
        target.write_bytes(b"OTTNchanged")
        self.assertEqual(classify_reference("build/original.sav", self.root, aliases=aliases).verification, "changed")

    def test_aliases_and_destinations_cannot_conflict(self):
        """Keep complete aliases for equal bytes and refuse competing authorities.

        @param self Active isolated test.
        @return None; duplication is allowed only for matching original identities.
        """
        self.fixture("build/a.sav", b"OTTNsame")
        self.fixture("build/b.sav", b"OTTNsame")
        rows = collect_candidates(self.root)
        manifest = migration_manifest(rows)
        self.assertEqual(manifest["original_path_count"], 2)
        self.assertEqual(manifest["payloads"][0]["original_paths"], ["build/a.sav", "build/b.sav"])
        with self.assertRaisesRegex(ValueError, "conflicting original"):
            migration_manifest([rows[0], replace(rows[0], sha256="0" * 64)])
        with self.assertRaisesRegex(ValueError, "conflicting destination"):
            migration_manifest([rows[0], replace(rows[1], sha256="0" * 64, destination=rows[0].destination)])

    def test_redirections_traversal_and_nested_repositories_refuse(self):
        """Reject ambiguous ownership before reading a candidate tree.

        @param self Active isolated test.
        @return None; junction and nested-repository targets remain untouched.
        """
        with self.assertRaises(ValueError):
            classify_reference("../elsewhere.sav", self.root)
        target = self.root / "src/target"
        target.mkdir()
        junction = self.root / "redirected"
        # Junctions need no elevated symbolic-link privilege on Windows; test only
        # our owned target and remove the junction before temporary-tree cleanup.
        if sys.platform == "win32":
            subprocess.run(["cmd", "/c", "mklink", "/J", str(junction), str(target)],
                check=True, capture_output=True, timeout=60)
            try:
                with self.assertRaisesRegex(ValueError, "redirected"):
                    collect_candidates(self.root)
            finally:
                junction.rmdir()
        nested = self.root / "nested/.git"
        nested.mkdir(parents=True)
        with self.assertRaisesRegex(ValueError, "nested repository"):
            collect_candidates(self.root)


if __name__ == "__main__":
    unittest.main()
