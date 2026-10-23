"""Synthetic ordinary reload witnesses around independent retained Space worlds.

@file space_operation_restore_fixture.py Transport fixture, never native execution proof.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group36.
All mutable images, reports and worlds live in the test's repository-local scratch.
"""

import shutil

from artifact_paths import create_run_context
from space_checkpoint import ROOT, CONFIGURATIONS
from space_checkpoint_load import identity
from space_checkpoint_operation_fixture import OperationFixture
from space_operation_restore import SOURCE_FILES, SCENARIO, FLAGS
import space_checkpoint_foundation as owner


class RestoreFixture:
    """Extend the existing explicitly synthetic operation transport with reload roles.

    @param testcase Parent unittest case owning scratch cleanup.
    """

    def __init__(self, testcase):
        """Prepare two synthetic reloads without running an executable.

        @param self Active synthetic reload fixture.
        @param testcase Parent case receiving all scratch cleanup ownership.
        """
        self.operation = OperationFixture(testcase)
        self.root = self.operation.root
        self.documents = {}
        self.contexts = {}
        # Distinct worlds retain independent process/save references even when
        # this small fake PE happens to have identical bytes in both configurations.
        for configuration in CONFIGURATIONS:
            self.make_configuration(configuration)

    def make_configuration(self, configuration):
        """Create current-shaped ordinary evidence using only synthetic process metadata.

        @param self Active synthetic reload fixture.
        @param configuration Independent original/build selection represented by this row.
        """
        operation = self.operation.documents[configuration]
        # Native and ordinary children need distinct project-wide reservations,
        # even though their exact source/output identities bind them together.
        context = create_run_context("space", SCENARIO, root=self.root, run_id=operation["run_id"] + "-ordinary")
        self.contexts[configuration] = context
        build = owner.read(operation["build_result"], self.root)
        epoch = owner.read(build["source_epoch"], self.root)
        files = {reference["path"]: reference for reference in epoch["files"]}
        # New ordinary verification has an explicit source identity too. Preserve
        # existing copied fixture sources and add only the missing current helper.
        for name in SOURCE_FILES:
            target = self.root / name
            if not target.exists():
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, target)
            files[name] = identity(target, self.root)
        epoch["files"] = list(files.values())
        build["source_epoch"] = self.operation.base.write(context.data / "synthetic-source-epoch.json", epoch)
        operation["build_result"] = self.operation.base.write(context.reports / "synthetic-build.json", build)
        application = self.root / build["targets"]["openttd"]["path"]
        encoded = bytearray(application.read_bytes())
        encoded[220:222] = (3).to_bytes(2, "little")
        executable = context.runtime_dir / "openttd_console.exe"
        executable.write_bytes(encoded)
        source = self.root / operation["output"]["path"]
        output = context.saves / "ordinary.sav"
        shutil.copyfile(source, output)
        terminal = context.evidence / "synthetic-ordinary.txt"
        terminal.write_text("Synthetic transport only; no image executed.\ndbg: [sl:3] Synthetic load marker.\n", encoding="utf-8")
        output_reference, terminal_reference = [identity(path, self.root) for path in (output, terminal)]
        process = dict(_documentation=__doc__, exit_status=0, configuration=configuration,
            inputs=[operation["output"]], executable=identity(executable, self.root),
            argv=[str(executable), *FLAGS, str(context.config_path), "-g", str(source)],
            feature="space", scenario=SCENARIO, run_id=context.run_id,
            runtime=context.runtime_dir.relative_to(self.root).as_posix(), artifacts=[output_reference, terminal_reference],
            transcripts=[terminal_reference])
        process_reference = self.operation.base.write(context.reports / "synthetic-process.json", process)
        document = dict(_documentation=__doc__, schema_version=1, kind="ordinary_space_operation_restore", complete=True,
            configuration=configuration, task=operation["task"], phase="ordinary-static", source=operation["output"],
            application=identity(application, self.root), console_application=identity(executable, self.root),
            output=output_reference, terminal=terminal_reference, native_process=process_reference, run_id=context.run_id,
            source_files=[identity(self.root / name, self.root) for name in SOURCE_FILES],
            projection=dict(method="pe_subsystem_2_to_3", application=identity(application, self.root),
                            executable=identity(executable, self.root), dlls={}), gameplay_ticks_executed=0)
        operation["schema_version"] = 2
        self.documents[configuration] = document
        self.rewrite_ordinary(configuration)

    def rewrite_ordinary(self, configuration):
        """Rebind intentionally edited synthetic metadata so guards test semantic tampering.

        @param self Active synthetic reload fixture.
        @param configuration Independent row deliberately mutated by a test.
        """
        context = self.contexts[configuration]
        operation = self.operation.documents[configuration]
        operation["ordinary_result"] = self.operation.base.write(context.reports / "synthetic-ordinary.json", self.documents[configuration])
        target = self.root / self.operation.operations[configuration]["path"]
        self.operation.operations[configuration] = self.operation.base.write(target, operation)

    def publish(self):
        """Invoke the real successor owner with explicit synthetic business validation.

        @param self Active synthetic reload fixture.
        @return Typed scratch successor, granting no actual native/gameplay qualification.
        """
        return self.operation.publish()
