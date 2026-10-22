"""Start real paid trains or earn native cutting, spoil haulage and actual receiver settlement.

@file surface_entry_access_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.3.
Every interval earns exactly3000native game ticks. Original packet, finite rock,
industry stock and receiver ledgers independently account for every dug tonne.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

MACHINERY = runpy.run_path(str(Path(__file__).with_name("surface_entry_access_checks-3.py")))
OWNERS = MACHINERY["OWNERS"]


def observe(path):
    """Reconcile actual finite work, native physical spoil and original receivers.

    @param path Actual complete native world from the genuine paid predecessor.
    @return Conserved native work, spoil, floor, project and receiver observations.
    """
    table = OWNERS["saved_table"]
    owners = MACHINERY["machinery"](path)
    project, = owners["projects"].values()
    job, = owners["jobs"].values()
    assignment, = owners["assignments"].values()
    receivers = table(path, b"SPRR")
    industries = table(path, b"INDY")
    held = {}
    # Original wagon packets retain their own source and whole quantities.
    # Each spoil kind balances physical train stock, actual industry stock and
    # the sole receiver's historical processed quantity against committed cuts.
    for receiver in receivers.values():
        industry = industries[receiver["industry"]]
        cargo = industry["accepted[0].cargo"]
        waiting = industry["accepted[0].waiting"]
        aboard = sum(packet["count"] for part in owners["trains"].values() if part["cargo_type"] == cargo for packet in part["packets"])
        name = "earth" if receiver["kind"] == 0 else "rock"
        processed = receiver["disposed"] + receiver["crushed"]
        assert job[name] == waiting + aboard + processed, (name, job[name], waiting, aboard, processed)
        assert receiver["fees"] == (processed + waiting) * (2 if name == "earth" else 3)
        held[name] = dict(cargo=cargo, aboard=aboard, waiting=waiting, processed=processed, receiver=receiver)
    _, states = MACHINERY["terrain"](path)
    floor = sorted(tile for tile, state in states.items() if state == 2)
    return dict(owners=owners, project=project, job=job, assignment=assignment, spoil=held, floor=floor,
                ready=project["phase"] == 7 and job["cursor"] == job["routes"] and assignment["exchanges"] > 0
                and all(not row["packets"] for row in owners["trains"].values()) and len(floor) == 15)


def execute(configuration, source, phase, require_complete):
    """Perform one bounded real native operation without movement or inventory shortcuts.

    @param configuration Independently linked focused original application.
    @param source Exact immediate physical checkpoint.
    @param phase Original train start commands or exactly3000native game ticks.
    @param require_complete Require the actual finished project and empty physical trains.
    @return Exact original native process, finite ledgers, products, clocks and next checkpoint.
    """
    context = create_run_context("space", "surface-entry-access-" + phase)
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    command = "surface_entry_access" if phase == "start" else "surface_entry_production"
    (context.runtime_dir / "scripts/game_start.scr").write_text(command + "\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / (phase + ".log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-access]" if phase == "start" else "[.surface-entry-production]"],
                            cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                     OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_ACCESS_PHASE=phase))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal, log
    assert identity(source) == original
    output = context.saves / ("surface-access-start-440.sav" if phase == "start" else "surface-entry-replayed-440.sav")
    first, final = [OWNERS["observe"](path) for path in (source, output)]
    assert first["debt"] == final["debt"] == 3500000
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"]
    assert first["parked_steel"] == final["parked_steel"] == 36 and first["parked_goods"] == final["parked_goods"] == 14
    initial, last = [observe(path) for path in (source, output)]
    dates = [OWNERS["saved_table"](path, b"DATE")[0] for path in (source, output)]
    earned = 0 if phase == "start" else 3000
    assert dates[1]["tick_counter"] - dates[0]["tick_counter"] == earned and dates[1]["id"] == dates[0]["id"]
    before, after = chunks(source)[1], chunks(output)[1]
    for family in (b"GDEP", b"GSPN", b"GVNT"):
        assert before[family] == after[family], family
    if phase == "start":
        assert "SURFACE_ACCESS_STARTED" in terminal and first == final
        assert initial["job"] == last["job"] and before[b"GEOS"] == after[b"GEOS"]
    if require_complete:
        assert last["ready"], last
        assert last["job"]["earth"] == 45 and last["job"]["rock"] == 90
    return dict(source=original, output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, work=last, earned_ticks=earned, supplied_stock_floor_or_progress=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests" / ("surface_entry_access.cpp" if phase == "start" else "surface_entry_production.cpp")),
                         identity(Path(__file__).with_name("surface_entry_access_checks-3.py"))])


def main():
    """Retain one bounded original command or actual work interval and its complete finite ledgers."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("start", "drive"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--require-complete", action="store_true")
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, source, args.phase, args.require_complete)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), ready=result["work"]["ready"], job=result["work"]["job"],
                          phase=result["work"]["project"]["phase"], wait=result["work"]["assignment"]["wait"])), flush=True)


if __name__ == "__main__":
    main()
