"""Qualify a sealed Space save through a dedicated zero-tick native callback.

@file space_checkpoint_load.py Native load proof, distinct from legacy operation receipts.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group11.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md.
Actual compilation/execution is staged by Group15. No query/work entry point,
paid replay, historical executable fallback or generated golden is used here.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))

from artifact_inventory import checked_path, file_identity
from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from savegame_tables import saved_owner_table as table
from space_checkpoint import SpaceCheckpoint, load_checkpoint, verify_checkpoint
from space_preview_checks import chunks
from surface_constructor_retirement_ledger import observe

TAG = "[.space-checkpoint-load]"
PHASE = "load-only"
SCENARIO = "checkpoint-load"
KIND = "native_space_checkpoint_load"
CLOCK_FIELDS = {"tick_counter", "date", "date_fract", "economy_date", "economy_date_fract",
                "calendar_sub_date_fract", "random_state[0]", "random_state[1]",
                "company_tick_counter", "trees_tick_counter"}
SOURCE_FILES = ("src/tests/space_checkpoint_load.h", "src/tests/surface_entry_productive_admission.cpp",
                "src/tests/surface_entry_resupply.cpp", "src/tests/layer_save_fixture.h",
                "src/tests/multilayer/acceptance/space_checkpoint_load.py",
                "src/tests/multilayer/acceptance/space_checkpoint.py",
                "src/tests/multilayer/acceptance/space_saved_state.py",
                "src/tests/multilayer/acceptance/surface_constructor_retirement_ledger.py",
                "src/tests/multilayer/support/savegame_tables.py",
                "src/tests/multilayer/support/runner.py", "src/tests/multilayer/support/artifact_paths.py")
FIELDS = {"_documentation", "schema_version", "kind", "complete", "configuration", "phase", "tag",
          "seal", "source", "application", "run_id", "before", "output", "terminal", "native_process",
          "source_files", "observed", "gameplay_ticks_executed"}


def identity(path, root):
    """Bind an existing contained source or generated file to its full identity.

    @param path Explicit physical file, never selected by a latest-run scan.
    @param root Source checkout containing that file.
    @return Exact repository-relative path and streamed SHA256.
    """
    selected = checked_path(path, root)
    return dict(path=selected.relative_to(root).as_posix(), sha256=file_identity(selected)[0])


def checked(reference, root):
    """Verify one explicit live proof input, preserving containment and identity.

    @param reference Exact path/SHA256 object for this invocation or retained seal.
    @param root Explicit source checkout.
    @return Existing file after full-byte verification.
    """
    # Only explicit live identities cross this proof boundary. A legacy receipt
    # or loosely path-shaped metadata cannot select an unverified process input.
    if not isinstance(reference, dict) or set(reference) != {"path", "sha256"}:
        raise ValueError("native load reference requires exact path/SHA256")
    selected = checked_path(reference["path"], root)
    if identity(selected, root) != reference:
        raise ValueError(f"native load input changed: {reference['path']}")
    return selected


def terminal_fields(terminal):
    """Read one unambiguous native observation/completion from the actual transcript.

    @param terminal Decoded current-run evidence text, not an old optional log.
    @return Numeric native owner/revision fields after strict phase/success checks.
    """
    parsed = {}
    # Duplicate native completion/observation lines could combine two different
    # processes. Exactly one selected paused callback must explain the receipt.
    for prefix in ("SPACE_CHECKPOINT_NATIVE", "SPACE_CHECKPOINT_COMPLETE"):
        lines = [line for line in terminal.splitlines() if line.startswith(prefix + " ")]
        if len(lines) != 1:
            raise ValueError("native checkpoint requires one exact observation/completion")
        tokens = [token.split("=", 1) for token in lines[0].split()[1:]]
        fields = dict(tokens)
        if len(fields) != len(tokens):
            raise ValueError("duplicate native checkpoint terminal field")
        parsed[prefix] = fields
    complete = parsed["SPACE_CHECKPOINT_COMPLETE"]
    if complete != dict(phase=PHASE, ticks="0", snapshots="2"):
        raise ValueError("unexpected native checkpoint phase/tick count")
    passed = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", terminal)
    # A work callback or additional selected case is not a zero-tick load proof,
    # even when Catch2 ultimately reports success for the combined invocation.
    if passed is None or int(passed[1]) == 0 or int(passed[2]) != 1 or "SURFACE_RESUPPLY_COMPLETE" in terminal:
        raise ValueError("native checkpoint process did not pass its sole load case")
    native = parsed["SPACE_CHECKPOINT_NATIVE"]
    if set(native) != {"company", "cash", "debt", "tick_before", "tick_after", "revision_text", "revision_modified", "revision_newgrf", "save_version"}:
        raise ValueError("incomplete native checkpoint observation")
    if re.fullmatch(r"(?:[0-9a-f]{2})+", native["revision_text"]) is None:
        raise ValueError("invalid native revision identity")
    return {key: value if key == "revision_text" else int(value) for key, value in native.items()}


def compare_clocks(source, before, output):
    """Compare every clock/RNG field before permitting only worker-pause normalization.

    @param source Exact sealed source save.
    @param before Native snapshot at the beginning of the paused callback.
    @param output Native snapshot at its end, without any game-loop calls.
    @return Explicit source/before/output clocks and narrow pause observation.
    """
    clocks = []
    for path in (source, before, output):
        rows = table(path, b"DATE")
        if set(rows) != {0}:
            raise ValueError("checkpoint requires one actual DATE owner")
        clocks.append(rows[0])
    original, first, last = clocks
    if not CLOCK_FIELDS <= original.keys():
        raise ValueError("checkpoint lacks explicit clock/RNG fields")
    # Check actual tick/date counters first. DATE is never ignored as a family;
    # all other saved clock, RNG, scheduler and identity fields remain exact too.
    for observed in (first, last):
        if set(observed) != set(original):
            raise ValueError("checkpoint DATE schema changed")
        for key, value in original.items():
            if key == "pause_mode":
                continue
            if observed[key] != value:
                raise ValueError(f"checkpoint clock/state changed: {key}")
    # Both serialized snapshots belong to the same normal-paused inspection;
    # an unpaused or independently worker-paused output needs another contract.
    if first["pause_mode"] != 1 or last["pause_mode"] != 1:
        raise ValueError("native checkpoint snapshots must retain the normal pause")
    if original["pause_mode"] not in (1, 65):
        raise ValueError("unexpected original pause flags")
    if original["pause_mode"] == 65:
        jobs, schedule = table(source, b"LGRJ"), table(source, b"LGRS")[0]
        # Only the existing AfterLoad_LinkGraphPauseControl bit may be cleared
        # by SaveSurfaceResupply's normal-pause serialization. Validate its due
        # original job; an arbitrary pause or clock difference remains forbidden.
        if not any(jobs[reference - 1]["join_date"] <= original["economy_date"] for reference in schedule["running"]):
            raise ValueError("worker pause lacks its actual due linkgraph job")
    return dict(source=original, before=first, output=last,
                pause=dict(source=original["pause_mode"], snapshots=1, owner="SaveSurfaceResupply/AfterLoad_LinkGraphPauseControl"))


def compare_revision(source, before, native):
    """Validate unchanged history or one exact actual load-revision append.

    @param source Original sealed game history.
    @param before First paused native snapshot.
    @param native Current process's actual compiled revision fields.
    @return Whether one known-format load revision was appended.
    """
    original, loaded = table(source, b"GLOG"), table(before, b"GLOG")
    expected_revision = {"revision.text": native["revision_text"], "revision.newgrf": native["revision_newgrf"],
                         "revision.slver": native["save_version"], "revision.modified": native["revision_modified"]}
    if native["revision_modified"] not in (0, 1, 2):
        raise ValueError("invalid compiled revision status")
    if original == loaded:
        revisions = []
        # With no append, the most recent genuine revision must already match
        # this binary. An unrelated older matching row cannot bless a new image.
        for row in original.values():
            for index in range(row["action.count"]):
                prefix = f"action[{index}].revision[0]."
                if row.get(f"action[{index}].ct") == 1:
                    revisions.append({name: row[prefix + name] for name in expected_revision})
        if not revisions or revisions[-1] != expected_revision:
            raise ValueError("unchanged GLOG disagrees with actual compiled revision")
        return False
    # Preserve the complete original action prefix and admit only one appended
    # load action. Renumbering or dropping a prior action would rewrite history.
    if set(original) != set(range(len(original))) or set(loaded) != set(original) | {len(original)}:
        raise ValueError("unexpected GLOG revision append extent")
    for key, value in original.items():
        if loaded[key] != value:
            raise ValueError("original GLOG history changed")
    expected = dict(at=1, tick=table(source, b"DATE")[0]["tick_counter"])
    expected.update({"action.count": 1, "action[0].ct": 1, "action[0].revision.count": 1})
    expected.update({"action[0].revision[0]." + key: value for key, value in expected_revision.items()})
    # A load may append exactly one revision action, with no setting, GRF,
    # emergency, mode or other gameplay action hidden in the same table row.
    for kind in ("mode", "oldver", "setting", "grfadd", "grfrem", "grfcompat", "grfparam", "grfmove", "grfbug", "emergency"):
        expected[f"action[0].{kind}.count"] = 0
    if loaded[len(original)] != expected:
        raise ValueError("GLOG append differs from the actual sole load revision")
    return True


def compare_loaded_saves(source, before, output, expected, native):
    """Check every saved family and independent owner expectation without blanket exceptions.

    @param source Exact sealed source whose original hash has already verified.
    @param before Native callback's first actual saved snapshot.
    @param output Native callback's second actual saved snapshot.
    @param expected Independent original observation from the retained audit.
    @param native Actual numeric/revision fields parsed from this process's terminal.
    @return Explicit semantic/clock/revision verification facts.
    """
    clocks = compare_clocks(source, before, output)
    versions, owners = zip(*(chunks(path) for path in (source, before, output)))
    original, first, last = owners
    # This sealed source owns the current440 complete family set. A version or
    # family migration cannot be concealed as a routine zero-tick load check.
    if versions != (440, 440, 440) or len(original) != 121 or original.keys() != first.keys() or original.keys() != last.keys():
        raise ValueError("checkpoint save version/121-family closure changed")
    # Before/after snapshots are within one paused callback, so even DATE/GLOG
    # must be byte-identical between them. Only source-to-load metadata uses the
    # individually checked clock/pause and single revision rules above/below.
    for family, raw in first.items():
        if last[family] != raw:
            raise ValueError(f"paused native snapshot changed family: {family!r}")
        if family not in (b"DATE", b"GLOG") and original[family] != raw:
            raise ValueError(f"native load changed family: {family!r}")
    appended = compare_revision(source, before, native)
    actual = json.loads(json.dumps(observe(before)))
    # Equal newly written files are insufficient: each actual loaded owner set
    # must equal the independently retained original observation.
    if actual != expected or json.loads(json.dumps(observe(output))) != expected:
        raise ValueError("loaded actual owner/material observation disagrees")
    # The native observation must identify the same real owner/counters, not
    # merely print a zero-tick marker while a different world was serialized.
    if native["company"] != 0 or native["cash"] != expected["company"]["cash"] or native["debt"] != expected["company"]["debt"] or native["tick_before"] != clocks["source"]["tick_counter"] or native["tick_after"] != native["tick_before"] or native["save_version"] != 440:
        raise ValueError("native owner/clock marker disagrees with saved state")
    return dict(chunk_families=121, saved_family_comparisons=242, clocks=clocks,
                appended_load_revision=appended, observed=actual, gameplay_ticks_executed=0)


def verify_load_result(document, root=None):
    """Verify one exact native load receipt with no imports of old paid chains.

    @param document Versioned native-space-checkpoint-load receipt, never a legacy dict.
    @param root Explicit source checkout, defaulting to the executing producer's root.
    @return Checked semantic result; compilation qualification remains a later layer.
    """
    source_root = Path(root or ROOT).resolve()
    # The dedicated typed receipt keeps native recovery separate from old work
    # operations; incomplete or extended legacy shapes are never auto-converted.
    if not isinstance(document, dict) or set(document) != FIELDS:
        raise ValueError("native load receipt requires its exact typed fields")
    # Only this dedicated tag/phase can supply recovery evidence. A successful
    # work/query receipt or synthetic legacy complete=true value cannot substitute.
    if type(document["schema_version"]) is not int or document["schema_version"] != 1 or document["kind"] != KIND or document["complete"] is not True or document["tag"] != TAG or document["phase"] != PHASE or type(document["gameplay_ticks_executed"]) is not int or document["gameplay_ticks_executed"] != 0:
        raise ValueError("unexpected native load schema/tag/phase")
    seal_path = checked(document["seal"], source_root)
    checkpoint = load_checkpoint(seal_path, document["configuration"], source_root)
    verify_checkpoint(checkpoint)
    seal = json.loads(seal_path.read_text(encoding="utf-8"))
    selected = seal["configurations"][document["configuration"]]
    if document["source"] != selected["save"]:
        raise ValueError("native load selected the wrong save/configuration")
    source = checked(document["source"], source_root)
    application = checked(document["application"], source_root)
    # A successful different target/configuration says nothing about this
    # independently linked load callback and its selected original save.
    if application.parent.name != document["configuration"] or application.name not in ("openttd_surface_capital_test.exe", "openttd_surface_capital_test"):
        raise ValueError("native load application/configuration mismatch")
    context = RunContext(source_root, "space", SCENARIO, identifier(document["run_id"]))
    before, output, terminal, process_path = [checked(document[field], source_root) for field in ("before", "output", "terminal", "native_process")]
    # Required outputs and terminal observations belong to this one invocation;
    # another run's equally named snapshots cannot complete this load proof.
    if before != context.saves / "checkpoint-before.sav" or output != context.saves / "checkpoint-after.sav" or terminal.parent != context.evidence or process_path.parent != context.reports:
        raise ValueError("native load outputs belong to another invocation/role")
    process = json.loads(process_path.read_text(encoding="utf-8"))
    expected_argv = [str(application), TAG, "--durations", "yes"]
    # Bind success to the exact selected image/input/tag/process home. A generic
    # successful subprocess or the other configuration is not recovery evidence.
    if process["exit_status"] != 0 or process["inputs"] != [document["source"]] or process["executable"] != document["application"] or process["argv"] != expected_argv or process["configuration"] != document["configuration"] or process["run_id"] != context.run_id or process["feature"] != context.feature or process["scenario"] != context.scenario or process["runtime"] != context.runtime_dir.relative_to(source_root).as_posix():
        raise ValueError("native load process inputs/image/tag/status disagree")
    required_outputs = [document[field] for field in ("before", "output", "terminal")]
    # The native process must actually claim both snapshots and its retained
    # terminal; finding files afterward cannot supply missing process evidence.
    if any(reference not in process["artifacts"] for reference in required_outputs) or document["terminal"] not in process["transcripts"]:
        raise ValueError("incomplete native process outputs/transcript")
    if {reference["path"] for reference in document["source_files"]} != set(SOURCE_FILES):
        raise ValueError("native load lacks explicit source owner identities")
    for reference in document["source_files"]:
        checked(reference, source_root)
    native = terminal_fields(terminal.read_text(encoding="utf-8", errors="replace"))
    expected = json.loads(checked(selected["expected_observation"], source_root).read_text(encoding="utf-8"))["observation"]
    facts = compare_loaded_saves(source, before, output, expected, native)
    if facts["observed"] != document["observed"]:
        raise ValueError("native load receipt observation changed")
    # Recheck all live authorities at close so a concurrently changed source,
    # image, process or serialized output cannot produce mixed-epoch acceptance.
    for reference in [document[field] for field in ("seal", "source", "application", "before", "output", "terminal", "native_process")] + document["source_files"]:
        checked(reference, source_root)
    return facts


def execute_load_check(checkpoint, configuration, application):
    """Execute only the dedicated paused callback against one sealed original.

    @param checkpoint Typed original seal, already present in this checkout.
    @param configuration Explicit independently linked Debug/RelWithDebInfo image.
    @param application Explicit fresh isolated capital application; no build fallback.
    @return Published current-run load receipt identity, after complete verification.
    """
    if not isinstance(checkpoint, SpaceCheckpoint):
        raise TypeError("native load requires a typed sealed SpaceCheckpoint")
    # A caller cannot select a different configuration after loading an explicit
    # typed selection, even though the seal physically retains both originals.
    if configuration not in ("RelWithDebInfo", "Debug") or checkpoint.configuration not in (None, configuration):
        raise ValueError("native load configuration disagrees with its typed selection")
    verify_checkpoint(checkpoint)
    root = checkpoint.root
    seal = json.loads(checkpoint.manifest_path.read_text(encoding="utf-8"))
    selected = seal["configurations"][configuration]
    source = checked(selected["save"], root)
    app_reference = identity(application, root)
    application = checked(app_reference, root)
    # Reject wrong targets/configurations before reserving an invocation or
    # allowing a child to touch the selected runtime distribution.
    if application.parent.name != configuration or application.name not in ("openttd_surface_capital_test.exe", "openttd_surface_capital_test"):
        raise ValueError("select the exact configuration's capital load application")
    sources = [identity(root / path, root) for path in SOURCE_FILES]
    context = create_run_context("space", SCENARIO, root=root)
    # Fresh native proof never overwrites an earlier process in an inherited
    # run. Restart inspection must verify that receipt without rerunning it.
    if any(path.is_file() for role in ("images", "saves", "data", "reports", "evidence", "scratch") for path in (context.directory / role).rglob("*")):
        raise ValueError("native checkpoint invocation already contains output")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("space_checkpoint_load\n", encoding="utf-8")
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment.update(OTTD_TEST_CONFIGURATION=configuration, OTTD_SPACE_CHECKPOINT_SOURCE=str(source))
    log = context.output("logs", "native.log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), TAG, "--durations", "yes"], cwd=context.runtime_dir,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=60, env=environment)
    if result.returncode != 0:
        raise ValueError(f"native checkpoint load failed; inspect {log}")
    reports = list(context.reports.glob("native-*.json"))
    if len(reports) != 1:
        raise ValueError("native checkpoint requires one exact child process")
    process = json.loads(reports[0].read_text(encoding="utf-8"))
    if len(process["transcripts"]) != 1:
        raise ValueError("native checkpoint requires its actual retained terminal")
    output = context.saves / "checkpoint-after.sav"
    document = dict(_documentation=__doc__, schema_version=1, kind=KIND, complete=True,
        configuration=configuration, phase=PHASE, tag=TAG,
        seal=dict(path=checkpoint.manifest_path.relative_to(root).as_posix(), sha256=checkpoint.sha256),
        source=selected["save"], application=app_reference, run_id=context.run_id,
        before=identity(context.saves / "checkpoint-before.sav", root), output=identity(output, root),
        terminal=process["transcripts"][0], native_process=identity(reports[0], root),
        source_files=sources, observed=json.loads(json.dumps(observe(output))), gameplay_ticks_executed=0)
    verify_load_result(document, root)
    target = context.output("reports", "space-checkpoint-load.json")
    target.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return identity(target, root)


def main():
    """Run an explicitly selected load check, or verify its existing receipt read-only.

    @return None; no legacy receipt, phase/work selector or historical chain is loaded.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoint", type=Path)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--application", type=Path)
    parser.add_argument("--verify-result", type=Path)
    args = parser.parse_args()
    if args.verify_result is not None:
        result = json.loads(checked_path(args.verify_result, ROOT).read_text(encoding="utf-8"))
        print(json.dumps(verify_load_result(result), indent=2))
        return
    # Partial CLI selection must not reserve a profile or invoke a default image.
    if args.configuration is None or args.application is None:
        parser.error("select --configuration and --application for the dedicated load check")
    print(json.dumps(execute_load_check(load_checkpoint(args.checkpoint, args.configuration), args.configuration, args.application), indent=2))


if __name__ == "__main__":
    main()
