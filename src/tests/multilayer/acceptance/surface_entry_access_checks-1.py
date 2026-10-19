"""Verify native banking, pure joint layout and actual Surface access infrastructure.

@file surface_entry_access_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.2.
Every process consumes the exact authentic game. Information never substitutes
for native placement, ordinary command finance or earned underground work.
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


def execute(configuration, source, phase):
    """Execute exactly one genuine access credit, pure layout or paid infrastructure phase.

    @param configuration Independently linked focused application.
    @param source Exact ordinary productive input or its actual matching-bank-credit output.
    @param phase Native operation, with no automatic retry on already borrowed/funded outputs.
    @return Exact native process, owners, financial receipts and actual selected geometry.
    """
    context = create_run_context("space", "surface-entry-access-" + phase)
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_access\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "access.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-access]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_ACCESS_PHASE=phase))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and f"SURFACE_ACCESS_COMPLETE phase={phase}" in terminal, log
    assert identity(source) == original
    before = context.saves / f"surface-access-before-{phase}-440.sav"
    output = context.saves / f"surface-access-{phase}-440.sav"
    first, final = [OWNERS["observe"](path) for path in (before, output)]
    # Original products, provenance, native hulls, production and income cannot
    # change during paused credit, layout or construction commands. Only the
    # native finance-owned scalars may change where that phase authorizes them.
    for name, value in first.items():
        if name not in ("cash", "debt", "expense"):
            assert final[name] == value, name
    assert first["parked_steel"] == final["parked_steel"] == 36
    assert first["parked_goods"] == final["parked_goods"] == 14
    start, end = chunks(before)[1], chunks(output)[1]
    layout = None
    if phase == "plan":
        assert start == end
    elif phase == "credit":
        query = context.saves / "surface-access-credit-query-440.sav"
        assert chunks(before) == chunks(query)
        assert {key for key in start if start[key] != end[key]} == {b"PLYR"}
        a, = OWNERS["saved_table"](before, b"PLYR").values()
        b, = OWNERS["saved_table"](output, b"PLYR").values()
        for name, value in a.items():
            assert b[name] == (2199555 if name == "money" else 3500000 if name == "current_loan" else value), name
        assert final["expense"] == first["expense"]
    else:
        assert first["debt"] == final["debt"] == 3500000
        invoices = [int(value) for value in re.findall(r"SURFACE_ACCESS_INVOICE command=\w+ tile=\d+ cost=(-?\d+)", terminal)]
        assert first["cash"] - final["cash"] == final["expense"] - first["expense"] == sum(invoices)
        old, new = [OWNERS["saved_table"](path, b"INDY") for path in (before, output)]
        assert len(new) == len(old) + 2
        for key, value in old.items():
            assert new[key] == value
        assert len(OWNERS["saved_table"](output, b"STNN")) == 5
        assert len(OWNERS["saved_table"](output, b"DEPT")) == 2
        assert "source_proof=true solid_pocket=9" in terminal
        for family in (b"GEOS", b"GDEP", b"GSPN", b"GVNT"):
            assert start[family] == end[family], family
    if phase != "credit":
        portal, shaft, station, axis, distance, depot, earth, rock, capital = re.findall(
            r"SURFACE_ACCESS_PLAN portal=(\d+) shaft=(\d+) station=(\d+) axis=(\d+) distance=(\d+) depot=(\d+) earth=(\d+) rock=(\d+) capital=(\d+)", terminal)[0]
        layout = dict(zip(("portal", "shaft", "station", "axis", "distance", "depot", "earth", "rock", "capital"),
                          map(int, (portal, shaft, station, axis, distance, depot, earth, rock, capital)), strict=True))
    a, b = [OWNERS["saved_table"](path, b"DATE")[0] for path in (before, output)]
    # Real funding draws original appearance RNG. It still grants no game tick,
    # calendar work, identity, geology reserve or underground excavation state.
    for name, value in a.items():
        if not name.startswith("random_state["):
            assert b[name] == value, name
    return dict(source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                layout=layout, initial=first, final=final, simulation_ticks=0, supplied_stock_or_floor=False,
                snapshots=[identity(path) for path in sorted(context.saves.glob("*.sav"))], retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_access.cpp")])


def main():
    """Select one exact authentic input and retain a fresh bounded native access receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("credit", "plan", "infrastructure"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase,
                                     operation=execute(args.configuration, source, args.phase)), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
