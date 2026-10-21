"""Price actual existing-road farm joins and one native Livestock carrier.

@file surface_entry_farm_road_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGM.
Tile-specific ordinary authority decides each join. Station15already owns real
Farm7inventory; this query supplies neither civic permission nor more cargo.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_road_shortcut_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-farm-road-query"


def execute(configuration, receipt, recover_run=None):
    """Run a pure actual join/model/whole operating admission without buying any prerequisite.

    @param configuration Independently linked focused native application.
    @param receipt Exact immediately preceding unchanged native road-scope refusal.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Real local authority, physical source stock, native invoices/lives and literal funding result.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASE"]
    assert not previous["operation"]["road_producer_admitted"]
    files = BASE["FREIGHT"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_road_shortcut_checks-1.py",
        DIRECTORY / "surface_entry_intertown_checks-1.py", DIRECTORY / "surface_entry_farm_cycle_checks-1.py",
        DIRECTORY / "surface_entry_farm_rebalance_checks-1.py", ROOT / "src/tests/surface_entry_farm_road.h",
        ROOT / "src/tests/surface_entry_goods_income.h", ROOT / "src/tests/surface_entry_intertown.h",
        ROOT / "src/tests/surface_entry_module_capital.h", ROOT / "src/tests/surface_entry_income_capitalization.h",
        ROOT / "src/engine.cpp", ROOT / "src/engine_base.h", ROOT / "src/town_cmd.cpp", ROOT / "src/station_cmd.cpp"]
    operation = native(configuration, PHASE, receipt, observe,
        dict(source="FARM_ROAD_SOURCE", stops="FARM_ROAD_STOP", models="FARM_ROAD_MODEL", admission="FARM_ROAD_ADMISSION"),
        files, recover_run=recover_run, chemical_limit=2000)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    stops = operation["rows"]["stops"]
    assert [row["station"] for row in stops] == [15, 13]
    assert source["source"] == 15 and source["receiver"] == 13 and source["industry"] == 7 and source["accepts"]
    assert source["purchases"] == source["changed_roads"] == source["changed_rating"] == 0
    assert all(row["road_built"] == row["permission_granted"] == 0 for row in stops)
    station, packets = TABLE(output, b"STNN")[15], TABLE(output, b"CAPA")
    prefix = "normal[0].goods[4]."
    refs = [ref for key, values in station.items() if key.startswith(prefix + "cargo[") and key.endswith(".second") for ref in values]
    cargo = [dict(packet=ref - 1, **packets[ref - 1]) for ref in refs]
    assert all(row["source_type"] == 0 and row["source_id"] == 7 for row in cargo)
    physical = sum(row["count"] for row in cargo)
    # A native loading reservation may reduce AvailableCount while its packets
    # remain physically station-owned. Neither quantity is a funding credit.
    assert 0 <= source["waiting"] <= physical
    operation["physical_farm_offer"] = dict(total=physical, available=source["waiting"], packets=cargo, income_counted=False)
    cases = operation["rows"]["admission"]
    if source["complete"]:
        assert all(row["found"] for row in stops)
        model, = [row for row in operation["rows"]["models"] if row["engine"] == source["engine"]]
        assert model["available"] and model["capacity"] == source["capacity"] > 0 and model["life_days"] > 28
        assert [row["reserve"] for row in cases] == [100000, 40000] and not cases[0]["funded"]
        for row in cases:
            assert row["ticks"] == 2000 and row["vehicle_invoice"] == model["invoice"]
            assert row["stops"] == sum(stop["invoice"] for stop in stops)
            assert row["gross"] == row["stops"] + row["vehicle_invoice"]
            assert row["extra_running"] >= 2 and row["target"] == row["reserve"] + row["gross"] + row["operating"] + row["extra_running"]
            assert row["cash"] == 73667 and row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
        operation["farm_road_producer_admitted"] = bool(cases[1]["funded"])
    else:
        assert not cases
        operation["farm_road_producer_admitted"] = False
    return operation


def main():
    """Publish one immutable native actual farm-road prerequisite admission or refusal."""
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
                                     phase=PHASE, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
