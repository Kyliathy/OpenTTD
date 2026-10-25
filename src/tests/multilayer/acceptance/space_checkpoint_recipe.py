"""Execute the registered zero-tick Space recovery recipe through actual owners.

@file space_checkpoint_recipe.py Current load/static evidence, never historical earnings.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group15.
The parent recipe uses checkpoint-recovery; actual native/static views retain
their existing checkpoint-load/checkpoint-static identities in the same invocation.
"""

from contextlib import contextmanager
import json
import os

from space_checkpoint import CONFIGURATIONS, load_foundation, verify_foundation
from artifact_paths import RunContext, contained
from space_checkpoint_load import checked, identity, execute_load_check
from space_checkpoint_static import execute_static_check
from space_checkpoint_foundation import read, validate_build_document


@contextmanager
def native_children(request, root):
    """Select the existing strict native/main runtime contract for serial load children.

    @param request Exact current parent recipe request with its reserved invocation.
    @param root Selected checkout, shared with the actual native children.
    @yield None while native/static owners inherit the explicit same run identity.
    """
    if os.environ.get("OTTD_TEST_RUN_ID") != request["run_id"]:
        raise ValueError("Space recipe requires its explicit inherited invocation")
    keys = ("OTTD_TEST_PROFILE", "OTTD_TEST_PROCESS", "OTTD_TEST_RUNTIME_DIR")
    previous = {key: os.environ.get(key) for key in keys}
    context = RunContext(root, "space", "checkpoint-load", request["run_id"])
    os.environ.update(OTTD_TEST_PROFILE="native", OTTD_TEST_PROCESS="main", OTTD_TEST_RUNTIME_DIR=str(context.runtime_dir))
    try:
        yield
    finally:
        # Native/static qualifiers already define the strict native/main home.
        # Restore the parent stage's separate home after its serial children stop.
        for key, value in previous.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value


def execute_stage(request, root):
    """Prepare exact originals or verify them through current native and ordinary loaders.

    @param request Registered current-run stage, explicit configuration/inputs/build/outputs.
    @param root Selected source checkout; no old receipt chain is discovered.
    @return Actual measured checks; dispatcher publishes its matching stage result.
    """
    # Cleanup has exactly two recovery stages and no work/query mode. A different
    # financial action or missing configuration cannot enter this adapter.
    if request["stage"] not in ("prepare", "verify") or request["configuration"] not in CONFIGURATIONS or request["paid_work_authorized"] is not False or request["publishes_source"] is not False:
        raise ValueError("Space recovery requires explicit zero-tick recipe scope")
    configuration = request["configuration"]
    checkpoint = load_foundation(checked(request["inputs"]["selected-foundation"], root), configuration, root)
    verify_foundation(checkpoint)
    build_reference = request["build"]["result"]
    build = read(build_reference, root)
    validate_build_document(build, read(build["source_epoch"], root), read(build["tool_profile"], root), configuration, root, True)
    if request["stage"] == "prepare":
        return dict(checks=["original sealed save and independent observations verified",
                            "actual current capital/ordinary images and source epoch verified"], configuration=configuration)
    with native_children(request, root):
        native = execute_load_check(checkpoint, configuration, checked(build["targets"]["openttd_surface_capital_test"], root))
        ordinary = execute_static_check(checkpoint, configuration, checked(build["targets"]["openttd"], root), native)
    native_document, ordinary_document = read(native, root), read(ordinary, root)
    context = RunContext(root, request["feature"], request["scenario"], request["run_id"])
    terminal_sources = [native_document["terminal"], ordinary_document["terminal"]]
    transcript = contained(root / request["outputs"]["native-evidence"], context.evidence)
    # Aggregate only the exact actual child transcripts for the catalogue role.
    # Original process-owned terminal identities stay explicit in the manifest;
    # this aggregate never impersonates either native process's own output.
    with transcript.open("xb") as stream:
        for label, reference in zip(("Dedicated native load", "Ordinary static restore"), terminal_sources):
            stream.write(("\n# " + label + "; @see r2fc-file-cleanup3.md15\n").encode("utf-8"))
            stream.write(checked(reference, root).read_bytes())
    manifest = dict(_documentation=__doc__, schema_version=1, kind="space_checkpoint_recipe_result", complete=True,
        configuration=configuration, foundation=request['inputs']['selected-foundation'],
        source=native_document["source"], native_result=native, ordinary_result=ordinary,
        build_result=build_reference, terminal_sources=terminal_sources,
        evidence=identity(transcript, root), gameplay_ticks_executed=0, authorization_granted=False)
    target = contained(root / request["outputs"]["manifest"], context.reports)
    with target.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(manifest, indent=2) + "\n")
    return dict(checks=["actual dedicated native load preserved all128merged families and gameplay clocks",
                        "actual ordinary static restore matched independent owner/material state",
                        "original save hashes and closed financial scope remained unchanged"],
                native_result=native, ordinary_result=ordinary, gameplay_ticks_executed=0)
