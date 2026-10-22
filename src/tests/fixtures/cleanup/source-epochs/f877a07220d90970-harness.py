"""Verify real cutter commissioning and physical empty paid haulage in the authentic game.

@file surface_entry_access_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.2.
Native commands alone own capital, project claims, actual vehicles and coupling.
The original finite geology and useful Surface stock cannot become fixture grants.
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

OWNERS = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))
PREFIX = OWNERS["PREFIX"]


def machinery(path):
    """Read actual project/job, assignment, parts and their original packet custody.

    @param path Complete native file from the authentic predecessor chain.
    @return Original saved native owner rows, without inferred fixture identities.
    """
    table = OWNERS["saved_table"]
    projects = table(path, b"TBMP")
    jobs = table(path, b"XJOB")
    assignments = table(path, b"TBXA")
    vehicles = table(path, b"VEHS")
    packets = table(path, b"CAPA")
    trains = {}
    # Every actual train part retains its native engine, links and packets.
    # Saved one-based references resolve to the sole physical packet table;
    # no aggregate total may conceal cargo duplicated between paid vehicles.
    claimed = set()
    for vehicle, fields in vehicles.items():
        if fields["type"] != 0:
            continue
        cargo = []
        for reference in fields[PREFIX + "cargo.packets"]:
            packet = reference - 1
            assert packet not in claimed and packet in packets
            claimed.add(packet)
            cargo.append(dict(packet=packet, **packets[packet]))
        assert sum(row["count"] for row in cargo) == sum(fields[PREFIX + "cargo.action_counts"])
        trains[vehicle] = dict(engine=fields[PREFIX + "engine_type"], tile=fields[PREFIX + "tile"],
                               status=fields[PREFIX + "vehstatus"], cargo_type=fields[PREFIX + "cargo_type"],
                               capacity=fields[PREFIX + "cargo_cap"], packets=cargo)
    return dict(projects=projects, jobs=jobs, assignments=assignments, trains=trains,
                sets=table(path, b"TBXS"), wagons=table(path, b"TBXW"))


def execute(configuration, source):
    """Buy actual commissioned machinery and a native depot haul train exactly once.

    @param configuration Independently linked focused application.
    @param source Exact ready receiver checkpoint, never an already purchased output.
    @return Original process membership, native invoices, physical empty parts and uncut work.
    """
    context = create_run_context("space", "surface-entry-access-machinery")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_access\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "machinery.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-access]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                     OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_ACCESS_PHASE="machinery"))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_ACCESS_MACHINERY_COMPLETE" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-access-before-machinery-440.sav"
    output = context.saves / "surface-access-machinery-440.sav"
    first, final = [OWNERS["observe"](path) for path in (before, output)]
    # Paused native purchases change only the real expense/cash owner. Old
    # products, their provenance and all earned Surface production remain exact.
    for name, value in first.items():
        if name not in ("cash", "expense"):
            assert final[name] == value, name
    invoices = [int(value) for value in re.findall(r"SURFACE_ACCESS_INVOICE command=\w+ tile=\d+ cost=(-?\d+)", terminal)]
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"] == sum(invoices)
    assert final["parked_steel"] == 36 and final["parked_goods"] == 14 and final["debt"] == 3500000
    owners = machinery(output)
    assert len(owners["projects"]) == len(owners["jobs"]) == len(owners["assignments"]) == 1
    project, = owners["projects"].values()
    job, = owners["jobs"].values()
    assignment, = owners["assignments"].values()
    assert project["job"] == job["id"] and project["cutter"] == job["cutter"]
    assert project["portal"] == assignment["bay"] == 1949 and project["origin"] == 4
    assert assignment["project"] == project["id"] and assignment["exchanges"] == 0
    assert job["cursor"] == job["partial"] == job["earth"] == job["rock"] == job["cost"] == 0
    assert len(owners["trains"]) == 11 and all(not row["packets"] for row in owners["trains"].values())
    assert all(row["tile"] in (1949, 1373) for row in owners["trains"].values())
    start, end = chunks(before)[1], chunks(output)[1]
    for family in (b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"INDY"):
        assert start[family] == end[family], family
    a, b = [OWNERS["saved_table"](path, b"DATE")[0] for path in (before, output)]
    for name, value in a.items():
        if not name.startswith("random_state["):
            assert b[name] == value, name
    return dict(source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, invoices=invoices, owners=owners, simulation_ticks=0, supplied_stock_or_floor=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_access.cpp")])


def main():
    """Select one actual ready receiver save and retain its bounded machinery receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, source)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), invoice=sum(result["invoices"]))), flush=True)


if __name__ == "__main__":
    main()
