"""Verify ordinary fuel restore and identical real continuation after freight resumes.

@file surface_entry_propellant_checks-8.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BID.3.
The exact original first-output branch remains selected. An ordinary resave
proves identical continuation without supplying extra product, cash or work.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from surface_propellant_ledger import TABLE, observe, reserve_identity
from artifact_paths import create_run_context
from runner import run_native

RELOAD = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-12.py")))
FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]


def fuel_identity(snapshot):
    """@param snapshot Complete actual native owners. @return Original parked20PROP identity excluding naturally accrued packet age."""
    result = dict(snapshot["propellant_truck"])
    result["packets"] = [{key: value for key, value in packet.items() if key != "periods_in_transit"} for packet in result["packets"]]
    return result


def continuation(configuration, source, suffix):
    """@param configuration Actual focused native application. @param source Immutable original/resaved physical fuel game.
    @param suffix Explicit original or ordinary branch identity. @return Actual3000tick payment/material/native-process proof.
    """
    original = identity(source); context = create_run_context("space", "surface-entry-propellant-continue-" + suffix)
    context.prepare_runtime()
    retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_propellant\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "continue.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-propellant]"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                            OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_PROPELLANT_PHASE="continue"))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_PROPELLANT_COMPLETE phase=continue" in terminal, log
    assert identity(source) == original
    before, output = [context.saves / f"surface-propellant-{name}-440.sav" for name in ("before-continue", "continue")]
    first, loaded, last = [chunks(path)[1] for path in (source, before, output)]
    assert first.keys() == loaded.keys() == last.keys() and all(first[key] == loaded[key] for key in first if key != b"GLOG")
    if first[b"GLOG"] != loaded[b"GLOG"]:
        RELOAD["LOAD_REVISION"](source, before)
    assert loaded[b"GLOG"] == last[b"GLOG"]
    initial, final = observe(before), observe(output)
    assert reserve_identity(initial) == reserve_identity(final) and fuel_identity(initial) == fuel_identity(final)
    assert initial["plant"] == final["plant"] and final["plant"]["chemicals"] == 1
    assert initial["company"]["debt"] == final["company"]["debt"] == 5000000
    assert all(not row["halted"] for row in final["trains"])
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_PROPELLANT_" + prefix + " ")]
            for name, prefix in dict(earned="EARNED", expenses="EXPENSE", delivered="DELIVERED", state="STATE").items()}
    earned, = rows["earned"]; assert earned["earned"] == 3000
    assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == 3000
    assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
    return dict(source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                rows=rows, initial=initial, final=final, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])


def execute():
    """@return Complete ordinary restore and original/resaved native-continuation proof in both configurations."""
    foundation_path = ROOT / "logs/surface-entry-propellant-custody-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["useful_propellant_collected"] == 20
    processes, physical, summaries, build_rows = set(), [], [], []
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; source = ROOT / entry["output"]["path"]; assert identity(source) == entry["output"]
        restored, restore = RELOAD["ORDINARY"](configuration, source, "propellant-" + configuration.lower())
        RELOAD["REVISION"](source, restored); pause = RELOAD["restore_pause"](source, restored)
        first, last = [chunks(path)[1] for path in (source, restored)]
        assert first.keys() == last.keys() and all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
        assert observe(source) == observe(restored)
        restore_path = ROOT / f"logs/surface-entry-propellant-restored-{configuration}.json"
        RELOAD["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, operation=restore, original_pause=pause))
        branches, receipts, rows = [], [], []
        # Continue the selected actual game and its ordinary resave for the
        # same original3000ticks. The second branch is a proof, never another
        # source of material/capital for the selected first-output lineage.
        for candidate, suffix in ((source, "original"), (restored, "ordinary")):
            operation = continuation(configuration, candidate, suffix)
            path = ROOT / f"logs/surface-entry-propellant-continued-{configuration}-{suffix}.json"
            RELOAD["publish"](path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="continue", operation=operation))
            branches.append(chunks(ROOT / operation["output"]["path"])[1]); receipts.append(identity(path)); rows.append(operation["rows"])
            processes.update(reference["path"] for reference in operation["native_processes"])
        assert branches[0].keys() == branches[1].keys() and all(branches[0][key] == branches[1][key] for key in branches[0] if key != b"GLOG")
        assert rows[0] == rows[1]
        normalized = dict(last); normalized[b"DATE"] = first[b"DATE"]
        physical.append([normalized] + branches); build_rows.append(rows)
        processes.update(reference["path"] for reference in restore["native_processes"])
        selected = json.loads((ROOT / receipts[0]["path"]).read_text(encoding="utf-8"))["operation"]
        summaries.append(dict(configuration=configuration, restore=identity(restore_path), continuations=receipts,
                              output=selected["output"], company=selected["final"]["company"], plant=selected["final"]["plant"],
                              fuel=selected["final"]["propellant_truck"], original_pause=pause))
        print(configuration, "ordinary20PROPrestore and identical3000tick continuation passed", flush=True)
    assert build_rows[0] == build_rows[1]
    comparisons = 0
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += 120
    assert len(processes) == 6 and comparisons == 360
    return dict(_documentation=__doc__, complete=True, foundation=identity(foundation_path), configurations=summaries,
                native_processes=6, exact_cross_build_gameplay_comparisons=360, exact_native_replay_gameplay_comparisons=240,
                useful_propellant_collected=20, external_chemicals_consumed=20, residual_chemicals=1,
                actual_launch_complete=False, sources=[identity(Path(__file__)), identity(Path(__file__).with_name("surface_propellant_ledger.py")),
                                                       identity(ROOT / "src/tests/surface_entry_propellant.cpp"), identity(ROOT / "src/tests/surface_entry_work.h")])


def main():
    """Retain useful actual fuel with ordinary persistence before the genuine paid launch producer."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve()
    RELOAD["publish"](output, execute())
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
