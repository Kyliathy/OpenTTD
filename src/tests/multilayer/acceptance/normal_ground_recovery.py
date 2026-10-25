"""Qualify genuine exported waits/access and separately labelled recovery boundaries.

@file normal_ground_recovery.py Completion of the paid-original runtime handoff.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
@see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md
Closed paid-world proof retains its actual producing epoch. Only the new recovery
callbacks use this build; no old executable is required or relabelled current.
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
from normal_ground_merge import checked, read, CONTENT_SHA
from save_retention import resolve_exported_save
from savegame_tables import compare_merged_worlds, first_tick_worlds, complete_world, saved_owner_table


def fields(text, marker):
    """Read one actual integer-valued native observation, refusing combined runs.

    @param text Current or closed immutable actual stdout witness.
    @param marker Exact native owner observation prefix.
    @return Complete unique named integer fields.
    """
    rows = [line for line in text.splitlines() if line.startswith(marker + " ")]
    if len(rows) != 1:
        raise ValueError("expected one actual native observation: " + marker)
    return {key: int(value) for key, value in (item.split("=", 1) for item in rows[0].split()[1:])}


def run_selected(build_reference, scenario, tag, source=None, feature="acceptance", recorder=False, root=ROOT):
    """Execute only one selected recovery scope with complete process/source binding.

    @param build_reference Current actual selected Tube build identity.
    @param scenario Fresh process scenario; genuine observers use normal-ground-recovery.
    @param tag Exact case/tag selector, never the full native target suite.
    @param source Optional actual retained genuine world identity; absent for components.
    @param feature Existing feature owner, tbm for the existing logistics recorder.
    @param recorder Enable only the existing bounded component save recorder.
    @param root Checkout containing the declared inputs and current image.
    @return Invocation context, actual process/terminal/stdout identities and actual text.
    """
    build = read(build_reference, root)
    image = build["targets"]["openttd_tube_test"]
    context = create_run_context(feature, scenario, run_id="merge65b-" + uuid.uuid4().hex[:12], root=root)
    context.prepare_runtime()
    grfs = list(context.runtime_dir.rglob("openttd.grf"))
    if len(grfs) != 1:
        raise ValueError("recovery requires one exact materialized graphics container")
    runtime_grf = identity(grfs[0], root)
    if runtime_grf["sha256"] != CONTENT_SHA:
        raise ValueError("recovery materialized different accepted graphics")
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment["OTTD_TEST_CONFIGURATION"] = build["configuration"]
    if source is not None:
        environment["OTTD_NORMAL_GROUND_INPUT"] = str(checked(source, root))
        (context.runtime_dir / "scripts/game_start.scr").write_text("normal_ground_recovery\n", encoding="utf-8")
    if recorder:
        environment["OTTD_TBM_LOGISTICS_RECORD"] = "merged-recovery"
    sources = ("src/tests/normal_ground_recovery.cpp", "src/tests/normal_ground_runtime_fixture.h",
               "src/tests/normal_ground_video_fixture.h", "src/tests/save_first_tick_fixture.cpp",
               "src/tests/tbm_depot_stalls.cpp", "src/tests/transport_tube_recovery.cpp",
               "src/tests/multilayer/acceptance/normal_ground_recovery.py")
    identities = [identity(root / name, root) for name in sources]
    terminal, stdout = context.evidence / "native.txt", context.evidence / "stdout.txt"
    with stdout.open("wb") as sink:
        run_native(context, [str(checked(image, root)), tag, "--reporter", "compact", "--out", str(terminal)],
            cwd=root / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110, check=True)
    actual = terminal.read_text(encoding="utf-8", errors="replace")
    counts = re.search(r"Passed (?:(?:all|both) )?(\d+) test cases? with (\d+) assertions?", actual)
    if counts is None:
        raise ValueError("selected recovery lacks actual passing Catch2 counts")
    processes = list(context.reports.glob("native-*.json"))
    if len(processes) != 1:
        raise ValueError("selected recovery requires one exact owned process")
    # Source and image identities close only this actual new callback. Earlier
    # completed paid-world images remain frozen historical facts, never inputs.
    for reference in identities + [image] + ([source] if source is not None else []):
        checked(reference, root)
    proof = dict(process=identity(processes[0], root), terminal=identity(terminal, root), stdout=identity(stdout, root),
        image=image, build=build_reference, source=source, sources=identities, selector=tag,
        native_cases=int(counts[1]), native_assertions=int(counts[2]), runtime_grf=runtime_grf)
    return context, proof, stdout.read_text(encoding="utf-8", errors="replace")


def execute(build_reference, paid_acceptance, wait_proof, root=ROOT):
    """Complete genuine final access/reload and the minimum recovery component cases.

    @param build_reference Current new callback build identity.
    @param paid_acceptance Exact closed four-original qualification, kept at its own epoch.
    @param wait_proof Exact original wait save/process/stdout identities from that paid source.
    @param root Checkout containing committed exported saves and current native owners.
    @return Published recovery result; component worlds never acquire genuine classification.
    """
    paid = read(paid_acceptance, root)
    irregular = read(paid["records"]["irregular"]["operation"], root)
    useful = read(paid["records"]["useful"]["operation"], root)
    process = read(wait_proof["process"], root)
    historical_build = read(paid["build"], root)
    wait_text = checked(wait_proof["stdout"], root).read_text(encoding="utf-8", errors="replace")
    original_wait = fields(wait_text, "NORMAL_GROUND_USEFUL")
    # The saved wait must come from the same genuine useful predecessor and
    # actually passed native process. A prepared loaded train cannot replace it.
    if process["exit_status"] != 0 or process["executable"] != historical_build["targets"]["openttd_tube_test"] or process["inputs"] != [useful["source"]] or original_wait["complete"] != 0 or wait_proof["source"] not in process["artifacts"]:
        raise ValueError("actual genuine wait provenance disagrees")
    conversion_source = resolve_exported_save(irregular["output"], root)
    wait_source = resolve_exported_save(wait_proof["source"], root)
    context, conversion, text = run_selected(build_reference, "normal-ground-recovery", "[.normal-ground-conversion]", conversion_source, root=root)
    observed = fields(text, "GENUINE_GROUND_CONVERSION")
    before, output = context.saves / "conversion-before.sav", context.saves / "conversion-after.sav"
    compare_merged_worlds(before, context.saves / "conversion-before-repeat.sav")
    compare_merged_worlds(output, context.saves / "conversion-after-repeat.sav")
    # Public final access may change support/project/connection/payment owners.
    # It cannot consume another tick or change actual cargo packet history.
    first = complete_world(before)
    last = complete_world(output)
    if first[1][b"CAPA"] != last[1][b"CAPA"] or saved_owner_table(before, b"DATE") != saved_owner_table(output, b"DATE"):
        raise ValueError("final access changed cargo or simulation clocks")
    conversion.update(observed=observed, before=identity(before, root), output=identity(output, root),
        changed_families=sorted(name.decode() for name in first[1] if first[1][name] != last[1][name]),
        classification="genuine_original_continuation", gameplay_ticks_executed=0)
    context, waiting, text = run_selected(build_reference, "normal-ground-recovery", "[.normal-ground-wait]", wait_source, root=root)
    replay = first_tick_worlds(context.saves / "useful-wait")
    if "MERGED_FIRST_TICK_NATIVE" not in text:
        raise ValueError("genuine wait native first-frame marker missing")
    waiting.update(classification="genuine_original_continuation", original_wait=original_wait,
        saves={name: identity(replay[name], root) for name in ("before", "direct", "reloaded")},
        compared_families=replay["families"], actual_native_ticks_per_branch=1)
    context, depot, _ = run_selected(build_reference, "logistics", "[tbm-depot-infrastructure-files],[tbm-depot-capacity-files]", feature="tbm", recorder=True, root=root)
    depot.update(classification="prepared_component", bootstrap_claim=False,
        saves=[identity(path, root) for path in sorted(context.saves.glob("*.sav"))])
    # These existing component boundaries exercise saturation, unavailable
    # empties and real route repair. They remain distinct from the two genuine
    # saved worlds above and never become a financing/bootstrap receipt.
    if len(depot["saves"]) != 5:
        raise ValueError("depot recovery did not retain its five actual wait states")
    context, power, _ = run_selected(build_reference, "ground-power-recovery", "[transport-tube-power-recovery]", root=root)
    power.update(classification="prepared_component", bootstrap_claim=False)
    destination = create_run_context("acceptance", "normal-ground-recovery-publication", run_id="merge65b-result-" + uuid.uuid4().hex[:10], root=root)
    document = dict(_documentation=__doc__, kind="merged_original_ground_recovery", complete=True,
        build=build_reference, paid_acceptance=paid_acceptance, original_wait_proof=wait_proof,
        genuine_conversion=conversion, genuine_wait_replay=waiting, component_depot=depot, component_power=power,
        source_and_destination_empty_observed=dict(a_stock=original_wait["a_stock"], b_stock=original_wait["b_stock"]),
        accepted_graphics_sha256=CONTENT_SHA, human_acceptance=False)
    target = destination.reports / "recovery.json"
    target.write_text(json.dumps(document, indent=2) + "\n")
    return identity(target, root)
