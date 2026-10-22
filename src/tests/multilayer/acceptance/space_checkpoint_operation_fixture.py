"""Synthetic operation transport around independent retained Space worlds.

@file space_checkpoint_operation_fixture.py No native execution or gameplay qualification.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group12B.
Only the transport is synthetic; original save bytes and observer expectations
come from the separately retained pre-cleanup boundary.
"""

import json
from pathlib import Path
import shutil

from space_checkpoint import CONFIGURATIONS, POLICY, publish_successor
from space_checkpoint_foundation_fixture import FoundationFixture
from space_checkpoint_load import identity
import space_checkpoint_foundation as owner
from artifact_paths import create_run_context
from savegame_tables import saved_owner_table as table
from space_preview_checks import chunks


def validate_fixture_operation(source, before, output, operation, root):
    """Validate only this test's synthetic zero-tick query; grant no business scope.

    @param source Independently retained predecessor save.
    @param before Synthetic native before snapshot of the same immutable bytes.
    @param output Synthetic native result, required to remain entirely unchanged.
    @param operation Explicit synthetic operation metadata.
    @param root Isolated repository-local fixture root; no producer is executed.
    @return Test-only measured business facts accepted by the real transport schema.
    """
    if chunks(source) != chunks(before) or chunks(source) != chunks(output):
        raise ValueError("fixture query changed saved state")
    return dict(kind="plan11_operation_validation", task=operation["task"], phase=operation["phase"],
                elapsed_ticks=0, authorizes_future_operations=False,
                facts=dict(synthetic_transport=True, unchanged_families=121))


class OperationFixture:
    """Build explicitly synthetic current-operation evidence after a scratch foundation.

    @param testcase Active test owning all temporary files through its cleanup callback.
    """

    def __init__(self, testcase):
        """Prepare exact original worlds and paired fake operation records.

        @param self Active test-only transport builder.
        @param testcase Parent test receiving scratch cleanup ownership.
        @return None; no actual image, game loop, purchase or runtime is launched.
        """
        self.base = FoundationFixture(testcase)
        self.root = self.base.root
        self.predecessor = self.base.qualify()
        self.operations = {}
        self.documents = {}
        self.validator = self.root / "src/tests/multilayer/acceptance/space_checkpoint_operation_fixture.py"
        shutil.copyfile(Path(__file__), self.validator)
        self.native_source = self.root / "src/tests/space_operation_fixture.h"
        self.native_source.write_text("/** @file Synthetic test transport; @see r2fc-file-cleanup3.md12B. */\n"
                                      "// Native selector fixture: [.plan11-capacity-fixture]\n", encoding="utf-8")
        # Both independent configurations share the same pure query contract,
        # but retain their distinct original saves and native process identities.
        for configuration in CONFIGURATIONS:
            self.make_configuration(configuration)
        self.context = create_run_context("space", "checkpoint-successor", root=self.root)
        self.destination = self.context.reports / "operation.json"

    def make_configuration(self, configuration):
        """Write one synthetic result with actual independent saved clock metadata.

        @param self Active scratch transport fixture.
        @param configuration Explicit original world and build selection.
        @return None; every mock process/build document retains its synthetic label.
        """
        source_reference = self.base.native_case.seal["configurations"][configuration]["save"]
        source = self.root / source_reference["path"]
        context = create_run_context("space", "capacity-fixture", root=self.root)
        before, output = context.saves / "before.sav", context.saves / "after.sav"
        shutil.copyfile(source, before)
        shutil.copyfile(source, output)
        expected_reference = self.base.native_case.seal["configurations"][configuration]["expected_observation"]
        expected = owner.read(expected_reference, self.root)["observation"]
        date, revisions = table(source, b"DATE")[0], table(source, b"GLOG")
        last = revisions[max(revisions)]
        fields = dict(task="Plan11/15CSGZVEEI", phase="capacity-query", ticks_before=date["tick_counter"],
            ticks_after=date["tick_counter"], revision_text=last["action[0].revision[0].revision.text"],
            revision_modified=last["action[0].revision[0].revision.modified"],
            revision_newgrf=last["action[0].revision[0].revision.newgrf"], save_version=440, inherited_paid_scope="false")
        terminal = context.evidence / "synthetic-operation.txt"
        terminal.write_text("Synthetic transport only; no actual native image executed.\nSPACE_OPERATION_RESULT " +
            " ".join(f"{key}={value}" for key, value in fields.items()) +
            "\nAll tests passed (17 assertions in 1 test case)\n", encoding="utf-8")
        build = owner.read(self.base.build_results[configuration], self.root)
        epoch = owner.read(build["source_epoch"], self.root)
        extras = [identity(path, self.root) for path in (self.validator, self.native_source)]
        epoch["files"].extend(extras)
        build["source_epoch"] = self.base.write(context.data / "source-epoch.json", epoch)
        build_reference = self.base.write(context.reports / "synthetic-build.json", build)
        application = build["targets"]["openttd_surface_capital_test"]
        tag = "[.plan11-capacity-fixture]"
        process = dict(_documentation="Owner: r2fc-file-cleanup3.md12B; synthetic process, never executed.",
            exit_status=0, inputs=[source_reference], executable=application, configuration=configuration,
            argv=[str(self.root / application["path"]), tag, "--durations", "yes"],
            run_id=context.run_id, feature="space", scenario=context.scenario,
            artifacts=[identity(path, self.root) for path in (before, output, terminal)],
            transcripts=[identity(terminal, self.root)])
        process_reference = self.base.write(context.reports / "synthetic-process.json", process)
        operation = dict(_documentation="Owner: r2fc-file-cleanup3.md12B; synthetic operation, not a paid-world result.",
            schema_version=1, kind="space_operation_result", complete=True, configuration=configuration,
            task=fields["task"], phase=fields["phase"], tag=tag, source=source_reference,
            before=identity(before, self.root), output=identity(output, self.root), application=application,
            terminal=identity(terminal, self.root), native_process=process_reference,
            build_result=build_reference, source_files=extras, validator_source=extras[0],
            observer_contract="original-retirement-v1", initial=expected, final=expected,
            financial_policy=POLICY, scenario=context.scenario, run_id=context.run_id)
        self.documents[configuration] = operation
        self.operations[configuration] = self.base.write(context.reports / "synthetic-operation.json", operation)

    def publish(self, validator=validate_fixture_operation):
        """Invoke real paired successor validation on explicitly synthetic evidence.

        @param self Active isolated transport fixture.
        @param validator Pure test query validator; None tests mandatory-validator refusal.
        @return Typed scratch successor granting no real gameplay authority.
        """
        return publish_successor(self.predecessor, self.operations, "Plan11/15CSGZVEEI", self.destination, validator)
