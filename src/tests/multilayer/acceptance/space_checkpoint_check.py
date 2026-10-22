"""Focused seal, transfer and interruption checks for the original Space saves.

@file space_checkpoint_check.py Independent fixture expectations, no native replay.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group8.
Run with python -B; scratch copies and transcripts belong in test-cache/cleanup/.
"""

import copy
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import space_checkpoint as checkpoint


class SpaceCheckpointChecks(unittest.TestCase):
    """Exercise actual saved owners using isolated copies of independent history.

    @param self Test instance owning only repository-local disposable scratch.
    """

    def setUp(self):
        """Copy the small declared fixture closure into a disposable transfer root.

        @param self Active test instance.
        @return None; all later mutations affect only this test's copied inputs.
        """
        cache = checkpoint.ROOT / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="space-seal-", dir=cache)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.path = self.root / checkpoint.OWNER / "checkpoint.json"
        source = checkpoint.load_checkpoint()
        self.document = json.loads(source.manifest_path.read_text(encoding="utf-8"))
        history = json.loads((checkpoint.ROOT / self.document["history"]["path"]).read_text(encoding="utf-8"))
        paths = {source.manifest_path.relative_to(checkpoint.ROOT).as_posix()}
        paths.update(self.document[field]["path"] for field in ("history", "historical_validation", "migration_map"))
        paths.update(row["retained"]["path"] for row in history["live_retained_inputs"])
        paths.update(row[field]["path"] for row in self.document["configurations"].values()
                     for field in ("save", "expected_observation", "expected_chunks"))
        # The fixture copy contains only explicit live dependencies. No ignored
        # old logs, applications or generated review trees are needed to verify.
        for relative in paths:
            target = self.root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(checkpoint.ROOT / relative, target)

    def publish_test_manifest(self, document):
        """Write a synthetic candidate manifest into this test's isolated root.

        @param self Active test instance.
        @param document Explicit mutated contract whose validation is under test.
        @return Typed selection when the candidate passes schema validation.
        """
        self.path.write_text(json.dumps(document), encoding="utf-8")
        return checkpoint.load_checkpoint(root=self.root)

    def test_transferred_closure_verifies_without_writes_or_processes(self):
        """A minimal transfer verifies actual owners without any workstation history.

        @param self Active isolated test.
        @return None; both original hashes and all121 families remain independent.
        """
        with patch.object(Path, "mkdir", side_effect=AssertionError("write forbidden")), \
                patch.object(Path, "write_text", side_effect=AssertionError("write forbidden")), \
                patch.object(Path, "write_bytes", side_effect=AssertionError("write forbidden")), \
                patch("subprocess.Popen", side_effect=AssertionError("native process forbidden")):
            selected = checkpoint.load_checkpoint(configuration="Debug", root=self.root)
            result = checkpoint.verify_checkpoint(selected)
        self.assertTrue(result["complete"])
        self.assertEqual(result["selected_configuration"], "Debug")
        self.assertEqual([row["chunk_families"] for row in result["configurations"]], [121, 121])
        self.assertEqual(len({row["save"]["sha256"] for row in result["configurations"]}), 2)
        self.assertFalse(result["fresh_build_qualified"])

    def test_fresh_import_and_verification_do_not_materialize_runtime(self):
        """Start a fresh Python interpreter with writes blocked before reader import.

        @param self Active isolated test.
        @return None; imports and actual verification pass without runtime setup.
        """
        script = '''
from pathlib import Path
import os
import sys
from unittest.mock import patch
sys.path.insert(0, sys.argv[1])
def refuse_writes(event, arguments):
    """Reject every filesystem mutation before checkpoint imports.

    @param event Python audit event identifying the operation.
    @param arguments Original event arguments, including open flags.
    @return None for reads; attempted writes fail the isolated verification.
    """
    # Catch direct open()/os writes as well as Path convenience methods, so
    # an import cannot hide runtime materialization behind another file API.
    if event == "open" and arguments[2] & (os.O_WRONLY | os.O_RDWR | os.O_CREAT | os.O_TRUNC | os.O_APPEND):
        raise AssertionError("open for write forbidden")
    if event in {"os.mkdir", "os.remove", "os.rename", "os.rmdir", "os.symlink", "os.link", "os.truncate"}:
        raise AssertionError("filesystem mutation forbidden: " + event)
sys.addaudithook(refuse_writes)
with patch.object(Path, "mkdir", side_effect=AssertionError("mkdir forbidden")), \\
     patch.object(Path, "write_text", side_effect=AssertionError("write forbidden")), \\
     patch.object(Path, "write_bytes", side_effect=AssertionError("write forbidden")), \\
     patch("subprocess.Popen", side_effect=AssertionError("native process forbidden")):
    import space_checkpoint
    result = space_checkpoint.verify_checkpoint(space_checkpoint.load_checkpoint(root=Path(sys.argv[2])))
    assert result["gameplay_ticks_executed"] == 0
    assert result["live_history_witnesses"] == 75
'''
        result = subprocess.run([sys.executable, "-B", "-c", script,
                                 str(Path(__file__).resolve().parent), str(self.root)],
                                capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_legacy_unknown_fields_and_configuration_swaps_fail(self):
        """Legacy receipts and cross-configuration shortcuts cannot select saves.

        @param self Active isolated test.
        @return None; invalid contracts fail before owner observation.
        """
        malformed = [dict(complete=True, configuration="Debug", operation={})]
        extra = copy.deepcopy(self.document)
        extra["operation"] = {}
        malformed.append(extra)
        swapped = copy.deepcopy(self.document)
        swapped["configurations"]["Debug"]["save"] = swapped["configurations"]["RelWithDebInfo"]["save"]
        malformed.append(swapped)
        # Each candidate represents a separate consumer mistake. None may
        # silently degrade into the legacy dictionary-based operation API.
        for document in malformed:
            with self.subTest(document=document.get("kind")), self.assertRaises(ValueError):
                self.publish_test_manifest(document)
        with self.assertRaises(TypeError):
            checkpoint.verify_checkpoint(self.document)

    def test_scope_floors_and_fresh_claims_cannot_be_reopened(self):
        """Cleanup metadata cannot grant another tick/window or lower financial gates.

        @param self Active isolated test.
        @return None; schema admission rejects each reopened policy independently.
        """
        candidates = []
        for field, key, value in [("closed_scope", "ended", False), ("closed_scope", "window", 33),
                                  ("financial_policy", "operating_floor", 30000),
                                  ("financial_policy", "further_paid_scope_authorized", True)]:
            document = copy.deepcopy(self.document)
            document[field][key] = value
            candidates.append(document)
        fresh = copy.deepcopy(self.document)
        fresh["fresh_build_qualified"] = True
        candidates.append(fresh)
        for document in candidates:
            with self.subTest(policy=document["financial_policy"]), self.assertRaises(ValueError):
                self.publish_test_manifest(document)

    def test_live_reference_cannot_escape_fixture_owner(self):
        """An apparently durable prefix cannot hide traversal back into logs/build.

        @param self Active isolated test.
        @return None; containment rejects redirected consumer dependencies.
        """
        self.document["history"]["path"] = "src/tests/fixtures/../../../logs/fake.json"
        with self.assertRaisesRegex(ValueError, "canonical"):
            self.publish_test_manifest(self.document)

    def test_tampered_save_and_missing_historical_witness_fail(self):
        """A hash declaration never excuses missing or altered required bytes.

        @param self Active isolated test.
        @return None; the original payload is restored before checking witness loss.
        """
        selected = checkpoint.load_checkpoint(root=self.root)
        save = self.root / self.document["configurations"]["Debug"]["save"]["path"]
        original = save.read_bytes()
        save.write_bytes(original + b"tampered")
        with self.assertRaisesRegex(ValueError, "hash changed"):
            checkpoint.verify_checkpoint(selected)
        save.write_bytes(original)
        history = json.loads((self.root / self.document["history"]["path"]).read_text(encoding="utf-8"))
        (self.root / history["live_retained_inputs"][-1]["retained"]["path"]).unlink()
        with self.assertRaises(FileNotFoundError):
            checkpoint.verify_checkpoint(selected)

    def test_changed_manifest_and_wrong_observer_fail(self):
        """Loaded identity and actual observation are independently enforced.

        @param self Active isolated test.
        @return None; neither a modified seal nor observer drift yields success.
        """
        selected = checkpoint.load_checkpoint(root=self.root)
        self.path.write_text(self.path.read_text(encoding="utf-8") + "\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "manifest changed"):
            checkpoint.verify_checkpoint(selected)
        selected = checkpoint.load_checkpoint(root=self.root)
        with patch.object(checkpoint, "observe", return_value={"company": {"cash": 182302}}), \
                self.assertRaisesRegex(ValueError, "owner/material"):
            checkpoint.verify_checkpoint(selected)

    def test_rehashed_expectation_still_requires_original_independent_receipt(self):
        """Updating a generated expected file/hash cannot bless changed observation data.

        @param self Active isolated test.
        @return None; the retained audit/root remains the independent golden source.
        """
        reference = self.document["configurations"]["Debug"]["expected_observation"]
        path = self.root / reference["path"]
        expected = json.loads(path.read_text(encoding="utf-8"))
        expected["observation"]["company"]["cash"] += 1
        payload = json.dumps(expected).encode("utf-8")
        path.write_bytes(payload)
        reference["sha256"] = hashlib.sha256(payload).hexdigest()
        selected = self.publish_test_manifest(self.document)
        with self.assertRaisesRegex(ValueError, "independent checkpoint observation"):
            checkpoint.verify_checkpoint(selected)

    def test_interrupted_atomic_publication_is_not_loadable(self):
        """A partial rename leaves only an unselectable pending manifest.

        @param self Active isolated test.
        @return None; published payload conflicts are refused without replacement.
        """
        payload = self.path.read_bytes()
        self.path.unlink()
        with patch.object(checkpoint.os, "replace", side_effect=OSError("simulated interruption")), \
                self.assertRaisesRegex(OSError, "interruption"):
            checkpoint._publish_bytes(self.path, payload, self.root)
        with self.assertRaises(FileNotFoundError):
            checkpoint.load_checkpoint(root=self.root)
        pending = self.path.with_name("checkpoint.json.pending")
        with self.assertRaisesRegex(ValueError, "published Space checkpoint"):
            checkpoint.load_checkpoint(pending, root=self.root)
        checkpoint._publish_bytes(self.path, payload, self.root)
        checkpoint.verify_checkpoint(checkpoint.load_checkpoint(root=self.root))
        with self.assertRaisesRegex(ValueError, "conflicting retained payload"):
            checkpoint._publish_bytes(self.path, payload + b"conflict", self.root)
        self.assertEqual(self.path.read_bytes(), payload)


if __name__ == "__main__":
    unittest.main(verbosity=2)
