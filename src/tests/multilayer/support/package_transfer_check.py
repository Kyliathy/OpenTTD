"""Exercise complete transfer contents and refusal boundaries on a small tree.

@file package_transfer_check.py No live cleanup, Git mutation commands or build.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group29.
@see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
"""

import base64
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch
import zipfile
import zlib

from artifact_paths import repository_root
from package_transfer import MANIFEST, create_transfer_archive, enumerate_preserved_files, stage_transfer, verify_transfer_archive


class PackageTransferChecks(unittest.TestCase):
    """Protect untracked originals, save export, Git closure and safe extraction.

    @param self Active independent scratch fixture beneath the real test-cache.
    """

    def setUp(self):
        """Create a tiny complete repository without executing a Git write command.

        @param self Active isolated transfer fixture owner.
        @return None; all fixtures and archive output remain under test-cache.
        """
        self.workspace = repository_root()
        owner = self.workspace / "test-cache/cleanup/package-checks"
        owner.mkdir(parents=True, exist_ok=True)
        self.scratch = Path(tempfile.mkdtemp(dir=owner))
        self.root = self.scratch / "source"
        self.root.mkdir()
        self.git_store(self.root)
        self.write("CMakeLists.txt", b"# @see r2fc2-file-cleanup3.md; isolated package fixture.\n")
        self.write("src/untracked.py", b'"""@file untracked.py @see r2fc2-file-cleanup3.md; valuable source."""\n')
        self.write(".gitignore", b"media/authored.png\nsavegames/\nlogs/\nbuild/\n")
        # One independently selected RGBA pixel (32,64,96,255) is a complete
        # authored PNG, rather than a header-shaped stand-in for approved art.
        self.write("media/authored.png", base64.b64decode("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR4nGNQcEj4DwADBAHAgChw2gAAAABJRU5ErkJggg=="))
        self.write("savegames/retained/existing.sav", b"OTTXoriginal immutable save")
        self.write("logs/current.sav", b"OTTZnew mutable process save")
        self.write("logs/current.scr", b"# @see r2fc2-file-cleanup3.md\nset gui.vehicle_speed true\n")
        self.write("logs/optional.log", b"optional diagnostic")
        self.write("test-cache/.gitignore", b"# @see r2fc2-file-cleanup3.md; retained routing source.\n*\n!.gitignore\n")
        self.write("test-cache/temporary.bin", b"generated scratch")
        self.write("build/output.obj", b"generated object")
        self.write("mlm/reviews/README.md", b"Owner: r2fc2-file-cleanup3.md; retained routing fixture.\n")
        self.write("mlm/reviews/generated/report.json", b"{}")
        self.write("src/tests/fixtures/cleanup/migration.json", json.dumps(dict(payloads=[], original_path_count=0)).encode())
        self.write("src/tests/fixtures/cleanup/source-epochs.json", json.dumps(dict(payloads=[], generated=[], original_candidates=0)).encode())
        self.write("src/tests/fixtures/multilayer-inputs.json", b'{"inputs":{}}')
        shutil.copy2(self.workspace / "clean.ps1", self.root / "clean.ps1")

    def write(self, relative, data):
        """Write independently selected fixture bytes under this fake checkout.

        @param self Active scratch fixture owner.
        @param relative Exact relative source, media, save or metadata path.
        @param data Independently supplied bytes rather than current actual output.
        @return Created file path.
        """
        target = self.root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        return target

    def tearDown(self):
        """Remove only this explicitly owned synthetic repository and its packages.

        @param self Active fixture with a unique scratch root created by setUp.
        @return None; live Git metadata and source are outside the checked root.
        """
        owner = (self.workspace / "test-cache/cleanup/package-checks").resolve()
        target = self.scratch.resolve()
        if target == owner or not target.is_relative_to(owner):
            raise ValueError("package fixture cleanup escaped its scratch owner")
        shutil.rmtree(target)

    def git_store(self, root):
        """Construct minimal valid Git fixture objects directly, without Git writes.

        @param self Active scratch fixture owner.
        @param root New fake repository root, never an existing metadata store.
        @return None; the read-only Git verifier must accept the complete store.
        """
        metadata = root / ".git"
        (metadata / "objects").mkdir(parents=True)
        (metadata / "refs/heads").mkdir(parents=True)

        def object_bytes(kind, payload):
            """Encode one independent immutable fixture object in Git's format.

            @param kind Git tree/commit type for this tiny fixture.
            @param payload Exact independently selected object body.
            @return Content identity written only into the new scratch store.
            """
            raw = kind.encode() + b" " + str(len(payload)).encode() + b"\0" + payload
            digest = hashlib.sha1(raw).hexdigest()
            path = metadata / "objects" / digest[:2] / digest[2:]
            path.parent.mkdir(exist_ok=True)
            path.write_bytes(zlib.compress(raw))
            return digest

        tree = object_bytes("tree", b"")
        commit = object_bytes("commit", (f"tree {tree}\nauthor Fixture <fixture@example.invalid> 1 +0000\ncommitter Fixture <fixture@example.invalid> 1 +0000\n\nIndependent transfer fixture\n").encode())
        (metadata / "refs/heads/main").write_text(commit + "\n", encoding="ascii")
        (metadata / "HEAD").write_text("ref: refs/heads/main\n", encoding="ascii")
        (metadata / "config").write_text("[core]\nrepositoryformatversion = 0\nbare = false\n", encoding="ascii")

    def test_small_complete_stage_archive_and_extract(self):
        """Keep ignored art/untracked source, export saves and extract valid Git.

        @param self Active isolated complete-transfer check.
        @return None; exact independent bytes and excluded generated roots assert.
        """
        stage = self.scratch / "staged"
        result = stage_transfer(stage, self.root)
        self.assertEqual(result["required_inputs"]["unresolved_required_inputs"], 0)
        self.assertEqual((stage / "src/untracked.py").read_bytes(), (self.root / "src/untracked.py").read_bytes())
        self.assertEqual((stage / "media/authored.png").read_bytes(), (self.root / "media/authored.png").read_bytes())
        self.assertTrue((stage / ".git/objects").is_dir())
        self.assertTrue((stage / "mlm/reviews/README.md").is_file())
        self.assertFalse((stage / "logs").exists())
        self.assertFalse((stage / "build").exists())
        self.assertFalse((stage / "mlm/reviews/generated").exists())
        self.assertEqual((stage / "test-cache/.gitignore").read_bytes(), (self.root / "test-cache/.gitignore").read_bytes())
        self.assertFalse((stage / "test-cache/temporary.bin").exists())
        self.assertTrue((self.root / "logs/current.sav").exists())
        journal = json.loads((stage / "src/tests/fixtures/cleanup/save-exports.json").read_text())
        self.assertEqual((stage / journal["aliases"]["logs/current.sav"]["path"]).read_bytes(), b"OTTZnew mutable process save")
        self.assertEqual((stage / journal["aliases"]["logs/current.scr"]["path"]).read_bytes(), (self.root / "logs/current.scr").read_bytes())
        # Independent copies protect the original working tree from cold edits.
        (stage / "src/untracked.py").write_bytes(b'"""@see r2fc2-file-cleanup3.md; staged edit."""\n')
        self.assertNotEqual((stage / "src/untracked.py").read_bytes(), (self.root / "src/untracked.py").read_bytes())
        archive = self.scratch / "transfer.zip"
        packed = create_transfer_archive(archive, stage)
        self.assertTrue(packed["verified"])
        extracted = self.scratch / "extracted"
        checked = verify_transfer_archive(archive, extracted)
        self.assertEqual(checked["files"], packed["files"])
        self.assertEqual((extracted / "media/authored.png").read_bytes(), (self.root / "media/authored.png").read_bytes())
        self.assertFalse((extracted / MANIFEST).exists())
        with zipfile.ZipFile(archive) as package:
            self.assertNotIn(archive.name, package.namelist())

    def test_external_git_objects_and_worktrees_refuse(self):
        """Do not hide external Git dependencies inside an apparently local package.

        @param self Active isolated Git-dependency refusal check.
        @return None; no source is removed and no incomplete package succeeds.
        """
        alternates = self.write(".git/objects/info/alternates", b"C:/unselected/object/store\n")
        with self.assertRaisesRegex(ValueError, "external Git dependency"):
            enumerate_preserved_files(self.root)
        alternates.unlink()
        self.write(".git/worktrees/other/gitdir", b"C:/unselected/worktree/.git\n")
        with self.assertRaisesRegex(ValueError, "linked-worktree metadata"):
            enumerate_preserved_files(self.root)

    def test_committed_exports_reuse_only_unchanged_save_versions(self):
        """Reuse verified journals while preserving a later changed save separately.

        @param self Active isolated staging/reuse/data-preservation check.
        @return None; all reuse still performs current physical save verification.
        """
        stage_transfer(self.scratch / "first", self.root)
        journal = self.root / "src/tests/fixtures/cleanup/save-exports.json"
        original_record = journal.read_bytes()
        with patch("package_transfer.apply_save_exports", side_effect=AssertionError("unchanged committed exports must not be rewritten")):
            stage_transfer(self.scratch / "reused", self.root)
        self.assertEqual(journal.read_bytes(), original_record)
        self.write("logs/current.sav", b"OTTNchanged current save version")
        stage = self.scratch / "changed"
        stage_transfer(stage, self.root)
        record = json.loads((stage / "src/tests/fixtures/cleanup/save-exports.json").read_text())
        reference = record["aliases"]["logs/current.sav"]
        self.assertEqual((stage / reference["path"]).read_bytes(), b"OTTNchanged current save version")
        original = json.loads(original_record)["aliases"]["logs/current.sav"]
        self.assertEqual((stage / original["path"]).read_bytes(), b"OTTZnew mutable process save")

    def test_local_nested_repository_is_preserved_and_external_pointer_refuses(self):
        """Retain real nested source stores and refuse dangling worktree pointers.

        @param self Active isolated nested-repository check.
        @return None; only fixture metadata is constructed.
        """
        nested = self.root / "vendor/source"
        self.git_store(nested)
        self.write("vendor/source/valuable.txt", b"untracked nested source")
        manifest = enumerate_preserved_files(self.root)
        self.assertIn("vendor/source", manifest["git_repositories"])
        self.assertIn("vendor/source/.git/HEAD", {row["path"] for row in manifest["files"]})
        self.write("vendor/linked/.git", b"gitdir: C:/unselected/worktree/metadata\n")
        with self.assertRaisesRegex(ValueError, "external Git metadata"):
            enumerate_preserved_files(self.root)

    def test_promised_or_missing_git_objects_refuse(self):
        """Never treat remote promises or an absent commit as complete local history.

        @param self Active isolated Git object-closure check.
        @return None; no incomplete archive or source stage is created.
        """
        configuration = self.root / ".git/config"
        original = configuration.read_bytes()
        configuration.write_bytes(original + b"[remote \"origin\"]\npromisor = yes\n")
        with self.assertRaisesRegex(ValueError, "partial Git object store"):
            enumerate_preserved_files(self.root)
        configuration.write_bytes(original)
        marker = self.write(".git/objects/pack/fixture.promisor", b"")
        with self.assertRaisesRegex(ValueError, "partial Git object store"):
            enumerate_preserved_files(self.root)
        marker.unlink()
        commit = (self.root / ".git/refs/heads/main").read_text(encoding="ascii").strip()
        (self.root / ".git/objects" / commit[:2] / commit[2:]).unlink()
        with self.assertRaisesRegex(ValueError, "Git transfer dependency check failed"):
            enumerate_preserved_files(self.root)

    def test_output_self_inclusion_and_unexported_save_refuse(self):
        """Never omit a new generated save or include an archive in its payload.

        @param self Active isolated preservation/output refusal check.
        @return None; the original save remains independently available.
        """
        with self.assertRaisesRegex(ValueError, "self-exclusion"):
            create_transfer_archive(self.root / "test-cache/self.zip", self.root)
        with self.assertRaisesRegex(ValueError, "committed|export"):
            create_transfer_archive(self.scratch / "unsafe.zip", self.root)
        self.assertFalse((self.scratch / "unsafe.zip").exists())
        self.assertEqual((self.root / "logs/current.sav").read_bytes(), b"OTTZnew mutable process save")

    def test_archive_corruption_and_unsafe_members_refuse_before_extraction(self):
        """Verify byte integrity and traversal/device refusal before writing payloads.

        @param self Active isolated archive boundary check.
        @return None; every rejected archive leaves extraction absent.
        """
        manifest = dict(kind="source_transfer_manifest", self_exclusion=MANIFEST, git_repositories=["."], files=[])
        archive = self.scratch / "malformed.zip"
        # Independent malformed ZIP cases cover path escape, aliasing and wrong
        # member bytes rather than trusting a manifest derived from those bytes.
        for index, name in enumerate(("../escape.txt", "C:/escape.txt", "folder\\escape.txt", "NUL.txt", "trailing. ")):
            selected = copy.deepcopy(manifest)
            selected["files"] = [dict(path=name, sha256=hashlib.sha256(b"expected").hexdigest(), bytes=8)]
            with zipfile.ZipFile(archive, "w") as package:
                package.writestr(MANIFEST, json.dumps(selected))
                package.writestr(name, b"expected")
            archive.with_name(archive.name + ".sha256").write_text(hashlib.sha256(archive.read_bytes()).hexdigest(), encoding="ascii")
            target = self.scratch / f"refused-{index}"
            with self.assertRaisesRegex(ValueError, "unsafe"):
                verify_transfer_archive(archive, target)
            self.assertFalse(target.exists())
        archive.write_bytes(archive.read_bytes() + b"changed archive")
        with self.assertRaisesRegex(ValueError, "checksum"):
            verify_transfer_archive(archive)
        selected = copy.deepcopy(manifest)
        selected["files"] = [dict(path="safe.txt", sha256=hashlib.sha256(b"expected").hexdigest(), bytes=8)]
        with zipfile.ZipFile(archive, "w") as package:
            package.writestr(MANIFEST, json.dumps(selected))
            package.writestr("safe.txt", b"corrupted member")
        archive.with_name(archive.name + ".sha256").write_text(hashlib.sha256(archive.read_bytes()).hexdigest(), encoding="ascii")
        with self.assertRaisesRegex(ValueError, "member identity"):
            verify_transfer_archive(archive, self.scratch / "bad-bytes")
        self.assertFalse((self.scratch / "bad-bytes").exists())

    @unittest.skipUnless(os.name == "nt", "Windows tool/API path limit")
    def test_long_windows_extraction_refuses_before_creating_partial_tree(self):
        """Keep complete member identities and fail before a long-root partial copy.

        @param self Active isolated portable-path extraction check.
        @return None; the safe archive remains available for a shorter root.
        """
        name = "/".join(["portable"] * 30) + "/payload.bin"
        manifest = dict(kind="source_transfer_manifest", self_exclusion=MANIFEST,
                        git_repositories=["."], files=[dict(path=name, sha256=hashlib.sha256(b"expected").hexdigest(), bytes=8)])
        archive = self.scratch / "long-path.zip"
        with zipfile.ZipFile(archive, "w") as package:
            package.writestr(MANIFEST, json.dumps(manifest))
            package.writestr(name, b"expected")
        archive.with_name(archive.name + ".sha256").write_text(hashlib.sha256(archive.read_bytes()).hexdigest(), encoding="ascii")
        target = self.scratch / "refused-long-root"
        with self.assertRaisesRegex(ValueError, "shorter destination root"):
            verify_transfer_archive(archive, target)
        self.assertFalse(target.exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
