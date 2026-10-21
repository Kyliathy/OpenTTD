"""Query real Town1 return passengers on the existing extra bus without changing service.

@file surface_entry_return_passenger_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGX.
The actual packet offer, native two-order estimates and whole company bill
decide admission. Historical SourceTown0passenger defaults remain strict.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_factory_goods_liquidity_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-return-passenger-query"


def execute(configuration,receipt,recover_run=None):
    """Run pure native return-pickup admission against the actual stopped-recovery endpoint.

    @param configuration Independently linked focused native application.
    @param receipt Exact preceding actual productive-recovery window04receipt.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Actual Town1stock, routes, order prices and complete funding admission/refusal.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASE"]
    assert receipt.name == f"surface-entry-productive-reserve-work-04-{configuration}.json"
    assert previous["operation"]["final"]["company"]["cash"] == 54619
    files = [ROOT / r["path"] for r in previous["operation"]["sources"]] + [Path(__file__),
        ROOT / "src/tests/surface_entry_return_passenger.h",ROOT / "src/tests/surface_entry_intertown.h",ROOT / "src/order_cmd.cpp"]
    operation = native(configuration,PHASE,receipt,observe,
        dict(source="RETURN_PASSENGER_SOURCE",orders="RETURN_PASSENGER_ORDER",admission="RETURN_PASSENGER_ADMISSION"),
        files,recover_run=recover_run,chemical_limit=2000)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before,output = [ROOT / operation[k]["path"] for k in ("before","output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["vehicle"] == 9 and source["engine"] == 119 and source["capacity"] == 40 and source["held"] == 0
    assert source["outbound"] == 17 and source["outbound_town"] == 0 and source["return_source"] == 20 and source["return_town"] == 1
    assert source["forward"] == source["reverse"] > 0 and source["purchases"] == source["ticks"] == 0
    station,packets = TABLE(output,b"STNN")[20],TABLE(output,b"CAPA")
    refs = [ref for key,values in station.items() if key.startswith("normal[0].goods[0].cargo[") and key.endswith(".second") for ref in values]
    actual = [dict(packet=ref-1,**packets[ref-1]) for ref in refs]
    assert sum(p["count"] for p in actual) == source["physical"]
    assert source["exact_source"] == all(p["source_type"] == 1 and p["source_id"] == 1 and p["source"] == 20 for p in actual)
    assert 0 <= source["available"] <= source["physical"]
    orders = operation["rows"]["orders"]
    assert [r["kind"] for r in orders] == ["pickup20","unload17"] and all(r["paid"] == 0 for r in orders)
    cases = operation["rows"]["admission"]
    assert [r["reserve"] for r in cases] == [100000,40000] and not cases[0]["funded"]
    for row in cases:
        assert row["ticks"] == 2000 and row["target"] == row["operating"]+row["reserve"] and row["cash"] == 54619
        assert row["invoice"] == row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
        assert row["complete"] == source["complete"] and row["funded"] == (row["cash"] >= row["target"])
    operation["return_passenger_admitted"] = bool(source["complete"] and cases[1]["funded"])
    operation["actual_return_offer"] = actual
    return operation


def main():
    """Publish one immutable pure actual return-passenger admission or refusal."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration,ROOT / args.receipt,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,phase=PHASE,operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
