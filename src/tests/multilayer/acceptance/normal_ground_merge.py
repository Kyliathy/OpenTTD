"""Continue exact genuinely financed originals through merged native gameplay.

@file normal_ground_merge.py Original-world operations, distinct from prepared components.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
@see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md
Historical receipts establish original paid provenance. Only newly executed
public commands, full application frames and actual saved ledgers are current proof.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import create_run_context
from artifact_rebuild import identity
from runner import run_native
from savegame_tables import compare_merged_worlds, complete_world, saved_owner_table

MANIFEST = "src/tests/fixtures/checkpoints/underground/openttd3-75ea75718f/normal-ground-originals.json"
SCENARIOS = dict(standard="normal-ground-boring", deep="normal-ground-deep-work",
                 irregular="normal-ground-irregular-work", useful="normal-ground-useful")
SOURCES = ("src/tests/normal_ground_merge.cpp", "src/tests/normal_ground_runtime_fixture.h",
           "src/tests/normal_ground_video_fixture.h", "src/tests/normal_ground_boring_fixture.h",
           "src/tests/normal_ground_useful_fixture.h", "src/tests/multilayer/acceptance/normal_ground_merge.py")
CONTENT_SHA = "c04fbf0d5525edc53250cfbd963f5e3bbe0d0bce655a3553c73b76483a8e8744"  # Independently accepted Group61/62container.


def checked(reference, root):
    """Verify an exact current or retained identity through the existing owner.

    @param reference Explicit path/SHA256 object, without latest-run discovery.
    @param root Selected source checkout.
    @return Physical contained file after exact full-byte verification.
    """
    from artifact_inventory import checked_path
    path = checked_path(reference["path"], root)
    if identity(path, root) != reference:
        raise ValueError("paid-world input changed: " + reference["path"])
    return path


def read(reference, root):
    """Read one verified original or current JSON witness.

    @param reference Explicit contained identity.
    @param root Selected checkout.
    @return Parsed original document without rewriting it.
    """
    return json.loads(checked(reference, root).read_text(encoding="utf-8"))


def select_original(scenario, root=ROOT):
    """Select a genuine paid predecessor from independent retained historical facts.

    @param scenario Standard, Deep, irregular or useful original-world case.
    @param root Checkout containing the preserved manifest and raw originals.
    @return Exact selected row after source, handoff and current profile checks.
    """
    if scenario not in SCENARIOS:
        raise ValueError("unknown genuine paid-world scenario")
    manifest = json.loads((root / MANIFEST).read_text(encoding="utf-8"))
    selected = manifest["scenarios"][scenario]
    handoff = read(selected["historical_handoff"], root)
    original = handoff["records"][manifest["source_configuration"]]
    # A native-looking file alone is insufficient: its complete original hash
    # must be the paid readiness world named by the independent sealed handoff.
    if original["ready_input"] != selected["original_source"] or original["ready_input"]["sha256"] != selected["source"]["sha256"] or original["ready"] != selected["previous"]:
        raise ValueError("paid-world original provenance disagrees")
    checked(selected["source"], root)
    checked(selected["config"], root)
    return selected


def run_case(context, image, tag, environment):
    """Execute one bounded real native callback and retain its process evidence.

    @param context Fresh selected native invocation with matching feature/scenario.
    @param image Current actual Tube executable identity, never an old donor EXE.
    @param tag Sole explicit Catch2 callback tag.
    @param environment Explicit selected original/current input and numeric controls.
    @return Process identity, stdout text and retained Catch2 terminal identity.
    """
    root = context.root
    application = checked(image, root)
    terminal, stdout = context.evidence / "native.txt", context.evidence / "stdout.txt"
    with stdout.open("wb") as sink:
        run_native(context, [str(application), tag, "--reporter", "compact", "--out", str(terminal)],
                   cwd=root / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110, check=True)
    text = stdout.read_text(encoding="utf-8", errors="replace")
    result = terminal.read_text(encoding="utf-8", errors="replace")
    # A passing unrelated case cannot supply paid operation evidence. Require
    # this sole callback and its actual successful owned process/transcripts.
    if re.search(r"(?:All tests passed|Passed 1 test case)", result) is None:
        raise ValueError("paid-world callback lacks its actual passing terminal")
    processes = list(context.reports.glob("native-*.json"))
    if len(processes) != 1:
        raise ValueError("paid-world callback requires one exact native process")
    checked(image, root)
    return identity(processes[0], root), text, identity(terminal, root)


def migrate_original(scenario, build_reference, root=ROOT):
    """Load/copy/save the exact original through the existing digest-bound B reader.

    @param scenario Explicit genuinely paid predecessor selection.
    @param build_reference Current selected DebugTube build result identity.
    @param root Checkout containing the originals and native owner.
    @return Distinct actual migration result for the subsequent original-world operation.
    """
    selected = select_original(scenario, root)
    build = read(build_reference, root)
    image = build["targets"]["openttd_tube_test"]
    context = create_run_context("space", "machinery-lineage-original", run_id="merge65-migrate-" + scenario + "-" + uuid.uuid4().hex[:10], root=root)
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("machinery_lineage_test\n", encoding="utf-8")
    observations = json.loads((root / "src/tests/fixtures/saveload/lineage-originals.json").read_text())["observations"]
    bootstrap = next(row for row in observations if row["origin"] == "B" and row["version"] == 427)
    bootstrap_reference = {key: bootstrap[key] for key in ("path", "sha256")}
    output = context.saves / "original-441.sav"
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment.update(OTTD_TEST_CONFIGURATION=build["configuration"], OTTD_MACHINERY_SOURCE=str(checked(selected["source"], root)),
        OTTD_MACHINERY_DIGEST=selected["source"]["sha256"], OTTD_MACHINERY_BOOTSTRAP_INPUT=str(checked(bootstrap_reference, root)),
        OTTD_MACHINERY_OUTPUT=str(output))
    process, text, terminal = run_case(context, image, "[.machinery-lineage-native]", environment)
    if "MACHINERY_LINEAGE_NATIVE" not in text:
        raise ValueError("actual original migration observation missing")
    families = compare_merged_worlds(output, Path(str(output) + ".again.sav"))
    # Migration may normalize format metadata, but cannot finance the original
    # world or change its already paid company books before work begins.
    if saved_owner_table(checked(selected["source"], root), b"PLYR") != saved_owner_table(output, b"PLYR"):
        raise ValueError("original migration changed paid company accounting")
    document = dict(_documentation=__doc__, kind="genuine_paid_original_migration", complete=True, scenario=scenario,
        original=selected["source"], historical_handoff=selected["historical_handoff"], build=build_reference,
        source_epoch=build["source_epoch"], application=image, output=identity(output, root),
        second_save=identity(Path(str(output) + ".again.sav"), root), process=process, terminal=terminal,
        compared_families=families, gameplay_ticks_executed=0)
    target = context.reports / "genuine-paid-migration.json"
    target.write_text(json.dumps(document, indent=2) + "\n")
    return identity(target, root)


def execute(scenario, build_reference, migration_reference, ticks=16384, root=ROOT):
    """Run a minimum original-world paid operation through complete native frames.

    @param scenario Explicit Standard/Deep/irregular/useful case.
    @param build_reference Current selected native build identity.
    @param migration_reference Actual same-build migration of this exact paid original.
    @param ticks Bounded maximum complete native opportunities; unfinished results refuse acceptance.
    @param root Checkout containing actual retained/current inputs.
    @return Actual successful operation receipt, with all128owners and company ledgers.
    """
    selected = select_original(scenario, root)
    build, migration = read(build_reference, root), read(migration_reference, root)
    image = build["targets"]["openttd_tube_test"]
    # The current operation must continue its own exact actual migration. A
    # completed component fixture or another original cannot claim this history.
    if migration["scenario"] != scenario or migration["build"] != build_reference or migration["original"] != selected["source"] or migration["application"] != image:
        raise ValueError("genuine paid operation selected a different migration/build")
    source = checked(migration["output"], root)
    context = create_run_context("acceptance", SCENARIOS[scenario], run_id="merge65-" + scenario + "-" + uuid.uuid4().hex[:10], root=root)
    context.prepare_runtime()
    context.config_path.write_bytes(checked(selected["config"], root).read_bytes())
    grfs = list(context.runtime_dir.rglob("openttd.grf"))
    if len(grfs) != 1:
        raise ValueError("paid operation requires one exact materialized content container")
    runtime_grf = identity(grfs[0], root)
    if runtime_grf["sha256"] != CONTENT_SHA:
        raise ValueError("paid operation materialized different accepted graphics")
    startup = "normal_ground_useful" if scenario == "useful" else "normal_ground_boring"
    (context.runtime_dir / "scripts/game_start.scr").write_text(startup + "\n", encoding="utf-8")
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment.update(OTTD_TEST_CONFIGURATION=build["configuration"], OTTD_NORMAL_GROUND_INPUT=str(source),
        OTTD_NORMAL_GROUND_TICKS=str(ticks), OTTD_NORMAL_GROUND_RETURNED=str(selected["previous"].get("returned", 0)),
        OTTD_NORMAL_GROUND_RESUMED=str(selected["previous"].get("resumed", 0)))
    if scenario == "useful":
        environment.update(OTTD_NORMAL_GROUND_CARRIER=str(selected["machinery"]["carrier"]),
                           OTTD_NORMAL_GROUND_CAPSULE=str(selected["machinery"]["capsule"]))
    sources = [identity(root / name, root) for name in SOURCES]
    process, text, terminal = run_case(context, image, "[." + SCENARIOS[scenario] + "]", environment)
    marker = "NORMAL_GROUND_USEFUL" if scenario == "useful" else "NORMAL_GROUND_BORED"
    lines = [line for line in text.splitlines() if line.startswith(marker + " ")]
    if len(lines) != 1:
        raise ValueError("genuine operation lacks its sole real completion observation")
    observed = {key: int(value) for key, value in (token.split("=", 1) for token in lines[0].split()[1:])}
    if observed["complete"] != 1:
        raise ValueError("genuine paid operation did not complete; retain actual wait and continue explicitly")
    prefix = "useful-progress" if scenario == "useful" else "boring-progress"
    output, repeat = [context.saves / (prefix + suffix + ".sav") for suffix in ("", "-repeat")]
    families = compare_merged_worlds(output, repeat)
    version, owners, tables = complete_world(output)
    ledger = dict(version=version, owners={name.decode(): hashlib.sha256(raw).hexdigest() for name, raw in owners.items()},
                  tables={name.decode(): values for name, values in tables.items()})
    # Closing source/input identities bind what actually ran without launching
    # an unrelated test suite or scanning completed historical proof again.
    for reference in sources + [image, migration["output"], selected["source"]]:
        checked(reference, root)
    document = dict(_documentation=__doc__, kind="genuine_paid_ground_operation", complete=True,
        classification="genuine_original_continuation", bootstrap_repeated=False, prepared_world=False,
        scenario=scenario, configuration=build["configuration"], build=build_reference, application=image,
        migration=migration_reference, original=selected["source"], source=migration["output"], sources=sources,
        historical_handoff=selected["historical_handoff"], process=process, terminal=terminal,
        output=identity(output, root), repeat=identity(repeat, root), compared_families=families,
        observed=observed, ledger=ledger, native_observations=text.splitlines(),
        stdout=identity(context.evidence / "stdout.txt", root), runtime_grf=runtime_grf, human_acceptance=False)
    target = context.reports / "genuine-paid-operation.json"
    target.write_text(json.dumps(document, indent=2) + "\n")
    return identity(target, root)


def accept(operation_references, build_reference, root=ROOT):
    """Publish the four actual paid continuations with their complete public evidence.

    @param operation_references Explicit scenario-to-current-operation identities.
    @param build_reference Shared actual current Tube build identity.
    @param root Checkout containing current images and retained originals.
    @return Separate selected-scope acceptance identity; remaining65coverage stays explicit.
    """
    from space_checkpoint_foundation import validate_build_document
    if set(operation_references) != set(SCENARIOS):
        raise ValueError("paid continuation acceptance needs all four selected originals")
    build = read(build_reference, root)
    validate_build_document(build, read(build["source_epoch"], root), read(build["tool_profile"], root),
                            build["configuration"], root, True, required_targets=("openttd_tube_test",))
    rows = {}
    # Join each genuine original, exact running image, actual native completion,
    # full paused owner comparison and materialized accepted graphics. A copied
    # receipt or a prepared component cannot substitute for any actual process.
    for scenario, reference in operation_references.items():
        record = read(reference, root)
        process = read(record["process"], root)
        select_original(scenario, root)
        if record["build"] != build_reference or record["scenario"] != scenario or record["prepared_world"] is not False or record["observed"]["complete"] != 1:
            raise ValueError("paid continuation classification/build disagrees")
        if process["exit_status"] != 0 or process["executable"] != build["targets"]["openttd_tube_test"] or process["inputs"] != [record["source"]]:
            raise ValueError("paid continuation actual native process disagrees")
        compare_merged_worlds(checked(record["output"], root), checked(record["repeat"], root))
        for source in record["sources"]:
            checked(source, root)
        text = checked(record["stdout"], root).read_text(encoding="utf-8", errors="replace")
        if text.splitlines() != record["native_observations"]:
            raise ValueError("paid continuation observations differ from the actual transcript")
        checked(record["terminal"], root)
        checked(record["runtime_grf"], root)
        if record["runtime_grf"]["sha256"] != CONTENT_SHA:
            raise ValueError("paid continuation graphics differ from accepted content")
        rows[scenario] = dict(operation=reference, original=record["original"], output=record["output"],
                              process=record["process"], observed=record["observed"])
    context = create_run_context("acceptance", "normal-ground-merge", run_id="merge65-selected-" + uuid.uuid4().hex[:10], root=root)
    document = dict(_documentation=__doc__, kind="merged_genuine_paid_continuations", complete_selected_scope=True,
        build=build_reference, records=rows, graphics_sha256=CONTENT_SHA,
        coverage=["paid Standard clearing", "paid Deep clearing", "irregular retained-machine work", "useful modal/tube freight", "new canonical supply payment/delivery/retry"],
        remaining_group65=["installed geothermal binding", "original wait/refusal/recovery coverage"],
        bootstrap_repeated=False, human_acceptance=False)
    target = context.reports / "paid-continuations.json"
    target.write_text(json.dumps(document, indent=2) + "\n")
    return identity(target, root)
