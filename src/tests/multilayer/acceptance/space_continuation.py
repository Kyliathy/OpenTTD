"""Select an exact qualified Space continuation without running a producer.

@file space_continuation.py Explicit foundation/configuration/source selection.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group12B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md.
This boundary grants no paid scope and never imports the legacy thirteen-stage binder.
"""

import argparse
from dataclasses import dataclass
import json
from pathlib import Path

from space_checkpoint import CONFIGURATIONS, SpaceFoundation, load_foundation, verify_foundation
from space_checkpoint_load import identity
from artifact_paths import resolve_input_path


@dataclass(frozen=True)
class ContinuationInput:
    """One exact verified source for a caller-owned Plan11 task.

    @param foundation Exact typed completed qualification or operation selection.
    @param configuration Explicit independent native configuration.
    @param source Actual current save selected through the shared input catalogue.
    @param source_identity Exact selected physical payload identity.
    @param task Explicit next task; the task owns its business permissions.
    """

    foundation: SpaceFoundation
    configuration: str
    source: Path
    source_identity: dict
    task: str


def select_continuation(foundation, configuration, source, observer=None):
    """Verify explicit source selection against the completed foundation's exact save.

    @param foundation Typed foundation loaded from an explicitly named report.
    @param configuration Debug or RelWithDebInfo, never inferred from a filename.
    @param source Explicit save path or existing retained catalogue alias.
    @param observer Explicit current Plan11 observer, or original default semantics.
    @return Immutable selection after full foundation and source identity checks.
    """
    if not isinstance(foundation, SpaceFoundation):
        raise TypeError("continuation requires an explicitly loaded SpaceFoundation")
    # Independent native worlds cannot exchange saves just because observations
    # happen to agree. A typed single-configuration selection remains binding.
    if configuration not in CONFIGURATIONS or foundation.configuration not in (None, configuration):
        raise ValueError("explicit continuation configuration disagrees")
    facts = verify_foundation(foundation, observer)
    expected = facts["configurations"][configuration]["source"]
    selected = resolve_input_path(source, root=foundation.root)
    required = resolve_input_path(expected["path"], root=foundation.root)
    actual = identity(selected, foundation.root)
    # Catalogue aliases may relocate the original bytes, but cannot silently
    # select another current run or substitute a same-looking configuration.
    if selected != required or actual["sha256"] != expected["sha256"]:
        raise ValueError("explicit continuation save differs from the selected foundation")
    return ContinuationInput(foundation, configuration, selected, actual, facts["next_task"])


def main():
    """Inspect an explicitly named original-observer continuation without writes.

    @return None; validation errors fail without reserving a runtime/review run.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--foundation", required=True, type=Path)
    parser.add_argument("--configuration", required=True, choices=CONFIGURATIONS)
    parser.add_argument("--source", required=True, type=Path)
    arguments = parser.parse_args()
    selected = select_continuation(load_foundation(arguments.foundation, arguments.configuration),
                                   arguments.configuration, arguments.source)
    print(json.dumps(dict(configuration=selected.configuration, source=selected.source_identity,
                         task=selected.task, authorization_granted=False), indent=2))


if __name__ == "__main__":
    main()
