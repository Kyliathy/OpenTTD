"""Pay original Salt-route conversion without supplying track, work or capital.

@file surface_entry_salt_window_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CIC.1.
Native whole commands own every conversion, paired portal update and expense.
This driver recovers completed invocations rather than repeating paid work.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_retained as observe
from surface_gateway_native import FIELDS, REVISION, PAUSE
from surface_regional_ledger import TABLE


def execute(configuration, receipt, recover_run=None):
    """Execute or recover one genuinely paid native conversion boundary.

    @param configuration Actual independently rebuilt focused application.
    @param receipt Exact admitted original predecessor before any route payment.
    @param recover_run Successful invocation to recover without repeating expenses.
    @return Original invoices, physical powered-route proof and unchanged material owners.
    """
    value = json.loads(receipt.read_text(encoding="utf-8"))
    assert value["complete"] and value["configuration"] == configuration
    original = value["operation"]["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    scenario = "surface-entry-resupply-staged-convert"
    if recover_run is None:
        context = create_run_context("space", scenario)
        context.prepare_runtime()
        retained = context.directory / "harness.py"
        retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_resupply\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run))
        retained = context.directory / "harness.py"
        assert retained.is_file()
    application = ROOT / f"build/{configuration}/openttd_surface_supply_test.exe"
    log = context.logs / "native.log"
    if recover_run is None:
        environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source),
                           OTTD_SURFACE_RESUPPLY_PHASE="staged-convert")
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-resupply]"], cwd=ROOT / "build",
                                stdout=sink, stderr=subprocess.STDOUT, timeout=110, env=environment)
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and "SURFACE_RESUPPLY_COMPLETE phase=staged-convert" in terminal, log
    before, output = [context.saves / f"surface-resupply-{name}-440.sav" for name in ("before-staged-convert", "staged-convert")]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    assert first[b"DATE"] == last[b"DATE"]
    pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        pause = PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    prefixes = dict(peak="PEAK", converted="CONVERTED", invoices="INVOICE")
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_RESUPPLY_" + prefix + " ")]
            for name, prefix in prefixes.items()}
    converted, = rows["converted"]
    peak, = rows["peak"]
    assert peak["funded"] and peak["refund_counted"] == 0
    assert converted["cost"] == 663 and converted["tiles"] == 13 and converted["commands"] == 12 and converted["earned"] == 0
    assert len(rows["invoices"]) == 12 and sum(row["cost"] for row in rows["invoices"]) == 663
    assert converted["powered_route"] and converted["connected_portal"] and converted["wagon_capacity"] == 60
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]
    assert len(processes) == 1
    report = json.loads((ROOT / processes[0]["path"]).read_text(encoding="utf-8"))
    assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    initial, final = observe(before), observe(output)
    assert {key: row for key, row in initial.items() if key != "company"} == {key: row for key, row in final.items() if key != "company"}
    assert initial["company"]["debt"] == final["company"]["debt"] == 5000000
    assert final["company"]["cash"] == initial["company"]["cash"] - converted["cost"] == converted["cash"]
    assert identity(source) == original
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=final, source_pause=pause, retained_harness=identity(retained),
                native_processes=processes, sources=[identity(Path(__file__))] + [identity(ROOT / name) for name in
                ("src/tests/surface_entry_resupply.cpp", "src/rail_cmd.cpp", "src/layer_portal_cmd.cpp", "src/pathfinder/follow_track.hpp",
                 "src/tests/multilayer/acceptance/surface_gateway_component_delivery_ledger.py")])


def main():
    """Publish one paid native conversion without overwriting any original receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                      phase="staged-convert", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
