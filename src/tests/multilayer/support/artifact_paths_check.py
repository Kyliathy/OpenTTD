"""Focused behavioral checks for the shared artifact ownership boundary.

@file artifact_paths_check.py No native simulation or full regression suite.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Group 3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
"""

import json
import os
from pathlib import Path
import tempfile
import subprocess
import unittest
from unittest.mock import patch

from artifact_paths import contained, create_run_context, repository_root, resolve_input_path
from runner import collect_native_screenshots, console_executable


class ArtifactPathTests(unittest.TestCase):
    """Exercise ownership with disposable source roots inside this repository.

    Each case owns only its TemporaryDirectory; real fixtures/media remain read-only.
    """

    def setUp(self):
        """Prepare an isolated source shape without writing a real fixture.

        @return None; register cleanup only for the newly allocated scratch tree.
        """
        scratch = repository_root() / "mlm/runtime/routing-checks"
        scratch.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / "src/tests/fixtures/haulage/parking").mkdir(parents=True)
        (self.root / "CMakeLists.txt").write_text("# disposable routing test checkout\n", encoding="utf-8")
        self.process_environment = dict(os.environ)
        self.environment = patch.dict(os.environ, {}, clear=True)
        self.environment.start()
        self.addCleanup(self.environment.stop)

    def test_mixed_features_and_repeat_runs_keep_original_bytes(self):
        """Capture identical basenames in separate feature/run owners.

        @return None; assert an additional run cannot replace earlier captures.
        """
        first = create_run_context("tbm", "planning", root=self.root)
        second_feature = first.for_feature("geology", "survey")
        repeated = create_run_context("tbm", "planning", root=self.root)
        first.output("images", "frame.png").write_bytes(b"first capture")
        second_feature.output("images", "frame.png").write_bytes(b"survey capture")
        repeated.output("images", "frame.png").write_bytes(b"repeat capture")
        self.assertEqual(first.run_id, second_feature.run_id)
        self.assertNotEqual(first.run_id, repeated.run_id)
        self.assertEqual(first.output("images", "frame.png").read_bytes(), b"first capture")
        self.assertEqual(first.runtime_dir, second_feature.runtime_dir)
        self.assertEqual(len(first.capture_paths("frame.png")), 2)
        self.assertNotIn(repeated.output("images", "frame.png"), first.capture_paths("frame.png"))
        self.assertFalse((self.root / "docs").exists())

    def test_duplicate_reservation_and_invalid_names_refuse(self):
        """Reject caller-selected collisions and directory-shaped feature names.

        @return None; the original reserved invocation remains intact.
        """
        context = create_run_context("tbm", "planning", run_id="retained", root=self.root)
        with self.assertRaises(FileExistsError):
            create_run_context("tbm", "planning", run_id="retained", root=self.root)
        with self.assertRaises(ValueError):
            create_run_context("../fixtures", "planning", root=self.root)
        with self.assertRaises(ValueError):
            context.output("images", "../../fixture.sav")
        with self.assertRaises(ValueError):
            context.output("fixtures", "original.sav")

    def test_engine_screenshot_collection_preserves_feature_and_collision(self):
        """Collect process-owned engine containers without overwriting prior frames.

        @param self Active isolated ownership check.
        @return None; only transport is exercised, with no native-rendering claim.
        """
        context = create_run_context("geology", "engine-capture", root=self.root)
        source = context.runtime_dir / "screenshot/frame.png"
        source.parent.mkdir(parents=True)
        source.write_bytes(b"engine container identity")
        captures = collect_native_screenshots(context)
        self.assertEqual(captures, [context.images / "frame.png"])
        self.assertEqual(captures[0].read_bytes(), b"engine container identity")
        self.assertFalse(source.exists())
        source.write_bytes(b"later container identity")
        with self.assertRaises(ValueError):
            collect_native_screenshots(context)
        self.assertEqual(captures[0].read_bytes(), b"engine container identity")
        self.assertEqual(source.read_bytes(), b"later container identity")

    def test_nested_and_indexed_inputs_are_read_only_destinations(self):
        """Keep a relocated compatibility input distinct from generated saves.

        @return None; stable input keys resolve without recreating old folders.
        """
        fixture = self.root / "src/tests/fixtures/haulage/parking/original.sav"
        fixture.write_bytes(b"immutable old save")
        key = "src/tests/fixtures/haulage/original.sav"
        manifest = {"inputs": {key: {"path": fixture.relative_to(self.root).as_posix()}}}
        (self.root / "src/tests/fixtures/multilayer-inputs.json").write_text(json.dumps(manifest), encoding="utf-8")
        context = create_run_context("tbm", "parking", root=self.root)
        self.assertEqual(resolve_input_path(key, root=self.root), fixture)
        self.assertEqual(resolve_input_path(fixture, root=self.root), fixture)
        self.assertNotEqual(context.output("saves", "original.sav"), fixture)
        context.output("saves", "original.sav").write_bytes(b"new continuation")
        self.assertEqual(fixture.read_bytes(), b"immutable old save")
        with self.assertRaises(ValueError):
            resolve_input_path(self.root / "CMakeLists.txt", root=self.root)

    def test_peer_runtime_preparation_preserves_templates_and_startup(self):
        """Give host/client separate writable config and content directories.

        @return None; repeating preparation preserves a caller's startup command.
        """
        build = self.root / "build"
        (build / "scripts").mkdir(parents=True)
        (build / "scripts/game_start.scr").write_bytes(b"original startup")
        (build / "native-art.cfg").write_bytes(b"original configuration")
        host = create_run_context("geology", "network", root=self.root).for_process("host")
        client = host.for_process("client")
        host.prepare_runtime()
        client.prepare_runtime()
        self.assertNotEqual(host.runtime_dir, client.runtime_dir)
        (host.runtime_dir / "scripts/game_start.scr").write_bytes(b"host-specific startup")
        host.prepare_runtime()
        self.assertEqual((host.runtime_dir / "scripts/game_start.scr").read_bytes(), b"host-specific startup")
        self.assertEqual((client.runtime_dir / "scripts/game_start.scr").read_bytes(), b"original startup")
        self.assertEqual((build / "scripts/game_start.scr").read_bytes(), b"original startup")
        self.assertEqual((build / "native-art.cfg").read_bytes(), b"original configuration")

    @unittest.skipUnless(os.name == "nt", "Windows console projection")
    def test_console_regeneration_preserves_hard_linked_historical_bytes(self):
        """Regenerate one run without changing another run linked by cleanup.

        @return None; old executable bytes/hash survive and the new run owns a new file.
        """
        source = self.root / "build/RelWithDebInfo/openttd.exe"
        source.parent.mkdir(parents=True)
        (self.root / "build/native-art.cfg").write_bytes(b"test configuration")
        encoded = bytearray(512)
        encoded[60:64] = (128).to_bytes(4, "little")
        encoded[128:132] = b"PE\0\0"
        encoded[220:222] = (2).to_bytes(2, "little")
        source.write_bytes(encoded)
        first = create_run_context("geology", "console", root=self.root)
        second = first.for_process("historical")
        second.prepare_runtime()
        target = console_executable(first, source)
        historical = second.runtime_dir / target.name
        os.link(target, historical)
        original = historical.read_bytes()
        self.assertTrue(os.path.samefile(target, historical))

        encoded[-1] = 1
        source.write_bytes(encoded)
        console_executable(first, source)
        self.assertEqual(historical.read_bytes(), original)
        self.assertNotEqual(target.read_bytes(), original)
        self.assertFalse(os.path.samefile(target, historical))
        self.assertEqual(list(first.runtime_dir.glob("*.tmp")), [])

    def test_reports_bind_feature_artifacts_and_reject_other_runs(self):
        """Permit explicit mixed-feature evidence while refusing unrelated runs.

        @return None; report records exact bytes and declared producer identity.
        """
        context = create_run_context("tbm", "planning", root=self.root)
        survey = context.for_feature("geology", "survey").output("images", "survey.png")
        survey.write_bytes(b"observed pixels")
        report = context.write_report("proof.json", "test-producer", [survey], native_writer="NativeWriter")
        self.assertEqual(json.loads(report.read_text(encoding="utf-8"))["native_writer"], "NativeWriter")
        other = create_run_context("tbm", "planning", root=self.root).output("images", "other.png")
        other.write_bytes(b"different invocation")
        with self.assertRaises(ValueError):
            context.write_report("invalid.json", "test-producer", [other])

    def test_canonical_boundary_rejects_traversal_and_symlink_escape(self):
        """Do not allow lexical or linked paths to redirect an output owner.

        @return None; use a Windows junction when symbolic-link privilege is absent.
        """
        owner = self.root / "mlm/reviews"
        owner.mkdir(parents=True)
        with self.assertRaises(ValueError):
            contained(owner / "../../src/tests/fixtures/replaced.sav", owner)
        link = owner / "redirect"
        try:
            link.symlink_to(self.root / "src/tests/fixtures", target_is_directory=True)
        except OSError as error:
            if os.name != "nt":
                self.skipTest(f"symlink creation unavailable: {error}")
            # Ordinary Windows users can create a junction without symbolic-link
            # privilege. Both ends belong to this disposable test tree, and the
            # constant PowerShell program reads literal paths from its own env.
            environment = dict(self.process_environment, ARTIFACT_LINK_PATH=str(link), ARTIFACT_TARGET_PATH=str(self.root / "src/tests/fixtures"))
            powershell = Path(environment.get("SystemRoot", "C:/Windows")) / "System32/WindowsPowerShell/v1.0/powershell.exe"
            subprocess.run([str(powershell), "-NoProfile", "-NonInteractive", "-Command",
                            "New-Item -ItemType Junction -Path $env:ARTIFACT_LINK_PATH -Target $env:ARTIFACT_TARGET_PATH -ErrorAction Stop | Out-Null"],
                           env=environment, creationflags=subprocess.CREATE_NO_WINDOW, check=True, capture_output=True)
        with self.assertRaises(ValueError):
            contained(link / "original.sav", owner)


if __name__ == "__main__":
    unittest.main()
