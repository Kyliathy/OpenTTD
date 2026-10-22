"""Query actual direct Factory5Goods offers, existing roads and complete operating admission.

@file surface_entry_factory_goods_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGT.
Current physical stock supplies no cash. The actual two paid commercial
carriers, native order quotes and unchanged company books remain authoritative.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_feeder_rail_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-factory-goods-query"


def execute(configuration, receipt, recover_run=None):
    """Run pure native direct-trade admission with all original families unchanged.

    @param configuration Independently linked focused native application.
    @param receipt Exact preceding actual native rail-followthrough receipt.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Actual stock/orders/routes and whole funding admission or literal refusal.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASE"]
    files = [ROOT / row["path"] for row in previous["operation"]["sources"]] + [Path(__file__), ROOT / "src/tests/surface_entry_factory_goods.h"]
    operation = native(configuration, PHASE, receipt, observe,
        dict(source="FACTORY_GOODS_SOURCE", orders="FACTORY_GOODS_ORDER", vehicles="FACTORY_GOODS_VEHICLE", admission="FACTORY_GOODS_ADMISSION"),
        files, recover_run=recover_run, chemical_limit=2000)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["station"] == 13 and source["industry"] == 5 and source["receiver"] == 20
    assert source["tile"] == 2572 and source["receiver_tile"] == 731 and source["purchases"] == source["ticks"] == 0
    station, packets = TABLE(output, b"STNN")[13], TABLE(output, b"CAPA")
    refs = [ref for key, values in station.items() if key.startswith("normal[0].goods[5].cargo[") and key.endswith(".second") for ref in values]
    actual = [dict(packet=ref-1, **packets[ref-1]) for ref in refs]
    assert sum(p["count"] for p in actual) == source["physical"]
    assert source["exact_source"] == all(p["source_type"] == 0 and p["source_id"] == 5 and p["source"] == 13 for p in actual)
    assert 0 <= source["available"] <= source["physical"]
    orders = operation["rows"]["orders"]
    vehicles = operation["rows"]["vehicles"]
    assert [(r["vehicle"],r["index"],r["destination"]) for r in orders] == [(3,0,13),(3,1,20),(25,0,13),(25,1,20)]
    assert all(r["non_stop"] and r["paid"] == 0 for r in orders)
    assert [r["vehicle"] for r in vehicles] == [3,25]
    assert all(r["engine"] == 140 and r["capacity"] == 18 and r["finish_loaded_at"] == 20 and r["changed"] == 0 for r in vehicles)
    cases = operation["rows"]["admission"]
    assert [r["reserve"] for r in cases] == [100000,40000] and not cases[0]["funded"]
    # Whole current costs and cash decide funding. Real offers and distance
    # comparisons never become projected fares or a present spending credit.
    for row in cases:
        assert row["ticks"] == 2000 and row["invoice"] == row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
        assert row["target"] == row["reserve"] + row["operating"] and row["cash"] == 54793
        assert row["funded"] == (row["cash"] >= row["target"]) and row["complete"] == source["complete"]
    operation["factory_goods_admitted"] = bool(source["complete"] and cases[1]["funded"])
    operation["actual_source_offer"] = actual
    operation["static_route_shorter"] = source["distance"] < source["old_distance"]
    return operation


def main():
    """Publish one immutable pure actual Factory5trade admission or refusal."""
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
