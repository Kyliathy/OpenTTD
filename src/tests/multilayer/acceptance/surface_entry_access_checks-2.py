"""Earn native construction time for actual paid spoil receivers without fabricating tile stages.

@file surface_entry_access_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.2.
Public native StateGameLoop owns exactly3000earned ticks per process. Original
named industry/map save fields independently prove every receiver cell's stage.
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

OWNERS = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))


def receivers(path, layout):
    """Read every actual machinery cell from the public native save format.

    @param path Actual complete native checkpoint.
    @param layout Original paid infrastructure geometry, used only to select its actual new industries.
    @return Native founder/stock/readiness and every actual construction stage.
    """
    fields = OWNERS["saved_table"](path, b"MAPS")[0]
    width = fields["dim_x"]
    data = chunks(path)[1]
    # Current RIFF owners retain their three-byte payload lengths. MAPT stores
    # TileType in its upper nibble, MAP2 owns native industry IDs, and MAPO is
    # m1: industry_map.h owns completion bit7/stage bits0..1/counter bits2..3.
    types, owners, ids = [data[key][3:] for key in (b"MAPT", b"MAPO", b"MAP2")]
    assert len(types) == len(owners) and len(ids) == len(types) * 2
    industries = OWNERS["saved_table"](path, b"INDY")
    result = []
    for kind in ("earth", "rock"):
        industry, = [key for key, value in industries.items() if value["location.tile"] == layout[kind]]
        value = industries[industry]
        assert value["founder"] == 0
        cells = []
        # Native industry rectangles may have holes. Only type/identity-matched
        # machinery cells belonging to this exact funded receiver can establish
        # readiness; a neighbouring industry or empty cell cannot count.
        for y in range(value["location.h"]):
            for x in range(value["location.w"]):
                tile = value["location.tile"] + y * width + x
                if types[tile] >> 4 != 8:
                    continue
                if int.from_bytes(ids[tile * 2:tile * 2 + 2], "big") != industry:
                    continue
                cells.append(dict(tile=tile, complete=bool(owners[tile] & 128), stage=owners[tile] & 3, counter=owners[tile] >> 2 & 3))
        assert cells
        assert all(amount == 0 for name, amount in value.items() if name.endswith(".waiting"))
        result.append(dict(kind=kind, industry=industry, type=value["type"], founder=value["founder"],
                           construction_date=value["construction_date"], cells=cells, complete=all(cell["complete"] for cell in cells)))
    return result


def execute(configuration, source, infrastructure, require_ready):
    """Earn genuine exact native construction work and preserve paid products/financial conservation.

    @param configuration Independently linked focused application containing the actual native game loop.
    @param source Exact funded infrastructure or its immediately preceding actual construction output.
    @param infrastructure Original paid infrastructure membership, selecting actual receiver identities.
    @param require_ready Require both original receivers to have earned complete construction.
    @return Actual native process, exact clocks, original products and complete original cell states.
    """
    accepted = json.loads(infrastructure.read_text())
    assert accepted["complete"] and accepted["phase"] == "infrastructure" and accepted["configuration"] == configuration
    layout = accepted["operation"]["layout"]
    initial = OWNERS["observe"](source)
    initial_cells = receivers(source, layout)
    context = create_run_context("space", "surface-entry-access-construction")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_production\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "construction.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-production]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_ENTRY_REPLAY_COMPLETE" in terminal, log
    assert identity(source) == original
    output = context.saves / "surface-entry-replayed-440.sav"
    final = OWNERS["observe"](output)
    assert initial["debt"] == final["debt"] == 3500000
    assert initial["cash"] - final["cash"] == final["expense"] - initial["expense"]
    assert initial["parked_steel"] == final["parked_steel"] == 36
    assert initial["parked_goods"] == final["parked_goods"] == 14
    first, last = [OWNERS["saved_table"](path, b"DATE")[0] for path in (source, output)]
    assert last["tick_counter"] - first["tick_counter"] == 3000
    assert first["id"] == last["id"]
    before, after = chunks(source)[1], chunks(output)[1]
    for owner in (b"GEOS", b"GDEP", b"GSPN", b"GVNT"):
        assert before[owner] == after[owner], owner
    final_cells = receivers(output, layout)
    if require_ready:
        assert all(row["complete"] for row in final_cells)
    return dict(source=original, output=identity(output), infrastructure=identity(infrastructure), application=identity(application), terminal=identity(log),
                initial=initial, final=final, initial_cells=initial_cells, final_cells=final_cells, earned_ticks=3000,
                supplied_progress_or_floor=False, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_production.cpp")])


def main():
    """Run one bounded actual construction interval and retain its exact original membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--infrastructure", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--require-ready", action="store_true")
    args = parser.parse_args()
    source, infrastructure, target = [(ROOT / item).resolve() for item in (args.source, args.infrastructure, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, source, infrastructure, args.require_ready)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), receivers_ready=all(row["complete"] for row in result["final_cells"]))), flush=True)


if __name__ == "__main__":
    main()
