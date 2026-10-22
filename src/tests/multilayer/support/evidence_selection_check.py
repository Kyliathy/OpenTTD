"""Focused storage/provenance checks for selected graphics evidence.

@file evidence_selection_check.py Synthetic metadata exercises refusals, not native rendering.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group16.
All synthetic source/image/process bytes stay in disposable repository test-cache.
"""

import copy
from functools import partial
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

from artifact_paths import RunContext, repository_root

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
from evidence import EvidenceSet, bind_capture, identity, selection_document


class EvidenceSelectionTests(unittest.TestCase):
    """Exercise exact current roles separately from explicitly historical inspection."""

    def setUp(self):
        """Allocate a synthetic source/run tree inside the permitted scratch owner.

        @return None; register cleanup only for this test's newly created tree.
        """
        scratch = ROOT / "test-cache/cleanup/evidence-selection"
        scratch.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.context = RunContext(self.root, "art", "selected", "unit-selection")
        self.producer = "src/tests/capture_fixture.py"
        self.writer = "src/tests/capture_fixture.cpp"
        self.catalogue = {"producers": [{"producer": self.producer,
                          "features": [{"feature": "art", "scenario": "selected"}],
                          "native_writers": [self.writer], "test_tags": ["[.unit-fixture]"]}]}
        self.write("mlm/review-producers.json", self.catalogue)
        self.source = self.bytes("src/tests/fixtures/source.sav", b"immutable unit save")
        self.bytes(self.producer, b'"""@file Synthetic wrapper; @see r2fc-file-cleanup3.md16."""\n')
        self.bytes(self.writer, b"/** @file Synthetic writer; @see r2fc-file-cleanup3.md16. */\n")
        self.executable = self.bytes("build/Debug/unit-image.bin", b"synthetic image, never executed")
        self.frame = self.bytes(self.relative(self.context.images / "native.png"), b"synthetic native pixels")
        self.terminal = self.bytes(self.relative(self.context.evidence / "native.log"), b"synthetic process output\n")
        self.diagnostic = self.bytes("logs/unit.log", self.terminal.read_bytes())
        self.process = dict(producer=self.producer, run_id=self.context.run_id, configuration="Debug", feature="art", scenario="selected",
                            exit_status=0, argv=[str(self.executable), "[.unit-fixture]"],
                            executable=identity(self.executable, self.root),
                            transcripts=[identity(self.terminal, self.root)],
                            artifacts=[identity(self.terminal, self.root), identity(self.frame, self.root)])
        self.process_path = self.context.reports / "native-unit.json"
        self.write(self.relative(self.process_path), self.process)
        self.raw = dict(producer=self.producer, feature="art", scenario="selected", run_id=self.context.run_id,
                        configuration="Debug", source=self.relative(self.source), source_sha256=identity(self.source, self.root)["sha256"],
                        log=self.relative(self.diagnostic), frames=[dict(identity(self.frame, self.root),
                        feature="art", scenario="selected", run_id=self.context.run_id)])
        self.raw_path = self.context.reports / "capture.json"
        self.write(self.relative(self.raw_path), self.raw)
        self.capture = bind_capture(self.context, self.raw_path, "Debug")
        self.capture_path = self.context.reports / "selected-capture.json"
        self.write(self.relative(self.capture_path), self.capture)
        self.document = selection_document(self.context, "Debug")
        self.document["reports"]["capture"] = identity(self.capture_path, self.root)
        self.document["evidence"]["capture"] = identity(self.terminal, self.root)
        self.document["diagnostics"]["optional"] = identity(self.diagnostic, self.root)
        self.path = self.context.reports / "selection.json"
        self.save()

    def relative(self, path):
        """Format one synthetic path without introducing a machine-local prefix.

        @param path Concrete path inside this test's source/run tree.
        @return Repository-relative POSIX path string.
        """
        return path.relative_to(self.root).as_posix()

    def bytes(self, relative, data):
        """Write only a newly allocated synthetic test artifact.

        @param relative Path beneath this test's TemporaryDirectory.
        @param data Explicit independent fixture bytes.
        @return Concrete fixture path.
        """
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def write(self, relative, value):
        """Serialize synthetic fixture metadata in explicit UTF8.

        @param relative Path inside the test's disposable source tree.
        @param value JSON-compatible fixture document.
        @return Written fixture path.
        """
        return self.bytes(relative, (json.dumps(value) + "\n").encode("utf-8"))

    def save(self):
        """Write the selected fixture manifest after an intentional mutation.

        @return None; never touches real retained inputs or generated reviews.
        """
        self.write(self.relative(self.path), self.document)

    def selection(self):
        """Open exactly the fixture invocation selected by this case.

        @return Content-bound EvidenceSet using the explicit fixture RunContext.
        """
        return EvidenceSet(self.path, context=self.context, root=self.root)

    def test_current_selection_survives_absent_optional_diagnostics(self):
        """Deleting optional logs must preserve required process/frame validation.

        @return None; actual required evidence, selected output roles and source stay bound.
        """
        self.diagnostic.unlink()
        self.assertEqual(bind_capture(self.context, self.raw_path, "Debug"), self.capture)
        selected = self.selection()
        self.assertEqual(selected.report("capture"), self.capture)
        self.assertEqual(selected.terminal("capture"), self.relative(self.terminal))
        self.assertEqual(selected.output("packed"), self.context.data / "packed-manifest.json")
        self.assertEqual(selected.output("gallery"), self.context.reports / "m1-runtime-gallery.json")

    def test_required_terminal_missing_and_changed_fail_independently(self):
        """Required transcript deletion and byte corruption each refuse.

        @return None; absence of optional logs cannot mask either failure.
        """
        original = self.terminal.read_bytes()
        self.terminal.write_bytes(b"changed required evidence")
        with self.assertRaises(AssertionError):
            self.selection().report("capture")
        self.terminal.unlink()
        with self.assertRaises(FileNotFoundError):
            self.selection().report("capture")
        self.terminal.write_bytes(original)
        self.document["evidence"].clear()
        self.save()
        with self.assertRaisesRegex(AssertionError, "not declared"):
            self.selection().report("capture")

    def test_frame_and_source_changes_fail_independently(self):
        """Edited pixels and an edited immutable source each invalidate selection.

        @return None; check source and frame refusals without rebasing their expectations.
        """
        original = self.frame.read_bytes()
        self.frame.write_bytes(b"altered native frame")
        with self.assertRaisesRegex(AssertionError, "changed"):
            self.selection().report("capture")
        self.frame.write_bytes(original)
        self.source.write_bytes(b"different world")
        with self.assertRaisesRegex(AssertionError, "changed"):
            self.selection().report("capture")

    def test_wrong_run_configuration_and_image_role_refuse(self):
        """Matching hashes cannot admit another run, configuration or image owner.

        @return None; every metadata fault is checked from an independent copy.
        """
        selected = self.selection()
        # Each fault changes a different storage boundary; passing an earlier
        # boundary cannot authorize a foreign native invocation or frame role.
        for key, value in (("run_id", "foreign"), ("configuration", "RelWithDebInfo"), ("feature", "foreign")):
            report = copy.deepcopy(self.capture)
            report[key] = value
            with self.assertRaises((ValueError, AssertionError)):
                selected.validate_capture(report)
        report = copy.deepcopy(self.capture)
        target = self.bytes(self.relative(self.context.data / "native.png"), self.frame.read_bytes())
        report["frames"][0].update(identity(target, self.root))
        with self.assertRaisesRegex(ValueError, "generated role"):
            selected.validate_capture(report)
        other = RunContext(self.root, "art", "selected", "foreign")
        with self.assertRaisesRegex(ValueError, "another recipe"):
            EvidenceSet(self.path, context=other, root=self.root)

    def test_native_writer_source_and_process_provenance_are_required(self):
        """A current capture must retain its declared wrapper, writer and native child.

        @return None; forged routing/failed process metadata cannot replace actual identity.
        """
        report = copy.deepcopy(self.capture)
        report["native_writers"] = [report["producer_source"]]
        with self.assertRaisesRegex(AssertionError, "native writer"):
            self.selection().validate_capture(report)
        self.bytes(self.writer, b"edited writer")
        with self.assertRaisesRegex(AssertionError, "changed"):
            self.selection().validate_capture(self.capture)
        self.bytes(self.writer, b"/** @file Synthetic writer; @see r2fc-file-cleanup3.md16. */\n")
        self.process["exit_status"] = 1
        self.write(self.relative(self.process_path), self.process)
        report = copy.deepcopy(self.capture)
        report["native_processes"] = [identity(self.process_path, self.root)]
        with self.assertRaisesRegex(AssertionError, "failed/foreign"):
            self.selection().validate_capture(report)

    def test_native_conversion_remains_bound_after_ppm_disposal(self):
        """Retain exact native container provenance without requiring disposable PPMs.

        @return None; unchanged conversion passes and a forged native origin refuses.
        """
        source = dict(path=self.relative(self.context.images / "native.ppm"), sha256="a" * 64)
        self.process["artifacts"] = [identity(self.terminal, self.root), source]
        self.write(self.relative(self.process_path), self.process)
        process = identity(self.process_path, self.root)
        row = dict(identity(self.frame, self.root), source_container=source, native_reports=[process], pixels_unchanged=True)
        ledger = self.write(self.relative(self.context.reports / "capture-conversions.jsonl"), row)
        report = copy.deepcopy(self.capture)
        report["native_processes"] = [process]
        report["frames"][0]["conversion"] = identity(ledger, self.root)
        self.selection().validate_capture(report)
        self.diagnostic.unlink()
        converted = bind_capture(self.context, self.raw_path, "Debug")
        self.assertEqual(converted["native_processes"], [process])
        self.selection().validate_capture(converted)
        row["source_container"]["sha256"] = "b" * 64
        self.write(self.relative(ledger), row)
        report["frames"][0]["conversion"] = identity(ledger, self.root)
        with self.assertRaisesRegex(AssertionError, "no selected native writer"):
            self.selection().validate_capture(report)

    def test_mixed_schemas_and_cross_run_outputs_refuse(self):
        """Ambiguous migration metadata and output escapes must fail before use.

        @return None; no old-cache default or fixture publication becomes available.
        """
        self.document["logs"] = {}
        self.save()
        with self.assertRaisesRegex(ValueError, "mixed"):
            self.selection()
        del self.document["logs"]
        self.document["outputs"]["packed"] = "src/tests/fixtures/packed.json"
        self.save()
        with self.assertRaises(ValueError):
            self.selection()
        with self.assertRaisesRegex(ValueError, "explicit"):
            EvidenceSet(None, root=self.root)

    def test_historical_relocation_requires_explicit_read_only_inspection(self):
        """Legacy captures remain historical and cannot publish current outputs.

        @return None; default selection, current context and provenance relabel all refuse.
        """
        owner = "logs/proofs/mlm/native-art/"
        frame = self.bytes(owner + "images/old.png", b"historical pixels")
        terminal = self.bytes(owner + "logs/old.log", b"historical terminal")
        report = dict(source=self.relative(self.source), source_sha256=identity(self.source, self.root)["sha256"],
                      frames=[identity(frame, self.root)], log=self.relative(terminal), provenance={"kind": "historical_relocation"})
        capture = self.write(owner + "reports/old.json", report)
        document = dict(schema_version=1, provenance="historical_relocation", reports={"old": identity(capture, self.root)},
                        logs={"old": identity(terminal, self.root)}, outputs={})
        manifest = self.write(owner + "evidence.json", document)
        with self.assertRaisesRegex(ValueError, "historical only"):
            EvidenceSet(manifest, root=self.root)
        historical = EvidenceSet(manifest, root=self.root, allow_historical=True)
        self.assertEqual(historical.report("old")["provenance"]["kind"], "historical_relocation")
        with self.assertRaisesRegex(ValueError, "cannot publish"):
            historical.output("gallery")
        with self.assertRaises(ValueError):
            EvidenceSet(manifest, context=self.context, root=self.root, allow_historical=True)
        with self.assertRaisesRegex(ValueError, "cannot be mixed"):
            self.selection().validate_capture(report)

    def test_runtime_gallery_and_sound_consumers_share_the_selected_owner(self):
        """Exercise direct consumers without executing renderers or sound decoders.

        @return None; all routing uses the same explicit source/run/terminal selection.
        """
        import activate_m1_art as activation
        import review_m1_art as gallery
        from verify_m1_sound import selected_sound_inputs
        self.document["evidence"]["sound-selection"] = identity(self.terminal, self.root)
        self.save()
        selected = self.selection()
        with patch.object(activation, "EvidenceSet", partial(EvidenceSet, root=self.root)), \
                patch.multiple(activation, _ACTIVE_EVIDENCE=None, PACKED=None, RUNTIME=None), \
                patch.multiple(gallery, REVIEWS=None, GALLERY=None):
            activation.select_evidence(self.path)
            gallery.configure(activation.evidence())
            self.assertEqual(activation.PACKED, selected.output("packed"))
            self.assertEqual(gallery.GALLERY, selected.output("gallery"))
            self.assertEqual(gallery.REVIEWS, self.context.images)
            self.assertEqual(activation.evidence().capture_terminal(self.capture), self.relative(self.terminal))
        capture, output = selected_sound_inputs(selected, self.capture_path, self.relative(self.terminal), self.relative(self.terminal))
        self.assertEqual(capture, self.capture)
        self.assertEqual(output, selected.output("sound"))
        with self.assertRaisesRegex(AssertionError, "sound and actual"):
            selected_sound_inputs(selected, self.capture_path, self.relative(self.diagnostic), self.relative(self.terminal))
        with self.assertRaisesRegex(ValueError, "output differs"):
            selected_sound_inputs(selected, self.capture_path, self.relative(self.terminal), self.relative(self.terminal), self.source)

    def test_orchestration_stops_before_native_generation_when_prerequisites_are_missing(self):
        """A selected run cannot bypass a missing retained approval root.

        @return None; inspect uses the selected configuration and no producer executes.
        """
        import regenerate_art_proofs as proofs
        pending = dict(executable=False, missing_inputs=[{"role": "source-approval-contract"}], pending_groups={"m1-derived-inputs": [17]})
        with patch.object(sys, "argv", ["regenerate_art_proofs.py", "focused", "--manifest", str(self.path)]), \
                patch.object(proofs, "EvidenceSet", partial(EvidenceSet, root=self.root)), \
                patch.object(proofs, "load_recipe", return_value=pending) as inspect, \
                patch.object(proofs, "execute") as execute, \
                patch.dict("os.environ", {}, clear=True):
            with self.assertRaisesRegex(ValueError, "missing/pending"):
                proofs.main()
            inspect.assert_called_once_with("m1-native-loader", "Debug")
            execute.assert_not_called()


if __name__ == "__main__":
    unittest.main(verbosity=2)
