"""Characterize genuine finite Salt and actual native access/disposal quotes without mutation.

@file surface_entry_salt_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.1.
Public/free geology never grants floor. Exact whole native before/after owners
protect genuine Steel/Goods, bank authority, native terrain and world identity.
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

PRODUCTS = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))


def execute(configuration, source):
    """Run one actual full-loader pure Salt/access characterization against retained products.

    @param configuration Independently linked focused application.
    @param source Exact ordinary paused productive checkpoint, never a fixture or replay branch.
    @return Original source, unchanged native owners and actual current placement/capital terms.
    """
    context = create_run_context("space", "surface-entry-salt")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_salt\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    initial_products = PRODUCTS["observe"](source)
    assert initial_products["ready"] and initial_products["cash"] == 699555
    log = context.logs / "salt.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-salt]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_SALT_QUERIES_COMPLETE" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-entry-before-salt-440.sav"
    output = context.saves / "surface-entry-after-salt-440.sav"
    assert chunks(before) == chunks(output)
    assert PRODUCTS["observe"](before) == PRODUCTS["observe"](output) == initial_products
    deposits = [dict(id=int(deposit), layer=int(layer), initial=int(initial), remaining=int(remaining), surveyed=int(surveyed), spans=int(spans))
                for deposit, layer, initial, remaining, surveyed, spans in re.findall(
                    r"SURFACE_SALT_DEPOSIT id=(\d+) layer=(\d+) initial=(\d+) remaining=(\d+) surveyed=(\d+) spans=(\d+)", terminal)]
    assert deposits and all(row["initial"] == row["remaining"] > 0 for row in deposits)
    portals = [dict(deposit=int(deposit), anchor=int(anchor), target=int(target), direction=int(direction), cost=int(cost), solid=True)
               for deposit, anchor, target, direction, cost in re.findall(
                   r"SURFACE_SALT_PORTAL deposit=(\d+) anchor=(\d+) target=(\d+) direction=(\d+) cost=(\d+) solid=true", terminal)]
    assert len(portals) == 4
    receivers = [dict(kind=int(kind), type=int(kind_id), anchor=int(anchor), cost=int(cost), batch=int(batch), capacity=int(capacity),
                      intake=int(intake), fee=int(fee), surplus_fee=int(surplus), interval=int(interval))
                 for kind, kind_id, anchor, cost, batch, capacity, intake, fee, surplus, interval in re.findall(
                     r"SURFACE_SALT_RECEIVER kind=(\d+) type=(\d+) anchor=(\d+) cost=(\d+) batch=(\d+) capacity=(\d+) intake=(\d+) fee=(\d+) surplus_fee=(\d+) interval=(\d+)", terminal)]
    assert len(receivers) == 2 and [row["kind"] for row in receivers] == [0, 1]
    machinery, = re.findall(r"SURFACE_SALT_MACHINERY ([^\r\n]+)", terminal)
    haulage = [dict(model=int(model), engine=int(engine), cost=int(cost), capacity=int(capacity))
               for model, engine, cost, capacity in re.findall(
                   r"SURFACE_SALT_HAULAGE model=(\d+) engine=(\d+) cost=(\d+) capacity=(\d+)", terminal)]
    assert [row["model"] for row in haulage] == [4, 5]
    anchor, direction, cost = re.findall(r"SURFACE_SALT_SERVICE anchor=(\d+) direction=(\d+) cost=(\d+) rock_refused=true", terminal)[0]
    service = dict(anchor=int(anchor), direction=int(direction), cost=int(cost), rock_refused=True)
    return dict(source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                deposits=deposits, portals=portals, receivers=receivers, published_machinery_terms=machinery,
                haulage=haulage, service=service,
                unchanged_native_families=len(chunks(before)[1]), original_products_preserved=True, supplied_floor_or_capital=False,
                retained_harness=identity(retained), native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_salt.cpp")])


def main():
    """Select one exact retained product world and retain fresh pure query evidence."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     operation=execute(args.configuration, source)), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
