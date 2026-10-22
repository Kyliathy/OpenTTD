"""Focused current-invocation compile sharing and stale-image refusal checks.

@file artifact_shared_build_check.py Real local identity checks; no compiler or gameplay.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups18C/19D.
"""

import json
from pathlib import Path
import tempfile
import unittest

import artifact_rebuild as rebuild


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
        self.source = self.root / "native.cpp"
        self.profile = self.root / "profile.json"
        self.image = self.root / "Debug/space.exe"
        self.image.parent.mkdir()
        # These deliberately tiny bytes exercise actual SHA closure rather
        # than replacing the identity validator with a mocked success outcome.
        for path, data in ((self.source, b"source"), (self.profile, b"profile"), (self.image, b"image")):
            path.write_bytes(data)
        epoch = self.root / "epoch.json"
        epoch.write_text(json.dumps(dict(files=[rebuild.identity(self.source, self.root)])), encoding="utf-8")
        self.result = self.root / "build.json"
        document = dict(kind="native_build_result", complete=True, configuration="Debug",
                        targets=dict(space=rebuild.identity(self.image, self.root),
                                     ground=rebuild.identity(self.image, self.root)),
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


if __name__ == "__main__":
    unittest.main()
