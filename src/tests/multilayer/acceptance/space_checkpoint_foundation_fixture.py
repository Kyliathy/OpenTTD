"""Synthetic qualification transport around independent retained Space save bytes.

@file space_checkpoint_foundation_fixture.py Test-only builder, never actual execution evidence.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group12.
Mock images/process/build records exist only in repository-local disposable roots.
"""

import json
from pathlib import Path
import shutil

import space_checkpoint_load_check as native_fixture
import space_checkpoint_foundation as owner
from space_checkpoint_load import identity
from artifact_paths import create_run_context


class FoundationFixture:
    """Own independent original saves plus explicitly synthetic qualification records.

    @param testcase Parent unittest case responsible for scratch cleanup.
    """

    def __init__(self, testcase):
        """Prepare both independent configurations without launching any image.

        @param self Active synthetic qualification fixture.
        @param testcase Parent test receiving the repository-local cleanup callback.
        @return None; every generated record declares its synthetic fixture role.
        """
        self.native_case = native_fixture.CheckpointLoadChecks("test_wrong_save_and_configuration_fail")
        self.native_case.setUp()
        testcase.addCleanup(self.native_case.doCleanups)
        self.root = self.native_case.root
        # Reuse the earlier test's fixture builder without inheriting its tests
        # or rerunning a previous group's suite as part of this focused burst.
        for relative in owner.QUALIFICATION_SOURCES:
            target = self.root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(owner.ROOT / relative, target)
        self.native_results, self.ordinary_results, self.build_results = {}, {}, {}
        for configuration in owner.CONFIGURATIONS:
            if self.native_case.document["configuration"] != configuration:
                self.native_case.make_receipt(configuration)
            self.make_configuration(configuration)
        self.context = create_run_context("space", "checkpoint-qualification", root=self.root)
        self.destination = self.context.reports / "foundation.json"

    def write(self, path, document):
        """Write one synthetic metadata fixture and return its independent byte identity.

        @param self Active isolated fixture.
        @param path Explicit destination beneath the synthetic root.
        @param document Test metadata, never published to a real user checkpoint.
        @return Repository-relative identity of the exact encoded document.
        """
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
        return identity(path, self.root)

    def make_configuration(self, configuration):
        """Create mock ordinary/PE/build records tied to the independent source save.

        @param self Active isolated fixture.
        @param configuration Explicit native/ordinary build selection represented here.
        @return None; no executable image is run and no real qualification is claimed.
        """
        native = self.native_case.document
        native_path = self.native_case.context.reports / "space-checkpoint-load.json"
        native_reference = self.write(native_path, native)
        self.native_results[configuration] = native_reference
        context = create_run_context("space", "checkpoint-static", root=self.root)
        application = self.root / "build" / configuration / "openttd.exe"
        # The tiny PE fixture independently encodes the already established
        # subsystem2-to3 allowance. All unrelated bytes must remain exact.
        encoded = bytearray(512)
        encoded[60:64] = (128).to_bytes(4, "little")
        encoded[128:132] = b"PE\0\0"
        encoded[220:222] = (2).to_bytes(2, "little")
        application.write_bytes(encoded)
        executable = context.runtime_dir / "openttd_console.exe"
        encoded[220:222] = (3).to_bytes(2, "little")
        executable.write_bytes(encoded)
        source = self.root / native["source"]["path"]
        output = context.saves / "ordinary.sav"
        shutil.copyfile(source, output)
        terminal = context.evidence / "ordinary-synthetic.txt"
        terminal.write_text("Synthetic ordinary paused-load transport fixture; no image executed.\n", encoding="utf-8")
        projection = dict(method="pe_subsystem_2_to_3", application=identity(application, self.root),
                          executable=identity(executable, self.root), dlls={})
        process = dict(_documentation="Owner: r2fc-file-cleanup3.md12; synthetic transport, no execution.",
            exit_status=0, argv=[str(executable), "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-d", "sl=3",
                                  "-c", str(context.config_path), "-g", str(source)],
            inputs=[native["source"]], executable=identity(executable, self.root), configuration=configuration,
            run_id=context.run_id, feature=context.feature, scenario=context.scenario,
            runtime=context.runtime_dir.relative_to(self.root).as_posix(),
            artifacts=[identity(path, self.root) for path in (output, terminal)], transcripts=[identity(terminal, self.root)])
        process_reference = self.write(context.reports / "native-synthetic.json", process)
        ordinary = dict(_documentation="Owner: r2fc-file-cleanup3.md12; synthetic static receipt, no execution.",
            schema_version=1, kind="ordinary_space_checkpoint_restore", complete=True, configuration=configuration, phase="ordinary-static",
            seal=native["seal"], source=native["source"], application=identity(application, self.root), console_application=identity(executable, self.root),
            output=identity(output, self.root), terminal=identity(terminal, self.root), native_process=process_reference,
            native_load=native_reference, run_id=context.run_id,
            source_files=[identity(self.root / path, self.root) for path in owner.STATIC_SOURCE_FILES],
            gameplay_ticks_executed=0, projection=projection)
        self.ordinary_results[configuration] = self.write(context.reports / "space-checkpoint-static.json", ordinary)
        build_context = create_run_context("space", "build-fixture", root=self.root)
        epoch = dict(_documentation="Owner: r2fc-file-cleanup3.md12; synthetic source epoch, no build.",
                     schema_version=1, kind="native_source_epoch", files=[identity(self.root / path, self.root) for path in owner.QUALIFICATION_SOURCES])
        epoch_reference = self.write(build_context.data / "source-epoch.json", epoch)
        profile = dict(_documentation="Owner: r2fc-file-cleanup3.md12; synthetic portable profile, no build.",
                       schema_version=1, kind="portable_native_build_profile")
        profile_reference = self.write(self.root / "src/tests/fixtures/config/multilayer/native-build-profile.json", profile)
        build = dict(_documentation="Owner: r2fc-file-cleanup3.md12; synthetic result, neither target was compiled.",
            schema_version=1, kind="native_build_result", complete=True, configuration=configuration, build_directory="build",
            targets=dict(openttd_surface_capital_test=native["application"], openttd=ordinary["application"]),
            source_epoch=epoch_reference, tool_profile=profile_reference, tools=dict(compiler="synthetic fixture"), options=dict(cxx_standard=20),
            steps=[dict(stage="configure", exit_status=0), dict(stage="build", exit_status=0)])
        self.build_results[configuration] = self.write(build_context.reports / "native-build.json", build)

    def qualify(self):
        """Invoke actual qualification logic using only this test's synthetic transport.

        @param self Active isolated fixture.
        @return Typed scratch foundation; it is removed with the test's fake root.
        """
        return owner.qualify_checkpoint(owner.load_checkpoint(root=self.root), self.native_results,
                                        self.ordinary_results, self.build_results, self.destination)
