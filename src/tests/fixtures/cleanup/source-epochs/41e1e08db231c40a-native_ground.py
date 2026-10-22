"""Execute bounded ground acceptance commands and retain actual owner/artifact evidence.

@file native_ground.py
@see r2mlvs15-integrated-acceptance-and-balance.md Groups1-5
@see r2mlvs3mp-master-plan.md checkpoint11.1
@see r2mlvs14-savegame-determinism-and-lifecycle.md canonical serialized owners
"""

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context
CONTEXT = current_run_context("acceptance", "native-ground")
sys.path.insert(0, str(ROOT / "src/tests"))
from tbm_logistics_save_fixture import owned_chunks, saved_table
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks

SCHEMA_VERSION = 2
PRODUCERS = {
    "src/multilayer_content.h": ("MultiLayerGameplayProfile", "CheckDeepLayerAvailability"),
    "src/excavation.h": ("CheckExcavatedFootprint", "CommitExcavationCut"),
    "src/layer_portal_cmd.h": ("CommissionLayerPortalTBM", "LayerPortalWorkState"),
    "src/tbm.h": ("TBMProject", "AdvanceTBMWork"),
    "src/tbm_exchange.h": ("ExecuteTBMWagonExchange", "ValidateTBMWagonExchange"),
    "src/multilayer_state.h": ("MultiLayerStateSchema", "ValidateMultiLayerStateReferences"),
}
FIXTURES = {
    "S1": {"kind": "economics", "classification": "isolated_boundary",
           "source": "src/tests/underground_workflow_fixture.h",
           "shortcuts": ["fixture-created company/calendar/geology", "declared dry Surface and completed receiver setup"],
           "layer": 1},
    "S2": {"kind": "economics", "classification": "isolated_boundary",
           "source": "src/tests/underground_workflow_fixture.h",
           "shortcuts": ["fixture-created company/calendar/geology", "pre-existing excavated Underground assembly/access floor",
                         "declared dry Surface and completed receiver setup"], "layer": 3},
    "S8": {"kind": "failure_boundaries", "classification": "isolated_boundary",
           "source": "src/tests/tbm_recovery.cpp",
           "shortcuts": ["declared native owner fixtures and individually triggered recovery/refusal conditions"]},
    "S9": {"kind": "native_replay", "classification": "isolated_boundary",
           "source": "src/tests/fixtures/haulage/source-loop/source-native-deep-stopped.sav",
           "shortcuts": ["retained producer-created stopped checkpoint with declared pre-owned logistics"], "layer": 3},
}


class EvidenceError(RuntimeError):
    """A refused evidence claim; no failure or absent artifact may become a passing scenario."""


def repository_path(value, require_file=False):
    """Resolve one read/write artifact inside the shared authorized repository.

    @param value Repository-relative or absolute local path.
    @param require_file Whether an existing regular input is mandatory.
    @return Canonical authorized path; traversal/junction escapes and missing inputs refuse.
    """
    path = (ROOT / value).resolve()
    if not path.is_relative_to(ROOT):
        raise EvidenceError("artifact escapes repository: " + str(path))
    # Saved inputs may use stable migration keys, but never an implicit latest run.
    if require_file and path.suffix == ".sav" and not path.is_file():
        try:
            path = resolve_input_path(path)
        except ValueError:
            pass
    if require_file and not path.is_file():
        raise EvidenceError("required artifact missing: " + str(path))
    return path


def file_identity(path):
    """Hash actual bytes without modifying the source artifact.

    @param path Existing repository file.
    @return Stable relative path, SHA256 and actual byte count.
    """
    path = repository_path(path, True)
    data = path.read_bytes()
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data)}


def capture_ledger(runtime_artifacts):
    """Decode actual native saved owners, preserving their sole quantity/identity authority.

    @param runtime_artifacts Existing native saves emitted by the actual loader/game loop.
    @return Actual owner byte hashes and decoded sparse pool tables; absent/unsupported transport refuses.
    """
    ledgers = []
    # A complete ground record includes geometry, actual packets/vehicles,
    # invoices and work queues together; a hand-written expected balance cannot
    # substitute for any missing physical owner.
    for artifact in runtime_artifacts:
        path = repository_path(artifact, True)
        version, owners = owned_chunks(path)
        _, stream = read_stream(path)
        records = chunks(stream)
        names = {record["name"] for record in records}
        if (b"UPLH" in names) != (b"UPLD" in names):
            raise EvidenceError("partial utility transport: " + str(path))
        for record in records:
            if record["name"] in (b"UPLH", b"UPLD"):
                owners[record["name"]] = record["raw"]
        tables = {}
        # These public serialized tables expose each real pool ID and stock;
        # opaque packed-map/header chunks remain exact bytes rather than a
        # second gameplay registry invented by the runner.
        for name in (b"VEHS", b"CAPA", b"PLYR", b"TBMP", b"TBXA", b"SPRR", b"LPWK"):
            tables[name.decode("ascii")] = saved_table(path, name)
        ledgers.append({"artifact": file_identity(path), "save_version": version,
                        "owners": {name.decode("ascii"): {"sha256": hashlib.sha256(raw).hexdigest(), "bytes": len(raw)}
                                   for name, raw in sorted(owners.items())}, "tables": tables})
    if not ledgers:
        raise EvidenceError("no actual runtime save artifacts")
    return ledgers


def compare_state_digest(reference, replay):
    """Compare complete actual serialized owner state including the saved clocks.

    @param reference Actual reference save at the agreed simulation boundary.
    @param replay Actual independently loaded/replayed save at that same boundary.
    @return Canonical owner digest on exact equality; this is not host/client join evidence.
    """
    left_version, left = owned_chunks(repository_path(reference, True))
    right_version, right = owned_chunks(repository_path(replay, True))
    if left_version != right_version:
        raise EvidenceError("different save transport versions")
    left_records = chunks(read_stream(repository_path(reference, True))[1])
    right_records = chunks(read_stream(repository_path(replay, True))[1])
    # Electricity must participate when present; an otherwise matching ground
    # snapshot cannot hide a changed grant or missing utility owner.
    for target, records in ((left, left_records), (right, right_records)):
        names = {record["name"] for record in records}
        if (b"UPLH" in names) != (b"UPLD" in names):
            raise EvidenceError("partial utility transport in digest input")
        for record in records:
            if record["name"] in (b"UPLH", b"UPLD"):
                target[record["name"]] = record["raw"]
    differences = sorted(name.decode("ascii") for name in left.keys() | right.keys() if left.get(name) != right.get(name))
    if differences:
        raise EvidenceError("owner/clock mismatch: " + ", ".join(differences))
    digest = hashlib.sha256()
    # Explicit chunk labels/lengths delimit the canonical byte stream; object
    # addresses, raw structure padding and client camera state never enter it.
    for name, raw in sorted(left.items()):
        digest.update(name)
        digest.update(len(raw).to_bytes(8, "big"))
        digest.update(raw)
    return {"sha256": digest.hexdigest(), "owner_count": len(left), "comparison": "saved_owner_and_clock_equality"}


def execute_command(argv, cwd, log, timeout_seconds=110, environment=None):
    """Run one bounded real process, retaining exact arguments, status and stdout/stderr.

    @param argv Structured executable/argument list; no shell interpolation.
    @param cwd Repository working directory required by the actual fixture.
    @param log Capture path in the repository logs directory.
    @param timeout_seconds Per-process deadline below the two-minute fixture limit.
    @param environment Optional explicit inherited environment plus scenario hooks.
    @return Actual command record; a failed/expired process raises with its retained evidence.
    """
    cwd = repository_path(cwd)
    log = repository_path(log)
    if not log.is_relative_to(CONTEXT.logs):
        raise EvidenceError("application capture must live in this invocation's logs/")
    log.parent.mkdir(parents=True, exist_ok=True)
    started = time.perf_counter()
    try:
        with log.open("wb") as sink:
            result = subprocess.run([str(argument) for argument in argv], cwd=cwd, env=CONTEXT.environment(environment),
                                    stdout=sink, stderr=subprocess.STDOUT, timeout=timeout_seconds,
                                    creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0, check=False)
    except subprocess.TimeoutExpired as error:
        raise EvidenceError("command timed out; evidence: " + str(log)) from error
    record = {"argv": [str(argument) for argument in argv], "cwd": cwd.relative_to(ROOT).as_posix(),
              "exit_status": result.returncode, "timeout_seconds": timeout_seconds,
              "wall_seconds": time.perf_counter() - started, "log": file_identity(log)}
    if result.returncode != 0:
        raise EvidenceError("command failed with status " + str(result.returncode) + "; evidence: " + str(log))
    return record


def validate_art_manifest(log, manifest=None):
    """Run the canonical gallery validator against one exact generated proof set.

    @param log Actual validator process log owned by this acceptance invocation.
    @param manifest Optional explicit art-evidence manifest; default is the named proof cache.
    @return Current verified contract/gallery/payload identities; missing proof refuses.
    """
    sys.path.insert(0, str(ROOT / "media/multilayer"))
    from evidence import EvidenceSet, DEFAULT_MANIFEST
    selection = EvidenceSet(manifest or DEFAULT_MANIFEST)
    command = execute_command([sys.executable, ROOT / "media/multilayer/review_m1_art.py",
                               "--evidence-manifest", selection.path], ROOT, log)
    files = (ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json",
             selection.output("packed"), selection.output("runtime"), selection.output("gallery"),
             ROOT / "media/baseset/openttd.grf")
    return {"scope": "selected_M1_42_families", "command": command,
            "evidence_manifest": file_identity(selection.path), "artifacts": [file_identity(path) for path in files]}


def validate_fixture(scenario_id, fixture, bootstrap_claim):
    """Enforce immutable known fixture classification before executing a requested claim.

    @param scenario_id Known delivered scenario ID.
    @param fixture Optional caller metadata, which cannot relabel a seeded fixture.
    @param bootstrap_claim Whether the caller requests full real-bootstrap acceptance.
    @return Exact known fixture definition; shortcuts/unavailable scenarios refuse bootstrap claims.
    """
    if scenario_id not in FIXTURES:
        raise EvidenceError("scenario has no delivered owner/fixture: " + scenario_id)
    definition = FIXTURES[scenario_id]
    if fixture is not None:
        # Caller metadata cannot launder development setup into first-build
        # acceptance by deleting shortcuts or changing its classification.
        if fixture.get("classification") != definition["classification"] or fixture.get("shortcuts") != definition["shortcuts"]:
            raise EvidenceError("fixture classification/shortcut metadata differs from its actual producer")
    if bootstrap_claim:
        if definition["classification"] != "real_bootstrap" or definition["shortcuts"]:
            raise EvidenceError("bootstrap claim refused: declared seeded/test-only predecessors")
    repository_path(definition["source"], True)
    return definition


def economics_ledger(log, layer):
    """Read actual observed native invoice/material output rather than synthesizing expected costs.

    @param log Actual successful native economics stdout.
    @param layer Selected saved layer identity for this scenario.
    @return Exact measured native fields after checking their complete invoice equation.
    """
    text = repository_path(log, True).read_text(encoding="utf-8", errors="replace")
    required = {"layer", "seed", "date", "ticks", "cuts", "exchanges", "setup", "purchase", "work_support",
                "active_cost", "idle_cost", "haulage", "disposal", "total", "earth_t", "rock_t"}
    for section in text.split("MLVS_TBM_PROJECT ")[1:]:
        # Catch wraps long diagnostic values after '='. Whitespace is only
        # transport formatting; retain the actual numeric value across wraps.
        values = dict(re.findall(r"([a-z_]+)=\s*([0-9][0-9-]*)", section.split("-------------------------------------------------------------------------------")[0]))
        if values.get("layer") != str(layer):
            continue
        if not required.issubset(values):
            raise EvidenceError("actual economics record misses required owner fields")
        result = {key: value if key == "date" else int(value) for key, value in values.items()}
        categories = ("setup", "purchase", "work_support", "active_cost", "idle_cost", "haulage", "disposal")
        if sum(result[key] for key in categories) != result["total"]:
            raise EvidenceError("actual company invoice categories do not balance")
        return result
    raise EvidenceError("actual selected-layer economics record missing")


def run_scenario(scenario_id, fixture=None, config="src/tests/fixtures/config/multilayer/native-art.cfg", configuration="Debug",
                 bootstrap_claim=False, presentation="headless", art_evidence=None):
    """Execute one delivered ground evidence scope without bypassing fixture/producer/artifact gates.

    @param scenario_id S1/S2 native economics, S8 actual owner failure boundaries or S9 saved full-loop continuation.
    @param fixture Optional metadata matching the real known producer classification.
    @param config Existing repository runtime config; native S9 uses the verified native-art.cfg.
    @param configuration Debug or RelWithDebInfo executable configuration.
    @param bootstrap_claim Request full bootstrap proof; current seeded fixture definitions explicitly refuse.
    @param presentation Actual S9 headless/visible/hidden viewport policy; no simulation differences are injected.
    @param art_evidence Exact regenerated art proof manifest; omitted only for the documented cache default.
    @return Evidence record from real commands and artifacts; any missing producer, bad claim or failed command refuses.
    """
    definition = validate_fixture(scenario_id, fixture, bootstrap_claim)
    if configuration not in ("Debug", "RelWithDebInfo"):
        raise EvidenceError("unsupported build configuration")
    if presentation not in ("headless", "visible", "hidden"):
        raise EvidenceError("unsupported presentation policy")
    config = repository_path(config, True)
    CONTEXT.prepare_runtime(config.name)
    executable = repository_path("build/" + configuration + "/openttd_test.exe", True)
    producers = []
    # Source presence is a prerequisite only; the following actual executable
    # owner fixture supplies behavior evidence for the selected scenario.
    for filename, exports in PRODUCERS.items():
        path = repository_path(filename, True)
        text = path.read_text(encoding="utf-8")
        for export in exports:
            if not re.search(r"\b" + re.escape(export) + r"\b", text):
                raise EvidenceError("required public export absent: " + filename + ":" + export)
        producers.append(file_identity(path))
    if definition["kind"] == "failure_boundaries":
        utility = repository_path("src/utility_power.h", True)
        utility_text = utility.read_text(encoding="utf-8")
        # Selected electrical refusal tests need their real atomic owner as
        # well as the ground producer list; missing APIs cannot become a pass.
        for export in ("PrepareUtilityMutation", "CommitUtilityMutation"):
            if not re.search(r"\b" + export + r"\b", utility_text):
                raise EvidenceError("required utility export absent: " + export)
        producers.append(file_identity(utility))
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S%f")
    basename = "acceptance-" + scenario_id.lower() + "-" + configuration.lower() + "-" + presentation + "-" + stamp
    log = CONTEXT.logs / ("r2mlvs3mp-z-a-" + basename + ".log")
    protected = [config, repository_path(definition["source"], True)]
    before = {path: file_identity(path) for path in protected}
    result = {"_documentation": "Owner: r2mlvs15-integrated-acceptance-and-balance.md Groups1-5; master11.1",
              "schema_version": SCHEMA_VERSION, "scenario_id": scenario_id,
              "feature": CONTEXT.feature, "scenario": CONTEXT.scenario, "run_id": CONTEXT.run_id,
              "families": ["content", "geology", "portal", "TBM", "exchange", "schema", "selected_M1_art"],
              "classification": definition["classification"], "shortcuts": definition["shortcuts"],
              "bootstrap_claim": bootstrap_claim, "configuration": configuration,
              "executable": file_identity(executable), "config": before[config], "producers": producers,
              "fixture": before[repository_path(definition["source"], True)]}
    revision = subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True, check=True)
    result["source_revision"] = revision.stdout.strip()
    result["runner"] = file_identity(__file__)
    generated_contract = repository_path("build/generated/table/multilayer_content_manifest.h", True)
    contract_text = generated_contract.read_text(encoding="utf-8")
    result["declared_context"] = {
        "profile_id": 1,
        "profile_version": int(re.search(r"_multilayer_profile_version = (\d+)", contract_text)[1]),
        "package_version": int(re.search(r"_multilayer_package_version = (\d+)", contract_text)[1]),
        "map_seed": 420 if definition["kind"] == "economics" else "actual saved/individual fixture metadata",
        "calendar_date": "1980-01-01" if definition["kind"] == "economics" else "actual saved/individual fixture metadata",
        "starting_money": 10000000 if definition["kind"] == "economics" else "individual actual fixture/checkpoint",
    }
    result["toolchain"] = {"cmake_cache": file_identity("build/CMakeCache.txt"),
                          "generated_content_contract": file_identity(generated_contract),
                          "required_package": file_identity("build/newgrf/multilayer_industry.grf")}
    result["working_source"] = [file_identity(path) for path in
                                ("src/tests/tbm_economics.cpp", "src/tests/tbm_logistics_runtime.cpp",
                                 "src/tests/underground_workflow_fixture.h", "src/saveload/saveload.h")]
    try:
        result["art"] = validate_art_manifest(CONTEXT.logs / ("r2mlvs3mp-z-a-" + basename + "-art.log"), art_evidence)
        if definition["kind"] == "economics":
            result["command"] = execute_command([executable, "[tbm-economics]"], ROOT / "build", log)
            text = log.read_text(encoding="utf-8", errors="replace")
            if "All tests passed" not in text or "No test cases matched" in text:
                raise EvidenceError("native owner test did not prove its selected boundary")
            result["ledger"] = economics_ledger(log, definition["layer"])
        elif definition["kind"] == "failure_boundaries":
            result["command"] = execute_command([executable, "[tbm-recovery],[tbm-exchange-proposal],[utility-power-mutation]"],
                                                ROOT / "build", log)
            text = log.read_text(encoding="utf-8", errors="replace")
            if "All tests passed" not in text or "No test cases matched" in text:
                raise EvidenceError("actual recovery/refusal owners did not pass")
            result["boundary_tests"] = "native cancellation/origin loss/reassignment/atomic exchange and utility refusal"
        else:
            # Native replay must use the verified template bytes, regardless of
            # whether the caller supplied its immutable or materialized location.
            if config.read_bytes() != (ROOT / "src/tests/fixtures/config/multilayer/native-art.cfg").read_bytes():
                raise EvidenceError("native replay fixture must use its actual native-art.cfg")
            prefix = CONTEXT.saves / basename
            startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
            existed = startup.exists()
            previous = startup.read_bytes() if existed else None
            environment = dict(os.environ, OTTD_TBM_LOGISTICS_SOURCE=str(repository_path(definition["source"], True)),
                               OTTD_TBM_LOGISTICS_OUTPUT=str(prefix), OTTD_TBM_LOGISTICS_COMPLETE_LOOP="1",
                               OTTD_TBM_LOGISTICS_VIEW=presentation, OTTD_M1_LOOP_ART="1")
            try:
                startup.write_text("# @see r2mlvs15-integrated-acceptance-and-balance.md Group5 actual native continuation.\nlogistics_probe\n", encoding="utf-8")
                result["command"] = execute_command([executable, "[.tbm-logistics-runtime]"],
                                                    ROOT / "build", log, environment=environment)
            finally:
                if existed:
                    startup.write_bytes(previous)
                else:
                    startup.unlink()
            text = log.read_text(encoding="utf-8", errors="replace")
            match = re.search(r"LOGISTICS_COMPLETE_LOOP ticks=(\d+) exchanges=(\d+) presentation=" + presentation, text)
            if match is None or "All tests passed" not in text:
                raise EvidenceError("actual native full loop did not complete and dispose its carried spoil")
            result["simulation_ticks"] = int(match[1])
            result["exchanges"] = int(match[2])
            result["presentation"] = presentation
            result["ledger"] = capture_ledger([Path(str(prefix) + "-loaded.sav"), Path(str(prefix) + "-complete-loop.sav")])
        result["status"] = "passed"
        result["claim_scope"] = "actual_saved_full_loop_continuation" if definition["kind"] == "native_replay" else "declared_native_boundary"
        return result
    finally:
        # Source saves/configs are read-only inputs even on exceptions or timeouts.
        # Restoring the startup helper does not authorize changing those inputs.
        for path, identity in before.items():
            if file_identity(path) != identity:
                raise EvidenceError("protected input changed: " + str(path))


def main():
    """Run one explicitly selected scenario and save its actual bounded evidence record.

    @return None; refusals exit nonzero and never emit a passing record.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--scenario", required=True, choices=tuple(FIXTURES))
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    parser.add_argument("--presentation", choices=("headless", "visible", "hidden"), default="headless")
    parser.add_argument("--bootstrap-claim", action="store_true")
    parser.add_argument("--art-evidence", type=Path, help="exact native-art proof manifest")
    parser.add_argument("--output", default="acceptance.json", help="new report basename within this feature run")
    arguments = parser.parse_args()
    output = CONTEXT.output("reports", arguments.output)
    if output.name.startswith("r2") or output.suffix != ".json":
        raise EvidenceError("evidence JSON requires a non-r2 basename in this run")
    if output.exists():
        raise EvidenceError("refuse replacing existing evidence: " + str(output))
    result = run_scenario(arguments.scenario, configuration=arguments.configuration,
                          bootstrap_claim=arguments.bootstrap_claim, presentation=arguments.presentation, art_evidence=arguments.art_evidence)
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print("Verified " + result["claim_scope"] + "; record=" + str(output), flush=True)


if __name__ == "__main__":
    try:
        main()
    except (EvidenceError, AssertionError, OSError, ValueError) as error:
        print("Acceptance refused: " + str(error), file=sys.stderr)
        raise SystemExit(2)
