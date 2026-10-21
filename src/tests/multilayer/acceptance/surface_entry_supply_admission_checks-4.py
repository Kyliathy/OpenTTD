"""Query post-cleanup finite-Salt alternatives from the exact original worlds.

@file surface_entry_supply_admission_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CH.2.
Native track followers and whole command quotes retain route and price
authority. These pure observations grant no purchase, coupling or fuel.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe
from surface_gateway_native import FIELDS, PAUSE
from surface_regional_ledger import TABLE

DIRECTORY = Path(__file__).resolve().parent


def rebuilt_revision(source, output, application):
    """Admit only one load-revision row matching this rebuilt executable.

    @param source Exact selected original save before native loading.
    @param output Native paused save with one new compiled revision record.
    @param application Hash-qualified executable recorded by the actual process.
    @return Exact generated revision authority and unchanged historical row count.
    """
    before, after = [TABLE(path, b"GLOG") for path in (source, output)]
    assert set(after) == set(before) | {len(before)}
    assert all(after[key] == row for key, row in before.items())
    appended = after[len(before)]
    assert appended["at"] == 1 and appended["action.count"] == 1 and appended["action[0].ct"] == 1
    assert appended["tick"] == TABLE(source, b"DATE")[0]["tick_counter"]
    revision = ROOT / "build/generated/rev.cpp"
    definition = revision.read_text(encoding="utf-8")
    compiled_hash, = re.findall(r'_openttd_revision_hash = "([0-9a-f]{40})"', definition)
    modified, = re.findall(r'_openttd_revision_modified = ([012]);', definition)
    expected_text = "gum"[int(modified)] + compiled_hash
    assert compiled_hash.encode() in application.read_bytes()
    assert bytes.fromhex(appended["action[0].revision[0].revision.text"]).decode() == expected_text
    assert appended["action[0].revision[0].revision.modified"] == int(modified)
    # A rebuilt application may append its exact new revision text, but its
    # action shape, format and content version must match the last native
    # revision. This refuses settings, cheats and extra history under GLOG.
    previous = [row for row in before.values() if row.get("action[0].revision.count") == 1][-1]
    exceptions = {"tick", "action[0].revision[0].revision.text", "action[0].revision[0].revision.modified"}
    assert {key: value for key, value in appended.items() if key not in exceptions} == {
        key: value for key, value in previous.items() if key not in exceptions}
    return dict(source=identity(revision), revision=expected_text, original_history_rows=len(before))


def execute(configuration, boundary, receipt, recover_run=None):
    """Run one pure native alternative query and compare every before/output family.

    @param configuration Independently linked RelWithDebInfo or Debug executable.
    @param boundary Current module quotes or complete finite-refill admission.
    @param receipt Exact original module-query predecessor for this configuration.
    @param recover_run Successful invocation to inspect without repeating native work.
    @return Hash-qualified native process, routes, quotes and unchanged original owners.
    """
    value = json.loads(receipt.read_text(encoding="utf-8"))
    assert value["complete"] and value["configuration"] == configuration
    original = value["operation"]["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    phase = "expansion-query" if boundary == "modules" else "current-admission"
    family = "gateway" if boundary == "modules" else "resupply"
    scenario = "surface-entry-gateway-expansion-query" if boundary == "modules" else "surface-entry-resupply-current-admission"
    if recover_run is None:
        context = create_run_context("space", scenario)
        context.prepare_runtime()
        retained = context.directory / "harness.py"
        retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text(f"surface_entry_{family}\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run))
        retained = context.directory / "harness.py"
        assert retained.is_file()
    application = ROOT / f"build/{configuration}/openttd_surface_supply_test.exe"
    log = context.logs / "native.log"
    if recover_run is None:
        environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                           OTTD_SURFACE_ENTRY_SOURCE=str(source))
        environment["OTTD_SURFACE_" + ("GATEWAY" if boundary == "modules" else "RESUPPLY") + "_PHASE"] = phase
        with log.open("wb") as sink:
            result = run_native(context, [str(application), f"[.surface-entry-{family}]"], cwd=ROOT / "build",
                                stdout=sink, stderr=subprocess.STDOUT, timeout=110, env=environment)
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    marker = "SURFACE_GATEWAY_COMPLETE " if boundary == "modules" else "SURFACE_RESUPPLY_COMPLETE phase=current-admission"
    assert "All tests passed" in terminal and marker in terminal, log
    before, output = [context.saves / f"surface-{family}-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert first == last
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = PAUSE(before, source)
    revision = None
    if saved[b"GLOG"] != first[b"GLOG"]:
        revision = rebuilt_revision(source, before, application)
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]
    assert len(processes) == 1
    report = json.loads((ROOT / processes[0]["path"]).read_text(encoding="utf-8"))
    assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    prefixes = dict(admission="ADMISSION", fuel_carriers="FUEL_CARRIER", rail="RAIL", crossings="CROSSING",
                    custody="CUSTODY", reuse="REUSE", heads="HEAD", alternative="ALTERNATIVE", routes="ROUTE", window="WINDOW",
                    conversion="CONVERSION", electrification="ELECTRIFICATION")
    if boundary == "modules":
        prefixes = dict(layout="EXPANSION_LAYOUT", inputs="EXPANSION_INPUT", fuel="EXPANSION_FUEL", budget="EXPANSION_BUDGET", flight="EXPANSION_FLIGHT", invoices="INVOICE")
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith(f"SURFACE_{family.upper()}_" + prefix + " ")]
            for name, prefix in prefixes.items()}
    if boundary == "modules":
        assert [(row["needed"], row["held"], row["missing"], row["load_admitted"]) for row in rows["inputs"]] == [(16, 32, 0, True), (28, 50, 0, True)]
        fuel, = rows["fuel"]
        budget, = rows["budget"]
        assert fuel["needed"] == 30 and fuel["held"] == 12 and fuel["missing"] == 18
        assert budget["base"] == 510000 and not budget["funded"] and not budget["bank_admitted"]
    else:
        assert len(rows["reuse"]) == 3 and len(rows["alternative"]) == 1
        assert rows["heads"]
        electrification, = rows["electrification"]
        assert len(rows["window"]) == 1 + int(electrification["full_bill"])
        assert len(rows["routes"]) == 4 * len(rows["window"])
        # Dedicated and proposed converted-route budgets remain separate:
        # each counts every whole purchase, conversion and bounded allowance
        # once, without making either unpurchased option into physical supply.
        for window in rows["window"]:
            assert window["target"] == sum(window[key] for key in ("head_cost", "wagon_cost", "replacement_cost", "handling", "work", "reserve", "infrastructure"))
            assert window["fuel"] >= 18 and window["chemical_feed"] == window["oil_needed"] == window["fuel"]
            assert window["bounded_bill_complete"] and not window["fuel_procured"]
    current = dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output),
                   application=identity(application), terminal=identity(log), rows=rows,
                   initial=observe(before), final=observe(output), source_pause=source_pause, source_revision=revision,
                   retained_harness=identity(retained), native_processes=processes,
                   sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_resupply.cpp"),
                            identity(ROOT / "src/pathfinder/follow_track.hpp"), identity(ROOT / "src/underground_vehicle.cpp"),
                            identity(ROOT / "src/train_cmd.cpp"), identity(ROOT / "src/tests/surface_entry_gateway.cpp"),
                            identity(ROOT / "build/generated/rev.cpp"), identity(ROOT / "src/gamelog.cpp"),
                            identity(ROOT / "src/rail_cmd.cpp"), identity(ROOT / "src/layer_portal_cmd.cpp")])
    assert current["initial"] == current["final"] and identity(source) == original
    return current


def main():
    """Publish one fresh immutable pure module or alternative-refill boundary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--boundary", choices=("modules", "refill"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    current = execute(args.configuration, args.boundary, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                      phase=args.boundary, operation=current), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
