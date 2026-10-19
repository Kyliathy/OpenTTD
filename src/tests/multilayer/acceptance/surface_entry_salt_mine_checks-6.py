"""Verify actual empty Salt train purchase and finite native haulage to Surface custody.

@file surface_entry_salt_mine_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BG.3.
Original mine/packet/vehicle owners must prove actual loaded depot stock. No
vehicle placement, supplied cargo or direct production call earns transport.
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

MINE = runpy.run_path(str(Path(__file__).with_name("surface_entry_salt_mine_checks-4.py")))
OWNERS = MINE["OWNERS"]
PREFIX = "train[0].common[0]."


def observe(path):
    """Read the actual paid Salt train and original source14packets at its real saved pose.

    @param path Complete original native save after actual train purchase.
    @return Conserved finite mine output and actual physical Salt custody.
    """
    native = MINE["observe"](path)
    vehicles = OWNERS["saved_table"](path, b"VEHS")
    packets = OWNERS["saved_table"](path, b"CAPA")
    trains = {key: row for key, row in vehicles.items() if row["type"] == 0}
    head, = [key for key, row in trains.items() if bytes.fromhex(row[PREFIX + "name"]).decode("utf-8") == "Surface Salt reserve"]
    salt_parts = []
    claimed = set()
    # Native packet references resolve to the sole CAPA table. Quantity,
    # action counts and original Industry14provenance agree for each actual
    # wagon; neither a copied total nor two references may invent Surface stock.
    for key, row in trains.items():
        if row[PREFIX + "cargo_type"] != native["cargo"]:
            continue
        held = []
        for reference in row[PREFIX + "cargo.packets"]:
            packet = reference - 1
            assert packet not in claimed and packet in packets
            claimed.add(packet)
            value = packets[packet]
            assert value["source_type"] == 0 and value["source_id"] == 14
            held.append(dict(packet=packet, **value))
        quantity = sum(value["count"] for value in held)
        assert quantity == sum(row[PREFIX + "cargo.action_counts"])
        salt_parts.append(dict(id=key, engine=row[PREFIX + "engine_type"], tile=row[PREFIX + "tile"],
                               status=row[PREFIX + "vehstatus"], track=row["train[0].track"], capacity=row[PREFIX + "cargo_cap"],
                               held=quantity, packets=held))
    wagon, = salt_parts
    assert wagon["engine"] == 261 and wagon["capacity"] == 60
    front = trains[head]
    surface = wagon["held"] == 60 and wagon["tile"] == front[PREFIX + "tile"] == 1373
    surface = surface and bool(front[PREFIX + "vehstatus"] & 2) and wagon["track"] == front["train[0].track"] == 128
    return dict(mine=native, head=head, wagon=wagon, surface_custody=surface)


def execute(configuration, source, phase, require_surface):
    """Buy real empty carriers or earn exact3000native loading/travel ticks.

    @param configuration Independently linked focused native application.
    @param source Exact completed mine or preceding actual fleet/work checkpoint.
    @param phase Original empty purchase/orders or real game-tick drive.
    @param require_surface Require actual60Salt in the halted Surface depot train.
    @return Actual process membership, finite reserve and original physical packets.
    """
    context = create_run_context("space", "surface-entry-salt-haul-" + phase)
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    command = "surface_entry_salt_mine" if phase == "fleet" else "surface_entry_production"
    (context.runtime_dir / "scripts/game_start.scr").write_text(command + "\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / (phase + ".log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-salt-mine]" if phase == "fleet" else "[.surface-entry-production]"],
                            cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_SALT_MINE_PHASE=phase))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal, log
    assert identity(source) == original
    output = context.saves / ("surface-salt-mine-fleet-440.sav" if phase == "fleet" else "surface-entry-replayed-440.sav")
    first, final = [OWNERS["observe"](path) for path in (source, output)]
    assert first["debt"] == final["debt"] == 4250000
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"]
    assert first["parked_steel"] == final["parked_steel"] == 36 and first["parked_goods"] == final["parked_goods"] == 14
    native = observe(output)
    before, after = chunks(source)[1], chunks(output)[1]
    for family in (b"GEOS", b"GSPN", b"GVNT", b"XJOB", b"XJRT", b"XJTL"):
        assert before[family] == after[family], family
    dates = [OWNERS["saved_table"](path, b"DATE")[0] for path in (source, output)]
    earned = 0 if phase == "fleet" else 3000
    assert dates[1]["tick_counter"] - dates[0]["tick_counter"] == earned and dates[1]["id"] == dates[0]["id"]
    if phase == "fleet":
        assert "capacity=60 length=16 invoice=164559 empty=true" in terminal
        assert first["cash"] - final["cash"] == 164559 and native["wagon"]["held"] == 0
        for family in (b"LMNR", b"GDEP", b"CAPA", b"INDY", b"STNN"):
            assert before[family] == after[family], family
        old, new = [OWNERS["saved_table"](path, b"VEHS") for path in (source, output)]
        assert len(new) == len(old) + 2
        for key, value in old.items():
            assert new[key] == value, key
    if require_surface:
        assert native["surface_custody"], native
    return dict(source=original, output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, native=native, earned_ticks=earned, supplied_stock_or_pose=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests" / ("surface_entry_salt_mine.cpp" if phase == "fleet" else "surface_entry_production.cpp")),
                         identity(Path(__file__).with_name("surface_entry_salt_mine_checks-4.py"))])


def main():
    """Publish one bounded genuine empty purchase or actual finite Salt haulage interval."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("fleet", "drive"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--require-surface", action="store_true")
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, source, args.phase, args.require_surface)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), surface=result["native"]["surface_custody"],
                          held=result["native"]["wagon"]["held"], reserve=result["native"]["mine"]["reserve"])), flush=True)


if __name__ == "__main__":
    main()
