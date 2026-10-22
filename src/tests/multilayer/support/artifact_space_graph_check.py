"""Focused metadata traversal checks; synthetic saves are never loaded natively.

@file artifact_space_graph_check.py Tests graph integrity independently of old gameplay.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group3.
"""

import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from artifact_inventory import trace_references
from artifact_paths import repository_root
from artifact_space_graph import graph_cycles, reference_edges


class SpaceGraphTests(unittest.TestCase):
    """Exercise exact roots, alias authorities and graph topology in an owned checkout.

    @param self Test instance whose scratch files remain within repository test-cache.
    """

    def setUp(self):
        """Create an isolated source shape and a linked two-configuration checkpoint.

        @param self Active isolated graph check.
        @return None; register removal of only this newly allocated scratch tree.
        """
        scratch = repository_root() / "test-cache/cleanup/graph-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / "src").mkdir()
        self.bytes("CMakeLists.txt", b"# Owner: r2fc-file-cleanup3.md; synthetic graph checkout.\n")
        self.saved = self.bytes("mlm/reviews/pair/original.sav", b"OTTNsynthetic static world")
        self.application = self.bytes("build/current.exe", b"synthetic historical executable identity")
        self.operation = self.document("logs/operation.json", dict(complete=True, configuration="Debug",
            operation=dict(source=self.saved, before=self.saved, output=self.saved, application=self.application)))
        self.archive = self.bytes("test-cache/retained/old.exe", b"old application epoch")
        self.product = dict(source=dict(path="build/old.exe", sha256=self.archive["sha256"]), retained=self.archive)
        self.products = self.document("test-cache/retained.json", dict(products=[self.product]))
        self.bound = self.document("logs/bound.json", dict(complete=True, products=self.products,
            configurations=[dict(configuration=name, output=self.saved, receipt=self.operation,
                observation=dict(cash=182302)) for name in ("Debug", "RelWithDebInfo")]))

    def bytes(self, relative, content):
        """Write exact synthetic bytes and return their independent identity.

        @param self Active test root.
        @param relative Explicit fixture path beneath that root.
        @param content Synthetic immutable bytes.
        @return Path/SHA256 identity calculated without using the traced producer.
        """
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        return dict(path=relative, sha256=hashlib.sha256(content).hexdigest())

    def document(self, relative, value):
        """Write a source-documented synthetic metadata authority.

        @param self Active test root.
        @param relative Explicit JSON path beneath that root.
        @param value Typed metadata fields for the selected adapter.
        @return Exact serialized fixture identity.
        """
        return self.bytes(relative, json.dumps(dict(_documentation="Owner: r2fc-file-cleanup3.md; synthetic metadata fixture.", **value)).encode())

    def test_selected_configuration_keeps_pair_and_observations(self):
        """A selected history traversal still exposes both final originals and expectations.

        @param self Active isolated test.
        @return None; verify zero execution and exact pair linkage.
        """
        result = trace_references(self.bound, self.root, "Debug")
        self.assertTrue(result["complete"], result["errors"])
        self.assertEqual(len(result["checkpoints"]), 2)
        self.assertEqual(result["gameplay_ticks_executed"], 0)
        self.assertTrue(all(row["observation"] == dict(cash=182302) for row in result["checkpoints"]))
        archived = [edge for edge in result["edges"] if edge["input"]["path"] == "build/old.exe"]
        self.assertEqual(archived[0]["verification"], "archived_verified")
        self.assertFalse(archived[0]["required_for_continuation"])

    def test_wrong_hash_is_reported_without_rebasing(self):
        """Changed original save bytes cannot become a fresh expected baseline.

        @param self Active isolated test.
        @return None; current continuation equality remains mandatory.
        """
        (self.root / self.saved["path"]).write_bytes(b"OTTNchanged world")
        result = trace_references(self.bound, self.root)
        self.assertFalse(result["complete"])
        self.assertTrue(any(row["error"] == "changed" for row in result["errors"]))

    def test_missing_nested_receipt_fails_honestly(self):
        """Missing nested metadata must not be hidden by an otherwise valid root.

        @param self Active isolated test.
        @return None; exact missing receipt appears in the failure report.
        """
        (self.root / self.operation["path"]).unlink()
        result = trace_references(self.bound, self.root)
        self.assertFalse(result["complete"])
        self.assertTrue(any(row.get("document") == self.operation["path"] for row in result["errors"]))

    def test_duplicate_aliases_are_shared_but_conflicts_fail(self):
        """Matching aliases are reusable; conflicting physical alias identities fail.

        @param self Active isolated test.
        @return None; the graph never chooses whichever historical hash is convenient.
        """
        self.products = self.document("test-cache/retained.json", dict(products=[self.product, self.product]))
        bound = self.document("logs/duplicates.json", dict(products=self.products,
            configurations=[dict(configuration="Debug", output=self.saved)]))
        self.assertTrue(trace_references(bound, self.root)["complete"])
        conflict = dict(source=dict(path="build/other.exe", sha256="0" * 64),
            retained=dict(path=self.archive["path"], sha256="0" * 64))
        products = self.document("test-cache/conflict.json", dict(products=[self.product, conflict]))
        bound = self.document("logs/conflict.json", dict(products=products,
            configurations=[dict(configuration="Debug", output=self.saved)]))
        result = trace_references(bound, self.root)
        self.assertFalse(result["complete"])
        self.assertTrue(any("conflicting retained alias" in row["error"] for row in result["errors"]))

    def test_cycles_distinguish_shared_predecessors(self):
        """Back-edges fail while a diamond with shared immutable history remains acyclic.

        @param self Active isolated test.
        @return None; traversal is bounded even when references are cyclic.
        """
        a, b, c, d = [(name, "hash", "bound") for name in "abcd"]
        self.assertEqual(graph_cycles({a: [b, c], b: [d], c: [d], d: []}), [])
        self.assertTrue(graph_cycles({a: [b], b: [a]}))

    def test_arbitrary_json_strings_are_not_input_edges(self):
        """Only declared schema fields can select metadata or files.

        @param self Active isolated test.
        @return None; prose and unregistered shapes do not grant authority.
        """
        self.assertEqual(reference_edges(dict(notes="logs/not-a-real-dependency.json"), "leaf"), [])
        with self.assertRaisesRegex(ValueError, "unregistered"):
            reference_edges({}, "invented_schema")

    def test_superseded_ancestor_source_mismatch_stays_historical(self):
        """Current validation follows the immediate flattened foundation only.

        @param self Active isolated test.
        @return None; report old mismatches without certifying them as current proof.
        """
        obsolete = self.bytes("src/obsolete.dat", b"previous epoch")
        ancestor = self.document("logs/ancestor.json", dict(source=obsolete, configurations=[]))
        foundation = self.document("logs/foundation.json", dict(foundation=ancestor,
            products=self.products, configurations=[]))
        bound = self.document("logs/current.json", dict(foundation=foundation,
            products=self.products, configurations=[dict(configuration="Debug", output=self.saved)]))
        (self.root / obsolete["path"]).write_bytes(b"newer source epoch")
        result = trace_references(bound, self.root)
        self.assertTrue(result["complete"], result["errors"])
        self.assertTrue(any(issue["error"] == "changed" for issue in result["historical_issues"]))
        old = next(edge for edge in result["edges"] if edge["input"] == obsolete)
        self.assertEqual(old["verification"], "changed")
        self.assertFalse(old["required_for_current_validation"])

    def test_paired_version_bundle_and_replay_summary_are_metadata_only(self):
        """Read older pair-style epochs and ordinary summaries without executing branches.

        @param self Active isolated test.
        @return None; both declared summary branches and source epochs are traversed.
        """
        epoch = self.document("test-cache/epoch.json", dict(products=[self.product]))
        summary = self.document("logs/summary.json", dict(source_receipt=self.operation,
            restore=self.operation, continuations=[self.operation, self.operation], output=self.saved))
        bound = self.document("logs/summary-bound.json", dict(historical_versions=epoch,
            configurations=[dict(configuration="Debug", receipt=summary, output=self.saved)]))
        result = trace_references(bound, self.root)
        self.assertTrue(result["complete"], result["errors"])
        self.assertEqual(result["gameplay_ticks_executed"], 0)
        self.assertTrue(any(edge["json_pointer"] == "/continuations/1" for edge in result["edges"]))


if __name__ == "__main__":
    unittest.main()
