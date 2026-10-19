"""Verify actual retained-cutter gallery work and cumulative finite spoil custody.

@file surface_entry_mine_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BF.2.
Both original geological jobs remain historical owners. Original packets,
receiver stocks and processed totals account for their combined physical yield.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

ACCESS = runpy.run_path(str(Path(__file__).with_name("surface_entry_access_checks-3.py")))
OWNERS = ACCESS["OWNERS"]


def observe(path):
    """Conserve both genuine jobs' actual Earth/Rock across the physical train/receiver owners.

    @param path Actual complete native checkpoint from the genuine predecessor.
    @return Exact current/historical jobs, physical spoil, receivers and earned floor.
    """
    table = OWNERS["saved_table"]
    owners = ACCESS["machinery"](path)
    jobs = owners["jobs"]
    receivers = table(path, b"SPRR")
    industries = table(path, b"INDY")
    spoil = {}
    # Retiring a controller must not erase its committed historical quantity.
    # Each material balances both original jobs against real train packets,
    # the receiver industry's own stock and the sole processed ledger.
    for receiver in receivers.values():
        industry = industries[receiver["industry"]]
        cargo = industry["accepted[0].cargo"]
        waiting = industry["accepted[0].waiting"]
        aboard = sum(packet["count"] for part in owners["trains"].values() if part["cargo_type"] == cargo for packet in part["packets"])
        name = "earth" if receiver["kind"] == 0 else "rock"
        dug = sum(job[name] for job in jobs.values())
        processed = receiver["disposed"] + receiver["crushed"]
        assert dug == waiting + aboard + processed, (name, dug, waiting, aboard, processed)
        assert receiver["fees"] == (processed + waiting) * (2 if name == "earth" else 3)
        spoil[name] = dict(dug=dug, aboard=aboard, waiting=waiting, processed=processed, receiver=receiver)
    _, states = ACCESS["terrain"](path)
    floor = sorted(tile for tile, state in states.items() if state == 2)
    project = next(iter(owners["projects"].values()), None)
    ready = project is not None and project["id"] == 1 and project["phase"] == 7 and jobs[1]["cursor"] == jobs[1]["routes"]
    ready = ready and all(not row["packets"] for row in owners["trains"].values()) and len(floor) == 27
    return dict(owners=owners, spoil=spoil, floor=floor, ready=ready, salt_remaining=table(path, b"GDEP")[4]["remaining"])


def check_reserves(source, output, newly_open):
    """Verify original finite reserve losses for the exact newly opened seam cells.

    @param source Actual preceding checkpoint.
    @param output Actual next native checkpoint.
    @param newly_open Exact original world cells that gained earned floor.
    @return Independently calculated native mineral loss per original deposit.
    """
    table = OWNERS["saved_table"]
    before, after = [table(path, b"GDEP") for path in (source, output)]
    spans = table(output, b"GSPN").values()
    geometry = table(output, b"MAPS")[0]
    width, plane = geometry["dim_x"], geometry["dim_x"] * geometry["dim_y"]
    losses = {}
    # Native excavation destroys only each newly opened cell's original seam
    # quantum. Overlap, empty travel and earlier floor cannot be charged twice.
    for key, row in before.items():
        cells = {row["layer"] * plane + span["y"] * width + x for span in spans if span["deposit"] == row["id"]
                 for x in range(span["first_x"], span["last_x"] + 1)}
        loss = min(row["remaining"], row["initial"] // len(cells) * len(cells & newly_open))
        assert after[key] == dict(row, remaining=row["remaining"] - loss), key
        losses[key] = loss
    return losses


def execute(configuration, source, phase, require_complete):
    """Run one actual gallery command or exact3000tick native work interval.

    @param configuration Independently linked focused original application.
    @param source Exact retired, commissioned, started or working predecessor.
    @param phase Normal commission/start command or native game-tick drive.
    @param require_complete Require actual completed work and finite receiver settlement.
    @return Original process membership, native ledgers and physical next checkpoint.
    """
    context = create_run_context("space", "surface-entry-mine-" + phase)
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    command = "surface_entry_production" if phase == "drive" else "surface_entry_mine"
    (context.runtime_dir / "scripts/game_start.scr").write_text(command + "\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / (phase + ".log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-production]" if phase == "drive" else "[.surface-entry-mine]"],
                            cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_MINE_PHASE=phase))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal, log
    assert identity(source) == original
    output = context.saves / ("surface-entry-replayed-440.sav" if phase == "drive" else f"surface-mine-{phase}-440.sav")
    first, final = [OWNERS["observe"](path) for path in (source, output)]
    assert first["debt"] == final["debt"] == 3500000
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"]
    assert first["parked_steel"] == final["parked_steel"] == 36 and first["parked_goods"] == final["parked_goods"] == 14
    initial, last = [observe(path) for path in (source, output)]
    assert initial["owners"]["trains"].keys() == last["owners"]["trains"].keys() and len(last["owners"]["trains"]) == 11
    assert initial["owners"]["jobs"][0] == last["owners"]["jobs"][0]
    newly_open = set(last["floor"]) - set(initial["floor"])
    assert set(initial["floor"]).issubset(last["floor"])
    losses = check_reserves(source, output, newly_open)
    before, after = chunks(source)[1], chunks(output)[1]
    for family in (b"GSPN", b"GVNT"):
        assert before[family] == after[family], family
    dates = [OWNERS["saved_table"](path, b"DATE")[0] for path in (source, output)]
    earned = 3000 if phase == "drive" else 0
    assert dates[1]["tick_counter"] - dates[0]["tick_counter"] == earned and dates[1]["id"] == dates[0]["id"]
    invoices = [int(value) for value in re.findall(r"SURFACE_MINE_INVOICE command=\w+ cost=(-?\d+)", terminal)]
    if phase != "drive":
        assert not newly_open and first["cash"] - final["cash"] == sum(invoices)
        # Normal conversion purchases permanent track on existing earned
        # floor. Only actual invoice settlement may change cash/expenses;
        # stopped products, prior production and every bank field stay exact.
        for name, value in first.items():
            if name not in ("cash", "expense"):
                assert final[name] == value, name
    if phase == "commission":
        assert "new_parts=0 price=0" in terminal and last["owners"]["jobs"][1]["cursor"] == 0
        assert last["owners"]["trains"] == initial["owners"]["trains"]
    if require_complete:
        assert last["ready"] and last["salt_remaining"] == 24000, last
        assert last["owners"]["jobs"][1]["earth"] == 36 and last["owners"]["jobs"][1]["rock"] == 72
        assert last["owners"]["jobs"][1]["cost"] == 360
    return dict(source=original, output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, work=last, earned_ticks=earned, invoices=invoices,
                finite_reserve_losses=losses, supplied_stock_floor_or_progress=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests" / ("surface_entry_production.cpp" if phase == "drive" else "surface_entry_mine.cpp")),
                         identity(Path(__file__).with_name("surface_entry_access_checks-3.py"))])


def main():
    """Publish one bounded actual mine-gallery operation and its conserved original owners."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("commission", "start", "drive"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--require-complete", action="store_true")
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, source, args.phase, args.require_complete)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), ready=result["work"]["ready"], floor=len(result["work"]["floor"]), salt=result["work"]["salt_remaining"])), flush=True)


if __name__ == "__main__":
    main()
