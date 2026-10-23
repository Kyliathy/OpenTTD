"""Exercise save-export data-loss boundaries in disposable repository fixtures.

@file save_retention_check.py Exact copies, immutable owners and stale-plan refusal.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group24.
@see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
No real cleanup or native execution occurs in these tests.
"""

import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch

from artifact_paths import repository_root
from save_retention import JOURNAL, MIGRATION, apply_save_exports, plan_save_exports, verify_save_exports


class SaveRetentionChecks(unittest.TestCase):
    """Verify export authorization with independently authored synthetic payloads.

    @param self Active isolated test; all writable paths remain inside the repo.
    """

    def setUp(self):
        """Reserve a disposable fixture without touching actual save owners.

        @param self Active isolated test.
        @return None; fixture cleanup removes only this repository-local scratch.
        """
        cache = repository_root() / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=cache)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.payload = b"OTTD\x00\x00legacy LZO bytes; deliberately not decodable"
        self.source = self.write("logs/run/current.sav", self.payload)

    def write(self, relative, payload):
        """Write independent fixture bytes within the active disposable root.

        @param self Active isolated test.
        @param relative Exact fixture-relative input or conflicting output name.
        @param payload Independently specified bytes, never generated expectations.
        @return Created fixture path.
        """
        target = self.root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(payload)
        return target

    def plan(self):
        """Select generated fixtures, including an initially absent runtime owner.

        @param self Active isolated test.
        @return Read-only export proposal.
        """
        return plan_save_exports(self.root, ("logs", "mlm/runtime"))

    def test_planning_has_no_writes_and_keeps_unknown_formats(self):
        """Header/name recognition never depends on Python save decoder support.

        @param self Active isolated test.
        @return None; no retained directory or journal may appear while planning.
        """
        # Each name/header is a distinct retention obligation; malformed content
        # and old backup forms must not disappear from a conservative scan.
        for name, payload in (("old.ss0", b"bad old save"), ("game.sc9.bak", b"backup"),
                              ("renamed.bin", b"OTTQ future"), ("old.sav~", b"old"),
                              ("autosave.previous", b"unknown"), ("known.dat", b"OTTZ invalid zlib")):
            self.write("logs/run/" + name, payload)
        with patch.object(Path, "mkdir", side_effect=AssertionError("planning writes")), \
                patch("save_retention.shutil.copyfile", side_effect=AssertionError("planning copies")), \
                patch("save_retention.publish_document", side_effect=AssertionError("planning publishes")):
            plan = self.plan()
        self.assertEqual(len(plan["observations"]), 7)
        self.assertEqual(plan["observations"][1]["format"], "lzo")
        self.assertFalse((self.root / "savegames").exists())
        self.assertFalse((self.root / JOURNAL).exists())

    def test_export_deduplicates_equal_bytes_and_preserves_sources(self):
        """Equal saves share retained storage while every original name survives.

        @param self Active isolated test.
        @return None; independently supplied bytes remain unchanged at both sources.
        """
        peer = self.write("mlm/runtime/host/renamed.dat", self.payload)
        plan = self.plan()
        result = apply_save_exports(plan, self.root)
        self.assertEqual((result["payloads"], result["save_aliases"]), (1, 2))
        self.assertEqual((self.source.read_bytes(), peer.read_bytes()), (self.payload, self.payload))
        row = plan["payloads"][0]
        self.assertEqual((self.root / row["destination"]).read_bytes(), self.payload)
        self.assertEqual(len(json.loads((self.root / JOURNAL).read_text())["aliases"]), 2)
        self.assertEqual(verify_save_exports(plan, self.root), result)
        self.assertEqual(apply_save_exports(self.plan(), self.root)["save_aliases"], 2)

    def test_changed_same_name_keeps_both_versions(self):
        """Mutable original aliases cannot replace a previously retained version.

        @param self Active isolated test.
        @return None; the Group9-style first destination remains byte-identical.
        """
        first = self.plan()
        apply_save_exports(first, self.root)
        replacement = b"OTTX independent later save"
        self.source.write_bytes(replacement)
        second = self.plan()
        self.assertNotEqual(first["payloads"][0]["destination"], second["payloads"][0]["destination"])
        apply_save_exports(second, self.root)
        journal = json.loads((self.root / JOURNAL).read_text())
        self.assertEqual(len(journal["payloads"]), 2)
        self.assertEqual((self.root / first["payloads"][0]["destination"]).read_bytes(), self.payload)
        self.assertEqual(journal["aliases"]["logs/run/current.sav"]["sha256"], hashlib.sha256(replacement).hexdigest())

    def test_existing_conflict_gets_distinct_alias_but_hash_owner_conflict_refuses(self):
        """Neither a same-name save nor a conflicting hash owner is overwritten.

        @param self Active isolated test.
        @return None; all pre-existing conflicting bytes remain unchanged.
        """
        old = self.write("savegames/retained/logs/run/current.sav", b"another original save")
        plan = self.plan()
        alternate = plan["payloads"][0]["destination"]
        self.assertIn("/by-hash/", alternate)
        self.write(alternate, b"explicit hash-owner conflict")
        with self.assertRaisesRegex(ValueError, "conflicting retained"):
            self.plan()
        self.assertEqual(old.read_bytes(), b"another original save")

    def test_group9_identity_is_reused_without_rebinding_frozen_catalogue(self):
        """The original migration can supply a durable identical payload directly.

        @param self Active isolated test.
        @return None; no historical manifest or stable input key is rewritten.
        """
        destination = "src/tests/fixtures/checkpoints/example.sav"
        self.write(destination, self.payload)
        original = json.dumps(dict(payloads=[dict(sha256=hashlib.sha256(self.payload).hexdigest(), destination=destination)]))
        migration = self.write(MIGRATION, original.encode())
        plan = self.plan()
        self.assertEqual(plan["payloads"][0]["destination"], destination)
        apply_save_exports(plan, self.root)
        self.assertEqual(migration.read_text(), original)

    def test_source_change_before_apply_refuses(self):
        """A stale plan cannot bless a changed original even before copying starts.

        @param self Active isolated test.
        @return None; no journal or retained payload is published.
        """
        plan = self.plan()
        self.source.write_bytes(b"new save data")
        with self.assertRaisesRegex(ValueError, "candidates changed"):
            apply_save_exports(plan, self.root)
        self.assertFalse((self.root / JOURNAL).exists())

    def test_source_change_during_copy_refuses(self):
        """A copy matching the old hash cannot conceal an original writer.

        @param self Active isolated test.
        @return None; the changed source remains intact and uncommitted.
        """
        plan = self.plan()
        original_copy = shutil.copyfile

        def changing_copy(source, destination):
            """Simulate a writer immediately after the original bytes were copied.

            @param source Original fixture save path.
            @param destination Exporter's invocation-local temporary target.
            @return None; source mutation is the explicit injected failure.
            """
            original_copy(source, destination)
            Path(source).write_bytes(b"OTTN concurrently modified save")

        with patch("save_retention.shutil.copyfile", side_effect=changing_copy):
            with self.assertRaisesRegex(ValueError, "source save changed"):
                apply_save_exports(plan, self.root)
        self.assertFalse((self.root / JOURNAL).exists())
        self.assertFalse((self.root / plan["payloads"][0]["destination"]).exists())

    def test_truncated_copy_refuses_and_same_plan_can_retry(self):
        """A partial temporary file never becomes a retained payload or authority.

        @param self Active isolated test.
        @return None; a subsequent ordinary copy succeeds without source repair.
        """
        plan = self.plan()

        def truncated_copy(source, destination):
            """Inject a short copy without changing the independent original.

            @param source Unmodified original fixture, intentionally unused.
            @param destination Temporary export file to truncate.
            @return None; corruption must be detected by the production hash check.
            """
            Path(destination).write_bytes(b"OTT")

        with patch("save_retention.shutil.copyfile", side_effect=truncated_copy):
            with self.assertRaisesRegex(ValueError, "truncated"):
                apply_save_exports(plan, self.root)
        self.assertFalse((self.root / JOURNAL).exists())
        self.assertFalse(list(self.root.rglob("*.pending")))
        self.assertTrue(apply_save_exports(plan, self.root)["verified"])

    def test_new_candidate_during_export_blocks_commit(self):
        """New saves in an initially absent watched owner block deletion authority.

        @param self Active isolated test.
        @return None; any completed copy remains safe but no journal commits.
        """
        plan = self.plan()
        original_copy = shutil.copyfile

        def new_save_copy(source, destination):
            """Inject an additional autosave after copying the selected source.

            @param source Selected original fixture save.
            @param destination Temporary export target.
            @return None; the new save belongs to the watched runtime owner.
            """
            original_copy(source, destination)
            self.write("mlm/runtime/main/autosave.sav", b"new original")

        with patch("save_retention.shutil.copyfile", side_effect=new_save_copy):
            with self.assertRaisesRegex(ValueError, "candidates changed"):
                apply_save_exports(plan, self.root)
        self.assertFalse((self.root / JOURNAL).exists())
        self.assertEqual(apply_save_exports(self.plan(), self.root)["save_aliases"], 2)

    def test_interrupted_record_commit_reuses_exact_completed_copy(self):
        """Journal failure grants no deletion and reruns reuse verified payloads.

        @param self Active isolated test.
        @return None; the retained copy and source survive the injected interruption.
        """
        plan = self.plan()
        with patch("save_retention.publish_document", side_effect=OSError("interrupted commit")):
            with self.assertRaisesRegex(OSError, "interrupted"):
                apply_save_exports(plan, self.root)
        with self.assertRaisesRegex(ValueError, "no committed alias"):
            verify_save_exports(plan, self.root)
        self.assertTrue(apply_save_exports(plan, self.root)["verified"])

    def test_retained_corruption_and_new_save_each_block_deletion(self):
        """The final gate rechecks actual retained bytes and the complete save set.

        @param self Active isolated test.
        @return None; neither later corruption nor a new original is hidden by JSON.
        """
        plan = self.plan()
        apply_save_exports(plan, self.root)
        retained = self.root / plan["payloads"][0]["destination"]
        retained.write_bytes(b"corrupt retained copy")
        with self.assertRaisesRegex(ValueError, "retained save changed"):
            verify_save_exports(plan, self.root)
        retained.write_bytes(self.payload)
        self.write("mlm/runtime/main/new.scn", b"another independent save")
        with self.assertRaisesRegex(ValueError, "candidates changed"):
            verify_save_exports(plan, self.root)

    def test_concurrent_record_edit_refuses_and_generated_retention_is_invalid(self):
        """An edited record cannot erase aliases or certify a copy being deleted.

        @param self Active isolated test.
        @return None; changed preservation authority requires a new reviewed plan.
        """
        plan = self.plan()
        self.write(MIGRATION, b'{"payloads": []}')
        with self.assertRaisesRegex(ValueError, "record changed"):
            apply_save_exports(plan, self.root)
        bad = dict(payloads=[dict(sha256=hashlib.sha256(self.payload).hexdigest(), destination="logs/run/current.sav")])
        self.write(MIGRATION, json.dumps(bad).encode())
        with self.assertRaisesRegex(ValueError, "not a durable owner"):
            self.plan()


if __name__ == "__main__":
    unittest.main(verbosity=2)
