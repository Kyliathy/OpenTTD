"""Run one selected native farm-return query, order boundary or funded work interval.

@file surface_farm_two_return_native.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVBC.
Native owners retain packets, rolling stock, clocks, financial books and work.
Callers provide their explicit current observer and typed diagnostic families.
"""
import json
import os
from pathlib import Path
import subprocess

from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_gateway_native import FIELDS, REVISION, PAUSE
from surface_regional_ledger import TABLE


def execute(configuration, phase, receipt, observer, prefixes, caller_files, recover_run=None, work_limit=500):
    """Run or recover one exact native query, order boundary or bounded farm return.

    @param configuration Independently linked focused application.
    @param phase Explicit native resupply console phase; native code validates its allowlist.
    @param receipt Exact immediately preceding original native receipt.
    @param observer Explicit caller-owned current physical/material contract.
    @param prefixes Caller field names mapped to native diagnostic suffixes.
    @param caller_files Exact relevant current producer/observer source paths.
    @param recover_run Successful invocation to inspect without repeating payment or work.
    @param work_limit Newly priced ceiling up to500normal ticks; never an earned-time grant.
    @return Original process/save identities, native rows and observed original owners.
    """
    assert phase in ("farm-two-return-query", "farm-two-return-work")
    assert 0 < work_limit <= 500
    value = json.loads(receipt.read_text(encoding="utf-8"))
    assert value["complete"] and value["configuration"] == configuration
    original = value["operation"]["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    scenario = "surface-entry-resupply-" + phase
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
    application = ROOT / f"build/{configuration}/openttd_surface_capital_test.exe"
    log = context.logs / "native.log"
    if recover_run is None:
        environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source),
                           OTTD_SURFACE_RESUPPLY_PHASE=phase, OTTD_SURFACE_CHEMICAL_WORK_LIMIT=str(work_limit))
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-farm-return]"], cwd=ROOT / "build",
                                stdout=sink, stderr=subprocess.STDOUT, timeout=110, env=environment)
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and "SURFACE_RESUPPLY_COMPLETE phase=" + phase in terminal, log
    before, output = [context.saves / f"surface-resupply-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        pause = PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]
    assert len(processes) == 1
    report = json.loads((ROOT / processes[0]["path"]).read_text(encoding="utf-8"))
    assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    assert identity(source) == original
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_RESUPPLY_" + suffix + " ")]
            for name, suffix in prefixes.items()}
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=observer(before), final=observer(output), source_pause=pause,
                retained_harness=identity(retained), native_processes=processes,
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_productive_admission.cpp"), identity(ROOT / "src/tests/surface_entry_resupply.cpp")] + [identity(path) for path in caller_files])
