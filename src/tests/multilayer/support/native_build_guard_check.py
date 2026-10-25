"""Exercise selected-build admission, reserve crossing and exact source membership.

@file native_build_guard_check.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
Fixtures persist under test-cache/merge-harness. No compiler, Git write or
recursive cleanup runs; owned-tree behavior is separately checked with real children.
"""
import json
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch
import uuid
import xml.etree.ElementTree as ET

from artifact_paths import RunContext
import native_build as build

ROOT = Path(__file__).resolve().parents[4]


class OwnedBuildRoot:
    """Minimal owned Popen fixture; it cannot name or stop any operating-system process."""

    def __init__(self):
        """Begin as an explicitly live fake compiler root."""
        self.returncode = None
        self.pid = 12345

    def poll(self):
        """Return only this fixture's completion state."""
        return self.returncode

    def wait(self, timeout=None):
        """Reap the controlled fake root or report that it remains live.

        @param timeout Caller budget, retained in the simulated timeout exception.
        @return The explicit fixture exit after its owned stop action.
        """
        if self.returncode is None:
            raise subprocess.TimeoutExpired("owned-fixture", timeout)
        return self.returncode


class NativeBuildGuards(unittest.TestCase):
    """Keep desktop reserves, current source inventory and serial compiler pressure explicit."""

    def setUp(self):
        """Create a persistent contained source/build fixture without cleanup deletion.

        @param self Active admission/epoch case.
        """
        self.root = ROOT / "test-cache/merge-harness" / ("build-" + uuid.uuid4().hex)
        (self.root / "src").mkdir(parents=True)
        (self.root / "CMakeLists.txt").write_text("# @see archi-map-layers.md; identity-only source fixture.\n", encoding="utf-8")
        (self.root / "vcpkg.json").write_text('{"_documentation":"@see r2fc-file-cleanup3.md; identity fixture."}\n', encoding="utf-8")
        (self.root / "source.dat").write_text("@see r2fc-file-cleanup3.md; source identity fixture only.\n", encoding="utf-8")
        self.profile = self.root / "profile.json"
        self.profile.write_text(json.dumps({"_documentation": "@see r2fc-file-cleanup3.md; epoch-only fixture.",
                                           "dependencies": {"source_bundle": {"path": "source.dat"}}}), encoding="utf-8")
        self.context = RunContext(self.root, "build", "guard", "guard-fixture").for_feature("build", "guard")
        self.high = {"physical_total": 32 * build.GIB, "physical_available": 12 * build.GIB,
                     "commit_total": 64 * build.GIB, "commit_available": 24 * build.GIB}

    def stopped(self, child):
        """Retire only the explicit fake Popen object supplied by this case.

        @param self Active owned-root identity check.
        @param child Actual object received by the build owner's stop boundary.
        @return Successful retirement; no real PID is signalled.
        """
        self.assertIs(child, self.child)
        child.returncode = 75
        return True

    def record(self):
        """Read the actual selected-build failure record emitted by this fixture.

        @param self Active case whose run_step owns this report.
        @return Current actual report, including memory and root-reaping fields.
        """
        return json.loads((self.context.reports / "guard.json").read_text(encoding="utf-8"))

    def test_low_opening_reserve_refuses_before_creating_a_child(self):
        """Preserve both physical and commit headroom before a compiler can start.

        @param self Active pre-admission check.
        """
        for resource in ("physical_available", "commit_available"):
            current = dict(self.high, **{resource: 1})
            context = self.context.for_feature("build", resource)
            with patch.object(build, "memory_status", return_value=current), patch.object(build, "launch_owned_process") as start:
                with self.assertRaisesRegex(ValueError, "6GiB physical and 8GiB commit"):
                    build.run_step(context, "guard", ["never-launched"], {})
                start.assert_not_called()
            report = json.loads((context.reports / "guard.json").read_text())
            self.assertIsNone(report["exit_status"])
            self.assertTrue(report["error"])

    def test_crossed_reserve_stops_and_reaps_only_our_root(self):
        """Stop the selected compiler tree at a real observed reserve crossing.

        @param self Active sampled-reserve check with a controlled native owner.
        """
        self.child = OwnedBuildRoot()
        low = dict(self.high, physical_available=build.PHYSICAL_RESERVE - 1)
        with patch.object(build, "memory_status", side_effect=[self.high, low]), patch.object(build, "launch_owned_process", return_value=self.child), patch.object(build, "stop_owned_process_tree", side_effect=self.stopped) as stop:
            with self.assertRaisesRegex(ValueError, "crossing"):
                build.run_step(self.context, "guard", ["owned-compiler"], {})
            stop.assert_called_once_with(self.child)
        self.assertTrue(self.record()["root_reaped"])
        self.assertTrue(self.record()["memory"]["stopped_for_memory"])

    def test_lost_memory_observer_keeps_failed_evidence_and_reaps(self):
        """A failed monitor cannot silently turn an unobserved heavy build into a pass.

        @param self Active observation failure check.
        """
        self.child = OwnedBuildRoot()
        with patch.object(build, "memory_status", side_effect=[self.high, OSError("memory observer unavailable")]), patch.object(build, "launch_owned_process", return_value=self.child), patch.object(build, "stop_owned_process_tree", side_effect=self.stopped):
            with self.assertRaisesRegex(OSError, "observer unavailable"):
                build.run_step(self.context, "guard", ["owned-compiler"], {})
        self.assertTrue(self.record()["root_reaped"])
        self.assertIn("observer unavailable", self.record()["error"])

    def test_timeout_reaps_the_exact_build_root_and_records_failure(self):
        """Retain the timed-out stage rather than leaving a compiler tree or false success.

        @param self Active bounded timeout check.
        """
        self.child = OwnedBuildRoot()
        with patch.object(build, "memory_status", return_value=self.high), patch.object(build, "launch_owned_process", return_value=self.child), patch.object(build.time, "monotonic", side_effect=[0, 2, 2]), patch.object(build, "stop_owned_process_tree", side_effect=self.stopped):
            with self.assertRaises(subprocess.TimeoutExpired):
                build.run_step(self.context, "guard", ["owned-compiler"], {}, timeout=1)
        self.assertTrue(self.record()["root_reaped"])
        self.assertTrue(self.record()["error"])

    def test_unbounded_or_boolean_worker_counts_refuse_before_tool_discovery(self):
        """Admit only the explicit one/two-worker pressure contract.

        @param self Active compiler fan-out boundary check.
        """
        for workers in (0, 3, True):
            with patch.object(build, "load_profile", return_value={}), patch.object(build, "discover_tools") as discover:
                with self.assertRaisesRegex(ValueError, "one or two"):
                    build.build_selected(None, "Debug", ["openttd_tube_test"], root=ROOT, workers=workers)
                discover.assert_not_called()

    def test_added_removed_and_changed_native_source_membership_refuses(self):
        """Detect all source epoch changes without deleting or editing any real source.

        @param self Active complete source inventory check.
        """
        source = self.root / "src/gate.cpp"
        original = "/** @see archi-map-layers.md; identity-only source fixture. */\nint fixture_value;\n"
        source.write_text(original, encoding="utf-8")
        with patch.object(build, "runtime_asset_sources", return_value=[]):
            epoch = build.source_epoch(self.root, self.profile)
            source.write_text(original + "int changed_value;\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "inventory"):
                build.verify_source_epoch(epoch, self.root, self.profile)
            source.write_text(original, encoding="utf-8")
            added = self.root / "src/added.cpp"
            added.write_text(original, encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "inventory"):
                build.verify_source_epoch(epoch, self.root, self.profile)
            added.rename(added.with_suffix(".dat"))
            build.verify_source_epoch(epoch, self.root, self.profile)
            source.rename(source.with_suffix(".dat"))
            with self.assertRaisesRegex(ValueError, "inventory"):
                build.verify_source_epoch(epoch, self.root, self.profile)

    def test_reconfigure_preserves_tree_and_refuses_foreign_generator_source(self):
        """No cache mismatch authorizes CMake's recursive --fresh cleanup.

        @param self Active existing generated-tree preservation boundary check.
        """
        target = self.root / "build"
        target.mkdir()
        sentinel = target / "CMakeFiles/valuable.dat"
        sentinel.parent.mkdir()
        sentinel.write_text("@see r2fc-file-cleanup3.md; preserved generated-tree fixture.")
        cache = target / "CMakeCache.txt"
        cache.write_text("CMAKE_HOME_DIRECTORY:INTERNAL=" + str(self.root) + "\nCMAKE_GENERATOR:INTERNAL=Visual Studio 17 2022\n"
                         "CMAKE_GENERATOR_PLATFORM:INTERNAL=x64\nCMAKE_GENERATOR_TOOLSET:INTERNAL=v143,version=14.44.35207\n")
        profile = dict(generator="Visual Studio 17 2022", architecture="x64", tools=dict(toolset="14.44.35207", windows_sdk="10.0.26100.0"),
                       dependencies=dict(target_triplet="x64-windows-static"), options={})
        tools = {key: dict(path="identity-only-" + key) for key in ("cmake", "visual_studio", "grfcodec", "grfid", "nforenum")}
        command = build.configure_command(profile, tools, self.root / "provider", target, self.root)
        self.assertNotIn("--fresh", command)
        self.assertTrue(sentinel.is_file())
        cache.write_text(cache.read_text().replace("Visual Studio 17 2022", "Foreign generator"))
        with self.assertRaisesRegex(ValueError, "without deleting"):
            build.configure_command(profile, tools, self.root / "provider", target, self.root)

    def test_qualification_catalogue_art_and_independent_contract_changes_refuse(self):
        """Bind every live qualification owner and original art, beyond native translation units.

        @param self Active complete source/art epoch inventory case.
        """
        names = ("src/tests/current_check.py", "src/tests/multilayer/controller.py",
                 "media/multilayer/source/original.png", "media/multilayer/contracts/source.json",
                 "mlm/tools/run/current.py", "mlm/review-producers.json",
                 "src/tests/fixtures/graphics/facts.json")
        # Each owner changes independently so a missing inventory branch cannot
        # hide behind another file's detected difference. Renames model removal
        # without deleting any original or recursively cleaning the fixture.
        for name in names:
            with self.subTest(owner=name):
                path = self.root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                with patch.object(build, "runtime_asset_sources", return_value=[]):
                    before = build.source_epoch(self.root, self.profile)
                    path.write_bytes(b"identity-only fixture; @see archi-historical-debug.md\n")
                    with self.assertRaisesRegex(ValueError, "inventory"):
                        build.verify_source_epoch(before, self.root, self.profile)
                    current = build.source_epoch(self.root, self.profile)
                    path.write_bytes(path.read_bytes() + b"changed\n")
                    with self.assertRaisesRegex(ValueError, "inventory"):
                        build.verify_source_epoch(current, self.root, self.profile)
                    path.rename(path.with_name(path.name + ".held"))
                    with self.assertRaisesRegex(ValueError, "inventory"):
                        build.verify_source_epoch(current, self.root, self.profile)

    def test_generated_python_bytecode_does_not_change_the_authored_art_epoch(self):
        """Keep interpreter-generated cache bytes outside native/art source authority.

        @param self Active authored-source versus generated-bytecode boundary case.
        """
        media = self.root / "media/multilayer"
        media.mkdir(parents=True)
        (media / "source.png").write_bytes(b"identity-only original; @see archi-historical-debug.md\n")
        with patch.object(build, "runtime_asset_sources", return_value=[]):
            epoch = build.source_epoch(self.root, self.profile)
            cache = media / "__pycache__"
            cache.mkdir()
            (cache / "tool.pyc").write_bytes(b"generated interpreter cache")
            (media / "legacy.pyc").write_bytes(b"generated legacy cache")
            (media / "legacy.pyo").write_bytes(b"generated optimized cache")
            build.verify_source_epoch(epoch, self.root, self.profile)

    def test_independent_compiler_process_scheduler_retains_the_worker_bound(self):
        """Changing compiler batching cannot silently enable a second unbounded scheduler.

        @param self Active separate-translation-unit pressure boundary case.
        """
        # Both supported counts must reach the CL and MultiToolTask schedulers;
        # project parallelism stays one independently of the requested worker count.
        for workers in (1, 2):
            command = build.selected_target_command({"cmake": {"path": "cmake-identity"}}, self.root / "build",
                                                    "Debug", "openttd_merge_contract_test", workers)
            self.assertEqual(command[command.index("--parallel") + 1], "1")
            self.assertIn("/nodeReuse:false", command)
            self.assertIn("/p:UseMultiToolTask=true", command)
            self.assertIn(f"/p:MultiProcMaxCount={workers}", command)
            self.assertIn(f"/p:CL_MPCount={workers}", command)

    def project(self, name, sources=(), references=()):
        """Create a contained identity-only CMake-shaped project without running a compiler.

        @param self Active generated-project boundary case.
        @param name Explicit project name beneath this fixture's build owner.
        @param sources Exact literal source items for this project.
        @param references Explicit dependency projects, including deliberate refusal inputs.
        @return Persistent generated project path.
        """
        directory = self.root / "build"
        directory.mkdir(exist_ok=True)
        namespace = "{http://schemas.microsoft.com/developer/msbuild/2003}"
        tree = ET.Element(namespace + "Project")
        tree.append(ET.Comment("@see archi-historical-debug.md; identity-only generated-project fixture."))
        group = ET.SubElement(tree, namespace + "ItemGroup")
        # Each declared item remains explicit so tests can distinguish source
        # selection from accidentally compiling all/default or another project.
        for source in sources:
            ET.SubElement(group, namespace + "ClCompile", Include=str(source))
        for reference in references:
            ET.SubElement(group, namespace + "ProjectReference", Include=str(reference))
        path = directory / (name + ".vcxproj")
        path.write_bytes(ET.tostring(tree, encoding="utf-8", xml_declaration=True))
        return path

    def test_generated_project_order_keeps_shared_codegen_before_consumers(self):
        """Use CMake's actual dependency order once rather than constructing a compiler graph.

        @param self Active generator/shared-project order check.
        """
        generator = self.project("generator")
        shared = self.project("shared", references=[generator])
        self.project("first", references=[shared])
        self.project("second", references=[shared])
        rows = build.generated_project_order(self.root / "build", ["first", "second"], self.root)
        self.assertEqual([row["target"] for row in rows], ["generator", "shared", "first", "second"])

    def test_foreign_project_and_unresolved_selected_sources_refuse(self):
        """Do not turn a generated reference or item-list macro into uncontrolled compilation.

        @param self Active contained-project and literal-source refusal case.
        """
        self.project("foreign", references=[self.root / "outside.vcxproj"])
        with self.assertRaises(ValueError):
            build.generated_project_order(self.root / "build", ["foreign"], self.root)
        self.project("macro", sources=["$(UnresolvedSource)"])
        with self.assertRaisesRegex(ValueError, "source item"):
            build.generated_project_order(self.root / "build", ["macro"], self.root)

    def test_source_burst_response_binds_only_the_requested_sources_and_count(self):
        """Keep explicit file selection and scheduler limits attached to the compile invocation.

        @param self Active response-file isolation check; real MSBuild scoping has its separate probe.
        """
        project = self.project("consumer")
        path = self.context.output("data", "selected.rsp")
        selected = [str(self.root / "src/first.cpp"), str(self.root / "src/second.cpp")]
        reference = build.write_compile_response(path, project, "Debug", selected, 1, self.root)
        text = path.read_text(encoding="utf-8")
        self.assertIn('/p:SelectedFiles="' + ";".join(selected) + '"', text)
        self.assertIn("/p:BuildProjectReferences=false", text)
        self.assertIn("/p:MultiProcMaxCount=1", text)
        self.assertEqual(build.identity(path, self.root), reference)

    def test_invalid_source_burst_counts_refuse_before_tool_discovery(self):
        """Burst sizing cannot admit unbounded or boolean compile selections.

        @param self Active early burst-pressure admission check.
        """
        for count in (-1, 33, True):
            with patch.object(build, "load_profile", return_value={}), patch.object(build, "discover_tools") as discover:
                with self.assertRaisesRegex(ValueError, "source burst"):
                    build.build_selected(None, "Debug", ["openttd_merge_contract_test"], root=ROOT, compile_batch_size=count)
                discover.assert_not_called()

    def test_working_memory_gate_waits_above_reserves_without_starting_a_child(self):
        """A bare reserve is not the compiler's operating budget.

        @param self Active pre-child working-headroom admission check.
        """
        low = dict(self.high, physical_available=build.PHYSICAL_RESERVE + build.GIB // 2)
        with patch.object(build, "memory_status", side_effect=[low, self.high]), patch.object(build.time, "sleep") as wait, patch.object(build, "launch_owned_process") as start:
            reference = build.wait_for_compile_headroom(self.context, "working-memory", 1)
        document = json.loads((self.root / reference["path"]).read_text())
        self.assertTrue(document["admitted"])
        self.assertEqual(document["physical_reserve"], 6 * build.GIB)
        self.assertEqual(document["working_budget"], 2 * build.GIB)
        self.assertEqual(len(document["samples"]), 2)
        wait.assert_called_once()
        start.assert_not_called()

    def test_working_memory_timeout_retains_refusal_without_a_compiler(self):
        """A lost operating budget cannot turn repeated unsafe launches into progress.

        @param self Active bounded resource-wait refusal case.
        """
        low = dict(self.high, physical_available=build.PHYSICAL_RESERVE + build.GIB // 2)
        with patch.object(build, "memory_status", return_value=low), patch.object(build, "launch_owned_process") as start:
            with self.assertRaisesRegex(ValueError, "working headroom"):
                build.run_compile_step(self.context, "working-timeout", ["never-launched"], {}, 1, admission_timeout=0)
        document = json.loads((self.context.reports / "working-timeout-admission.json").read_text())
        self.assertFalse(document["admitted"])
        self.assertFalse((self.context.reports / "working-timeout.json").exists())
        start.assert_not_called()


if __name__ == "__main__":
    unittest.main(verbosity=2)
