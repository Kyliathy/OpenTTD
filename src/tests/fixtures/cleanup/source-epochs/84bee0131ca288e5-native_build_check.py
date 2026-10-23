"""Focused portable profile/dependency safety checks without application builds.

@file native_build_check.py Independent fault cases for the bounded native bootstrap.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group13.
"""

import copy
import io
import json
from pathlib import Path
import shutil
import tarfile
import tempfile
import unittest
from unittest.mock import patch

import native_build as build
import native_build_dependencies as dependencies


class NativeBuildChecks(unittest.TestCase):
    """Keep wrong tools, incomplete dependencies and redirected exports from compiling.

    @see r2fc-file-cleanup3.md Group13 portable prerequisites and containment.
    """

    def setUp(self):
        """Prepare one repository-local source root and independent profile copy.

        @param self Active isolated test.
        @return None; all test mutations remain beneath the disposable cache owner.
        """
        cache = build.ROOT / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="build-contract-", dir=cache)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / "src").mkdir()
        (self.root / "CMakeLists.txt").write_text("# @see r2fc-file-cleanup3.md13; synthetic root.\n", encoding="utf-8")
        shutil.copyfile(build.ROOT / "vcpkg.json", self.root / "vcpkg.json")
        self.profile_path = self.root / build.PROFILE
        self.profile_path.parent.mkdir(parents=True)
        self.profile = build.load_profile()
        self.write_profile(self.profile)
        bundle = self.root / self.profile["dependencies"]["source_bundle"]["path"]
        shutil.copyfile(build.ROOT / self.profile["dependencies"]["source_bundle"]["path"], bundle)

    def write_profile(self, document):
        """Write only the test's explicitly selected portable profile.

        @param self Active isolated test.
        @param document Candidate profile for independent schema/pin refusal.
        @return None; the real versioned profile remains unchanged.
        """
        self.profile_path.write_text(json.dumps(document), encoding="utf-8")

    def test_missing_foreign_and_unbounded_profiles_fail(self):
        """Reject missing profiles and changes to platform, worker or source baseline policy.

        @param self Active isolated test.
        @return None; failures occur before any tool is launched or run reserved.
        """
        with self.assertRaises(FileNotFoundError):
            build.load_profile("absent.json", self.root)
        for field, value in (("platform", "foreign"), ("maximum_workers", 8), ("msbuild_node_reuse", True)):
            self.write_profile(dict(self.profile, **{field: value}))
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "profile"):
                build.load_profile(root=self.root)
        changed = copy.deepcopy(self.profile)
        changed["dependencies"]["manifest_baseline"] = "0" * 40
        self.write_profile(changed)
        with self.assertRaisesRegex(ValueError, "baseline"):
            build.load_profile(root=self.root)

    def test_installed_status_cannot_hide_missing_or_changed_packages(self):
        """Use independent package records to detect version and physical payload loss.

        @param self Active isolated test.
        @return None; no existing workstation dependency is mutated.
        """
        installed = self.root / "build/vcpkg_installed"
        self.assertIsNone(dependencies.verify_installed(self.profile, installed, self.root))
        info = installed / "vcpkg/info"
        info.mkdir(parents=True)
        row = dict(name="zlib", version="1.3.2", port_version=0, triplet="x64-windows-static")
        profile = dict(dependencies=dict(packages=[row]))
        status = installed / "vcpkg/status"
        status.write_text("Package: zlib\nVersion: 1.3.2\nArchitecture: x64-windows-static\nStatus: install ok installed\n", encoding="utf-8")
        (info / "zlib_1.3.2_x64-windows-static.list").write_text("x64-windows-static/include/zlib.h\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "payload missing"):
            dependencies.verify_installed(profile, installed, self.root)
        header = installed / "x64-windows-static/include/zlib.h"
        header.parent.mkdir(parents=True)
        header.write_text("/* @see r2fc-file-cleanup3.md13; synthetic dependency payload. */\n", encoding="utf-8")
        self.assertEqual(dependencies.verify_installed(profile, installed, self.root)["payload_files"], 1)
        partial = dict(dependencies=dict(packages=[row, dict(row, name="missing-transitive")]))
        self.assertIsNone(dependencies.verify_installed(partial, installed, self.root, allow_partial=True))
        with self.assertRaisesRegex(ValueError, "versions differ"):
            dependencies.verify_installed(partial, installed, self.root)
        row["version"] = "other"
        with self.assertRaisesRegex(ValueError, "versions differ"):
            dependencies.verify_installed(profile, installed, self.root)

    def test_dependency_source_bundle_is_independent_and_hash_bound(self):
        """Verify retained recipes without an external Git checkout and reject changed bytes.

        @param self Active isolated test with exact immutable source fixture copy.
        @return None; a changed recipe archive cannot start dependency compilation.
        """
        scripts, ports = dependencies.source_archives(self.profile, self.root)
        self.assertGreater(len(scripts), 0)
        self.assertEqual(len(ports), 13)
        bundle = self.root / self.profile["dependencies"]["source_bundle"]["path"]
        bundle.write_bytes(bundle.read_bytes() + b"tampered")
        with self.assertRaisesRegex(ValueError, "source bundle changed"):
            build.load_profile(root=self.root)

    def test_archive_traversal_and_links_fail_before_export(self):
        """A tar member cannot redirect a pinned port export outside its local owner.

        @param self Active isolated test.
        @return None; rejected archives leave no partially exported payload.
        """
        for name, kind in (("../escape", tarfile.REGTYPE), ("linked", tarfile.SYMTYPE)):
            buffer = io.BytesIO()
            with tarfile.open(fileobj=buffer, mode="w") as archive:
                member = tarfile.TarInfo(name)
                member.type = kind
                member.linkname = "../../escape"
                archive.addfile(member)
            destination = self.root / "build/provider"
            with self.subTest(name=name), self.assertRaises(ValueError):
                dependencies.unpack_archive(buffer.getvalue(), destination, self.root)
            self.assertFalse(destination.exists())

    def test_build_owners_source_epochs_and_dependency_destinations(self):
        """Keep build state disposable and immutable archived source outside live epochs.

        @param self Active isolated test.
        @return None; all command checks inspect structured arguments without execution.
        """
        with self.assertRaisesRegex(ValueError, "disposable owner"):
            build.build_directory(self.profile, self.root, "src/generated-build")
        archive = self.root / "src/tests/fixtures/old/source.cpp"
        archive.parent.mkdir(parents=True)
        archive.write_text("// @see r2fc-file-cleanup3.md13; historical fixture data.\n", encoding="utf-8")
        live = self.root / "src/live.cpp"
        live.write_text("// @see r2fc-file-cleanup3.md13; synthetic live source.\n", encoding="utf-8")
        epoch = build.source_epoch(self.root)
        paths = {row["path"] for row in epoch["files"]}
        self.assertIn("src/live.cpp", paths)
        self.assertNotIn("src/tests/fixtures/old/source.cpp", paths)
        command = dependencies.dependency_command(self.profile, self.root / "build/provider", self.root / "build")
        self.assertIn("--classic", command)
        self.assertIn("--binarysource=clear", command)
        self.assertTrue(all(str(self.root / "build") in value for value in command
                            if value.startswith(("--x-install-root=", "--x-buildtrees-root=", "--downloads-root=", "--x-packages-root="))))

    def test_missing_and_wrong_tools_stop_before_process_builds(self):
        """Fail discovery on unavailable prerequisites and actual mismatched version output.

        @param self Active isolated test.
        @return None; no compiler/configure/build process may execute.
        """
        with self.assertRaisesRegex(ValueError, "prerequisite unavailable"):
            build.executable(self.root / "missing.exe", "test compiler")
        # Existing-file/tool lookup is synthetic here; the version text is an
        # independently wrong observation which must stop before any build.
        with patch.object(build, "executable", return_value=self.root / "tool.exe"), \
                patch.object(Path, "is_file", return_value=True), \
                patch.object(build, "probe", return_value="cmake version 0.0"), \
                self.assertRaisesRegex(ValueError, "required cmake version"):
            build.discover_tools(self.profile, dict(visual_studio=self.root, vcpkg=self.root,
                                                   cmake=self.root, git=self.root, patch=self.root))


if __name__ == "__main__":
    unittest.main(verbosity=2)
