"""Prepare one registered recipe closure with exact inputs and explicit restart state.

@file artifact_rebuild.py Inspection is read-only and never imports producer entry points.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups14/18C.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group28 current dependency guards.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Groups30-32 shared cold build.
Every native/generated prerequisite has a separate outcome. Only actual completed
verification publishes successful output identities; failed requests grant no scope.
"""

import argparse
import ctypes
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import uuid

from artifact_paths import (RunContext, contained, create_run_context, identifier,
                            repository_root, resolve_input_path)
from artifact_inventory import file_identity
from artifact_recipes import load_catalogue, recipe_order, validate_recipes

DISPATCHER = "src/tests/multilayer/support/artifact_recipe_stage.py"
BUILD_PROFILE = "src/tests/fixtures/config/multilayer/native-build-profile.json"


def identity(path, root):
    """Capture a contained current payload's actual identity.

    @param path Exact source/input/current output path.
    @param root Explicit checkout.
    @return Repository-relative path and full SHA256.
    """
    actual = contained(path, root)
    return dict(path=actual.relative_to(root).as_posix(), sha256=file_identity(actual)[0])


def input_stamp(path):
    """Observe a selected file's replacement/content epoch without caching bytes across checks.

    @param path Exact existing input or optional input-catalogue path.
    @return Size/mtime/inode/mode tuple, or None when absent.
    """
    try:
        status = path.stat()
    except FileNotFoundError:
        return None
    return status.st_size, status.st_mtime_ns, status.st_ino, status.st_mode


def input_observations(root):
    """Start one inspection/snapshot-local input observation cache.

    @param root Exact selected checkout, including an optional retained-first alias catalogue.
    @return Fresh cache; it must never survive this one before/after observation pass.
    """
    catalogue = root / "src/tests/fixtures/multilayer-inputs.json"
    return {None: dict(path=catalogue, stamp=input_stamp(catalogue))}


def close_input_observations(observations):
    """Reject changes while a single deduplicated input snapshot was being assembled.

    @param observations This pass's original-path-to-observation cache, including alias epoch.
    @return None; missing/replaced/modified originals and changed aliases refuse.
    """
    # Every unique input and the alias authority must still belong to the same
    # observation epoch. A later snapshot starts fresh and hashes bytes again.
    for observation in observations.values():
        if input_stamp(observation["path"]) != observation["stamp"]:
            raise ValueError("retained input or alias catalogue changed while collecting snapshot")


def observed_retained_inputs(recipe, root, observations):
    """Resolve/hash each exact selected input once within one owned observation pass.

    @param recipe Registered recipe declaration, already schema/graph validated.
    @param root Exact selected source checkout.
    @param observations Fresh inspection/snapshot-local cache closed by its owning pass.
    @return Role-to-current-exact-identity mapping; missing material fails explicitly.
    """
    result = {}
    # Each semantic input role selects its own immutable payload. Aliases may
    # relocate exact bytes, but missing prerequisites never fall back to logs.
    for row in recipe["retained_inputs"]:
        observation = observations.get(row["path"])
        if observation is not None:
            reference = observation["reference"]
            # Each declaration keeps its own expected identity even when a
            # shared original is observed once for several selected suppliers.
            if row.get("sha256") not in (None, reference["sha256"]):
                raise ValueError("recipe retained input differs from its declared identity")
            result[row["role"]] = dict(reference)
            continue
        # Authored media is an existing immutable source owner, while saved
        # worlds/fixtures use the shared alias resolver's retained-first policy.
        # Original source-conversion JSON fixtures also live beside their
        # existing tests; explicit declarations may read that established owner
        # without granting source files a generated-output or native-save role.
        relative = Path(row["path"])
        source_fixture = relative.parts[:3] == ("src", "tests", "multilayer") and relative.suffix == ".json"
        if source_fixture:
            path = contained(root / relative, root / "src/tests/multilayer")
        elif relative.parts[0] == "media":
            path = contained(root / relative, root / "media")
        else:
            path = resolve_input_path(row["path"], root)
        stamp = input_stamp(path)
        reference = identity(path, root)
        if input_stamp(path) != stamp:
            raise ValueError("retained input changed while collecting snapshot")
        if row.get("sha256") not in (None, reference["sha256"]):
            raise ValueError("recipe retained input differs from its declared identity")
        result[row["role"]] = dict(reference)
        observations[row["path"]] = dict(path=path, stamp=stamp, reference=reference)
    return result


def verify_retained_inputs(recipe, root=None):
    """Verify actual selected bytes through the shared retained-first ownership boundary.

    @param recipe Registered declaration with exact retained input roles/optional SHA256 values.
    @param root Optional selected checkout; no previous invocation cache is reused.
    @return Role-to-current identity map after closing this fresh observation epoch.
    """
    root = repository_root(root)
    observations = input_observations(root)
    result = observed_retained_inputs(recipe, root, observations)
    close_input_observations(observations)
    return result


def load_recipe(name, configuration=None, root=None):
    """Inspect a selected registered graph without writes, producer imports or processes.

    @param name Exact catalogue recipe identifier.
    @param configuration Explicit native configuration, or None for independent conversion.
    @param root Optional selected checkout.
    @return Validated selected closure, missing/pending details and execution eligibility.
    """
    root = repository_root(root)
    recipes = validate_recipes(load_catalogue(root), root)
    order = recipe_order(name, recipes)
    if configuration not in (None, "Debug", "RelWithDebInfo"):
        raise ValueError("unsupported recipe configuration")
    # Any native prerequisite needs its own exact configuration even when the
    # selected leaf is a conversion or metadata consumer.
    if configuration is None and any(recipes[item]["configuration_required"] for item in order):
        raise ValueError("selected recipe closure requires an explicit configuration")
    missing, pending = [], {}
    destinations = set()
    observations = input_observations(root)
    for item in order:
        recipe = recipes[item]
        if not recipe["enabled"]:
            pending[item] = recipe["pending_groups"]
        for row in recipe["retained_inputs"]:
            # Enabled physical inputs were validated retained-first above and
            # are hashed by the observer below; missing old logical paths alone
            # cannot make a cold aliased recipe non-executable.
            if not recipe["enabled"] and not (root / row["path"]).is_file():
                missing.append(dict(recipe=item, **row))
        # Enabled recipes may deliberately use a stable key whose generated old
        # original is gone. The retained-first observer, not raw path existence,
        # decides whether their cold inputs are present and hash-correct.
        if recipe["enabled"] or all((root / row["path"]).is_file() for row in recipe["retained_inputs"]):
            observed_retained_inputs(recipe, root, observations)
        # Shared invocation IDs do not authorize two prerequisite recipes to
        # overwrite the same feature/scenario/output role.
        for row in recipe["outputs"]:
            destination = (recipe["feature"], recipe["scenario"], row["path"].format(configuration=configuration).casefold())
            if destination in destinations:
                raise ValueError("selected recipe prerequisites have colliding output destinations")
            destinations.add(destination)
    close_input_observations(observations)
    return dict(recipe=name, configuration=configuration, order=order,
                recipes={item: recipes[item] for item in order}, missing_inputs=missing,
                pending_groups=pending, executable=not missing and not pending,
                reserves_run_id=False, publishes_source=False, paid_work_authorized=False)


def snapshot(selection, root):
    """Bind restart authority to exact selected recipes, inputs and actual producer sources.

    @param selection Read-only selected catalogue closure.
    @param root Explicit checkout.
    @return Stable restart signature; unrelated workstation paths are absent.
    """
    # Routing/declaration policy is part of restart authority, just like the
    # producer. Changed child validation cannot reuse an earlier completed gate.
    sources = {DISPATCHER, "src/tests/multilayer/support/artifact_rebuild.py", "mlm/review-producers.json",
               "src/tests/multilayer/support/artifact_recipes.py", "src/tests/multilayer/support/artifact_paths.py",
               "src/tests/multilayer/support/artifact_inventory.py", "src/tests/multilayer/support/artifact_space_graph.py"}
    inputs = {}
    observations = input_observations(root)
    # Hash the actual declared producers and selectors before any reservation.
    # A failed stage can resume only under the identical semantic source epoch.
    for name, recipe in selection["recipes"].items():
        sources.update([recipe["producer"], *recipe.get("tag_sources", [])])
        sources.update(recipe.get("implementation_sources", []))
        inputs[name] = observed_retained_inputs(recipe, root, observations)
    close_input_observations(observations)
    catalogue = "src/tests/fixtures/multilayer-inputs.json"
    if (root / catalogue).is_file():
        sources.add(catalogue)
    if any(recipe["target"] for recipe in selection["recipes"].values()):
        sources.update([BUILD_PROFILE, "src/tests/multilayer/support/native_build.py",
                        "src/tests/multilayer/support/native_build_dependencies.py"])
    return dict(recipe=selection["recipe"], configuration=selection["configuration"],
                declarations=selection["recipes"], inputs=inputs,
                sources=[identity(root / relative, root) for relative in sorted(sources)])


def write_state(path, document):
    """Atomically replace only this invocation's mutable generated state.

    @param path Exact current reports destination.
    @param document Actual running/failed/completed stage state.
    @return None; an interrupted write cannot expose a partial success document.
    """
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    # Windows scanners may briefly deny replacement after a completed write.
    # Retry only documented access/sharing failures for a bounded interval;
    # persistent errors keep the previous complete state and fail this operation.
    for attempt in range(4):
        try:
            temporary.replace(path)
            return
        except PermissionError as error:
            if getattr(error, "winerror", None) not in (5, 32, 33) or attempt == 3:
                raise
            time.sleep(0.05 * (attempt + 1))


def alive(pid):
    """Check an owned supervisor/child identity without signaling or terminating it.

    @param pid Recorded positive process ID from this invocation's lock.
    @return True when a current process owns that ID; reuse conservatively blocks resume.
    """
    if not pid:
        return False
    if os.name == "nt":
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.OpenProcess.restype = ctypes.c_void_p
        kernel.OpenProcess.argtypes = [ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
        handle = kernel.OpenProcess(0x1000, False, pid)
        if not handle:
            return ctypes.get_last_error() == 5
        status = ctypes.c_ulong()
        kernel.GetExitCodeProcess.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ulong)]
        kernel.CloseHandle.argtypes = [ctypes.c_void_p]
        try:
            return bool(kernel.GetExitCodeProcess(handle, ctypes.byref(status))) and status.value == 259
        finally:
            kernel.CloseHandle(handle)
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return True


def acquire_lock(path, resume):
    """Own execution exclusively; only explicit resume may reclaim a stopped stale lock.

    @param path Exact generated invocation lock path.
    @param resume User explicitly selected an existing invocation.
    @return None; running supervisors/children are never interrupted or overwritten.
    """
    if path.exists():
        previous = json.loads(path.read_text(encoding="utf-8"))
        # A stale directory name never grants execution ownership. Both the old
        # supervisor and its selected child must have stopped before reclamation.
        if not resume or alive(previous.get("owner_pid")) or alive(previous.get("child_pid")):
            raise ValueError("recipe invocation still has an active execution owner")
        path.unlink()
    with path.open("x", encoding="utf-8") as stream:
        json.dump(dict(owner_pid=os.getpid(), child_pid=None), stream)


def run_stage(context, stage, request, lock, attempt):
    """Launch one fixed dispatcher with an explicit hash-bound stage request.

    @param context Exact declared recipe feature/scenario under the selected invocation.
    @param stage Prepare or verify, never arbitrary shell text.
    @param request Exact generated stage request document path.
    @param lock Current supervisor/child ownership file.
    @param attempt Monotonic retry ordinal, preserving earlier failed transcripts.
    @return Actual child status and terminal identity after bounded execution.
    """
    command = [sys.executable, "-B", str(context.root / DISPATCHER), "--request", str(request)]
    terminal = context.evidence / f"recipe-{stage}-{attempt}.txt"
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment = context.environment(environment)
    environment.update(PYTHONDONTWRITEBYTECODE="1", GIT_OPTIONAL_LOCKS="0")
    started = time.monotonic()
    with terminal.open("xb") as stream:
        process = subprocess.Popen(command, cwd=context.root, env=environment, stdout=stream, stderr=subprocess.STDOUT)
        write_state(lock, dict(owner_pid=os.getpid(), child_pid=process.pid))
        try:
            status = process.wait(timeout=120)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            status = -1
        finally:
            write_state(lock, dict(owner_pid=os.getpid(), child_pid=None))
    outcome = dict(argv=command, exit_status=status, elapsed_seconds=round(time.monotonic() - started, 3),
                   terminal=identity(terminal, context.root))
    process_record = context.reports / f"recipe-{stage}-{attempt}-process.json"
    write_state(process_record, dict(_documentation="Owner: r2fc-file-cleanup3.md14; actual dispatcher child outcome.", **outcome))
    outcome["process_record"] = identity(process_record, context.root)
    if status == 0:
        requested = json.loads(request.read_text(encoding="utf-8"))
        result_path = contained(context.root / requested["result_path"], context.reports)
        result = json.loads(result_path.read_text(encoding="utf-8"))
        # A no-op child or stale success file cannot advance the current stage.
        # Actual measured checks, request bytes, configuration and roles agree.
        if result["kind"] != "artifact_recipe_stage_result" or result["schema_version"] != 1 or result["complete"] is not True or result["recipe"] != requested["recipe"] or result["stage"] != stage or result["run_id"] != context.run_id or result["configuration"] != requested["configuration"] or result["request"] != identity(request, context.root) or result["inputs"] != requested["inputs"] or result["sources"] != requested["sources"] or not result["facts"].get("checks"):
            raise ValueError("recipe child did not publish its actual matching stage result")
        for role, reference in result["outputs"].items():
            if requested["outputs"].get(role) != reference["path"] or identity(context.root / reference["path"], context.root) != reference:
                raise ValueError("recipe stage output identity disagrees")
        if stage == "verify" and result["outputs"].keys() != requested["outputs"].keys():
            raise ValueError("recipe verification lacks a declared output role")
        outcome["result"] = identity(result_path, context.root)
    return outcome


def verify_passed_stage(stage, root):
    """Recheck exact passed-stage result and output witnesses before restart reuse.

    @param stage Recorded successful prepare/verify outcome from this invocation.
    @param root Explicit checkout.
    @return None; a missing or changed generated prerequisite cannot be reused.
    """
    reference = stage["result"]
    if identity(root / reference["path"], root) != reference:
        raise ValueError("completed recipe stage witness changed before resume")
    result = json.loads((root / reference["path"]).read_text(encoding="utf-8"))
    for output in result["outputs"].values():
        if identity(root / output["path"], root) != output:
            raise ValueError("completed prerequisite output changed before resume")


def verify_current_build(stage, configuration, recipe, root):
    """Reject changed current build outputs/source before another actual recipe stage.

    @param stage Recorded successful native build outcome for this invocation.
    @param configuration Exact selected independent build configuration.
    @param recipe Actual registered native recipe and required targets.
    @param root Explicit checkout.
    @return None; a completed build record cannot certify another current image.
    """
    reference = stage["result"]
    if identity(root / reference["path"], root) != reference:
        raise ValueError("current recipe build result changed")
    document = json.loads((root / reference["path"]).read_text(encoding="utf-8"))
    targets = recipe["original_parameters"].get("build_targets", [recipe["target"]])
    # Build success, exact configuration and the required images remain current
    # prerequisites; historical image identities cannot satisfy this boundary.
    if document["kind"] != "native_build_result" or document["complete"] is not True or document["configuration"] != configuration or not set(targets) <= document["targets"].keys() or not document["steps"] or any(step["exit_status"] != 0 for step in document["steps"]):
        raise ValueError("current recipe build configuration/targets/status disagree")
    build = contained(root / document["build_directory"], root)
    if build.relative_to(root).parts[0] not in ("build", "test-cache"):
        raise ValueError("current recipe build is outside an audited build owner")
    # An exact old EXE hash is still historical when its path is an archive.
    # Require each declared current target in this build/configuration's slot.
    for target, image in document["targets"].items():
        if contained(root / image["path"], root) != build / configuration / (target + ".exe"):
            raise ValueError("current recipe build image is an archived or foreign target")
    references = [document["source_epoch"], document["tool_profile"], *document["targets"].values()]
    for current in references:
        if identity(root / current["path"], root) != current:
            raise ValueError("current recipe build input/image changed")
    epoch = json.loads((root / document["source_epoch"]["path"]).read_text(encoding="utf-8"))
    from native_build import verify_source_epoch
    verify_source_epoch(epoch, root, document["tool_profile"]["path"])


def shared_current_build(state, configuration, recipe, root):
    """Select a checked compile prerequisite already produced by this exact invocation.

    @param state Current invocation's bound progress; no external/latest-run lookup.
    @param configuration Exact independently selected native configuration.
    @param recipe Registered leaf and its complete required target set.
    @param root Checkout owning source, build result and actual executable images.
    @return Passed build outcome with its originating recipe, or None before a compatible build.
    """
    targets = set(recipe["original_parameters"].get("build_targets", [recipe["target"]]))
    # Reconfiguring for each family can relink a shared image and invalidate
    # earlier native observations. Share only this run's compile prerequisite;
    # each leaf still executes its own independent configuration-bound test.
    for name, node in state["nodes"].items():
        stage = node["stages"].get("build", {})
        if stage.get("status") != "passed":
            continue
        reference = stage["result"]
        if identity(root / reference["path"], root) != reference:
            raise ValueError("current shared recipe build result changed")
        document = json.loads((root / reference["path"]).read_text(encoding="utf-8"))
        if not targets <= document["targets"].keys():
            continue
        # A stale compatible build must stop the run. Silently rebuilding it
        # would erase the executable identity that earlier leaves actually ran.
        verify_current_build(stage, configuration, recipe, root)
        return dict(status="passed", result=reference, shared_from_recipe=name)
    return None


def selected_build_targets(selection):
    """Collect only declared native images required anywhere in this selected graph.

    @param selection Validated dependency closure returned by load_recipe, never the full catalogue.
    @return Deterministic deduplicated target names to build before its first native observer.
    """
    targets = set()
    # Later ordinary captures can need another application target alongside an
    # earlier decoder. Compile their selected union first so a later configure/
    # link cannot replace an image that already owns native evidence in this run.
    for recipe in selection["recipes"].values():
        if recipe["target"]:
            targets.update(recipe["original_parameters"].get("build_targets", [recipe["target"]]))
    return sorted(targets)


def compile_plan(selection, configuration, root, prebuild_recipes=(), build_result=None):
    """Plan explicit future image targets or validate an explicitly selected current build.

    @param selection Current selected recipe closure; later recipes are not executed here.
    @param configuration Exact native configuration for every selected image.
    @param root Current checkout owning source, retained inputs and generated images.
    @param prebuild_recipes Explicit registered future graphs whose targets compile now.
    @param build_result Optional current native-build report path; never an implicit cache search.
    @return Bound compile scope with selected targets, future recipe names and optional build identity.
    """
    targets = set(selected_build_targets(selection))
    additional = sorted(set(prebuild_recipes))
    # A future target must be built by this graph's first native prerequisite;
    # an entirely pure conversion cannot silently claim to have compiled it.
    if additional and not targets:
        raise ValueError("explicit prebuild requires a selected native recipe closure")
    # Compile planned later observers before the first native image is observed.
    # Each future graph still performs its own source/native verification later;
    # target preparation grants no execution or financial acceptance for it.
    for name in additional:
        future = load_recipe(name, configuration, root)
        if not future["executable"]:
            raise ValueError(f"explicit prebuild recipe is pending or missing inputs: {name}")
        targets.update(selected_build_targets(future))
    reference = None
    if build_result is not None:
        if not targets:
            raise ValueError("a pure conversion cannot claim an unrequested native build")
        reference = identity(root / Path(build_result), root)
        required = dict(target=sorted(targets)[0], original_parameters=dict(build_targets=sorted(targets)))
        verify_current_build(dict(result=reference), configuration, required, root)
    return dict(targets=sorted(targets), prebuild_recipes=additional, provided_build=reference)


def execution_signature(selection, root, compilation):
    """Bind restart and stage checks to the exact explicit compilation selection.

    @param selection Current recipe closure; its catalogue hash binds future declarations too.
    @param root Current source/input owner.
    @param compilation Explicit compile_plan result, never a historical latest-build lookup.
    @return Existing source/input signature extended with this invocation's compile intent.
    """
    result = snapshot(selection, root)
    result["compilation"] = compilation
    return result


def prepare_recipe(name, configuration=None, root=None, run_id=None, resume=None, tool_overrides=None,
                   prebuild_recipes=(), build_result=None):
    """Execute one bounded selected closure, or explicitly resume identical interrupted work.

    @param name Exact registered selected recipe ID.
    @param configuration Exact independent native configuration when required.
    @param root Optional checkout, including a cold rehearsal root.
    @param run_id Optional fresh explicit invocation ID; duplicates always refuse.
    @param resume Explicit existing invocation ID, mutually exclusive with run_id.
    @param tool_overrides Optional current native-build external prerequisite paths.
    @param prebuild_recipes Explicit registered future graph names whose images compile before observers.
    @param build_result Explicit current native-build report path to reuse after full current-source/image checks.
    @return Exact completed rebuild report identity; failures retain incomplete state.
    """
    root = repository_root(root)
    selection = load_recipe(name, configuration, root)
    if not selection["executable"]:
        raise ValueError(f"recipe prerequisites are pending/missing: {selection['pending_groups']} {selection['missing_inputs']}")
    if run_id is not None and resume is not None:
        raise ValueError("choose a fresh run or explicit resume, never both")
    compilation = compile_plan(selection, configuration, root, prebuild_recipes, build_result)
    signature = execution_signature(selection, root, compilation)
    compile_targets = compilation["targets"]
    if resume is None:
        context = create_run_context("rebuild", name, run_id=run_id or ("rebuild-" + uuid.uuid4().hex), root=root, profile="rebuild")
        state = dict(_documentation="Owner: r2fc-file-cleanup3.md14; generated stage progress, no gameplay authority.",
            schema_version=1, kind="artifact_rebuild_result", complete=False, status="reserved", run_id=context.run_id,
            signature=signature, nodes={item: dict(status="pending", stages={}, outputs={}, attempts={}, history=[]) for item in selection["order"]})
    else:
        context = RunContext(root, "rebuild", identifier(name), identifier(resume), "rebuild")
        state = json.loads((context.reports / "rebuild.json").read_text(encoding="utf-8"))
        marker = root / "mlm/runtime/.runs" / context.run_id
        # Resume belongs to an exact prior reservation/source/input selection;
        # a completed run, changed producer or coincident directory is insufficient.
        if not marker.is_dir() or state["run_id"] != context.run_id or state["signature"] != signature or state["complete"]:
            raise ValueError("resume identity/source/input selection changed or already completed")
    report = context.reports / "rebuild.json"
    lock = context.reports / "rebuild.lock"
    acquire_lock(lock, resume is not None)
    try:
        state.pop("error", None)
        write_state(report, state)
        # Dependencies complete first, once each. A failed prerequisite stops
        # every dependent stage; successful prior nodes are rehashed on resume.
        for item in selection["order"]:
            recipe, node = selection["recipes"][item], state["nodes"][item]
            current = context.for_feature(recipe["feature"], recipe["scenario"])
            for stage in ("prepare", "verify"):
                if node["stages"].get(stage, {}).get("status") == "passed":
                    verify_passed_stage(node["stages"][stage], root)
            if node["status"] == "verified":
                for reference in node["outputs"].values():
                    if identity(root / reference["path"], root) != reference:
                        raise ValueError("completed prerequisite output changed before resume")
                continue
            dependencies = {}
            for requirement in recipe["requires"]:
                parent = state["nodes"][requirement["recipe"]]
                if parent["status"] != "verified":
                    raise ValueError("recipe prerequisite has not actually verified")
                dependencies[requirement["as"]] = parent["outputs"][requirement["role"]]
            if node["stages"].get("build", {}).get("status") not in ("passed", "not_required"):
                if recipe["target"]:
                    from native_build import build_selected
                    # A supplied current build is an explicit checked prerequisite,
                    # not inherited native success. Every leaf still executes its
                    # own prepare/verify stages and rechecks the actual images.
                    shared = (dict(status="passed", result=compilation["provided_build"], provided_current_build=True)
                              if compilation["provided_build"] is not None else
                              shared_current_build(state, configuration, recipe, root))
                    if shared is not None:
                        node["stages"]["build"] = shared
                    else:
                        node["stages"]["build"] = dict(status="running")
                        write_state(report, state)
                        try:
                            built = build_selected(None, configuration, compile_targets, tool_overrides, root)
                        except BaseException as error:
                            node["stages"]["build"] = dict(status="failed", error=str(error))
                            raise
                        node["stages"]["build"] = dict(status="passed", result=built)
                else:
                    node["stages"]["build"] = dict(status="not_required")
                write_state(report, state)
            if node["stages"]["build"]["status"] == "passed":
                verify_current_build(node["stages"]["build"], configuration, recipe, root)
            # Requests bind actual input/producer bytes and all generated roles.
            # Only actual verification plus present outputs can complete a node.
            for stage in ("prepare", "verify"):
                if node["stages"].get(stage, {}).get("status") == "passed":
                    continue
                if execution_signature(selection, root, compilation) != signature:
                    raise ValueError("recipe source/input changed during execution")
                attempt = node["attempts"].get(stage, 0) + 1
                node["attempts"][stage] = attempt
                outputs = {row["role"]: contained(current.directory / row["path"].format(configuration=configuration), current.directory).relative_to(root).as_posix()
                           for row in recipe["outputs"]}
                request_document = dict(_documentation="Owner: r2fc-file-cleanup3.md14; explicit current recipe stage.",
                    schema_version=1, kind="artifact_recipe_stage_request", recipe=item, stage=stage,
                    declaration=recipe,
                    configuration=configuration, run_id=context.run_id, feature=recipe["feature"], scenario=recipe["scenario"],
                    inputs=dict(signature["inputs"][item], **dependencies), outputs=outputs,
                    sources=signature["sources"], build=node["stages"]["build"],
                    result_path=(current.reports / f"recipe-{stage}-{attempt}-result.json").relative_to(root).as_posix(),
                    publishes_source=False, paid_work_authorized=False)
                request = current.reports / f"recipe-{stage}-{attempt}.json"
                write_state(request, request_document)
                node["status"] = "running"
                state["status"] = "running"
                write_state(report, state)
                outcome = run_stage(current, stage, request, lock, attempt)
                node["history"].append(dict(stage=stage, attempt=attempt, **outcome))
                node["stages"][stage] = dict(status="passed" if outcome["exit_status"] == 0 else "failed", **outcome)
                write_state(report, state)
                if outcome["exit_status"] != 0:
                    raise ValueError(f"recipe {item}/{stage} failed; see {outcome['terminal']['path']}")
            node["outputs"] = {row["role"]: identity(current.directory / row["path"].format(configuration=configuration), root)
                               for row in recipe["outputs"]}
            node["status"] = "verified"
            write_state(report, state)
        if execution_signature(selection, root, compilation) != signature:
            raise ValueError("recipe source/input changed before completion")
        state.update(complete=True, status="verified")
        write_state(report, state)
        return identity(report, root)
    except BaseException as error:
        # A finished but invalid child is failed, not still running. Preserve
        # verified predecessors while leaving every affected node incomplete.
        for node in state["nodes"].values():
            if node["status"] == "running" or any(stage.get("status") in ("running", "failed") for stage in node["stages"].values()):
                node["status"] = "failed"
        state.update(complete=False, status="failed", error=str(error))
        write_state(report, state)
        raise
    finally:
        lock.unlink()


def main():
    """Inspect a registered recipe or explicitly execute/resume its exact selected run.

    @return None; actual failure status is preserved for scripts and callers.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("recipe")
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--inspect", action="store_true")
    parser.add_argument("--run-id")
    parser.add_argument("--resume")
    parser.add_argument("--prebuild-recipe", action="append", default=[], help="Compile this registered later graph's targets before the first observer; do not execute it")
    parser.add_argument("--build-result", type=Path, help="Explicit current build to verify/reuse; never searches historical outputs")
    for name in ("cmake", "visual-studio", "vcpkg", "grfcodec", "git", "patch", "windows-sdk"):
        parser.add_argument("--" + name, type=Path)
    arguments = parser.parse_args()
    if arguments.inspect:
        result = load_recipe(arguments.recipe, arguments.configuration)
        # Inspection exposes the same explicit compile scope without reserving
        # a run or executing a future recipe, so callers can review the union.
        if arguments.prebuild_recipe or arguments.build_result is not None:
            result["compilation"] = compile_plan(result, arguments.configuration, repository_root(),
                                                  arguments.prebuild_recipe, arguments.build_result)
        result.pop("recipes")
    else:
        overrides = {name: value for name, value in vars(arguments).items()
                     if name in ("cmake", "visual_studio", "vcpkg", "grfcodec", "git", "patch", "windows_sdk") and value is not None}
        result = prepare_recipe(arguments.recipe, arguments.configuration, run_id=arguments.run_id,
                                resume=arguments.resume, tool_overrides=overrides,
                                prebuild_recipes=arguments.prebuild_recipe, build_result=arguments.build_result)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
