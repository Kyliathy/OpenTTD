"""Actual bounded configure/compile and GNU patch checks, without game code.

@file native_build_bootstrap_check.py Explicit tools are rediscovered and version checked.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group13.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group30 cold patch line endings.
Rebuild tools metadata through native_build --inspect; this runner requires the
same explicit external prerequisite paths and writes only disposable test owners.
"""

import argparse
import os
from pathlib import Path
import tempfile
import unittest

import native_build as build
from artifact_paths import create_run_context

TOOLS = None


class NativeBootstrapChecks(unittest.TestCase):
    """Exercise real compiler and patch tools through bounded generated fixtures.

    @see r2fc-file-cleanup3.md13; this never qualifies the OpenTTD application.
    """

    def setUp(self):
        """Allocate repository-local scratch and current-run diagnostic ownership.

        @param self Active actual-tool fixture.
        @return None; no external installation or gameplay process is modified.
        """
        cache = build.ROOT / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="native-bootstrap-", dir=cache)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.context = create_run_context("cleanup", "build-bootstrap", root=build.ROOT)
        self.environment = dict(os.environ, TEMP=str(self.root), TMP=str(self.root),
                                MSBUILDDISABLENODEREUSE="1", GIT_OPTIONAL_LOCKS="0")

    def test_missing_build_configures_and_compiles_a_small_existing_probe(self):
        """Prove portable configure works from an absent build without a transferred cache.

        @param self Active bounded actual-tool fixture.
        @return None; only the existing hash utility is compiled, with no game sources.
        """
        profile = build.load_profile()
        provider = self.root / "provider"
        toolchain = provider / "scripts/buildsystems/vcpkg.cmake"
        toolchain.parent.mkdir(parents=True)
        toolchain.write_text("# @see r2fc-file-cleanup3.md13; no dependencies needed by this tool-only fixture.\n", encoding="utf-8")
        source = build.ROOT / "src/tests/multilayer/support/input_sha256_check.cpp"
        (self.root / "CMakeLists.txt").write_text(
            "# @see r2fc-file-cleanup3.md13; actual existing utility, no application.\n"
            "cmake_minimum_required(VERSION 3.24)\nproject(native_bootstrap LANGUAGES CXX)\n"
            "set(CMAKE_CXX_STANDARD 20)\n"
            f'add_executable(input_sha256_check "{source.as_posix()}")\n'
            'target_precompile_headers(input_sha256_check PRIVATE <cstdint>)\n', encoding="utf-8")
        destination = self.root / "build"
        self.assertFalse(destination.exists())
        configure = build.run_step(self.context, "configure-probe",
            build.configure_command(profile, TOOLS, provider, destination, self.root), self.environment, timeout=90)
        compile_result = build.run_step(self.context, "compile-probe",
            [TOOLS["cmake"]["path"], "--build", str(destination), "--config", "RelWithDebInfo",
             "--target", "input_sha256_check", "--parallel", "2", "--", "/nodeReuse:false", "/p:CL_MPCount=2"],
            self.environment, timeout=90)
        self.assertEqual(configure["exit_status"], 0)
        self.assertEqual(compile_result["exit_status"], 0)
        self.assertTrue((destination / "RelWithDebInfo/input_sha256_check.exe").is_file())
        precompiled_header = destination / "CMakeFiles/input_sha256_check.dir/RelWithDebInfo/cmake_pch.hxx"
        compiled_object = destination / "input_sha256_check.dir/RelWithDebInfo/cmake_pch.obj"
        self.assertTrue(compiled_object.is_file())
        before = (precompiled_header.stat().st_mtime_ns, compiled_object.stat().st_mtime_ns)
        command = build.configure_command(profile, TOOLS, provider, destination, self.root)
        self.assertNotIn("--fresh", command)
        build.run_step(self.context, "matching-configure-probe", command, self.environment, timeout=90)
        build.run_step(self.context, "incremental-compile-probe", compile_result["argv"], self.environment, timeout=90)
        self.assertEqual(before, (precompiled_header.stat().st_mtime_ns, compiled_object.stat().st_mtime_ns))
        print("Actual tool-only configure/build evidence:", self.context.directory)

    def test_original_patch_bytes_apply_without_git_and_bad_patch_fails(self):
        """Exercise real GNU patch success and failure through the actual provider adapter.

        @param self Active repository-local original-patch fixture.
        @return None; no Git command, source checkout or game is altered.
        """
        source = self.root / "source"
        source.mkdir()
        sample = source / "sample.txt"
        sample.write_bytes(b"old value\n")
        patch_file = self.root / "change.patch"
        # The actual libdisasm cold source is LF and its pinned patch is CRLF.
        # Exercise that mismatch rather than letting Windows give both CRLF.
        patch_bytes = b"--- a/sample.txt\r\n+++ b/sample.txt\r\n@@ -1 +1 @@\r\n-old value\r\n+new value\r\n"
        patch_file.write_bytes(patch_bytes)
        helper = self.root / "apply.cmake"
        template = Path(build.__file__).with_name("native_build_apply_patches.cmake.in").read_text(encoding="utf-8")
        helper.write_text(template.replace("@PATCH_EXECUTABLE@", Path(TOOLS["patch"]["path"]).as_posix()), encoding="utf-8")
        command = [TOOLS["cmake"]["path"], "-DPATCH_HELPER=" + str(helper), "-DSOURCE_PATH=" + str(source),
                   "-DPATCH_PATH=" + str(patch_file), "-DCURRENT_BUILDTREES_DIR=" + str(self.root),
                   "-DCURRENT_PORT_DIR=" + str(self.root), "-DTARGET_TRIPLET=x64-windows-static",
                   "-P", str(Path(build.__file__).with_name("native_build_patch_fixture.cmake"))]
        build.run_step(self.context, "patch-probe", command, self.environment, timeout=60)
        self.assertEqual(sample.read_text(encoding="utf-8"), "new value\n")
        self.assertEqual(patch_file.read_bytes(), patch_bytes)
        with self.assertRaisesRegex(ValueError, "stage failed"):
            build.run_step(self.context, "bad-patch-probe", command, self.environment, timeout=60)
        self.assertTrue((self.context.reports / "bad-patch-probe.json").is_file())


def main():
    """Discover explicit real prerequisite tools, then run only this bounded fixture suite.

    @return None; command status preserves discovery, configure, compile and test failures.
    """
    global TOOLS
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vcpkg", type=Path, required=True)
    parser.add_argument("--grfcodec", type=Path, required=True)
    parser.add_argument("--visual-studio", type=Path)
    args = parser.parse_args()
    overrides = {name: value for name, value in vars(args).items() if value is not None}
    TOOLS = build.discover_tools(build.load_profile(), overrides)
    unittest.main(argv=[__file__], verbosity=2)


if __name__ == "__main__":
    main()
