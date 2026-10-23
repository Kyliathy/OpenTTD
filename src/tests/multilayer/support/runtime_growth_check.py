"""Bound runtime growth while comparing the actual Python and native owners.

@file runtime_growth_check.py Isolated mutable homes and atomic current images.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group25.
@see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
Build artifact_paths_probe.cmd first; outputs and fixture scratch use test-cache.
The native probe links no world implementation and claims no gameplay or pixels.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from artifact_paths import create_run_context, repository_root
from runner import console_executable

APPLICATION = None


class RuntimeGrowthChecks(unittest.TestCase):
    """Check distribution reduction and process isolation against independent bytes.

    @param self Active fixture inside this repository, never a real game home.
    """

    def setUp(self):
        """Prepare a small source distribution with deliberately unused translation.

        @param self Active isolated test.
        @return None; automatic cleanup removes only the reserved scratch root.
        """
        cache = repository_root() / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="runtime-growth-", dir=cache)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / "src").mkdir()
        self.write("CMakeLists.txt", b"# @see r2fc2-file-cleanup3.md; isolated runtime fixture.\n")
        self.write("build/native-art.cfg", b'[misc]\nlanguage = "swedish.lng"\n')
        self.write("build/scripts/game_start.scr", b"original startup")
        self.write("build/baseset/native.grf", b"independent original content")
        # Large unused language makes the size assertion an independent business
        # check: new homes must copy the selected/default packs, not the whole set.
        for name, payload in (("english.lng", b"English"), ("english_US.lng", b"US English"),
                              ("swedish.lng", b"Swedish"), ("unused.lng", b"x" * 8192)):
            self.write("build/lang/" + name, payload)
        environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
        inherited = patch.dict(os.environ, environment, clear=True)
        inherited.start()
        self.addCleanup(inherited.stop)
        self.context = create_run_context("tbm", "growth", root=self.root)

    def write(self, relative, payload):
        """Write independent bytes into the active test's explicit fixture path.

        @param self Active isolated test.
        @param relative Fixture-relative source or mutable runtime spelling.
        @param payload Independent bytes required by this scenario.
        @return Written path beneath this disposable fixture.
        """
        target = self.root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(payload)
        return target

    def native_prepare(self, context):
        """Run actual native preparation in one explicit process home.

        @param self Active isolated test.
        @param context Host/client or ordinary Python-reserved routing identity.
        @return Decoded config path after bounded native process success.
        """
        result = subprocess.run([str(APPLICATION), "runtime"], env=context.environment(),
                                capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr)
        return Path(json.loads(result.stdout)["path"])

    def test_native_python_languages_mutable_isolation_and_repeated_size(self):
        """Both owners copy the required language set and preserve private changes.

        @param self Active isolated test.
        @return None; repeated native/Python preparations add zero duplicate bytes.
        """
        host = self.context.for_process("host")
        client = self.context.for_process("client")
        host.prepare_runtime()
        self.assertEqual(self.native_prepare(client), client.config_path)
        for context in (host, client):
            packs = {p.name: p.read_bytes() for p in (context.runtime_dir / "lang").iterdir()}
            self.assertEqual(packs, {"english.lng": b"English", "english_US.lng": b"US English", "swedish.lng": b"Swedish"})
        (host.runtime_dir / "scripts/game_start.scr").write_bytes(b"private host command")
        (host.runtime_dir / "native-art.cfg").write_bytes(b"[misc]\nlanguage = english.lng\n")
        (host.runtime_dir / "autosave.sav").write_bytes(b"private original save")
        before = {path: (path.read_bytes(), path.stat().st_mtime_ns) for path in host.runtime_dir.rglob("*") if path.is_file()}
        host.prepare_runtime()
        self.native_prepare(host)
        after = {path: (path.read_bytes(), path.stat().st_mtime_ns) for path in host.runtime_dir.rglob("*") if path.is_file()}
        self.assertEqual(after, before)
        self.assertEqual((client.runtime_dir / "scripts/game_start.scr").read_bytes(), b"original startup")
        self.assertEqual((self.root / "build/scripts/game_start.scr").read_bytes(), b"original startup")
        copied_language_bytes = sum(path.stat().st_size for path in (host.runtime_dir / "lang").iterdir())
        self.assertEqual(copied_language_bytes, len(b"EnglishUS EnglishSwedish"))
        self.assertLess(copied_language_bytes, (self.root / "build/lang/unused.lng").stat().st_size)

    def test_both_owners_refuse_language_path_traversal(self):
        """A language setting cannot copy arbitrary files out of the distribution.

        @param self Active isolated test.
        @return None; native and Python preparation refuse the same malformed name.
        """
        self.write("build/native-art.cfg", b"[misc]\nlanguage = ../secret.lng\n")
        with self.assertRaisesRegex(ValueError, "language-pack basename"):
            self.context.prepare_runtime()
        result = subprocess.run([str(APPLICATION), "runtime"], env=self.context.for_process("client").environment(),
                                capture_output=True, text=True, timeout=15)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("language-pack basename", result.stderr)

    def test_bom_and_empty_language_have_native_python_parity(self):
        """Valid BOM and empty-language templates keep the established fallback.

        @param self Active isolated test.
        @return None; both owners select the same bytes for both config forms.
        """
        self.write("build/native-art.cfg", b'\xef\xbb\xbf[misc]\nlanguage = "swedish.lng"\n')
        self.native_prepare(self.context.for_process("bom-native"))
        self.context.for_process("bom-python").prepare_runtime()
        self.write("build/native-art.cfg", b'[misc]\nlanguage = "" ; native fallback\n')
        self.native_prepare(self.context.for_process("empty-native"))
        self.context.for_process("empty-python").prepare_runtime()
        # Expected pack names are authored here rather than derived from either
        # implementation; agreement alone must not bless a shared selection bug.
        for process, expected in (("bom-native", {"english.lng", "english_US.lng", "swedish.lng"}),
                                  ("bom-python", {"english.lng", "english_US.lng", "swedish.lng"}),
                                  ("empty-native", {"english.lng", "english_US.lng"}),
                                  ("empty-python", {"english.lng", "english_US.lng"})):
            actual = {p.name for p in (self.context.for_process(process).runtime_dir / "lang").iterdir()}
            self.assertEqual(actual, expected)

    @unittest.skipUnless(os.name == "nt", "Windows console projection")
    def test_repeated_console_and_dll_reuse_preserves_source_and_linked_peers(self):
        """Unchanged images reuse entries; changed images cannot mutate peer links.

        @param self Active isolated test with a synthetic PE, never an executable game.
        @return None; source, historical peers and repeated image timestamps stay exact.
        """
        encoded = bytearray(512)
        encoded[60:64] = (128).to_bytes(4, "little")
        encoded[128:132] = b"PE\0\0"
        encoded[220:222] = (2).to_bytes(2, "little")
        source = self.write("build/RelWithDebInfo/openttd.exe", encoded)
        dependency = self.write("build/RelWithDebInfo/native.dll", b"independent DLL")
        image = console_executable(self.context, source)
        dll = self.context.runtime_dir / dependency.name
        previous = {path: (path.read_bytes(), path.stat().st_mtime_ns) for path in (image, dll)}
        console_executable(self.context, source)
        self.assertEqual(previous, {path: (path.read_bytes(), path.stat().st_mtime_ns) for path in (image, dll)})
        peer = self.context.for_process("peer")
        peer.prepare_runtime()
        os.link(image, peer.runtime_dir / image.name)
        os.link(dll, peer.runtime_dir / dll.name)
        encoded[-1] = 1
        source.write_bytes(encoded)
        dependency.write_bytes(b"new independent DLL")
        console_executable(self.context, source)
        self.assertEqual((peer.runtime_dir / image.name).read_bytes(), previous[image][0])
        self.assertEqual((peer.runtime_dir / dll.name).read_bytes(), previous[dll][0])
        self.assertEqual(source.read_bytes(), encoded)
        self.assertEqual(dependency.read_bytes(), b"new independent DLL")
        self.assertFalse(list(self.context.runtime_dir.glob("*.tmp")))


def main():
    """Require the actual source-built routing probe for the focused parity burst.

    @return None; unittest reports fixture checks without a gameplay claim.
    """
    global APPLICATION
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--application", type=Path, required=True)
    args, remaining = parser.parse_known_args()
    APPLICATION = args.application.resolve()
    unittest.main(argv=[sys.argv[0], *remaining], verbosity=2)


if __name__ == "__main__":
    main()
