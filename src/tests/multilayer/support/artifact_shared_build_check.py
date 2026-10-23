"""Focused current-invocation compile sharing and stale-image refusal checks.

@file artifact_shared_build_check.py Real local identity checks; no compiler or gameplay.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups18C/19D.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group28 complete inventory fixtures.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group30 explicit planned/current builds.
"""

import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import artifact_rebuild as rebuild
import native_build


class SharedBuildChecks(unittest.TestCase):
    """Keep compile reuse distinct from independently executed native qualification."""

    def test_late_application_is_included_before_first_native_observer(self):
        """Plan one selected target union before any decoder can observe an executable.

        @param self Active isolated test owner.
        @return None; pure conversions and unrelated catalogue targets add no build scope.
        """
        selection = dict(recipes=dict(
            source=dict(target=None, original_parameters=dict(build_targets=["unrelated"])),
            decoder=dict(target="space", original_parameters={}),
            selector=dict(target="space", original_parameters={}),
            ordinary=dict(target="openttd", original_parameters=dict(build_targets=["space", "openttd"]))))
        self.assertEqual(rebuild.selected_build_targets(selection), ["openttd", "space"])
        self.assertEqual(rebuild.selected_build_targets(dict(recipes=dict(source=selection["recipes"]["source"]))), [])

    def test_selected_target_plan_preserves_explicit_aggregate_requirements(self):
        """Keep every declared selected image while removing duplicate compile requests.

        @param self Active isolated test owner.
        @return None; no native verification result or external build is introduced.
        """
        selection = dict(recipes=dict(aggregate=dict(target="space", original_parameters=dict(build_targets=["space", "ground"])),
                                     later=dict(target="ground", original_parameters={})))
        self.assertEqual(rebuild.selected_build_targets(selection), ["ground", "space"])

    def test_explicit_prebuild_only_adds_selected_targets_and_rejects_pending_graphs(self):
        """Precompile later selected images without executing or accepting their observations.

        @param self Active isolated compile-selection fixture owner.
        @return None; pending graphs and undeclared image scope cannot enter the plan.
        """
        current = dict(recipes=dict(current=dict(target="space", original_parameters={})))
        future = dict(executable=True, recipes=dict(later=dict(target="ground", original_parameters={})))
        with patch.object(rebuild, "load_recipe", return_value=future) as load:
            plan = rebuild.compile_plan(current, "Debug", self.root, ["later", "later"])
        self.assertEqual(plan, dict(targets=["ground", "space"], prebuild_recipes=["later"], provided_build=None))
        load.assert_called_once_with("later", "Debug", self.root)
        with patch.object(rebuild, "load_recipe", return_value=dict(future, executable=False)):
            with self.assertRaisesRegex(ValueError, "pending or missing"):
                rebuild.compile_plan(current, "Debug", self.root, ["later"])
        pure = dict(recipes=dict(source=dict(target=None, original_parameters={})))
        with self.assertRaisesRegex(ValueError, "selected native recipe closure"):
            rebuild.compile_plan(pure, "Debug", self.root, ["later"])

    def test_explicit_current_build_reuse_requires_current_images_source_and_configuration(self):
        """Treat the selected current build as a prerequisite, never a native test pass.

        @param self Active isolated physical build/source/image fixture owner.
        @return None; changed source and another configuration still refuse before execution.
        """
        selection = dict(recipes=dict(current=self.recipe))
        plan = rebuild.compile_plan(selection, "Debug", self.root, build_result=self.result)
        self.assertEqual(plan["provided_build"], self.stage["result"])
        self.assertNotIn("verify", plan)
        with self.assertRaisesRegex(ValueError, "configuration/targets/status"):
            rebuild.compile_plan(selection, "RelWithDebInfo", self.root, build_result=self.result)
        self.source.write_bytes(self.source.read_bytes() + b"changed")
        with self.assertRaisesRegex(ValueError, "source changed"):
            rebuild.compile_plan(selection, "Debug", self.root, build_result=self.result)

    def setUp(self):
        """Create actual source/profile/image files and a bound synthetic build result.

        @param self Active isolated test owner.
        @return None; only this test's repository-local scratch is allocated.
        """
        cache = Path(__file__).resolve().parents[4] / "test-cache/cleanup"
        cache.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(prefix="shared-build-", dir=cache)
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.source = self.root / "src/native.cpp"
        self.source.parent.mkdir()
        self.profile = self.root / "profile.json"
        self.image = self.root / "build/Debug/space.exe"
        self.image.parent.mkdir(parents=True)
        # These deliberately tiny bytes exercise actual SHA closure rather
        # than replacing the identity validator with a mocked success outcome.
        for path, data in ((self.source, b"// @file native.cpp @see r2fc2-file-cleanup3.md; independent source."), (self.profile, b'{"_documentation":"r2fc2-file-cleanup3.md fixture","dependencies":{"source_bundle":{"path":"dependency.bin"}}}'), (self.image, b"image")):
            path.write_bytes(data)
        (self.root / "dependency.bin").write_bytes(b"independent dependency fixture")
        (self.root / "CMakeLists.txt").write_text("# @see r2fc2-file-cleanup3.md; isolated source inventory.\n", encoding="utf-8")
        (self.root / "vcpkg.json").write_text('{"_documentation":"r2fc2-file-cleanup3.md; isolated manifest"}', encoding="utf-8")
        self.ground_image = self.image.with_name("ground.exe")
        self.ground_image.write_bytes(b"independent ground image")
        epoch = self.root / "epoch.json"
        epoch.write_text(json.dumps(native_build.source_epoch(self.root, self.profile)), encoding="utf-8")
        self.result = self.root / "build.json"
        document = dict(kind="native_build_result", complete=True, configuration="Debug",
                        build_directory="build", targets=dict(space=rebuild.identity(self.image, self.root),
                                     ground=rebuild.identity(self.ground_image, self.root)),
                        source_epoch=rebuild.identity(epoch, self.root),
                        tool_profile=rebuild.identity(self.profile, self.root), steps=[dict(exit_status=0)])
        self.result.write_text(json.dumps(document), encoding="utf-8")
        self.stage = dict(status="passed", result=rebuild.identity(self.result, self.root))
        self.state = dict(nodes=dict(first=dict(stages=dict(build=self.stage)), second=dict(stages={})))
        self.recipe = dict(target="ground", original_parameters={})

    def test_same_invocation_subset_keeps_exact_build_identity(self):
        """Share compile prerequisites without copying a previous native test result.

        @param self Active isolated test owner.
        @return None; the exact existing image/result remains authoritative.
        """
        actual = rebuild.shared_current_build(self.state, "Debug", self.recipe, self.root)
        self.assertEqual(actual, dict(status="passed", result=self.stage["result"], shared_from_recipe="first"))
        self.assertNotIn("verify", actual)
        self.assertIsNone(rebuild.shared_current_build(dict(nodes={}), "Debug", self.recipe, self.root))

    def test_incompatible_target_needs_a_distinct_build(self):
        """A different executable cannot borrow an unrelated compile prerequisite.

        @param self Active isolated test owner.
        @return None; a target superset requires its own actual build.
        """
        recipe = dict(target="space", original_parameters=dict(build_targets=["space", "missing"]))
        self.assertIsNone(rebuild.shared_current_build(self.state, "Debug", recipe, self.root))

    def test_changed_source_profile_image_or_result_stops_reuse(self):
        """Refuse compatible stale builds instead of silently replacing earlier images.

        @param self Active isolated test owner.
        @return None; each fault independently reaches the actual identity guard.
        """
        # Restore each independent authority before the next fault so a stale
        # result cannot mask source, tool-profile or executable replacement.
        for path in (self.source, self.profile, self.image, self.result):
            with self.subTest(path=path.name):
                original = path.read_bytes()
                try:
                    path.write_bytes(original + b"changed")
                    with self.assertRaisesRegex(ValueError, "changed"):
                        rebuild.shared_current_build(self.state, "Debug", self.recipe, self.root)
                finally:
                    path.write_bytes(original)

    def test_other_configuration_is_never_a_compile_or_native_pass(self):
        """Reject Debug compile evidence for a RelWithDebInfo native invocation.

        @param self Active isolated test owner.
        @return None; native configuration reuse remains forbidden.
        """
        with self.assertRaisesRegex(ValueError, "configuration/targets/status"):
            rebuild.shared_current_build(self.state, "RelWithDebInfo", self.recipe, self.root)

    def test_added_and_removed_sources_each_refuse_current_build_reuse(self):
        """Complete inventory comparison catches changes that old-file hashing misses.

        @param self Active isolated build/source fixture owner.
        @return None; added and removed files are independent refusal scenarios.
        """
        added = self.root / "src/added.cpp"
        added.write_bytes(b"// @file added.cpp @see r2fc2-file-cleanup3.md; new independent input.")
        with self.assertRaisesRegex(ValueError, "inventory was added/removed"):
            rebuild.shared_current_build(self.state, "Debug", self.recipe, self.root)
        added.unlink()
        self.source.unlink()
        with self.assertRaisesRegex(ValueError, "inventory was added/removed"):
            rebuild.shared_current_build(self.state, "Debug", self.recipe, self.root)

    def test_archived_executable_cannot_be_called_current_even_with_same_hash(self):
        """Current build evidence requires its actual target slot, not a retained EXE.

        @param self Active isolated build/source fixture owner.
        @return None; the historical copy remains present and byte-identical.
        """
        archive = self.root / "logs/old/ground.exe"
        archive.parent.mkdir(parents=True)
        archive.write_bytes(self.ground_image.read_bytes())
        document = json.loads(self.result.read_text())
        document["targets"]["ground"] = rebuild.identity(archive, self.root)
        self.result.write_text(json.dumps(document), encoding="utf-8")
        self.stage["result"] = rebuild.identity(self.result, self.root)
        with self.assertRaisesRegex(ValueError, "archived or foreign"):
            rebuild.shared_current_build(self.state, "Debug", self.recipe, self.root)


if __name__ == "__main__":
    unittest.main()
